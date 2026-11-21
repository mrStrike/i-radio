#pragma once

// Подключать после объявления переменных спектра.
// Шаблон хранится в заголовке, чтобы обработчик .ino не создавал
// для него прототип без template<typename Sample>.
// Ограниченный Goertzel-анализ 256 PCM-кадров: 8 частотных полос раз в 100 мс.
// Никаких выделений памяти, блокировок, I2S-вызовов или изменения сэмплов.
template<typename Sample>
static void analyzeSpectrum(const Sample* pcm, uint16_t frames, int channels, float scale) {
    static uint32_t lastAnalysis = 0;
    const uint32_t now = millis();
    if (!pcm || !frames || now - lastAnalysis < 100) return;
    const uint32_t rate = spectrumSampleRate.load();
    if ((channels != 1 && channels != 2) || rate < 16000) return;
    static float window[256];
    static bool windowReady = false;
    if (!windowReady) {
        for (int n = 0; n < 256; ++n)
            window[n] = 0.5f - 0.5f * cosf(2.0f * PI * n / 255.0f);
        windowReady = true;
    }
    // В 4.0.1 блок может содержать меньше 256 кадров: собираем окно
    // из последовательных блоков, не читая за границами буфера.
    static float samples[256];
    static uint16_t collected = 0;
    for (uint16_t n = 0; n < frames && collected < 256; ++n) {
        const int idx = n * channels;
        const float sample = channels == 2
            ? (float(pcm[idx]) + float(pcm[idx + 1])) * 0.5f : float(pcm[idx]);
        samples[collected] = sample * scale * window[collected];
        ++collected;
    }
    if (collected < 256) return;
    collected = 0;
    lastAnalysis = now;
    // Представительные частоты восьми укрупнённых полос.
    // Это визуализатор, а не изменение звучания радио.
    static constexpr float frequencies[SPECTRUM_BANDS] = {
        220, 440, 850, 1600, 2700, 4200, 6200, 8500
    };
    static float coefficients[SPECTRUM_BANDS];
    static uint32_t coefficientRate = 0;
    if (coefficientRate != rate) {
        for (int band = 0; band < SPECTRUM_BANDS; ++band)
            coefficients[band] = 2.0f * cosf(2.0f * PI * frequencies[band] / rate);
        coefficientRate = rate;
    }
    for (int band = 0; band < SPECTRUM_BANDS; ++band) {
        float level = 0;
        if (frequencies[band] < rate * 0.45f) {
            const float coeff = coefficients[band];
            float q1 = 0, q2 = 0;
            for (int n = 0; n < 256; ++n) {
                const float q = samples[n] + coeff * q1 - q2;
                q2 = q1;
                q1 = q;
            }
            const float power = fmaxf(0.0f, q1*q1 + q2*q2 - coeff*q1*q2);
            // 20*log10(sqrt(power)/64) == 10*log10(power/4096).
            // Та же шкала, но без sqrtf в аудиообработчике.
            const float normalizedPower = power * (1.0f / 4096.0f);
            if (normalizedPower > 0.000001f)
                level = constrain((10.0f * log10f(normalizedPower) + 60.0f) * (40.0f / 60.0f), 0.0f, 40.0f);
        }
        spectrumLevels[band].store(uint8_t(level));
    }
    spectrumAt.store(now);
}
