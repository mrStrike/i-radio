#pragma once
#include <ArduinoJson.h>
// The live catalogue is immutable between boots: no audio/GUI pointer can change.
static String stationNames[MAX_STATIONS], stationUrls[MAX_STATIONS];
static JsonDocument stationDisk;
static bool stationCatalogLoaded=false, stationCatalogWritable=false;
static const char* stationCatalogMessage="Нет SD: встроенный список, только чтение";
static bool stationValidate(JsonDocument& d) {
    if(d["version"].as<int>()!=1 || !d["stations"].is<JsonArray>())return false;
    JsonArray a=d["stations"].as<JsonArray>();
    if(a.size()<1 || a.size()>MAX_STATIONS)return false;
    for(size_t i=0;i<a.size();i++) {
        if(!a[i]["name"].is<const char*>() || !a[i]["url"].is<const char*>())return false;
        String n=a[i]["name"].as<String>(), u=a[i]["url"].as<String>();
        if(!n.length() || n.length()>120 || u.length()>768 || (!u.startsWith("http://")&&!u.startsWith("https://")))return false;
        for(size_t k=0;k<n.length();k++)if((uint8_t)n[k]<32)return false;
        for(size_t k=0;k<u.length();k++)if((uint8_t)u[k]<=32)return false;
        if(a[i]["aliases"].is<JsonArray>()) {
            if(a[i]["aliases"].size()>16)return false;
            for(JsonVariant v:a[i]["aliases"].as<JsonArray>())if(!v.is<const char*>() || strlen(v.as<const char*>())>768)return false;
        }
        for(size_t j=0;j<i;j++)if(u==a[j]["url"].as<String>())return false;
    }
    return true;
}
static bool stationRead(const char* path,JsonDocument& d) {
    File f=SD_MMC.open(path,FILE_READ);
    if(!f || f.size()>196608)return false;
    auto e=deserializeJson(d,f);f.close();return !e && stationValidate(d);
}
static bool stationWrite(JsonDocument& d) {
    SD_MMC.remove("/.stations.tmp");
    File f=SD_MMC.open("/.stations.tmp",FILE_WRITE);if(!f)return false;
    size_t expected=measureJson(d), written=serializeJson(d,f);f.flush();f.close();
    JsonDocument check;
    if(written!=expected || !stationRead("/.stations.tmp",check))return false;
    if(SD_MMC.exists("/.stations")) {
        SD_MMC.remove("/.stations.bak");
        if(!SD_MMC.rename("/.stations","/.stations.bak"))return false;
    }
    if(!SD_MMC.rename("/.stations.tmp","/.stations")) {
        SD_MMC.rename("/.stations.bak","/.stations");return false;
    }
    return true;
}
static bool stationUrlMatches(int i,const String& url) {
    if(url==ALL_STATIONS[i].url)return true;
    if(stationCatalogLoaded)for(JsonVariant v:stationDisk["stations"][i]["aliases"].as<JsonArray>())if(url==v.as<String>())return true;
    return false;
}
static void stationCatalogInit() {
    for(int i=0;i<BUILTIN_STATIONS_COUNT;i++)ALL_STATIONS[i]=BUILTIN_STATIONS[i];
    SD_MMC.setPins(38,40,39,41,48,47); // existing, proven SD wiring
    if(!SD_MMC.begin("/sdcard",false,false) || SD_MMC.cardType()==CARD_NONE)return;
    bool exists=SD_MMC.exists("/.stations");
    if(!exists && !SD_MMC.exists("/.stations.bak")) {
        stationDisk["version"]=1;
        JsonArray a=stationDisk["stations"].to<JsonArray>();
        for(int i=0;i<BUILTIN_STATIONS_COUNT;i++){JsonObject o=a.add<JsonObject>();o["name"]=BUILTIN_STATIONS[i].name;o["url"]=BUILTIN_STATIONS[i].url;}
        if(!stationWrite(stationDisk)){stationCatalogMessage="SD: не удалось создать /.stations; встроенный список";return;}
    } else if(!stationRead("/.stations",stationDisk)) {
        if(!exists && stationRead("/.stations.bak",stationDisk)) {
            // Interrupted rename: use backup, leave files untouched.
            stationCatalogMessage="Каталог восстановлен из /.stations.bak";
        } else {stationDisk.clear();stationCatalogMessage="Файл /.stations повреждён: встроенный список, редактирование отключено";return;}
    }
    ALL_STATIONS_COUNT=stationDisk["stations"].size();
    for(int i=0;i<ALL_STATIONS_COUNT;i++) {
        stationNames[i]=stationDisk["stations"][i]["name"].as<String>();stationUrls[i]=stationDisk["stations"][i]["url"].as<String>();
        ALL_STATIONS[i]={stationNames[i].c_str(),stationUrls[i].c_str()};
    }
    stationCatalogLoaded=stationCatalogWritable=true;
    if(exists)stationCatalogMessage="Список с SD: /.stations";
    else if(!SD_MMC.exists("/.stations.bak"))stationCatalogMessage="Список скопирован на SD: /.stations";
}
