#pragma once

#include "../Material.h"
#include "../../../Utils/Math/Vector2D.h"

class SlotMachineMaterial : public Material {
private:
    static constexpr uint8_t kReels = 3;
    static constexpr uint8_t kSymbols = 6;

    Vector2D size;
    Vector2D offset;

    float reelOffset[kReels] = {0.0f, 0.0f, 0.0f};
    float reelSpeed[kReels] = {0.0f, 0.0f, 0.0f};
    bool snapPending[kReels] = {false, false, false};
    bool reelStopping[kReels] = {false, false, false};
    bool reelStopped[kReels] = {false, false, false};

    float baseSpeed = 2.5f;
    float spinSpeed = 9.5f;
    float spinTimer = 0.0f;
    float spinDuration = 2.0f;
    float idlePause = 5.0f;
    bool spinning = true;
    bool stopping = false;
    uint32_t stopStartMs = 0;
    float stopDelay = 1.0f;
    float stopMinSpeed = 0.35f;
    float stopDamp = 9.0f;

    uint32_t lastUpdateMs = 0;

    float cellWidth = 12.0f;
    float cellHeight = 12.0f;
    float gridHalfW = 0.0f;
    float gridHalfH = 0.0f;
    float gutter = 2.5f;
    float borderThickness = 0.08f;

    bool confettiActive = false;
    float confettiTimer = 0.0f;
    float confettiDuration = 1.5f;

    RGBColor backgroundColor = RGBColor(0, 0, 0);
    RGBColor frameColor = RGBColor(205, 205, 215);
    RGBColor symbolColors[kSymbols] = {
        RGBColor(240, 210, 60),   // gold (bar)
        RGBColor(200, 60, 60),    // cherry
        RGBColor(240, 90, 90),    // seven
        RGBColor(100, 220, 255),  // diamond
        RGBColor(255, 180, 60),   // bell
        RGBColor(255, 220, 100)   // star
    };

    void RecalculateDimensions();
    void ResetSpin();
    uint8_t GetSymbol(uint8_t reelIndex, int8_t row) const;

public:
    SlotMachineMaterial(Vector2D dimensions, Vector2D center);

    void SetSize(Vector2D dimensions);
    void SetPosition(Vector2D center);
    void Update();

    RGBColor GetRGB(const Vector3D& position, const Vector3D& normal, const Vector3D& uvw) override;
};
