// Список по алфавиту: цифры, латиница, кириллица; без учёта регистра.
// В NVS станции сохраняются по URL, порядок списка не влияет на избранное.
#pragma once
struct RadioStation {
    const char* name;
    const char* url;
};
const RadioStation BUILTIN_STATIONS[] = {
    {"25 Регион", "https://aska.ru-hoster.com:8024/25Region_live"},
    {"Business FM", "https://bfm.hostingradio.ru:8004/bfm32.aacp"},
    {"Comedy Radio", "https://gpm.hostingradio.ru/gpm-comedyradio495.aacp"},
    {"Play FM", "https://radio.play-fm.ru:8001/playfm"},
    {"ROCK FM", "https://nashe1.hostingradio.ru/rock32.aacp"},
    {"Авторадио", "https://gpm.hostingradio.ru/gpm-avtoradio495.aacp"},
    {"ВАРНА РАДИО", "https://air.varnaradio.ru:8443/region"},
    {"Вести FM", "https://icecast-vgtrk.cdnvideo.ru/vestifm_mp3_192kbps"},
    {"Восток FM", "https://microit.n340.com:8443/VgMv0WV17ZVx1uuo_18_vostok_64_reg_1"},
    {"Дорожное Радио", "https://dorognoe.hostingradio.ru/dorognoe32.aacp"},
    {"Европа Плюс", "https://epdop.hostingradio.ru:8033/europaplus32.aacp"},
    {"Жара FM", "https://zharafm.hostingradio.ru/zharafm32.aacp"},
    {"Железо ФМ", "https://online.fefm.ru:8443/fefm"},
    {"Импульс FM", "https://impulsfm.ru/impuls"},
    {"Казак FM", "https://radio.kazak.fm/kazak32.aacp"},
    {"Калина Красная", "https://stream.kalina.fm/kalina32.aacp"},
    {"Красная Армия", "https://live.rzs.ru/ka.128.mp3"},
    {"МАКСИМУМ", "https://maximum.hostingradio.ru/maximum32.aacp"},
    {"Мария FM", "https://online.mariafm.ru:8443/MariaFM"},
    {"Маруся FM", "https://cdn.radio-holding.ru/marusya_default"},
    {"Маяк", "https://icecast-vgtrk.cdnvideo.ru/mayakfm_mp3_128kbps"},
    {"Модель для сборки", "http://mds-station.com:8000/mds"},
    {"Москва FM", "https://icecast-vgtrk.cdnvideo.ru/moscowfm128"},
    {"Народное Радио", "https://rodniki.hostingradio.ru/rodniki32.aacp"},
    {"НАШЕ Радио", "https://nashe1.hostingradio.ru/nashe32.aacp"},
    {"Новое Радио", "https://stream.newradio.ru/novoe32.aacp"},
    {"Первое Сетевое", "https://streamer.radiovseti.ru:8443/192"},
    {"Песни от всей души", "https://hls.radiopovd.ru/povd/playlist.m3u8"},
    {"Пульс-Радио", "https://puls-online.ru:8443/pulson_192"},
    {"Радио ENERGY", "https://gpm.hostingradio.ru/gpm-energyfm495.aacp"},
    {"Радио JAZZ", "https://nashe1.hostingradio.ru/jazz32.aacp"},
    {"Радио Record", "https://radiorecord.hostingradio.ru/rr_main32.aacp"},
    {"Радио Адам", "https://radioadam.hostingradio.ru/radioadam32.aacp"},
    {"Радио Дача", "https://microit2.n340.ru:8443/VgMv0WV17ZVx1uuo_14_dacha_64"},
    {"Радио Звезда", "https://zvezda-radio0.mediacdn.ru/radio/zvezda/Zvezda_REG_0"},
    {"Радио КП", "https://kpradio.hostingradio.ru:8000/radiokp32.aacp"},
    {"Радио Культура", "https://icecast-vgtrk.cdnvideo.ru/kulturafm_mp3_192kbps"},
    {"Радио Мария", "https://hls-01-radiomaria.hostingradio.ru/radiomaria/playlist.m3u8"},
    {"Радио Мир", "https://icecast-mirtv.cdnvideo.ru/radio_mir_256"},
    {"Радио Москвы", "https://icecast-vgtrk.cdnvideo.ru/moscowtalk128"},
    {"Радио Прибой", "https://online.priboyfm.ru/"},
    {"Радио Рандеву", "https://stream.randevuradio.ru/randevu"},
    {"Радио РБК", "https://rbcreg.hostingradio.ru/rbc32.aacp"},
    {"Радио Республика", "https://radio.gtrklnr.ru:8000/republic.fm"},
    {"Радио Родных Дорог", "https://stream1.radiord.ru:8000/s0_64"},
    {"Радио России", "https://icecast-vgtrk.cdnvideo.ru/rrzonam_mp3_192kbps"},
    {"Радио Русь", "https://radio-holding.ru/rus"},
    {"Радио Хит", "https://hit.trkeurasia.ru/hit128"},
    {"Радио-Ноябрьск", "https://radionoyabrsk.ru:8443/"},
    {"Ретро FM", "https://retro.hostingradio.ru:8043/retro32.aacp"},
    {"Родина FM", "https://cast.modnoeradio.ru/live"},
    {"Русская Волна", "https://irp.volna.top/ruwave.aacp"},
    {"Русский Хит", "https://microit.n340.com:8443/VgMv0WV17ZVx1uuo_19_ruhit_64_reg_1"},
    {"Русское Радио", "https://rusradio.hostingradio.ru/rusradio32.aacp"},
    {"Серебряный Дождь", "https://silverrain.hostingradio.ru/silver32.aacp"},
    {"Сигма", "https://radio.sigma-ktv.ru:8443/sigma"},
    {"Соловьев Live", "https://solovievfm.hostingradio.ru/solovievfm128.aacp"},
    {"Спутник FM", "https://radio.mediacdn.ru/sputnik_fm.mp3"},
    {"Такси FM", "https://microit.n340.com:8443/VgMv0WV17ZVx1uuo_13_taxi_64"},
    {"Топ Радио", "https://streamer.radiovseti.ru:8443/tr_hi"},
    {"Умное Радио", "https://radio.sarov.info/radio/8010/clever-radio/"},
    {"Хит FM", "https://hitfm.hostingradio.ru/hitfm96.aacp"},
    {"Юмор FM", "https://gpm.hostingradio.ru/gpm-humorfm495.aacp"},
};
constexpr int BUILTIN_STATIONS_COUNT = sizeof(BUILTIN_STATIONS) / sizeof(BUILTIN_STATIONS[0]);
constexpr int FAV_0 = 7; // Вести FM
constexpr int FAV_1 = 20; // Маяк
constexpr int FAV_2 = 21; // Модель для сборки
constexpr int FAV_3 = 56; // Соловьев Live
const RadioStation FAVORITES[] = {
    BUILTIN_STATIONS[FAV_0], BUILTIN_STATIONS[FAV_1],
    BUILTIN_STATIONS[FAV_2], BUILTIN_STATIONS[FAV_3],
};
constexpr int FAVORITES_COUNT = 20;
// Версия формата хранения, не версия списка. Не повышать при добавлении станций.
constexpr int STATION_STORAGE_VERSION = 2;

constexpr int MAX_STATIONS = 150;
static RadioStation ALL_STATIONS[MAX_STATIONS];
static int ALL_STATIONS_COUNT = BUILTIN_STATIONS_COUNT;
