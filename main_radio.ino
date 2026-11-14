/* ============================================================================
 *  LCDWiki ES3C28P — Internet Radio Panel v6
 *  - Автоподключение к сохранённой сети при старте (без автопортала)
 *  - Портал Wi-Fi только по кнопке
 *  - Исправлен краш на Хит-ФМ (стек аудио 20 КБ)
 * ==========================================================================*/
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <Preferences.h>
#include <WiFiManager.h>
#include <LovyanGFX.hpp>
#include <lvgl.h>
#include <Audio.h>
#include <esp_heap_caps.h>
#include <esp_system.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <esp_log.h>
#include <LittleFS.h>
#include <math.h>
#include <atomic>
#include <time.h>
LV_FONT_DECLARE(dejavu_16);
#include "stations.h"
constexpr int STATION_COUNT = FAVORITES_COUNT;
// Сохраняю распиновку встроенного звукового тракта: разрешение усилителя и линии цифрового звука I2S.
constexpr int PA_EN = 1, MCLK = 4, BCLK = 5, DOUT = 8, LRCK = 7;
constexpr int TP_SDA = 16, TP_SCL = 15, TP_RST = 18, TP_INT = 17, TFT_BL = 45;
static constexpr bool RADIO_DIAGNOSTICS = false;
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
// Logging is compiled out: arguments are not evaluated in the hot paths.
#define LOG(...) ((void)0)
#define LOGE(...) ((void)0)
// При невосстановимой ошибке отключаю усилитель и останавливаю выполнение. Не продолжаю запуск с отсутствующей памятью или неготовым оборудованием.
static void fatal(const char* text) {
    LOGE("FATAL", "%s", text);
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
    LOG("STATUS", "%s", text);
}
// Передаю метаданные тем же механизмом очереди. Сейчас GUI их принимает, но не выводит название композиции на экран.
static void pushMetadata(const char* text) {
    UiEvent e{
    };
    e.type = UI_METADATA;
    strlcpy(e.text, text, sizeof(e.text));
    xQueueOverwrite(uiEventQueue, &e);
    LOG("META", "%s", text);
}
/* ---------- ES8311 ---------- */
// Записываю одну пару «регистр — значение» в ES8311 по адресу 0x18. Блокировка защищает общую I2C-шину от одновременного обращения сенсора и кодека; ошибка возвращается вызывающему коду.
static bool codecWrite(uint8_t r, uint8_t v) {
    WireGuard guard;
    Wire.beginTransmission(0x18);
    Wire.write(r);
    Wire.write(v);
    uint8_t err = Wire.endTransmission();
    if (err) LOGE("CODEC", "reg=0x%02X value=0x%02X I2C error=%u", r, v, err);
    return err == 0;
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
            0x32,0xbf
        }
    };
    for (auto &r : run) if (!codecWrite(r[0], r[1])) return false;
    if (!codecWrite(0x00, 0x80)) return false;
    delay(20);
    LOG("CODEC", "ES8311 final REG00=0x80; DAC volume REG32=0xBF");
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
    x = constrain(240 - int(((r[1] & 15) << 8) | r[2]), 0, 239);
    y = constrain(320 - int(((r[3] & 15) << 8) | r[4]), 0, 319);
    return true;
}
/* ---------- LVGL ---------- */
static lv_display_t* disp = nullptr;
static lv_indev_t* indev = nullptr;
static uint16_t* dmaLine = nullptr;
// Храню указатели на один раз созданные экраны и элементы. Обращаюсь к объектам LVGL из GUI, не из аудиозадачи.
static lv_obj_t* mainScreen = nullptr;
static lv_obj_t* listScreen = nullptr;
static lv_obj_t* settingsScreen = nullptr;
static lv_obj_t* wifiScreen = nullptr;
static lv_obj_t* volumeSlider = nullptr;
static lv_obj_t* volumeLabel = nullptr;
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
static uint32_t lastTouchAt = 0;
static bool displaySleeping = false;
static bool suppressWakeTouch = false;
static void setTileText(lv_obj_t* label, const char* text, int width);
// Избранное храню как четыре индекса общего списка; -1 обозначает пустое место. dirty сообщает аудиозадаче, что список пора сохранить.
static std::atomic<int> favoriteIds[4]{
    {
        FAV_0
    },{
        FAV_1
    },{
        FAV_2
    },{
        FAV_3
    }
};
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
static lv_obj_t* timerLabel = nullptr;
static lv_obj_t* radioTimerSlider = nullptr;
static lv_obj_t* favoriteChecks[ALL_STATIONS_COUNT]{
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
// Меняю только состав четырёх избранных станций, не запускаю звук и не закрываю список. Если все места заняты, пятый выбор не принимаю; затем согласовываю все чекбоксы и подписи плиток.
static void favoriteEvent(lv_event_t* e) {
    int id = int(uintptr_t(lv_event_get_user_data(e)));
    if(id < 0 || id >= ALL_STATIONS_COUNT) return;
    // LVGL toggles the checkbox before VALUE_CHANGED is delivered.
    bool requested = lv_obj_has_state((lv_obj_t*)lv_event_get_target(e), LV_STATE_CHECKED);
    int slot = -1, empty = -1;
    for(int j=0;j<4;j++) {
        int n = favoriteIds[j].load();
        if(n == id) slot = j;
        if(n < 0 && empty < 0) empty = j;
    }
    bool changed = false;
    if(!requested && slot >= 0) {
        // Clear every matching slot, including duplicates in older saved data.
        for(int j=0;j<4;j++) if(favoriteIds[j].load() == id) favoriteIds[j] = -1;
        changed = true;
    } else if(requested && slot < 0 && empty >= 0) {
        favoriteIds[empty] = id;
        changed = true;
    }
    // Resync every checkbox, including a rejected fifth selection.
    for(int i=0;i<ALL_STATIONS_COUNT;i++) {
        if(!favoriteChecks[i]) continue;
        bool selected = false;
        for(int j=0;j<4;j++) if(favoriteIds[j].load() == i) selected = true;
        if(selected) lv_obj_add_state(favoriteChecks[i], LV_STATE_CHECKED);
        else lv_obj_remove_state(favoriteChecks[i], LV_STATE_CHECKED);
    }
    if(!changed) return;
    favoritesDirty = true;
    for(int j=0;j<4;j++) {
        int n=favoriteIds[j].load();
        setTileText(tileLabels[j], n>=0 ? ALL_STATIONS[n].name : "Не выбрано",96);
    }
}
static Preferences radioPrefs;
static int savedStation = -1;
static String savedUrl;
static lv_obj_t* nowTile = nullptr;
static lv_obj_t* nowLabel = nullptr;
static lv_obj_t* nowIcon = nullptr;
static lv_obj_t* favIcons[STATION_COUNT];
static lv_obj_t* listIcons[ALL_STATIONS_COUNT];
static lv_obj_t* wifiConfirm = nullptr;
// Преобразую внутренний номер в запись станции: 0–3 обозначают места избранного, 100 + индекс — общий список. Для пустого или неизвестного номера возвращаю nullptr.
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
    if (xQueueSend(audioCmdQueue, &c, 0) != pdTRUE) LOGE("UI", "command queue full");
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
        LOGE("UI", "create '%s' NULL", name);
        fatal("LVGL OOM");
    }
    return o;
}
// Создаю кнопку с базовым непрозрачным фоном, белым текстом и рабочим шрифтом. Размер и оформление конкретной кнопки задаю при построении экрана.
static lv_obj_t* mustButton(lv_obj_t* parent, const char* name) {
    lv_obj_t* o = lv_button_create(parent);
    if (!o) {
        LOGE("UI", "button '%s' NULL", name);
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
        LOGE("UI", "label '%s' NULL", name);
        fatal("LVGL OOM");
    }
    return o;
}
// Передаю изменённую область LVGL на дисплей построчно. Копирую строку из буфера рисования в внутреннюю DMA-память и жду окончания передачи перед повторным использованием этой строки.
static void flush(lv_display_t* d, const lv_area_t* a, uint8_t* px_map) {
    const uint16_t* pixels = (const uint16_t*)px_map;
    int w = a->x2 - a->x1 + 1;
    for (int y = a->y1; y <= a->y2; y++) {
        memcpy(dmaLine, pixels, w * 2);
        gfx.pushImageDMA(a->x1, y, w, 1, dmaLine);
        gfx.waitDMA();
        pixels += w;
    }
    lv_display_flush_ready(d);
}
// Передаю касание LVGL и сбрасываю счётчик бездействия. Первое касание спящего дисплея только включает подсветку; до отпускания пальца подавляю его, чтобы случайно не нажать кнопку.
static void touchRead(lv_indev_t*, lv_indev_data_t* data) {
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
    LOG("UI", "brightness=%d%% duty=%d", percent, duty);
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
    LOG("TONE GUI", "CLICKED; sending toggle; active snapshot=%d", int(testToneActive));
    Command c{
        CMD_TEST_TONE, 0
    };
    if (xQueueSend(audioCmdQueue, &c, 0) != pdTRUE) LOGE("TONE", "command queue full");
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
/* ---------- Портал Wi-Fi (только по кнопке) ---------- */
static volatile bool portalReturnRequested = false;
// Запускаю отдельную задачу портала Radio-Setup на срок до пяти минут. После сохранения сети перезагружаю устройство; при таймауте прошу GUI вернуть главный экран.
static void runWifiPortalTask(void*) {
    LOG("WIFI", "portal task started");
    WiFiManager wm;
    wm.setDebugOutput(RADIO_DIAGNOSTICS);
    wm.setConfigPortalTimeout(300);
    wm.setBreakAfterConfig(true);
    wm.setSaveConfigCallback([]() {
        LOG("WIFI", "config saved, rebooting...");
        vTaskDelay(pdMS_TO_TICKS(1000));
        ESP.restart();
    });
    if (!wm.startConfigPortal("Radio-Setup")) {
        LOGE("WIFI", "portal timeout");
        pushStatus("Таймаут настройки Wi-Fi");
        portalReturnRequested = true;
    }
    vTaskDelete(NULL);
}
// После подтверждения показываю инструкцию подключения и создаю задачу Wi-Fi-портала. Блокирующая работа портала не выполняется внутри LVGL.
static void startWifiConfirmed(lv_event_t*) {
    if (wifiConfirm) lv_obj_add_flag(wifiConfirm, LV_OBJ_FLAG_HIDDEN);
    LOG("UI", "wifi setup requested");
    if (wifiScreen) lv_screen_load(wifiScreen);
    if (xTaskCreatePinnedToCore(runWifiPortalTask, "wifi_portal", 8192, nullptr, 1, nullptr, 0) != pdPASS) {
        LOGE("WIFI", "cannot create portal task");
    }
}
// Закрываю подтверждение, не запускаю настройку сети.
static void cancelWifi(lv_event_t*) {
    lv_obj_add_flag(wifiConfirm, LV_OBJ_FLAG_HIDDEN);
}
// Показываю предупреждение перед открытием портала настройки сети.
static void wifiSetupEvent(lv_event_t*) {
    lv_obj_remove_flag(wifiConfirm, LV_OBJ_FLAG_HIDDEN);
}
/* ---------- Главный экран ---------- */
// Один раз создаю главный экран, индикаторы, текущую станцию и четыре плитки избранного. Координаты и цвета сохраняю; позже обновляю существующие объекты через refreshUi.
static void buildMainScreen() {
    mainScreen = lv_obj_create(NULL);
    if (!mainScreen) fatal("mainScreen alloc failed");
    lv_obj_set_style_pad_all(mainScreen, 0, 0);
    lv_obj_set_style_border_width(mainScreen, 0, 0);
    lv_obj_set_style_bg_color(mainScreen, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(mainScreen, &dejavu_16, 0);
    lv_obj_t* title = mustLabel(mainScreen, "title");
    lv_label_set_text(title, "i - Радио");
    lv_obj_set_style_text_color(title, lv_color_hex(0x4fc3f7), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);
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
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);
    clockLabel = mustLabel(mainScreen,"clock");
    lv_obj_set_width(clockLabel, 216);
    lv_obj_set_style_text_align(clockLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(clockLabel, LV_ALIGN_TOP_MID, 0, 272);
    lv_obj_set_style_text_color(clockLabel,lv_color_hex(0xb0b0b0),0);
    lv_label_set_text(clockLabel,"--:--");
    /* Плитки 2x2 */
    const int tileW = 104, tileH = 62, gap = 6;
    const int x0 = 12, y0 = 40;
    const uint32_t favoriteColors[4] = {
        0x1565C0, 0x1B5E20, 0xD96800, 0x606060
    };
    for (int i = 0; i < STATION_COUNT; i++) {
        int col = i % 2, row = i / 2;
        int x = x0 + col * (tileW + gap);
        int y = y0 + row * (tileH + gap);
        lv_obj_t* tile = mustButton(mainScreen, "tile");
        lv_obj_set_pos(tile, x, y);
        lv_obj_set_size(tile, tileW, tileH);
        lv_obj_set_style_bg_color(tile, lv_color_hex(favoriteColors[i]), 0);
        lv_obj_set_style_bg_opa(tile, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(tile, 0, 0);
        lv_obj_set_style_radius(tile, 0, 0);
        lv_obj_set_style_pad_all(tile, 0, 0);
        lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_shadow_width(tile, 0, 0);
        lv_obj_add_event_cb(tile, tileEvent, LV_EVENT_CLICKED, (void*)uintptr_t(i));
        lv_obj_t* lbl = mustLabel(tile, "tileLbl");
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffff), 0);
        lv_obj_set_size(lbl, tileW - 8, 42);
        lv_obj_set_pos(lbl, 4, 2);
        setTileText(lbl, favoriteIds[i]>=0 ? ALL_STATIONS[favoriteIds[i]].name : "Не выбрано", tileW - 8);
        favIcons[i] = makePlayIcon(tile);
        lv_obj_set_pos(favIcons[i], (tileW - 16) / 2, tileH - 23);
        setPlayIcon(favIcons[i], true);
        lv_obj_add_flag(favIcons[i], LV_OBJ_FLAG_HIDDEN);
        tileBtns[i] = tile;
        tileLabels[i] = lbl;
    }
    nowTile = mustButton(mainScreen, "nowPlaying");
    lv_obj_set_pos(nowTile, 12, 178);
    lv_obj_set_size(nowTile, 216, 38);
    lv_obj_set_style_pad_all(nowTile, 0, 0);
    lv_obj_set_style_radius(nowTile, 0, 0);
    lv_obj_set_style_bg_color(nowTile, lv_color_hex(0x404040), 0);
    lv_obj_add_event_cb(nowTile, toggleEvent, LV_EVENT_CLICKED, nullptr);
    nowLabel = mustLabel(nowTile, "nowName");
    lv_obj_set_pos(nowLabel, 4, 9);
    lv_obj_set_width(nowLabel, 180);
    lv_obj_set_style_text_color(nowLabel, lv_color_hex(0xffffff), 0);
    lv_label_set_long_mode(nowLabel, LV_LABEL_LONG_MODE_DOTS);
    nowIcon = makePlayIcon(nowTile);
    lv_obj_set_pos(nowIcon, 195, 17);
    /* Кнопки внизу */
    lv_obj_t* allBtn = mustButton(mainScreen, "allBtn");
    lv_obj_set_pos(allBtn, 12, 288);
    lv_obj_set_size(allBtn, 216, 30);
    lv_obj_set_style_bg_color(allBtn, lv_color_hex(0x000000), 0);
    lv_obj_set_style_radius(allBtn, 4, 0);
    lv_obj_set_style_border_width(allBtn, 0, 0);
    lv_obj_add_event_cb(allBtn, openListEvent, LV_EVENT_CLICKED, nullptr);
    lv_obj_t* allLbl = mustLabel(allBtn, "allLbl");
    lv_label_set_text(allLbl, "Станции");
    lv_obj_center(allLbl);
    lv_obj_set_style_text_color(allLbl, lv_color_hex(0x9090a0), 0);
    lv_obj_t* wifiBtn = mustButton(mainScreen, "wifiBtn");
    lv_obj_set_pos(wifiBtn, 204, 2);
    lv_obj_set_size(wifiBtn, 32, 32);
    lv_obj_set_style_pad_all(wifiBtn, 0, 0);
    lv_obj_set_style_bg_color(wifiBtn, lv_color_hex(0x000000), 0);
    lv_obj_set_style_radius(wifiBtn, 0, 0);
    lv_obj_set_style_bg_opa(wifiBtn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(wifiBtn, 0, 0);
    lv_obj_add_event_cb(wifiBtn, openSettingsEvent, LV_EVENT_CLICKED, nullptr);
    // Draw a gear, independent of font glyph coverage.
    for (int i = 0; i < 8; ++i) {
        lv_obj_t* tooth = lv_line_create(wifiBtn);
        static lv_point_precise_t teeth[8][2];
        float angle = i * 3.14159265f / 4;
        teeth[i][0] = {
            float(16 + 8*cosf(angle)), float(16 + 8*sinf(angle))
        };
        teeth[i][1] = {
            float(16 + 12*cosf(angle)), float(16 + 12*sinf(angle))
        };
        lv_line_set_points(tooth, teeth[i], 2);
        lv_obj_set_style_line_width(tooth, 4, 0);
        lv_obj_set_style_line_color(tooth, lv_color_hex(0xffffff), 0);
        lv_obj_clear_flag(tooth, LV_OBJ_FLAG_CLICKABLE);
    }
    lv_obj_t* ring = lv_obj_create(wifiBtn);
    lv_obj_remove_style_all(ring);
    lv_obj_set_pos(ring, 7, 7);
    lv_obj_set_size(ring, 18, 18);
    lv_obj_set_style_radius(ring, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(ring, 4, 0);
    lv_obj_set_style_border_color(ring, lv_color_hex(0xffffff), 0);
    lv_obj_clear_flag(ring, LV_OBJ_FLAG_CLICKABLE);
    statusLabel = mustLabel(mainScreen, "statusLabel");
    lv_label_set_text(statusLabel, currentStatus);
    lv_obj_set_style_text_color(statusLabel, lv_color_hex(0x8080a0), 0);
    lv_obj_set_width(statusLabel, 216);
    lv_obj_set_pos(statusLabel, 12, 252);
    lv_label_set_long_mode(statusLabel, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
}
/* ---------- Список станций ---------- */
// Создаю прокручиваемый список. В каждой строке чекбокс и кнопка воспроизведения являются соседними объектами, чтобы управление избранным не запускало станцию.
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
    lv_obj_t* backLbl = mustLabel(backBtn, "backLbl");
    lv_label_set_text(backLbl, "<");
    lv_obj_center(backLbl);
    lv_obj_set_style_text_color(backLbl, lv_color_hex(0xffffff), 0);
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
        LOG("BOOT", "station row %d/%d: %s", i + 1, ALL_STATIONS_COUNT, ALL_STATIONS[i].name);
        // Checkbox and playback button are siblings, not parent and child.
        lv_obj_t* row = lv_obj_create(list);
        if(!row) {
            LOGE("UI", "list row OOM at %d", i);
            break;
        }
        lv_obj_remove_style_all(row);
        lv_obj_set_size(row, 216, 42);
        lv_obj_remove_flag(row, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_t* btn = lv_button_create(row);
        if (!btn) {
            LOGE("UI", "list OOM at %d", i);
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
            LOGE("UI", "list label OOM at %d", i);
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
        for(int j=0;j<4;j++) if(favoriteIds[j]==i) lv_obj_add_state(cb,LV_STATE_CHECKED);
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
/* ---------- Настройки ---------- */
// Создаю громкость, яркость, сон дисплея, таймер радио, настройку сети и окно автора. Ползунки связаны с отдельными обработчиками; построение экрана не запускает их действия вручную.
static void buildSettingsScreen() {
    settingsScreen = lv_obj_create(NULL);
    if (!settingsScreen) fatal("settingsScreen alloc failed");
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
    lv_obj_t* backLbl = mustLabel(backBtn, "setBackLbl");
    lv_label_set_text(backLbl, "<");
    lv_obj_center(backLbl);
    lv_obj_set_style_text_color(backLbl, lv_color_hex(0xffffff), 0);
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
    lv_obj_set_pos(slider, 16, 128);
    lv_obj_set_size(slider, 208, 12);
    lv_slider_set_range(slider, 15, 100);
    lv_slider_set_value(slider, currentBrightness, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0x2a2a3a), LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0x4fc3f7), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0xffffff), LV_PART_KNOB);
    lv_obj_add_event_cb(slider, brightnessEvent, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_t* sleepTitle = mustLabel(settingsScreen, "sleepTitle");
    lv_label_set_text(sleepTitle, "Отключение экрана");
    lv_obj_set_pos(sleepTitle, 16, 150);
    sleepLabel = mustLabel(settingsScreen, "sleepValue");
    lv_obj_set_pos(sleepLabel, 16, 174);
    lv_label_set_text(sleepLabel, sleepNames[sleepChoice.load()]);
    lv_obj_t* sleepSlider = lv_slider_create(settingsScreen);
    lv_obj_set_pos(sleepSlider, 16, 204);
    lv_obj_set_size(sleepSlider, 208, 12);
    lv_slider_set_range(sleepSlider, 0, 6);
    lv_slider_set_value(sleepSlider, sleepChoice.load(), LV_ANIM_OFF);
    lv_obj_set_style_bg_color(sleepSlider, lv_color_hex(0x707070), LV_PART_MAIN);
    lv_obj_set_style_bg_color(sleepSlider, lv_color_hex(0x4fc3f7), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(sleepSlider, lv_color_hex(0xffffff), LV_PART_KNOB);
    lv_obj_add_event_cb(sleepSlider, sleepEvent, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_t* wifiBtn = mustButton(settingsScreen, "wifiSettings");
    timerLabel=mustLabel(settingsScreen,"radioTimer");
    lv_obj_set_pos(timerLabel,16,228);
    lv_label_set_text(timerLabel,"Таймер радио выключен");
    lv_obj_t* timerSlider=lv_slider_create(settingsScreen);
    radioTimerSlider=timerSlider;
    lv_obj_set_pos(timerSlider,16,258);
    lv_obj_set_size(timerSlider,208,12);
    lv_slider_set_range(timerSlider,0,4);
    lv_obj_add_event_cb(timerSlider,radioTimerEvent,LV_EVENT_VALUE_CHANGED,nullptr);
    lv_obj_set_pos(wifiBtn, 16, 282);
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
    if (prevStation != currentStation || prevPlaying != isPlaying) {
        prevStation = currentStation;
        prevPlaying = isPlaying;
        for (int i = 0; i < STATION_COUNT; ++i) {
            bool active = sameStation(currentStation, i) && isPlaying;
            lv_obj_set_style_border_width(tileBtns[i], active ? 2 : 0, 0);
            lv_obj_set_style_border_color(tileBtns[i], lv_color_hex(0xffffff), 0);
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
    }
    const char* toneText = testToneActive ? "Тестовый тон: ВКЛ" : "Тестовый тон: ВЫКЛ";
    if (toneLabel && strcmp(lv_label_get_text(toneLabel), toneText))
    lv_label_set_text(toneLabel, toneText);
    if (statusLabel && strcmp(lv_label_get_text(statusLabel), currentStatus)) lv_label_set_text(statusLabel, currentStatus);
}
/* ---------- Audio task ---------- */
#include "audio_pinout.h"
// В этой задаче владею объектом Audio: обрабатываю команды, обслуживаю поток, сохраняю настройки и повторяю подключение. GUI общается с ней через очереди и общие значения, не обращаясь к Audio напрямую.
void audio_task(void*) {
    LOG("AUDIO", "task start (stack 20K)");
    Audio audio;
    if (!configureAudioPins(audio, 0)) fatal("I2S pinout failed");
    audio.forceMono(true);
    audio.setVolume(currentVolume);
    LOG("I2S", "BCLK=%d LRCK=%d DOUT=%d MCLK=%d; PA_EN=%d (LOW=enabled); volume=%d/21", BCLK, LRCK, DOUT, MCLK, digitalRead(PA_EN), currentVolume.load());
    bool wasRunning = false;
    uint32_t toneStarts = 0;
    // Test WAV uses Audio's own driver, never a second I2S driver.
    bool toneFsReady = false;
    // Формирую двухсекундный WAV: PCM, 48 кГц, два канала, 16 бит, тон 1 кГц. Воспроизвожу его только через существующий объект Audio.
    auto startTestTone = [&]() -> bool {
        if (!toneFsReady) {
            // Never format the user's flash if no filesystem is available.
            if (!LittleFS.begin(false)) {
                LOGE("TONE", "LittleFS unavailable; test requires a filesystem partition");
                return false;
            }
            File f = LittleFS.open("/radio_test.wav", "w");
            if (!f) {
                LOGE("TONE FILE", "open for write failed");
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
            LOG("TONE FILE", "reopened header_ok=%d", int(headerOk));
            LOG("TONE FILE", "created bytes=%u expected=192044; writes_ok=%d", unsigned(writtenSize), int(ok));
            if (!ok || !headerOk || writtenSize != 192044) return false;
            toneFsReady = true;
        }
        File check = LittleFS.open("/radio_test.wav", "r");
        if (!check) {
            LOGE("TONE FILE", "open for read failed");
            return false;
        }
        size_t fileSize = check.size();
        check.close();
        uint32_t started = millis();
        bool accepted = audio.connecttoFS(LittleFS, "/radio_test.wav");
        ++toneStarts;
        if (toneStarts == 1 || !accepted || toneStarts % 5 == 0)
        LOG("TONE OPEN", "attempt=%lu bytes=%u accepted=%d elapsed=%lu ms (not proof of sound)",
        (unsigned long)toneStarts, unsigned(fileSize), int(accepted), (unsigned long)(millis()-started));
        return accepted;
    };
    bool restorePending = savedStation >= 0;
    int persistedVolume = currentVolume;
    int observedVolume = currentVolume;
    uint32_t volumeChangedAt = millis();
    int appliedVolume = currentVolume.load();
    // Reapply the UI setting after each stream open, in the audio task only.
    // После открытия потока повторно применяю выбранную громкость — сохраняю проверенное исправление потери уровня при смене станции.
    auto applyStreamVolume = [&]() {
        const int requested = currentVolume.load();
        audio.setVolume(requested);
        appliedVolume = requested;
        LOG("VOLUME", "stream open: selected=%d/21 applied=%u/21", requested, unsigned(audio.getVolume()));
    };
    int persistedBright = currentBrightness.load(), observedBright = persistedBright;
    int persistedSleep = sleepChoice.load(), observedSleep = persistedSleep;
    uint32_t displayChangedAt = millis();
    // Повторные подключения начинаю с паузы 3 секунды и увеличиваю её до 30 секунд. wantPlayback отличает обрыв от намеренной остановки.
    uint32_t nextRetry=0;
    unsigned retryDelay=3000;
    for (;;) {
        // Записываю избранное вне GUI; прочие настройки сохраняю после секунды без изменений, а не на каждом движении ползунка.
        if(favoritesDirty.exchange(false)) for(int j=0;j<4;j++) {
            char key[8];
            snprintf(key,sizeof(key),"fav%d",j);
            radioPrefs.putInt(key,favoriteIds[j]);
        }
        // Проверяю таймер радио с разностью знаковых миллисекунд. По окончании отменяю намерение воспроизведения и автоматическое восстановление.
        uint32_t deadline=radioDeadline.load();
        if(deadline && int32_t(millis()-deadline)>=0) {
            radioDeadline=0;
            wantPlayback=false;
            restorePending=false;
            audio.stopSong();
            isPlaying=false;
            pushStatus("Таймер: радио выключено");
        }
        int desiredVolume = currentVolume.load();
        if (appliedVolume != desiredVolume) {
            audio.setVolume(desiredVolume);
            appliedVolume = desiredVolume;
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
        if (restorePending && WiFi.status() == WL_CONNECTED) {
            restorePending = false;
            sendCommand({
                savedStation >= 100 ? CMD_PLAY_ALL : CommandType(CMD_PLAY_FAV_0 + savedStation), uint8_t(savedStation >= 100 ? savedStation-100 : 0)
            });
        }
        Command c;
        if (xQueueReceive(audioCmdQueue, &c, 0) == pdTRUE) {
            LOG("AUDIO CMD", "received type=%d value=%u queued=%u", int(c.type), c.value, unsigned(uxQueueMessagesWaiting(audioCmdQueue)));
            if (c.type == CMD_SET_VOLUME) {
                /* Legacy command: the atomic mailbox is authoritative. */
            }
            else if (c.type == CMD_TOGGLE) {
                restorePending = false;
                if(wantPlayback && !audio.isRunning()) {
                    wantPlayback=false;
                    audio.stopSong();
                    pushStatus("Пауза");
                }
                else if (currentStation >= 0 && audio.pauseResume()) {
                    isPlaying = audio.isRunning();
                    wantPlayback=isPlaying;
                    wasRunning = isPlaying;
                    pushStatus(isPlaying ? "Воспроизведение" : "Пауза");
                }
            }
            else if (c.type == CMD_TEST_TONE) {
                audio.stopSong();
                isPlaying = false;
                currentStation = -1;
                LOG("TONE CMD", "toggle previous=%d next=%d", int(testToneActive), int(!testToneActive));
                testToneActive = !testToneActive;
                wasRunning = false;
                bool testFailed = false;
                if (testToneActive && !startTestTone()) {
                    testFailed = true;
                    testToneActive = false;
                    pushStatus("Ошибка запуска теста");
                }
                LOG("TONE", "test=%s; 1000 Hz / 48000 Hz / stereo 16-bit; volume=%d/21", testToneActive ? "ON" : "OFF", currentVolume.load());
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
                nextRetry=millis()+3000;
                retryDelay=3000;
                int i;
                const char* url;
                const char* name;
                if (c.type == CMD_PLAY_ALL) {
                    i = c.value;
                    if (i >= ALL_STATIONS_COUNT) {
                        LOGE("AUDIO", "bad idx %d", i);
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
                else if (WiFi.status() != WL_CONNECTED) pushStatus("Нет Wi-Fi");
                else {
                    LOG("CONNECT", "%s | %s", name, url);
                    pushStatus("Подключение...");
                    bool connected = audio.connecttohost(url);
                    applyStreamVolume();
                    if (!connected) pushStatus("Ошибка");
                    else {
                        isPlaying = true;
                        if (savedUrl != url && radioPrefs.putString("station", url)) savedUrl = url;
                        char b[64];
                        snprintf(b, sizeof(b), "Играет: %s", name);
                        pushStatus(b);
                    }
                }
            }
        }
        audio.loop();
        bool running = audio.isRunning();
        if (testToneActive) {
            if (!running && !startTestTone()) {
                testToneActive = false;
                pushStatus("Ошибка тестового WAV");
            }
            vTaskDelay(1);
            continue;
        }
        if (wantPlayback && currentStation>=0 && WiFi.status()!=WL_CONNECTED) {
            if(running) audio.stopSong();
            running=false;
            isPlaying=false;
            if(wasRunning) pushStatus("Нет сети: ожидание Wi-Fi");
        }
        // Переподключаюсь только при сохранённом намерении играть и доступном Wi-Fi. После открытия снова применяю текущую громкость.
        if(wantPlayback && !running && currentStation>=0 && WiFi.status()==WL_CONNECTED && int32_t(millis()-nextRetry)>=0) {
            const auto* station=selectedStation(currentStation);
            if(station) {
                pushStatus("Переподключение...");
                audio.stopSong();
                audio.connecttohost(station->url);
                applyStreamVolume();
            }
            nextRetry=millis()+retryDelay;
            retryDelay=min(retryDelay*2,30000U);
        }
        if(running) retryDelay=3000;
        if (running != wasRunning) {
            wasRunning = running;
            isPlaying = running;
            if(running) pushStatus("Воспроизведение");
        }
        vTaskDelay(1);
    }
}
/* ---------- GUI task ---------- */
// В этой задаче создаю LVGL и все экраны, опрашиваю касания и обслуживаю графику. Буферы рисования размещаю в PSRAM, а строку передачи — в памяти, доступной DMA.
void gui_task(void*) {
    LOG("GUI", "task start");
    LOG("BOOT", "GUI task: before lv_init; compiled LV_MEM_SIZE=%u", (unsigned)LV_MEM_SIZE);
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
    LOG("BOOT", "LVGL extra PSRAM pool registered: %u bytes", (unsigned)extraUiPoolBytes);
    // Два буфера по 20 строк позволяют рисовать частями. Отдельная внутренняя строка нужна для безопасной передачи через DMA.
    constexpr int bufPixels = 240 * 20;
    lv_color_t* buf1 = (lv_color_t*)heap_caps_malloc(bufPixels * sizeof(lv_color_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    lv_color_t* buf2 = (lv_color_t*)heap_caps_malloc(bufPixels * sizeof(lv_color_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    dmaLine = (uint16_t*)heap_caps_malloc(240 * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!buf1 || !buf2 || !dmaLine) fatal("Display buffers failed");
    disp = lv_display_create(240, 320);
    if (!disp) fatal("lv_display_create failed");
    lv_display_set_flush_cb(disp, flush);
    lv_display_set_buffers(disp, buf1, buf2, bufPixels * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    indev = lv_indev_create();
    if (!indev) fatal("lv_indev_create failed");
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touchRead);
    LOG("BOOT", "building Main screen");
    buildMainScreen();
    LOG("BOOT", "building List screen");
    buildListScreen();
    LOG("BOOT", "building Settings screen");
    buildSettingsScreen();
    LOG("BOOT", "building Wifi screen");
    buildWifiScreen();
    LOG("BOOT", "loading main screen");
    lv_screen_load(mainScreen);
    refreshUi();
    LOG("GUI", "entering loop");
    lastTouchAt = millis();
    uint32_t last = millis(), lastWifiCheck = 0, lastUiRefresh = 0;
    for (;;) {
        uint32_t now = millis();
        lv_tick_inc(now - last);
        last = now;
        if (portalReturnRequested) {
            portalReturnRequested = false;
            lv_screen_load(mainScreen);
        }
        UiEvent e;
        if (xQueueReceive(uiEventQueue, &e, 0) == pdTRUE) {
            if (e.type == UI_STATUS) {
                strlcpy(currentStatus, e.text, sizeof(currentStatus));
            }
            else if (e.type == UI_METADATA) {
                /* Metadata is not displayed yet. */
            }
        }
        if (now - lastUiRefresh >= 20) {
            lastUiRefresh = now;
            refreshUi();
        }
        if (now - lastWifiCheck > 1000) {
            lastWifiCheck = now;
            updateWifiIndicator();
            updateBatteryIndicator();
            time_t t=time(nullptr);
            struct tm local;
            localtime_r(&t,&local);
            char b[64];
            uint32_t d=radioDeadline.load();
            if(t>1700000000) snprintf(b,sizeof(b),"%02d:%02d",local.tm_hour,local.tm_min);
            else snprintf(b,sizeof(b),"--:--");
            if(d) {
                size_t n=strlen(b);
                snprintf(b+n,sizeof(b)-n,"  Стоп: %lu мин",(unsigned long)((uint32_t)(d-millis())/60000+1));
            }
            if(strcmp(lv_label_get_text(clockLabel),b)) lv_label_set_text(clockLabel,b);
            if(!d && radioTimerChoice) {
                radioTimerChoice=0;
                lv_slider_set_value(radioTimerSlider,0,LV_ANIM_OFF);
                lv_label_set_text(timerLabel,"Таймер радио выключен");
            }
        }
        // LVGL обслуживает касания и перерисовку; сон ниже выключает только подсветку, аудиозадача продолжает работать.
        lv_timer_handler();
        uint32_t timeout = sleepMinutes[sleepChoice.load()] * 60000UL;
        if (!displaySleeping && !suppressWakeTouch && timeout && millis() - lastTouchAt >= timeout) {
            displaySleeping = true;
            ledcWrite(TFT_BL, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
/* ---------- Колбэки метаданных ---------- */
// Получаю уведомление об окончании файла. В этой версии обработчик оставляет только отключённый диагностический вызов.
void audio_eof_mp3(const char* info) {
    LOG("AUDIO EOF", "file ended: %s", info ? info : "");
}
// Получаю уведомление об окончании потока. Логику повторных попыток оставляю в аудиозадаче.
void audio_eof_stream(const char* info) {
    LOG("AUDIO EOF", "stream ended: %s", info ? info : "");
}
// Принимаю информационное сообщение библиотеки. Общая диагностика отключена макросом LOG.
void audio_info(const char* info) {
    if(info) LOG("AUDIO INFO", "%s", info);
}
// Передаю полученное от сервера название станции в очередь метаданных.
void audio_showstation(const char* info) {
    if (info) {
        char b[96];
        snprintf(b, sizeof(b), "%s", info);
        pushMetadata(b);
    }
}
// Передаю название композиции от сервера в очередь; отображение этой информации пока не реализовано.
void audio_showstreamtitle(const char* info) {
    if (info) {
        char b[96];
        snprintf(b, sizeof(b), "%s", info);
        pushMetadata(b);
    }
}
/* ---------- Setup ---------- */
// Выполняю однократный запуск: оборудование, очереди, сохранённые настройки и сеть, затем создаю аудиозадачу на ядре 0 и GUI на ядре 1. Порядок и десятисекундную паузу сохраняю.
void setup() {
    pinMode(9, INPUT);
    analogSetPinAttenuation(9, ADC_11db);
    pinMode(PA_EN, OUTPUT);
    digitalWrite(PA_EN, LOW);
    esp_log_level_set("*", ESP_LOG_NONE);
    delay(10000);
    // Keep the requested ten-second startup pause, silently.
    if (!psramFound()) fatal("PSRAM not found");
    audioCmdQueue = xQueueCreate(12, sizeof(Command));
    uiEventQueue = xQueueCreate(1, sizeof(UiEvent));
    if (!audioCmdQueue || !uiEventQueue) fatal("Queue failed");
    ledcAttach(TFT_BL, 5000, 8);
    ledcWrite(TFT_BL, 255);
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
    LOG("BOOT", "initializing ES8311");
    if (!es8311_init()) fatal("ES8311 init failed");
    LOG("BOOT", "initializing LovyanGFX");
    gfx.init();
    gfx.setRotation(0);
    gfx.setColorDepth(16);
    gfx.initDMA();
    gfx.setTextWrap(false, false);
    gfx.fillScreen(TFT_BLACK);
    LOG("BOOT", "LovyanGFX init ok");
    // Восстанавливаю настройки из NVS. Старые индексы избранного переношу через таблицу соответствия; последнюю станцию ищу по сохранённому URL.
    if (radioPrefs.begin("radio", false)) {
        currentVolume = constrain(int(radioPrefs.getUChar("volume", 6)), 0, 21);
        currentBrightness = constrain(int(radioPrefs.getUChar("brightness", 100)), 15, 100);
        sleepChoice = constrain(int(radioPrefs.getUChar("sleep", 0)), 0, 6);
        const bool migrateFavorites = radioPrefs.getInt("listVer", 0) < STATIONS_LIST_VERSION;
        bool migrationSaved = true;
        for(int j=0;j<4;j++) {
            char key[8];
            snprintf(key,sizeof(key),"fav%d",j);
            int id = favoriteIds[j].load();
            if(radioPrefs.isKey(key)) {
                id = radioPrefs.getInt(key, -1);
                if(migrateFavorites)
                id = (id >= 0 && id < int(sizeof(LEGACY_STATION_IDS)/sizeof(LEGACY_STATION_IDS[0])))
                ? LEGACY_STATION_IDS[id] : -1;
            }
            favoriteIds[j] = (id>=0 && id<ALL_STATIONS_COUNT) ? id : -1;
            if(migrateFavorites && !radioPrefs.putInt(key, favoriteIds[j].load())) migrationSaved = false;
        }
        if(migrateFavorites && migrationSaved) radioPrefs.putInt("listVer", STATIONS_LIST_VERSION);
        savedUrl = radioPrefs.getString("station", "");
        if (savedUrl.length()) {
            for (int i = 0; i < ALL_STATIONS_COUNT; ++i)
            if (savedUrl == ALL_STATIONS[i].url) {
                savedStation = 100+i;
                break;
            }
        }
    } else LOGE("SETTINGS", "cannot open saved settings");
    setBrightness(currentBrightness.load());
    /* ---- WiFi: подключаемся к сохранённой сети, портал НЕ запускаем ---- */
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    configTime(3*3600,0,"pool.ntp.org","time.google.com");
    // Moscow time
    WiFi.persistent(true);
    WiFiManager wm;
    wm.setDebugOutput(RADIO_DIAGNOSTICS);
    wm.setEnableConfigPortal(false);
    // портал только по кнопке
    wm.setConnectTimeout(30);
    // 30 секунд на попытку
    if (wm.autoConnect()) {
        LOG("WIFI", "connected to saved network, IP=%s", WiFi.localIP().toString().c_str());
    } else {
        LOG("WIFI", "no saved network, use button 'Настройка Wi-Fi'");
    }
    LOG("BOOT", "creating audio task");
    // Разделяю звук и графику по ядрам, сохраняя проверенные размеры стеков и приоритеты. Эти размеры — рабочий стек задач, не сетевой буфер.
    if (xTaskCreatePinnedToCore(audio_task, "audio", 20480, nullptr, 2, nullptr, 0) != pdPASS) fatal("audio task");
    LOG("BOOT", "creating GUI task");
    if (xTaskCreatePinnedToCore(gui_task, "gui", 12288, nullptr, 5, nullptr, 1) != pdPASS) fatal("gui task");
    LOG("BOOT", "setup done");
}
// Основная работа уже выполняется отдельными задачами. Здесь только освобождаю процессор задержкой, без дополнительного обслуживания Audio или LVGL.
void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}