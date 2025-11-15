#include "SpectrumAnalyzer.h"
#include <cmath>

SpectrumAnalyzer::SpectrumAnalyzer(Vector2D size, Vector2D offset, bool bounce, bool flipY, bool mirrorY) {
    this->size = size.Divide(2.0f);
    this->offset = offset;
    this->mirrorY = mirrorY;
    this->flipY = flipY;
    this->bounce = bounce;
    this->material = &gM;

    if (bounce) {
        for (uint8_t i = 0; i < 128; i++) {
            bPhy[i] = new BouncePhysics(35.0f, 15.0f);
        }
    }
}

SpectrumAnalyzer::~SpectrumAnalyzer() {
    for (uint8_t i = 0; i < 128; i++) {
        delete bPhy[i];
    }
}

void SpectrumAnalyzer::SetMirrorYState(bool state) {
    mirrorY = state;
}

void SpectrumAnalyzer::SetFlipYState(bool state) {
    flipY = state;
}

void SpectrumAnalyzer::SetMaterial(Material* material) {
    this->material = material;
}

float* SpectrumAnalyzer::GetFourierData() {
    return bounce ? bounceData : data;
}

void SpectrumAnalyzer::SetSize(Vector2D size) {
    this->size = size.Divide(2.0f);
}

void SpectrumAnalyzer::SetPosition(Vector2D offset) {
    this->offset = offset;
}

void SpectrumAnalyzer::SetRotation(float angle) {
    this->angle = angle;
}

void SpectrumAnalyzer::SetHueAngle(float hueAngle) {
    this->hueAngle = hueAngle;
}

void SpectrumAnalyzer::Update(float* readData) {
    if (!readData) {
        data = processedData;
        visualReady = false;
        return;
    }

    constexpr float emphasisSlope = 0.3f;
    constexpr float lowBinBoost = 1.32f;
    constexpr float riseCoeff = 0.5f;
    constexpr float fallCoeff = 0.16f;
    constexpr float peakFall = 0.92f;
    constexpr uint8_t peakHoldFrames = 24;
    constexpr float holdTolerance = 0.01f;

    float peak = 0.0f;
    float noiseAccumulator = 0.0f;

    for (uint8_t i = 0; i < bins; ++i) {
        float value = Mathematics::Constrain(readData[i], 0.0f, 1.0f);
        float emphasis = 0.85f + (float(i) / float(bins - 1)) * emphasisSlope;
        if (i < 6) emphasis *= lowBinBoost;
        value = Mathematics::Constrain(value * emphasis, 0.0f, 1.2f);
        noiseAccumulator += value;

        float previous = smoothedData[i];
        float coeff = (value > previous) ? riseCoeff : fallCoeff;
        float smooth = previous + (value - previous) * coeff;
        smoothedData[i] = smooth;

        if (smooth + holdTolerance >= peakHoldData[i]) {
            peakHoldData[i] = smooth;
            peakHoldTimer[i] = peakHoldFrames;
        } else if (peakHoldTimer[i] > 0) {
            peakHoldTimer[i]--;
        } else {
            peakHoldData[i] *= peakFall;
            if (peakHoldData[i] < 0.001f) peakHoldData[i] = 0.0f;
        }

        processedData[i] = peakHoldData[i];
        if (processedData[i] > peak) peak = processedData[i];
    }

    noiseAccumulator /= float(bins);
    noiseFloor = noiseFloor * 0.9f + noiseAccumulator * 0.1f;
    float floorClamp = Mathematics::Constrain(noiseFloor * 1.05f, 0.01f, 0.08f);

    constexpr float peakTarget = 0.9f;
    bool quiet = peak < floorClamp * 2.2f;
    idleFrames = quiet ? uint8_t(std::min<int>(idleFrames + 1, 90)) : 0;

    if (peak > 0.005f) {
        float desiredGain = peakTarget / peak;
        desiredGain = Mathematics::Constrain(desiredGain, 0.6f, 2.2f);
        float response = desiredGain > autoGain ? 0.18f : 0.06f;
        autoGain += (desiredGain - autoGain) * response;
    } else {
        autoGain += (1.0f - autoGain) * 0.04f;
    }

    float suppression = floorClamp * 2.0f;
    for (uint8_t i = 0; i < bins; ++i) {
        float value = processedData[i] - suppression;
        value = value < 0.0f ? 0.0f : value;
        value = Mathematics::Constrain(value * autoGain, 0.0f, 1.1f);
        processedData[i] = powf(value, 0.88f);
    }

    if (quiet && idleFrames > 18) {
        float quietDecay = idleFrames > 45 ? 0.5f : 0.7f;
        for (uint8_t i = 0; i < bins; ++i) {
            peakHoldTimer[i] = 0;
            peakHoldData[i] *= quietDecay;
            smoothedData[i] *= quietDecay;
            processedData[i] = peakHoldData[i];
        }
        visualReady = false;
    }

    const float* source = processedData;
    if (bounce) {
        for (uint8_t i = 0; i < bins; ++i) {
            bounceData[i] = bPhy[i]->Calculate(processedData[i], 0.1f);
        }
        data = bounceData;
        source = bounceData;
    } else {
        data = processedData;
    }

    BuildVisualData(source);
}

