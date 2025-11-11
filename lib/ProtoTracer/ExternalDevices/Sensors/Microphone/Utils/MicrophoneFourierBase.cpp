#include "MicrophoneFourierBase.h"

const uint16_t MicrophoneFourierBase::FFTSize;
const uint16_t MicrophoneFourierBase::OutputBins;
uint16_t MicrophoneFourierBase::sampleRate = 8000;
uint8_t MicrophoneFourierBase::pin = 0;
float MicrophoneFourierBase::minDB = 50.0f;
float MicrophoneFourierBase::maxDB = 120.0f;
float MicrophoneFourierBase::threshold = 400.0f;
float MicrophoneFourierBase::currentValue = 0.0f;
bool MicrophoneFourierBase::isInitialized = false;
DerivativeFilter MicrophoneFourierBase::peakFilterRate;

float MicrophoneFourierBase::inputSamp[];
float MicrophoneFourierBase::inputStorage[];
float MicrophoneFourierBase::outputMagn[];
float MicrophoneFourierBase::outputData[];
float MicrophoneFourierBase::outputDataFilt[];
float MicrophoneFourierBase::outputWaveform[];
float MicrophoneFourierBase::waveformNormalization = 1.0f / 32768.0f;
FFTFilter MicrophoneFourierBase::fftFilters[];

FFT<MicrophoneFourierBase::FFTSize> MicrophoneFourierBase::fft;

void MicrophoneFourierBase::GenerateWaveform(uint16_t validSamples) {
    if (validSamples == 0) {
        for (uint16_t i = 0; i < OutputBins; ++i) {
            outputWaveform[i] = 0.0f;
        }
        return;
    }

    if (validSamples > FFTSize) {
        validSamples = FFTSize;
    }

    float mean = 0.0f;
    for (uint16_t i = 0; i < validSamples; ++i) {
        mean += inputStorage[i];
    }
    mean /= float(validSamples);

    float maxDeviation = 0.0f;
    for (uint16_t i = 0; i < validSamples; ++i) {
        float deviation = fabsf(inputStorage[i] - mean);
        if (deviation > maxDeviation) {
            maxDeviation = deviation;
        }
    }

    if (maxDeviation < 1e-3f) {
        for (uint16_t i = 0; i < OutputBins; ++i) {
            outputWaveform[i] = 0.0f;
        }
        return;
    }

    constexpr float noiseFloor = 0.02f;
    constexpr float visualizationBoost = 8.0f;
    if (waveformNormalization <= 0.0f) {
        waveformNormalization = 1.0f / 32768.0f;
    }

    float amplitudeScale = maxDeviation * waveformNormalization * visualizationBoost;
    if (amplitudeScale > 1.0f) {
        amplitudeScale = 1.0f;
    }

    for (uint16_t bin = 0; bin < OutputBins; ++bin) {
        uint32_t start = (uint32_t(bin) * validSamples) / OutputBins;
        uint32_t end = (uint32_t(bin + 1) * validSamples) / OutputBins;

        if (start >= validSamples) {
            outputWaveform[bin] = 0.0f;
            continue;
        }

        if (end <= start) {
            end = start + 1;
        }
        if (end > validSamples) {
            end = validSamples;
        }

        float binAccum = 0.0f;
        for (uint32_t idx = start; idx < end; ++idx) {
            binAccum += fabsf(inputStorage[idx] - mean);
        }

        uint32_t sampleCount = end - start;
        if (sampleCount == 0) {
            sampleCount = 1;
        }

        float normalized = binAccum / float(sampleCount); // average absolute deviation
        normalized = normalized / maxDeviation; // relative energy per bin (0-1)
        float value = normalized * amplitudeScale;

        if (value <= noiseFloor) {
            outputWaveform[bin] = 0.0f;
        } else {
            value -= noiseFloor;
            if (value > 1.0f) value = 1.0f;
            outputWaveform[bin] = value;
        }
    }
}

float MicrophoneFourierBase::AverageMagnitude(uint16_t binL, uint16_t binH) {
    float average = 0.0f;

    for (uint16_t i = 1; i < FFTSize / 2; i++) {
        if (i >= binL && i <= binH)
            average += outputMagn[i];
    }

    return average / float(binH - binL + 1);
}

bool MicrophoneFourierBase::IsInitialized() {
    return isInitialized;
}

float MicrophoneFourierBase::GetSampleRate() {
    return sampleRate;
}

float* MicrophoneFourierBase::GetSamples() {
    return inputStorage;
}

float* MicrophoneFourierBase::GetFourier() {
    return outputData;
}

float* MicrophoneFourierBase::GetFourierFiltered() {
    return outputDataFilt;
}

float* MicrophoneFourierBase::GetWaveform() {
    return outputWaveform;
}

void MicrophoneFourierBase::SetWaveformNormalization(float normalization) {
    waveformNormalization = normalization <= 0.0f ? 1.0f / 32768.0f : normalization;
}

float MicrophoneFourierBase::GetCurrentMagnitude() {
    return threshold;
}
