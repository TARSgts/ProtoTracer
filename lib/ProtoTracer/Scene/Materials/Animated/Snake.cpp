#include "Snake.h"

#include <Arduino.h>
#include <cmath>

#include "../../../Utils/Math/Mathematics.h"

namespace {
constexpr float kMaxDelta = 0.05f;
constexpr float kMinMoveInterval = 0.12f;
constexpr float kMaxMoveInterval = 0.24f;
constexpr uint16_t kMaxFoodAttempts = 200;
}

SnakeMaterial::SnakeMaterial(Vector2D dimensions, Vector2D center)
    : size(dimensions.Divide(2.0f)),
      offset(center) {
    RecalculateDimensions();
    Reset();
}

void SnakeMaterial::SetSize(Vector2D dimensions) {
    size = dimensions.Divide(2.0f);
    RecalculateDimensions();
    Reset();
}

void SnakeMaterial::SetPosition(Vector2D center) {
    offset = center;
}

void SnakeMaterial::RecalculateDimensions() {
    float width = size.X * 2.0f;
    float height = size.Y * 2.0f;

    uint8_t targetCols = static_cast<uint8_t>(Mathematics::Max(12.0f, Mathematics::Min(28.0f, width / 8.0f)));
    uint8_t targetRows = static_cast<uint8_t>(Mathematics::Max(8.0f, Mathematics::Min(20.0f, height / 8.0f)));
    if (targetCols < 4) targetCols = 4;
    if (targetRows < 4) targetRows = 4;

    gridWidth = targetCols;
    gridHeight = targetRows;

    float cellX = width / static_cast<float>(gridWidth);
    float cellY = height / static_cast<float>(gridHeight);
    cellSize = Mathematics::Min(cellX, cellY);
    if (cellSize < 4.0f) cellSize = 4.0f;

    gridHalfWidth = cellSize * gridWidth * 0.5f;
    gridHalfHeight = cellSize * gridHeight * 0.5f;

    float speedScale = Mathematics::Max(gridWidth, gridHeight) / 20.0f;
    moveInterval = Mathematics::Max(kMinMoveInterval, Mathematics::Min(kMaxMoveInterval, 0.2f / speedScale));
}

void SnakeMaterial::Reset() {
    if (gridWidth == 0 || gridHeight == 0) return;

    snakeLength = 4;
    if (snakeLength > kMaxSegments) snakeLength = kMaxSegments;
    if (snakeLength > gridWidth) snakeLength = gridWidth;
    if (snakeLength < 2) snakeLength = 2;

    int16_t startX = gridWidth / 2;
    int16_t startY = gridHeight / 2;
    int16_t minHeadX = static_cast<int16_t>(snakeLength - 1);
    if (startX < minHeadX) startX = minHeadX;
    if (startX >= gridWidth) startX = gridWidth - 1;
    if (startY >= gridHeight) startY = gridHeight - 1;
    dirX = 1;
    dirY = 0;

    for (uint16_t i = 0; i < snakeLength; ++i) {
        snakeX[i] = startX - static_cast<int16_t>(i);
        snakeY[i] = startY;
    }

    PlaceFood();
    moveTimer = 0.0f;
    lastUpdateMs = millis();
}

bool SnakeMaterial::IsCellOccupied(int16_t x, int16_t y) const {
    for (uint16_t i = 0; i < snakeLength; ++i) {
        if (snakeX[i] == x && snakeY[i] == y) return true;
    }
    return false;
}

void SnakeMaterial::PlaceFood() {
    if (gridWidth == 0 || gridHeight == 0) return;
    for (uint16_t attempt = 0; attempt < kMaxFoodAttempts; ++attempt) {
        int16_t x = static_cast<int16_t>(random(0, gridWidth));
        int16_t y = static_cast<int16_t>(random(0, gridHeight));
        if (!IsCellOccupied(x, y)) {
            foodX = x;
            foodY = y;
            return;
        }
    }

    foodX = gridWidth / 2;
    foodY = gridHeight / 2;
}

