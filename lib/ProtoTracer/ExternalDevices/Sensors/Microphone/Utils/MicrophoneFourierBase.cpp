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
float MicrophoneFourierBase::outputWaveformTrace[];
float MicrophoneFourierBase::waveformNormalization = 1.0f / 32768.0f;
FFTFilter MicrophoneFourierBase::fftFilters[];

FFT<MicrophoneFourierBase::FFTSize> MicrophoneFourierBase::fft;

void MicrophoneFourierBase::GenerateWaveform(uint16_t validSamples) {
    if (validSamples == 0) {
        for (uint16_t i = 0; i < OutputBins; ++i) {
            outputWaveform[i] = 0.0f;
            outputWaveformTrace[i] = 0.0f;
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

    float normalization = waveformNormalization > 0.0f ? waveformNormalization : 1.0f / 32768.0f;
    float normalizedPeak = maxDeviation * normalization;
    if (normalizedPeak < 0.01f) {
        for (uint16_t i = 0; i < OutputBins; ++i) {
            outputWaveform[i] = 0.0f;
            outputWaveformTrace[i] = 0.0f;
        }
        return;
    }

    constexpr float magnitudeBoost = 6.0f;
    constexpr float magnitudeNoiseFloor = 0.02f;
    constexpr float traceBoost = 4.0f;
    constexpr float traceNoiseFloor = 0.01f;

    for (uint16_t bin = 0; bin < OutputBins; ++bin) {
        uint32_t start = (uint32_t(bin) * validSamples) / OutputBins;
        uint32_t end = (uint32_t(bin + 1) * validSamples) / OutputBins;

        if (start >= validSamples) {
            outputWaveform[bin] = 0.0f;
            outputWaveformTrace[bin] = 0.0f;
            continue;
        }

        if (end <= start) {
            end = start + 1;
        }
        if (end > validSamples) {
            end = validSamples;
        }

        float absAccum = 0.0f;
        float signedAccum = 0.0f;
        for (uint32_t idx = start; idx < end; ++idx) {
            float centered = inputStorage[idx] - mean;
            absAccum += fabsf(centered);
            signedAccum += centered;
        }

        uint32_t sampleCount = end - start;
        if (sampleCount == 0) {
            sampleCount = 1;
        }

        float averageAbs = (absAccum / float(sampleCount)) * normalization;
        float magnitude = averageAbs * magnitudeBoost;
        if (magnitude <= magnitudeNoiseFloor) {
            magnitude = 0.0f;
        } else {
            magnitude = (magnitude - magnitudeNoiseFloor);
            if (magnitude > 1.0f) magnitude = 1.0f;
        }

        float averageSigned = (signedAccum / float(sampleCount)) * normalization * traceBoost;
        if (averageSigned > 1.0f) averageSigned = 1.0f;
        if (averageSigned < -1.0f) averageSigned = -1.0f;
        if (fabsf(averageSigned) <= traceNoiseFloor) {
            averageSigned = 0.0f;
        }

        outputWaveform[bin] = magnitude;
        outputWaveformTrace[bin] = averageSigned;
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

float* MicrophoneFourierBase::GetWaveformTrace() {
    return outputWaveformTrace;
}

void MicrophoneFourierBase::SetWaveformNormalization(float normalization) {
    waveformNormalization = normalization <= 0.0f ? 1.0f / 32768.0f : normalization;
}

float MicrophoneFourierBase::GetCurrentMagnitude() {
    return threshold;
}
