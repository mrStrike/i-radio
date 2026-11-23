#pragma once
#include "station_catalog.h"
// Веб-пульт храню в прошивке. HTTP обслуживает собственная задача
// ESP-IDF с низким приоритетом; из неё не вызываю Audio, LVGL или Preferences.
// Порт 8080 отделяю от порта 80 существующей настройки Wi-Fi.
static const char remotePage[] PROGMEM = R"HTML(<!doctype html>
<html lang="ru"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Моё радио — пульт</title><style>
/* Оформляю пульт плоскими плитками Metro, без внешних шрифтов и картинок. */
:root{color-scheme:dark;--blue:#0078d7}*{box-sizing:border-box}
body{background:#080808;color:#fff;font:17px/1.45 "Segoe UI",Arial,sans-serif;margin:0;padding:28px 20px;max-width:860px;margin-inline:auto}
h1{font-size:38px;font-weight:300;margin:0}header{border-bottom:1px solid #333;padding-bottom:20px;margin-bottom:24px}.subtitle{color:#aaa;margin:4px 0 0;font-size:14px;letter-spacing:2px;text-transform:uppercase}
h2{font-size:24px;font-weight:300;margin:0 0 14px}.section{margin:26px 0}.now{background:#181818;border-left:4px solid var(--blue);padding:20px;margin-bottom:16px}
#station{font-size:28px;font-weight:300;margin:0;overflow-wrap:anywhere}#status{color:#bbb;margin:8px 0 0;min-height:25px}
button,select,input{font:inherit;border-radius:0}button{border:0;background:#292929;color:#fff;padding:14px 18px;cursor:pointer;min-height:52px;text-align:left;overflow-wrap:anywhere}
button:hover{outline:2px solid #ddd;outline-offset:-2px}button:active{filter:brightness(.8)}button:disabled{opacity:.4;cursor:default}button:focus-visible,select:focus-visible,input:focus-visible{outline:2px solid #fff;outline-offset:3px}
.actions{display:flex;gap:10px;flex-wrap:wrap}.actions button{flex:1}#toggle,#play,#setTimer{background:var(--blue)}#stop{background:#333}
select{width:100%;background:#161616;color:#fff;border:1px solid #666;padding:12px;margin:0 0 12px;min-height:50px}
input[type=range]{width:100%;accent-color:var(--blue);margin:16px 0;cursor:pointer;min-height:28px}
.value{float:right;font-size:22px;font-weight:400}#favorites{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px;margin-bottom:14px}
#favorites button{min-height:112px;font-size:23px;font-weight:300;padding:18px;background:#a200ff}
#favorites button[data-slot="1"]{background:#ff0097}
#favorites button[data-slot="2"]{background:#00aba9}
#favorites button[data-slot="3"]{background:#8cbf26}
#favorites button[data-slot="4"]{background:#a05000}
#favorites button[data-slot="5"]{background:#e671b8}
#favorites button[data-slot="6"]{background:#1ba1e2}
#favorites button[data-slot="7"]{background:#e51400}
#favorites button[data-slot="8"]{background:#339933}
#favorites:empty:before{content:"Пока нет избранных станций";color:#999;grid-column:1/-1;padding:16px 0}
.minor button{font-size:15px;background:#202020}.settings{border-top:1px solid #333;padding-top:24px}.settings-grid{display:grid;grid-template-columns:1fr 1fr;gap:20px}label{display:block;color:#ccc}label select{margin-top:10px}#remaining{color:#bbb;margin-bottom:0}#error{color:#ff8585}#error:empty{display:none}footer{border-top:1px solid #333;color:#888;font-size:14px;margin-top:28px;padding-top:16px}
@media(max-width:520px){body{padding:20px 16px}h1{font-size:32px}#station{font-size:24px}.settings-grid{grid-template-columns:1fr}#favorites button{font-size:20px}.minor{flex-direction:column}}
details.mp3{background:#181818;padding:14px;margin:20px 0}details.mp3 summary{cursor:pointer;color:#80ff80}#mp3Name{white-space:nowrap;overflow:hidden;text-overflow:ellipsis}.mp3 .actions button{text-align:center;color:#80ff80;padding:10px;min-width:40px}.mp3 svg{width:22px;height:22px;fill:currentColor;shape-rendering:crispEdges}#mp3Files{max-height:300px;overflow:auto;margin:12px 0}#mp3Files button{display:block;width:100%;white-space:nowrap;overflow:hidden;text-overflow:ellipsis;min-height:42px;padding:8px 12px;font-size:15px}#mp3Files button.current{background:#0078d7}#mp3Time,#mp3Page{font-size:14px;color:#bbb}
/* MP3: компактный список и единый зелёный акцент, отдельно от радио. */
details.mp3{background:#101410;border:1px solid #344438;border-top:3px solid #80ff80;padding:14px}
.mp3 .actions{gap:6px}.mp3 .actions button{background:#243028;border:1px solid #3c5040;min-height:44px}
#mp3Name{color:#e0ffe0;margin:16px 0 6px}#mp3Time{font-variant-numeric:tabular-nums;color:#a6b8aa}
#mp3Files{background:#080c08;border:1px solid #344438}
#mp3Files button{min-height:32px;padding:4px 10px;line-height:24px;background:#101410;color:#c8d8cc;border-bottom:1px solid #202a22}
#mp3Files button.current{background:#183820;color:#80ff80;border-left:3px solid #80ff80;padding-left:7px}
#mp3Seek{appearance:none;-webkit-appearance:none;accent-color:#80ff80;background:transparent;min-height:30px;margin:10px 0}
#mp3Seek::-webkit-slider-runnable-track{height:14px;background:#29342c;border:1px solid #3c5040}
#mp3Seek::-webkit-slider-thumb{-webkit-appearance:none;width:12px;height:22px;margin-top:-5px;background:#80ff80;border:1px solid #101410;border-radius:0}
#mp3Seek::-moz-range-track{height:14px;background:#29342c;border:1px solid #3c5040}
#mp3Seek::-moz-range-progress{height:14px;background:#80ff80}
#mp3Seek::-moz-range-thumb{width:12px;height:22px;background:#80ff80;border:1px solid #101410;border-radius:0}
#mp3Seek:disabled{opacity:.4}
</style></head><body>
<header><h1>Моё радио</h1><p class="subtitle">Домашний веб-пульт</p></header>
<main><section aria-label="Текущая станция"><div class="now"><p id="station">Загрузка…</p><p id="status" aria-live="polite"></p><div class="actions minor" style="margin-top:14px"><button id="currentFavorite" disabled>Выберите станцию</button></div></div>
<div class="actions"><button id="toggle" disabled>Пуск / Пауза</button><button id="stop">Остановить</button></div></section>
<section class="section"><h2>Громкость <span class="value" id="volumeText"></span></h2><input aria-label="Громкость" id="volume" type="range" min="0" max="21"></section>
<details class="mp3" id="mp3Panel"><summary>MP3 с SD-карты</summary>
<p id="mp3Name">Файл не выбран</p><div id="mp3Time">00:00:00 / 00:00:00</div>
<input id="mp3Seek" aria-label="Перемотка MP3" type="range" min="0" max="1" value="0" disabled>
<div class="actions" id="mp3Controls"></div>
<button id="mp3Retry">Прочитать SD-карту</button><p id="mp3Info"></p>
<div id="mp3Files"></div><div class="actions minor"><button id="mp3PrevPage">←</button><span id="mp3Page"></span><button id="mp3NextPage">→</button></div>
</details>
<section class="section"><h2>Избранное <span id="favoriteCount" style="color:#aaa;font-size:18px">· 0 из 20</span></h2><div id="favorites"></div></section>
<section class="section"><h2>Станции</h2><select aria-label="Выбор станции" id="stations"></select><div class="actions"><button id="play">Слушать выбранную</button></div>
<p style="color:#aaa;margin:14px 0 8px">Выбранная станция</p><div class="actions minor"><button id="selectedFavorite" disabled>Выберите станцию</button></div></section>
<details class="section"><summary>Редактор станций на SD</summary><p id="catalogInfo"></p><p>Изменения применяются после перезапуска радио. До него текущий список и звук не меняются.</p><select id="catalogSelect"></select><label>Название<input id="catalogName" maxlength="120" style="width:100%"></label><label>Адрес потока<input id="catalogUrl" maxlength="768" style="width:100%"></label><div class="actions"><button id="catalogUpdate">Изменить</button><button id="catalogAdd">Добавить</button><button id="catalogDelete">Удалить</button></div><button id="catalogSave">Сохранить на SD</button><p id="catalogResult"></p></details>
<section class="section settings"><h2>Настройки</h2><label>Яркость <span class="value" id="brightnessText"></span><input id="brightness" type="range" min="15" max="100"></label>
<div class="settings-grid"><label>Сон экрана<select id="sleep"><option value="0">Выключен</option><option value="1">Через 2 минуты</option><option value="2">Через 5 минут</option><option value="3">Через 10 минут</option><option value="4">Через 15 минут</option><option value="5">Через 30 минут</option><option value="6">Через час</option></select></label>
<div><label>Остановка радио<select id="timer"><option value="0">Отменить таймер</option><option value="1">Через 15 минут</option><option value="2">Через 30 минут</option><option value="3">Через 60 минут</option><option value="4">Через 90 минут</option></select></label><button id="setTimer">Применить таймер</button><p id="remaining"></p></div></div></section>
<p id="error" role="alert"></p></main><footer>Пульт доступен в моей Wi-Fi-сети. Таймер радио действует только на текущий сеанс. Настройку подключения Wi-Fi запускаю на самом радио.</footer>
<script>
const $=id=>document.getElementById(id);let busy=false,editing=false,loaded=false,brightEditing=false,remoteStateCache=null;
// Для текущей и выбранной станции показываю только подходящее действие.
// При заполненных двадцати местах оставляю удаление доступным.
function updateFavoriteButtons(){
if(!remoteStateCache)return;
const s=remoteStateCache,count=s.favorites.filter(id=>id>=0).length;
$('favoriteCount').textContent='· '+count+' из 20';
for(const [button,id] of [[$('currentFavorite'),s.station],[$('selectedFavorite'),Number($('stations').value)]]){
const valid=Number.isInteger(id)&&s.stations.some(x=>x.id===id),exists=valid&&s.favorites.includes(id);
button.textContent=!valid?'Выберите станцию':exists?'− Удалить из избранного':count>=20?'Избранное заполнено — удалите одну станцию':'+ Добавить в избранное';
button.disabled=!valid||(!exists&&count>=20);
button.onclick=()=>command(exists?'favremove':'favadd',id);
}
}
async function command(action,value){try{const r=await fetch('/api/control',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded','X-Radio-Control':'1'},body:new URLSearchParams({action,value:String(value??'')})});if(!r.ok)throw Error(await r.text());$('error').textContent='';await refresh()}catch(e){$('error').textContent='Команда не принята: '+e.message}}
async function refresh(){if(busy)return;busy=true;try{const r=await fetch('/api/state'+(loaded?'':'?full=1'),{cache:'no-store',signal:AbortSignal.timeout(4000)});if(!r.ok)throw Error('HTTP '+r.status);const s=await r.json();if(!s.stations)s.stations=remoteStateCache?.stations||[];remoteStateCache=s;$('station').textContent=s.name||'Станция не выбрана';$('status').textContent=s.status;$('toggle').textContent=s.requested?'Пауза':'Продолжить';$('toggle').disabled=s.station<0;if(!editing){$('volume').value=s.volume;$('volumeText').textContent=Math.round(s.volume*100/21)+'%'}if(!loaded){for(const x of s.stations){const o=document.createElement('option');o.value=x.id;o.textContent=x.name;$('stations').append(o)}loaded=true;if(s.station>=0)$('stations').value=s.station}if(!brightEditing){$('brightness').value=s.brightness;$('brightnessText').textContent=s.brightness+'%'}
updateFavoriteButtons();
updateMp3(s.mp3);
if(document.activeElement!==$('sleep'))$('sleep').value=s.sleep;
$('remaining').textContent=s.remaining?'До остановки радио: '+Math.ceil(s.remaining/60)+' мин':'Таймер радио выключен';
const signature=JSON.stringify(s.favorites);if($('favorites').dataset.signature!==signature){$('favorites').dataset.signature=signature;$('favorites').replaceChildren();let colorIndex=0;for(const [slot,id] of s.favorites.entries()){if(id<0)continue;const button=document.createElement('button');button.dataset.slot=colorIndex++%9;button.textContent=s.stations[id].name;button.onclick=()=>command('play',id);$('favorites').append(button)}}
}catch(e){$('error').textContent='Нет связи с радио: '+e.message}finally{busy=false}}
$('toggle').onclick=()=>command('toggle');$('stop').onclick=()=>command('stop');$('play').onclick=()=>command('play',$('stations').value);
$('volume').oninput=()=>{editing=true;$('volumeText').textContent=Math.round($('volume').value*100/21)+'%'};
$('volume').onchange=()=>{editing=false;command('volume',$('volume').value)};
$('stations').onchange=updateFavoriteButtons;
$('brightness').oninput=()=>{brightEditing=true;$('brightnessText').textContent=$('brightness').value+'%'};
$('brightness').onchange=()=>{brightEditing=false;command('brightness',$('brightness').value)};
$('sleep').onchange=()=>command('sleep',$('sleep').value);$('setTimer').onclick=()=>command('timer',$('timer').value);
let mp3Page=0,mp3Track=-2,mp3Signature='',seekEditing=false;
const clock=n=>[Math.floor(n/3600),Math.floor(n/60)%60,n%60].map(x=>String(x).padStart(2,'0')).join(':');
const icons=[['sdprev','Предыдущий','M4 3h3v16H4z M18 3v16L8 11z'],['sdplay','Воспроизвести','M5 3v16l14-8z'],['sdpause','Пауза','M5 3h4v16H5z M13 3h4v16h-4z'],['sdstop','Стоп','M4 3h16v16H4z'],['sdnext','Следующий','M15 3h3v16h-3z M4 3v16l10-8z']];
for(const [action,label,path] of icons){const b=document.createElement('button');b.title=label;b.setAttribute('aria-label',label);b.innerHTML='<svg viewBox="0 0 24 22" aria-hidden="true"><path d="'+path+'"/></svg>';b.onclick=()=>command(action);$('mp3Controls').append(b)}
function updateMp3(m){if(!m)return;
$('mp3Name').textContent=m.name||'Файл не выбран';$('mp3Time').textContent=clock(m.elapsed)+' / '+clock(m.duration);
if(!seekEditing){$('mp3Seek').max=Math.max(1,m.duration-1);$('mp3Seek').value=m.elapsed}
$('mp3Seek').disabled=!m.active||!m.duration;
$('mp3Retry').hidden=m.scan===1||m.scan===2;
$('mp3Info').textContent=m.scan===1?'Чтение карты…':m.scan===3?'Карта не обнаружена или не читается':m.scan===2?'Файлов: '+m.count+(m.paused?' · Пауза':m.active?' · Воспроизведение':''):'';
$('mp3Controls').querySelectorAll('button').forEach(b=>b.disabled=m.scan!==2||!m.count);
if(m.current!==mp3Track){mp3Track=m.current;if(m.current>=0){mp3Page=Math.floor(m.current/30);mp3Signature=''}}
if($('mp3Panel').open)loadMp3Files();
}
async function loadMp3Files(){const m=remoteStateCache?.mp3;if(!m||m.scan!==2)return;
const sig=mp3Page+':'+m.current+':'+m.count;if(sig===mp3Signature)return;
try{const page=mp3Page;const r=await fetch('/api/mp3?page='+page,{cache:'no-store'});if(!r.ok)throw Error('HTTP '+r.status);const d=await r.json();if(page!==mp3Page)return;
$('mp3Files').replaceChildren();for(const f of d.files){const b=document.createElement('button');b.textContent=f.name;b.title=f.name;b.className=f.id===m.current?'current':'';b.onclick=()=>command('sdfile',f.id);$('mp3Files').append(b)}
$('mp3Page').textContent=(page+1)+' / '+Math.max(1,Math.ceil(m.count/30));$('mp3PrevPage').disabled=page===0;$('mp3NextPage').disabled=(page+1)*30>=m.count;mp3Signature=sig;
$('mp3Files .current')?.scrollIntoView({block:'nearest'});
}catch(e){$('error').textContent='Список MP3: '+e.message}}
$('mp3Panel').ontoggle=()=>{if($('mp3Panel').open){if(remoteStateCache?.mp3.scan===0)command('sdscan');else loadMp3Files()}};
$('mp3Retry').onclick=()=>command('sdscan');
$('mp3PrevPage').onclick=()=>{if(mp3Page>0){mp3Page--;loadMp3Files()}};
$('mp3NextPage').onclick=()=>{if((mp3Page+1)*30<(remoteStateCache?.mp3.count||0)){mp3Page++;loadMp3Files()}};
$('mp3Seek').oninput=()=>seekEditing=true;
$('mp3Seek').onchange=()=>{seekEditing=false;command('sdseek',$('mp3Seek').value)};
let catalog=null;
function drawCatalog(){const sel=$('catalogSelect');sel.replaceChildren();for(const [i,x] of catalog.stations.entries()){const o=document.createElement('option');o.value=i;o.textContent=x.name;sel.append(o)}selectCatalog()}
function selectCatalog(){const x=catalog?.stations[Number($('catalogSelect').value)];$('catalogName').value=x?.name||'';$('catalogUrl').value=x?.url||''}
$('catalogSelect').onchange=selectCatalog;
async function loadCatalog(){try{const r=await fetch('/api/catalog');const d=await r.json();$('catalogInfo').textContent=d.message;catalog=d.catalog;for(const id of ['catalogSelect','catalogName','catalogUrl','catalogUpdate','catalogAdd','catalogDelete','catalogSave'])$(id).disabled=!d.writable;if(d.writable)drawCatalog()}catch(e){$('catalogInfo').textContent='Не удалось прочитать каталог'}}
function editCatalog(add){const name=$('catalogName').value.trim(),url=$('catalogUrl').value.trim();if(!name||!/^https?:\/\/\S+$/.test(url))return alert('Введите название и HTTP/HTTPS адрес потока');if(catalog.stations.some((x,i)=>x.url===url&&(add||i!==Number($('catalogSelect').value))))return alert('Этот адрес уже есть');if(add){if(catalog.stations.length>=150)return alert('Максимум 150 станций');catalog.stations.push({name,url})}else{const x=catalog.stations[Number($('catalogSelect').value)];if(x.url!==url){x.aliases=[...new Set([...(x.aliases||[]),x.url])];if(x.aliases.length>16)return alert('Слишком много замен адреса: удалите и добавьте станцию заново')}x.name=name;x.url=url}drawCatalog();$('catalogResult').textContent='Есть несохранённые изменения'}
$('catalogUpdate').onclick=()=>editCatalog(false);$('catalogAdd').onclick=()=>editCatalog(true);
$('catalogDelete').onclick=()=>{if(catalog.stations.length<=1)return alert('Оставьте хотя бы одну станцию');if(confirm('Удалить выбранную станцию?')){catalog.stations.splice(Number($('catalogSelect').value),1);drawCatalog();$('catalogResult').textContent='Есть несохранённые изменения'}};
$('catalogSave').onclick=async()=>{const b=$('catalogSave');b.disabled=true;try{const r=await fetch('/api/catalog',{method:'POST',headers:{'Content-Type':'application/json','X-Radio-Control':'1'},body:JSON.stringify(catalog)});if(!r.ok)throw Error(await r.text());$('catalogResult').textContent='Сохранено. Перезапустите радио, чтобы применить список.'}catch(e){$('catalogResult').textContent='Не сохранено: '+e.message}finally{b.disabled=false}};
loadCatalog();
async function poll(){await refresh();setTimeout(poll,2000)}poll();
</script></body></html>)HTML";

// Экранирую строки JSON, чтобы названия из моего списка не ломали ответ.
// A bounded output buffer; failures stop further writes, never send partial success.
class RemoteJsonWriter {
    httpd_req_t* req; char buffer[512]; size_t used=0; esp_err_t result=ESP_OK;
public:
    explicit RemoteJsonWriter(httpd_req_t* r):req(r) {
        httpd_resp_set_type(req,"application/json; charset=utf-8");
        httpd_resp_set_hdr(req,"Cache-Control","no-store");
    }
    void flush() {
        if(result==ESP_OK && used) result=httpd_resp_send_chunk(req,buffer,used);
        used=0;
    }
    void ch(char c) { if(result!=ESP_OK)return; if(used==sizeof(buffer))flush(); if(result==ESP_OK)buffer[used++]=c; }
    void text(const char* s) { while(*s && result==ESP_OK)ch(*s++); }
    void number(long n) { char b[24];snprintf(b,sizeof(b),"%ld",n);text(b); }
    void boolean(bool b) { text(b?"true":"false"); }
    void json(const char* s) {
        ch('"');
        for(const unsigned char* p=(const unsigned char*)s;*p && result==ESP_OK;++p) {
            if(*p=='"'||*p=='\\'){ch('\\');ch(*p);}
            else if(*p<32){char b[7];snprintf(b,sizeof(b),"\\u%04x",*p);text(b);}
            else ch(*p);
        }
        ch('"');
    }
    esp_err_t finish() {flush();return result==ESP_OK?httpd_resp_send_chunk(req,nullptr,0):result;}
};
static esp_err_t remoteRoot(httpd_req_t* req) {
    httpd_resp_set_type(req,"text/html; charset=utf-8");
    return httpd_resp_send(req,remotePage,sizeof(remotePage)-1);
}
// Copy under the playlist mutex; never keep it locked during network sends.
static bool remoteMp3Name(int index,char* name,size_t capacity) {
    name[0]=0;
    if(xSemaphoreTake(sdFilesMutex,pdMS_TO_TICKS(20))!=pdTRUE)return false;
    bool ready=sdScanState.load()==2;
    if(ready && index>=0 && index<sdFileCount.load()) {
        const char* path=sdMp3Files[index].c_str();
        const char* slash=strrchr(path,'/');
        strlcpy(name,slash?slash+1:path,capacity);
    }
    xSemaphoreGive(sdFilesMutex);
    return ready;
}
static esp_err_t remoteMp3Files(httpd_req_t* req) {
    char query[32]={},value[12]={};int page=0;
    if(httpd_req_get_url_query_str(req,query,sizeof(query))==ESP_OK && httpd_query_key_value(query,"page",value,sizeof(value))==ESP_OK) {
        char* end;long n=strtol(value,&end,10);
        if(!value[0]||*end||n<0||n>66)return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Invalid page");
        page=int(n);
    }
    // Snapshot this page, then release the mutex before any TCP transmission.
    String names[30];int count=0;
    if(xSemaphoreTake(sdFilesMutex,pdMS_TO_TICKS(20))!=pdTRUE) {
        httpd_resp_set_status(req,"503 Service Unavailable");
        return httpd_resp_send(req,"Playlist busy",HTTPD_RESP_USE_STRLEN);
    }
    if(sdScanState.load()==2) {
        int end=std::min((page+1)*30,sdFileCount.load());
        for(int i=page*30;i<end;++i) {
            const String& path=sdMp3Files[i];names[count++]=path.substring(path.lastIndexOf('/')+1);
        }
    }
    xSemaphoreGive(sdFilesMutex);
    RemoteJsonWriter out(req);out.text("{\"files\":[");
    for(int i=0;i<count;++i){if(i)out.ch(',');out.text("{\"id\":");out.number(page*30+i);out.text(",\"name\":");out.json(names[i].c_str());out.ch('}');}
    out.text("]}");return out.finish();
}
static esp_err_t remoteState(httpd_req_t* req) {
    char query[32] = {}, fullValue[8] = {};
    const bool full = httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK &&
        httpd_query_key_value(query, "full", fullValue, sizeof(fullValue)) == ESP_OK &&
        strcmp(fullValue, "1") == 0;
    char status[96],name[512];
    portENTER_CRITICAL(&webStatusMux);strlcpy(status,webStatus,sizeof(status));portEXIT_CRITICAL(&webStatusMux);
    int id=currentStation;
    if(id>=100)id-=100;else if(id>=0 && id<FAVORITES_COUNT)id=favoriteIds[id].load();
    if(id<0||id>=ALL_STATIONS_COUNT)id=-1;
    int track=sdCurrent.load(); bool ready=remoteMp3Name(track,name,sizeof(name));
    RemoteJsonWriter out(req);
    out.text("{\"station\":");out.number(id);out.text(",\"name\":");out.json(id>=0?ALL_STATIONS[id].name:"");
    out.text(",\"status\":");out.json(status);out.text(",\"requested\":");out.boolean(wantPlayback.load());
    out.text(",\"volume\":");out.number(currentVolume.load());
    if(full) {
        out.text(",\"stations\":[");
        for(int i=0;i<ALL_STATIONS_COUNT;++i){if(i)out.ch(',');out.text("{\"id\":");out.number(i);out.text(",\"name\":");out.json(ALL_STATIONS[i].name);out.ch('}');}
        out.ch(']');
    }
    out.text(",\"favorites\":[");for(int i=0;i<FAVORITES_COUNT;++i){if(i)out.ch(',');out.number(favoriteIds[i].load());}
    uint32_t deadline=radioDeadline.load();int32_t left=deadline?int32_t(deadline-millis()):0;
    out.text("],\"brightness\":");out.number(currentBrightness.load());out.text(",\"sleep\":");out.number(sleepChoice.load());
    out.text(",\"remaining\":");out.number(left>0?(left+999)/1000:0);
    out.text(",\"mp3\":{\"scan\":");out.number(sdScanState.load());out.text(",\"count\":");out.number(ready?sdFileCount.load():0);
    out.text(",\"current\":");out.number(track);out.text(",\"name\":");out.json(name);
    out.text(",\"active\":");out.boolean(sdActive.load());out.text(",\"paused\":");out.boolean(sdPaused.load());
    out.text(",\"elapsed\":");out.number(sdElapsed.load());out.text(",\"duration\":");out.number(sdDuration.load());out.text("}}");return out.finish();
}
static esp_err_t remoteControl(httpd_req_t* req) {
    // Требую собственный заголовок браузера; CORS не разрешаю. Чужая страница
    // не может отправить такой POST без предварительного разрешения сервера.
    char marker[8];
    if (httpd_req_get_hdr_value_str(req, "X-Radio-Control", marker, sizeof(marker)) != ESP_OK || strcmp(marker,"1"))
        return httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "Forbidden");
    if (req->content_len <= 0 || req->content_len >= 96)
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid body");
    char contentType[80];
    if(httpd_req_get_hdr_value_str(req,"Content-Type",contentType,sizeof(contentType))!=ESP_OK ||
       strncasecmp(contentType,"application/x-www-form-urlencoded",33) ||
       (contentType[33] && contentType[33]!=';' && contentType[33]!=' '))
        return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Invalid Content-Type");
    char body[96] = {}, action[20] = {}, value[12] = {};
    int received = 0;
    while (received < req->content_len) {
        int n = httpd_req_recv(req, body + received, req->content_len - received);
        if (n <= 0) return httpd_resp_send_err(req, HTTPD_408_REQ_TIMEOUT, "Timeout");
        received += n;
    }
    if (httpd_query_key_value(body,"action",action,sizeof(action)) != ESP_OK)
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing action");
    if(!strncmp(action,"sd",2)) {
        if(!strcmp(action,"sdscan")) {sdWebScanRequested.store(true);return httpd_resp_send(req,"OK",2);}
        if(sdScanState.load()!=2 || sdFileCount.load()==0) return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"SD not ready");
        int index=sdCurrent.load();
        if(!strcmp(action,"sdfile") || !strcmp(action,"sdseek")) {
            if(httpd_query_key_value(body,"value",value,sizeof(value))!=ESP_OK) return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Missing value");
            char* end; long n=strtol(value,&end,10);
            long maximum=!strcmp(action,"sdfile")?long(sdFileCount.load())-1:long(std::min(65535u,sdDuration.load()?sdDuration.load()-1:0u));
            if(!value[0]||*end||n<0||n>maximum) return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Invalid value");
            if(!strcmp(action,"sdseek")) {
                if(!sdActive.load()||!sdDuration.load())return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Cannot seek");
                sdSeekTrack.store(index);sdSeekSeconds.store(n);
            }else{sdRequested.store(n);sdAction.store(1);}
        }else if(!strcmp(action,"sdpause")){if(sdActive.load()&&!sdPaused.load())sdAction.store(2);}
        else if(!strcmp(action,"sdstop"))sdAction.store(3);
        else if(!strcmp(action,"sdplay")){if(!sdActive.load()||sdPaused.load()){sdRequested.store(index<0?0:index);sdAction.store(1);}}
        else if(!strcmp(action,"sdprev")||!strcmp(action,"sdnext")) {
            index=index<0?0:index+(!strcmp(action,"sdprev")?-1:1);
            if(index>=0&&index<sdFileCount.load()){sdRequested.store(index);sdAction.store(1);}
        }else return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Unknown action");
        return httpd_resp_send(req,"OK",2);
    }
    // Настройки и избранное передаю GUI-задаче; здесь не трогаю LVGL и Preferences.
    int kind = !strcmp(action,"brightness") ? 0 : !strcmp(action,"sleep") ? 1 :
        !strcmp(action,"timer") ? 2 : !strcmp(action,"favadd") ? 3 : !strcmp(action,"favremove") ? 4 : -1;
    if(kind >= 0) {
        if(httpd_query_key_value(body,"value",value,sizeof(value)) != ESP_OK)
            return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Missing value");
        char* end; long n=strtol(value,&end,10);
        int minimum=kind==0 ? 15 : 0;
        int maximum=kind==0 ? 100 : kind==1 ? 6 : kind==2 ? 4 : ALL_STATIONS_COUNT-1;
        if(!value[0] || *end || n<minimum || n>maximum)
            return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Invalid value");
        if(kind==3) {
            bool exists=false, empty=false;
            for(int i=0;i<FAVORITES_COUNT;i++){int id=favoriteIds[i].load();exists|=id==n;empty|=id<0;}
            if(!exists && !empty) {
                httpd_resp_set_status(req,"409 Conflict");
                return httpd_resp_send(req,"В избранном уже 20 станций",HTTPD_RESP_USE_STRLEN);
            }
        }
        RemoteSetting setting{kind,int(n)};
        if(!remoteSettingsQueue || xQueueSend(remoteSettingsQueue,&setting,0)!=pdTRUE) {
            httpd_resp_set_status(req,"503 Service Unavailable");
            return httpd_resp_send(req,"Очередь занята, повторите команду",HTTPD_RESP_USE_STRLEN);
        }
        return httpd_resp_send(req,"OK",2);
    }
    Command c{CMD_STOP,0};
    if (!strcmp(action,"volume") || !strcmp(action,"play")) {
        if (httpd_query_key_value(body,"value",value,sizeof(value)) != ESP_OK)
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing value");
        char* end; long n = strtol(value,&end,10);
        int maximum = !strcmp(action,"volume") ? 21 : ALL_STATIONS_COUNT-1;
        if (!value[0] || *end || n<0 || n>maximum)
            return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid value");
        if (!strcmp(action,"volume")) {
            currentVolume.store(n);
            return httpd_resp_send(req,"OK",2);
        }
        c = {CMD_PLAY_ALL,uint8_t(n)};
    } else if (!strcmp(action,"toggle")) c = {CMD_TOGGLE,0};
    else if (strcmp(action,"stop")) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Unknown action");
    if (xQueueSend(audioCmdQueue,&c,0) != pdTRUE) {
        httpd_resp_set_status(req, "503 Service Unavailable");
        return httpd_resp_send(req, "Command queue full", HTTPD_RESP_USE_STRLEN);
    }
    return httpd_resp_send(req,"OK",2);
}
// HTTP owns the editable disk snapshot. Live catalogue stays immutable until reboot.
static esp_err_t remoteCatalog(httpd_req_t* req) {
    static JsonDocument editorDisk;
    static bool initialized=false;
    if(!initialized){editorDisk.set(stationDisk);initialized=true;}
    if(req->method==HTTP_GET) {
        RemoteJsonWriter out(req);out.text("{\"writable\":");out.boolean(stationCatalogWritable);
        out.text(",\"message\":");out.json(stationCatalogMessage);out.text(",\"catalog\":");
        String json;serializeJson(editorDisk,json);out.text(json.c_str());out.ch('}');return out.finish();
    }
    char marker[8];
    if(httpd_req_get_hdr_value_str(req,"X-Radio-Control",marker,sizeof(marker))!=ESP_OK || strcmp(marker,"1"))return httpd_resp_send_err(req,HTTPD_403_FORBIDDEN,"Forbidden");
    if(!stationCatalogWritable)return httpd_resp_send_err(req,HTTPD_403_FORBIDDEN,"SD catalogue is read-only");
    if(req->content_len<1 || req->content_len>196608)return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Invalid size");
    String body; if(!body.reserve(req->content_len+1))return httpd_resp_send_err(req,HTTPD_500_INTERNAL_SERVER_ERROR,"No memory");
    char chunk[512];int left=req->content_len;
    while(left){int n=httpd_req_recv(req,chunk,std::min(left,int(sizeof(chunk))));if(n<=0)return httpd_resp_send_err(req,HTTPD_408_REQ_TIMEOUT,"Timeout");body.concat(chunk,n);left-=n;}
    JsonDocument next;
    if(deserializeJson(next,body) || !stationValidate(next))return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Invalid catalogue");
    // aliases are derived on the device, not trusted from a browser.
    JsonArray a=next["stations"].as<JsonArray>();
    for(JsonObject o:a) {
        String url=o["url"].as<String>();
        for(JsonObject old:editorDisk["stations"].as<JsonArray>()) {
            String previous=old["url"].as<String>();bool match=url==previous;
            for(JsonVariant v:o["aliases"].as<JsonArray>())if(previous==v.as<String>())match=true;
            if(match){JsonArray aliases=o["aliases"].to<JsonArray>();for(JsonVariant v:old["aliases"].as<JsonArray>())aliases.add(v.as<String>());if(url!=previous)aliases.add(previous);break;}
        }
    }
    if(!stationValidate(next))return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Too many previous addresses");
    if(sdActive.load() || sdScanState.load()==1){httpd_resp_set_status(req,"409 Conflict");return httpd_resp_send(req,"Остановите MP3 и дождитесь завершения поиска на SD",HTTPD_RESP_USE_STRLEN);}
    if(xSemaphoreTake(sdFilesMutex,pdMS_TO_TICKS(100))!=pdTRUE){httpd_resp_set_status(req,"503 Service Unavailable");return httpd_resp_send(req,"SD busy",HTTPD_RESP_USE_STRLEN);}
    bool ok=stationWrite(next);xSemaphoreGive(sdFilesMutex);
    if(!ok)return httpd_resp_send_err(req,HTTPD_500_INTERNAL_SERVER_ERROR,"SD write failed; previous catalogue preserved");
    editorDisk.set(next);
    // Keep boot-time aliases unchanged: audio task still uses them for settings.
    return httpd_resp_send(req,"Saved; restart radio to apply",HTTPD_RESP_USE_STRLEN);
}
static void startWebRemote() {
    remoteSettingsQueue = xQueueCreate(8,sizeof(RemoteSetting));
    if(!remoteSettingsQueue){pushStatus("Нет памяти для веб-пульта");return;}
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 8080;
    config.task_priority = 1;
    config.core_id = 1;
    config.stack_size = 8192;
    config.max_open_sockets = 3;
    config.lru_purge_enable = true;
    config.recv_wait_timeout = 2;
    config.send_wait_timeout = 2;
    static httpd_handle_t server = nullptr;
    if (httpd_start(&server,&config) != ESP_OK) { pushStatus("Веб-пульт не запущен"); return; }
    httpd_uri_t root = {}; root.uri="/"; root.method=HTTP_GET; root.handler=remoteRoot;
    httpd_uri_t state = {}; state.uri="/api/state"; state.method=HTTP_GET; state.handler=remoteState;
    httpd_uri_t control = {}; control.uri="/api/control"; control.method=HTTP_POST; control.handler=remoteControl;
    httpd_uri_t mp3 = {}; mp3.uri="/api/mp3"; mp3.method=HTTP_GET; mp3.handler=remoteMp3Files;
    httpd_uri_t cat={};cat.uri="/api/catalog";cat.method=HTTP_GET;cat.handler=remoteCatalog;
    httpd_register_uri_handler(server,&cat);cat.method=HTTP_POST;httpd_register_uri_handler(server,&cat);
    httpd_register_uri_handler(server,&mp3);
    httpd_register_uri_handler(server,&root);
    httpd_register_uri_handler(server,&state);
    httpd_register_uri_handler(server,&control);
}
