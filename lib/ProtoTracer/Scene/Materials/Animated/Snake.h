#pragma once

#include "../Material.h"
#include "../../../Utils/Math/Vector2D.h"

class SnakeMaterial : public Material {
private:
    static constexpr uint16_t kMaxSegments = 256;

    Vector2D size;
    Vector2D offset;
    uint8_t gridWidth = 0;
    uint8_t gridHeight = 0;
    float cellSize = 6.0f;
    float gridHalfWidth = 0.0f;
    float gridHalfHeight = 0.0f;

    int16_t snakeX[kMaxSegments];
    int16_t snakeY[kMaxSegments];
    uint16_t snakeLength = 0;
    int8_t dirX = 1;
    int8_t dirY = 0;
    int16_t foodX = 0;
    int16_t foodY = 0;

    float moveTimer = 0.0f;
    float moveInterval = 0.18f;
    uint32_t lastUpdateMs = 0;

    RGBColor backgroundColor = RGBColor(0, 0, 0);
    RGBColor snakeColor = RGBColor(30, 200, 80);
    RGBColor snakeHeadColor = RGBColor(80, 255, 120);
    RGBColor foodColor = RGBColor(255, 60, 60);

    void RecalculateDimensions();
    void Reset();
    void PlaceFood();
    bool IsCellOccupied(int16_t x, int16_t y) const;
    bool IsMoveSafe(int16_t x, int16_t y, bool grow) const;
    void Step();

public:
    SnakeMaterial(Vector2D dimensions, Vector2D center);

    void SetSize(Vector2D dimensions);
    void SetPosition(Vector2D center);
    void Update();

    RGBColor GetRGB(const Vector3D& position, const Vector3D& normal, const Vector3D& uvw) override;
};
