#include "SpectrumAnalyzer.h"

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
    if (bounce) {
        return bounceData;
    } else {
        return data;
    }
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
        return;
    }

    float peak = 0.0f;
    constexpr float emphasisSlope = 0.3f;
    constexpr float lowBinBoost = 1.3f;
    constexpr float riseCoeff = 0.4f;
    constexpr float fallCoeff = 0.08f;
    constexpr float peakFall = 0.94f;

    for (uint8_t i = 0; i < bins; i++) {
        float value = Mathematics::Constrain(readData[i], 0.0f, 1.0f);
        float emphasis = 0.85f + (float(i) / float(bins - 1)) * emphasisSlope;
        if (i < 6) emphasis *= lowBinBoost;
        value = Mathematics::Constrain(value * emphasis, 0.0f, 1.2f);

        float previous = smoothedData[i];
        float coeff = (value > previous) ? riseCoeff : fallCoeff;
        float smooth = previous + (value - previous) * coeff;
        smoothedData[i] = smooth;

        if (smooth > peakHoldData[i]) {
            peakHoldData[i] = smooth;
        } else {
            peakHoldData[i] *= peakFall;
        }

        processedData[i] = peakHoldData[i];
        if (processedData[i] > peak) peak = processedData[i];
    }

    constexpr float peakTarget = 0.9f;
    if (peak > 0.005f) {
        float desiredGain = peakTarget / peak;
        desiredGain = Mathematics::Constrain(desiredGain, 0.6f, 2.2f);
        float response = desiredGain > autoGain ? 0.18f : 0.06f;
        autoGain += (desiredGain - autoGain) * response;
    } else {
        autoGain += (1.0f - autoGain) * 0.04f;
    }

    for (uint8_t i = 0; i < bins; i++) {
        float value = Mathematics::Constrain(processedData[i] * autoGain, 0.0f, 1.1f);
        processedData[i] = powf(value, 0.9f);
    }

    if (bounce) {
        for (uint8_t i = 0; i < bins; i++) {
            bounceData[i] = bPhy[i]->Calculate(processedData[i], 0.1f);
        }
        data = bounceData;
    } else {
        data = processedData;
    }
}

RGBColor SpectrumAnalyzer::GetRGB(const Vector3D& position, const Vector3D& normal, const Vector3D& uvw) {
    Vector2D rPos = Mathematics::IsClose(angle, 0.0f, 0.1f)
                        ? Vector2D(position.X, position.Y) - offset
                        : Vector2D(position.X, position.Y).Rotate(angle, offset) - offset;

    if (data == nullptr) return RGBColor();

    if (rPos.X < -size.X || rPos.X > size.X) return RGBColor();
    if (rPos.Y < -size.Y || rPos.Y > size.Y) return RGBColor();

    float mapped = Mathematics::Map(rPos.X, -size.X, size.X, 0.0f, float(bins - 1));
    if (mapped < 0.0f) mapped = 0.0f;
    float maxIndex = float(bins - 1);
    if (mapped > maxIndex) mapped = maxIndex;

    uint8_t x = uint8_t(mapped);
    uint8_t nextIndex = x < bins - 1 ? x + 1 : x;
    float ratio = mapped - float(x);

    float firstVal = bounce ? bounceData[x] : data[x];
    float secondVal = bounce ? bounceData[nextIndex] : data[nextIndex];
    float height = Mathematics::CosineInterpolation(firstVal, secondVal, ratio);
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
