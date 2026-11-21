// Original text glyphs plus local 1bpp service icons; main font unchanged.
LV_FONT_DECLARE(wifi_ui_font);
// Local Wi-Fi setup. Only GUI touches LVGL; the worker owns network operations.
static lv_obj_t *localWifiScreen, *localWifiList, *localWifiStatus;
static lv_obj_t *localWifiEditor, *localWifiPassword, *localWifiKeyboard, *localWifiName;
static std::atomic<bool> localWifiBusy{false};
static std::atomic<int> localWifiResult{0};
static bool localWifiConnecting = false;
static String localWifiNames[30];
static bool localWifiOpen[30];
static int localWifiCount;
static String localWifiSelected, localWifiKey;
// GUI-owned, bounded ASCII WPA password. Never log its contents.
static char localWifiPasswordBuffer[64] = {};
static size_t localWifiPasswordLength = 0;
static void localWifiClearPassword() {
    memset(localWifiPasswordBuffer, 0, sizeof(localWifiPasswordBuffer));
    localWifiPasswordLength = 0;
    lv_label_set_text(localWifiPassword, "");
}
// GUI-owned mailbox: never mutate widgets inside the keyboard event dispatch.
static char localWifiPendingKey[32] = {};
static bool localWifiKeyPending = false;
static void localWifiKeyboardEvent(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
    lv_obj_t* keyboard = (lv_obj_t*)lv_event_get_current_target(e);
    if (keyboard != localWifiKeyboard || lv_event_get_target(e) != keyboard) return;
    Serial.println("[WiFi keyboard] event entered");
    uint32_t id = lv_keyboard_get_selected_button(keyboard);
    if (id == LV_BUTTONMATRIX_BUTTON_NONE) return;
    const char* text = lv_keyboard_get_button_text(keyboard, id);
    if (!text || localWifiKeyPending) return;
    size_t n = strlen(text);
    if (n >= sizeof(localWifiPendingKey)) return;
    memcpy(localWifiPendingKey, text, n + 1);
    localWifiKeyPending = true;
    Serial.println("[WiFi keyboard] command queued");
}
static void localWifiProcessKey() {
    if (!localWifiKeyPending) return;
    localWifiKeyPending = false;
    char key[sizeof(localWifiPendingKey)];
    memcpy(key, localWifiPendingKey, sizeof(key));
    memset(localWifiPendingKey, 0, sizeof(localWifiPendingKey));
    const char* text = key;
    lv_obj_t* keyboard = localWifiKeyboard;
    if (lv_obj_has_flag(localWifiEditor, LV_OBJ_FLAG_HIDDEN)) return;
    Serial.println("[WiFi keyboard] processing command");
    // This keyboard has only our input callback; handle layouts explicitly.
    if (!strcmp(text, "abc")) {
        lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_TEXT_LOWER);
        return;
    }
    if (!strcmp(text, "ABC")) {
        lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_TEXT_UPPER);
        return;
    }
    if (!strcmp(text, "1#")) {
        lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_SPECIAL);
        return;
    }
    if (!strcmp(text, LV_SYMBOL_KEYBOARD) || !strcmp(text, LV_SYMBOL_CLOSE) ||
        !strcmp(text, LV_SYMBOL_OK) || !strcmp(text, LV_SYMBOL_LEFT) ||
        !strcmp(text, LV_SYMBOL_RIGHT) || !strcmp(text, LV_SYMBOL_NEW_LINE) ||
        !strcmp(text, "Enter") || !strcmp(text, "+/-")) return;
    if (!strcmp(text, LV_SYMBOL_BACKSPACE)) {
        if (localWifiPasswordLength) localWifiPasswordBuffer[--localWifiPasswordLength] = '\0';
    } else {
        // Wi-Fi password editor accepts printable ASCII, up to 63 bytes.
        for (const unsigned char* c = (const unsigned char*)text; *c; ++c)
            if (*c < 32 || *c > 126) return;
        size_t n = strlen(text);
        if (n > 63 - localWifiPasswordLength) return;
        memcpy(localWifiPasswordBuffer + localWifiPasswordLength, text, n);
        localWifiPasswordLength += n;
        localWifiPasswordBuffer[localWifiPasswordLength] = '\0';
    }
    char masked[64];
    memset(masked, '*', localWifiPasswordLength);
    masked[localWifiPasswordLength] = '\0';
    Serial.println("[WiFi keyboard] updating mask");
    lv_label_set_text(localWifiPassword, masked);
    Serial.println("[WiFi keyboard] mask updated");
}
static void localWifiWorker(void*) {
    int result;
    if (!localWifiConnecting) {
        int n = WiFi.scanNetworks(false, false, false, 120);
        localWifiCount = 0;
        for (int i = 0; i < n && localWifiCount < 30; ++i) {
            String name = WiFi.SSID(i);
            if (!name.length()) continue;
            bool duplicate = false;
            for (int j = 0; j < localWifiCount; ++j) if (localWifiNames[j] == name) duplicate = true;
            if (duplicate) continue;
            localWifiNames[localWifiCount] = name;
            localWifiOpen[localWifiCount++] = WiFi.encryptionType(i) == WIFI_AUTH_OPEN;
        }
        WiFi.scanDelete();
        result = n < 0 ? 2 : 1;
    } else {
        String previousName = WiFi.SSID(), previousKey = WiFi.psk();
        WiFi.setAutoReconnect(false);
        WiFi.persistent(false); // Save only after successful association.
        WiFi.disconnect(false, false);
        WiFi.begin(localWifiSelected.c_str(), localWifiKey.c_str());
        uint32_t started = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - started < 20000)
            vTaskDelay(pdMS_TO_TICKS(100));
        if (WiFi.status() == WL_CONNECTED) {
            // Persist the tested credentials without another disconnect.
            wifi_config_t config = {};
            strlcpy((char*)config.sta.ssid, localWifiSelected.c_str(), sizeof(config.sta.ssid));
            strlcpy((char*)config.sta.password, localWifiKey.c_str(), sizeof(config.sta.password));
            esp_wifi_set_storage(WIFI_STORAGE_FLASH);
            result = esp_wifi_set_config(WIFI_IF_STA, &config) == ESP_OK ? 3 : 5;
        } else {
            WiFi.disconnect(false, false);
            if (previousName.length()) WiFi.begin(previousName.c_str(), previousKey.c_str());
            result = 4;
        }
        WiFi.setAutoReconnect(true);
        WiFi.persistent(true);
        localWifiKey = "";
    }
    localWifiResult.store(result);
    vTaskDelete(nullptr);
}
static void localWifiStart(bool connect) {
    if (localWifiBusy.exchange(true)) return;
    localWifiConnecting = connect;
    lv_label_set_text(localWifiStatus, connect ? "Подключение…" : "Поиск сетей…");
    if (xTaskCreatePinnedToCore(localWifiWorker, "wifi_local", 6144, nullptr, 1, nullptr, 0) != pdPASS) {
        localWifiBusy = false;
        lv_label_set_text(localWifiStatus, "Не удалось запустить задачу");
    }
}
static void localWifiScanEvent(lv_event_t*) { localWifiStart(false); }
static void localWifiCancel(lv_event_t*) {
    localWifiClearPassword();
    lv_obj_add_flag(localWifiEditor, LV_OBJ_FLAG_HIDDEN);
}
static void localWifiConnectEvent(lv_event_t*) {
    if (localWifiBusy.load()) return;
    localWifiKey = localWifiPasswordBuffer;
    if (localWifiKey.length() && (localWifiKey.length() < 8 || localWifiKey.length() > 63)) {
        lv_label_set_text(localWifiStatus, "Пароль: от 8 до 63 символов");
        return;
    }
    localWifiCancel(nullptr);
    localWifiStart(true);
}
static void localWifiSelect(lv_event_t* e) {
    if (localWifiBusy.load()) return;
    int i = (int)(intptr_t)lv_event_get_user_data(e);
    localWifiSelected = localWifiNames[i];
    lv_label_set_text(localWifiName, localWifiSelected.c_str());
    localWifiClearPassword();
    lv_label_set_text(localWifiStatus, localWifiOpen[i] ? "Сеть без пароля" : "Введите пароль Wi-Fi");
    lv_keyboard_set_mode(localWifiKeyboard, LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_obj_remove_flag(localWifiEditor, LV_OBJ_FLAG_HIDDEN);
}
static lv_obj_t* localWifiButton(lv_obj_t* parent, const char* text, int x, int y, int w, lv_event_cb_t cb) {
    lv_obj_t* b = mustButton(parent, "localWifiButton");
    lv_obj_set_pos(b, x, y); lv_obj_set_size(b, w, 32);
    lv_obj_set_style_bg_color(b, lv_color_hex(0x00ABA9), 0);
    lv_obj_t* l = mustLabel(b, "localWifiCaption"); lv_label_set_text(l, text); lv_obj_center(l);
    lv_obj_add_event_cb(b, cb, LV_EVENT_SHORT_CLICKED, nullptr);
    return b;
}
static void buildLocalWifiScreen() {
    localWifiScreen = mustCreate(nullptr, "localWifiScreen");
    lv_obj_set_style_pad_all(localWifiScreen, 0, 0);
    lv_obj_set_style_border_width(localWifiScreen, 0, 0);
    lv_obj_set_style_bg_color(localWifiScreen, lv_color_hex(0), 0);
    lv_obj_set_style_text_color(localWifiScreen, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(localWifiScreen, &wifi_ui_font, 0);
    lv_obj_remove_flag(localWifiScreen, LV_OBJ_FLAG_SCROLLABLE);
    localWifiButton(localWifiScreen, "Назад", 4, 4, 108, backEvent);
    localWifiButton(localWifiScreen, "Обновить", 120, 4, 116, localWifiScanEvent);
    localWifiStatus = mustLabel(localWifiScreen, "localWifiStatus");
    lv_obj_set_pos(localWifiStatus, 6, 42); lv_obj_set_size(localWifiStatus, 228, 48);
    lv_label_set_text(localWifiStatus, "Доступные сети Wi-Fi");
    localWifiList = mustCreate(localWifiScreen, "localWifiList");
    lv_obj_set_pos(localWifiList, 0, 94); lv_obj_set_size(localWifiList, 240, 226);
    lv_obj_set_style_pad_all(localWifiList, 4, 0);
    lv_obj_set_style_bg_color(localWifiList, lv_color_hex(0), 0);
    lv_obj_set_style_border_width(localWifiList, 0, 0);
    lv_obj_set_scroll_dir(localWifiList, LV_DIR_VER);
    localWifiEditor = mustCreate(localWifiScreen, "localWifiEditor");
    lv_obj_set_pos(localWifiEditor, 0, 88); lv_obj_set_size(localWifiEditor, 240, 232);
    lv_obj_set_style_pad_all(localWifiEditor, 0, 0);
    lv_obj_set_style_border_width(localWifiEditor, 0, 0);
    lv_obj_set_style_bg_color(localWifiEditor, lv_color_hex(0), 0);
    lv_obj_remove_flag(localWifiEditor, LV_OBJ_FLAG_SCROLLABLE);
    localWifiName = mustLabel(localWifiEditor, "localWifiName");
    lv_obj_set_pos(localWifiName, 4, 0); lv_obj_set_size(localWifiName, 232, 22);
    lv_label_set_long_mode(localWifiName, LV_LABEL_LONG_MODE_CLIP);
    localWifiPassword = mustLabel(localWifiEditor, "localWifiPassword");
    lv_obj_set_pos(localWifiPassword, 4, 24); lv_obj_set_size(localWifiPassword, 232, 38);
    lv_label_set_long_mode(localWifiPassword, LV_LABEL_LONG_MODE_CLIP);
    lv_obj_set_style_bg_color(localWifiPassword, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_opa(localWifiPassword, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(localWifiPassword, lv_color_hex(0), 0);
    lv_obj_set_style_pad_all(localWifiPassword, 6, 0);
    localWifiClearPassword();
    localWifiButton(localWifiEditor, "Отмена", 4, 66, 96, localWifiCancel);
    localWifiButton(localWifiEditor, "Подключить", 104, 66, 132, localWifiConnectEvent);
    localWifiKeyboard = lv_keyboard_create(localWifiEditor);
    // Remove only the keyboard's built-in input callback, not class events.
    lv_obj_remove_event_cb(localWifiKeyboard, lv_keyboard_def_event_cb);
    // Keyboard constructor uses BOTTOM_MID; reset alignment before positioning.
    lv_obj_set_size(localWifiKeyboard, 240, 130);
    lv_obj_align(localWifiKeyboard, LV_ALIGN_TOP_LEFT, 0, 102);
    lv_obj_set_style_pad_all(localWifiKeyboard, 2, 0);
    lv_obj_set_style_pad_row(localWifiKeyboard, 3, 0);
    lv_obj_set_style_pad_column(localWifiKeyboard, 2, 0);
    lv_obj_set_style_bg_color(localWifiKeyboard, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(localWifiKeyboard, LV_OPA_COVER, 0);
    // Same crisp, opaque black-on-white keys in every interaction state.
    const lv_style_selector_t keyboardStates[] = {
        LV_STATE_DEFAULT, LV_STATE_CHECKED, LV_STATE_PRESSED,
        LV_STATE_CHECKED | LV_STATE_PRESSED, LV_STATE_FOCUSED,
        LV_STATE_FOCUS_KEY, LV_STATE_DISABLED
    };
    for (lv_style_selector_t state : keyboardStates) {
        lv_style_selector_t sel = LV_PART_ITEMS | state;
        lv_obj_set_style_bg_color(localWifiKeyboard, lv_color_hex(0xffffff), sel);
        lv_obj_set_style_text_color(localWifiKeyboard, lv_color_hex(0x000000), sel);
        lv_obj_set_style_bg_opa(localWifiKeyboard, LV_OPA_COVER, sel);
        lv_obj_set_style_text_opa(localWifiKeyboard, LV_OPA_COVER, sel);
        lv_obj_set_style_opa(localWifiKeyboard, LV_OPA_COVER, sel);
        lv_obj_set_style_text_font(localWifiKeyboard, &wifi_ui_font, sel);
        lv_obj_set_style_bg_grad_dir(localWifiKeyboard, LV_GRAD_DIR_NONE, sel);
        lv_obj_set_style_radius(localWifiKeyboard, 0, sel);
        lv_obj_set_style_shadow_width(localWifiKeyboard, 0, sel);
        lv_obj_set_style_border_width(localWifiKeyboard, 0, sel);
        lv_obj_set_style_outline_width(localWifiKeyboard, 0, sel);
        lv_obj_set_style_transition(localWifiKeyboard, nullptr, sel);
    }
    // Do not invoke LVGL textarea insert/password/cursor machinery.
    lv_keyboard_set_textarea(localWifiKeyboard, nullptr);
    lv_obj_add_event_cb(localWifiKeyboard, localWifiKeyboardEvent, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_add_flag(localWifiEditor, LV_OBJ_FLAG_HIDDEN);
}
static void openLocalWifi() {
    lv_screen_load(localWifiScreen);
    if (!localWifiBusy.load()) localWifiStart(false);
}
static void refreshLocalWifi() {
    localWifiProcessKey();
    int result = localWifiResult.exchange(0);
    if (!result) return;
    localWifiBusy = false;
    if (result == 1) {
        lv_obj_clean(localWifiList);
        for (int i = 0; i < localWifiCount; ++i) {
            lv_obj_t* b = mustButton(localWifiList, "network");
            lv_obj_set_pos(b, 0, i * 44); lv_obj_set_size(b, 228, 40);
            lv_obj_t* l = mustLabel(b, "ssid"); lv_obj_set_width(l, 220);
            lv_label_set_long_mode(l, LV_LABEL_LONG_MODE_CLIP);
            lv_label_set_text(l, localWifiNames[i].c_str()); lv_obj_center(l);
            lv_obj_add_event_cb(b, localWifiSelect, LV_EVENT_SHORT_CLICKED, (void*)(intptr_t)i);
        }
        lv_label_set_text(localWifiStatus, localWifiCount ? "Выберите сеть" : "Сети не найдены");
    } else lv_label_set_text(localWifiStatus, result == 2 ? "Ошибка поиска. Повторите" :
        result == 3 ? "Подключено. Сеть сохранена" : result == 5 ? "Подключено, ошибка сохранения" : "Не подключено. Возврат к прежней сети");
}
