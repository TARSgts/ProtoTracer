#include "FractalMaterial.h"

FractalMaterial::FractalMaterial(Vector2D size, Vector2D offset) {
    this->size = size.Divide(2.0f);
    this->offset = offset;
}

void FractalMaterial::SetSize(Vector2D size) {
    this->size = size.Divide(2.0f);
}

void FractalMaterial::SetPosition(Vector2D offset) {
    this->offset = offset;
}

void FractalMaterial::SetHueAngle(float hue) {
    this->hueAngle = hue;
}

void FractalMaterial::Update(float ratio) {
    timePhase = ratio;
}

RGBColor FractalMaterial::GetRGB(const Vector3D& position, const Vector3D& normal, const Vector3D& uvw) {
    (void)normal;
    (void)uvw;

    Vector2D p(position.X - offset.X, position.Y - offset.Y);
    if (size.X <= 0.0f || size.Y <= 0.0f) {
        return RGBColor(0, 0, 0);
    }

    float nx = p.X / size.X;
    float ny = p.Y / size.Y;
    if (nx < -1.0f || nx > 1.0f || ny < -1.0f || ny > 1.0f) {
        return RGBColor(0, 0, 0);
    }

    float zoom = 0.80f + 0.35f * sinf(timePhase * 2.0f * Mathematics::MPI);
    float cx = -0.78f + 0.20f * cosf(timePhase * 2.0f * Mathematics::MPI * 0.5f);
    float cy =  0.15f + 0.20f * sinf(timePhase * 2.0f * Mathematics::MPI * 0.5f);

    float x = nx * 1.5f / zoom;
    float y = ny * 1.0f / zoom;

    uint8_t i = 0;
    while (i < maxIterations) {
        float x2 = x * x - y * y + cx;
        float y2 = 2.0f * x * y + cy;
        x = x2;
        y = y2;
        if ((x * x + y * y) > 4.0f) break;
        i++;
    }

    if (i >= maxIterations) {
        return RGBColor(0, 0, 0);
    }

    float t = float(i) / float(maxIterations);
    RGBColor base(
        uint8_t(40.0f + 215.0f * t),
        uint8_t(255.0f * (1.0f - t)),
        uint8_t(180.0f + 75.0f * t)
    );

    return base.HueShift(hueAngle + t * 180.0f);
}

