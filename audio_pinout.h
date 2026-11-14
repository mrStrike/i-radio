// Подбираю совместимую сигнатуру setPinout на этапе компиляции.
// Сохраняю существующий вызов configureAudioPins(audio, 0): подходящая перегрузка
// выбирается через decltype, без дополнительных попыток настройки в ходе работы.
// Выводы BCLK, LRCK, DOUT и MCLK определены перед подключением этого заголовка.
#pragma once
// Support both Audio APIs: newer (4 pins) and older (5 pins).
template<typename T>
static auto configureAudioPins(T& audio, long)
-> decltype(audio.setPinout(BCLK, LRCK, DOUT, MCLK)) {
    return audio.setPinout(BCLK, LRCK, DOUT, MCLK);
}
template<typename T>
static auto configureAudioPins(T& audio, int)
-> decltype(audio.setPinout(BCLK, LRCK, DOUT, -1, MCLK)) {
    return audio.setPinout(BCLK, LRCK, DOUT, -1, MCLK);
}
