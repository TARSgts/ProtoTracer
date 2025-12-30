#pragma once

#include "../Material.h"
#include "../../../Utils/Math/Vector2D.h"

class SpaceInvadersMaterial : public Material {
private:
    Vector2D size;
    Vector2D offset;
    uint32_t lastUpdateMs = 0;

    static constexpr uint8_t kRows = 4;
    static constexpr uint8_t kCols = 6;
    bool alive[kRows][kCols];

    float invaderStep = 0.0f;
    int8_t invaderDir = 1;
    uint8_t invaderTopRow = 0;
    float invaderDrop = 0.0f;
    float invaderMoveTimer = 0.0f;
    float invaderMoveInterval = 0.45f;

    float invaderSize = 6.0f;
    float invaderSpacing = 4.0f;
    float invaderMarginTop = 6.0f;

    float playerX = 0.0f;
    float playerSpeed = 90.0f;
    float playerWidth = 10.0f;
    float playerHeight = 4.0f;
    float playerYOffset = 8.0f;

    bool playerShotActive = false;
    Vector2D playerShotPos = Vector2D();
    float playerShotSpeed = 150.0f;

    bool invaderShotActive = false;
    Vector2D invaderShotPos = Vector2D();
    float invaderShotSpeed = 70.0f;
    float invaderShotCooldown = 0.0f;

    RGBColor invaderColor = RGBColor(120, 220, 140);
    RGBColor playerColor = RGBColor(220, 220, 255);
    RGBColor shotColor = RGBColor(255, 230, 180);
    RGBColor backgroundColor = RGBColor(3, 3, 6);

    void ResetWave();
    void UpdateInvaders(float delta);
    void UpdatePlayer(float delta);
    void UpdateShots(float delta);
    Vector2D GetInvaderOrigin() const;
    bool IsInvaderAlive(uint8_t row, uint8_t col) const;
    void KillInvader(uint8_t row, uint8_t col);
    bool AllInvadersCleared() const;
    float GetInvaderRowY(uint8_t row) const;
    float GetInvaderColX(uint8_t col) const;
    void TryFireInvader();

public:
    SpaceInvadersMaterial(Vector2D dimensions, Vector2D center);

    void SetSize(Vector2D dimensions);
    void SetPosition(Vector2D center);
    void Update();

    RGBColor GetRGB(const Vector3D& position, const Vector3D& normal, const Vector3D& uvw) override;
};
