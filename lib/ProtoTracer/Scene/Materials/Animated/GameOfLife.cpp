#include "GameOfLife.h"

#include <Arduino.h>
#include <cmath>
#include <cstring>

#include "../../../Utils/Math/Mathematics.h"

namespace {
constexpr float kMaxDelta = 0.05f;
constexpr float kTargetCellSize = 6.0f;
constexpr uint8_t kMinCols = 8;
constexpr uint8_t kMinRows = 6;
constexpr float kMinStepInterval = 0.12f;
constexpr float kMaxStepInterval = 0.35f;
constexpr uint8_t kSpawnChancePercent = 28;
constexpr uint32_t kStableResetMs = 10000;
}

GameOfLifeMaterial::GameOfLifeMaterial(Vector2D dimensions, Vector2D center)
    : size(dimensions.Divide(2.0f)),
      offset(center) {
    ClearGrid();
    RecalculateDimensions();
    Reset();
}

void GameOfLifeMaterial::SetSize(Vector2D dimensions) {
    size = dimensions.Divide(2.0f);
    RecalculateDimensions();
    Reset();
}

void GameOfLifeMaterial::SetPosition(Vector2D center) {
    offset = center;
}

void GameOfLifeMaterial::ClearGrid() {
    for (uint8_t y = 0; y < kMaxRows; ++y) {
        for (uint8_t x = 0; x < kMaxCols; ++x) {
            cells[y][x] = 0;
            nextCells[y][x] = 0;
        }
    }
}

void GameOfLifeMaterial::SeedGlider() {
    if (gridWidth == 0 || gridHeight == 0) {
        return;
    }

    ClearGrid();

    int16_t cx = gridWidth / 2;
    int16_t cy = gridHeight / 2;

    if (gridWidth >= 3 && gridHeight >= 3 && cx + 2 < gridWidth && cy + 2 < gridHeight) {
        cells[cy][cx + 1] = 1;
        cells[cy + 1][cx + 2] = 1;
        cells[cy + 2][cx] = 1;
        cells[cy + 2][cx + 1] = 1;
        cells[cy + 2][cx + 2] = 1;
    } else if (gridWidth >= 3 && gridHeight >= 3 && cx >= 2 && cy >= 2) {
        cells[cy][cx - 1] = 1;
        cells[cy - 1][cx] = 1;
        cells[cy - 2][cx - 2] = 1;
        cells[cy - 2][cx - 1] = 1;
        cells[cy - 2][cx] = 1;
    } else {
        cells[cy][cx] = 1;
    }
}

void GameOfLifeMaterial::RecalculateDimensions() {
    float width = size.X * 2.0f;
    float height = size.Y * 2.0f;

    float idealCols = width / kTargetCellSize;
    idealCols = Mathematics::Max(idealCols, static_cast<float>(kMinCols));
    idealCols = Mathematics::Min(idealCols, static_cast<float>(kMaxCols));
    uint8_t targetCols = static_cast<uint8_t>(idealCols);
    if (targetCols < 1) targetCols = 1;

    cellSize = floorf(width / static_cast<float>(targetCols));
    if (cellSize < 1.0f) {
        cellSize = 1.0f;
    }

    gridWidth = static_cast<uint8_t>(Mathematics::Min(static_cast<float>(kMaxCols), floorf(width / cellSize)));
    gridHeight = static_cast<uint8_t>(Mathematics::Min(static_cast<float>(kMaxRows), floorf(height / cellSize)));
    if (gridWidth < kMinCols) gridWidth = kMinCols;
    if (gridHeight < kMinRows) gridHeight = kMinRows;

    gridHalfWidth = cellSize * static_cast<float>(gridWidth) * 0.5f;
    gridHalfHeight = cellSize * static_cast<float>(gridHeight) * 0.5f;
    cellPadding = 0.0f;

    float maxDimension = Mathematics::Max(static_cast<float>(gridWidth), static_cast<float>(gridHeight));
    float densityScale = maxDimension / 18.0f;
    stepInterval = Mathematics::Max(kMinStepInterval,
                                    Mathematics::Min(kMaxStepInterval, 0.18f * densityScale));
}

void GameOfLifeMaterial::Reset() {
    if (gridWidth == 0 || gridHeight == 0) return;

    uint16_t population = 0;
    for (uint8_t y = 0; y < gridHeight; ++y) {
        for (uint8_t x = 0; x < gridWidth; ++x) {
            bool alive = (random(0, 100) < kSpawnChancePercent);
            cells[y][x] = alive ? 1 : 0;
            if (alive) {
                ++population;
            }
        }
    }

    if (population < 3) {
        SeedGlider();
    }

    stepTimer = 0.0f;
    lastUpdateMs = millis();
    lastChangeMs = lastUpdateMs;
    historyCount = 0;
    historyIndex = 0;
    memset(history, 0, sizeof(history));
    RecordHash(ComputeHash());
}

