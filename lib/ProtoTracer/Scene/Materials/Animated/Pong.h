#pragma once

#include "../Material.h"
#include "../../../Utils/Math/Vector2D.h"

class PongFace : public Material {
private:
    Vector2D size;
    Vector2D offset;
    Vector2D ballPos;
    Vector2D ballVel;
    float paddleLeftY = 0.0f;
    float paddleRightY = 0.0f;
    float paddleWidth = 4.0f;
    float paddleHeight = 20.0f;
    float ballRadius = 3.0f;
    float paddleSpeed = 80.0f;
    float ballSpeed = 70.0f;
    float centerLineWidth = 2.0f;
    float paddleInset = 6.0f;
    uint32_t lastUpdateMs = 0;

    RGBColor paddleColor = RGBColor(230, 230, 255);
    RGBColor ballColor = RGBColor(255, 255, 255);
    RGBColor lineColor = RGBColor(40, 70, 120);
    RGBColor backgroundColor = RGBColor(3, 3, 6);

    void RecalculateDimensions();
    void ResetBall(bool serveRight);

public:
    PongFace(Vector2D dimensions, Vector2D center);

    void SetSize(Vector2D dimensions);
    void SetPosition(Vector2D center);
    void Update();

    RGBColor GetRGB(const Vector3D& position, const Vector3D& normal, const Vector3D& uvw) override;
};
