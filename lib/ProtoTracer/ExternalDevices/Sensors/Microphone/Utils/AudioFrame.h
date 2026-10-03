#pragma once
#include <cmath>
#include <cstddef>

// ADC-count based visual levels; these are not calibrated acoustic SPL.
namespace AudioFrame {
inline float Unit(float value) {
    return !std::isfinite(value) || value < 0.0f ? 0.0f : value > 1.0f ? 1.0f : value;
}
inline float Intensity(float magnitude, float minDB, float maxDB) {
    if (!std::isfinite(magnitude) || magnitude <= 0.0f || maxDB <= minDB) return 0.0f;
    return Unit((20.0f * std::log10(magnitude) - minDB) / (maxDB - minDB));
}
// Compensate the Hann window's coherent gain; calculate it once at startup.
inline float Window(size_t index, size_t count) {
    if (count < 2) return 1.0f;
    return (1.0f - std::cos(6.28318530718f * float(index) / float(count - 1))) *
        float(count) / float(count - 1);
}
inline void BuildWindow(float* window, size_t count) {
    for (size_t i = 0; i < count; ++i) window[i] = Window(i, count);
}
inline float Prepare(const float* raw, float* complex, size_t count, const float* window = nullptr) {
    if (count == 0) return 0.0f;
    float mean = 0.0f, energy = 0.0f;
    for (size_t i = 0; i < count; ++i) mean += raw[i];
    mean /= float(count);
    for (size_t i = 0; i < count; ++i) {
        float centered = raw[i] - mean;
        energy += centered * centered;
        float hann = window ? window[i] : Window(i, count);
        complex[2 * i] = centered * hann;
        complex[2 * i + 1] = 0.0f;
    }
    return std::sqrt(energy / float(count)) / 4095.0f;
}
// Keep the FFT grid and tone scale while capturing a shorter real audio window.
// Zero padding interpolates the spectrum; it does not restore frequency resolution.
inline float PrepareSpectrum(const float* raw, float* complex, size_t count,
                             size_t fftSize, const float* window) {
    if (count > fftSize) count = fftSize;
    float rms = Prepare(raw, complex, count, window);
    float scale = count ? float(fftSize) / float(count) : 0.0f;
    for (size_t i = 0; i < count * 2; ++i) complex[i] *= scale;
    for (size_t i = count * 2; i < fftSize * 2; ++i) complex[i] = 0.0f;
    return rms;
}
inline float Follow(float current, float target, float attack, float release) {
    current = Unit(current);
    target = Unit(target);
    float coefficient = Unit(target > current ? attack : release);
    return Unit(current + (target - current) * coefficient);
}
inline float BandRange(const float* spectrum, size_t fftSize, size_t left, size_t right) {
    const size_t last = fftSize / 2 - 1;
    left = left < 1 ? 1 : left > last ? last : left;
    right = right < left ? left : right > last ? last : right;
    float sum = 0.0f;
    for (size_t bin = left; bin <= right; ++bin) sum += spectrum[bin];
    return sum / float(right - left + 1);
}
}
