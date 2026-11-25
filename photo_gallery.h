#pragma once
#include <JPEGENC.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
// Worker owns SD reads and JPEG decoding; only GUI owns LVGL objects.
// Yield on a time budget, not after every small read; never change audio.
static TaskHandle_t photoExifTaskHandle=nullptr;
static bool photoAudioBusy() {
    return wantPlayback.load() || (sdActive.load() && !sdPaused.load());
}
static void photoWorkPause() {
    // Separate clocks for the decoder and EXIF writer: each must yield to IDLE.
    static uint32_t workerLast=0, writerLast=0;
    uint32_t& last=(xTaskGetCurrentTaskHandle()==photoExifTaskHandle) ? writerLast : workerLast;
    const bool busy=photoAudioBusy();
    const uint32_t now=millis();
    if(uint32_t(now-last)<(busy ? 5u : 10u)) return;
    vTaskDelay(std::max(TickType_t(1),pdMS_TO_TICKS(busy ? 2u : 1u)));
    last=millis();
}
static bool photoReadChunks(File& f,uint8_t* data,size_t bytes) {
    while(bytes) {
        size_t n=std::min(bytes,size_t(2048));
        if(f.read(data,n)!=n) return false;
        data+=n;bytes-=n;photoWorkPause();
    }
    return true;
}
static bool photoWriteChunks(File& f,const uint8_t* data,size_t bytes) {
    while(bytes) {
        size_t n=std::min(bytes,size_t(2048));
        if(f.write(data,n)!=n) return false;
        data+=n;bytes-=n;photoWorkPause();
    }
    return true;
}
struct PhotoTimedFile : public lgfx::DataWrapperT<fs::FS> {
    uint64_t ioUs=0;
    uint32_t bytes=0;
    PhotoTimedFile() : lgfx::DataWrapperT<fs::FS>(static_cast<fs::FS*>(&SD_MMC)) {}
    bool open(const char* path) override {
        int64_t t=esp_timer_get_time();
        bool ok=lgfx::DataWrapperT<fs::FS>::open(path);
        ioUs+=esp_timer_get_time()-t; return ok;
    }
    int read(uint8_t* buf,uint32_t len) override {
        int64_t t=esp_timer_get_time();
        int n=0;
        while(len) {
            uint32_t chunk=std::min(len,uint32_t(2048));
            int got=lgfx::DataWrapperT<fs::FS>::read(buf+n,chunk);
            if(got>0) {n+=got;len-=got;}
            photoWorkPause();
            if(got<int(chunk)) break;
        }
        ioUs+=esp_timer_get_time()-t; if(n>0) bytes+=n; return n;
    }
    void skip(int32_t offset) override {
        int64_t t=esp_timer_get_time(); lgfx::DataWrapperT<fs::FS>::skip(offset);
        ioUs+=esp_timer_get_time()-t;
    }
    bool seek(uint32_t offset) override {
        int64_t t=esp_timer_get_time(); bool ok=lgfx::DataWrapperT<fs::FS>::seek(offset);
        ioUs+=esp_timer_get_time()-t; return ok;
    }
};
static std::vector<String> photoFiles;
static std::atomic<int> photoState{0}, photoRequest{-1}, photoResult{-1};
static std::atomic<bool> photoLargeRequest{false};
static std::atomic<int> photoPhase{0}; // 1 decode, 2 EXIF write
static uint16_t* photoPixels=nullptr;
static bool photoResultLarge=false, photoResultOk=false;
static lv_obj_t *photoGrid, *photoMessage, *photoView, *photoImage, *photoBack, *photoTimer;
static lv_obj_t* photoWaiting;
static std::vector<lv_obj_t*> photoTiles;
static int photoTileIndex[8]={-1,-1,-1,-1,-1,-1,-1,-1};
static int photoGridTop=-1;
struct PhotoCache {int index=-1; uint16_t* pixels=nullptr; lv_image_dsc_t dsc{};};
static PhotoCache photoCache[8];
static lv_image_dsc_t photoFullDsc{};
static uint16_t* photoFullPixels=nullptr;
static uint16_t* photoNextPixels=nullptr;
static int photoNextIndex=-1;
static int photoCurrent=-1, photoLoading=-1;
static uint32_t photoStarted=0;
static uint32_t photoBackActivity=0;
// Directory/cache discovery has a separate, conservative budget.
// Yield between entries even when each individual FAT operation is quick.
static void photoScanPause() {
    vTaskDelay(std::max(TickType_t(1),pdMS_TO_TICKS(photoAudioBusy() ? 10u : 1u)));
}
static void photoWalk(const String& path,int depth) {
    if(depth>16 || photoFiles.size()>=1000) return;
    photoScanPause();
    File dir=SD_MMC.open(path); if(!dir || !dir.isDirectory()) return;
    for(File f=dir.openNextFile();f;f=dir.openNextFile()) {
        String p=f.path(); bool folder=f.isDirectory(); f.close();
        if(!folder && p.endsWith(".photo-old")) {
            String original=p.substring(0,p.length()-10);
            if(!SD_MMC.exists(original) && SD_MMC.rename(p,original)) p=original;
            else if(SD_MMC.exists(original)) SD_MMC.remove(p);
        }
        if(!folder && p.endsWith(".photo-bak")) {
            String original=p.substring(0,p.length()-10);
            if(!SD_MMC.exists(original) && SD_MMC.rename(p,original)) p=original;
        }
        if(folder) photoWalk(p,depth+1);
        else {String lower=p;lower.toLowerCase(); if(lower.endsWith(".jpg")||lower.endsWith(".jpeg")) photoFiles.push_back(p);}
        photoScanPause(); if(photoFiles.size()>=1000) break;
    }
}
// Read only JPEG markers, never allocate the complete compressed file.
static bool photoDimensions(File& f,int& w,int& h) {
    if(f.read()!=255 || f.read()!=216) return false;
    while(f.available()) {
        if(f.read()!=255) continue;
        int m; do {m=f.read();} while(m==255);
        if(m<0 || m==0xd9 || m==0xda) break;
        if(m==0x01 || (m>=0xd0 && m<=0xd8)) continue;
        int hi=f.read(),lo=f.read(); if(hi<0 || lo<0) break;
        size_t len=(hi<<8)|lo; if(len<2 || f.position()+len-2>f.size()) break;
        if(m==0xc0 && len>=8) {
            f.read(); int hh=f.read(),hl=f.read(),wh=f.read(),wl=f.read();
            if(hh<0 || hl<0 || wh<0 || wl<0) return false;
            h=(hh<<8)|hl;w=(wh<<8)|wl;return w>0 && h>0;
        }
        if(!f.seek(f.position()+len-2)) break;
        vTaskDelay(1);
    }
    return false;
}
// Read APP1 only (at most 65533 bytes).
static bool photoExif(File& f, std::vector<uint8_t>& jpg, int& w, int& h) {
    f.seek(0);
    if(f.read()!=255 || f.read()!=216) return false;
    while(f.available()) {
        if(f.read()!=255) continue;
        int m; do {m=f.read();} while(m==255);
        if(m<0 || m==0xda || m==0xd9) break;
        if(m==1 || (m>=0xd0 && m<=0xd8)) continue;
        int hi=f.read(),lo=f.read(); if(hi<0 || lo<0) break;
        uint32_t len=(hi<<8)|lo;
        if(len<2 || len-2>f.size()-f.position()) break;
        uint32_t next=f.position()+len-2;
        if(m==0xe1 && len>=16) {
            std::vector<uint8_t> a(len-2);
            if(f.read(a.data(),a.size())!=a.size()) return false;
            if(memcmp(a.data(),"Exif\0\0",6)==0) {
                const uint8_t* t=a.data()+6; uint32_t n=a.size()-6;
                bool le=t[0]=='I' && t[1]=='I';
                if(!le && !(t[0]=='M' && t[1]=='M')) return false;
                auto u16=[&](uint32_t o)->uint32_t {if(o>n || n-o<2)return 0;return le?(t[o]|(t[o+1]<<8)):((t[o]<<8)|t[o+1]);};
                auto u32=[&](uint32_t o)->uint32_t {if(o>n || n-o<4)return 0;return le?(uint32_t(t[o])|(uint32_t(t[o+1])<<8)|(uint32_t(t[o+2])<<16)|(uint32_t(t[o+3])<<24)):((uint32_t(t[o])<<24)|(uint32_t(t[o+1])<<16)|(uint32_t(t[o+2])<<8)|t[o+3]);};
                if(u16(2)!=42) return false;
                uint32_t ifd=u32(4);
                if(ifd<8 || ifd>n || n-ifd<2) return false;
                uint32_t count=u16(ifd);
                if(count>(n-ifd-2)/12) return false;
                uint32_t link=ifd+2+count*12;
                if(link>n || n-link<4) return false;
                ifd=u32(link); if(ifd<8 || ifd>n || n-ifd<2) return false;
                count=u16(ifd); if(count>(n-ifd-2)/12) return false;
                uint32_t off=0,bytes=0;
                for(uint32_t i=0;i<count;i++) {
                    uint32_t e=ifd+2+i*12;
                    if(u16(e+2)==4 && u32(e+4)==1) {
                        if(u16(e)==0x0201) off=u32(e+8);
                        if(u16(e)==0x0202) bytes=u32(e+8);
                    }
                }
                if(off<8 || bytes<4 || off>n || bytes>n-off) return false;
                // SOF0 dimensions from the embedded baseline JPEG.
                const uint8_t* b=t+off;
                if(b[0]!=255 || b[1]!=216) return false;
                uint32_t k=2;
                while(k+4<=bytes) {
                    if(b[k++]!=255) continue;
                    uint8_t marker=b[k++]; if(marker==255) {--k;continue;}
                    if(marker==0xda || marker==0xd9) break;
                    if(marker==1 || (marker>=0xd0 && marker<=0xd8)) continue;
                    uint32_t l=(b[k]<<8)|b[k+1];
                    if(l<2 || l>bytes-k) break;
                    if(marker==0xc0 && l>=8) {
                        h=(b[k+3]<<8)|b[k+4]; w=(b[k+5]<<8)|b[k+6];
                        if(!w || !h) return false;
                        jpg.assign(b,b+bytes); return true;
                    }
                    k+=l;
                }
            }
        }
        if(!f.seek(next)) break;
        vTaskDelay(1);
    }
    return false;
}
// Preserve all original JPEG bytes except the extended EXIF APP1 segment.
// Only transient replacement files; no permanent backup is created.
struct PhotoSaveTiming {
    uint64_t encode=0,prepare=0,copy=0,flush=0,verify=0,rename=0,remove=0;
    uint32_t oldApp1Size=0,newApp1Size=0;
    bool exifFound=false,inplacePossible=false;
};
static const char* photoSaveExif(const String& path,uint16_t* pixels,int w,int h,PhotoSaveTiming& timing) {
    int64_t phaseStart=esp_timer_get_time();
    String tmp=path+".photo-tmp", rollback=path+".photo-old";
    if(SD_MMC.exists(rollback)) return "RECOVERY_REQUIRED";
    std::vector<uint8_t> jpg(32768);
    JPEGENC encoder; JPEGENCODE enc{};
    if(encoder.open(jpg.data(),jpg.size())!=JPEGE_SUCCESS) return "ENCODE_FAIL";
    int rc=encoder.encodeBegin(&enc,w,h,JPEGE_PIXEL_RGB565,JPEGE_SUBSAMPLE_420,JPEGE_Q_LOW);
    if(rc==JPEGE_SUCCESS) rc=encoder.addFrame(&enc,(uint8_t*)pixels,w*2);
    int bytes=encoder.close();
    if(rc!=JPEGE_SUCCESS || encoder.getLastError()!=JPEGE_SUCCESS || bytes<4 || bytes>int(jpg.size())) return "ENCODE_FAIL";
    jpg.resize(bytes);
    timing.encode=esp_timer_get_time()-phaseStart;phaseStart=esp_timer_get_time();
    File in=SD_MMC.open(path,FILE_READ);
    if(!in || in.read()!=255 || in.read()!=216) return "OPEN_FAIL";
    uint32_t begin=2,end=2; bool found=false;
    std::vector<uint8_t> a;
    while(in.available()) {
        uint32_t markerPos=in.position();
        if(in.read()!=255) return "HEADER_FAIL";
        int m;do {m=in.read();}while(m==255);
        if(m<0) return "HEADER_FAIL";
        if(m==0xda || m==0xd9) break;
        if(m==1 || (m>=0xd0 && m<=0xd8)) continue;
        int hi=in.read(),lo=in.read();if(hi<0 || lo<0) return "HEADER_FAIL";
        uint32_t len=(hi<<8)|lo;
        if(len<2 || len-2>in.size()-in.position()) return "HEADER_FAIL";
        uint32_t next=in.position()+len-2;
        if(m==0xe1 && len>=8) {
            a.resize(len-2);
            if(in.read(a.data(),a.size())!=a.size()) return "READ_FAIL";
            if(memcmp(a.data(),"Exif\0\0",6)==0) {begin=markerPos;end=next;found=true;break;}
        }
        if(!in.seek(next)) return "READ_FAIL";
    }
    timing.exifFound=found;
    timing.oldApp1Size=found?end-begin:0;
    if(!found) a={'E','x','i','f',0,0,'I','I',42,0,8,0,0,0,0,0,0,0,0,0};
    // Preserve IFD0 when structurally valid, but always replace its thumbnail link.
    bool le=true;
    uint32_t link=16;
    auto fresh=[&]() {a={'E','x','i','f',0,0,'I','I',42,0,8,0,0,0,0,0,0,0,0,0};le=true;link=16;};
    bool valid=a.size()>=20;
    if(valid) {
        le=a[6]=='I' && a[7]=='I';
        valid=le || (a[6]=='M' && a[7]=='M');
    }
    auto u16=[&](uint32_t o)->uint32_t {return le?(a[o]|(a[o+1]<<8)):((a[o]<<8)|a[o+1]);};
    auto u32=[&](uint32_t o)->uint32_t {return le?(uint32_t(a[o])|(uint32_t(a[o+1])<<8)|(uint32_t(a[o+2])<<16)|(uint32_t(a[o+3])<<24)):((uint32_t(a[o])<<24)|(uint32_t(a[o+1])<<16)|(uint32_t(a[o+2])<<8)|a[o+3]);};
    if(valid) valid=u16(8)==42;
    if(valid) {
        uint32_t ifd=u32(10), n=a.size()-6;
        valid=ifd>=8 && ifd<=n && n-ifd>=2;
        if(valid) {
            uint32_t count=u16(6+ifd);
            valid=count<=(n-ifd-2)/12;
            if(valid) {link=6+ifd+2+count*12;valid=link<=a.size() && a.size()-link>=4;}
        }
    }
    if(!valid || a.size()+1+42+jpg.size()>65533) fresh();
    auto put=[&](uint32_t o,uint32_t v,int n) {for(int i=0;i<n;i++) a[o+i]=uint8_t(v>>(8*(le?i:n-1-i)));};
    if(a.size()&1) a.push_back(0);
    uint32_t newIfd=a.size()-6,base=a.size();
    // Compression, JPEG offset, JPEG length; next IFD = 0.
    a.resize(base+42,0);
    put(link,newIfd,4);put(base,3,2);
    auto entry=[&](int i,uint16_t tag,uint16_t type,uint32_t value) {
        uint32_t e=base+2+i*12;put(e,tag,2);put(e+2,type,2);put(e+4,1,4);put(e+8,value,type==3?2:4);
    };
    entry(0,0x0103,3,6);entry(1,0x0201,4,a.size()-6);entry(2,0x0202,4,jpg.size());
    a.insert(a.end(),jpg.begin(),jpg.end());
    timing.newApp1Size=4+a.size(); // Complete segment: marker, length, payload.
    timing.inplacePossible=found && timing.newApp1Size<=timing.oldApp1Size;
    if(a.size()>65533) return "EXIF_FULL";
    timing.prepare=esp_timer_get_time()-phaseStart;phaseStart=esp_timer_get_time();
    // Validate the generated JPEG using the same reader before replacing anything.
    SD_MMC.remove(tmp);
    File out=SD_MMC.open(tmp,FILE_WRITE);if(!out) return "WRITE_FAIL";
    // Large PSRAM blocks reduce FatFS call overhead. Keep bounded yields for audio.
    const uint32_t copyBlock=2048;
    uint8_t* buf=(uint8_t*)heap_caps_malloc(copyBlock,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    if(!buf) {out.close();SD_MMC.remove(tmp);return "NO_MEMORY";}
    uint32_t lastYield=millis();
    auto copy=[&](uint32_t start,uint32_t length)->bool {
        if(!in.seek(start)) return false;
        while(length) {uint32_t n=std::min(length,copyBlock);
            if(in.read(buf,n)!=n || out.write(buf,n)!=n) return false;
            length-=n;
            photoWorkPause();lastYield=millis();
        }return true;
    };
    uint32_t originalSize=in.size();
    uint32_t len=a.size()+2;uint8_t header[]={255,225,uint8_t(len>>8),uint8_t(len)};
    bool ok=copy(0,begin) && out.write(header,4)==4 && out.write(a.data(),a.size())==a.size() && copy(end,originalSize-end);
    free(buf);
    timing.copy=esp_timer_get_time()-phaseStart;phaseStart=esp_timer_get_time();
    out.flush();uint32_t written=out.size();out.close();in.close();
    timing.flush=esp_timer_get_time()-phaseStart;phaseStart=esp_timer_get_time();
    uint32_t expected=originalSize-(end-begin)+4+a.size();
    File verify=SD_MMC.open(tmp,FILE_READ);std::vector<uint8_t> check;int vw=0,vh=0;
    ok=ok && written==expected && verify && verify.size()==expected && photoExif(verify,check,vw,vh) && check==jpg && vw==w && vh==h;
    verify.close();
    timing.verify=esp_timer_get_time()-phaseStart;phaseStart=esp_timer_get_time();
    if(!ok) {SD_MMC.remove(tmp);return "VERIFY_FAIL";}
    if(!SD_MMC.rename(path,rollback)) {SD_MMC.remove(tmp);return "RENAME_FAIL";}
    if(!SD_MMC.rename(tmp,path)) {SD_MMC.rename(rollback,path);return "REPLACE_FAIL";}
    timing.rename=esp_timer_get_time()-phaseStart;phaseStart=esp_timer_get_time();
    SD_MMC.remove(rollback);
    timing.remove=esp_timer_get_time()-phaseStart;
    return "SAVED";
}
// Each queued job owns its pixels; GUI never sees or frees this copy.
struct PhotoExifJob {int index,w,h;uint16_t* pixels;};
static QueueHandle_t photoExifQueue=nullptr;
static SemaphoreHandle_t photoFileMutex=nullptr;
static void photoExifWriter(void*) {
    photoExifTaskHandle=xTaskGetCurrentTaskHandle();
    PhotoExifJob job;
    for(;;) {
        if(xQueueReceive(photoExifQueue,&job,portMAX_DELAY)!=pdTRUE) continue;
        // EXIF replacement copies the original file: do not run it during playback.
        while(photoAudioBusy()) vTaskDelay(pdMS_TO_TICKS(250));
        xSemaphoreTake(photoFileMutex,portMAX_DELAY);
        PhotoSaveTiming timing;
        const char* status=photoSaveExif(photoFiles[job.index],job.pixels,job.w,job.h,timing);
        xSemaphoreGive(photoFileMutex);
        free(job.pixels);
        vTaskDelay(1);
    }
}
// Append-only slot database. Publish magic last; incomplete writes are ignored.
struct PhotoDiskRecord {uint32_t magic,capacity,length,size,stamp;char path[256];};
struct PhotoDiskSlot {uint32_t offset;PhotoDiskRecord record;};
static std::vector<PhotoDiskSlot> photoDiskSlots;
static const uint32_t photoDiskMagic=0x54484D32;
static void photoDiskScan() {
    File f=SD_MMC.open("/.thumbs",FILE_READ);
    uint32_t at=0;
    while(f && at+sizeof(PhotoDiskRecord)<=f.size()) {
        PhotoDiskRecord r{};f.seek(at);
        if(f.read((uint8_t*)&r,sizeof(r))!=sizeof(r) || !r.capacity || r.capacity>131072 || r.capacity>f.size()-at-sizeof(r)) break;
        r.path[255]=0;
        photoDiskSlots.push_back({at,r});at+=sizeof(r)+r.capacity;photoScanPause();
    }
    f.close();
}
static bool photoDiskRead(const String& path,lgfx::LGFX_Sprite& sprite) {
    File original=SD_MMC.open(path);if(!original) return false;
    uint32_t size=original.size(),stamp=original.getLastWrite();original.close();
    for(auto& slot:photoDiskSlots) {
        auto& r=slot.record;
        if(r.magic!=photoDiskMagic || path!=r.path || size!=r.size || stamp!=r.stamp || r.length<4 || r.length>r.capacity) continue;
        std::vector<uint8_t> data(r.length);File f=SD_MMC.open("/.thumbs",FILE_READ);
        bool ok=f && f.seek(slot.offset+sizeof(r)) && photoReadChunks(f,data.data(),data.size());f.close();
        if(ok && sprite.drawJpg(data.data(),data.size(),0,0,240,320)) return true;
    }
    return false;
}
static void photoDiskSave(const String& path,uint16_t* pixels) {
    if(path.length()>255) return;
    // Medium quality, full screen frame, including aspect-ratio borders.
    std::vector<uint8_t> data(131072);JPEGENC encoder;JPEGENCODE enc{};
    if(encoder.open(data.data(),data.size())!=JPEGE_SUCCESS) return;
    int rc=encoder.encodeBegin(&enc,240,320,JPEGE_PIXEL_RGB565,JPEGE_SUBSAMPLE_420,JPEGE_Q_MED);
    if(rc==JPEGE_SUCCESS) {
        // Same encoder and quality, but yield between MCU rows.
        for(int y=0;y<320 && rc==JPEGE_SUCCESS;y+=enc.cy) {
            for(int x=0;x<240 && rc==JPEGE_SUCCESS;x+=enc.cx)
                rc=encoder.addMCU(&enc,(uint8_t*)(pixels+y*240+x),480);
            photoWorkPause();
        }
    }
    int bytes=encoder.close();
    if(rc!=JPEGE_SUCCESS || encoder.getLastError()!=JPEGE_SUCCESS || bytes<4 || bytes>int(data.size())) return;
    xSemaphoreTake(photoFileMutex,portMAX_DELAY);
    File original=SD_MMC.open(path);
    PhotoDiskRecord r{};r.capacity=bytes;r.length=bytes;
    if(!original) {xSemaphoreGive(photoFileMutex);return;}
    r.size=original.size();r.stamp=original.getLastWrite();original.close();strncpy(r.path,path.c_str(),255);
    File f=SD_MMC.open("/.thumbs","r+");if(!f) f=SD_MMC.open("/.thumbs","w+");
    if(f) {
        PhotoDiskSlot* reuse=nullptr;
        for(auto& slot:photoDiskSlots) if(slot.record.magic!=photoDiskMagic && slot.record.capacity>=r.length) {reuse=&slot;break;}
        uint32_t at=reuse?reuse->offset:f.size();if(reuse) r.capacity=reuse->record.capacity;
        bool ok=f.seek(at) && f.write((uint8_t*)&r,sizeof(r))==sizeof(r);
        f.flush();
        ok=ok && photoWriteChunks(f,data.data(),bytes);
        // Reserve complete slot before committing its header.
        if(ok && r.capacity>r.length) {ok=f.seek(at+sizeof(r)+r.capacity-1) && f.write(uint8_t(0))==1;}
        f.flush();
        if(ok) {r.magic=photoDiskMagic;ok=f.seek(at) && f.write((uint8_t*)&r,sizeof(r))==sizeof(r);f.flush();}
        if(ok) {if(reuse) reuse->record=r;else photoDiskSlots.push_back({at,r});}
        f.close();
    }
    xSemaphoreGive(photoFileMutex);
}
// Check one entry per idle interval, never from the GUI/audio task.
static void photoDiskMaintain() {
    static size_t cursor=0;static uint32_t last=0;
    if(photoDiskSlots.empty() || millis()-last<2000) return;
    last=millis();if(cursor>=photoDiskSlots.size()) cursor=0;
    auto& slot=photoDiskSlots[cursor++];if(slot.record.magic!=photoDiskMagic) return;
    if(xSemaphoreTake(photoFileMutex,0)!=pdTRUE) return;
    File original=SD_MMC.open(slot.record.path);
    bool valid=original && original.size()==slot.record.size && uint32_t(original.getLastWrite())==slot.record.stamp;original.close();
    if(!valid) {
        File f=SD_MMC.open("/.thumbs","r+");uint32_t zero=0;
        if(f && f.seek(slot.offset) && f.write((uint8_t*)&zero,4)==4) {f.flush();slot.record.magic=0;}
        f.close();
    }
    xSemaphoreGive(photoFileMutex);
}
static void photoWorker(void*) {
    photoWalk("/",0);
    std::sort(photoFiles.begin(),photoFiles.end(),sdNaturalLess);
    photoExifQueue=xQueueCreate(1,sizeof(PhotoExifJob));
    if(!photoFileMutex || !photoExifQueue || xTaskCreate(photoExifWriter,"exifWriter",8192,nullptr,1,nullptr)!=pdPASS) {
        if(photoExifQueue) {vQueueDelete(photoExifQueue);photoExifQueue=nullptr;}

    }
    photoDiskScan();
    photoState.store(2);
    for(;;) {
        if(photoResult.load()>=0) {vTaskDelay(5);continue;}
        int index=photoRequest.exchange(-1);
        if(index<0) {photoDiskMaintain();vTaskDelay(5);continue;}
        photoPhase.store(1);
        bool large=photoLargeRequest.load(); int tw=large?240:108, th=large?320:90;
        photoPixels=nullptr;photoResultOk=false;photoResultLarge=large;
        if(large && photoFileMutex) {
            lgfx::LGFX_Sprite cached;cached.setPsram(true);cached.setColorDepth(16);
            if(cached.createSprite(240,320)) {
                cached.fillScreen(0);xSemaphoreTake(photoFileMutex,portMAX_DELAY);
                bool hit=photoDiskRead(photoFiles[index],cached);xSemaphoreGive(photoFileMutex);
                if(hit) {
                    photoPixels=(uint16_t*)heap_caps_malloc(240*320*2,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
                    if(photoPixels) {
                        const uint16_t* src=(const uint16_t*)cached.getBuffer();
                        for(int k=0;k<240*320;k++) photoPixels[k]=(src[k]>>8)|(src[k]<<8);
                        photoResultOk=true;photoPhase.store(0);photoResult.store(index);continue;
                    }
                }
            }
        }
        int64_t startedUs=esp_timer_get_time();
        uint64_t decodeUs=0,saveUs=0;
        PhotoSaveTiming saveTiming;
        PhotoTimedFile timed;
        PhotoExifJob pending{index,tw,th,nullptr};
        if(photoFileMutex) xSemaphoreTake(photoFileMutex,portMAX_DELAY);
        File f=SD_MMC.open(photoFiles[index]); int w=0,h=0;
        size_t fileBytes=f?f.size():0;
        std::vector<uint8_t> exifJpg;
        int ew=0,eh=0;
        // Full-screen frames always come from the original, never its tiny EXIF thumbnail.
        bool exif=!large && f && photoExif(f,exifJpg,ew,eh);
        if(f && !exif) f.seek(0);
        bool dimensions=exif || (f && photoDimensions(f,w,h));
        f.close();
        if(photoFileMutex) xSemaphoreGive(photoFileMutex);
        const char* source="REDUCED";
        const char* saved="NONE";
        if(exif) {w=ew;h=eh;dimensions=true;}
        if(dimensions) {
            lgfx::LGFX_Sprite sprite; sprite.setPsram(true);sprite.setColorDepth(16);
            if(sprite.createSprite(tw,th)) {
                sprite.fillScreen(0); float scale=std::min(float(tw)/w,float(th)/h);
                int64_t decodeStart=esp_timer_get_time();
                bool ok=false;
                if(exif) {
                    ok=sprite.drawJpg(exifJpg.data(),exifJpg.size(),(tw-int(w*scale))/2,(th-int(h*scale))/2,tw,th,0,0,scale,scale);
                    if(ok) source="EXIF";
                    else {
                        if(photoFileMutex) xSemaphoreTake(photoFileMutex,portMAX_DELAY);
                        File original=SD_MMC.open(photoFiles[index]);
                        dimensions=original && photoDimensions(original,w,h);original.close();
                        if(photoFileMutex) xSemaphoreGive(photoFileMutex);
                        scale=dimensions?std::min(float(tw)/w,float(th)/h):1;
                        sprite.fillScreen(0);
                    }
                }
                if(!ok && dimensions) {
                    if(photoFileMutex) xSemaphoreTake(photoFileMutex,portMAX_DELAY);
                    if(timed.open(photoFiles[index].c_str())) {
                    ok=sprite.drawJpg(&timed,(tw-int(w*scale))/2,(th-int(h*scale))/2,tw,th,0,0,scale,scale);
                    timed.close();
                    }
                    if(photoFileMutex) xSemaphoreGive(photoFileMutex);
                }
                decodeUs=esp_timer_get_time()-decodeStart;
                if(ok) {photoPixels=(uint16_t*)heap_caps_malloc(tw*th*2,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
                    if(photoPixels) { // Lovyan sprite storage is wire-order RGB565; LVGL expects native order.
                        const uint16_t* src=(const uint16_t*)sprite.getBuffer();
                        for(int k=0;k<tw*th;k++) photoPixels[k]=(src[k]>>8)|(src[k]<<8);
                        photoResultOk=true;
                        if(!large && strcmp(source,"REDUCED")==0) {
                            if(photoExifQueue) {
                                pending.pixels=(uint16_t*)heap_caps_malloc(tw*th*2,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
                                if(pending.pixels) {memcpy(pending.pixels,photoPixels,tw*th*2);saved="QUEUED";}
                                else saved="NO_MEMORY";
                            } else if(!photoAudioBusy()) {int64_t saveStart=esp_timer_get_time();saved=photoSaveExif(photoFiles[index],photoPixels,tw,th,saveTiming);saveUs=esp_timer_get_time()-saveStart;}
                        }
                    }
                }
            }
        }
        uint64_t totalUs=esp_timer_get_time()-startedUs;
        uint64_t processingUs=decodeUs>timed.ioUs?decodeUs-timed.ioUs:0;
        if(large && photoResultOk && photoFileMutex) photoDiskSave(photoFiles[index],photoPixels);
        photoPhase.store(0);
        photoResult.store(index);
        // Never wait for the writer; retry uncached thumbnails on a later visit.
        if(pending.pixels && xQueueSend(photoExifQueue,&pending,0)!=pdTRUE) free(pending.pixels);
    }
}
static void photoDescriptor(lv_image_dsc_t& d,uint16_t* pixels,int w,int h) {
    d={};d.header.magic=LV_IMAGE_HEADER_MAGIC;d.header.cf=LV_COLOR_FORMAT_RGB565;
    d.header.w=w;d.header.h=h;d.header.stride=w*2;d.data_size=w*h*2;d.data=(const uint8_t*)pixels;
}
static void photoShow(int index) {
    photoCurrent=index;photoStarted=0;photoViewing=true;
    lv_obj_add_flag(photoGrid,LV_OBJ_FLAG_HIDDEN);lv_obj_remove_flag(photoView,LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(photoBack,LV_OBJ_FLAG_HIDDEN);
    if(globalVolumeButton) lv_obj_add_flag(globalVolumeButton,LV_OBJ_FLAG_HIDDEN);
    closeVolumePopup(nullptr);
    lv_obj_add_flag(photoMessage,LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(photoWaiting,LV_OBJ_FLAG_HIDDEN);
    if(photoNextIndex==index && photoNextPixels) {
        lv_image_set_src(photoImage,nullptr);free(photoFullPixels);
        photoFullPixels=photoNextPixels;photoNextPixels=nullptr;photoNextIndex=-1;
        photoDescriptor(photoFullDsc,photoFullPixels,240,320);
        lv_image_set_src(photoImage,&photoFullDsc);lv_obj_remove_flag(photoImage,LV_OBJ_FLAG_HIDDEN);
        photoStarted=millis();lv_bar_set_value(photoTimer,240,LV_ANIM_OFF);
    }
    // Keep the previous frame visible until the requested frame is ready.
    else {
        if(!photoFullPixels) lv_obj_add_flag(photoImage,LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(photoWaiting,LV_OBJ_FLAG_HIDDEN);
        lv_bar_set_value(photoTimer,0,LV_ANIM_OFF);
    }
}
static void photoChoose(lv_event_t* e) {photoShow(int(uintptr_t(lv_event_get_user_data(e))));}
static void photoExit(lv_event_t*) {
    int index=photoCurrent;photoCurrent=-1;photoStarted=0;photoViewing=false;lastTouchAt=millis();
    if(globalVolumeButton) lv_obj_remove_flag(globalVolumeButton,LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(photoView,LV_OBJ_FLAG_HIDDEN);lv_obj_add_flag(photoMessage,LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(photoGrid,LV_OBJ_FLAG_HIDDEN);
    if(index>=0) lv_obj_scroll_to_y(photoGrid,(index/2)*120,LV_ANIM_OFF);
}
static void photoTap(lv_event_t*) {
    photoBackActivity=millis();
    lv_obj_remove_flag(photoBack,LV_OBJ_FLAG_HIDDEN);
}
static void buildGalleryScreen() {
    galleryScreen=mustCreate(nullptr,"gallery");lv_obj_remove_style_all(galleryScreen);
    lv_obj_set_style_bg_color(galleryScreen,lv_color_hex(0),0);lv_obj_set_style_bg_opa(galleryScreen,LV_OPA_COVER,0);
    lv_obj_set_style_text_font(galleryScreen,&dejavu_16,0);lv_obj_set_style_text_color(galleryScreen,lv_color_hex(0xffffff),0);
    lv_obj_remove_flag(galleryScreen,LV_OBJ_FLAG_SCROLLABLE);
    photoGrid=mustCreate(galleryScreen,"photoGrid");lv_obj_remove_style_all(photoGrid);
    lv_obj_set_pos(photoGrid,0,32);lv_obj_set_size(photoGrid,240,288);lv_obj_set_scroll_dir(photoGrid,LV_DIR_VER);
    lv_obj_t* title=mustLabel(galleryScreen,"galleryTitle");lv_label_set_text(title,"Галерея");lv_obj_align(title,LV_ALIGN_TOP_MID,0,6);
    photoView=mustCreate(galleryScreen,"photoView");lv_obj_remove_style_all(photoView);lv_obj_set_size(photoView,240,320);
    lv_obj_set_style_bg_color(photoView,lv_color_hex(0),0);lv_obj_set_style_bg_opa(photoView,LV_OPA_COVER,0);
    lv_obj_remove_flag(photoView,LV_OBJ_FLAG_SCROLLABLE);lv_obj_add_flag(photoView,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(photoView,photoTap,LV_EVENT_CLICKED,nullptr);
    photoImage=lv_image_create(photoView);lv_obj_remove_flag(photoImage,LV_OBJ_FLAG_CLICKABLE);
    photoTimer=lv_bar_create(photoView);lv_obj_set_pos(photoTimer,0,316);lv_obj_set_size(photoTimer,240,4);
    lv_bar_set_range(photoTimer,0,240);lv_obj_set_style_bg_color(photoTimer,lv_color_hex(0x40d060),LV_PART_INDICATOR);
    lv_obj_set_style_radius(photoTimer,0,LV_PART_MAIN);lv_obj_set_style_radius(photoTimer,0,LV_PART_INDICATOR);
    // Static, opaque pixel icon: no font, smoothing or animation work.
    photoWaiting=mustCreate(photoView,"photoWaiting");lv_obj_remove_style_all(photoWaiting);
    lv_obj_set_size(photoWaiting,48,56);lv_obj_center(photoWaiting);
    lv_obj_set_style_bg_color(photoWaiting,lv_color_hex(0),0);
    lv_obj_set_style_bg_opa(photoWaiting,LV_OPA_COVER,0);
    lv_obj_remove_flag(photoWaiting,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(photoWaiting,LV_OBJ_FLAG_SCROLLABLE);
    static const char* hourglass[]={
        "############", "############", " #        # ", " # ###### # ",
        "  # #### #  ", "   # ## #   ", "    #  #    ", "     ##     ",
        "     ##     ", "    #  #    ", "   # ## #   ", "  # #### #  ",
        " # ###### # ", " #        # ", "############", "############"
    };
    for(int y=0;y<16;y++) for(int x=0;x<12;) {
        if(hourglass[y][x]!='#') {++x;continue;}
        int start=x;while(x<12 && hourglass[y][x]=='#') ++x;
        lv_obj_t* run=mustCreate(photoWaiting,"hourglassPixel");lv_obj_remove_style_all(run);
        lv_obj_set_pos(run,6+start*3,4+y*3);lv_obj_set_size(run,(x-start)*3,3);
        lv_obj_set_style_bg_color(run,lv_color_hex(0xffffff),0);
        lv_obj_set_style_bg_opa(run,LV_OPA_COVER,0);
        lv_obj_remove_flag(run,LV_OBJ_FLAG_CLICKABLE);lv_obj_remove_flag(run,LV_OBJ_FLAG_SCROLLABLE);
    }
    lv_obj_add_flag(photoWaiting,LV_OBJ_FLAG_HIDDEN);
    photoBack=mustButton(photoView,"photoBack");lv_obj_set_size(photoBack,40,30);lv_obj_set_pos(photoBack,4,4);
    navigationArrow(photoBack,false,0x80ff80);sdPressedStyle(photoBack);lv_obj_add_event_cb(photoBack,photoExit,LV_EVENT_CLICKED,nullptr);
    lv_obj_add_flag(photoView,LV_OBJ_FLAG_HIDDEN);
    photoMessage=mustLabel(galleryScreen,"photoMessage");lv_obj_set_width(photoMessage,220);lv_obj_set_pos(photoMessage,10,140);
    lv_label_set_text(photoMessage,"Считывание SD ...");
}
static void updateGallery() {
    if(lv_screen_active()!=galleryScreen) return;
    if(photoState.load()==0) {
        static bool mountRequested=false;
        if(!mountRequested && (sdScanState.load()==0 || sdScanState.load()==3)) {mountRequested=true;sdRetry(nullptr);}
        if(sdScanState.load()==3) {lv_label_set_text(photoMessage,"Подключите SD карту");return;}
        if(sdScanState.load()!=2) return;
        if(!photoFileMutex) photoFileMutex=xSemaphoreCreateMutex();
        if(!photoFileMutex) {lv_label_set_text(photoMessage,"Не хватает памяти");return;}
        photoState.store(1);
        if(xTaskCreate(photoWorker,"photoWorker",16384,nullptr,1,nullptr)!=pdPASS) {photoState.store(0);lv_label_set_text(photoMessage,"Не хватает памяти");}
    }
    if(photoState.load()!=2) return;
    if(photoTiles.empty()) {
        lv_label_set_text(photoMessage,photoFiles.empty()?"Изображения не найдены":"");
        // Eight reusable tiles instead of up to 1000 LVGL object trees.
        lv_obj_t* extent=lv_obj_create(photoGrid);lv_obj_remove_style_all(extent);
        lv_obj_set_size(extent,1,1);lv_obj_set_pos(extent,0,std::max(0,int((photoFiles.size()+1)/2)*120-1));
        lv_obj_remove_flag(extent,LV_OBJ_FLAG_CLICKABLE);
        for(int i=0;i<8 && !photoFiles.empty();i++) {
            lv_obj_t* tile=mustButton(photoGrid,"photoTile");lv_obj_set_size(tile,108,112);
            lv_obj_set_style_pad_all(tile,0,0);sdPressedStyle(tile);
            lv_obj_t* im=lv_image_create(tile);lv_obj_set_pos(im,0,0);lv_obj_remove_flag(im,LV_OBJ_FLAG_CLICKABLE);
            lv_obj_t* label=mustLabel(tile,"photoName");lv_obj_set_pos(label,2,91);lv_obj_set_size(label,104,20);
            // A bounded single line prevents wrapped filename fragments below the tile.
            lv_obj_set_style_pad_all(label,0,0);
            lv_obj_set_style_text_line_space(label,0,0);
            lv_label_set_long_mode(label,LV_LABEL_LONG_DOT);lv_obj_remove_flag(label,LV_OBJ_FLAG_CLICKABLE);
            lv_obj_t* status=mustLabel(tile,"photoStatus");
            lv_obj_set_width(status,104);lv_obj_set_pos(status,2,28);
            lv_obj_set_style_text_align(status,LV_TEXT_ALIGN_CENTER,0);
            lv_obj_remove_flag(status,LV_OBJ_FLAG_CLICKABLE);
            lv_label_set_text(status,"Ожидание");
            photoTiles.push_back(tile);
        }
    }
    int gridTop=std::max(0,int(lv_obj_get_scroll_y(photoGrid)/120));
    if(gridTop!=photoGridTop) {
        photoGridTop=gridTop;
        for(int j=0;j<int(photoTiles.size());j++) {
            int index=gridTop*2+j;lv_obj_t* tile=photoTiles[j];
            photoTileIndex[j]=index<int(photoFiles.size())?index:-1;
            lv_obj_remove_event_cb(tile,photoChoose);
            lv_image_set_src(lv_obj_get_child(tile,0),nullptr);
            if(photoTileIndex[j]<0) {lv_obj_add_flag(tile,LV_OBJ_FLAG_HIDDEN);continue;}
            lv_obj_remove_flag(tile,LV_OBJ_FLAG_HIDDEN);lv_obj_set_pos(tile,6+(index%2)*120,(index/2)*120);
            lv_obj_add_event_cb(tile,photoChoose,LV_EVENT_CLICKED,(void*)uintptr_t(index));
            String name=photoFiles[index].substring(photoFiles[index].lastIndexOf('/')+1);
            lv_label_set_text(lv_obj_get_child(tile,1),name.c_str());
            for(auto& c:photoCache) if(c.index==index && c.pixels) lv_image_set_src(lv_obj_get_child(tile,0),&c.dsc);
        }
    }
    int result=photoResult.load();
    if(result>=0) {
        if(photoResultLarge && result==photoCurrent) {
            lv_obj_add_flag(photoWaiting,LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(photoMessage,LV_OBJ_FLAG_HIDDEN);
            if(photoResultOk) {lv_image_set_src(photoImage,nullptr);free(photoFullPixels);photoFullPixels=photoPixels;photoPixels=nullptr;
                photoDescriptor(photoFullDsc,photoFullPixels,240,320);lv_image_set_src(photoImage,&photoFullDsc);lv_obj_remove_flag(photoImage,LV_OBJ_FLAG_HIDDEN);
            } else {lv_label_set_text(photoMessage,"Не удалось открыть JPG");lv_obj_remove_flag(photoMessage,LV_OBJ_FLAG_HIDDEN);}
            photoStarted=millis();lv_bar_set_value(photoTimer,240,LV_ANIM_OFF);
        } else if(photoResultLarge) {
            if(photoResultOk && photoCurrent>=0 && result==(photoCurrent+1)%int(photoFiles.size())) {
                free(photoNextPixels);photoNextPixels=photoPixels;photoPixels=nullptr;photoNextIndex=result;
            }
        } else if(!photoResultLarge) {
            int slot=-1;int top=lv_obj_get_scroll_y(photoGrid)/120;
            for(int i=0;i<8;i++) if(photoCache[i].index<0 || photoCache[i].index/2<top || photoCache[i].index/2>top+2) {slot=i;break;}
            if(slot>=0) {auto& c=photoCache[slot];for(int j=0;j<int(photoTiles.size());j++) if(photoTileIndex[j]==c.index) lv_image_set_src(lv_obj_get_child(photoTiles[j],0),nullptr);
                free(c.pixels);c.index=result;c.pixels=photoPixels;photoPixels=nullptr;photoDescriptor(c.dsc,c.pixels,108,90);
                for(int j=0;j<int(photoTiles.size());j++) if(photoTileIndex[j]==result && c.pixels) lv_image_set_src(lv_obj_get_child(photoTiles[j],0),&c.dsc);}
        }
        free(photoPixels);photoPixels=nullptr;photoLoading=-1;photoResult.store(-1);
    }
    for(int j=0;j<int(photoTiles.size());j++) {
        int idx=photoTileIndex[j];if(idx<0) continue;
        bool ready=false,failed=false;
        for(auto& c:photoCache) if(c.index==idx) {ready=c.pixels!=nullptr;failed=!ready;}
        const char* text=ready?"":failed?"Ошибка JPG":idx==photoLoading?(photoPhase.load()==2?"Запись EXIF":"Обработка"):"Ожидание";
        lv_obj_t* status=lv_obj_get_child(photoTiles[j],2);
        if(strcmp(lv_label_get_text(status),text)!=0) lv_label_set_text(status,text);
    }
    if(photoCurrent>=0) {
        lastTouchAt=millis();
        if(!lv_obj_has_flag(photoBack,LV_OBJ_FLAG_HIDDEN) && millis()-photoBackActivity>=3000)
            lv_obj_add_flag(photoBack,LV_OBJ_FLAG_HIDDEN);
        if(photoStarted) {uint32_t elapsed=millis()-photoStarted;int value=elapsed>=60000?0:240-elapsed/250;
            if(lv_bar_get_value(photoTimer)!=value) lv_bar_set_value(photoTimer,value,LV_ANIM_OFF);
            if(elapsed>=60000) photoShow((photoCurrent+1)%photoFiles.size());}
        if(photoLoading<0) {
            int wanted=!photoStarted?photoCurrent:(photoCurrent+1)%int(photoFiles.size());
            if(!photoStarted || photoNextIndex!=wanted) {
                photoLoading=wanted;photoLargeRequest.store(true);photoRequest.store(wanted);
            }
        }
    } else if(photoLoading<0 && !photoFiles.empty()) {
        int top=std::max(0,int(lv_obj_get_scroll_y(photoGrid)/120));
        for(int i=top*2;i<std::min(int(photoFiles.size()),(top+3)*2);i++) {
            bool cached=false;for(auto& c:photoCache) if(c.index==i) cached=true;
            if(!cached) {photoLoading=i;photoLargeRequest.store(false);photoRequest.store(i);break;}
        }
    }
}
