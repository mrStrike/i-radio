/* ============================================================================
 *  LCDWiki ES3C28P — Internet Radio Panel v6
 *  - Автоподключение к сохранённой сети при старте (без автопортала)
 *  - Портал Wi-Fi только по кнопке
 *  - Исправлен краш на Хит-ФМ (стек аудио 20 КБ)
 * ==========================================================================*/
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_http_server.h>
#include <Preferences.h>
#include <WiFiManager.h>
#include <LovyanGFX.hpp>
#include <lvgl.h>
#include <Audio.h>

// Resume API differs between audioI2S 3.x (bytes) and 4.x (seconds).
// This compatibility check is needed even when diagnostics are removed.
#if __has_include("audiolib_structs.hpp") && __has_include("psram_unique_ptr.hpp")
#define AUDIO_RESUME_SECONDS 1
#else
#define AUDIO_RESUME_SECONDS 0
#endif
#include <esp_heap_caps.h>
#include <esp_system.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <esp_log.h>
#include <LittleFS.h>
#include <SD_MMC.h>
#include <vector>
#include <algorithm>
#include <math.h>
#include <atomic>
#include <time.h>
#include <string.h>
#include "stations.h"
#include "station_catalog.h"

// Explicit prototype prevents Arduino from inserting it before RadioStation is known.
static const RadioStation* selectedStation(int n);

// Данные информационного экрана объявлены здесь для переноса в .ino.
// Short bounded copies only; no GUI calls or allocation in audio callbacks.
static portMUX_TYPE nowDataMux = portMUX_INITIALIZER_UNLOCKED;
static char nowTrack[256]{}, nowBroadcast[128]{}, nowDescription[256]{}, nowWebsite[192]{};
static std::atomic<uint32_t> nowRate{0}, nowBitrate{0};
static std::atomic<uint8_t> nowChannels{0};
static std::atomic<bool> startupScreenReady{false};
static void storeNowText(char* destination, size_t capacity, const char* text) {
    portENTER_CRITICAL(&nowDataMux);
    strlcpy(destination, text ? text : "", capacity);
    portEXIT_CRITICAL(&nowDataMux);
}
static void clearNowData() {
    portENTER_CRITICAL(&nowDataMux);
    nowTrack[0] = nowBroadcast[0] = nowDescription[0] = nowWebsite[0] = 0;
    portEXIT_CRITICAL(&nowDataMux);
    nowRate = 0; nowBitrate = 0; nowChannels = 0;
}

LV_FONT_DECLARE(dejavu_16);
constexpr int STATION_COUNT = FAVORITES_COUNT;
// Сохраняю распиновку встроенного звукового тракта: разрешение усилителя и линии цифрового звука I2S.
constexpr int PA_EN = 1, MCLK = 4, BCLK = 5, DOUT = 8, LRCK = 7;
constexpr int TP_SDA = 16, TP_SCL = 15, TP_RST = 18, TP_INT = 17, TFT_BL = 45;
// Кодек и сенсор используют одну I2C-шину. WireGuard берёт mutex при входе и освобождает при выходе из области видимости.
static SemaphoreHandle_t wireDiagMutex;
struct WireGuard {
    WireGuard() {
        if (wireDiagMutex) xSemaphoreTake(wireDiagMutex, portMAX_DELAY);
    }
    ~WireGuard() {
        if (wireDiagMutex) xSemaphoreGive(wireDiagMutex);
    }
};
// При невосстановимой ошибке отключаю усилитель и останавливаю выполнение. Не продолжаю запуск с отсутствующей памятью или неготовым оборудованием.
static void fatal(const char* text) {

    digitalWrite(PA_EN, HIGH);
    for (;;) vTaskDelay(pdMS_TO_TICKS(1000));
}
static const uint32_t tileColors[STATION_COUNT] = {
    0xC62828, 0x1565C0, 0x6A1B9A, 0x2E7D32
};
// Здесь задаю подключение встроенного ILI9341 к SPI. Сохраняю проверенные частоты, выводы и инверсию; саму последовательность запуска панели выполняет LovyanGFX.
class LGFX : public lgfx::LGFX_Device {
    lgfx::Bus_SPI bus;
    lgfx::Panel_ILI9341 panel;
    public:
    LGFX() {
        auto b = bus.config();
        b.spi_host = SPI2_HOST;
        b.freq_write = 40000000;
        b.freq_read = 16000000;
        b.spi_3wire = false;
        b.use_lock = true;
        b.dma_channel = SPI_DMA_CH_AUTO;
        b.pin_sclk = 12;
        b.pin_mosi = 11;
        b.pin_miso = 13;
        b.pin_dc = 46;
        bus.config(b);
        panel.setBus(&bus);
        auto p = panel.config();
        p.pin_cs = 10;
        p.pin_rst = -1;
        p.pin_busy = -1;
        p.memory_width = p.panel_width = 240;
        p.memory_height = p.panel_height = 320;
        p.invert = true;
        panel.config(p);
        setPanel(&panel);
    }
};
LGFX gfx;
// Команды направляю из GUI в аудио; статусы и метаданные — обратно. Поле value содержит индекс станции или значение команды.
QueueHandle_t audioCmdQueue, uiEventQueue;
enum CommandType {
    CMD_PLAY_FAV_0, CMD_PLAY_FAV_1, CMD_PLAY_FAV_2, CMD_PLAY_FAV_3, CMD_PLAY_ALL, CMD_STOP, CMD_SET_VOLUME, CMD_TEST_TONE, CMD_TOGGLE
};
struct Command {
    CommandType type;
    uint8_t value;
};
// Explicit prototype prevents Arduino .ino preprocessing from placing it before Command.
static void sendCommand(Command c);
enum UiEventType {
    UI_STATUS, UI_METADATA
};
struct UiEvent {
    UiEventType type;
    char text[96];
};
// Передаю новый статус аудиозадачи в GUI через очередь длиной один элемент. Новый статус заменяет прежний: интерфейсу нужен актуальный текст, а не история сообщений.
static void pushStatus(const char* text) {
    UiEvent e{
    };
    e.type = UI_STATUS;
    strlcpy(e.text, text, sizeof(e.text));
    xQueueOverwrite(uiEventQueue, &e);

}
// Передаю метаданные тем же механизмом очереди. Сейчас GUI их принимает, но не выводит название композиции на экран.
static void pushMetadata(const char* text) {
    UiEvent e{
    };
    e.type = UI_METADATA;
    strlcpy(e.text, text, sizeof(e.text));
    xQueueOverwrite(uiEventQueue, &e);

}
/* ---------- ES8311 ---------- */
// Записываю одну пару «регистр — значение» в ES8311 по адресу 0x18. Блокировка защищает общую I2C-шину от одновременного обращения сенсора и кодека; ошибка возвращается вызывающему коду.
static bool codecWrite(uint8_t r, uint8_t v) {
    WireGuard guard;
    Wire.beginTransmission(0x18);
    Wire.write(r);
    Wire.write(v);
    return Wire.endTransmission() == 0;
}
// Запускаю ES8311 двумя проверенными таблицами регистров и задержками. Порядок записей и значения сохраняю без изменений: это рабочая настройка звукового тракта, а не оформление интерфейса.
static bool es8311_init() {
    const uint8_t seq[][2] = {
        {
            0x01,0x30
        },{
            0x02,0
        },{
            0x03,0x10
        },{
            0x16,0x24
        },{
            0x04,0x10
        },{
            0x05,0
        },{
            0x0b,0
        },{
            0x0c,0
        },{
            0x10,0x1f
        },{
            0x11,0x7f
        },{
            0x00,0x80
        }
    };
    for (auto &r : seq) if (!codecWrite(r[0], r[1])) return false;
    delay(20);
    const uint8_t run[][2] = {
        {
            0x00,0
        },{
            0x01,0x3f
        },{
            0x02,0
        },{
            0x03,0x10
        },{
            0x04,0x10
        },{
            0x05,0
        },{
            0x06,3
        },{
            0x07,0
        },{
            0x08,0xff
        },{
            0x09,0x0c
        },{
            0x0a,0x4c
        },{
            0x13,0x10
        },{
            0x1b,0x0a
        },{
            0x1c,0x6a
        },{
            0x0e,2
        },{
            0x12,0
        },{
            0x14,0x10
        },{
            0x0d,1
        },{
            0x15,0
        },{
            0x37,0x48
        },{
            0x45,0
        },{
            0x31,0
        },{
            0x32,0xbf // 0 dB: восстановлен прежний уровень
        }
    };
    for (auto &r : run) if (!codecWrite(r[0], r[1])) return false;
    if (!codecWrite(0x00, 0x80)) return false;
    delay(20);

    return true;
}
/* ---------- Тач ---------- */
// Читаю пять байтов сенсора по I2C-адресу 0x38. Проверяю число касаний, собираю координаты из старших и младших битов и преобразую их в систему координат экрана 240 × 320.
static bool readTouch(int16_t& x, int16_t& y) {
    WireGuard guard;
    Wire.beginTransmission(0x38);
    Wire.write(0x02);
    if (Wire.endTransmission(false)) return false;
    if (Wire.requestFrom(uint8_t(0x38), uint8_t(5)) != 5) {
        while (Wire.available()) Wire.read();
        return false;
    }
    uint8_t r[5];
    for (auto &v : r) v = Wire.read();
    if (!(r[0] & 15)) return false;
    // Pixel coordinates are 0..239 and 0..319: mirror around the last pixel.
    x = constrain(239 - int(((r[1] & 15) << 8) | r[2]), 0, 239);
    y = constrain(319 - int(((r[3] & 15) << 8) | r[4]), 0, 319);
    return true;
}
/* ---------- LVGL ---------- */
static lv_display_t* disp = nullptr;
static lv_indev_t* indev = nullptr;
static constexpr int DMA_BLOCK_ROWS = 8;
static uint16_t* dmaLine = nullptr;
// Храню указатели на один раз созданные экраны и элементы. Обращаюсь к объектам LVGL из GUI, не из аудиозадачи.
static lv_obj_t* mainScreen = nullptr;
static lv_obj_t* listScreen = nullptr;
static lv_obj_t* settingsScreen = nullptr;
static lv_obj_t* debugScreen = nullptr;
static lv_obj_t* galleryScreen = nullptr;
static bool photoViewing=false;
static int pendingPhotoSwipe=0; // 1 next, -1 previous, 2 list
static lv_obj_t* pendingScreenSwipe = nullptr;
static bool swipeMovesLeft = true;
static uint32_t screenSlideUntil = 0;
// Animate resident screens without snapshots or deleting their controls.
static void slideToScreen(lv_obj_t* screen, bool movesLeft) {
    if (!screen || screen == lv_screen_active()) return;
    screenSlideUntil = millis() + 130;
    lv_screen_load_anim(screen, movesLeft ? LV_SCREEN_LOAD_ANIM_MOVE_LEFT : LV_SCREEN_LOAD_ANIM_MOVE_RIGHT,
                        90, 0, false);
}
static lv_obj_t* alarmScreen = nullptr;
static bool scheduleTimePopupOpen = false;
static lv_obj_t* remoteAddressLabel = nullptr;
static lv_obj_t* wifiScreen = nullptr;
static lv_obj_t *localWifiScreen = nullptr, *localWifiEditor = nullptr;
static lv_obj_t* volumeSlider = nullptr;
static lv_obj_t* volumeLabel = nullptr;
static lv_obj_t* globalVolumeButton=nullptr;
static lv_obj_t *volumeOverlay = nullptr, *popupVolumeSlider = nullptr, *popupVolumeLabel = nullptr;
static bool sdFolderPopupOpen=false;
static bool volumePopupOpen = false;
static uint32_t volumePopupActivity = 0;