RGBColor SpectrumAnalyzer::GetRGB(const Vector3D& position, const Vector3D& normal, const Vector3D& uvw) {
    Vector2D rPos = Mathematics::IsClose(angle, 0.0f, 0.1f)
                        ? Vector2D(position.X, position.Y) - offset
                        : Vector2D(position.X, position.Y).Rotate(angle, offset) - offset;

    const float* renderData = visualReady ? visualData : data;
    if (renderData == nullptr) return RGBColor();

    if (rPos.X < -size.X || rPos.X > size.X) return RGBColor();
    if (rPos.Y < -size.Y || rPos.Y > size.Y) return RGBColor();

    float mapped = Mathematics::Map(rPos.X, -size.X, size.X, 0.0f, float(bins - 1));
    if (mapped < 0.0f) mapped = 0.0f;
    float maxIndex = float(bins - 1);
    if (mapped > maxIndex) mapped = maxIndex;

    float height = SampleFrequency(mapped);
    height = Mathematics::Constrain(height * 2.6f, 0.0f, 1.0f);
    float yColor;

    if (mirrorY) {
        yColor = Mathematics::Map(fabsf(rPos.Y), size.Y, 0.0f, 1.0f, 0.0f);
    } else {
        yColor = Mathematics::Map(rPos.Y, -size.Y, size.Y, 1.0f, 0.0f);
    }

    if (flipY) yColor = 1.0f - yColor;

    if (yColor <= height) {
        return material->GetRGB(Vector3D(1.0f - height - yColor, 0, 0), Vector3D(), Vector3D()).HueShift(hueAngle);
    } else {
        return RGBColor(0, 0, 0);
    }
}

void SpectrumAnalyzer::BuildVisualData(const float* source) {
    if (!source) {
        visualReady = false;
        return;
    }

    constexpr float kernel[5] = {0.08f, 0.2f, 0.44f, 0.2f, 0.08f};
    constexpr int radius = 2;

    for (uint8_t i = 0; i < bins; ++i) {
        float sum = 0.0f;
        for (int k = -radius; k <= radius; ++k) {
            int idx = int(i) + k;
            if (idx < 0) idx = 0;
            if (idx >= bins) idx = bins - 1;
            sum += source[idx] * kernel[k + radius];
        }
        visualData[i] = sum;
    }

    visualReady = true;
}

float SpectrumAnalyzer::SampleFrequency(float index) const {
    const float* source = visualReady ? visualData : (bounce ? bounceData : data);
    if (source == nullptr) return 0.0f;

    float maxIndex = float(bins - 1);
    if (index < 0.0f) index = 0.0f;
    if (index > maxIndex) index = maxIndex;

    uint8_t left = uint8_t(index);
    uint8_t right = left < bins - 1 ? left + 1 : left;
    float frac = index - float(left);
    return Mathematics::CosineInterpolation(source[left], source[right], frac);
}
