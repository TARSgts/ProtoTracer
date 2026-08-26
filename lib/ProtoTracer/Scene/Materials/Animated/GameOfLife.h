#pragma once

#include "../Material.h"
#include "../../../Utils/Math/Vector2D.h"

class GameOfLifeMaterial : public Material {
private:
    static constexpr uint8_t kMaxCols = 32;
    static constexpr uint8_t kMaxRows = 24;
    static constexpr uint8_t kHashHistory = 32;

    Vector2D size;
    Vector2D offset;
    uint8_t gridWidth = 0;
    uint8_t gridHeight = 0;
    float cellSize = 6.0f;
    float gridHalfWidth = 0.0f;
    float gridHalfHeight = 0.0f;

    uint8_t cells[kMaxRows][kMaxCols];
    uint8_t nextCells[kMaxRows][kMaxCols];

    float stepTimer = 0.0f;
    float stepInterval = 0.22f;
    uint32_t lastUpdateMs = 0;
    uint32_t lastChangeMs = 0;
    uint32_t history[kHashHistory] = {};
    uint8_t historyCount = 0;
    uint8_t historyIndex = 0;

    RGBColor backgroundColor = RGBColor(0, 0, 0);
    RGBColor aliveColor = RGBColor(40, 220, 120);

    void RecalculateDimensions();
    void Reset();
    void ClearGrid();
    void SeedGlider();
    uint32_t ComputeHash() const;
    bool IsHashInHistory(uint32_t hash) const;
    void RecordHash(uint32_t hash);
    uint8_t CountNeighbors(int16_t x, int16_t y) const;
    void Step();

public:
    GameOfLifeMaterial(Vector2D dimensions, Vector2D center);

    void SetSize(Vector2D dimensions);
    void SetPosition(Vector2D center);
    void Update();

    RGBColor GetRGB(const Vector3D& position, const Vector3D& normal, const Vector3D& uvw) override;
};
