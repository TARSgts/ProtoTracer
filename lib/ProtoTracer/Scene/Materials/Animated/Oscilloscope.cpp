#include "Oscilloscope.h"
#include <cmath>

Oscilloscope::Oscilloscope(Vector2D size, Vector2D offset) {
    this->size = size.Divide(2.0f);
    this->offset = offset;
    this->material = &gM;
    for (uint8_t i = 0; i < bins; ++i) {
        scopedData[i] = 0.5f;
        filteredWave[i] = 0.0f;
    }
}

Oscilloscope::~Oscilloscope() {}

void Oscilloscope::SetMaterial(Material* material) {
    this->material = material;
}

float* Oscilloscope::GetSampleData() {
    return data;
}

void Oscilloscope::SetSize(Vector2D size) {
    this->size = size.Divide(2.0f);
}

void Oscilloscope::SetPosition(Vector2D offset) {
    this->offset = offset;
}

void Oscilloscope::SetRotation(float angle) {
    this->angle = angle;
}

void Oscilloscope::SetHueAngle(float hueAngle) {
    this->hueAngle = hueAngle;
}

void Oscilloscope::Update(float* data) {
    if (data == nullptr) {
        this->data = nullptr;
        hasData = false;
        return;
    }

    constexpr float kMinimumSignal = 1.0f;
    constexpr float kAttack = 0.55f;
    constexpr float kRelease = 0.22f;
    constexpr float kAmplitudeAttack = 0.30f;
    constexpr float kAmplitudeRelease = 0.08f;

    float mean = 0.0f;
    for (uint16_t i = 0; i < kInputSamples; ++i) {
        mean += data[i];
    }
    mean /= float(kInputSamples);

    float peakAbs = 0.0f;
    for (uint16_t i = 0; i < kInputSamples; ++i) {
        float centered = data[i] - mean;
        float absCentered = fabsf(centered);
        if (absCentered > peakAbs) peakAbs = absCentered;
    }
    if (peakAbs < kMinimumSignal) peakAbs = kMinimumSignal;

    float framePeak = 0.0f;
    for (uint8_t i = 0; i < bins; ++i) {
        uint16_t start = (uint32_t(i) * kInputSamples) / bins;
        uint16_t end = (uint32_t(i + 1) * kInputSamples) / bins;
        if (end <= start) end = static_cast<uint16_t>(start + 1);
        if (end > kInputSamples) end = kInputSamples;

        float sum = 0.0f;
        uint16_t count = 0;
        for (uint16_t s = start; s < end; ++s) {
            sum += data[s] - mean;
            ++count;
        }
        if (count == 0) count = 1;

        float sample = (sum / float(count)) / peakAbs;
        sample = Mathematics::Constrain(sample, -1.0f, 1.0f);

        uint8_t prevIdx = i > 0 ? static_cast<uint8_t>(i - 1) : i;
        uint8_t nextIdx = i + 1 < bins ? static_cast<uint8_t>(i + 1) : i;
        float spatial = filteredWave[prevIdx] * 0.20f + sample * 0.60f + filteredWave[nextIdx] * 0.20f;

        float previous = filteredWave[i];
        float coeff = fabsf(spatial) > fabsf(previous) ? kAttack : kRelease;
        filteredWave[i] = previous + (spatial - previous) * coeff;

        float absValue = fabsf(filteredWave[i]);
        if (absValue > framePeak) framePeak = absValue;
    }

    float targetAmplitude = Mathematics::Constrain(framePeak * 1.6f, 0.18f, 1.0f);
    float amplitudeCoeff = targetAmplitude > displayAmplitude ? kAmplitudeAttack : kAmplitudeRelease;
    displayAmplitude += (targetAmplitude - displayAmplitude) * amplitudeCoeff;
    if (displayAmplitude < 0.18f) displayAmplitude = 0.18f;

    float invAmplitude = 0.5f / displayAmplitude;
    for (uint8_t i = 0; i < bins; ++i) {
        float normalized = 0.5f + filteredWave[i] * invAmplitude;
        scopedData[i] = Mathematics::Constrain(normalized, 0.0f, 1.0f);
    }

    this->data = scopedData;
    hasData = true;
}

RGBColor Oscilloscope::GetRGB(const Vector3D& position, const Vector3D& normal, const Vector3D& uvw) {
    if (!hasData || data == nullptr) return RGBColor();

    Vector2D rPos = Mathematics::IsClose(angle, 0.0f, 0.1f) ? Vector2D(position.X, position.Y) - offset : Vector2D(position.X, position.Y).Rotate(angle, offset) - offset;

    // Outside of size bounds
    if (rPos.X < -size.X || rPos.X > size.X) return RGBColor();
    if (rPos.Y < -size.Y || rPos.Y > size.Y) return RGBColor();

    float mappedX = Mathematics::Map(rPos.X, -size.X, size.X, 0.0f, float(bins - 1));
    if (mappedX < 0.0f) mappedX = 0.0f;
    float maxIndex = float(bins - 2);
    if (mappedX > maxIndex) mappedX = maxIndex;

    uint8_t x = static_cast<uint8_t>(mappedX);
    float ratio = mappedX - float(x);

    float firstPoint = data[x];
    float secondPoint = data[x + 1];
    float delta = secondPoint - firstPoint;
    float absDelta = fabsf(delta);

    // Smoothstep interpolation keeps peaks connected while remaining cheaper than trig interpolation.
    float smoothRatio = ratio * ratio * (3.0f - 2.0f * ratio);
    float height = firstPoint + delta * smoothRatio; // 0..1
    float yRatio = Mathematics::Map(rPos.Y, -size.Y, size.Y, 0.0f, 1.0f);
    float distance = fabsf(yRatio - height);

    constexpr float traceCoreThickness = 0.020f;
    constexpr float traceFeatherThickness = 0.045f;
    float slopeBoost = Mathematics::Constrain(absDelta * 0.10f, 0.0f, 0.035f);
    float effectiveCoreThickness = traceCoreThickness + slopeBoost;
    float effectiveFeatherThickness = traceFeatherThickness + slopeBoost;
    if (distance > effectiveFeatherThickness) return RGBColor(0, 0, 0);

    RGBColor baseColor = material->GetRGB(Vector3D(1.0f - height, 0, 0), Vector3D(), Vector3D()).HueShift(hueAngle);
    if (distance <= effectiveCoreThickness) return baseColor;

    float fade = (effectiveFeatherThickness - distance) / (effectiveFeatherThickness - effectiveCoreThickness);
    if (fade < 0.0f) fade = 0.0f;
    if (fade > 1.0f) fade = 1.0f;

    float minEdgeBrightness = 0.18f;
    float edgeBrightness = minEdgeBrightness + (1.0f - minEdgeBrightness) * fade;
    return baseColor.Scale(static_cast<uint8_t>(edgeBrightness * 255.0f));
}
