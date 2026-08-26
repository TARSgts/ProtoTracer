#pragma once

#include <stdint.h>

#include "../Material.h"
#include "../../../Utils/Math/Vector2D.h"

class LavaLampMaterial : public Material {
private:
    struct Blob {
        float x;
        float y;
        float vx;
        float vy;
        float radius;
        float phase;
        float pulseRate;
        float driftRate;
        float anchorX;
    };

    static constexpr uint8_t kBlobCount = 6;
    Blob blobs[kBlobCount];

    Vector2D size = Vector2D(192.0f, 94.0f);
    Vector2D offset = Vector2D(96.0f, 47.0f);
    float halfWidth = 96.0f;
    float halfHeight = 47.0f;

    RGBColor backgroundColor = RGBColor(0, 0, 0);
    RGBColor shellColor = RGBColor(190, 78, 26);
    RGBColor coreColor = RGBColor(255, 195, 98);

    float outerThreshold = 1.00f;
    float innerThreshold = 1.44f;
    float coreThreshold = 2.12f;

    uint32_t lastUpdateMs = 0;
    float timeSeconds = 0.0f;

    static float Clamp01(float value);
    static float Clamp(float value, float minimum, float maximum);
    static float SmoothStep(float value);
    static float Random01();
    static RGBColor ScaleColor(const RGBColor& color, float factor);
    static RGBColor LiftColor(const RGBColor& color, uint8_t amount);

    void RecomputeBounds();
    void ResetBlob(uint8_t index);
    void ResolveBlobSeparation(float dt);

public:
    LavaLampMaterial(Vector2D dimensions = Vector2D(192.0f, 94.0f), Vector2D center = Vector2D(96.0f, 47.0f));

    void SetSize(Vector2D dimensions);
    void SetPosition(Vector2D center);
    void SetPalette(const RGBColor& baseColor);
    void Update();

    RGBColor GetRGB(const Vector3D& position, const Vector3D& normal, const Vector3D& uvw) override;
};

