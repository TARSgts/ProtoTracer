#include "SpaceInvaders.h"

#include <Arduino.h>
#include <cmath>

#include "../../../Utils/Math/Mathematics.h"

namespace {
constexpr float kMaxDelta = 0.05f;
constexpr float kShotRadius = 1.6f;
constexpr float kInvaderEdgePadding = 2.0f;
}

SpaceInvadersMaterial::SpaceInvadersMaterial(Vector2D dimensions, Vector2D center)
    : size(dimensions.Divide(2.0f)),
      offset(center) {
    ResetWave();
}

void SpaceInvadersMaterial::SetSize(Vector2D dimensions) {
    size = dimensions.Divide(2.0f);
    invaderSize = Mathematics::Max(4.0f, size.X * 0.06f);
    invaderSpacing = Mathematics::Max(3.0f, invaderSize * 0.6f);
    playerWidth = Mathematics::Max(8.0f, invaderSize * 1.5f);
    playerHeight = Mathematics::Max(3.0f, invaderSize * 0.4f);
    playerYOffset = Mathematics::Max(6.0f, size.Y * 0.08f);
    playerSpeed = Mathematics::Max(60.0f, size.X * 0.9f);
    playerShotSpeed = Mathematics::Max(120.0f, size.Y * 1.5f);
    invaderShotSpeed = Mathematics::Max(60.0f, size.Y * 0.9f);
}

void SpaceInvadersMaterial::SetPosition(Vector2D center) {
    offset = center;
}

void SpaceInvadersMaterial::ResetWave() {
    for (uint8_t row = 0; row < kRows; ++row) {
        for (uint8_t col = 0; col < kCols; ++col) {
            alive[row][col] = true;
        }
    }

    invaderDir = 1;
    invaderStep = 0.0f;
    invaderDrop = 0.0f;
    invaderMoveTimer = 0.0f;
    invaderMoveInterval = 0.45f;

    playerX = 0.0f;
    playerShotActive = false;
    invaderShotActive = false;
    invaderShotCooldown = 0.8f;

    SetSize(size.Multiply(2.0f));
}

Vector2D SpaceInvadersMaterial::GetInvaderOrigin() const {
    float gridWidth = kCols * invaderSize + (kCols - 1) * invaderSpacing;
    float gridHeight = kRows * invaderSize + (kRows - 1) * invaderSpacing;
    float startX = -gridWidth * 0.5f + invaderStep;
    float startY = size.Y - invaderMarginTop - gridHeight * 0.5f - invaderDrop;
    return Vector2D(startX, startY);
}

float SpaceInvadersMaterial::GetInvaderRowY(uint8_t row) const {
    Vector2D origin = GetInvaderOrigin();
    return origin.Y - float(row) * (invaderSize + invaderSpacing);
}

float SpaceInvadersMaterial::GetInvaderColX(uint8_t col) const {
    Vector2D origin = GetInvaderOrigin();
    return origin.X + float(col) * (invaderSize + invaderSpacing);
}

void SpaceInvadersMaterial::KillInvader(uint8_t row, uint8_t col) {
    if (row >= kRows || col >= kCols) return;
    alive[row][col] = false;
}

bool SpaceInvadersMaterial::AllInvadersCleared() const {
    for (uint8_t row = 0; row < kRows; ++row) {
        for (uint8_t col = 0; col < kCols; ++col) {
            if (alive[row][col]) return false;
        }
    }
    return true;
}

void SpaceInvadersMaterial::UpdateInvaders(float delta) {
    invaderMoveTimer += delta;
    if (invaderMoveTimer < invaderMoveInterval) return;
    invaderMoveTimer = 0.0f;

    float stepSize = invaderSize * 0.5f + invaderSpacing * 0.5f;
    invaderStep += stepSize * float(invaderDir);

    float leftEdge = GetInvaderColX(0) - invaderSize * 0.5f;
    float rightEdge = GetInvaderColX(kCols - 1) + invaderSize * 0.5f;
    float maxX = size.X - kInvaderEdgePadding;

    if (rightEdge > maxX || leftEdge < -maxX) {
        invaderDir *= -1;
        invaderStep += stepSize * float(invaderDir);
        invaderDrop += invaderSize * 0.6f;
        invaderMoveInterval = Mathematics::Max(0.18f, invaderMoveInterval * 0.96f);
    }
}

void SpaceInvadersMaterial::UpdatePlayer(float delta) {
    float targetX = 0.0f;
    float targetDistance = 100000.0f;

    for (uint8_t col = 0; col < kCols; ++col) {
        for (int8_t row = kRows - 1; row >= 0; --row) {
            if (alive[row][col]) {
                float x = GetInvaderColX(static_cast<uint8_t>(col));
                float dist = fabsf(x - playerX);
                if (dist < targetDistance) {
                    targetDistance = dist;
                    targetX = x;
                }
                break;
            }
        }
    }

    float maxStep = playerSpeed * delta;
    float deltaX = targetX - playerX;
    if (deltaX > maxStep) deltaX = maxStep;
    if (deltaX < -maxStep) deltaX = -maxStep;
    playerX += deltaX;

    float bounds = size.X - playerWidth * 0.6f;
    if (playerX > bounds) playerX = bounds;
    if (playerX < -bounds) playerX = -bounds;

    if (!playerShotActive && fabsf(targetX - playerX) < invaderSize * 0.25f) {
        playerShotActive = true;
        playerShotPos = Vector2D(playerX, -size.Y + playerYOffset + playerHeight);
    }
}