static lv_obj_t* statusLabel = nullptr;
static lv_obj_t* brightnessLabel = nullptr;
static lv_obj_t* wifiIndicator = nullptr;
static lv_obj_t* wifiPixels[160] = {
};
static unsigned wifiPixelCount = 0;
static lv_obj_t* wifiSlash = nullptr;
static lv_obj_t* batteryIndicator = nullptr;
static lv_obj_t* batteryBars[4] = {
};
static lv_obj_t* batteryUnknown = nullptr;
static lv_obj_t* tileBtns[STATION_COUNT];
static lv_obj_t* tileLabels[STATION_COUNT];
static char currentStatus[96] = "Нажмите плитку";
// Копию статуса для веб-пульта защищаю отдельно: HTTP не обращается к LVGL.
static portMUX_TYPE webStatusMux = portMUX_INITIALIZER_UNLOCKED;
static char webStatus[96] = "Нажмите плитку";
static int pendingPlayingSwipe = 0;
static bool pendingLocalWifiSwipe = false;
static volatile int currentStation = -1;
static volatile bool isPlaying = false;
static volatile bool testToneActive = false;
static lv_obj_t* toneLabel = nullptr;
// Атомарные значения передаю между задачами без обращения к объектам LVGL. Громкость: 0–21, яркость: 15–100 %, sleepChoice: индекс списка задержек.
static std::atomic<int> currentVolume{
    6
};
static std::atomic<int> currentBrightness{
    100
};
static std::atomic<int> sleepChoice{
    0
};
static const uint32_t sleepMinutes[] = {
    0, 2, 5, 10, 15, 30, 60
};
static const char* sleepNames[] = {
    "0: сон выключен", "1: через 2 минуты", "2: через 5 минут", "3: через 10 минут", "4: через 15 минут", "5: через 30 минут", "6: через час"
};
static lv_obj_t* sleepLabel = nullptr;
static lv_obj_t* brightnessSlider = nullptr;
static lv_obj_t* displaySleepSlider = nullptr;
// Команды настроек исполняю только в GUI-задаче.
struct RemoteSetting { int kind; int value; };
static QueueHandle_t remoteSettingsQueue = nullptr;
static uint32_t lastTouchAt = 0;
static bool displaySleeping = false;
static bool suppressWakeTouch = false;
static void setTileText(lv_obj_t* label, const char* text, int width);
// Храню до 20 индексов; прежние ключи fav0–fav3 сохраняю для совместимости.
static std::atomic<int> favoriteIds[FAVORITES_COUNT];
static lv_obj_t* favoritesPanel = nullptr;
// Спектр: только атомарные значения между аудио и GUI, без вызовов LVGL из аудио.
static constexpr int SPECTRUM_BANDS = 8;
static std::atomic<uint8_t> spectrumLevels[SPECTRUM_BANDS];
static std::atomic<uint32_t> spectrumAt{0};
static std::atomic<uint32_t> spectrumSampleRate{48000};
static std::atomic<uint8_t> spectrumChannels{2};
static lv_obj_t* spectrumBars[SPECTRUM_BANDS]{};
// Заполнение измеряется только аудиозадачей; GUI читает готовую долю.
static std::atomic<uint16_t> inputBufferPermille{0};
static lv_obj_t* inputBufferFill = nullptr;
static constexpr int INPUT_BUFFER_BAR_WIDTH = 240;
static uint32_t favoritesRevision = 0;
static void rebuildFavoriteTiles();
static std::atomic<bool> favoritesDirty{
    false
};
static std::atomic<uint32_t> radioDeadline{
    0
};
static std::atomic<bool> wantPlayback{
    false
};
static lv_obj_t* clockLabel = nullptr;
static lv_obj_t* radioCountdownLabel = nullptr;
static lv_obj_t* timerLabel = nullptr;
static lv_obj_t* radioTimerSlider = nullptr;
static lv_obj_t* favoriteChecks[MAX_STATIONS]{
};
static const int radioMinutes[] = {
    0,15,30,60,90
};
static int radioTimerChoice = 0;
// Выбираю время остановки радио на текущий сеанс. Ноль отменяет таймер; иначе сохраняю момент окончания в миллисекундах. Саму остановку выполняет аудиозадача, а не обработчик LVGL.
static void radioTimerEvent(lv_event_t* e) {
    radioTimerChoice = lv_slider_get_value((lv_obj_t*)lv_event_get_target(e));
    int minutes = radioMinutes[radioTimerChoice];
    radioDeadline.store(minutes ? millis() + minutes * 60000UL : 0);
    char b[64];
    if(minutes) snprintf(b,sizeof(b),"Радио: стоп через %d мин",minutes);
    else snprintf(b,sizeof(b),"Таймер радио выключен");
    lv_label_set_text(timerLabel,b);
}
// Меняю избранное без запуска звука. При заполнении 20 мест отклоняю добавление и согласовываю чекбоксы.
static void changeFavorite(int id, bool requested) {
    if(id < 0 || id >= ALL_STATIONS_COUNT) return;
    int slot = -1, empty = -1;
    for(int j=0;j<FAVORITES_COUNT;j++) {
        int n = favoriteIds[j].load();
        if(n == id) slot = j;
        if(n < 0 && empty < 0) empty = j;
    }
    bool changed = false;
    if(!requested && slot >= 0) {
        // Clear every matching slot, including duplicates in older saved data.
        for(int j=0;j<FAVORITES_COUNT;j++) if(favoriteIds[j].load() == id) favoriteIds[j] = -1;
        changed = true;
    } else if(requested && slot < 0 && empty >= 0) {
        favoriteIds[empty] = id;
        changed = true;
    }
    // Согласовываю чекбоксы, в том числе после отказа при заполненном списке.
    for(int i=0;i<ALL_STATIONS_COUNT;i++) {
        if(!favoriteChecks[i]) continue;
        bool selected = false;
        for(int j=0;j<FAVORITES_COUNT;j++) if(favoriteIds[j].load() == i) selected = true;
        if(selected) lv_obj_add_state(favoriteChecks[i], LV_STATE_CHECKED);
        else lv_obj_remove_state(favoriteChecks[i], LV_STATE_CHECKED);
    }
    if(!changed) return;
    favoritesDirty = true;
    rebuildFavoriteTiles();
}
static void favoriteEvent(lv_event_t* e) {
    changeFavorite(int(uintptr_t(lv_event_get_user_data(e))),
        lv_obj_has_state((lv_obj_t*)lv_event_get_target(e), LV_STATE_CHECKED));
}
static Preferences radioPrefs;
static int savedStation = -1;
static String savedUrl;
static lv_obj_t* nowTile = nullptr;
static lv_obj_t* nowLabel = nullptr;
static lv_obj_t* nowIcon = nullptr;
static lv_obj_t* favIcons[STATION_COUNT];
static lv_obj_t* listIcons[MAX_STATIONS];
static lv_obj_t* wifiConfirm = nullptr;
// Преобразую внутренний номер в запись станции: 0–19 обозначают места избранного, 100 + индекс — общий список. Для пустого или неизвестного номера возвращаю nullptr.
static const RadioStation* selectedStation(int n) {
    if (n >= 100 && n < 100 + ALL_STATIONS_COUNT) return &ALL_STATIONS[n-100];
    if (n >= 0 && n < STATION_COUNT) return favoriteIds[n]>=0 ? &ALL_STATIONS[favoriteIds[n]] : nullptr;
    return nullptr;
}
// Сравниваю станции по URL, а не по месту на экране. Одна станция может одновременно быть в общем списке и в избранном.
static bool sameStation(int a, int b) {
    const auto* x = selectedStation(a);
    const auto* y = selectedStation(b);
    return x && y && !strcmp(x->url, y->url);
}
// Отправляю команду аудиозадаче без ожидания свободного места. GUI не должен блокироваться на работе звукового тракта; переполнение остаётся в существующей ветке диагностики.
static void sendCommand(Command c) {
    (void)xQueueSend(audioCmdQueue, &c, 0);
}
// Передаю запрос Play/Pause аудиозадаче. Сам объект Audio из обработчика интерфейса не вызываю.
static void toggleEvent(lv_event_t*) {
    sendCommand({
        CMD_TOGGLE, 0
    });
}
// Geometric icons: no dependency on the Cyrillic font's symbol glyphs.
// Собираю Play/Pause из линии и прямоугольников без символов шрифта. Создаю обе формы сразу, а при обновлении только переключаю видимость дочерних объектов.
static lv_obj_t* makePlayIcon(lv_obj_t* parent) {
    lv_obj_t* icon = lv_obj_create(parent);
    lv_obj_remove_style_all(icon);
    lv_obj_set_size(icon, 16, 16);
    lv_obj_clear_flag(icon, static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));
    lv_obj_t* triangle = lv_line_create(icon);
    static const lv_point_precise_t points[] = {
        {
            1,1
        },{
            13,8
        },{
            1,15
        },{
            1,1
        }
    };
    lv_line_set_points(triangle, points, 4);
    lv_obj_set_style_line_color(triangle, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_line_width(triangle, 2, 0);
    for (int i = 0; i < 2; ++i) {
        lv_obj_t* bar = lv_obj_create(icon);
        lv_obj_remove_style_all(bar);
        lv_obj_set_size(bar, 4, 14);
        lv_obj_set_pos(bar, 1 + i*9, 1);
        lv_obj_set_style_bg_color(bar, lv_color_hex(0xffffff), 0);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
        lv_obj_clear_flag(bar, static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));
    }
    return icon;
}
// Показываю треугольник для запуска или две полоски для паузы. Порядок дочерних объектов соответствует makePlayIcon.
static void setPlayIcon(lv_obj_t* icon, bool playing) {
    for (int i = 0; i < 3; ++i) {
        lv_obj_t* child = lv_obj_get_child(icon, i);
        bool visible = i == 0 ? !playing : playing;
        if (visible) lv_obj_remove_flag(child, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(child, LV_OBJ_FLAG_HIDDEN);
    }
}
// Insert breaks only at spaces; a single overlong word is clipped, never split.
// Переношу название только между словами по ширине рабочего шрифта. Длинное одиночное слово не разрываю: лишняя часть обрезается границами подписи.
static void setTileText(lv_obj_t* label, const char* name, int width) {
    String out, line;
    const char* p = name;
    while (*p) {
        const char* end = strchr(p, ' ');
        String word = end ? String(p).substring(0, end-p) : String(p);
        String candidate = line.length() ? line + " " + word : word;
        lv_point_t size;
        lv_text_get_size(&size, candidate.c_str(), &dejavu_16, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
        if (line.length() && size.x > width) {
            out += line + "\n";
            line = word;
        }
        else line = candidate;
        if (!end) break;
        p = end + 1;
    }
    out += line;
    lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_CLIP);
    lv_label_set_text(label, out.c_str());
}
// Создаю обычный объект LVGL и проверяю результат. При отсутствии памяти останавливаю запуск вместо дальнейшего обращения к нулевому указателю.
static lv_obj_t* mustCreate(lv_obj_t* parent, const char* name) {
    lv_obj_t* o = lv_obj_create(parent);
    if (!o) {

        fatal("LVGL OOM");
    }
    return o;
}
// Создаю кнопку с базовым непрозрачным фоном, белым текстом и рабочим шрифтом. Размер и оформление конкретной кнопки задаю при построении экрана.
static lv_obj_t* mustButton(lv_obj_t* parent, const char* name) {
    lv_obj_t* o = lv_button_create(parent);
    if (!o) {

        fatal("LVGL OOM");
    }
    lv_obj_remove_style_all(o);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(o, lv_color_hex(0x404040), 0);
    lv_obj_set_style_text_color(o, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(o, &dejavu_16, 0);
    return o;
}
// Создаю подпись LVGL с проверкой результата. Текст, положение и цвет задаю отдельно на соответствующем экране.
static lv_obj_t* mustLabel(lv_obj_t* parent, const char* name) {
    lv_obj_t* o = lv_label_create(parent);
    if (!o) {

        fatal("LVGL OOM");
    }
    return o;
}
// Передаю изменённую область блоками до 8 строк через внутреннюю DMA-память.
// Жду окончания каждого блока: буфер нельзя менять, пока его читает DMA.
static void flush(lv_display_t* d, const lv_area_t* a, uint8_t* px_map) {
    const uint16_t* pixels = (const uint16_t*)px_map;
    const int w = a->x2 - a->x1 + 1;
    for (int y = a->y1; y <= a->y2; ) {
        const int rows = min(DMA_BLOCK_ROWS, int(a->y2 - y + 1));
        memcpy(dmaLine, pixels, size_t(w) * rows * sizeof(uint16_t));
        gfx.pushImageDMA(a->x1, y, w, rows, dmaLine);
        gfx.waitDMA();
        pixels += w * rows;
        y += rows;
    }
    lv_display_flush_ready(d);
}
// Передаю касание LVGL и сбрасываю счётчик бездействия. Первое касание спящего дисплея только включает подсветку; до отпускания пальца подавляю его, чтобы случайно не нажать кнопку.
static void touchRead(lv_indev_t*, lv_indev_data_t* data) {
    // Replay a short tap as press/release only after ruling out navigation.
    static bool tapReleasePending = false;
    if (tapReleasePending) {
        tapReleasePending = false;
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }
    int16_t x, y;
    bool p = readTouch(x, y);
    if (p) {
        lastTouchAt = millis();
        if (displaySleeping) {
            displaySleeping = false;
            ledcWrite(TFT_BL, currentBrightness.load() * 255 / 100);
            suppressWakeTouch = true;
        }
    }
    if (suppressWakeTouch) {
        data->state = LV_INDEV_STATE_RELEASED;
        if (!p) suppressWakeTouch = false;
        return;
    }
    // Ignore touches during the short transition; require release afterwards.
    if (screenSlideUntil && (int32_t)(millis() - screenSlideUntil) < 0) {
        suppressWakeTouch = p;
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }
    // Classify horizontal motion before release so a swipe cannot click a tile.
    static bool tracking = false, horizontal = false, blockNavigation = false;
    static int startX = 0, startY = 0;
    static lv_obj_t* startScreen = nullptr;
    static bool arbitrate = false, controlTouch = false;
    static uint32_t touchStartedAt = 0;
    static int maxTouchTravel = 0;
    static bool photoTouch = false;
    static int photoDx = 0, photoDy = 0;
    if (p && !tracking) {
        tracking = true; horizontal = false;
        photoTouch = photoViewing && lv_screen_active() == galleryScreen && !volumePopupOpen;
        photoDx = photoDy = 0;
        blockNavigation = (lv_screen_active() == localWifiScreen && localWifiEditor && !lv_obj_has_flag(localWifiEditor, LV_OBJ_FLAG_HIDDEN) && y >= 88) || scheduleTimePopupOpen || photoViewing || sdFolderPopupOpen || volumePopupOpen || (x >= 204 && y <= 38);
        startX = x; startY = y; startScreen = lv_screen_active();
        arbitrate = !blockNavigation && (startScreen == settingsScreen || startScreen == debugScreen || startScreen == localWifiScreen);
        controlTouch = false;
        touchStartedAt = millis();
        maxTouchTravel = 0;
    }
    if (p && tracking) maxTouchTravel = max(maxTouchTravel, max(abs(x - startX), abs(y - startY)));
    // Defer photo taps until release: a swipe must not reveal/click controls.
    // Queue actions for the GUI loop, never decode or change objects here.
    if (tracking && photoTouch) {
        if (p) {
            photoDx = x - startX; photoDy = y - startY;
            data->state = LV_INDEV_STATE_RELEASED;
            return;
        }
        tracking = false; photoTouch = false;
        if (abs(photoDy) >= 45 && abs(photoDy) > abs(photoDx) * 2)
            pendingPhotoSwipe = photoDy < 0 ? 1 : -1;
        else if (photoDx >= 45 && abs(photoDx) > abs(photoDy) * 2)
            pendingPhotoSwipe = 2;
        else if (maxTouchTravel < 12) {
            data->point.x = startX; data->point.y = startY;
            data->state = LV_INDEV_STATE_PRESSED;
            data->continue_reading = true;
            tapReleasePending = true;
            return;
        }
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }
    if (p && volumePopupOpen) volumePopupActivity = millis();
    if (p && tracking && !controlTouch && !blockNavigation && !volumePopupOpen && (startScreen == mainScreen || startScreen == settingsScreen || startScreen == alarmScreen || startScreen == debugScreen || startScreen == galleryScreen || startScreen == localWifiScreen)) {
        int dx = x - startX, dy = y - startY;
        if (!horizontal && abs(dx) >= 45 && abs(dx) > abs(dy) * 2) {
            horizontal = true;
            swipeMovesLeft = dx < 0;
            lv_indev_wait_release(lv_indev_active());
            if (startScreen == mainScreen && dx < 0) pendingPlayingSwipe = 1;
            if (startScreen == mainScreen && dx > 0) pendingScreenSwipe = alarmScreen;
            if (startScreen == debugScreen && dx > 0) pendingPlayingSwipe = -1;
            if (startScreen == debugScreen && dx < 0) pendingScreenSwipe = galleryScreen;
            if (startScreen == alarmScreen && dx < 0) pendingScreenSwipe = mainScreen;
            if (startScreen == alarmScreen && dx > 0) pendingScreenSwipe = settingsScreen;
            if (startScreen == localWifiScreen && dx < 0) pendingScreenSwipe = settingsScreen;
            if (startScreen == settingsScreen && dx < 0) pendingScreenSwipe = alarmScreen;
            if (startScreen == settingsScreen && dx > 0) pendingLocalWifiSwipe = true;
            if (startScreen == galleryScreen && dx > 0) pendingScreenSwipe = debugScreen;
        }
    }
    if (arbitrate && tracking && !horizontal && !controlTouch) {
        if (p) {
            const int dx = x - startX, dy = y - startY;
            // Vertical scrolling stays available. A stationary hold grants the
            // control exclusive ownership, so dragging it cannot change screens.
            if ((abs(dy) >= 12 && abs(dy) > abs(dx)) ||
                (millis() - touchStartedAt >= 180 && abs(dx) < 12 && abs(dy) < 12)) {
                controlTouch = true;
            } else {
                data->state = LV_INDEV_STATE_RELEASED;
                return;
            }
        } else {
            tracking = false;
            // A moving gesture that did not reach a neighbouring screen is not a tap.
            if (maxTouchTravel < 12) {
                data->point.x = startX;
                data->point.y = startY;
                data->state = LV_INDEV_STATE_PRESSED;
                data->continue_reading = true;
                tapReleasePending = true;
                return;
            }
        }
    }
    if (!p) { tracking = false; horizontal = false; }
    if (horizontal) { data->state = LV_INDEV_STATE_RELEASED; return; }
    data->state = p ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    if (p) {
        data->point.x = x;
        data->point.y = y;
    }
}
// Ограничиваю яркость диапазоном 15–100 % и перевожу её в восьмибитное значение PWM. Если дисплей спит, сохраняю новый уровень, но не включаю подсветку.
static void setBrightness(int percent) {
    if (percent < 15) percent = 15;
    if (percent > 100) percent = 100;
    currentBrightness = percent;
    int duty = (percent * 255) / 100;
    if (!displaySleeping) ledcWrite(TFT_BL, duty);

}
// Меняю цвет пиксельного Wi-Fi и видимость перечёркивания только при изменении подключения. Этот значок показывает наличие сети, а не качество аудиопотока.
static void updateWifiIndicator() {
    bool connected = (WiFi.status() == WL_CONNECTED);
    static int previous = -1;
    if (!wifiIndicator || previous == int(connected)) return;
    previous = int(connected);
    lv_color_t color = lv_color_hex(connected ? 0xE0E0E0 : 0x808090);
    for (unsigned i = 0; i < wifiPixelCount; ++i)
    lv_obj_set_style_bg_color(wifiPixels[i], color, 0);
    if (connected) lv_obj_add_flag(wifiSlash, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_remove_flag(wifiSlash, LV_OBJ_FLAG_HIDDEN);
}
// LCDWiki ES3C28P: GPIO9, onboard 1:2 voltage divider.
// No accessible USB/CHRG status signal: do not infer it from battery voltage.
// Оцениваю заряд по напряжению: усредняю восемь измерений и сглаживаю результат. Шкала приблизительная; неправдоподобное измерение показываю знаком вопроса, не объявляю его признаком USB или зарядки.
static void updateBatteryIndicator() {
    static uint32_t lastSample = 0;
    static float filteredMv = 0;
    static int previous = -2;
    if (!batteryIndicator || (lastSample && millis() - lastSample < 5000)) return;
    lastSample = millis();
    uint32_t sum = 0;
    for (int i = 0; i < 8; ++i) sum += analogReadMilliVolts(9);
    float mv = sum / 8.0f * 2.0f;
    // Implausible readings are unknown, not a power-source detector.
    bool valid = mv >= 2800 && mv <= 4350;
    if (valid) filteredMv = filteredMv == 0 ? mv : filteredMv * 0.75f + mv * 0.25f;
    else filteredMv = 0;
    int level = !valid ? -1 : filteredMv < 3500 ? 0 : filteredMv < 3700 ? 1 :
    filteredMv < 3850 ? 2 : filteredMv < 4050 ? 3 : 4;
    if (previous == level) return;
    previous = level;
    lv_color_t color = lv_color_hex(level == 0 ? 0xE05050 : level < 0 ? 0x808090 : 0xE0E0E0);
    lv_obj_set_style_border_color(batteryIndicator, color, 0);
    for (int i = 0; i < 4; ++i) {
        lv_obj_set_style_bg_color(batteryBars[i], color, 0);
        if (i < level) lv_obj_remove_flag(batteryBars[i], LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(batteryBars[i], LV_OBJ_FLAG_HIDDEN);
    }
    if (valid) lv_obj_add_flag(batteryUnknown, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_remove_flag(batteryUnknown, LV_OBJ_FLAG_HIDDEN);
}
// По нажатию избранного выбираю его станцию. Для уже выбранного URL передаю Play/Pause, для другого — запуск из общего списка; пустая плитка ничего не делает.
static void tileEvent(lv_event_t* e) {
    int idx = int(uintptr_t(lv_event_get_user_data(e)));
    int id=favoriteIds[idx];
    if(id<0) return;
    Command c{
        sameStation(currentStation, 100+id) ? CMD_TOGGLE : CMD_PLAY_ALL, uint8_t(id)
    };
    xQueueSend(audioCmdQueue, &c, 0);
}
// Обрабатываю только собственное нажатие кнопки станции, затем возвращаю главный экран. Чекбокс избранного не должен попадать в эту ветку.
// Создаю только заполненные плитки. Номера слотов не уплотняю: удаление не меняет станцию.
static void rebuildFavoriteTiles() {
    if (!favoritesPanel) return;
    const int oldScroll = lv_obj_get_scroll_y(favoritesPanel);
    lv_obj_clean(favoritesPanel);
    const uint32_t colors[] = {0xA200FF, 0xFF0097, 0x00ABA9, 0x8CBF26, 0xA05000, 0xE671B8, 0x1BA1E2, 0xE51400, 0x339933};
    int count = 0;
    for (int i = 0; i < FAVORITES_COUNT; ++i) {
        tileBtns[i] = nullptr;
        tileLabels[i] = nullptr;
        favIcons[i] = nullptr;
        int id = favoriteIds[i].load();
        if (id < 0 || id >= ALL_STATIONS_COUNT) continue;
        lv_obj_t* tile = mustButton(favoritesPanel, "favoriteTile");
        // Три компактных ряда помещаются в видимой области избранного.
        lv_obj_set_pos(tile, (count % 2) * 110, (count / 2) * 54);
        lv_obj_set_size(tile, 104, 48);
        lv_obj_set_style_bg_color(tile, lv_color_hex(colors[count % (sizeof(colors) / sizeof(colors[0]))]), 0);
        lv_obj_set_style_bg_opa(tile, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(tile, 0, 0);
        lv_obj_set_style_radius(tile, 0, 0);
        lv_obj_set_style_pad_all(tile, 0, 0);
        lv_obj_set_style_shadow_width(tile, 0, 0);
        lv_obj_remove_flag(tile, LV_OBJ_FLAG_SCROLLABLE);
        // SHORT_CLICKED не отправляется после распознанной прокрутки.
        lv_obj_add_event_cb(tile, tileEvent, LV_EVENT_SHORT_CLICKED, (void*)uintptr_t(i));
        tileBtns[i] = tile;
        tileLabels[i] = mustLabel(tile, "favoriteName");
        lv_obj_set_style_text_color(tileLabels[i], lv_color_hex(0xffffff), 0);
        // Две строки названия; справа оставляю место для паузы.
        lv_obj_set_size(tileLabels[i], 76, 38);
        lv_obj_set_pos(tileLabels[i], 4, 5);
        setTileText(tileLabels[i], ALL_STATIONS[id].name, 76);
        favIcons[i] = makePlayIcon(tile);
        lv_obj_set_pos(favIcons[i], 84, 16);
        setPlayIcon(favIcons[i], true);
        lv_obj_add_flag(favIcons[i], LV_OBJ_FLAG_HIDDEN);
        ++count;
    }
    if (!count) {
        lv_obj_t* empty = mustLabel(favoritesPanel, "emptyFavorites");
        lv_label_set_text(empty, "Добавьте избранное\nв списке станций");
        lv_obj_set_style_text_color(empty, lv_color_hex(0x9090a0), 0);
    }
    if (lv_screen_active() == mainScreen)
        lv_obj_scroll_to_y(favoritesPanel, oldScroll, LV_ANIM_OFF);
    ++favoritesRevision;
}
static void allStationEvent(lv_event_t* e) {
    // Only the playback button itself may request audio, never a child event.
    if(lv_event_get_target(e) != lv_event_get_current_target(e)) return;
    int idx = int(uintptr_t(lv_event_get_user_data(e)));
    Command c{
        sameStation(currentStation, 100 + idx) ? CMD_TOGGLE : CMD_PLAY_ALL, (uint8_t)idx
    };
    xQueueSend(audioCmdQueue, &c, 0);
    if (mainScreen) lv_screen_load(mainScreen);
}
// Передаю команду существующего WAV-теста через аудиоочередь. Здесь не создаю отдельный низкоуровневый I2S-драйвер.
static void toneEvent(lv_event_t*) {

    Command c{
        CMD_TEST_TONE, 0
    };
    (void)xQueueSend(audioCmdQueue, &c, 0);
}
// Отправляю команду полной остановки. Изменение состояния воспроизведения выполняется в аудиозадаче.
static void stopEvent(lv_event_t*) {
    Command c{
        CMD_STOP, 0
    };
    xQueueSend(audioCmdQueue, &c, 0);
}
// Сохраняю положение ползунка в атомарном значении громкости 0–21. На экране показываю проценты; применение к Audio и запись в память выполняет аудиозадача.
static void volumeEvent(lv_event_t* e) {
    lv_obj_t* slider = (lv_obj_t*)lv_event_get_target(e);
    int v = lv_slider_get_value(slider);
    currentVolume = v;
    // Latest volume is an atomic mailbox: it cannot be lost in a full command queue.
    if (volumeLabel) {
        char b[16];
        snprintf(b, sizeof(b), "%d%%", v * 100 / 21);
        lv_label_set_text(volumeLabel, b);
    }
}
// One shared overlay; only the GUI task touches these objects.
static void closeVolumePopup(lv_event_t*) {
    volumePopupOpen = false;
    lv_obj_add_flag(volumeOverlay, LV_OBJ_FLAG_HIDDEN);
}
static void openVolumePopup(lv_event_t*) {
    if (volumePopupOpen) { closeVolumePopup(nullptr); return; }
    volumePopupOpen = true;
    volumePopupActivity = millis();
    lv_slider_set_value(popupVolumeSlider, currentVolume.load(), LV_ANIM_OFF);
    char text[16]; snprintf(text, sizeof(text), "%d%%", currentVolume.load()*100/21);
    lv_label_set_text(popupVolumeLabel, text);
    lv_obj_remove_flag(volumeOverlay, LV_OBJ_FLAG_HIDDEN);
}
static void popupVolumeEvent(lv_event_t* e) {
    volumePopupActivity = millis();
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        volumeEvent(e);
        char text[16]; snprintf(text, sizeof(text), "%d%%", currentVolume.load()*100/21);
        lv_label_set_text(popupVolumeLabel, text);
    }
}
static void buildVolumePopup() {
    lv_obj_t* top = lv_layer_top();
    volumeOverlay = mustCreate(top, "volumeOverlay");
    lv_obj_remove_style_all(volumeOverlay);
    lv_obj_set_size(volumeOverlay, 240, 320);
    lv_obj_add_flag(volumeOverlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(volumeOverlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(volumeOverlay, closeVolumePopup, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* panel = mustCreate(volumeOverlay, "volumePanel");
    lv_obj_remove_style_all(panel);
    lv_obj_set_pos(panel, 8, 40); lv_obj_set_size(panel, 224, 88);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0x18221b), 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(panel, lv_color_hex(0x508050), 0);
    lv_obj_set_style_border_width(panel, 1, 0);
    lv_obj_add_flag(panel, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(panel, static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_EVENT_BUBBLE | LV_OBJ_FLAG_GESTURE_BUBBLE));
    lv_obj_t* caption = mustLabel(panel, "volumeCaption");
    lv_obj_set_style_text_font(panel, &dejavu_16, 0);
    lv_obj_set_style_text_color(panel, lv_color_hex(0x80ff80), 0);
    lv_label_set_text(caption, "Громкость"); lv_obj_set_pos(caption, 12, 10);
    popupVolumeLabel = mustLabel(panel, "popupVolumePercent");
    lv_obj_set_pos(popupVolumeLabel, 170, 10);
    popupVolumeSlider = lv_slider_create(panel);
    lv_obj_set_pos(popupVolumeSlider, 20, 53); lv_obj_set_size(popupVolumeSlider, 184, 16);
    lv_obj_set_ext_click_area(popupVolumeSlider, 12);
    lv_slider_set_range(popupVolumeSlider, 0, 21);
    lv_obj_set_style_bg_color(popupVolumeSlider, lv_color_hex(0x39463c), LV_PART_MAIN);
    lv_obj_set_style_bg_color(popupVolumeSlider, lv_color_hex(0x80ff80), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(popupVolumeSlider, lv_color_hex(0xffffff), LV_PART_KNOB);
    lv_obj_remove_flag(popupVolumeSlider, static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE));
    lv_obj_add_event_cb(popupVolumeSlider, popupVolumeEvent, LV_EVENT_ALL, nullptr);
    lv_obj_add_flag(volumeOverlay, LV_OBJ_FLAG_HIDDEN);
    // Opaque square pixels, no glyphs or antialiasing.
    lv_obj_t* button = mustButton(top, "globalVolume");
    globalVolumeButton=button;
    lv_obj_remove_style_all(button);
    lv_obj_set_pos(button, 204, 2); lv_obj_set_size(button, 34, 34);
    lv_obj_set_style_pad_all(button, 0, 0);
    lv_obj_set_style_bg_opa(button, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_remove_flag(button, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(button, openVolumePopup, LV_EVENT_CLICKED, nullptr);
    const int pixels[][4] = {{6,13,4,8},{10,11,2,12},{12,9,2,16},{14,7,2,20},{20,11,2,12},{24,8,2,18}};
    for (const auto& r : pixels) {
        lv_obj_t* pixel = mustCreate(button, "speakerPixel");
        lv_obj_remove_style_all(pixel);
        lv_obj_set_pos(pixel,r[0],r[1]); lv_obj_set_size(pixel,r[2],r[3]);
        lv_obj_set_style_bg_color(pixel,lv_color_hex(0xffffff),0);
        lv_obj_set_style_bg_opa(pixel,LV_OPA_COVER,0);
        lv_obj_remove_flag(pixel,static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE));
    }
}

// Открываю уже созданный список станций без повторного выделения объектов.
static void openListEvent(lv_event_t*) {
    if (listScreen) lv_screen_load(listScreen);
}
// Возвращаю главный экран, не меняя воспроизведение.
static void backEvent(lv_event_t*) {
    if (mainScreen) lv_screen_load(mainScreen);
}
// Открываю готовый экран настроек; радио продолжает работать в своей задаче.
static void openSettingsEvent(lv_event_t*) {
    if (settingsScreen) lv_screen_load(settingsScreen);
}
// Возвращаюсь из настроек на главный экран без остановки радио.
static void backFromSettingsEvent(lv_event_t*) {
    if (mainScreen) lv_screen_load(mainScreen);
}
// Получаю значение ползунка, применяю яркость и обновляю процент рядом с ним.
static void brightnessEvent(lv_event_t* e) {
    lv_obj_t* slider = (lv_obj_t*)lv_event_get_target(e);
    int v = lv_slider_get_value(slider);
    setBrightness(v);
    if (brightnessLabel) {
        char b[16];
        snprintf(b, sizeof(b), "%d%%", v);
        lv_label_set_text(brightnessLabel, b);
    }
}
// Выбираю задержку сна дисплея и начинаю отсчёт бездействия заново. Это не таймер выключения радио.
static void sleepEvent(lv_event_t* e) {
    int v = lv_slider_get_value((lv_obj_t*)lv_event_get_target(e));
    sleepChoice.store(v);
    lastTouchAt = millis();
    lv_label_set_text(sleepLabel, sleepNames[v]);
}
#include "wifi_local_ui.h"

/* ---------- Портал Wi-Fi (только по кнопке) ---------- */
static volatile bool portalReturnRequested = false;
// Запускаю отдельную задачу портала Radio-Setup на срок до пяти минут. После сохранения сети перезагружаю устройство; при таймауте прошу GUI вернуть главный экран.
static void runWifiPortalTask(void*) {

    WiFiManager wm;
    wm.setDebugOutput(false);
    wm.setConfigPortalTimeout(300);
    wm.setBreakAfterConfig(true);
    wm.setSaveConfigCallback([]() {

        vTaskDelay(pdMS_TO_TICKS(1000));
        if (WiFi.status() == WL_CONNECTED) wifiSavedRemember(WiFi.SSID(), WiFi.psk());
        ESP.restart();
    });
    if (!wm.startConfigPortal("Radio-Setup")) {

        pushStatus("Таймаут настройки Wi-Fi");
        portalReturnRequested = true;
    }
    wifiPortalBusy.store(false);
    vTaskDelete(NULL);
}
// После подтверждения показываю инструкцию подключения и создаю задачу Wi-Fi-портала. Блокирующая работа портала не выполняется внутри LVGL.
static void startWifiConfirmed(lv_event_t*) {
    if (localWifiBusy.load()) {
        lv_label_set_text(localWifiStatus, "Дождитесь завершения поиска");
        return;
    }
    if (wifiPortalBusy.exchange(true)) return;
    if (wifiConfirm) lv_obj_add_flag(wifiConfirm, LV_OBJ_FLAG_HIDDEN);

    if (wifiScreen) lv_screen_load(wifiScreen);
    if (xTaskCreatePinnedToCore(runWifiPortalTask, "wifi_portal", 8192, nullptr, 1, nullptr, 0) != pdPASS) {
        wifiPortalBusy.store(false);
        lv_screen_load(localWifiScreen);
        lv_label_set_text(localWifiStatus, "Не удалось открыть портал");
    }
}
// Закрываю подтверждение, не запускаю настройку сети.
static void cancelWifi(lv_event_t*) {
    lv_obj_add_flag(wifiConfirm, LV_OBJ_FLAG_HIDDEN);
}
// Показываю предупреждение перед открытием портала настройки сети.
static void wifiSetupEvent(lv_event_t*) {
    lv_obj_remove_flag(wifiConfirm, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(wifiConfirm);
}
// Schedule shortcuts: opaque pixel art inside generous touch targets.
static lv_obj_t *alarmShortcut=nullptr, *offShortcut=nullptr;
static void updateScheduleShortcuts(bool alarmOn,bool offOn) {
    if(alarmShortcut) {
        if(alarmOn) lv_obj_remove_flag(alarmShortcut,LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(alarmShortcut,LV_OBJ_FLAG_HIDDEN);
    }
    if(offShortcut) {
        if(offOn) lv_obj_remove_flag(offShortcut,LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(offShortcut,LV_OBJ_FLAG_HIDDEN);
    }
}
static lv_obj_t* makeScheduleShortcut(int x,bool alarm) {
    lv_obj_t* button=lv_obj_create(mainScreen);
    lv_obj_remove_style_all(button);
    lv_obj_set_pos(button,x,0); lv_obj_set_size(button,40,38);
    lv_obj_remove_flag(button,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(button,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(button,[](lv_event_t* e){
        lv_screen_load(lv_event_get_user_data(e)?alarmScreen:settingsScreen);
    },LV_EVENT_CLICKED,alarm?reinterpret_cast<void*>(1):nullptr);
    // Two permanent masks/descriptors: no allocation and no growing descriptor counter.
    static uint8_t masks[2][400] = {};
    static lv_image_dsc_t descriptors[2] = {};
    const int slot = alarm ? 0 : 1;
    uint8_t* mask = masks[slot];
    for(int iy=0;iy<20;iy++) for(int ix=0;ix<20;ix++) {
        // Sample the original pixel art at 20 x 20; keep the touch target unchanged.
        int px=ix*24/20,y=iy*24/20;
        int dx=px-12,dy=y-12,d=dx*dx+dy*dy;
        bool ink;
        if(alarm) {
            ink=(d>=64 && d<=100) || (px>=11 && px<=12 && y>=6 && y<=13)
                || (y>=12 && y<=13 && px>=12 && px<=17)
                || (y>=1 && y<=3 && ((px>=3 && px<=7)||(px>=17 && px<=21)))
                || (y>=21 && y<=23 && ((px>=5 && px<=7)||(px>=17 && px<=19)));
        } else {
            ink=(d>=64 && d<=100 && !(y<10 && px>=8 && px<=16))
                || (px>=11 && px<=13 && y>=1 && y<=12);
        }
        mask[iy*20+ix] = ink ? 255 : 0;
    }
    lv_image_dsc_t& d = descriptors[slot];
    d.header.magic = LV_IMAGE_HEADER_MAGIC;
    d.header.cf = LV_COLOR_FORMAT_A8;
    d.header.w = 20; d.header.h = 20; d.header.stride = 20;
    d.data_size = 400; d.data = mask;
    lv_obj_t* image = lv_image_create(button);
    lv_image_set_src(image, &d);
    lv_obj_set_pos(image, 10, 8);
    lv_obj_set_style_image_recolor(image, lv_color_white(), 0);
    lv_obj_set_style_image_recolor_opa(image, LV_OPA_COVER, 0);
    lv_obj_remove_flag(image, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(button,LV_OBJ_FLAG_HIDDEN);
    return button;
}
/* ---------- Главный экран ---------- */
// Один раз создаю главный экран, индикаторы, текущую станцию и область избранного. Координаты и цвета сохраняю; позже обновляю существующие объекты через refreshUi.
static void buildMainScreen() {
    mainScreen = lv_obj_create(NULL);
    if (!mainScreen) fatal("mainScreen alloc failed");
    lv_obj_set_style_pad_all(mainScreen, 0, 0);
    lv_obj_set_style_border_width(mainScreen, 0, 0);
    lv_obj_set_style_bg_color(mainScreen, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(mainScreen, &dejavu_16, 0);
    alarmShortcut=makeScheduleShortcut(66,true);
    offShortcut=makeScheduleShortcut(108,false);
    lv_obj_remove_flag(mainScreen, LV_OBJ_FLAG_SCROLLABLE);
    // Graphic Wi-Fi icon: no font changes, 24 x 24 pixels.
    wifiIndicator = mustCreate(mainScreen, "wifiIndicator");
    lv_obj_remove_style_all(wifiIndicator);
    lv_obj_set_pos(wifiIndicator, 8, 6);
    lv_obj_set_size(wifiIndicator, 24, 24);
    lv_obj_remove_flag(wifiIndicator, LV_OBJ_FLAG_SCROLLABLE);
    // Binary pixel mask: opaque rectangles, no rounded edges or antialiasing.
    static const char* mask[24] = {
        "........................",
        "........................",
        "........................",
        "........................",
        "......############......",
        "....################....",
        "..#####..........#####..",
        ".####..............####.",
        "###..................###",
        "##.....##########.....##",
        "#....##############....#",
        "....####........####....",
        "...###............###...",
        "...##..............##...",
        "........########........",
        ".......##########.......",
        ".......##......##.......",
        "........................",
        "........................",
        "..........####..........",
        "..........####..........",
        "..........####..........",
        "........................",
        "........................",
    };
    for (int y = 0; y < 24; ++y) {
        for (int x = 0; x < 24;) {
            if (mask[y][x] != '#') {
                ++x;
                continue;
            }
            int start = x;
            while (x < 24 && mask[y][x] == '#') ++x;
            lv_obj_t* pixel = lv_obj_create(wifiIndicator);
            lv_obj_remove_style_all(pixel);
            lv_obj_set_pos(pixel, start, y);
            lv_obj_set_size(pixel, x - start, 1);
            lv_obj_set_style_bg_opa(pixel, LV_OPA_COVER, 0);
            lv_obj_remove_flag(pixel, LV_OBJ_FLAG_SCROLLABLE);
            wifiPixels[wifiPixelCount++] = pixel;
        }
    }
    wifiSlash = lv_obj_create(wifiIndicator);
    lv_obj_remove_style_all(wifiSlash);
    lv_obj_set_size(wifiSlash, 24, 24);
    lv_obj_set_pos(wifiSlash, 0, 0);
    lv_obj_remove_flag(wifiSlash, LV_OBJ_FLAG_SCROLLABLE);
    for (int i = 3; i <= 20; ++i) {
        lv_obj_t* pixel = lv_obj_create(wifiSlash);
        lv_obj_remove_style_all(pixel);
        lv_obj_set_pos(pixel, i, i);
        lv_obj_set_size(pixel, 2, 1);
        lv_obj_set_style_bg_color(pixel, lv_color_hex(0x808090), 0);
        lv_obj_set_style_bg_opa(pixel, LV_OPA_COVER, 0);
    }
    updateWifiIndicator();
    // Compact battery outline, immediately right of the Wi-Fi icon.
    batteryIndicator = mustCreate(mainScreen, "batteryIndicator");
    lv_obj_remove_style_all(batteryIndicator);
    lv_obj_set_pos(batteryIndicator, 38, 11);
    lv_obj_set_size(batteryIndicator, 23, 14);
    lv_obj_set_style_border_width(batteryIndicator, 1, 0);
    lv_obj_remove_flag(batteryIndicator, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t* terminal = mustCreate(mainScreen, "batteryTerminal");
    lv_obj_remove_style_all(terminal);
    lv_obj_set_pos(terminal, 61, 15);
    lv_obj_set_size(terminal, 2, 6);
    lv_obj_set_style_bg_color(terminal, lv_color_hex(0x808090), 0);
    lv_obj_set_style_bg_opa(terminal, LV_OPA_COVER, 0);
    for (int i = 0; i < 4; ++i) {
        batteryBars[i] = mustCreate(batteryIndicator, "batteryBar");
        lv_obj_remove_style_all(batteryBars[i]);
        lv_obj_set_pos(batteryBars[i], 2 + i * 5, 2);
        lv_obj_set_size(batteryBars[i], 4, 8);
        lv_obj_set_style_bg_opa(batteryBars[i], LV_OPA_COVER, 0);
    }
    batteryUnknown = mustLabel(batteryIndicator, "batteryUnknown");
    lv_label_set_text(batteryUnknown, "?");
    lv_obj_set_style_text_color(batteryUnknown, lv_color_hex(0x808090), 0);
    lv_obj_center(batteryUnknown);
    updateBatteryIndicator();
    clockLabel = mustLabel(mainScreen,"clock");
    lv_obj_set_width(clockLabel, 52);
    lv_obj_set_style_text_align(clockLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(clockLabel, 150, 8);
    lv_obj_set_style_text_color(clockLabel,lv_color_hex(0xb0b0b0),0);
    lv_label_set_text(clockLabel,"--:--");
    radioCountdownLabel = mustLabel(mainScreen, "radioCountdown");
    lv_obj_set_size(radioCountdownLabel, 216, 18);
    lv_obj_set_pos(radioCountdownLabel, 12, 282);
    lv_obj_set_style_text_align(radioCountdownLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(radioCountdownLabel, lv_color_hex(0xb0b0b0), 0);
    lv_label_set_text(radioCountdownLabel, "");
    favoritesPanel = mustCreate(mainScreen, "favoritesPanel");
    lv_obj_remove_style_all(favoritesPanel);
    lv_obj_set_pos(favoritesPanel, 12, 38);
    lv_obj_set_size(favoritesPanel, 216, 156);
    lv_obj_set_scroll_dir(favoritesPanel, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(favoritesPanel, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_remove_flag(favoritesPanel, static_cast<lv_obj_flag_t>(
        LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_CHAIN_VER | LV_OBJ_FLAG_SCROLL_CHAIN_HOR));
    lv_obj_add_flag(favoritesPanel, LV_OBJ_FLAG_SCROLLABLE);
    rebuildFavoriteTiles();
    nowTile = mustButton(mainScreen, "nowPlaying");
    lv_obj_set_pos(nowTile, 12, 200);
    lv_obj_set_size(nowTile, 216, 30);
    lv_obj_set_style_pad_all(nowTile, 0, 0);
    lv_obj_set_style_radius(nowTile, 0, 0);
    lv_obj_set_style_bg_color(nowTile, lv_color_hex(0x404040), 0);
    lv_obj_add_event_cb(nowTile, toggleEvent, LV_EVENT_CLICKED, nullptr);
    nowLabel = mustLabel(nowTile, "nowName");
    lv_obj_set_pos(nowLabel, 4, 5);
    lv_obj_set_width(nowLabel, 180);
    lv_obj_set_style_text_color(nowLabel, lv_color_hex(0xffffff), 0);
    lv_label_set_long_mode(nowLabel, LV_LABEL_LONG_MODE_DOTS);
    nowIcon = makePlayIcon(nowTile);
    lv_obj_set_pos(nowIcon, 195, 9);
    // Восемь широких цветных столбиков с чёткими краями.
    const uint32_t spectrumColors[] = {0xA200FF, 0xFF0097, 0x00ABA9, 0x8CBF26,
        0xA05000, 0xE671B8, 0x1BA1E2, 0xE51400, 0x339933};
    for (int i = 0; i < SPECTRUM_BANDS; ++i) {
        spectrumBars[i] = mustCreate(mainScreen, "spectrumBar");
        lv_obj_remove_style_all(spectrumBars[i]);
        lv_obj_set_pos(spectrumBars[i], 12 + i * 28, 276);
        lv_obj_set_size(spectrumBars[i], 20, 0);
        lv_obj_set_style_bg_color(spectrumBars[i], lv_color_hex(spectrumColors[i % 9]), 0);
        lv_obj_set_style_bg_opa(spectrumBars[i], LV_OPA_COVER, 0);
        lv_obj_remove_flag(spectrumBars[i], static_cast<lv_obj_flag_t>(
            LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));
    }
    // Два пустых ряда после основания спектра (y=276), затем 3 пикселя.
    lv_obj_t* inputBufferTrack = mustCreate(mainScreen, "inputBufferTrack");
    lv_obj_remove_style_all(inputBufferTrack);
    lv_obj_set_pos(inputBufferTrack, 0, 278);
    lv_obj_set_size(inputBufferTrack, INPUT_BUFFER_BAR_WIDTH, 3);
    lv_obj_set_style_bg_color(inputBufferTrack, lv_color_hex(0xE51400), 0);
    lv_obj_set_style_bg_opa(inputBufferTrack, LV_OPA_COVER, 0);
    lv_obj_remove_flag(inputBufferTrack, static_cast<lv_obj_flag_t>(
        LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));
    inputBufferFill = mustCreate(inputBufferTrack, "inputBufferFill");
    lv_obj_remove_style_all(inputBufferFill);
    lv_obj_set_pos(inputBufferFill, 0, 0);
    lv_obj_set_size(inputBufferFill, 0, 3);
    lv_obj_set_style_bg_color(inputBufferFill, lv_color_hex(0x339933), 0);
    lv_obj_set_style_bg_opa(inputBufferFill, LV_OPA_COVER, 0);
    lv_obj_remove_flag(inputBufferFill, static_cast<lv_obj_flag_t>(
        LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));
    /* Кнопки внизу */
    lv_obj_t* allBtn = mustButton(mainScreen, "allBtn");
    lv_obj_set_pos(allBtn, 148, 298);
    lv_obj_set_size(allBtn, 80, 22);
    lv_obj_set_ext_click_area(allBtn, 6);
    lv_obj_set_style_pad_all(allBtn, 0, 0);
    lv_obj_set_style_bg_color(allBtn, lv_color_hex(0x000000), 0);
    lv_obj_set_style_radius(allBtn, 0, 0);
    lv_obj_set_style_border_width(allBtn, 0, 0);
    lv_obj_add_event_cb(allBtn, openListEvent, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* allLbl = mustLabel(allBtn, "allLbl");
    lv_label_set_text(allLbl, "Станции");
    lv_obj_center(allLbl);
    lv_obj_set_style_text_color(allLbl, lv_color_hex(0x9090a0), 0);
    statusLabel = mustLabel(mainScreen, "statusLabel");
    lv_label_set_text(statusLabel, currentStatus);
    lv_obj_set_style_text_color(statusLabel, lv_color_hex(0x8080a0), 0);
    lv_obj_set_size(statusLabel, 132, 20);
    lv_obj_set_pos(statusLabel, 12, 298);
    lv_label_set_long_mode(statusLabel, LV_LABEL_LONG_MODE_DOTS);
}
/* ---------- Список станций ---------- */
// Создаю прокручиваемый список. В каждой строке чекбокс и кнопка воспроизведения являются соседними объектами, чтобы управление избранным не запускало станцию.
// Opaque pixel navigation arrows. Decorative children never intercept taps.
static void navigationArrow(lv_obj_t* button, bool right, uint32_t color) {
    lv_obj_set_style_pad_all(button,0,0);
    lv_obj_remove_flag(button,static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER));
    lv_obj_update_layout(button);
    int cx=lv_obj_get_width(button)/2, cy=lv_obj_get_height(button)/2;
    auto pixel=[&](int x,int y,int w,int h) {
        lv_obj_t* p=lv_obj_create(button); lv_obj_remove_style_all(p);
        lv_obj_set_pos(p,x,y); lv_obj_set_size(p,w,h);
        lv_obj_set_style_bg_color(p,lv_color_hex(color),0);
        lv_obj_set_style_bg_opa(p,LV_OPA_COVER,0);
        lv_obj_remove_flag(p,static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE));
    };
    pixel(cx-8,cy-2,16,4);
    for(int i=0;i<5;++i) {
        int x=right ? cx+6-i*2 : cx-8+i*2;
        pixel(x,cy-2-i*2,2,4); pixel(x,cy-2+i*2,2,4);
    }
}
static void buildListScreen() {
    listScreen = lv_obj_create(NULL);
    if (!listScreen) fatal("listScreen alloc failed");
    lv_obj_set_style_pad_all(listScreen, 0, 0);
    lv_obj_set_style_border_width(listScreen, 0, 0);
    lv_obj_set_style_bg_color(listScreen, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(listScreen, &dejavu_16, 0);
    lv_obj_t* title = mustLabel(listScreen, "listTitle");
    lv_label_set_text(title, "Все станции");
    lv_obj_set_style_text_color(title, lv_color_hex(0x4fc3f7), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_t* backBtn = mustButton(listScreen, "backBtn");
    lv_obj_set_size(backBtn, 40, 30);
    lv_obj_align(backBtn, LV_ALIGN_TOP_LEFT, 8, 4);
    lv_obj_set_style_bg_color(backBtn, lv_color_hex(0x2a2a3a), 0);
    lv_obj_add_event_cb(backBtn, backEvent, LV_EVENT_CLICKED, nullptr);
    navigationArrow(backBtn,false,0xffffff);
    lv_obj_t* list = mustCreate(listScreen, "listContainer");
    lv_obj_set_pos(list, 8, 40);
    lv_obj_set_size(list, 224, 272);
    lv_obj_set_style_bg_color(list, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_pad_all(list, 0, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(list, 4, 0);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_AUTO);
    for (int i = 0; i < ALL_STATIONS_COUNT; i++) {

        // Checkbox and playback button are siblings, not parent and child.
        lv_obj_t* row = lv_obj_create(list);
        if(!row) {

            break;
        }
        lv_obj_remove_style_all(row);
        lv_obj_set_size(row, 216, 42);
        lv_obj_remove_flag(row, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_t* btn = lv_button_create(row);
        if (!btn) {

            break;
        }
        lv_obj_remove_style_all(btn);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_size(btn, 178, 42);
        lv_obj_set_pos(btn, 38, 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x404040), 0);
        lv_obj_set_style_pad_all(btn, 0, 0);
        lv_obj_set_style_radius(btn, 0, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_add_event_cb(btn, allStationEvent, LV_EVENT_CLICKED, (void*)uintptr_t(i));
        lv_obj_t* lbl = lv_label_create(btn);
        if (!lbl) {

            break;
        }
        lv_label_set_text(lbl, ALL_STATIONS[i].name);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xe0e0e0), 0);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 4, 0);
        lv_obj_set_width(lbl, 148);
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_MODE_DOTS);
        lv_obj_t* cb = lv_checkbox_create(row);
        if (!cb) fatal("Station checkbox allocation failed");
        favoriteChecks[i]=cb;
        lv_checkbox_set_text(cb, "");
        lv_obj_set_pos(cb,0,0);
        lv_obj_set_size(cb, 34, 42);
        lv_obj_set_style_pad_left(cb, 5, 0);
        lv_obj_set_style_pad_top(cb, 10, 0);
        lv_obj_set_style_pad_right(cb, 0, 0);
        lv_obj_set_style_pad_bottom(cb, 0, 0);
        for(int j=0;j<FAVORITES_COUNT;j++) if(favoriteIds[j]==i) lv_obj_add_state(cb,LV_STATE_CHECKED);
        // Use the native checkbox toggle; never bubble its tap to station playback.
        lv_obj_remove_flag(cb, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_add_flag(cb, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(cb, LV_OBJ_FLAG_CHECKABLE);
        lv_obj_remove_flag(cb, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(cb,favoriteEvent,LV_EVENT_VALUE_CHANGED,(void*)uintptr_t(i));
        listIcons[i] = makePlayIcon(btn);
        lv_obj_set_pos(listIcons[i], 157, 21);
        setPlayIcon(listIcons[i], false);
    }
}
// Показываю панель автора, переданную в пользовательских данных события. Воспроизведение не изменяю.
static void showAuthorEvent(lv_event_t* e) {
    lv_obj_remove_flag(static_cast<lv_obj_t*>(lv_event_get_user_data(e)), LV_OBJ_FLAG_HIDDEN);
}
// Скрываю панель автора и возвращаю доступ к настройкам.
static void hideAuthorEvent(lv_event_t* e) {
    lv_obj_add_flag(static_cast<lv_obj_t*>(lv_event_get_user_data(e)), LV_OBJ_FLAG_HIDDEN);
}
// First SD-player milestone: read-only discovery; audio ownership unchanged.
#include "sd_player_ui.h"
#include "photo_gallery.h"

#include "alarm_ui.h"

/* ---------- Настройки ---------- */
// Создаю громкость, яркость, сон дисплея, таймер радио, настройку сети и окно автора. Ползунки связаны с отдельными обработчиками; построение экрана не запускает их действия вручную.
static void buildSettingsScreen() {
    settingsScreen = lv_obj_create(NULL);
    if (!settingsScreen) fatal("settingsScreen alloc failed");
    lv_obj_set_scroll_dir(settingsScreen, LV_DIR_VER);
    lv_obj_set_style_pad_all(settingsScreen, 0, 0);
    lv_obj_set_style_border_width(settingsScreen, 0, 0);
    lv_obj_set_style_bg_color(settingsScreen, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(settingsScreen, &dejavu_16, 0);
    lv_obj_t* title = mustLabel(settingsScreen, "setTitle");
    lv_label_set_text(title, "Настройки");
    lv_obj_set_style_text_color(title, lv_color_hex(0x4fc3f7), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_t* backBtn = mustButton(settingsScreen, "setBackBtn");
    lv_obj_set_size(backBtn, 40, 30);
    lv_obj_align(backBtn, LV_ALIGN_TOP_LEFT, 8, 4);
    lv_obj_set_style_bg_color(backBtn, lv_color_hex(0x2a2a3a), 0);
    lv_obj_add_event_cb(backBtn, backFromSettingsEvent, LV_EVENT_CLICKED, nullptr);
    navigationArrow(backBtn,false,0xffffff);
    lv_obj_t* volTitle = mustLabel(settingsScreen, "volTitle");
    lv_label_set_text(volTitle, "Громкость");
    lv_obj_set_style_text_color(volTitle, lv_color_hex(0x9090a0), 0);
    lv_obj_set_pos(volTitle, 16, 44);
    volumeLabel = mustLabel(settingsScreen, "volumeLabel");
    char initialVolume[16];
    snprintf(initialVolume, sizeof(initialVolume), "%d%%", currentVolume.load() * 100 / 21);
    lv_label_set_text(volumeLabel, initialVolume);
    lv_obj_set_style_text_color(volumeLabel, lv_color_hex(0x4fc3f7), 0);
    lv_obj_align(volumeLabel, LV_ALIGN_TOP_RIGHT, -16, 44);
    volumeSlider = lv_slider_create(settingsScreen);
    lv_obj_set_pos(volumeSlider, 16, 76);
    lv_obj_set_size(volumeSlider, 208, 12);
    lv_obj_set_ext_click_area(volumeSlider, 10); // 32 px touch height, unchanged appearance
    lv_slider_set_range(volumeSlider, 0, 21);
    lv_slider_set_value(volumeSlider, currentVolume, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(volumeSlider, lv_color_hex(0x707070), LV_PART_MAIN);
    lv_obj_set_style_bg_color(volumeSlider, lv_color_hex(0x4fc3f7), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(volumeSlider, lv_color_hex(0xffffff), LV_PART_KNOB);
    lv_obj_add_event_cb(volumeSlider, volumeEvent, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_t* brightLbl = mustLabel(settingsScreen, "brightLbl");
    lv_label_set_text(brightLbl, "Яркость");
    lv_obj_set_style_text_color(brightLbl, lv_color_hex(0x9090a0), 0);
    lv_obj_set_pos(brightLbl, 16, 100);
    brightnessLabel = mustLabel(settingsScreen, "brightnessLabel");
    char b[16];
    snprintf(b, sizeof(b), "%d%%", currentBrightness.load());
    lv_label_set_text(brightnessLabel, b);
    lv_obj_set_style_text_color(brightnessLabel, lv_color_hex(0x4fc3f7), 0);
    lv_obj_align(brightnessLabel, LV_ALIGN_TOP_RIGHT, -16, 100);
    lv_obj_t* slider = lv_slider_create(settingsScreen);
    brightnessSlider = slider;
    lv_obj_set_pos(slider, 16, 128);
    lv_obj_set_size(slider, 208, 12);
    lv_obj_set_ext_click_area(slider, 10); // 32 px touch height, unchanged appearance
    lv_slider_set_range(slider, 15, 100);
    lv_slider_set_value(slider, currentBrightness, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0x2a2a3a), LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0x4fc3f7), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0xffffff), LV_PART_KNOB);
    lv_obj_add_event_cb(slider, brightnessEvent, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_t* sleepTitle = mustLabel(settingsScreen, "sleepTitle");
    lv_label_set_text(sleepTitle, "Отключение экрана");
    lv_obj_set_style_text_color(sleepTitle, lv_color_hex(0xe0e0e0), 0);
    lv_obj_set_pos(sleepTitle, 16, 150);
    sleepLabel = mustLabel(settingsScreen, "sleepValue");
    lv_obj_set_style_text_color(sleepLabel, lv_color_hex(0x4fc3f7), 0);
    lv_obj_set_pos(sleepLabel, 16, 174);
    lv_label_set_text(sleepLabel, sleepNames[sleepChoice.load()]);
    lv_obj_t* sleepSlider = lv_slider_create(settingsScreen);
    displaySleepSlider = sleepSlider;
    lv_obj_set_pos(sleepSlider, 16, 204);
    lv_obj_set_size(sleepSlider, 208, 12);
    lv_obj_set_ext_click_area(sleepSlider, 10); // 32 px touch height, unchanged appearance
    lv_slider_set_range(sleepSlider, 0, 6);
    lv_slider_set_value(sleepSlider, sleepChoice.load(), LV_ANIM_OFF);
    lv_obj_set_style_bg_color(sleepSlider, lv_color_hex(0x707070), LV_PART_MAIN);
    lv_obj_set_style_bg_color(sleepSlider, lv_color_hex(0x4fc3f7), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(sleepSlider, lv_color_hex(0xffffff), LV_PART_KNOB);
    lv_obj_add_event_cb(sleepSlider, sleepEvent, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_t* wifiBtn = mustButton(settingsScreen, "wifiSettings");
    timerLabel=mustLabel(settingsScreen,"radioTimer");
    lv_obj_set_style_text_color(timerLabel, lv_color_hex(0xe0e0e0), 0);
    lv_obj_set_pos(timerLabel,16,228);
    lv_label_set_text(timerLabel,"Таймер радио выключен");
    lv_obj_t* timerSlider=lv_slider_create(settingsScreen);
    radioTimerSlider=timerSlider;
    lv_obj_set_pos(timerSlider,16,258);
    lv_obj_set_size(timerSlider,208,12);
    lv_obj_set_ext_click_area(timerSlider, 10); // 32 px touch height, unchanged appearance
    lv_slider_set_range(timerSlider,0,4);
    lv_obj_add_event_cb(timerSlider,radioTimerEvent,LV_EVENT_VALUE_CHANGED,nullptr);
    buildOffControls();
    lv_obj_set_pos(wifiBtn, 16, 382);
    lv_obj_set_size(wifiBtn, 208, 30);
    lv_obj_add_event_cb(wifiBtn, wifiSetupEvent, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* wifiText = mustLabel(wifiBtn, "wifiSettingsText");
    lv_label_set_text(wifiText, "Настройка Wi-Fi");
    lv_obj_center(wifiText);
    wifiConfirm = mustCreate(settingsScreen, "confirmWifi");
    lv_obj_set_pos(wifiConfirm, 8, 120);
    lv_obj_set_size(wifiConfirm, 224, 190);
    lv_obj_set_style_pad_all(wifiConfirm, 4, 0);
    lv_obj_t* warning = mustLabel(wifiConfirm, "warning");
    lv_obj_set_width(warning, 208);
    lv_label_set_text(warning, "Открыть настройку Wi-Fi?\nПодключение к радио может прерваться.");
    lv_obj_t* yes = mustButton(wifiConfirm, "yes");
    lv_obj_set_pos(yes, 0, 125);
    lv_obj_set_size(yes, 100, 40);
    lv_obj_add_event_cb(yes, startWifiConfirmed, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* yl = mustLabel(yes, "yesText");
    lv_label_set_text(yl, "Открыть");
    lv_obj_center(yl);
    lv_obj_t* no = mustButton(wifiConfirm, "no");
    lv_obj_set_pos(no, 108, 125);
    lv_obj_set_size(no, 100, 40);
    lv_obj_add_event_cb(no, cancelWifi, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* nl = mustLabel(no, "noText");
    lv_label_set_text(nl, "Отмена");
    lv_obj_center(nl);
    lv_obj_add_flag(wifiConfirm, LV_OBJ_FLAG_HIDDEN);
    // Full opaque overlay blocks settings taps without interrupting playback.
    lv_obj_t* authorPanel = mustCreate(settingsScreen, "authorPanel");
    lv_obj_remove_style_all(authorPanel);
    lv_obj_set_pos(authorPanel, 0, 0);
    lv_obj_set_size(authorPanel, 240, 320);
    lv_obj_set_style_bg_color(authorPanel, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(authorPanel, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(authorPanel, lv_color_hex(0xffffff), 0);
    lv_obj_add_flag(authorPanel, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(authorPanel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t* authorText = mustLabel(authorPanel, "authorText");
    lv_obj_set_width(authorText, 224);
    lv_obj_set_style_text_align(authorText, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(authorText, "Автор и разработчик:\nБорисенко Д.А.\nг.Краснодон, ЛНР,\n2026 год.");
    lv_obj_align(authorText, LV_ALIGN_CENTER, 0, -24);
    // В окне информации показываю адрес пульта, не перестраивая настройки.
    remoteAddressLabel = mustLabel(authorPanel, "remoteAddress");
    lv_obj_set_width(remoteAddressLabel, 224);
    lv_obj_set_style_text_align(remoteAddressLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(remoteAddressLabel, 8, 205);
    lv_label_set_text(remoteAddressLabel, "Веб-пульт: нет Wi-Fi");
    lv_obj_t* closeAuthor = mustButton(authorPanel, "closeAuthor");
    lv_obj_set_size(closeAuthor, 160, 40);
    lv_obj_align(closeAuthor, LV_ALIGN_BOTTOM_MID, 0, -28);
    lv_obj_set_style_bg_color(closeAuthor, lv_color_hex(0x2a2a3a), 0);
    lv_obj_add_event_cb(closeAuthor, hideAuthorEvent, LV_EVENT_CLICKED, authorPanel);
    lv_obj_t* closeText = mustLabel(closeAuthor, "closeAuthorText");
    lv_label_set_text(closeText, "Закрыть");
    lv_obj_center(closeText);
    lv_obj_add_flag(authorPanel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t* infoBtn = mustButton(settingsScreen, "authorInfo");
    lv_obj_set_size(infoBtn, 40, 30);
    lv_obj_align(infoBtn, LV_ALIGN_TOP_RIGHT, -8, 4);
    lv_obj_set_style_bg_color(infoBtn, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(infoBtn, 0, 0);
    lv_obj_set_style_shadow_width(infoBtn, 0, 0);
    lv_obj_set_style_pad_all(infoBtn, 0, 0);
    lv_obj_add_event_cb(infoBtn, showAuthorEvent, LV_EVENT_CLICKED, authorPanel);
    static const char* infoMask[16] = {
        ".....######.....", "...##......##...",
        "..#..........#..", ".#............#.",
        ".#.....##.....#.", "#......##......#",
        "#..............#", "#.....###......#",
        "#......##......#", "#......##......#",
        "#......##......#", ".#....####....#.",
        ".#............#.", "..#..........#..",
        "...##......##...", ".....######....."
    };
    for (int y = 0; y < 16; ++y) {
        for (int x = 0; x < 16;) {
            if (infoMask[y][x] != '#') {
                ++x;
                continue;
            }
            const int start = x;
            while (x < 16 && infoMask[y][x] == '#') ++x;
            lv_obj_t* pixel = mustCreate(infoBtn, "infoPixel");
            lv_obj_remove_style_all(pixel);
            lv_obj_set_pos(pixel, 12 + start, 7 + y);
            lv_obj_set_size(pixel, x - start, 1);
            lv_obj_set_style_bg_color(pixel, lv_color_hex(0xffffff), 0);
            lv_obj_set_style_bg_opa(pixel, LV_OPA_COVER, 0);
            lv_obj_remove_flag(pixel, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_remove_flag(pixel, LV_OBJ_FLAG_SCROLLABLE);
        }
    }
    // Keep the overlay above the info button too.
    lv_obj_move_foreground(authorPanel);
}
/* ---------- Экран Wi-Fi ---------- */
// Создаю экран инструкции для портала: имя временной сети, адрес браузера и порядок сохранения домашней сети.
static void buildWifiScreen() {
    wifiScreen = lv_obj_create(NULL);
    if (!wifiScreen) fatal("wifiScreen alloc failed");
    lv_obj_set_style_pad_all(wifiScreen, 0, 0);
    lv_obj_set_style_border_width(wifiScreen, 0, 0);
    lv_obj_set_style_bg_color(wifiScreen, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(wifiScreen, &dejavu_16, 0);
    lv_obj_t* title = mustLabel(wifiScreen, "wifiTitle");
    lv_label_set_text(title, "Настройка Wi-Fi");
    lv_obj_set_style_text_color(title, lv_color_hex(0x4fc3f7), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_t* line1 = mustLabel(wifiScreen, "wifiLine1");
    lv_label_set_text(line1, "1. Подключитесь к сети");
    lv_obj_set_style_text_color(line1, lv_color_hex(0xe0e0e0), 0);
    lv_obj_set_pos(line1, 16, 70);
    lv_obj_t* ssid = mustLabel(wifiScreen, "wifiSsid");
    lv_label_set_text(ssid, "Radio-Setup");
    lv_obj_set_style_text_color(ssid, lv_color_hex(0x00E676), 0);
    lv_obj_set_pos(ssid, 16, 100);
    lv_obj_t* line2 = mustLabel(wifiScreen, "wifiLine2");
    lv_label_set_text(line2, "2. Откройте в браузере:");
    lv_obj_set_style_text_color(line2, lv_color_hex(0xe0e0e0), 0);
    lv_obj_set_pos(line2, 16, 140);
    lv_obj_t* url = mustLabel(wifiScreen, "wifiUrl");
    lv_label_set_text(url, "192.168.4.1");
    lv_obj_set_style_text_color(url, lv_color_hex(0x4fc3f7), 0);
    lv_obj_set_pos(url, 16, 170);
    lv_obj_t* line3 = mustLabel(wifiScreen, "wifiLine3");
    lv_label_set_text(line3, "3. Выберите свою сеть\n   и введите пароль.");
    lv_obj_set_style_text_color(line3, lv_color_hex(0xe0e0e0), 0);
    lv_obj_set_pos(line3, 16, 210);
    lv_obj_t* hint = mustLabel(wifiScreen, "wifiHint");
    lv_label_set_text(hint, "После сохранения\nплата перезагрузится.");
    lv_obj_set_style_text_color(hint, lv_color_hex(0x9090a0), 0);
    lv_obj_set_pos(hint, 16, 270);
}
// Согласовываю подписи и значки с текущим состоянием. Сравниваю с предыдущими значениями, чтобы не обновлять неизменившиеся элементы на каждом проходе GUI.
static void refreshUi() {
    static int prevStation = -999, prevVolume = -1;
    static bool prevPlaying = false;
    static uint32_t prevFavoritesRevision = UINT32_MAX;
    if (prevStation != currentStation || prevPlaying != isPlaying || prevFavoritesRevision != favoritesRevision) {
        prevFavoritesRevision = favoritesRevision;
        prevStation = currentStation;
        prevPlaying = isPlaying;
        for (int i = 0; i < STATION_COUNT; ++i) {
            if (!tileBtns[i]) continue;
            bool active = sameStation(currentStation, i) && isPlaying;
            lv_obj_set_style_border_width(tileBtns[i], active ? 2 : 0, 0);
            lv_obj_set_style_border_color(tileBtns[i], lv_color_hex(0x00e676), 0);
            if (active) lv_obj_remove_flag(favIcons[i], LV_OBJ_FLAG_HIDDEN);
            else lv_obj_add_flag(favIcons[i], LV_OBJ_FLAG_HIDDEN);
        }
        for (int i = 0; i < ALL_STATIONS_COUNT; ++i)
        if (listIcons[i]) setPlayIcon(listIcons[i], sameStation(currentStation, 100+i) && isPlaying);
        const auto* station = selectedStation(currentStation);
        lv_label_set_text(nowLabel, station ? station->name : "Выберите станцию");
        setPlayIcon(nowIcon, isPlaying);
    }
    if (prevVolume != currentVolume) {
        prevVolume = currentVolume;
        char b[16];
        snprintf(b, sizeof(b), "%d%%", currentVolume.load() * 100 / 21);
        lv_label_set_text(volumeLabel, b);
        lv_slider_set_value(volumeSlider, currentVolume.load(), LV_ANIM_OFF);
        if (popupVolumeLabel) lv_label_set_text(popupVolumeLabel, b);
        if (popupVolumeSlider) lv_slider_set_value(popupVolumeSlider, currentVolume.load(), LV_ANIM_OFF);
    }
    const char* toneText = testToneActive ? "Тестовый тон: ВКЛ" : "Тестовый тон: ВЫКЛ";
    if (toneLabel && strcmp(lv_label_get_text(toneLabel), toneText))
    lv_label_set_text(toneLabel, toneText);
    if (statusLabel && strcmp(lv_label_get_text(statusLabel), currentStatus)) lv_label_set_text(statusLabel, currentStatus);
}
#include "spectrum_analysis.h"

static void refreshSpectrum() {
    static uint32_t lastRefresh = 0;
    static uint8_t heights[SPECTRUM_BANDS]{};
    const uint32_t now = millis();
    if (now - lastRefresh < 100 || displaySleeping || lv_screen_active() != mainScreen) return;
    lastRefresh = now;
    static int previousBufferWidth = -1;
    const int bufferWidth = (uint32_t(inputBufferPermille.load()) * INPUT_BUFFER_BAR_WIDTH + 500) / 1000;
    if (bufferWidth != previousBufferWidth) {
        previousBufferWidth = bufferWidth;
        lv_obj_set_width(inputBufferFill, bufferWidth);
    }
    const bool live = isPlaying && now - spectrumAt.load() < 350;
    for (int i = 0; i < SPECTRUM_BANDS; ++i) {
        const uint8_t target = live ? spectrumLevels[i].load() : 0;
        uint8_t height = target;
        if (live && target < heights[i]) height = std::max(int(target), int(heights[i]) - 4);
        // Квантуем высоту по 2 пикселя: чёткое движение без сглаживания.
        height = height / 2 * 2;
        if (height == heights[i]) continue;
        heights[i] = height;
        lv_obj_set_height(spectrumBars[i], height);
        lv_obj_set_y(spectrumBars[i], 276 - height);
    }
}
// Отмечаю получение декодированного блока, а не просто открытого соединения.
// Не изменяю сэмплы и разрешаю библиотеке продолжить обычную передачу в I2S.
// Этот признак подтверждает обработку аудиоданных, но не звук из динамика.
static std::atomic<uint32_t> lastDecodedAudioAt{0};
void audio_process_extern(int16_t* buff, uint16_t len, bool* continueI2S) {
    if (len) lastDecodedAudioAt.store(millis());
    analyzeSpectrum(buff, len, spectrumChannels.load(), 1.0f / 32768.0f);
    *continueI2S = true;
}


// В ESP32-audioI2S 4.0.1 отмечаю блок через актуальный обработчик.
// Он вызывается до регулировки громкости: нулевая громкость и тишина
// в эфире не должны выглядеть как обрыв. Сэмплы оставляю без изменений.
// Старый обработчик выше сохраняю для закреплённой сборки Schematik.
void audio_process_raw_samples(int32_t* outBuff, int16_t validSamples) {
    if (validSamples <= 0) return;
    lastDecodedAudioAt.store(millis());
    // 4.0.1: validSamples — число 32-битных слов, а не кадров.
    // Выходной PCM уже стерео, Q31; оригинальный буфер не изменяем.
    analyzeSpectrum(outBuff, uint16_t(validSamples / 2), 2, 1.0f / 2147483648.0f);
}

#include "web_remote.h"

/* ---------- Audio task ---------- */
#include "audio_pinout.h"
// Сохраняю пользовательскую шкалу 0–21 для экрана, пульта и памяти.
// Компенсирую квадратичную кривую библиотеки квадратным корнем:
// выбранная доля шкалы становится примерно такой же долей амплитуды.
// Использую 255 внутренних шагов, чтобы уменьшить погрешность округления.
static void applyLinearVolume(Audio& audio, int selected) {
    const int level = constrain(selected, 0, 21);
    const uint8_t internal = static_cast<uint8_t>(lroundf(255.0f * sqrtf(level / 21.0f)));
    audio.setVolume(internal, 0);
}

// В этой задаче владею объектом Audio: обрабатываю команды, обслуживаю поток, сохраняю настройки и повторяю подключение. GUI общается с ней через очереди и общие значения, не обращаясь к Audio напрямую.
void audio_task(void*) {

    // Не создаём аудиодрайвер и не открываем поток до исчезновения заставки.
    while (!startupScreenReady.load()) vTaskDelay(pdMS_TO_TICKS(10));
    Audio audio;
#if ESP_ARDUINO_VERSION_MAJOR >= 4
    // На ядре ESP32 4 использую API новой аудиобиблиотеки: декодер
    // обслуживаю на ядре 0, оставляя ядро 1 интерфейсу и сенсору.
    audio.setAudioTaskCore(0);
#endif
    if (!configureAudioPins(audio, 0)) fatal("I2S pinout failed");
    // Ограничиваю сетевое подключение: HTTP — 1 с, HTTPS — 3 с.
    // DNS и внутренние операции платформы могут занимать дополнительное время.
    audio.setConnectionTimeout(1000, 3000);
    audio.forceMono(true);
    audio.setVolumeSteps(255);
    applyLinearVolume(audio, currentVolume.load());

    bool wasRunning = false;
    // Test WAV uses Audio's own driver, never a second I2S driver.
    bool toneFsReady = false;
    // Формирую двухсекундный WAV: PCM, 48 кГц, два канала, 16 бит, тон 1 кГц. Воспроизвожу его только через существующий объект Audio.
    auto startTestTone = [&]() -> bool {
        if (!toneFsReady) {
            // Never format the user's flash if no filesystem is available.
            if (!LittleFS.begin(false)) {

                return false;
            }
            File f = LittleFS.open("/radio_test.wav", "w");
            if (!f) {

                return false;
            }
            const uint32_t bytes = 48000 * 2 * 2;
            auto u16 = [&](uint16_t v) {
                f.write(uint8_t(v));
                f.write(uint8_t(v >> 8));
            };
            auto u32 = [&](uint32_t v) {
                u16(v);
                u16(v >> 16);
            };
            f.write((const uint8_t*)"RIFF", 4);
            u32(36 + bytes);
            f.write((const uint8_t*)"WAVEfmt ", 8);
            u32(16);
            u16(1);
            u16(2);
            u32(48000);
            u32(192000);
            u16(4);
            u16(16);
            f.write((const uint8_t*)"data", 4);
            u32(bytes);
            int16_t samples[480 * 2];
            for (int n = 0; n < 480; ++n) {
                int16_t v = int16_t(1000 * sinf(6.28318530718f * (n % 48) / 48.0f));
                samples[2*n] = samples[2*n+1] = v;
            }
            bool ok = true;
            for (int block = 0; block < 100; ++block) {
                if (f.write((const uint8_t*)samples, sizeof(samples)) != sizeof(samples)) {
                    ok = false;
                    break;
                }
                vTaskDelay(1);
            }
            f.flush();
            f.close();
            // Reopen: the writer's cached size may not reflect appended data.
            File verify = LittleFS.open("/radio_test.wav", "r");
            size_t writtenSize = verify ? verify.size() : 0;
            bool headerOk = false;
            if (verify && writtenSize >= 44) {
                uint8_t header[44];
                headerOk = verify.read(header, sizeof(header)) == sizeof(header)
                && memcmp(header, "RIFF", 4) == 0
                && memcmp(header + 8, "WAVE", 4) == 0
                && memcmp(header + 36, "data", 4) == 0;
            }
            if (verify) verify.close();


            if (!ok || !headerOk || writtenSize != 192044) return false;
            toneFsReady = true;
        }
        File check = LittleFS.open("/radio_test.wav", "r");
        if (!check) {

            return false;
        }
        size_t fileSize = check.size();
        check.close();
        bool accepted = audio.connecttoFS(LittleFS, "/radio_test.wav");

        return accepted;
    };
    // Mount/scan the card before deciding between MP3 and radio restore.
    String sdBookmark;
    bool sdRestorePending=true;
    sdWebScanRequested.store(true);
    bool restorePending = false;
    auto clearSdPosition = [&]() {
        SD_MMC.remove("/.saved-state");
        SD_MMC.remove("/.saved-state.bak");
        SD_MMC.remove("/.saved-state.tmp");
    };
    uint32_t sdCheckpointAt=millis();
    auto saveSdPosition = [&]() {
        int index=sdCurrent.load();
        if(!sdActive.load() || index<0 || index>=int(sdMp3Files.size())) return;
#if AUDIO_RESUME_SECONDS
        // 4.x connecttoFS expects seconds, not a byte offset.
        String position=String("s")+String(audio.getAudioCurrentTime());
#else
        uint32_t pos=audio.getFilePos();
        uint32_t buffered=audio.inBufferFilled();
        pos=pos>buffered ? pos-buffered : 0;
        String position=String(pos);
#endif
        File file=SD_MMC.open(sdMp3Files[index],FILE_READ);
        uint32_t size=file ? file.size() : 0; if(file) file.close();
        String record=position+"\n"+String(size)+"\n"+String(sdPaused.load()?1:0)+"\n"+sdMp3Files[index];
        // Write a complete temporary record before rotating the previous one.
        SD_MMC.remove("/.saved-state.tmp");
        File state=SD_MMC.open("/.saved-state.tmp",FILE_WRITE);
        if(state) {
            bool ok=state.print(record)==record.length();
            state.flush(); state.close();
            if(ok) {
                SD_MMC.remove("/.saved-state.bak");
                bool rotated=!SD_MMC.exists("/.saved-state") ||
                    SD_MMC.rename("/.saved-state","/.saved-state.bak");
                if(rotated) SD_MMC.rename("/.saved-state.tmp","/.saved-state");
            }
        }
        sdCheckpointAt=millis();
    };
    int persistedVolume = currentVolume;
    int observedVolume = currentVolume;
    uint32_t volumeChangedAt = millis();
    int appliedVolume = currentVolume.load();
    // Reapply the UI setting after each stream open, in the audio task only.
    // После открытия потока повторно применяю выбранную громкость — сохраняю проверенное исправление потери уровня при смене станции.
    auto applyStreamVolume = [&]() {
        const int requested = currentVolume.load();
        applyLinearVolume(audio, requested);
        appliedVolume = requested;

    };
    int persistedBright = currentBrightness.load(), observedBright = persistedBright;
    int persistedSleep = sleepChoice.load(), observedSleep = persistedSleep;
    uint32_t displayChangedAt = millis();
    uint32_t savedAlarmConfig=alarmConfig.load(), savedOffConfig=offConfig.load();
    uint32_t observedAlarmConfig=savedAlarmConfig, observedOffConfig=savedOffConfig, scheduleChangedAt=millis();
    // Повторные подключения начинаю с паузы 3 секунды и увеличиваю её до 30 секунд. wantPlayback отличает обрыв от намеренной остановки.
    uint32_t nextRetry=0;
    unsigned retryDelay=3000;
    uint32_t streamOpenedAt = 0;
    bool streamHadAudio = false;
    constexpr uint32_t AUDIO_WAIT_MS = 12000;
    // Останавливаю ранее игравший поток до внутреннего reconnect (5–6 с).
    constexpr uint32_t AUDIO_STALL_MS = 4000;
    bool recoveryAllowed = false;
    bool stoppedByTimer = false;
    // Начинаю каждую попытку с чистого признака аудио. Старый блок предыдущей
    // станции не должен превратить новое соединение в «воспроизведение».
    auto openRadioStream = [&](const char* url) {
        stoppedByTimer = false;
        clearNowData();
        lastDecodedAudioAt.store(0);
        streamHadAudio = false;
        streamOpenedAt = millis();
        wasRunning = false;
        isPlaying = false;

        bool accepted = audio.connecttohost(url);

        applyStreamVolume();
        if (!accepted) {
            streamOpenedAt = 0;
            if (!recoveryAllowed) wantPlayback = false;
            pushStatus(recoveryAllowed ? "Нет соединения: повтор позже" : "Нет соединения: выберите станцию");
        } else {
            pushStatus("Ожидание аудиоданных...");
        }
        nextRetry = millis() + retryDelay;
        return accepted;
    };
    // Локальный кэш только для аудиозадачи: не опрашиваем Wi-Fi
    // на каждом проходе. Первый замер выполняется сразу при старте цикла.
    bool audioWifiConnected = (WiFi.status() == WL_CONNECTED);
    uint32_t audioWifiCheckedAt = millis();
    for (;;) {
        const uint32_t wifiCheckNow = millis();
        if (uint32_t(wifiCheckNow - audioWifiCheckedAt) >= 100U) {
            audioWifiCheckedAt = wifiCheckNow;
            audioWifiConnected = (WiFi.status() == WL_CONNECTED);
        }
        // Записываю избранное вне GUI; прочие настройки сохраняю после секунды без изменений, а не на каждом движении ползунка.
        if(favoritesDirty.exchange(false)) for(int j=0;j<FAVORITES_COUNT;j++) {
            char key[8];
            snprintf(key,sizeof(key),"favU%d",j);
            const int id = favoriteIds[j].load();
            radioPrefs.putString(key, (id >= 0 && id < ALL_STATIONS_COUNT) ? ALL_STATIONS[id].url : "");
        }
        // Проверяю таймер радио с разностью знаковых миллисекунд. По окончании отменяю намерение воспроизведения и автоматическое восстановление.
        uint32_t deadline=radioDeadline.load();
        if(!sdActive.load() && deadline && int32_t(millis()-deadline)>=0) {
            radioDeadline=0;
            wantPlayback=false;
            restorePending=false;
            audio.stopSong();
            isPlaying=false;
            stoppedByTimer = true;
            streamOpenedAt = 0;
            lastDecodedAudioAt.store(0);
            wasRunning = false;
            pushStatus("Таймер: радио выключено");
        }
        int desiredVolume = currentVolume.load();
        if (appliedVolume != desiredVolume) {
            applyLinearVolume(audio, desiredVolume);
            appliedVolume = desiredVolume;
        }
        uint32_t ac=alarmConfig.load(), oc=offConfig.load();
        if(ac!=observedAlarmConfig || oc!=observedOffConfig) { observedAlarmConfig=ac; observedOffConfig=oc; scheduleChangedAt=millis(); }
        if(millis()-scheduleChangedAt>1500) {
            if(savedAlarmConfig!=observedAlarmConfig && radioPrefs.putUInt("alarmCfg",observedAlarmConfig)) { savedAlarmConfig=observedAlarmConfig; radioPrefs.putString("alarmUrl",ALL_STATIONS[(observedAlarmConfig>>11)&255].url); }
            if(savedOffConfig!=observedOffConfig && radioPrefs.putUInt("offCfg",observedOffConfig)) savedOffConfig=observedOffConfig;
        }
        int bright = currentBrightness.load(), sleep = sleepChoice.load();
        if (bright != observedBright || sleep != observedSleep) {
            observedBright = bright;
            observedSleep = sleep;
            displayChangedAt = millis();
        }
        if (millis() - displayChangedAt > 1000) {
            if (persistedBright != observedBright && radioPrefs.putUChar("brightness", observedBright)) persistedBright = observedBright;
            if (persistedSleep != observedSleep && radioPrefs.putUChar("sleep", observedSleep)) persistedSleep = observedSleep;
        }
        if (observedVolume != currentVolume) {
            observedVolume = currentVolume;
            volumeChangedAt = millis();
        }
        if (persistedVolume != observedVolume && millis() - volumeChangedAt > 1000) {
            if (radioPrefs.putUChar("volume", observedVolume)) persistedVolume = observedVolume;
        }
        if (restorePending && audioWifiConnected) {
            restorePending = false;
            sendCommand({
                savedStation >= 100 ? CMD_PLAY_ALL : CommandType(CMD_PLAY_FAV_0 + savedStation), uint8_t(savedStation >= 100 ? savedStation-100 : 0)
            });
        }
        auto startSd = [&](int index, int32_t resumePos=-1) {
            wantPlayback=false; restorePending=false; testToneActive=false;
            recoveryAllowed=false; stoppedByTimer=true; streamOpenedAt=0;
            audio.stopSong(); isPlaying=false; wasRunning=false;
            lastDecodedAudioAt.store(0); sdEnded=false; sdPaused=false;
            sdElapsed=0; sdDuration=0;
            if(index>=0 && index<int(sdMp3Files.size())) {
                sdCurrent=index;
                sdActive=audio.connecttoFS(SD_MMC,sdMp3Files[index].c_str(),resumePos);
                applyStreamVolume();
                pushStatus(sdActive.load()?"MP3: воспроизведение":"Не удалось открыть MP3");
            } else sdActive=false;
        };
        if(sdRestorePending && (sdScanState.load()==2 || sdScanState.load()==3)) {
            sdRestorePending=false;
            if(sdScanState.load()==2) {
                File state=SD_MMC.open("/.saved-state",FILE_READ);
                if(!state) state=SD_MMC.open("/.saved-state.bak",FILE_READ);
                if(state) {
                    if(state.size()<=4096) sdBookmark=state.readString();
                    state.close();
                }
            }
            restorePending=sdBookmark.isEmpty() && savedStation>=0;
            int a=sdBookmark.indexOf('\n'), b=sdBookmark.indexOf('\n',a+1), d=sdBookmark.indexOf('\n',b+1);
            if(a>0 && b>a && d>b) {
                String path=sdBookmark.substring(d+1);
                auto it=std::find(sdMp3Files.begin(),sdMp3Files.end(),path);
                if(it!=sdMp3Files.end()) {
                    File f=SD_MMC.open(path,FILE_READ);
                    uint32_t size=f ? f.size() : 0; if(f) f.close();
                    String position=sdBookmark.substring(0,a);
                    bool seconds=position.startsWith("s");
                    uint32_t pos=strtoul(position.c_str()+(seconds?1:0),nullptr,10);
#if AUDIO_RESUME_SECONDS
                    bool compatible=seconds && pos<=INT32_MAX;
#else
                    bool compatible=!seconds && pos<size && pos<=INT32_MAX;
#endif
                    if(size && size==strtoul(sdBookmark.substring(a+1,b).c_str(),nullptr,10) && compatible) {
                        int index=int(it-sdMp3Files.begin());
                        startSd(index,int32_t(pos));
                        if(sdActive.load()) {
                            if(sdActive.load() && sdBookmark.substring(b+1,d)=="1") {audio.pauseResume(); sdPaused=true;}
                        }
                    }
                }
            }
        }
        int sdCmd=sdAction.exchange(0);
        if(sdCmd) sdRestorePending=false;
        if(sdCmd==1 && sdScanState.load()==2 && !sdMp3Files.empty()) {
            if(sdActive.load() && sdPaused.load() && sdCurrent.load()==sdRequested.load()) {
                audio.pauseResume(); sdPaused=false; saveSdPosition();
            } else {startSd(sdRequested.load()); saveSdPosition();}
        } else if(sdCmd==2 && sdActive.load()) {
            audio.pauseResume(); sdPaused=!sdPaused.load(); saveSdPosition();
        } else if(sdCmd==6) {
            clearSdPosition();
            if(sdActive.load()) audio.stopSong();
            sdActive=false; sdPaused=false; sdEnded=false;
            sdCurrent=-1; sdElapsed=0; sdDuration=0; sdSeekSeconds=-1;
            sdEditStopped.store(true);
        } else if(sdCmd==3 && sdActive.load()) {
            clearSdPosition();
            audio.stopSong(); sdActive=false; sdPaused=false; sdElapsed=0;
        }
        int seekSeconds=sdSeekSeconds.exchange(-1);
        if(seekSeconds>=0 && sdActive.load() && sdSeekTrack.load()==sdCurrent.load()) {
            // Both audio library versions support relative seeking in seconds.
            const int currentSeconds = int(audio.getAudioCurrentTime());
            audio.setTimeOffset(seekSeconds - currentSeconds);
        }
        Command c;
        if (xQueueReceive(audioCmdQueue, &c, 0) == pdTRUE) {
            // Пока сервер отвечал, могло накопиться несколько нажатий.
            // Выполняю последнее пожелание, а не подключаю все станции по очереди.
            Command newer;
            while (xQueueReceive(audioCmdQueue, &newer, 0) == pdTRUE) c = newer;
            if(c.type!=CMD_SET_VOLUME) {sdRestorePending=false; clearSdPosition();}
            if(sdActive.load() && c.type!=CMD_SET_VOLUME) {
                audio.stopSong(); sdActive=false; sdPaused=false; sdEnded=false;
                stoppedByTimer=true;
            }

            if (c.type == CMD_SET_VOLUME) {
                /* Legacy command: the atomic mailbox is authoritative. */
            }
            else if (c.type == CMD_TOGGLE) {
                restorePending = false;
                if(wantPlayback && !isPlaying) {
                    wantPlayback=false;
                    audio.stopSong();
                    pushStatus("Пауза");
                }
                else if (stoppedByTimer && currentStation >= 0) {
                    // stopSong() закрыл соединение: это не пауза, открываю URL заново.
                    const auto* station = selectedStation(currentStation);
                    if (station && station->url && *station->url) {
                        wantPlayback = true;
                        recoveryAllowed = false;
                        retryDelay = 3000;
                        nextRetry = millis() + retryDelay;
                        if (audioWifiConnected) {
                            pushStatus("Подключение...");
                            openRadioStream(station->url);
                        } else {
                            pushStatus("Нет сети: ожидание Wi-Fi");
                        }
                    }
                }
                else if (currentStation >= 0 && audio.pauseResume()) {
                    wantPlayback = audio.isRunning();
                    // После возобновления снова жду свежий декодированный блок.
                    lastDecodedAudioAt.store(0);
                    streamOpenedAt = wantPlayback ? millis() : 0;
                    isPlaying = false;
                    wasRunning = false;
                    pushStatus(wantPlayback ? "Ожидание аудиоданных..." : "Пауза");
                }
            }
            else if (c.type == CMD_TEST_TONE) {
                audio.stopSong();
                isPlaying = false;
                currentStation = -1;

                testToneActive = !testToneActive;
                wasRunning = false;
                bool testFailed = false;
                if (testToneActive && !startTestTone()) {
                    testFailed = true;
                    testToneActive = false;
                    pushStatus("Ошибка запуска теста");
                }

                if (!testFailed) pushStatus(testToneActive ? "Тестовый тон 1000 Гц" : "Тестовый тон выключен");
            }
            else if (c.type == CMD_STOP) {
                wantPlayback=false;
                restorePending=false;
                testToneActive = false;
                audio.stopSong();
                currentStation = -1;
                isPlaying = false;
                pushStatus("Остановлено");
            }
            else {
                restorePending = false;
                wantPlayback=true;
                recoveryAllowed=false;
                nextRetry=millis()+3000;
                retryDelay=3000;
                int i;
                const char* url;
                const char* name;
                if (c.type == CMD_PLAY_ALL) {
                    i = c.value;
                    if (i >= ALL_STATIONS_COUNT) {

                        continue;
                    }
                    url = ALL_STATIONS[i].url;
                    name = ALL_STATIONS[i].name;
                    currentStation = 100 + i;
                } else {
                    i = int(c.type) - int(CMD_PLAY_FAV_0);
                    int id=favoriteIds[i];
                    if(id<0) {
                        wantPlayback=false;
                        continue;
                    }
                    url = ALL_STATIONS[id].url;
                    name = ALL_STATIONS[id].name;
                    currentStation = 100+id;
                }
                testToneActive = false;
                audio.stopSong();
                isPlaying = false;
                if (!url || !*url) pushStatus("Нет URL");
                else if (!audioWifiConnected) pushStatus("Нет Wi-Fi");
                else {

                    pushStatus("Подключение...");
                    // NVS может блокировать выполнение и доступ к flash.
                    // Сохраняю выбор только при остановленном старом потоке,
                    // до открытия нового: не задерживаю первые audio.loop().
                    // Сохраняется последняя выбранная станция, даже если сервер
                    // сейчас недоступен; повторный выбор того же URL не пишет NVS.
                    if (savedUrl != url && radioPrefs.putString("station", url)) savedUrl = url;
                    openRadioStream(url);
                }
            }
        }
        // Не вхожу в очередное обслуживание старого потока, если уже есть
        // команда пользователя: сначала переключаю станцию или ставлю паузу.
        if (uxQueueMessagesWaiting(audioCmdQueue)) {
            vTaskDelay(1);
            continue;
        }
        // Соединение может числиться работающим, но не давать аудиоблоков.
        // Проверяю ДО audio.loop(): обрыв останавливаю раньше встроенного
        // переподключения библиотеки. После остановки оставляю паузу
        // до следующей попытки. GUI по-прежнему работает в отдельной задаче.
        uint32_t decodedAt = lastDecodedAudioAt.load();
        if (decodedAt) {
            streamHadAudio = true;
            recoveryAllowed = true;
        }
        uint32_t audioReference = decodedAt ? decodedAt : streamOpenedAt;
        if (wantPlayback && streamOpenedAt && millis() - audioReference >= (streamHadAudio ? AUDIO_STALL_MS : AUDIO_WAIT_MS)) {

            audio.stopSong();
            isPlaying = false;
            streamOpenedAt = 0;
            lastDecodedAudioAt.store(0);
            nextRetry = millis() + retryDelay;
            retryDelay = min(retryDelay * 2, 30000U);
            // Если станция ни разу не дала аудио, не запускаю её по кругу.
            // Восстановление сохраняю только для уже игравшего потока.
            if (!recoveryAllowed) {
                wantPlayback = false;
                pushStatus("Нет аудиоданных: выберите станцию");
            } else {
                pushStatus("Обрыв аудио: повтор позже");
            }
        }
        spectrumSampleRate.store(audio.getSampleRate());
        spectrumChannels.store(audio.getChannels());
        audio.loop();
        // Не обращаемся к Audio из GUI и не меняем обслуживание потока.
        static uint32_t lastBufferSample = 0;
        const uint32_t bufferSampleAt = millis();
        if (bufferSampleAt - lastBufferSample >= 500) {
            lastBufferSample = bufferSampleAt;
            const uint32_t filled = audio.inBufferFilled();
            const uint32_t free = audio.inBufferFree();
            const uint32_t total = filled + free;
            inputBufferPermille.store(total ? uint16_t(uint64_t(filled) * 1000 / total) : 0);
        }
        if(sdActive.load()) {
            sdElapsed=audio.getAudioCurrentTime(); sdDuration=audio.getAudioFileDuration();
            if(!sdPaused.load() && millis()-sdCheckpointAt>=30000) saveSdPosition();
            // EOF callback differs between audio library versions. Also handle
            // a completed local playback directly, in the Audio-owning task.
            // Stop clears sdActive above; Pause must never advance the playlist.
            const bool ended = sdEnded.exchange(false);
            if(!sdPaused.load() && (ended || !audio.isRunning())) {
                int next=sdCurrent.load()+1;
                if(next<int(sdMp3Files.size())) {startSd(next); saveSdPosition();}
                else {sdActive=false; sdPaused=false; audio.stopSong(); clearSdPosition();}
            }
            vTaskDelay(1); continue;
        }
        nowRate.store(audio.getSampleRate());
        nowBitrate.store(audio.getBitRate());
        nowChannels.store(audio.getChannels());
        bool running = audio.isRunning();
        if (testToneActive) {
            if (!running && !startTestTone()) {
                testToneActive = false;
                pushStatus("Ошибка тестового WAV");
            }
            vTaskDelay(1);
            continue;
        }
        if (wantPlayback && currentStation>=0 && !audioWifiConnected) {
            if(running) audio.stopSong();
            running=false;
            isPlaying=false;
            if(wasRunning) pushStatus("Нет сети: ожидание Wi-Fi");
        }
        // Переподключаюсь только при сохранённом намерении играть и доступном Wi-Fi. После открытия снова применяю текущую громкость.
        if(wantPlayback && !running && currentStation>=0 && audioWifiConnected && int32_t(millis()-nextRetry)>=0) {
            const auto* station=selectedStation(currentStation);
            if(station) {
                pushStatus("Переподключение...");
                audio.stopSong();
                openRadioStream(station->url);
            }
            nextRetry=millis()+retryDelay;
            retryDelay=min(retryDelay*2,30000U);
        }
        // Состояние интерфейса зависит от свежих декодированных блоков,
        // а не от isRunning(), который включается уже при TCP-соединении.
        decodedAt = lastDecodedAudioAt.load();
        bool receivingAudio = wantPlayback && running && decodedAt &&
                              millis() - decodedAt < AUDIO_WAIT_MS;
        if (receivingAudio) retryDelay = 3000;
        isPlaying = receivingAudio;
        if (receivingAudio != wasRunning) {
            wasRunning = receivingAudio;
            if (receivingAudio) pushStatus("Воспроизведение");
            else if (wantPlayback && running) pushStatus("Ожидание аудиоданных...");
        }
        vTaskDelay(1);
    }
}
static void showStartupLogo();

/* ---------- GUI task ---------- */
// В этой задаче создаю LVGL и все экраны, опрашиваю касания и обслуживаю графику. Буферы рисования размещаю в PSRAM, а блок передачи — в памяти, доступной DMA.
void gui_task(void*) {


    lv_init();
    // Register extra PSRAM explicitly: build-time LV_MEM_SIZE overrides
    // must not leave the 98-row interface with only the original 192 KiB.
    constexpr size_t extraUiPoolBytes = 512U * 1024U;
    static void* extraUiPool = nullptr;
    // Retained for the entire GUI lifetime.
    extraUiPool = heap_caps_malloc(extraUiPoolBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!extraUiPool) fatal("LVGL extra PSRAM allocation failed");
    if (!lv_mem_add_pool(extraUiPool, extraUiPoolBytes))
    fatal("LVGL extra PSRAM pool registration failed");

    // Два буфера по 20 строк позволяют рисовать частями. Отдельный внутренний блок из 8 строк нужен для безопасной передачи через DMA.
    constexpr int bufPixels = 240 * 20;
    lv_color_t* buf1 = (lv_color_t*)heap_caps_malloc(bufPixels * sizeof(lv_color_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    lv_color_t* buf2 = (lv_color_t*)heap_caps_malloc(bufPixels * sizeof(lv_color_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    dmaLine = (uint16_t*)heap_caps_malloc(240 * DMA_BLOCK_ROWS * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!buf1 || !buf2 || !dmaLine) fatal("Display buffers failed");
    disp = lv_display_create(240, 320);
    if (!disp) fatal("lv_display_create failed");
    // LVGL RGB565 uses native uint16_t values; LovyanGFX must swap bytes
    // for the display wire format (otherwise grey appears green).
    gfx.setSwapBytes(true);
    lv_display_set_flush_cb(disp, flush);
    lv_display_set_buffers(disp, buf1, buf2, bufPixels * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    indev = lv_indev_create();
    if (!indev) fatal("lv_indev_create failed");
    lv_indev_set_display(indev, disp);
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touchRead);
    lv_timer_set_period(lv_indev_get_read_timer(indev), 10);

    buildMainScreen();

    buildListScreen();

    buildSettingsScreen();
    buildAlarmScreen();
    buildDebugScreen();
    buildGalleryScreen();

    buildWifiScreen();
    buildLocalWifiScreen();
    buildVolumePopup();
    // Экраны уже готовы: после заставки сразу рисуем интерфейс.
    showStartupLogo();

    lv_screen_load(mainScreen);
    refreshUi();
    lv_refr_now(disp);
    gfx.waitDMA();
    startupScreenReady.store(true);

    lastTouchAt = millis();
    uint32_t last = millis(), lastWifiCheck = 0, lastUiRefresh = 0;
    for (;;) {
        uint32_t now = millis();
        updateSdPlayer();
        lv_tick_inc(now - last);
        last = now;
        if (portalReturnRequested) {
            portalReturnRequested = false;
            lv_screen_load(mainScreen);
        }
        // Обрабатываю ограниченную очередь без ожидания сети или аудиопотока.
        RemoteSetting setting;
        if (remoteSettingsQueue && xQueueReceive(remoteSettingsQueue, &setting, 0) == pdTRUE) {
            if (setting.kind == 0) {
                setBrightness(setting.value);
                lv_slider_set_value(brightnessSlider, setting.value, LV_ANIM_OFF);
                char text[16]; snprintf(text,sizeof(text),"%d%%",setting.value);
                lv_label_set_text(brightnessLabel,text);
            } else if (setting.kind == 1) {
                sleepChoice.store(setting.value);
                lastTouchAt = millis();
                lv_slider_set_value(displaySleepSlider,setting.value,LV_ANIM_OFF);
                lv_label_set_text(sleepLabel,sleepNames[setting.value]);
            } else if (setting.kind == 2) {
                radioTimerChoice = setting.value;
                int minutes = radioMinutes[setting.value];
                radioDeadline.store(minutes ? millis()+minutes*60000UL : 0);
                lv_slider_set_value(radioTimerSlider,setting.value,LV_ANIM_OFF);
                char text[64];
                if(minutes) snprintf(text,sizeof(text),"Радио: стоп через %d мин",minutes);
                else snprintf(text,sizeof(text),"Таймер радио выключен");
                lv_label_set_text(timerLabel,text);
            } else {
                changeFavorite(setting.value, setting.kind == 3);
            }
        }
        UiEvent e;
        if (xQueueReceive(uiEventQueue, &e, 0) == pdTRUE) {
            if (e.type == UI_STATUS) {
                strlcpy(currentStatus, e.text, sizeof(currentStatus));
                portENTER_CRITICAL(&webStatusMux);
                strlcpy(webStatus, e.text, sizeof(webStatus));
                portEXIT_CRITICAL(&webStatusMux);
            }
            else if (e.type == UI_METADATA) {
                /* Metadata is not displayed yet. */
            }
        }
        if (now - lastUiRefresh >= 20) {
            lastUiRefresh = now;
            refreshUi();
            refreshSpectrum();
        }
        if (now - lastWifiCheck > 1000) {
            lastWifiCheck = now;
            updateWifiIndicator();
            if (remoteAddressLabel) {
                char address[80];
                if (WiFi.status() == WL_CONNECTED) {
                    const IPAddress ip = WiFi.localIP();
                    snprintf(address, sizeof(address), "Веб-пульт:\nhttp://%u.%u.%u.%u:8080",
                             unsigned(ip[0]), unsigned(ip[1]), unsigned(ip[2]), unsigned(ip[3]));
                } else strlcpy(address, "Веб-пульт: нет Wi-Fi", sizeof(address));
                if (strcmp(lv_label_get_text(remoteAddressLabel), address))
                    lv_label_set_text(remoteAddressLabel, address);
            }
            updateBatteryIndicator();
            time_t t=time(nullptr);
            struct tm local;
            localtime_r(&t,&local);
            checkSchedules(t,local);
            char b[64];
            uint32_t d=radioDeadline.load();
            if(t>1700000000) snprintf(b,sizeof(b),"%02d:%02d",local.tm_hour,local.tm_min);
            else snprintf(b,sizeof(b),"--:--");
            if(strcmp(lv_label_get_text(clockLabel),b)) lv_label_set_text(clockLabel,b);
            if(d) snprintf(b,sizeof(b),"Стоп: %lu мин",(unsigned long)((uint32_t)(d-millis())/60000+1));
            else b[0] = '\0';
            if(strcmp(lv_label_get_text(radioCountdownLabel),b)) lv_label_set_text(radioCountdownLabel,b);
            if(!d && radioTimerChoice) {
                radioTimerChoice=0;
                lv_slider_set_value(radioTimerSlider,0,LV_ANIM_OFF);
                lv_label_set_text(timerLabel,"Таймер радио выключен");
            }
        }
        // LVGL обслуживает касания и перерисовку; сон ниже выключает только подсветку, аудиозадача продолжает работать.
        refreshLocalWifi();
        updateGallery();
        lv_timer_handler();
        if (pendingPhotoSwipe) {
            const int action = pendingPhotoSwipe;
            pendingPhotoSwipe = 0;
            if (photoViewing && lv_screen_active() == galleryScreen) {
                if (action == 2) photoExit(nullptr);
                else if (!photoFiles.empty() && photoCurrent >= 0) {
                    const int count = int(photoFiles.size());
                    photoShow((photoCurrent + action + count) % count);
                }
            }
        }
        if (volumePopupOpen && millis() - volumePopupActivity >= 3000)
            closeVolumePopup(nullptr);
        if (pendingLocalWifiSwipe) {
            pendingLocalWifiSwipe = false;
            slideToScreen(localWifiScreen, false);
            if (!localWifiBusy.load()) localWifiStart(false);
        }
        if (pendingPlayingSwipe) {
            slideToScreen(pendingPlayingSwipe > 0 ? debugScreen : mainScreen, pendingPlayingSwipe > 0);
            pendingPlayingSwipe = 0;
        }
        if (pendingScreenSwipe) {
            slideToScreen(pendingScreenSwipe, swipeMovesLeft);
            pendingScreenSwipe = nullptr;
        }
        lv_obj_t* activeScreen = lv_screen_active();
        bool hideVolume = activeScreen == settingsScreen || activeScreen == localWifiScreen || activeScreen == wifiScreen
                       || (activeScreen == galleryScreen && photoViewing);
        if (hideVolume) {
            lv_obj_add_flag(globalVolumeButton, LV_OBJ_FLAG_HIDDEN);
            if (volumePopupOpen) closeVolumePopup(nullptr);
        } else lv_obj_remove_flag(globalVolumeButton, LV_OBJ_FLAG_HIDDEN);
        uint32_t timeout = sleepMinutes[sleepChoice.load()] * 60000UL;
        if (!displaySleeping && !suppressWakeTouch && timeout && millis() - lastTouchAt >= timeout) {
            displaySleeping = true;
            ledcWrite(TFT_BL, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
/* ---------- Колбэки метаданных ---------- */
// Получаю уведомление об окончании файла для перехода к следующему треку.
void audio_eof_mp3(const char* info) {
    if(sdActive.load()) sdEnded.store(true);

}
// Передаю полученное от сервера название станции в очередь метаданных.
void audio_showstation(const char* info) {

    storeNowText(nowBroadcast, sizeof(nowBroadcast), info);
}
void audio_showstreamtitle(const char* info) {

    storeNowText(nowTrack, sizeof(nowTrack), info);
}
void audio_icydescription(const char* info) {

    storeNowText(nowDescription, sizeof(nowDescription), info);
}
void audio_icyurl(const char* info) {

    storeNowText(nowWebsite, sizeof(nowWebsite), info);
}
// Пиксельная заставка: перспективное кольцо, без изменения шрифта GUI.
static void showStartupLogo() {
    constexpr int size = 192, center = size / 2, segments = 96;
    constexpr float pi = 3.14159265358979323846f;
    lgfx::LGFX_Sprite frame(&gfx);
    frame.setColorDepth(16);
    gfx.waitDMA();
    gfx.fillScreen(TFT_BLACK);
    if (!frame.createSprite(size, size)) return; // Интерфейс важнее заставки.
    // Собственная пиксельная надпись «Радио»: рабочий шрифт интерфейса не меняем.
    // Пять букв, по 5 x 7 пикселей, масштаб 3, без сглаживания.
    static constexpr uint8_t title[5][7] = {
        {30,17,17,30,16,16,16}, // Р
        {0,0,14,1,15,17,15},   // а
        {0,0,14,10,10,31,17},  // д
        {0,0,17,19,21,25,17},  // и
        {0,0,14,17,17,17,14}   // о
    };
    struct Point { int x, y; };
    const uint32_t started = millis();
    constexpr uint32_t duration = 10000; // Сохраняем прежнее время запуска.
    for (;;) {
        const uint32_t elapsed = millis() - started;
        if (elapsed >= duration) break;
        // Короткие неподвижные начало и конец, три полных оборота между ними.
        float progress = (static_cast<float>(elapsed) - 500.0f) / 9000.0f;
        progress = fmaxf(0.0f, fminf(1.0f, progress));
        const float angle = progress * 6.0f * pi;
        const float ca = cosf(angle), sa = sinf(angle);
        auto project = [&](float radius, float t) -> Point {
            const float x = radius * cosf(t), y = radius * sinf(t);
            const float z = -x * sa;
            const float scale = 260.0f / (260.0f - z);
            return {center + static_cast<int>(lroundf(x * ca * scale)),
                    center + static_cast<int>(lroundf(y * scale))};
        };
        gfx.waitDMA();
        frame.fillSprite(TFT_BLACK);
        // Дальняя половина рисуется за надписью, ближняя — перед ней.
        for (int pass = 0; pass < 2; ++pass) {
            if (pass == 1) {
                constexpr int pixel = 3, textWidth = (5 * 6 - 1) * pixel;
                for (int letter = 0; letter < 5; ++letter)
                    for (int row = 0; row < 7; ++row)
                        for (int col = 0; col < 5; ++col)
                            if (title[letter][row] & (1U << (4 - col)))
                                frame.fillRect(center - textWidth / 2 + (letter * 6 + col) * pixel,
                                               center - 10 + row * pixel, pixel, pixel, TFT_WHITE);
            }
            for (int n = 0; n < segments; ++n) {
                const float t0 = n * 2.0f * pi / segments;
                const float t1 = (n + 1) * 2.0f * pi / segments;
                const float z = -68.5f * cosf((t0 + t1) * 0.5f) * sa;
                if ((z > 0.0f) != (pass == 1)) continue;
                Point a = project(72.0f, t0), b = project(72.0f, t1);
                Point c = project(65.0f, t1), d = project(65.0f, t0);
                const float light = 0.68f + 0.32f * (z / 72.0f);
                const uint32_t color = (static_cast<uint32_t>(30 * light) << 16)
                                     | (static_cast<uint32_t>(240 * light) << 8)
                                     | static_cast<uint32_t>(85 * light);
                frame.fillTriangle(a.x, a.y, b.x, b.y, c.x, c.y, color);
                frame.fillTriangle(a.x, a.y, c.x, c.y, d.x, d.y, color);
            }
        }
        frame.pushSprite((gfx.width() - size) / 2, (gfx.height() - size) / 2);
        delay(40); // Ограничиваем частоту; запуск аудио ждёт главного экрана.
    }
    gfx.waitDMA();
    frame.deleteSprite();
}
/* ---------- Setup ---------- */
// Выполняю однократный запуск: оборудование, очереди, сохранённые настройки и сеть, затем создаю аудиозадачу на ядре 0 и GUI на ядре 1. Заставка выполняется в GUI после создания экранов, без ожидания сети.
void setup() {
    pinMode(9, INPUT);
    analogSetPinAttenuation(9, ADC_11db);
    pinMode(PA_EN, OUTPUT);
    digitalWrite(PA_EN, LOW);
    // Disable runtime ESP-IDF logging as well as compile-time Arduino logs.
    esp_log_level_set("*", ESP_LOG_NONE);
    // Дисплей запускается перед прежней десятисекундной паузой.
    ledcAttach(TFT_BL, 5000, 8);
    ledcWrite(TFT_BL, 255);
    gfx.init();
    gfx.setRotation(0);
    gfx.setColorDepth(16);
    gfx.initDMA();
    gfx.setTextWrap(false, false);
    if (!psramFound()) fatal("PSRAM not found");
    audioCmdQueue = xQueueCreate(12, sizeof(Command));
    uiEventQueue = xQueueCreate(1, sizeof(UiEvent));
    if (!audioCmdQueue || !uiEventQueue) fatal("Queue failed");
    pinMode(TP_RST, OUTPUT);
    digitalWrite(TP_RST, LOW);
    delay(10);
    digitalWrite(TP_RST, HIGH);
    delay(50);
    pinMode(TP_INT, INPUT);
    Wire.begin(TP_SDA, TP_SCL);
    Wire.setClock(400000);
    wireDiagMutex = xSemaphoreCreateMutex();
    if (!wireDiagMutex) fatal("I2C diagnostic mutex failed");

    if (!es8311_init()) fatal("ES8311 init failed");
    stationCatalogInit();
    // Восстанавливаю станции по URL; старый формат сбрасываю один раз.
    for (int i = 0; i < FAVORITES_COUNT; ++i) favoriteIds[i] = -1;
    favoriteIds[0] = FAV_0;
    favoriteIds[1] = FAV_1;
    favoriteIds[2] = FAV_2;
    favoriteIds[3] = FAV_3;
    if (radioPrefs.begin("radio", false)) {
        if(stationCatalogLoaded) {
            const int defaults[]={FAV_0,FAV_1,FAV_2,FAV_3};
            for(int j=0;j<FAVORITES_COUNT;j++)favoriteIds[j]=-1;
            for(int j=0;j<4;j++)for(int i=0;i<ALL_STATIONS_COUNT;i++)if(stationUrlMatches(i,String(BUILTIN_STATIONS[defaults[j]].url)))favoriteIds[j]=i;
        }
        currentVolume = constrain(int(radioPrefs.getUChar("volume", 6)), 0, 21);
        currentBrightness = constrain(int(radioPrefs.getUChar("brightness", 100)), 15, 100);
        sleepChoice = constrain(int(radioPrefs.getUChar("sleep", 0)), 0, 6);
        uint32_t ac=radioPrefs.getUInt("alarmCfg",360), oc=radioPrefs.getUInt("offCfg",1320);
        if((ac&2047)>=1440) ac=360;
        String alarmUrl=radioPrefs.getString("alarmUrl", "");
        int station=-1;
        for(int i=0;i<ALL_STATIONS_COUNT;i++) if(stationUrlMatches(i,alarmUrl)) {station=i; break;}
        if(station<0) { for(int i=0;i<ALL_STATIONS_COUNT;i++) if(!strcmp(ALL_STATIONS[i].name,"Маяк")) station=i; }
        ac=(ac & ~(255UL<<11)) | (uint32_t(max(0,station))<<11);
        alarmConfig=ac; offConfig=(oc&2047)<1440?oc:1320;
        const bool resetStations = radioPrefs.getInt("stationFmt", 0) != STATION_STORAGE_VERSION;
        if (resetStations) {
            // Однократный сброс только станций. Wi-Fi и остальные настройки не трогаю.
            bool resetSaved = true;
            for (int j = 0; j < FAVORITES_COUNT; ++j) {
                char key[8];
                snprintf(key, sizeof(key), "fav%d", j);
                if (radioPrefs.isKey(key)) radioPrefs.remove(key);
                snprintf(key, sizeof(key), "favU%d", j);
                const int id = favoriteIds[j].load();
                const char* url = id >= 0 ? ALL_STATIONS[id].url : "";
                radioPrefs.putString(key, url);
                // Для пустой строки putString возвращает 0 даже при успехе.
                // Проверяю наличие ключа и прочитанное значение, а не длину записи.
                if (!radioPrefs.isKey(key) || radioPrefs.getString(key, "") != url) resetSaved = false;
            }
            if (radioPrefs.isKey("station") && !radioPrefs.remove("station")) resetSaved = false;
            if (radioPrefs.isKey("listVer")) radioPrefs.remove("listVer");
            if (resetSaved) radioPrefs.putInt("stationFmt", STATION_STORAGE_VERSION);
        } else {
            for (int j = 0; j < FAVORITES_COUNT; ++j) {
                char key[8];
                snprintf(key, sizeof(key), "favU%d", j);
                const String url = radioPrefs.getString(key, "");
                int id = -1;
                if (url.length()) {
                    for (int i = 0; i < ALL_STATIONS_COUNT; ++i) {
                        if (stationUrlMatches(i,url)) { id = i; break; }
                    }
                }
                favoriteIds[j] = id;
            }
        }
        savedUrl = radioPrefs.getString("station", "");
        if (savedUrl.length()) {
            for (int i = 0; i < ALL_STATIONS_COUNT; ++i)
            if (stationUrlMatches(i,savedUrl)) {
                savedStation = 100+i;
                break;
            }
        }
    }
    setBrightness(currentBrightness.load());
    // GUI запускается до блокирующего подключения Wi-Fi.

    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    wifiSavedInit();
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    WiFi.begin();
    if (xTaskCreatePinnedToCore(gui_task, "gui", 12288, nullptr, 5, nullptr, 1) != pdPASS) fatal("gui task");
    /* ---- WiFi: подключаемся к сохранённой сети, портал НЕ запускаем ---- */
    configTime(3*3600,0,"pool.ntp.org","time.google.com");
    // Moscow time
    // Association was initiated before GUI; no overlapping startup manager.

    // Разделяю звук и графику по ядрам, сохраняя проверенные размеры стеков и приоритеты. Эти размеры — рабочий стек задач, не сетевой буфер.
    if (xTaskCreatePinnedToCore(audio_task, "audio", 20480, nullptr, 2, nullptr, 0) != pdPASS) fatal("audio task");
    startWebRemote();
    if (xTaskCreatePinnedToCore(wifiSavedTask, "wifi_saved", 6144, nullptr, 1, nullptr, 0) != pdPASS) fatal("wifi saved task");

}
// Основная работа уже выполняется отдельными задачами. Здесь только освобождаю процессор задержкой, без дополнительного обслуживания Audio или LVGL.
void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}