#pragma once

#include "../Material.h"
#include "../../../Utils/Math/Vector2D.h"

class PacmanMaterial : public Material {
private:
    static constexpr uint8_t kMaxPellets = 24;

    Vector2D size;
    Vector2D offset;
    float pacmanX = 0.0f;
    float pacmanY = 0.0f;
    float pacmanSpeed = 50.0f;
    float pacmanRadius = 4.0f;
    float mouthPhase = 0.0f;
    float mouthSpeed = 6.0f;

    float ghostX = 0.0f;
    float ghostY = 0.0f;
    float ghostRadius = 4.0f;
    float ghostSpeed = 45.0f;
    float ghostSpacing = 20.0f;

    float pelletX[kMaxPellets];
    bool pelletActive[kMaxPellets];
    uint8_t pelletCount = 0;
    float pelletRadius = 1.0f;
    float pelletSpacing = 10.0f;

    float leftBound = 0.0f;
    float rightBound = 0.0f;

    uint32_t lastUpdateMs = 0;

    RGBColor backgroundColor = RGBColor(0, 0, 0);
    RGBColor pacmanColor = RGBColor(255, 230, 60);
    RGBColor ghostColor = RGBColor(70, 180, 255);
    RGBColor pelletColor = RGBColor(255, 255, 255);

    void RecalculateDimensions();
    void Reset();
    void SetupPellets();
    void ResetPellets();
    bool AllPelletsEaten() const;

public:
    PacmanMaterial(Vector2D dimensions, Vector2D center);

    void SetSize(Vector2D dimensions);
    void SetPosition(Vector2D center);
    void Update();

    RGBColor GetRGB(const Vector3D& position, const Vector3D& normal, const Vector3D& uvw) override;
};
