// Здесь храню названия и адреса потоков моего радио. URL и порядок записей сохраняю.
// name — подпись для интерфейса, url — адрес для Audio::connecttohost.
// Число станций вычисляю по размеру массива, а не задаю вручную.
// FAV_0–FAV_3 задают исходное избранное при отсутствии сохранённых настроек.
// LEGACY_STATION_IDS переводит старый индекс в новый; -1 означает удалённую станцию.
// Версия списка позволяет выполнить перенос сохранённых индексов однократно.
// stations.h — действующий список интернет-радиостанций.
// Порядок: цифры, латиница, кириллица; без учёта регистра.
#pragma once
struct RadioStation {
    const char* name;
    const char* url;
};
const RadioStation ALL_STATIONS[] = {
    {
        "25 Регион", "https://aska.ru-hoster.com:8024/25Region_live"
    },
    {
        "Business FM", "https://bfm.hostingradio.ru:8004/bfm32.aacp"
    },
    {
        "Comedy Radio", "https://gpm.hostingradio.ru/gpm-comedyradio495.aacp"
    },
    {
        "Play FM", "https://radio.play-fm.ru:8001/playfm"
    },
    {
        "ROCK FM", "https://nashe1.hostingradio.ru/rock32.aacp"
    },
    {
        "Авторадио", "https://gpm.hostingradio.ru/gpm-avtoradio495.aacp"
    },
    {
        "ВАРНА РАДИО", "https://air.varnaradio.ru:8443/region"
    },
    {
        "Вести FM", "https://icecast-vgtrk.cdnvideo.ru/vestifm_mp3_192kbps"
    },
    {
        "Восток FM", "https://microit.n340.com:8443/VgMv0WV17ZVx1uuo_18_vostok_64_reg_1"
    },
    {
        "Дорожное Радио", "https://dorognoe.hostingradio.ru/dorognoe32.aacp"
    },
    {
        "Европа Плюс", "https://epdop.hostingradio.ru:8033/europaplus32.aacp"
    },
    {
        "Жара FM", "https://zharafm.hostingradio.ru/zharafm32.aacp"
    },
    {
        "Железо ФМ", "https://online.fefm.ru:8443/fefm"
    },
    {
        "Импульс FM", "https://impulsfm.ru/impuls"
    },
    {
        "Казак FM", "https://radio.kazak.fm/kazak32.aacp"
    },
    {
        "Калина Красная", "https://stream.kalina.fm/kalina32.aacp"
    },
    {
        "Красная Армия", "https://live.rzs.ru/ka.128.mp3"
    },
    {
        "МАКСИМУМ", "https://maximum.hostingradio.ru/maximum32.aacp"
    },
    {
        "Мария FM", "https://online.mariafm.ru:8443/MariaFM"
    },
    {
        "Маруся FM", "https://cdn.radio-holding.ru/marusya_default"
    },
    {
        "Маяк", "https://icecast-vgtrk.cdnvideo.ru/mayakfm_mp3_128kbps"
    },
    {
        "Москва FM", "https://icecast-vgtrk.cdnvideo.ru/moscowfm128"
    },
    {
        "Народное Радио", "https://rodniki.hostingradio.ru/rodniki32.aacp"
    },
    {
        "НАШЕ Радио", "https://nashe1.hostingradio.ru/nashe32.aacp"
    },
    {
        "Новое Радио", "https://stream.newradio.ru/novoe32.aacp"
    },
    {
        "Первое Сетевое", "https://streamer.radiovseti.ru:8443/192"
    },
    {
        "Песни от всей души", "https://hls.radiopovd.ru/povd/playlist.m3u8"
    },
    {
        "Пульс-Радио", "https://puls-online.ru:8443/pulson_192"
    },
    {
        "Радио ENERGY", "https://gpm.hostingradio.ru/gpm-energyfm495.aacp"
    },
    {
        "Радио JAZZ", "https://nashe1.hostingradio.ru/jazz32.aacp"
    },
    {
        "Радио Record", "https://radiorecord.hostingradio.ru/rr_main32.aacp"
    },
    {
        "Радио Адам", "https://radioadam.hostingradio.ru/radioadam32.aacp"
    },
    {
        "Радио Дача", "https://microit2.n340.ru:8443/VgMv0WV17ZVx1uuo_14_dacha_64"
    },
    {
        "Радио Звезда", "https://zvezda-radio0.mediacdn.ru/radio/zvezda/Zvezda_REG_0"
    },
    {
        "Радио КП", "https://kpradio.hostingradio.ru:8000/radiokp32.aacp"
    },
    {
        "Радио Культура", "https://icecast-vgtrk.cdnvideo.ru/kulturafm_mp3_192kbps"
    },
    {
        "Радио Мария", "https://hls-01-radiomaria.hostingradio.ru/radiomaria/playlist.m3u8"
    },
    {
        "Радио Мир", "https://icecast-mirtv.cdnvideo.ru/radio_mir_256"
    },
    {
        "Радио Москвы", "https://icecast-vgtrk.cdnvideo.ru/moscowtalk128"
    },
    {
        "Радио Прибой", "https://online.priboyfm.ru/"
    },
    {
        "Радио Рандеву", "https://stream.randevuradio.ru/randevu"
    },
    {
        "Радио РБК", "https://rbcreg.hostingradio.ru/rbc32.aacp"
    },
    {
        "Радио Республика", "https://radio.gtrklnr.ru:8443/republic.fm"
    },
    {
        "Радио Родных Дорог", "https://stream1.radiord.ru:8000/s0_64"
    },
    {
        "Радио России", "https://icecast-vgtrk.cdnvideo.ru/rrzonam_mp3_192kbps"
    },
    {
        "Радио Русь", "https://radio-holding.ru/rus"
    },
    {
        "Радио Хит", "https://hit.trkeurasia.ru/hit128"
    },
    {
        "Радио-Ноябрьск", "https://radionoyabrsk.ru:8443/"
    },
    {
        "Ретро FM", "https://retro.hostingradio.ru:8043/retro32.aacp"
    },
    {
        "Родина FM", "https://cast.modnoeradio.ru/live"
    },
    {
        "Русская Волна", "https://irp.volna.top/ruwave.aacp"
    },
    {
        "Русский Хит", "https://microit.n340.com:8443/VgMv0WV17ZVx1uuo_19_ruhit_64_reg_1"
    },
    {
        "Русское Радио", "https://rusradio.hostingradio.ru/rusradio32.aacp"
    },
    {
        "Серебряный Дождь", "https://silverrain.hostingradio.ru/silver32.aacp"
    },
    {
        "Сигма", "https://radio.sigma-ktv.ru:8443/sigma"
    },
    {
        "Спутник FM", "https://radio.mediacdn.ru/sputnik_fm.mp3"
    },
    {
        "Такси FM", "https://microit.n340.com:8443/VgMv0WV17ZVx1uuo_13_taxi_64"
    },
    {
        "Топ Радио", "https://streamer.radiovseti.ru:8443/tr_hi"
    },
    {
        "Умное Радио", "https://radio.sarov.info/radio/8010/clever-radio/"
    },
    {
        "Хит FM", "https://hitfm.hostingradio.ru/hitfm96.aacp"
    },
    {
        "Юмор FM", "https://gpm.hostingradio.ru/gpm-humorfm495.aacp"
    },
};
constexpr int ALL_STATIONS_COUNT = sizeof(ALL_STATIONS) / sizeof(ALL_STATIONS[0]);
constexpr int FAV_0 = 10;
// Европа Плюс
constexpr int FAV_1 = 5;
// Авторадио
constexpr int FAV_2 = 59;
// Хит FM
constexpr int FAV_3 = 42;
// Радио Республика
const RadioStation FAVORITES[] = {
    ALL_STATIONS[FAV_0], ALL_STATIONS[FAV_1],
    ALL_STATIONS[FAV_2], ALL_STATIONS[FAV_3],
};
constexpr int FAVORITES_COUNT = sizeof(FAVORITES) / sizeof(FAVORITES[0]);
// Однократный перенос сохранённых индексов из прежнего списка 98 станций.
constexpr int STATIONS_LIST_VERSION = 1;
constexpr int LEGACY_STATION_IDS[] = {
    10, 52, 5, 48, 9, 24, 59, -1, 17, 23, 4, 29, 30, 28, -1, 32, 56, 8, 51, 60, 2, -1, -1, -1, -1, -1, -1, 53, 1, 34, -1, 11, -1, 37, 33, 20, 7, 44, 35, 21, 38, -1, -1, -1, -1, 22, -1, -1, 55, 57, 25, 45, -1, 19, 18, 13, -1, 43, 54, 14, -1, -1, -1, -1, -1, -1, 46, -1, -1, -1, 16, 41, 3, -1, -1, -1, -1, 6, 49, 40, 47, -1, 58, 27, -1, 15, 39, -1, 0, 12, -1, -1, -1, 50, 31, 26, 36, 42
};
