/**
 * @file GameOfLifeEffect.h
 * @brief Defines a pixel-perfect Conway's Game of Life effect.
 *
 * @date 22/12/2024
 * @author Coela Can't
 */

#pragma once

#include "Effect.h"
#include "../../Utils/RGBColor.h"

class GameOfLifeEffect : public Effect {
private:
    static constexpr uint8_t kMaxContexts = 4;
    static constexpr uint8_t kSpawnChancePercent = 28;
    static constexpr float kMaxDelta = 0.05f;
    static constexpr float kMinStepInterval = 0.12f;
    static constexpr float kMaxStepInterval = 0.35f;
    static constexpr uint32_t kStableResetMs = 10000;
    static constexpr uint8_t kHashHistory = 32;

    struct Context {
        IPixelGroup* group = nullptr;
        uint16_t pixelCount = 0;
        uint16_t bytes = 0;
        uint8_t* cells = nullptr;
        uint8_t* nextCells = nullptr;
        uint32_t lastUpdateMs = 0;
        uint32_t lastChangeMs = 0;
        uint32_t history[kHashHistory] = {};
        uint8_t historyCount = 0;
        uint8_t historyIndex = 0;
        float stepTimer = 0.0f;
        bool seeded = false;
    };

    Context contexts[kMaxContexts];
    float stepInterval = 0.22f;
    RGBColor backgroundColor = RGBColor(0, 0, 0);
    RGBColor aliveColor = RGBColor(40, 220, 120);

    Context* GetContext(IPixelGroup* pixelGroup);
    void AllocateContext(Context* context, uint16_t pixelCount);
    void ClearContext(Context* context);
    void ResetContext(Context* context, IPixelGroup* pixelGroup);
    bool GetCell(const Context* context, uint16_t index) const;
    void SetCell(Context* context, uint16_t index, bool value);
    uint32_t ComputeHash(const Context* context) const;
    bool IsHashInHistory(const Context* context, uint32_t hash) const;
    void RecordHash(Context* context, uint32_t hash);
    uint8_t CountNeighbors(const Context* context, IPixelGroup* pixelGroup, uint16_t index) const;
    void Step(Context* context, IPixelGroup* pixelGroup);
    void Update(Context* context, IPixelGroup* pixelGroup);

public:
    GameOfLifeEffect() = default;
    ~GameOfLifeEffect();

    void SetColors(RGBColor alive, RGBColor background);
    void SetStepInterval(float seconds);
    void ApplyEffect(IPixelGroup* pixelGroup) override;
};
