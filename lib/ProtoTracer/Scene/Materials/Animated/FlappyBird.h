#pragma once

#include "../Material.h"
#include "../../../Utils/Math/Vector2D.h"

class FlappyBirdMaterial : public Material {
private:
    Vector2D size;
    Vector2D offset;
    float birdX = 0.0f;
    float birdY = 0.0f;
    float birdVelocity = 0.0f;
    float birdRadius = 3.0f;

    static constexpr uint8_t kPipeCount = 3;
    float pipeX[kPipeCount];
    float pipeGapY[kPipeCount];

    float pipeWidth = 10.0f;
    float pipeGapHeight = 26.0f;
    float pipeSpacing = 60.0f;
    float pipeSpeed = 60.0f;

    float gravity = -160.0f;
    float flapVelocity = 85.0f;
    float flapCooldown = 0.0f;
    float groundHeight = 3.0f;
    float gapPadding = 4.0f;

    uint32_t lastUpdateMs = 0;

    RGBColor skyColor = RGBColor(0, 0, 0);
    RGBColor pipeColor = RGBColor(30, 200, 80);
    RGBColor groundColor = RGBColor(124, 252, 0);
    RGBColor birdColor = RGBColor(255, 210, 30);
    RGBColor birdAccent = RGBColor(255, 130, 20);

    void RecalculateDimensions();
    void Reset();
    void ResetPipe(uint8_t index, float xPosition);
    float RandomGapCenter();
    int8_t FindNextPipeIndex() const;
    bool CheckCollision() const;

public:
    FlappyBirdMaterial(Vector2D dimensions, Vector2D center);

    void SetSize(Vector2D dimensions);
    void SetPosition(Vector2D center);
    void Update();

    RGBColor GetRGB(const Vector3D& position, const Vector3D& normal, const Vector3D& uvw) override;
};
