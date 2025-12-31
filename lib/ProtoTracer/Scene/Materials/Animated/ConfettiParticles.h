#pragma once

#include "../Material.h"
#include "../../../Utils/Math/Vector2D.h"

class ConfettiParticles : public Material {
private:
    struct Particle {
        Vector2D position;
        Vector2D velocity;
        RGBColor color;
        float life = 0.0f;
        float maxLife = 0.0f;
        float radius = 1.0f;
        bool active = false;
    };

    static constexpr uint8_t kMaxParticles = 24;
    Particle particles[kMaxParticles];
    uint8_t nextIndex = 0;

    Vector2D size;
    Vector2D offset;
    uint32_t lastUpdateMs = 0;

    float gravity = -35.0f;
    float drag = 0.98f;

    RGBColor palette[6] = {
        RGBColor(255, 90, 90),
        RGBColor(255, 200, 70),
        RGBColor(80, 220, 120),
        RGBColor(80, 160, 255),
        RGBColor(190, 120, 255),
        RGBColor(255, 255, 255)
    };

    RGBColor ScaleColor(const RGBColor& color, float scale) const;
    void SpawnParticle(const Vector2D& origin, const Vector2D& direction);

public:
    ConfettiParticles(Vector2D dimensions = Vector2D(192.0f, 94.0f), Vector2D center = Vector2D(96.0f, 47.0f));

    void SetSize(Vector2D dimensions);
    void SetPosition(Vector2D center);

    void Trigger(const Vector2D& origin, const Vector2D& direction, float spreadDeg = 60.0f, uint8_t count = 16);
    void Update();

    RGBColor GetRGB(const Vector3D& position, const Vector3D& normal, const Vector3D& uvw) override;
};
