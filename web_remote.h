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
#favorites button{min-height:112px;font-size:23px;font-weight:300;padding:18px;background:#0078d7}
#favorites button[data-slot="1"]{background:#17633b}#favorites button[data-slot="2"]{background:#c95110}#favorites button[data-slot="3"]{background:#555}
#favorites:empty:before{content:"Пока нет избранных станций";color:#999;grid-column:1/-1;padding:16px 0}
.minor button{font-size:15px;background:#202020}.settings{border-top:1px solid #333;padding-top:24px}.settings-grid{display:grid;grid-template-columns:1fr 1fr;gap:20px}label{display:block;color:#ccc}label select{margin-top:10px}#remaining{color:#bbb;margin-bottom:0}#error{color:#ff8585}#error:empty{display:none}footer{border-top:1px solid #333;color:#888;font-size:14px;margin-top:28px;padding-top:16px}
@media(max-width:520px){body{padding:20px 16px}h1{font-size:32px}#station{font-size:24px}.settings-grid{grid-template-columns:1fr}#favorites button{font-size:20px}.minor{flex-direction:column}}
</style></head><body>
<header><h1>Моё радио</h1><p class="subtitle">Домашний веб-пульт</p></header>
<main><section aria-label="Текущая станция"><div class="now"><p id="station">Загрузка…</p><p id="status" aria-live="polite"></p><div class="actions minor" style="margin-top:14px"><button id="currentFavorite" disabled>Выберите станцию</button></div></div>
<div class="actions"><button id="toggle" disabled>Пуск / Пауза</button><button id="stop">Остановить</button></div></section>
<section class="section"><h2>Громкость <span class="value" id="volumeText"></span></h2><input aria-label="Громкость" id="volume" type="range" min="0" max="21"></section>
<section class="section"><h2>Избранное <span id="favoriteCount" style="color:#aaa;font-size:18px">· 0 из 4</span></h2><div id="favorites"></div></section>
<section class="section"><h2>Станции</h2><select aria-label="Выбор станции" id="stations"></select><div class="actions"><button id="play">Слушать выбранную</button></div>
<p style="color:#aaa;margin:14px 0 8px">Выбранная станция</p><div class="actions minor"><button id="selectedFavorite" disabled>Выберите станцию</button></div></section>
<section class="section settings"><h2>Настройки</h2><label>Яркость <span class="value" id="brightnessText"></span><input id="brightness" type="range" min="15" max="100"></label>
<div class="settings-grid"><label>Сон экрана<select id="sleep"><option value="0">Выключен</option><option value="1">Через 2 минуты</option><option value="2">Через 5 минут</option><option value="3">Через 10 минут</option><option value="4">Через 15 минут</option><option value="5">Через 30 минут</option><option value="6">Через час</option></select></label>
<div><label>Остановка радио<select id="timer"><option value="0">Отменить таймер</option><option value="1">Через 15 минут</option><option value="2">Через 30 минут</option><option value="3">Через 60 минут</option><option value="4">Через 90 минут</option></select></label><button id="setTimer">Применить таймер</button><p id="remaining"></p></div></div></section>
<p id="error" role="alert"></p></main><footer>Пульт доступен в моей Wi-Fi-сети. Таймер радио действует только на текущий сеанс. Настройку подключения Wi-Fi запускаю на самом радио.</footer>
<script>
const $=id=>document.getElementById(id);let busy=false,editing=false,loaded=false,brightEditing=false,remoteStateCache=null;
// Для текущей и выбранной станции показываю только подходящее действие.
// При заполненных четырёх местах оставляю удаление доступным.
function updateFavoriteButtons(){
if(!remoteStateCache)return;
const s=remoteStateCache,count=s.favorites.filter(id=>id>=0).length;
$('favoriteCount').textContent='· '+count+' из 4';
for(const [button,id] of [[$('currentFavorite'),s.station],[$('selectedFavorite'),Number($('stations').value)]]){
const valid=Number.isInteger(id)&&s.stations.some(x=>x.id===id),exists=valid&&s.favorites.includes(id);
button.textContent=!valid?'Выберите станцию':exists?'− Удалить из избранного':count>=4?'Избранное заполнено — удалите одну станцию':'+ Добавить в избранное';
button.disabled=!valid||(!exists&&count>=4);
button.onclick=()=>command(exists?'favremove':'favadd',id);
}
}
async function command(action,value){try{const r=await fetch('/api/control',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded','X-Radio-Control':'1'},body:new URLSearchParams({action,value:String(value??'')})});if(!r.ok)throw Error(await r.text());$('error').textContent='';await refresh()}catch(e){$('error').textContent='Команда не принята: '+e.message}}
async function refresh(){if(busy)return;busy=true;try{const r=await fetch('/api/state',{cache:'no-store',signal:AbortSignal.timeout(4000)});if(!r.ok)throw Error('HTTP '+r.status);const s=await r.json();remoteStateCache=s;$('station').textContent=s.name||'Станция не выбрана';$('status').textContent=s.status;$('toggle').textContent=s.requested?'Пауза':'Продолжить';$('toggle').disabled=s.station<0;if(!editing){$('volume').value=s.volume;$('volumeText').textContent=Math.round(s.volume*100/21)+'%'}if(!loaded){for(const x of s.stations){const o=document.createElement('option');o.value=x.id;o.textContent=x.name;$('stations').append(o)}loaded=true;if(s.station>=0)$('stations').value=s.station}if(!brightEditing){$('brightness').value=s.brightness;$('brightnessText').textContent=s.brightness+'%'}
updateFavoriteButtons();
if(document.activeElement!==$('sleep'))$('sleep').value=s.sleep;
$('remaining').textContent=s.remaining?'До остановки радио: '+Math.ceil(s.remaining/60)+' мин':'Таймер радио выключен';
const signature=JSON.stringify(s.favorites);if($('favorites').dataset.signature!==signature){$('favorites').dataset.signature=signature;$('favorites').replaceChildren();for(const [slot,id] of s.favorites.entries()){if(id<0)continue;const button=document.createElement('button');button.dataset.slot=slot;button.textContent=s.stations[id].name;button.onclick=()=>command('play',id);$('favorites').append(button)}}
}catch(e){$('error').textContent='Нет связи с радио: '+e.message}finally{busy=false}}
$('toggle').onclick=()=>command('toggle');$('stop').onclick=()=>command('stop');$('play').onclick=()=>command('play',$('stations').value);
$('volume').oninput=()=>{editing=true;$('volumeText').textContent=Math.round($('volume').value*100/21)+'%'};
$('volume').onchange=()=>{editing=false;command('volume',$('volume').value)};
$('stations').onchange=updateFavoriteButtons;
$('brightness').oninput=()=>{brightEditing=true;$('brightnessText').textContent=$('brightness').value+'%'};
$('brightness').onchange=()=>{brightEditing=false;command('brightness',$('brightness').value)};
$('sleep').onchange=()=>command('sleep',$('sleep').value);$('setTimer').onclick=()=>command('timer',$('timer').value);
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
static esp_err_t remoteState(httpd_req_t* req) {
    char status[96];
    portENTER_CRITICAL(&webStatusMux);
    strlcpy(status, webStatus, sizeof(status));
    portEXIT_CRITICAL(&webStatusMux);
    int id = currentStation;
    if (id >= 100) id -= 100;
    else if (id >= 0 && id < 4) id = favoriteIds[id].load();
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
    for(int i=0;i<4;i++){if(i)body+=',';body+=String(favoriteIds[i].load());}
    uint32_t deadline=radioDeadline.load();
    int32_t left=deadline ? int32_t(deadline-millis()) : 0;
    body += "],\"brightness\":" + String(currentBrightness.load());
    body += ",\"sleep\":" + String(sleepChoice.load());
    body += ",\"remaining\":" + String(left>0 ? (left+999)/1000 : 0) + "}";
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
            for(int i=0;i<4;i++){int id=favoriteIds[i].load();exists|=id==n;empty|=id<0;}
            if(!exists && !empty) {
                httpd_resp_set_status(req,"409 Conflict");
                return httpd_resp_send(req,"В избранном уже четыре станции",HTTPD_RESP_USE_STRLEN);
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
    httpd_register_uri_handler(server,&root);
    httpd_register_uri_handler(server,&state);
    httpd_register_uri_handler(server,&control);
}