uint32_t GameOfLifeMaterial::ComputeHash() const {
    if (gridWidth == 0 || gridHeight == 0) {
        return 0;
    }

    uint32_t hash = 2166136261u;
    for (uint8_t y = 0; y < gridHeight; ++y) {
        for (uint8_t x = 0; x < gridWidth; ++x) {
            hash ^= cells[y][x];
            hash *= 16777619u;
        }
    }

    return hash;
}

bool GameOfLifeMaterial::IsHashInHistory(uint32_t hash) const {
    if (historyCount == 0) {
        return false;
    }

    for (uint8_t i = 0; i < historyCount; ++i) {
        if (history[i] == hash) {
            return true;
        }
    }

    return false;
}

void GameOfLifeMaterial::RecordHash(uint32_t hash) {
    history[historyIndex] = hash;
    historyIndex = static_cast<uint8_t>((historyIndex + 1) % kHashHistory);
    if (historyCount < kHashHistory) {
        historyCount++;
    }
}

uint8_t GameOfLifeMaterial::CountNeighbors(int16_t x, int16_t y) const {
    uint8_t count = 0;
    for (int8_t dy = -1; dy <= 1; ++dy) {
        for (int8_t dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            int16_t nx = x + dx;
            int16_t ny = y + dy;

            if (wrapEdges) {
                if (nx < 0) nx += gridWidth;
                if (ny < 0) ny += gridHeight;
                if (nx >= gridWidth) nx -= gridWidth;
                if (ny >= gridHeight) ny -= gridHeight;
            } else {
                if (nx < 0 || ny < 0 || nx >= gridWidth || ny >= gridHeight) {
                    continue;
                }
            }

            count += (cells[ny][nx] != 0) ? 1 : 0;
        }
    }
    return count;
}

void GameOfLifeMaterial::Step() {
    if (gridWidth < 2 || gridHeight < 2) return;

    for (uint8_t y = 0; y < gridHeight; ++y) {
        for (uint8_t x = 0; x < gridWidth; ++x) {
            uint8_t neighbors = CountNeighbors(x, y);
            bool alive = (cells[y][x] != 0);
            bool nextAlive = (neighbors == 3) || (alive && neighbors == 2);
            nextCells[y][x] = nextAlive ? 1 : 0;
        }
    }

    for (uint8_t y = 0; y < gridHeight; ++y) {
        for (uint8_t x = 0; x < gridWidth; ++x) {
            cells[y][x] = nextCells[y][x];
        }
    }

    uint32_t now = millis();
    uint32_t hash = ComputeHash();
    bool isRepeat = IsHashInHistory(hash);
    if (!isRepeat) {
        lastChangeMs = now;
    }

    RecordHash(hash);

    if (now - lastChangeMs >= kStableResetMs) {
        Reset();
    }
}

void GameOfLifeMaterial::Update() {
    uint32_t now = millis();
    if (lastUpdateMs == 0) {
        lastUpdateMs = now;
        return;
    }

    float delta = (now - lastUpdateMs) / 1000.0f;
    if (delta > kMaxDelta) delta = kMaxDelta;
    lastUpdateMs = now;

    stepTimer += delta;
    while (stepTimer >= stepInterval) {
        stepTimer -= stepInterval;
        Step();
    }
}

RGBColor GameOfLifeMaterial::GetRGB(const Vector3D& position, const Vector3D& /*normal*/, const Vector3D& /*uvw*/) {
    Vector2D relative(position.X, position.Y);
    relative = relative - offset;

    if (relative.X < -gridHalfWidth || relative.X > gridHalfWidth) return backgroundColor;
    if (relative.Y < -gridHalfHeight || relative.Y > gridHalfHeight) return backgroundColor;

    float originX = -gridHalfWidth;
    float originY = -gridHalfHeight;
    int16_t col = static_cast<int16_t>(floorf((relative.X - originX) / cellSize));
    int16_t row = static_cast<int16_t>(floorf((relative.Y - originY) / cellSize));
    if (col < 0 || row < 0 || col >= gridWidth || row >= gridHeight) {
        return backgroundColor;
    }

    float localX = (relative.X - originX) - static_cast<float>(col) * cellSize;
    float localY = (relative.Y - originY) - static_cast<float>(row) * cellSize;
    if (localX < cellPadding || localX > (cellSize - cellPadding)) return backgroundColor;
    if (localY < cellPadding || localY > (cellSize - cellPadding)) return backgroundColor;

    return cells[row][col] ? aliveColor : backgroundColor;
}