void SpaceInvadersMaterial::TryFireInvader() {
    if (invaderShotActive || invaderShotCooldown > 0.0f) return;

    uint8_t liveCols[kCols];
    uint8_t liveCount = 0;
    for (uint8_t col = 0; col < kCols; ++col) {
        for (int8_t row = kRows - 1; row >= 0; --row) {
            if (alive[row][col]) {
                liveCols[liveCount++] = col;
                break;
            }
        }
    }

    if (liveCount == 0) return;

    uint8_t pick = liveCols[random(0, liveCount)];
    for (int8_t row = kRows - 1; row >= 0; --row) {
        if (alive[row][pick]) {
            invaderShotActive = true;
            invaderShotPos = Vector2D(GetInvaderColX(pick), GetInvaderRowY(static_cast<uint8_t>(row)));
            invaderShotCooldown = 0.8f;
            break;
        }
    }
}

void SpaceInvadersMaterial::UpdateShots(float delta) {
    if (invaderShotCooldown > 0.0f) {
        invaderShotCooldown -= delta;
        if (invaderShotCooldown < 0.0f) invaderShotCooldown = 0.0f;
    }

    if (playerShotActive) {
        playerShotPos.Y += playerShotSpeed * delta;
        if (playerShotPos.Y > size.Y) {
            playerShotActive = false;
        } else {
            for (uint8_t row = 0; row < kRows; ++row) {
                for (uint8_t col = 0; col < kCols; ++col) {
                    if (!alive[row][col]) continue;
                    float x = GetInvaderColX(col);
                    float y = GetInvaderRowY(row);
                    if (fabsf(playerShotPos.X - x) <= invaderSize * 0.5f &&
                        fabsf(playerShotPos.Y - y) <= invaderSize * 0.5f) {
                        KillInvader(row, col);
                        playerShotActive = false;
                        break;
                    }
                }
                if (!playerShotActive) break;
            }
        }
    }

    if (!invaderShotActive) {
        TryFireInvader();
    } else {
        invaderShotPos.Y -= invaderShotSpeed * delta;
        float playerY = -size.Y + playerYOffset;
        if (invaderShotPos.Y < -size.Y) {
            invaderShotActive = false;
        } else if (fabsf(invaderShotPos.X - playerX) <= playerWidth * 0.5f &&
                   fabsf(invaderShotPos.Y - playerY) <= playerHeight * 1.2f) {
            invaderShotActive = false;
        }
    }
}

void SpaceInvadersMaterial::Update() {
    uint32_t now = millis();
    if (lastUpdateMs == 0) {
        lastUpdateMs = now;
        return;
    }

    float delta = (now - lastUpdateMs) / 1000.0f;
    if (delta > kMaxDelta) delta = kMaxDelta;
    lastUpdateMs = now;

    UpdateInvaders(delta);
    UpdatePlayer(delta);
    UpdateShots(delta);

    if (AllInvadersCleared()) {
        ResetWave();
    }
}

RGBColor SpaceInvadersMaterial::GetRGB(const Vector3D& position, const Vector3D& /*normal*/, const Vector3D& /*uvw*/) {
    Vector2D relative(position.X, position.Y);
    relative = relative - offset;

    if (relative.X < -size.X || relative.X > size.X) return RGBColor();
    if (relative.Y < -size.Y || relative.Y > size.Y) return RGBColor();

    for (uint8_t row = 0; row < kRows; ++row) {
        for (uint8_t col = 0; col < kCols; ++col) {
            if (!alive[row][col]) continue;
            float x = GetInvaderColX(col);
            float y = GetInvaderRowY(row);
            if (fabsf(relative.X - x) <= invaderSize * 0.5f &&
                fabsf(relative.Y - y) <= invaderSize * 0.5f) {
                return invaderColor;
            }
        }
    }

    float playerY = -size.Y + playerYOffset;
    if (fabsf(relative.X - playerX) <= playerWidth * 0.5f &&
        fabsf(relative.Y - playerY) <= playerHeight * 0.5f) {
        return playerColor;
    }

    if (playerShotActive) {
        float dx = relative.X - playerShotPos.X;
        float dy = relative.Y - playerShotPos.Y;
        if ((dx * dx + dy * dy) <= (kShotRadius * kShotRadius)) {
            return shotColor;
        }
    }

    if (invaderShotActive) {
        float dx = relative.X - invaderShotPos.X;
        float dy = relative.Y - invaderShotPos.Y;
        if ((dx * dx + dy * dy) <= (kShotRadius * kShotRadius)) {
            return shotColor;
        }
    }

    return backgroundColor;
}
