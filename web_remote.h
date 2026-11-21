#pragma once
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
async function refresh(){if(busy)return;busy=true;try{const r=await fetch('/api/state',{cache:'no-store',signal:AbortSignal.timeout(4000)});if(!r.ok)throw Error('HTTP '+r.status);const s=await r.json();remoteStateCache=s;$('station').textContent=s.name||'Станция не выбрана';$('status').textContent=s.status;$('toggle').textContent=s.requested?'Пауза':'Продолжить';$('toggle').disabled=s.station<0;if(!editing){$('volume').value=s.volume;$('volumeText').textContent=Math.round(s.volume*100/21)+'%'}if(!loaded){for(const x of s.stations){const o=document.createElement('option');o.value=x.id;o.textContent=x.name;$('stations').append(o)}loaded=true;if(s.station>=0)$('stations').value=s.station}if(!brightEditing){$('brightness').value=s.brightness;$('brightnessText').textContent=s.brightness+'%'}
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
refresh();setInterval(refresh,2000);
</script></body></html>)HTML";

// Экранирую строки JSON, чтобы названия из моего списка не ломали ответ.
static String remoteJson(const char* text) {
    String out = "\"";
    for (const unsigned char* p = (const unsigned char*)text; *p; ++p) {
        if (*p == '"' || *p == '\\') { out += '\\'; out += char(*p); }
        else if (*p < 32) { char b[7]; snprintf(b, sizeof(b), "\\u%04x", *p); out += b; }
        else out += char(*p);
    }
    return out + '"';
}
static esp_err_t remoteRoot(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    return httpd_resp_send(req, remotePage, sizeof(remotePage)-1);
}
static String remoteMp3Name(int index) {
    if(sdScanState.load()!=2 || index<0 || index>=int(sdMp3Files.size())) return "";
    const String& path=sdMp3Files[index]; return path.substring(path.lastIndexOf('/')+1);
}
static esp_err_t remoteMp3Files(httpd_req_t* req) {
    char query[32]={}, value[12]={}; int page=0;
    if(httpd_req_get_url_query_str(req,query,sizeof(query))==ESP_OK && httpd_query_key_value(query,"page",value,sizeof(value))==ESP_OK) {
        char* end; long n=strtol(value,&end,10);
        if(!value[0]||*end||n<0||n>66) return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Invalid page");
        page=int(n);
    }
    String body="{\"files\":[";
    if(sdScanState.load()==2) {
        int end=std::min((page+1)*30,int(sdMp3Files.size()));
        for(int i=page*30;i<end;i++) {if(i!=page*30)body+=',';body+="{\"id\":"+String(i)+",\"name\":"+remoteJson(remoteMp3Name(i).c_str())+'}';}
    }
    body+="]}"; httpd_resp_set_type(req,"application/json; charset=utf-8");
    httpd_resp_set_hdr(req,"Cache-Control","no-store"); return httpd_resp_send(req,body.c_str(),body.length());
}
static esp_err_t remoteState(httpd_req_t* req) {
    char status[96];
    portENTER_CRITICAL(&webStatusMux);
    strlcpy(status, webStatus, sizeof(status));
    portEXIT_CRITICAL(&webStatusMux);
    int id = currentStation;
    if (id >= 100) id -= 100;
    else if (id >= 0 && id < FAVORITES_COUNT) id = favoriteIds[id].load();
    if (id < 0 || id >= ALL_STATIONS_COUNT) id = -1;
    String body; body.reserve(8192);
    body = "{\"station\":" + String(id) + ",\"name\":";
    body += remoteJson(id >= 0 ? ALL_STATIONS[id].name : "");
    body += ",\"status\":" + remoteJson(status);
    body += ",\"requested\":"; body += wantPlayback.load() ? "true" : "false";
    body += ",\"volume\":" + String(currentVolume.load()) + ",\"stations\":[";
    for (int i=0; i<ALL_STATIONS_COUNT; ++i) {
        if(i) body += ',';
        body += "{\"id\":" + String(i) + ",\"name\":" + remoteJson(ALL_STATIONS[i].name) + '}';
    }
    body += "],\"favorites\":[";
    for(int i=0;i<FAVORITES_COUNT;i++){if(i)body+=',';body+=String(favoriteIds[i].load());}
    uint32_t deadline=radioDeadline.load();
    int32_t left=deadline ? int32_t(deadline-millis()) : 0;
    body += "],\"brightness\":" + String(currentBrightness.load());
    body += ",\"sleep\":" + String(sleepChoice.load());
    body += ",\"remaining\":" + String(left>0 ? (left+999)/1000 : 0) + "}";
    body.remove(body.length()-1);
    body+=",\"mp3\":{\"scan\":"+String(sdScanState.load())+",\"count\":"+String(sdScanState.load()==2?sdMp3Files.size():0);
    body+=",\"current\":"+String(sdCurrent.load())+",\"name\":"+remoteJson(remoteMp3Name(sdCurrent.load()).c_str());
    body+=",\"active\":"; body+=sdActive.load()?"true":"false";
    body+=",\"paused\":"; body+=sdPaused.load()?"true":"false";
    body+=",\"elapsed\":"+String(sdElapsed.load())+",\"duration\":"+String(sdDuration.load())+"}}";
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, body.c_str(), body.length());
}
static esp_err_t remoteControl(httpd_req_t* req) {
    // Требую собственный заголовок браузера; CORS не разрешаю. Чужая страница
    // не может отправить такой POST без предварительного разрешения сервера.
    char marker[8];
    if (httpd_req_get_hdr_value_str(req, "X-Radio-Control", marker, sizeof(marker)) != ESP_OK || strcmp(marker,"1"))
        return httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "Forbidden");
    if (req->content_len <= 0 || req->content_len >= 96)
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid body");
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
        if(sdScanState.load()!=2 || sdMp3Files.empty()) return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"SD not ready");
        int index=sdCurrent.load();
        if(!strcmp(action,"sdfile") || !strcmp(action,"sdseek")) {
            if(httpd_query_key_value(body,"value",value,sizeof(value))!=ESP_OK) return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Missing value");
            char* end; long n=strtol(value,&end,10);
            long maximum=!strcmp(action,"sdfile")?long(sdMp3Files.size())-1:long(std::min(65535u,sdDuration.load()?sdDuration.load()-1:0u));
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
            if(index>=0&&index<int(sdMp3Files.size())){sdRequested.store(index);sdAction.store(1);}
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
    httpd_handle_t server = nullptr;
    if (httpd_start(&server,&config) != ESP_OK) { pushStatus("Веб-пульт не запущен"); return; }
    httpd_uri_t root = {}; root.uri="/"; root.method=HTTP_GET; root.handler=remoteRoot;
    httpd_uri_t state = {}; state.uri="/api/state"; state.method=HTTP_GET; state.handler=remoteState;
    httpd_uri_t control = {}; control.uri="/api/control"; control.method=HTTP_POST; control.handler=remoteControl;
    httpd_uri_t mp3 = {}; mp3.uri="/api/mp3"; mp3.method=HTTP_GET; mp3.handler=remoteMp3Files;
    httpd_register_uri_handler(server,&mp3);
    httpd_register_uri_handler(server,&root);
    httpd_register_uri_handler(server,&state);
    httpd_register_uri_handler(server,&control);
}
