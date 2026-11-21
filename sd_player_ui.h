#pragma once
// LCDWIKI ES3C28P: CLK38 CMD40 DATA0=39 DATA1=41 DATA2=48 DATA3=47.
// One-bit mode uses CLK/CMD/DATA0; never formats the card.
static std::vector<String> sdMp3Files;
static std::atomic<int> sdScanState{0}; // 0 idle, 1 scanning, 2 ready, 3 unreadable
static bool sdScanLimited = false;
static std::vector<String> sdFolders;
static String sdFolderPath="/", sdBuildPath="/";
static bool sdRecursive=true, sdBuildRecursive=true;
static std::atomic<int> sdPlaylistOperation{0}; // 1 replace, 2 append, 3 clear
static std::atomic<bool> sdEditStopped{false};
static lv_obj_t *sdFolderPopup=nullptr, *sdFolderRows=nullptr, *sdFolderTitle=nullptr, *sdRecursiveButton=nullptr;
static bool sdCollectFolders=false;
static std::atomic<bool> sdWebScanRequested{false};
static lv_obj_t *sdMessage, *sdFileList, *sdPageLabel, *sdRetryButton;
static size_t sdPage = 0;
static int sdSelected=0;
static std::atomic<int> sdAction{0}, sdRequested{0}, sdCurrent{-1};
static std::atomic<bool> sdActive{false}, sdPaused{false}, sdEnded{false};
static std::atomic<unsigned> sdElapsed{0}, sdDuration{0};
static lv_obj_t *sdTime, *sdProgress;
static std::atomic<int> sdSeekSeconds{-1}, sdSeekTrack{-1};
static void sdSeek(lv_event_t* e) {
    unsigned duration=sdDuration.load();
    if(!sdActive.load() || !duration) return;
    lv_indev_t* indev=lv_event_get_indev(e);
    if(!indev) return;
    lv_point_t point; lv_indev_get_point(indev,&point);
    lv_area_t area; lv_obj_get_coords(sdProgress,&area);
    int width=area.x2-area.x1;
    if(width<=0) return;
    int x=std::max(0,std::min(width,int(point.x-area.x1)));
    unsigned target=unsigned(uint64_t(x)*duration/width);
    // The library's absolute seek API accepts 16-bit seconds.
    target=std::min(65535u,std::min(target,duration-1));
    sdSeekTrack.store(sdCurrent.load()); sdSeekSeconds.store(int(target));
}
static void sdChoose(lv_event_t* e) {
    if(sdScanState.load()!=2) return;
    sdSelected=int(uintptr_t(lv_event_get_user_data(e)));
    sdRequested.store(sdSelected); sdAction.store(1);
}
static void sdControl(lv_event_t* e) {
    if(sdScanState.load()!=2) return;
    int action=int(uintptr_t(lv_event_get_user_data(e)));
    int index=sdCurrent.load();
    if(action==4 || action==5) {
        index=index<0 ? sdSelected : index+(action==4 ? -1 : 1);
        if(index<0 || index>=int(sdMp3Files.size())) return;
        sdSelected=index; sdRequested.store(index); sdAction.store(1);
    } else {sdRequested.store(sdSelected); sdAction.store(action);}
}
// Crisp icons built from opaque, square pixels; no font glyphs or antialiasing.
static void sdPixel(lv_obj_t* b,int x,int y,int w,int h) {
    lv_obj_t* p=lv_obj_create(b); lv_obj_remove_style_all(p);
    lv_obj_set_pos(p,x,y); lv_obj_set_size(p,w,h);
    lv_obj_set_style_bg_color(p,lv_color_hex(0x80ff80),0);
    lv_obj_set_style_bg_opa(p,LV_OPA_COVER,0);
    lv_obj_remove_flag(p,static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE));
}
static void sdIcon(lv_obj_t* b,int type) {
    lv_obj_set_style_pad_all(b,0,0);
    if(type==2) {sdPixel(b,12,8,5,16); sdPixel(b,23,8,5,16);}
    else if(type==3) sdPixel(b,12,8,16,16);
    else {
        bool left=type==4;
        for(int i=0;i<8;i++) sdPixel(b,left?26-i*2:12+i*2,8+i,2,16-2*i);
        if(type==4) sdPixel(b,8,8,3,16);
        if(type==5) sdPixel(b,31,8,3,16);
    }
}
static uint32_t sdFold(uint32_t c) {
    if(c >= 'A' && c <= 'Z') return c + 32;
    if(c >= 0x410 && c <= 0x42f) return c + 32;
    return c == 0x401 ? 0x451 : c;
}
static uint32_t sdCode(const char*& p) {
    unsigned char c = *p++;
    if(c < 128) return c;
    if((c & 0xe0) == 0xc0 && *p) return ((c & 31) << 6) | (*p++ & 63);
    if((c & 0xf0) == 0xe0 && p[0] && p[1]) {
        uint32_t v = ((c & 15) << 12) | ((p[0] & 63) << 6) | (p[1] & 63);
        p += 2; return v;
    }
    return c;
}
static bool sdNaturalLess(const String& a, const String& b) {
    const char *x=a.c_str(), *y=b.c_str();
    while(*x && *y) {
        if(*x>='0' && *x<='9' && *y>='0' && *y<='9') {
            const char *xe=x, *ye=y;
            while(*xe>='0' && *xe<='9') ++xe;
            while(*ye>='0' && *ye<='9') ++ye;
            const char *xs=x, *ys=y;
            while(xs<xe && *xs=='0') ++xs;
            while(ys<ye && *ys=='0') ++ys;
            if(xe-xs != ye-ys) return xe-xs < ye-ys;
            int cmp=strncmp(xs,ys,xe-xs);
            if(cmp) return cmp<0;
            x=xe; y=ye; continue;
        }
        uint32_t cx=sdFold(sdCode(x)), cy=sdFold(sdCode(y));
        if(cx!=cy) return cx<cy;
    }
    return *x==0 && *y!=0;
}
static void sdWalk(const String& path, unsigned depth) {
    if(depth>16 || sdMp3Files.size()>=2000) { sdScanLimited=true; return; }
    File dir=SD_MMC.open(path);
    if(!dir || !dir.isDirectory()) return;
    std::vector<String> files, folders;
    for(File f=dir.openNextFile(); f; f=dir.openNextFile()) {
        String name=f.name(); int slash=name.lastIndexOf('/');
        if(slash>=0) name=name.substring(slash+1);
        if(f.isDirectory()) folders.push_back(name);
        else { String lower=name; lower.toLowerCase(); if(lower.endsWith(".mp3")) files.push_back(name); }
        f.close(); vTaskDelay(1);
    }
    dir.close();
    std::sort(files.begin(), files.end(), sdNaturalLess);
    std::sort(folders.begin(), folders.end(), sdNaturalLess);
    String prefix=path=="/" ? "/" : path+"/";
    for(const auto& name:files) {
        if(sdMp3Files.size()>=2000) {sdScanLimited=true; break;}
        sdMp3Files.push_back(prefix+name);
    }
    for(const auto& name:folders) {
        if(sdCollectFolders) sdFolders.push_back(prefix+name);
        if(sdCollectFolders || sdBuildRecursive) sdWalk(prefix+name,depth+1);
    }
}
// Playlist snapshot lives on SD, not in the small settings partition.
static bool sdSavePlaylist() {
    File f=SD_MMC.open("/.radio-playlist.tmp",FILE_WRITE);
    if(!f) return false;
    bool ok=f.print("RADIO-PLAYLIST-1\n")>0;
    for(const auto& path:sdMp3Files) if(f.println(path)!=path.length()+2) ok=false;
    f.flush(); f.close();
    if(!ok) return false;
    SD_MMC.remove("/.radio-playlist.bak");
    if(SD_MMC.exists("/.radio-playlist.txt") && !SD_MMC.rename("/.radio-playlist.txt","/.radio-playlist.bak")) return false;
    return SD_MMC.rename("/.radio-playlist.tmp","/.radio-playlist.txt");
}
static void sdLoadPlaylist() {
    File f=SD_MMC.open("/.radio-playlist.txt",FILE_READ);
    if(!f) f=SD_MMC.open("/.radio-playlist.bak",FILE_READ);
    if(!f) return;
    String header=f.readStringUntil('\n'); header.trim();
    if(header!="RADIO-PLAYLIST-1") {f.close(); return;}
    sdMp3Files.clear();
    while(f.available() && sdMp3Files.size()<2000) {
        String path=f.readStringUntil('\n'); if(path.endsWith("\r")) path.remove(path.length()-1);
        if(path.startsWith("/") && SD_MMC.exists(path)) sdMp3Files.push_back(path);
        vTaskDelay(1);
    }
    f.close();
}
static void sdScanTask(void*) {
    SD_MMC.end(); sdMp3Files.clear(); sdFolders.clear(); sdScanLimited=false; sdCollectFolders=true;
    SD_MMC.setPins(38,40,39);
    if(!SD_MMC.begin("/sdcard",true,false) || SD_MMC.cardType()==CARD_NONE) sdScanState.store(3);
    else {sdWalk("/",0); sdCollectFolders=false; sdLoadPlaylist(); if(!SD_MMC.exists("/.radio-playlist.txt")) sdSavePlaylist(); sdScanState.store(2);}
    vTaskDelete(nullptr);
}
static void sdRetry(lv_event_t*) {
    if(sdScanState.load()==1 || sdScanState.load()==2 || sdActive.load()) return;
    sdScanState.store(1);
    lv_obj_clean(sdFileList);
    lv_label_set_text(sdMessage,"Проверка SD карты...");
    lv_label_set_text(sdPageLabel,"");
    if(xTaskCreate(sdScanTask,"sdScan",6144,nullptr,1,nullptr)!=pdPASS) {
        sdScanState.store(0); lv_label_set_text(sdMessage,"Не хватает памяти. Повторите.");
    }
}
static void sdShowPage(bool followTrack=false, bool preserveScroll=false) {
    int32_t scrollY=preserveScroll ? lv_obj_get_scroll_y(sdFileList) : 0;
    lv_obj_t* playingRow=nullptr;
    lv_obj_clean(sdFileList);
    size_t start=sdPage*30, end=std::min(start+30,sdMp3Files.size());
    for(size_t i=start;i<end;++i) {
        lv_obj_t* row=lv_label_create(sdFileList);
        if(int(i)==sdCurrent.load()) playingRow=row;
        lv_obj_add_flag(row,LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(row,sdChoose,LV_EVENT_CLICKED,(void*)uintptr_t(i));
        lv_obj_set_style_text_color(row,lv_color_hex(i==size_t(sdCurrent.load()) ? 0xffffff : 0x80ff80),0);
        lv_obj_set_width(row,204);
        lv_label_set_long_mode(row,int(i)==sdCurrent.load() ? LV_LABEL_LONG_SCROLL_CIRCULAR : LV_LABEL_LONG_CLIP);
        const String& path=sdMp3Files[i];
        lv_label_set_text(row,path.c_str()+path.lastIndexOf('/')+1);
        lv_obj_set_style_min_height(row,22,0);
        lv_obj_set_style_bg_color(row,lv_color_hex(0x183820),0);
        lv_obj_set_style_bg_opa(row,int(i)==sdCurrent.load() ? LV_OPA_COVER : LV_OPA_TRANSP,0);
    }
    char text[64]; snprintf(text,sizeof(text),"%u / %u",unsigned(sdPage+1),unsigned(std::max(size_t(1),(sdMp3Files.size()+29)/30)));
    lv_label_set_text(sdPageLabel,text);
    lv_obj_update_layout(sdFileList);
    lv_obj_scroll_to_y(sdFileList,scrollY,LV_ANIM_OFF);
    if(followTrack && playingRow) lv_obj_scroll_to_view(playingRow,LV_ANIM_OFF);
}
static void sdPageEvent(lv_event_t* e) {
    if(sdScanState.load()!=2) return;
    if(lv_event_get_user_data(e)) {if((sdPage+1)*30<sdMp3Files.size()) ++sdPage;}
    else if(sdPage) --sdPage;
    sdShowPage();
}
static void debugBackEvent(lv_event_t*) {lv_screen_load(mainScreen);}
static lv_obj_t* sdButton(const char* text,int x,int y,int w,lv_event_cb_t cb,void* data=nullptr) {
    lv_obj_t* b=mustButton(debugScreen,"sdButton");
    lv_obj_set_pos(b,x,y); lv_obj_set_size(b,w,32);
    // Handle taps across the whole button, without starting a scroll.
    lv_obj_add_flag(b,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(b,static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER));
    lv_obj_set_style_radius(b,0,0);
    lv_obj_set_style_bg_color(b,lv_color_hex(0x243028),0);
    lv_obj_set_style_border_width(b,1,0);
    lv_obj_set_style_border_color(b,lv_color_hex(0x3c5040),0);
    lv_obj_set_style_shadow_width(b,0,0);
    lv_obj_add_event_cb(b,cb,LV_EVENT_CLICKED,data);
    lv_obj_t* l=mustLabel(b,"sdCaption");
    lv_obj_remove_flag(l,static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE));
    lv_label_set_text(l,text); lv_obj_center(l); return b;
}
// Rebuild only after the Audio-owning task acknowledges stop. No SD writes.
static void sdPlaylistTask(void*) {
    while(!sdEditStopped.load()) vTaskDelay(1);
    int operation=sdPlaylistOperation.load();
    std::vector<String> previous;
    if(operation==2) previous.swap(sdMp3Files);
    else sdMp3Files.clear();
    sdScanLimited=false;
    if(operation!=3) sdWalk(sdBuildPath,0);
    if(operation==2) {
        for(const auto& path:sdMp3Files) {
            if(std::find(previous.begin(),previous.end(),path)!=previous.end()) continue;
            if(previous.size()>=2000) {sdScanLimited=true; break;}
            previous.push_back(path);
        }
        sdMp3Files.swap(previous);
    }
    sdSavePlaylist();
    sdPlaylistOperation.store(0);
    sdScanState.store(2);
    vTaskDelete(nullptr);
}
static void sdCloseFolders(lv_event_t*) {
    sdFolderPopupOpen=false;
    lv_obj_add_flag(sdFolderPopup,LV_OBJ_FLAG_HIDDEN);
}
static void sdRefreshFolders();
static void sdFolderEnter(lv_event_t* e) {
    size_t index=uintptr_t(lv_event_get_user_data(e));
    if(index<sdFolders.size()) {sdFolderPath=sdFolders[index]; sdRefreshFolders();}
}
static void sdFolderUp(lv_event_t*) {
    int slash=sdFolderPath.lastIndexOf('/');
    sdFolderPath=slash<=0 ? "/" : sdFolderPath.substring(0,slash);
    sdRefreshFolders();
}
static void sdToggleRecursive(lv_event_t*) {
    sdRecursive=!sdRecursive;
    sdRefreshFolders();
}
static lv_obj_t* sdFolderButton(lv_obj_t* parent,const char* text,int x,int y,int w,lv_event_cb_t cb,void* data=nullptr) {
    lv_obj_t* b=lv_button_create(parent);
    lv_obj_set_pos(b,x,y); lv_obj_set_size(b,w,32);
    lv_obj_add_flag(b,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(b,static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_SCROLL_CHAIN_HOR|LV_OBJ_FLAG_SCROLL_CHAIN_VER));
    lv_obj_set_style_border_width(b,0,0);
    lv_obj_set_style_radius(b,0,0); lv_obj_set_style_shadow_width(b,0,0);
    lv_obj_set_style_bg_color(b,lv_color_hex(0x243028),0);
    lv_obj_add_event_cb(b,cb,LV_EVENT_CLICKED,data);
    lv_obj_t* l=lv_label_create(b); lv_label_set_text(l,text); lv_obj_center(l);
    lv_obj_remove_flag(l,static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE));
    return b;
}
static void sdApplyFolder(lv_event_t* e) {
    if(sdScanState.load()!=2) return;
    int op=int(uintptr_t(lv_event_get_user_data(e)));
    // A second tap confirms clearing; it never deletes files from the card.
    static uint32_t clearAt=0;
    if(op==3 && (!clearAt || millis()-clearAt>3000)) {
        clearAt=millis(); lv_label_set_text(sdFolderTitle,"Очистить? Нажмите ещё раз"); return;
    }
    clearAt=0;
    sdBuildPath=op==4 ? "/" : sdFolderPath;
    sdBuildRecursive=op==4 || sdRecursive;
    sdPlaylistOperation.store(op==4?1:op);
    sdEditStopped.store(false); sdScanState.store(1);
    if(xTaskCreate(sdPlaylistTask,"sdPlaylist",6144,nullptr,1,nullptr)!=pdPASS) {
        sdPlaylistOperation.store(0); sdScanState.store(2);
        lv_label_set_text(sdFolderTitle,"Не хватает памяти"); return;
    }
    sdAction.store(6);
    sdCloseFolders(nullptr);
    lv_obj_clean(sdFileList); lv_label_set_text(sdMessage,"Обновление плейлиста...");
}
static void sdRefreshFolders() {
    lv_label_set_text(sdFolderTitle,sdFolderPath.c_str());
    lv_label_set_text(lv_obj_get_child(sdRecursiveButton,0),sdRecursive?"Подпапки: да":"Подпапки: нет");
    lv_obj_clean(sdFolderRows);
    String prefix=sdFolderPath=="/" ? "/" : sdFolderPath+"/";
    for(size_t i=0;i<sdFolders.size();++i) {
        const String& path=sdFolders[i];
        if(!path.startsWith(prefix)) continue;
        String name=path.substring(prefix.length());
        if(name.indexOf('/')>=0) continue;
        lv_obj_t* b=sdFolderButton(sdFolderRows,name.c_str(),0,0,196,sdFolderEnter,(void*)uintptr_t(i));
        lv_obj_set_style_min_height(b,32,0);
        lv_obj_t* l=lv_obj_get_child(b,0); lv_obj_set_width(l,180);
        lv_label_set_long_mode(l,LV_LABEL_LONG_CLIP);
    }
}
static void sdOpenFolders(lv_event_t*) {
    if(sdScanState.load()!=2) return;
    if(!sdFolderPopup) {
        sdFolderPopup=lv_obj_create(debugScreen);
        lv_obj_set_pos(sdFolderPopup,0,38); lv_obj_set_size(sdFolderPopup,240,282);
        lv_obj_set_style_pad_all(sdFolderPopup,0,0);
        lv_obj_set_style_radius(sdFolderPopup,0,0);
        lv_obj_set_style_bg_color(sdFolderPopup,lv_color_hex(0x101410),0);
        lv_obj_remove_flag(sdFolderPopup,LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_t* up=sdFolderButton(sdFolderPopup,"",0,0,44,sdFolderUp);
        lv_obj_set_size(up,44,40);
        lv_obj_set_style_pad_all(up,0,0);
        // Bent return arrow pointing up: opaque square pixels only.
        sdPixel(up,10,10,4,12);
        sdPixel(up,10,20,16,4);
        sdPixel(up,24,15,4,7);
        sdPixel(up,8,10,8,3);
        sdPixel(up,10,7,4,3);
        sdPixel(up,11,5,2,2);
        sdFolderButton(sdFolderPopup,"X",198,4,36,sdCloseFolders);
        sdFolderTitle=lv_label_create(sdFolderPopup); lv_obj_set_pos(sdFolderTitle,46,10);
        lv_obj_set_width(sdFolderTitle,148); lv_label_set_long_mode(sdFolderTitle,LV_LABEL_LONG_CLIP);
        sdFolderRows=lv_obj_create(sdFolderPopup); lv_obj_set_pos(sdFolderRows,4,40);
        lv_obj_set_size(sdFolderRows,230,116); lv_obj_set_flex_flow(sdFolderRows,LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_all(sdFolderRows,4,0); lv_obj_set_scroll_dir(sdFolderRows,LV_DIR_VER);
        lv_obj_set_style_bg_color(sdFolderRows,lv_color_hex(0x101410),0);
        sdRecursiveButton=sdFolderButton(sdFolderPopup,"",4,160,230,sdToggleRecursive);
        sdFolderButton(sdFolderPopup,"Выбрать",4,198,112,sdApplyFolder,(void*)1);
        sdFolderButton(sdFolderPopup,"Добавить",122,198,112,sdApplyFolder,(void*)2);
        sdFolderButton(sdFolderPopup,"Очистить",4,240,112,sdApplyFolder,(void*)3);
        sdFolderButton(sdFolderPopup,"Вся музыка",122,240,112,sdApplyFolder,(void*)4);
    }
    sdFolderPopupOpen=true; sdRefreshFolders();
    lv_obj_remove_flag(sdFolderPopup,LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(sdFolderPopup);
}
static void buildDebugScreen() {
    debugScreen=mustCreate(nullptr,"sdPlayerScreen");
    lv_obj_remove_style_all(debugScreen);
    lv_obj_set_style_bg_opa(debugScreen,LV_OPA_COVER,0);
    lv_obj_set_style_bg_color(debugScreen,lv_color_hex(0x101410),0);
    lv_obj_set_style_text_color(debugScreen,lv_color_hex(0x80ff80),0);
    lv_obj_set_style_text_font(debugScreen,&dejavu_16,0);
    lv_obj_remove_flag(debugScreen,LV_OBJ_FLAG_SCROLLABLE);
    navigationArrow(sdButton("",4,4,36,debugBackEvent),false,0x80ff80);
    sdTime=mustLabel(debugScreen,"sdTitle");
    lv_label_set_text(sdTime,"MP3 - 00:00:00/00:00:00");
    lv_obj_set_pos(sdTime,44,10); lv_obj_set_width(sdTime,158);
    lv_obj_set_style_text_letter_space(sdTime,-1,0);
    lv_label_set_long_mode(sdTime,LV_LABEL_LONG_CLIP);
    sdRetryButton=sdButton("Повторить",94,4,106,sdRetry);
    lv_obj_add_flag(sdRetryButton,LV_OBJ_FLAG_HIDDEN);
    const int actions[]={4,1,2,3,5};
    for(int i=0;i<5;i++) {
        lv_obj_t* b=sdButton("",4+i*47,42,43,sdControl,(void*)uintptr_t(actions[i]));
        sdIcon(b,actions[i]);
    }
    sdProgress=lv_bar_create(debugScreen); lv_obj_set_pos(sdProgress,6,107);
    lv_obj_set_size(sdProgress,228,14); lv_bar_set_range(sdProgress,0,1000);
    lv_obj_set_style_radius(sdProgress,0,0);
    lv_obj_set_style_bg_color(sdProgress,lv_color_hex(0x29342c),0);
    lv_obj_set_style_bg_opa(sdProgress,LV_OPA_COVER,0);
    lv_obj_set_style_radius(sdProgress,0,LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(sdProgress,lv_color_hex(0x80ff80),LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(sdProgress,LV_OPA_COVER,LV_PART_INDICATOR);
    lv_obj_add_flag(sdProgress,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_ext_click_area(sdProgress,8);
    lv_obj_add_event_cb(sdProgress,sdSeek,LV_EVENT_CLICKED,nullptr);
    sdMessage=mustLabel(debugScreen,"sdMessage"); lv_obj_set_pos(sdMessage,6,80); lv_obj_set_width(sdMessage,228); lv_label_set_long_mode(sdMessage,LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_text(sdMessage,"Проверка SD карты...");
    sdFileList=mustCreate(debugScreen,"sdFiles");
    lv_obj_set_pos(sdFileList,4,126); lv_obj_set_size(sdFileList,232,151);
    lv_obj_set_style_bg_color(sdFileList,lv_color_hex(0x000000),0);
    lv_obj_set_style_radius(sdFileList,0,0);
    lv_obj_set_style_pad_all(sdFileList,4,0);
    lv_obj_set_style_pad_row(sdFileList,2,0);
    lv_obj_set_style_border_width(sdFileList,1,0);
    lv_obj_set_style_border_color(sdFileList,lv_color_hex(0x344438),0);
    lv_obj_set_flex_flow(sdFileList,LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(sdFileList,LV_DIR_VER);
    navigationArrow(sdButton("",4,284,40,sdPageEvent),false,0x80ff80);
    navigationArrow(sdButton("",196,284,40,sdPageEvent,(void*)1),true,0x80ff80);
    sdPageLabel=mustLabel(debugScreen,"sdPage"); lv_obj_set_pos(sdPageLabel,50,291);
    lv_obj_t* folder=sdButton("",146,284,44,sdOpenFolders);
    sdPixel(folder,9,7,12,4); sdPixel(folder,7,11,30,15);
    lv_obj_t* inset=lv_obj_create(folder); lv_obj_remove_style_all(inset);
    lv_obj_set_pos(inset,9,13); lv_obj_set_size(inset,26,11);
    lv_obj_set_style_bg_color(inset,lv_color_hex(0x243028),0); lv_obj_set_style_bg_opa(inset,LV_OPA_COVER,0);
    lv_obj_remove_flag(inset,static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE));
}
// Display only the immediate parent folder; root files use ".".
static String sdPlayingCaption(const String& path) {
    int slash=path.lastIndexOf('/');
    String folder=".";
    if(slash>0) {
        String parent=path.substring(0,slash);
        folder=parent.substring(parent.lastIndexOf('/')+1);
    }
    return folder + " \\ " + path.substring(slash+1);
}
static void updateSdPlayer() {
    static bool wasVisible=false;
    static int shown=-1;
    if(sdWebScanRequested.exchange(false)) sdRetry(nullptr);
    bool visible=lv_screen_active()==debugScreen;
    if(visible && !wasVisible && sdScanState.load()!=1 && sdScanState.load()!=2) {sdRetry(nullptr); shown=-1;}
    bool entering=visible && !wasVisible;
    wasVisible=visible;
    int state=sdScanState.load();
    if(!visible) return;
    if(state==1) {shown=1; return;}
    static int oldTrack=-2; static unsigned oldElapsed=~0u,oldDuration=~0u;
    int track=sdCurrent.load(); unsigned elapsed=sdElapsed.load(),duration=sdDuration.load();
    if(track!=oldTrack || entering) {
        oldTrack=track;
        if(track>=0 && track<int(sdMp3Files.size())) {
            const String& path=sdMp3Files[track];
            lv_label_set_text(sdMessage,sdPlayingCaption(path).c_str());
            size_t playingPage=size_t(track)/30;
            bool samePage=sdPage==playingPage;
            sdPage=playingPage; sdSelected=track;
            sdShowPage(true,samePage);
        }
    }
    if(elapsed!=oldElapsed || duration!=oldDuration) {
        oldElapsed=elapsed; oldDuration=duration; char t[64];
        snprintf(t,sizeof(t),"MP3 - %02u:%02u:%02u/%02u:%02u:%02u",
                 elapsed/3600,(elapsed/60)%60,elapsed%60,
                 duration/3600,(duration/60)%60,duration%60);
        lv_label_set_text(sdTime,t);
        lv_bar_set_value(sdProgress,duration?std::min(1000u,unsigned(uint64_t(elapsed)*1000/duration)):0,LV_ANIM_OFF);
    }
    if(state==0 || state==3) lv_obj_remove_flag(sdRetryButton,LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(sdRetryButton,LV_OBJ_FLAG_HIDDEN);
    if(state==shown) return;
    shown=state;
    if(state==3) lv_label_set_text(sdMessage,"Подключите SD карту.\nИли проверьте формат FAT32.");
    if(state==2) {
        char text[120]; snprintf(text,sizeof(text),"MP3: %u%s",unsigned(sdMp3Files.size()),sdScanLimited ? " (лимит сканирования)" : "");
        lv_label_set_text(sdMessage,text);
        if(track>=0 && track<int(sdMp3Files.size())) {
            const String& path=sdMp3Files[track];
            lv_label_set_text(sdMessage,sdPlayingCaption(path).c_str());
            sdPage=size_t(track)/30; sdSelected=track; sdShowPage(true);
        } else {sdPage=0; sdShowPage();}
    }
}
