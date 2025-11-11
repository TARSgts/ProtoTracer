#include "Oscilloscope.h"
#include <cmath>

Oscilloscope::Oscilloscope(Vector2D size, Vector2D offset) {
    this->size = size.Divide(2.0f);
    this->offset = offset;
    this->material = &gM;
}

Oscilloscope::~Oscilloscope() {
    for (uint8_t i = 0; i < 128; i++) {
        delete bPhy[i];
    }
}

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
    this->data = data;

    if (data == nullptr) {
        minValue = -1.0f;
        maxValue = 1.0f;
        midPoint = 0.0f;
        return;
    }

    float sample = data[bins / 2];
    minValue = minF.Filter(sample);
    maxValue = maxF.Filter(sample);

    if (fabsf(maxValue - minValue) < 0.05f) {
        maxValue = minValue + 0.05f;
    }

    midPoint = (maxValue - minValue) / 2.0f + minValue;
}

RGBColor Oscilloscope::GetRGB(const Vector3D& position, const Vector3D& normal, const Vector3D& uvw) {
    if (data == nullptr) return RGBColor();

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

    float firstPoint = Mathematics::Map(data[x], minValue, maxValue, 0.0f, 1.0f);
    float secondPoint = Mathematics::Map(data[x + 1], minValue, maxValue, 0.0f, 1.0f);

    float height = Mathematics::CosineInterpolation(firstPoint, secondPoint, ratio); // 0..1
    float yRatio = Mathematics::Map(rPos.Y, -size.Y, size.Y, 0.0f, 1.0f);

    const float traceThickness = 0.05f;
    if (fabsf(yRatio - height) <= traceThickness) {
        return material->GetRGB(Vector3D(1.0f - height, 0, 0), Vector3D(), Vector3D()).HueShift(hueAngle);
    } else {
        return RGBColor(0, 0, 0);
    }
}