bool SnakeMaterial::IsMoveSafe(int16_t x, int16_t y, bool grow) const {
    if (x < 0 || y < 0 || x >= gridWidth || y >= gridHeight) return false;

    for (uint16_t i = 0; i < snakeLength; ++i) {
        if (snakeX[i] == x && snakeY[i] == y) {
            if (!grow && i == snakeLength - 1) {
                continue;
            }
            return false;
        }
    }

    return true;
}

void SnakeMaterial::Step() {
    if (gridWidth < 2 || gridHeight < 2) return;

    int16_t headX = snakeX[0];
    int16_t headY = snakeY[0];
    int8_t bestDX = dirX;
    int8_t bestDY = dirY;
    bool found = false;
    float bestScore = 100000.0f;

    static const int8_t directions[4][2] = {
        {1, 0},
        {-1, 0},
        {0, 1},
        {0, -1}
    };

    for (uint8_t i = 0; i < 4; ++i) {
        int8_t candDX = directions[i][0];
        int8_t candDY = directions[i][1];
        if (snakeLength > 1 && candDX == -dirX && candDY == -dirY) continue;

        int16_t nextX = headX + candDX;
        int16_t nextY = headY + candDY;
        bool grow = (nextX == foodX && nextY == foodY);
        if (!IsMoveSafe(nextX, nextY, grow)) continue;

        float score = fabsf(static_cast<float>(nextX - foodX)) +
                      fabsf(static_cast<float>(nextY - foodY));
        if (score < bestScore) {
            bestScore = score;
            bestDX = candDX;
            bestDY = candDY;
            found = true;
        }
    }

    if (!found) {
        int16_t nextX = headX + dirX;
        int16_t nextY = headY + dirY;
        bool grow = (nextX == foodX && nextY == foodY);
        if (!IsMoveSafe(nextX, nextY, grow)) {
            Reset();
            return;
        }
    }

    dirX = bestDX;
    dirY = bestDY;

    int16_t newHeadX = headX + dirX;
    int16_t newHeadY = headY + dirY;
    bool grow = (newHeadX == foodX && newHeadY == foodY);

    if (!IsMoveSafe(newHeadX, newHeadY, grow)) {
        Reset();
        return;
    }

    uint16_t shiftStart = snakeLength - 1;
    uint16_t newLength = snakeLength;
    if (grow && snakeLength < kMaxSegments) {
        shiftStart = snakeLength;
        newLength = snakeLength + 1;
    }

    for (int32_t i = shiftStart; i > 0; --i) {
        snakeX[i] = snakeX[i - 1];
        snakeY[i] = snakeY[i - 1];
    }

    snakeX[0] = newHeadX;
    snakeY[0] = newHeadY;
    snakeLength = newLength;

    if (grow) {
        PlaceFood();
    }
}

void SnakeMaterial::Update() {
    uint32_t now = millis();
    if (lastUpdateMs == 0) {
        lastUpdateMs = now;
        return;
    }

    float delta = (now - lastUpdateMs) / 1000.0f;
    if (delta > kMaxDelta) delta = kMaxDelta;
    lastUpdateMs = now;

    moveTimer += delta;
    while (moveTimer >= moveInterval) {
        moveTimer -= moveInterval;
        Step();
    }
}

RGBColor SnakeMaterial::GetRGB(const Vector3D& position, const Vector3D& /*normal*/, const Vector3D& /*uvw*/) {
    Vector2D relative(position.X, position.Y);
    relative = relative - offset;

    if (relative.X < -gridHalfWidth || relative.X > gridHalfWidth) return RGBColor();
    if (relative.Y < -gridHalfHeight || relative.Y > gridHalfHeight) return RGBColor();

    float originX = -gridHalfWidth;
    float originY = -gridHalfHeight;
    int16_t col = static_cast<int16_t>(floorf((relative.X - originX) / cellSize));
    int16_t row = static_cast<int16_t>(floorf((relative.Y - originY) / cellSize));
    if (col < 0 || row < 0 || col >= gridWidth || row >= gridHeight) {
        return backgroundColor;
    }

    if (col == foodX && row == foodY) {
        return foodColor;
    }

    if (col == snakeX[0] && row == snakeY[0]) {
        return snakeHeadColor;
    }

    for (uint16_t i = 1; i < snakeLength; ++i) {
        if (col == snakeX[i] && row == snakeY[i]) {
            return snakeColor;
        }
    }

    return backgroundColor;
}
