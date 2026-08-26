#pragma once

#include "../Material.h"
#include "../../../Utils/Math/Vector2D.h"

class DinoGameMaterial : public Material {
private:
    Vector2D size;
    Vector2D offset;

    float dinoX = 0.0f;
    float dinoY = 0.0f;
    float dinoVelocity = 0.0f;
    float dinoWidth = 10.0f;
    float dinoHeight = 14.0f;
    bool onGround = true;
    bool jumpLatched = false;
    bool legPhase = false;
    float legTimer = 0.0f;

    enum class CactusType : uint8_t { Small, Medium, Double };

    static constexpr uint8_t kObstacleCount = 3;
    float obstacleX[kObstacleCount];
    CactusType obstacleType[kObstacleCount];

    static constexpr uint8_t kCloudCount = 3;
    float cloudX[kCloudCount];
    float cloudY[kCloudCount];

    float groundHeight = 3.0f;
    float obstacleSpeed = 55.0f;
    float minObstacleSpeed = 55.0f;
    float maxObstacleSpeed = 130.0f;
    float speedRampPerSecond = 1.0f; ///< Time-based; ~120s to go from min to max speed.
    float minGapPixels = 40.0f;
    float gravity = -220.0f;
    float jumpVelocity = 95.0f;
    float survivalTime = 0.0f;
    float distance = 0.0f;
    uint32_t score = 0;

    float hillPhase = 0.0f;
    float hillSpeed = 18.0f;
    float hillAmplitude = 6.0f;
    float hillBaseHeight = 3.0f;
    float hillFrequency = 0.05f;

    float cloudSpeed = 9.0f;
    float cloudWidth = 10.0f;
    float cloudHeight = 3.5f;

    uint32_t lastUpdateMs = 0;

    // Monochrome palette, matching the real Chrome Dino's black/white look (this is the
    // "night mode" polarity -- black background, white foreground -- since every other
    // face in this project uses a black background too). Dino/cactus are pure white
    // ("hero" foreground); ground is a clearly dimmer mid-gray so it reads as the floor
    // rather than blending into the sprites standing on it; hills/clouds are dimmer
    // still, for background depth.
    RGBColor skyColor = RGBColor(0, 0, 0);
    RGBColor groundColor = RGBColor(150, 150, 150);
    RGBColor hillColor = RGBColor(55, 55, 55);
    RGBColor cloudColor = RGBColor(95, 95, 95);
    RGBColor dinoColor = RGBColor(255, 255, 255);
    RGBColor dinoEyeColor = RGBColor(0, 0, 0);
    RGBColor cactusColor = RGBColor(255, 255, 255);
    RGBColor scoreColor = RGBColor(255, 255, 255);

    void RecalculateDimensions();
    void Reset();
    void ResetObstacle(uint8_t index, float xPosition);
    CactusType RandomObstacleType() const;
    void ResetClouds();
    void UpdateClouds(float delta);
    /**
     * @brief Gap to the next obstacle, scaled off current speed (like the real Chrome
     *        Dino's minGap = width*speed + ...) so the time to react stays roughly
     *        constant even as the game speeds up, instead of shrinking.
     */
    float ComputeGap() const;
    bool CheckCollision() const;

public:
    DinoGameMaterial(Vector2D dimensions, Vector2D center);

    void SetSize(Vector2D dimensions);
    void SetPosition(Vector2D center);

    /**
     * @brief Feeds the jump button state in (driven by the boop sensor). Jumps on the
     *        rising edge while the dino is on the ground; holding does not repeat the jump.
     */
    void SetJumpPressed(bool pressed);
    void Update();

    RGBColor GetRGB(const Vector3D& position, const Vector3D& normal, const Vector3D& uvw) override;
};
