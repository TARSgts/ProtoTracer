#include "MicrophoneFourier_MAX9814.h"

IntervalTimer MicrophoneFourier::sampleTimer;
TimeStep MicrophoneFourier::timeStep = TimeStep(60);

uint16_t MicrophoneFourier::frequencyBins[];

uint16_t MicrophoneFourier::samples = 0;
uint16_t MicrophoneFourier::samplesStorage = 0;
float MicrophoneFourier::refreshRate = 60.0f;
bool MicrophoneFourier::samplesReady = false;


void MicrophoneFourier::SamplerCallback() {
    int inputSample = analogRead(pin);

    inputSamp[samples++] = (float)inputSample;
    inputSamp[samples++] = 0.0f;

    inputStorage[samplesStorage++] = inputSample;

    if (samples >= FFTSize * 2) {
        sampleTimer.end();
        samplesReady = true;
    }
}

void MicrophoneFourier::StartSampler() {
    samplesReady = false;
    samples = 0;
    samplesStorage = 0;
    sampleTimer.begin(SamplerCallback, 1000000 / sampleRate);
}

void MicrophoneFourier::Initialize(uint8_t pin, uint16_t sampleRate, float minDB, float maxDB, float refreshRate) {
    MicrophoneFourier::minDB = minDB;
    MicrophoneFourier::maxDB = maxDB;
    MicrophoneFourier::pin = pin;
    MicrophoneFourier::refreshRate = refreshRate;

    pinMode(pin, INPUT);
    analogReadResolution(12);
    SetWaveformNormalization(1.0f / 2048.0f);

    MicrophoneFourier::sampleRate = sampleRate;
    MicrophoneFourier::samples = 0;
    MicrophoneFourier::samplesReady = false;

    constexpr uint16_t kMaxFftBin = (FFTSize / 2) - 1;
    constexpr float kMinDisplayFrequencyHz = 35.0f;
    constexpr float kLogFrequencyCurve = 0.82f;
    const float binWidthHz = float(sampleRate) / float(FFTSize);
    const float nyquistHz = float(sampleRate) * 0.5f;
    float minFrequencyHz = kMinDisplayFrequencyHz;
    if (minFrequencyHz < binWidthHz) minFrequencyHz = binWidthHz;
    if (minFrequencyHz > nyquistHz) minFrequencyHz = nyquistHz;
    const float logMinFrequencyHz = logf(minFrequencyHz);
    const float logRangeHz = logf(nyquistHz) - logMinFrequencyHz;

    timeStep.SetFrequency(refreshRate);

    // Allocate visible bins logarithmically so bass has more horizontal detail than treble.
    for (uint8_t i = 0; i < OutputBins; i++) {
        float t = (OutputBins > 1) ? (float(i) / float(OutputBins - 1)) : 0.0f;
        float curvedT = powf(t, kLogFrequencyCurve);
        float frequencyHz = expf(logMinFrequencyHz + logRangeHz * curvedT);
        uint16_t fftBin = static_cast<uint16_t>(frequencyHz / binWidthHz);

        if (fftBin < 1) fftBin = 1;
        if (fftBin > kMaxFftBin) fftBin = kMaxFftBin;
        if (i > 0 && fftBin < frequencyBins[i - 1]) fftBin = frequencyBins[i - 1];

        frequencyBins[i] = fftBin;
    }

    StartSampler();
    isInitialized = true;
}

void MicrophoneFourier::Reset() {
    for (int i = 0; i < FFTSize * 2; i++) {
        inputSamp[i] = 0.0f;
    }
}

void MicrophoneFourier::Update() {
    // Process only when both the refresh interval elapsed and a full FFT window is ready.
    if (!samplesReady || !timeStep.IsReady()) return;

    GenerateWaveform(samplesStorage);

    fft.Radix2FFT(inputSamp);
    fft.ComplexMagnitude(inputSamp, outputMagn);

    float averageMagnitude = 0.0f;

    constexpr uint16_t kMaxFftBin = (FFTSize / 2) - 1;
    // Keep the old 500-900Hz hiss gate available for quick A/B testing.
    constexpr bool kEnableMidBandNoiseGate = false;
    constexpr float kMidBandMinHz = 500.0f;
    constexpr float kMidBandMaxHz = 900.0f;
    constexpr float kMidBandGate = 0.10f;
    constexpr float kMidBandLowLevelAttenuation = 0.28f;

    for (uint8_t i = 0; i < OutputBins; i++) {
        uint16_t binL = frequencyBins[i];
        uint16_t binH = (i + 1 < OutputBins)
                            ? static_cast<uint16_t>(frequencyBins[i + 1] > 0 ? frequencyBins[i + 1] - 1 : 0)
                            : kMaxFftBin;

        if (binL < 1) binL = 1;
        if (binL > kMaxFftBin) binL = kMaxFftBin;
        if (binH < 1) binH = 1;
        if (binH > kMaxFftBin) binH = kMaxFftBin;
        if (binH < binL) binH = binL;

        float magnitude = AverageMagnitude(binL, binH);
        if (magnitude < 1.0e-9f) magnitude = 1.0e-9f;
        float intensity = 20.0f * log10f(magnitude);

        float range = maxDB - minDB;
        if (range <= 0.0f) range = 1.0f;
        intensity = (intensity - minDB) / range;
        if (intensity < 0.0f) intensity = 0.0f;
        if (intensity > 1.0f) intensity = 1.0f;

        if (kEnableMidBandNoiseGate) {
            // Optional suppression for weak random hiss in the 500-900Hz band.
            float centerHz = ((float(binL) + float(binH)) * 0.5f) * (float(sampleRate) / float(FFTSize));
            if (centerHz >= kMidBandMinHz && centerHz <= kMidBandMaxHz) {
                if (intensity <= kMidBandGate) {
                    intensity *= kMidBandLowLevelAttenuation;
                } else {
                    intensity = (intensity - kMidBandGate) / (1.0f - kMidBandGate);
                }
            }
        }

        outputData[i] = intensity;
        outputDataFilt[i] = fftFilters[i].Filter(intensity);
        if (i % 12 == 0) averageMagnitude = peakFilterRate.Filter(inputStorage[i] / 4096.0f);
    }

    averageMagnitude *= 10.0f;
    threshold = powf(averageMagnitude, 2.0f);
    threshold = threshold > 0.2f ? (threshold * 5.0f > 1.0f ? 1.0f : threshold * 5.0f) : 0.0f;

    Reset();
    StartSampler();
}
