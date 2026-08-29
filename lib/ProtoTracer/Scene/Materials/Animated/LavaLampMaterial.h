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
        // Bistable thermal cycle: true while heating/rising, false while cooling/sinking.
        // Flips at the top/bottom of its travel range in Update() -- this replaces a
        // force-balance model that had a wide dead zone around vertical center where
        // buoyancy was ~0, which made blobs stall and hover mid-screen instead of
        // completing a full rise-and-fall cycle like a real lava lamp.
        bool rising;
        float riseStrength;
        float sinkStrength;
    };

    static constexpr uint8_t kBlobCount = 6;
    Blob blobs[kBlobCount];

    Vector2D size = Vector2D(192.0f, 94.0f);
    Vector2D offset = Vector2D(96.0f, 47.0f);
    float halfWidth = 96.0f;
    float halfHeight = 47.0f;

    RGBColor backgroundColor = RGBColor(0, 0, 0);
    // Color-temperature gradient: each blob's own color is derived from its current
    // height (hotColor near the bottom heat source, coolColor once it's risen and
    // cooled), blended per-pixel by field contribution -- see GetRGB(). Both are
    // re-derived from the active palette color in SetPalette().
    RGBColor hotColor = RGBColor(255, 195, 98);
    RGBColor coolColor = RGBColor(190, 78, 26);

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

