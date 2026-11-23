// Five tested networks, newest successful use first. Credentials never logged.
static struct WifiSavedData {
    uint32_t magic;
    struct Entry { char ssid[33]; char key[65]; uint8_t failures; } entries[5];
} wifiSaved = {};
static SemaphoreHandle_t wifiSavedMutex;
static std::atomic<unsigned> wifiAuthFailures{0};
static void wifiSavedWrite() {
    Preferences p;
    if (p.begin("wifi_saved", false)) {
        p.putBytes("networks", &wifiSaved, sizeof(wifiSaved));
        p.end();
    }
}
static bool wifiSavedKey(const String& name, String& key) {
    bool found = false;
    xSemaphoreTake(wifiSavedMutex, portMAX_DELAY);
    for (auto &e : wifiSaved.entries) if (name == e.ssid && e.failures < 2) {
        key = e.key; found = true; break;
    }
    xSemaphoreGive(wifiSavedMutex);
    return found;
}
static void wifiSavedRemember(const String& name, const String& key) {
    if (!name.length() || name.length() > 32 || key.length() > 64) return;
    xSemaphoreTake(wifiSavedMutex, portMAX_DELAY);
    int index = 4;
    for (int i = 0; i < 5; ++i) if (name == wifiSaved.entries[i].ssid) { index = i; break; }
    if (index != 0 || key != wifiSaved.entries[0].key || wifiSaved.entries[0].failures) {
        for (int i = index; i > 0; --i) wifiSaved.entries[i] = wifiSaved.entries[i-1];
        auto &e = wifiSaved.entries[0];
        memset(&e, 0, sizeof(e));
        strlcpy(e.ssid, name.c_str(), sizeof(e.ssid));
        strlcpy(e.key, key.c_str(), sizeof(e.key));
        wifiSavedWrite();
    }
    xSemaphoreGive(wifiSavedMutex);
}
static void wifiSavedFailed(const String& name) {
    if (!wifiAuthFailures.load()) return; // No signal/timeouts do NOT invalidate a password.
    xSemaphoreTake(wifiSavedMutex, portMAX_DELAY);
    for (auto &e : wifiSaved.entries) if (name == e.ssid && e.failures < 2) {
        ++e.failures; wifiSavedWrite(); break;
    }
    xSemaphoreGive(wifiSavedMutex);
}
static void wifiSavedInit() {
    wifiSavedMutex = xSemaphoreCreateMutex();
    if (!wifiSavedMutex) fatal("wifi saved mutex");
    Preferences p;
    if (p.begin("wifi_saved", true)) {
        if (p.getBytesLength("networks") == sizeof(wifiSaved)) p.getBytes("networks", &wifiSaved, sizeof(wifiSaved));
        p.end();
    }
    if (wifiSaved.magic != 0x57494635) {
        memset(&wifiSaved, 0, sizeof(wifiSaved)); wifiSaved.magic = 0x57494635;
        wifi_config_t config = {};
        if (esp_wifi_get_config(WIFI_IF_STA, &config) == ESP_OK) {
            char name[33] = {}, key[65] = {};
            memcpy(name, config.sta.ssid, 32); memcpy(key, config.sta.password, 64);
            wifiSavedRemember(String(name), String(key));
        }
    }
    for (auto &e : wifiSaved.entries) { e.ssid[32] = 0; e.key[64] = 0; }
    WiFi.onEvent([](WiFiEvent_t, WiFiEventInfo_t info) {
        if (info.wifi_sta_disconnected.reason == WIFI_REASON_AUTH_FAIL ||
            info.wifi_sta_disconnected.reason == WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT)
            wifiAuthFailures.fetch_add(1);
    }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
}
// Network work stays outside GUI/audio. No scans while a working connection exists.
static void wifiSavedTask(void*) {
    uint32_t lostAt = millis(), lastScan = millis() - 30000U;
    String remembered;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(2000));
        if (WiFi.status() == WL_CONNECTED) {
            lostAt = millis();
            if (!wifiPortalBusy.load() && !localWifiBusy.exchange(true)) {
                String name = WiFi.SSID();
                if (remembered != name) { wifiSavedRemember(name, WiFi.psk()); remembered = name; }
                localWifiBusy.store(false);
            }
            continue;
        }
        remembered = "";
        if (millis() - lostAt < 10000U || millis() - lastScan < 30000U || wifiPortalBusy.load()) continue;
        if (localWifiBusy.exchange(true)) continue;
        lastScan = millis();
        WiFi.setAutoReconnect(false);
        WiFi.persistent(false);
        WiFi.disconnect(false, false);
        vTaskDelay(pdMS_TO_TICKS(200));
        int n = WiFi.scanNetworks(false, false, false, 300);
        WifiSavedData snapshot;
        xSemaphoreTake(wifiSavedMutex, portMAX_DELAY); snapshot = wifiSaved; xSemaphoreGive(wifiSavedMutex);
        bool available[5] = {};
        for (int j = 0; j < 5; ++j) for (int i = 0; i < n; ++i)
            if (snapshot.entries[j].ssid[0] && snapshot.entries[j].failures < 2 && WiFi.SSID(i) == snapshot.entries[j].ssid) available[j] = true;
        WiFi.scanDelete();
        for (int j = 0; j < 5 && WiFi.status() != WL_CONNECTED; ++j) {
            if (!available[j]) continue;
            auto &e = snapshot.entries[j];
            wifiAuthFailures.store(0);
            WiFi.begin(e.ssid, e.key);
            uint32_t started = millis();
            while (WiFi.status() != WL_CONNECTED && millis() - started < 12000U) vTaskDelay(pdMS_TO_TICKS(100));
            if (WiFi.status() == WL_CONNECTED) wifiSavedRemember(String(e.ssid), String(e.key));
            else { wifiSavedFailed(String(e.ssid)); WiFi.disconnect(false, false); vTaskDelay(pdMS_TO_TICKS(200)); }
        }
        WiFi.setAutoReconnect(true);
        WiFi.persistent(true);
        localWifiBusy.store(false);
    }
}
