#pragma once

#include "../Material.h"
#include "../../../Utils/Math/Mathematics.h"

class FractalMaterial : public Material {
private:
    Vector2D size;
    Vector2D offset;
    float timePhase = 0.0f;
    float hueAngle = 0.0f;
    uint8_t maxIterations = 20;

public:
    FractalMaterial(Vector2D size, Vector2D offset);

    void SetSize(Vector2D size);
    void SetPosition(Vector2D offset);
    void SetHueAngle(float hue);
    void Update(float ratio);

    RGBColor GetRGB(const Vector3D& position, const Vector3D& normal, const Vector3D& uvw) override;
};

