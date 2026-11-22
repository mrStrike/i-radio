#pragma once
// One atomic snapshot: minute (11 bits), station (8 bits), enabled (1 bit).
static std::atomic<uint32_t> alarmConfig{360}, offConfig{1320};
static lv_obj_t *alarmHours, *alarmMinutes, *alarmStations, *alarmCheck, *alarmClockStatus;
static lv_obj_t *offCheck, *offTimeButton, *offOverlay, *offHours, *offMinutes;
static uint32_t packAlarm() {
    return (lv_roller_get_selected(alarmHours)*60+lv_roller_get_selected(alarmMinutes)) |
        (uint32_t(lv_dropdown_get_selected(alarmStations))<<11) |
        (lv_obj_has_state(alarmCheck,LV_STATE_CHECKED)?1UL<<19:0);
}
static void alarmChanged(lv_event_t*) {
    alarmConfig.store(packAlarm());
    updateScheduleShortcuts(alarmConfig.load()&(1UL<<19),offConfig.load()&(1UL<<19));
    bool on=lv_obj_has_state(alarmCheck,LV_STATE_CHECKED);
    for(auto o:{alarmHours,alarmMinutes,alarmStations}) {
        if(on) lv_obj_remove_state(o,LV_STATE_DISABLED); else lv_obj_add_state(o,LV_STATE_DISABLED);
    }
}
static lv_obj_t* timeRoller(lv_obj_t* parent,int x,int y,int count,int selected) {
    String options;
    for(int i=0;i<count;i++){char b[8]; snprintf(b,sizeof(b),"%02d",i); if(i) options+='\n'; options+=b;}
    lv_obj_t* r=lv_roller_create(parent);
    lv_roller_set_options(r,options.c_str(),LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(r,3);
    lv_obj_set_pos(r,x,y); lv_obj_set_width(r,88);
    lv_obj_set_style_bg_color(r,lv_color_hex(0x17251f),0);
    lv_obj_set_style_text_color(r,lv_color_hex(0xffffff),0);
    lv_obj_set_style_text_line_space(r,18,0);
    lv_obj_set_style_radius(r,0,0);
    lv_obj_set_style_bg_color(r,lv_color_hex(0x287b50),LV_PART_SELECTED);
    lv_obj_set_style_text_color(r,lv_color_hex(0xffffff),LV_PART_SELECTED);
    lv_roller_set_selected(r,selected,LV_ANIM_OFF);
    return r;
}
static void updateOffText() {
    uint32_t c=offConfig.load(); char b[24]; unsigned m=c&2047;
    snprintf(b,sizeof(b),"%02u:%02u",m/60,m%60);
    lv_label_set_text(lv_obj_get_child(offTimeButton,0),b);
    if(c&(1UL<<19)) lv_obj_remove_state(offTimeButton,LV_STATE_DISABLED);
    else lv_obj_add_state(offTimeButton,LV_STATE_DISABLED);
}
static void offChanged(lv_event_t*) {
    uint32_t c=offConfig.load()&2047;
    if(lv_obj_has_state(offCheck,LV_STATE_CHECKED)) c|=1UL<<19;
    offConfig.store(c); updateOffText();
    updateScheduleShortcuts(alarmConfig.load()&(1UL<<19),offConfig.load()&(1UL<<19));
}
static void buildOffControls() {
    offCheck=lv_checkbox_create(settingsScreen);
    lv_checkbox_set_text(offCheck,"Выключить радио в:"); lv_obj_set_pos(offCheck,16,286);
    lv_obj_set_style_text_color(offCheck,lv_color_hex(0xffffff),LV_PART_MAIN);
    if(offConfig.load()&(1UL<<19)) lv_obj_add_state(offCheck,LV_STATE_CHECKED);
    lv_obj_add_event_cb(offCheck,offChanged,LV_EVENT_VALUE_CHANGED,nullptr);
    offTimeButton=mustButton(settingsScreen,"offTime");
    lv_obj_set_pos(offTimeButton,16,322); lv_obj_set_size(offTimeButton,208,44);
    lv_obj_t* text=mustLabel(offTimeButton,"offTimeText"); lv_obj_center(text);
    updateOffText();
    offOverlay=mustCreate(lv_layer_top(),"offTimeOverlay");
    lv_obj_set_pos(offOverlay,0,0); lv_obj_set_size(offOverlay,240,320);
    lv_obj_set_style_pad_all(offOverlay,0,0);
    lv_obj_set_style_bg_color(offOverlay,lv_color_hex(0x000000),0);
    lv_obj_set_style_text_font(offOverlay,&dejavu_16,0);
    lv_obj_t* title=mustLabel(offOverlay,"offTitle"); lv_label_set_text(title,"Выключение радио"); lv_obj_align(title,LV_ALIGN_TOP_MID,0,20);
    unsigned m=offConfig.load()&2047;
    offHours=timeRoller(offOverlay,20,80,24,m/60); offMinutes=timeRoller(offOverlay,132,80,60,m%60);
    lv_obj_t* done=mustButton(offOverlay,"offDone"); lv_obj_set_pos(done,20,250); lv_obj_set_size(done,200,48);
    lv_obj_t* label=mustLabel(done,"offDoneText"); lv_label_set_text(label,"Готово"); lv_obj_center(label);
    lv_obj_add_event_cb(done,[](lv_event_t*){
        offConfig.store((offConfig.load()&(1UL<<19)) | (lv_roller_get_selected(offHours)*60+lv_roller_get_selected(offMinutes)));
        updateOffText(); scheduleTimePopupOpen=false; lv_obj_add_flag(offOverlay,LV_OBJ_FLAG_HIDDEN);
    },LV_EVENT_CLICKED,nullptr);
    lv_obj_add_event_cb(offTimeButton,[](lv_event_t*){
        unsigned m=offConfig.load()&2047;
        lv_roller_set_selected(offHours,m/60,LV_ANIM_OFF); lv_roller_set_selected(offMinutes,m%60,LV_ANIM_OFF);
        scheduleTimePopupOpen=true; lv_obj_remove_flag(offOverlay,LV_OBJ_FLAG_HIDDEN);
    },LV_EVENT_CLICKED,nullptr);
    lv_obj_add_flag(offOverlay,LV_OBJ_FLAG_HIDDEN);
}
static void buildAlarmScreen() {
    alarmScreen=lv_obj_create(nullptr);
    lv_obj_set_style_pad_all(alarmScreen,0,0); lv_obj_set_style_border_width(alarmScreen,0,0);
    lv_obj_set_style_bg_color(alarmScreen,lv_color_hex(0x000000),0);
    lv_obj_set_style_text_font(alarmScreen,&dejavu_16,0);
    lv_obj_set_style_text_color(alarmScreen,lv_color_hex(0xffffff),0);
    lv_obj_remove_flag(alarmScreen,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t* title=mustLabel(alarmScreen,"alarmTitle"); lv_label_set_text(title,"Будильник"); lv_obj_align(title,LV_ALIGN_TOP_MID,0,12);
    alarmCheck=lv_checkbox_create(alarmScreen); lv_checkbox_set_text(alarmCheck,"Будильник каждый день"); lv_obj_set_pos(alarmCheck,12,48);
    uint32_t c=alarmConfig.load(); unsigned m=c&2047;
    if(c&(1UL<<19)) lv_obj_add_state(alarmCheck,LV_STATE_CHECKED);
    alarmHours=timeRoller(alarmScreen,20,90,24,m/60); alarmMinutes=timeRoller(alarmScreen,132,90,60,m%60);
    lv_obj_t* colon=mustLabel(alarmScreen,"colon"); lv_label_set_text(colon,":"); lv_obj_set_pos(colon,116,132);
    String options; for(int i=0;i<ALL_STATIONS_COUNT;i++){if(i) options+='\n'; options+=ALL_STATIONS[i].name;}
    alarmStations=lv_dropdown_create(alarmScreen); lv_dropdown_set_options(alarmStations,options.c_str());
    lv_obj_set_pos(alarmStations,12,228); lv_obj_set_size(alarmStations,216,44);
    lv_dropdown_set_selected(alarmStations,(c>>11)&255);
    // Use the LVGL symbol font only for the dropdown arrow.
    lv_obj_set_style_text_font(alarmStations,LV_FONT_DEFAULT,LV_PART_INDICATOR);
    lv_obj_t* list=lv_dropdown_get_list(alarmStations); lv_obj_set_style_text_font(list,&dejavu_16,0); lv_obj_set_style_text_line_space(list,16,0);
    for(auto o:{alarmHours,alarmMinutes,alarmStations,alarmCheck}) lv_obj_add_event_cb(o,alarmChanged,LV_EVENT_VALUE_CHANGED,nullptr);
    alarmChanged(nullptr);
    alarmClockStatus=mustLabel(alarmScreen,"alarmClockStatus"); lv_obj_set_pos(alarmClockStatus,12,290);
    lv_label_set_text(alarmClockStatus,"Ожидание времени");
}
static void checkSchedules(time_t now,const tm& local) {
    if(now<=1700000000) return;
    static int64_t lastAlarm=-1,lastOff=-1;
    const int64_t minute=int64_t(now)/60;
    const unsigned today=local.tm_hour*60+local.tm_min;
    uint32_t a=alarmConfig.load(),o=offConfig.load();
    if((a&(1UL<<19)) && (a&2047)==today && lastAlarm!=minute) {
        Command c{CMD_PLAY_ALL,uint8_t((a>>11)&255)};
        if(xQueueSend(audioCmdQueue,&c,0)==pdTRUE) {
            lastAlarm=minute; radioDeadline=0;
            displaySleeping=false; lastTouchAt=millis(); ledcWrite(TFT_BL,currentBrightness.load()*255/100);
            lv_screen_load(mainScreen);
        }
    }
    if((o&(1UL<<19)) && (o&2047)==today && lastOff!=minute) {
        // Scheduled stop never interrupts an MP3 file.
        if(!sdActive.load() && (wantPlayback.load() || isPlaying)) {
            Command c{CMD_STOP,0}; if(xQueueSend(audioCmdQueue,&c,0)==pdTRUE) lastOff=minute;
        } else lastOff=minute;
    }
    char b[64]; snprintf(b,sizeof(b),"%02d:%02d - Москва",local.tm_hour,local.tm_min);
    if(strcmp(lv_label_get_text(alarmClockStatus),b)) lv_label_set_text(alarmClockStatus,b);
}
