#include "FlappyBird.h"

#include <Arduino.h>
#include <cmath>

#include "../../../Utils/Math/Mathematics.h"

namespace {
constexpr float kMaxDelta = 0.05f;
constexpr float kFlapCooldownSeconds = 0.18f;
constexpr float kAimBias = -0.12f;
}

FlappyBirdMaterial::FlappyBirdMaterial(Vector2D dimensions, Vector2D center)
    : size(dimensions.Divide(2.0f)),
      offset(center) {
    RecalculateDimensions();
    Reset();
}

void FlappyBirdMaterial::SetSize(Vector2D dimensions) {
    size = dimensions.Divide(2.0f);
    RecalculateDimensions();
}

void FlappyBirdMaterial::SetPosition(Vector2D center) {
    offset = center;
}

void FlappyBirdMaterial::RecalculateDimensions() {
    float minAxis = Mathematics::Min(size.X, size.Y);
    birdRadius = Mathematics::Max(2.0f, minAxis * 0.06f);
    pipeWidth = Mathematics::Max(6.0f, size.X * 0.12f);
    pipeGapHeight = Mathematics::Max(16.0f, size.Y * 0.45f);
    pipeSpacing = Mathematics::Max(pipeWidth * 1.6f, size.X * 0.65f);
    pipeSpeed = Mathematics::Max(35.0f, size.X * 0.6f);
    gravity = -Mathematics::Max(70.0f, size.Y * 1.8f);
    flapVelocity = Mathematics::Max(45.0f, size.Y * 0.95f);
    groundHeight = Mathematics::Max(2.0f, size.Y * 0.08f);
    gapPadding = birdRadius * 0.8f;

    float verticalSpace = size.Y * 2.0f - groundHeight - gapPadding * 2.0f;
    if (verticalSpace < 8.0f) verticalSpace = 8.0f;
    if (pipeGapHeight > verticalSpace) pipeGapHeight = verticalSpace;
}

void FlappyBirdMaterial::ResetPipe(uint8_t index, float xPosition) {
    if (index >= kPipeCount) return;
    pipeX[index] = xPosition;
    pipeGapY[index] = RandomGapCenter();
}

float FlappyBirdMaterial::RandomGapCenter() {
    float minGap = -size.Y + groundHeight + pipeGapHeight * 0.5f + gapPadding;
    float maxGap = size.Y - pipeGapHeight * 0.5f - gapPadding;
    if (maxGap <= minGap) return 0.0f;
    float t = static_cast<float>(random(0, 10000)) / 10000.0f;
    return minGap + t * (maxGap - minGap);
}

void FlappyBirdMaterial::Reset() {
    birdX = -size.X * 0.45f;
    birdY = 0.0f;
    birdVelocity = 0.0f;
    flapCooldown = 0.0f;

    float startX = size.X + pipeWidth;
    for (uint8_t i = 0; i < kPipeCount; ++i) {
        ResetPipe(i, startX + pipeSpacing * i);
    }

    lastUpdateMs = millis();
}

int8_t FlappyBirdMaterial::FindNextPipeIndex() const {
    int8_t nextIndex = -1;
    float nearest = 100000.0f;
    for (uint8_t i = 0; i < kPipeCount; ++i) {
        float pipeFront = pipeX[i] + pipeWidth * 0.5f;
        if (pipeFront >= birdX && pipeFront < nearest) {
            nearest = pipeFront;
            nextIndex = static_cast<int8_t>(i);
        }
    }
    if (nextIndex >= 0) return nextIndex;

    float maxX = pipeX[0];
    nextIndex = 0;
    for (uint8_t i = 1; i < kPipeCount; ++i) {
        if (pipeX[i] > maxX) {
            maxX = pipeX[i];
            nextIndex = static_cast<int8_t>(i);
        }
    }
    return nextIndex;
}

bool FlappyBirdMaterial::CheckCollision() const {
    float topBound = size.Y - birdRadius;
    float bottomBound = -size.Y + groundHeight + birdRadius;
    if (birdY > topBound || birdY < bottomBound) return true;

    for (uint8_t i = 0; i < kPipeCount; ++i) {
        float left = pipeX[i] - pipeWidth * 0.5f;
        float right = pipeX[i] + pipeWidth * 0.5f;
        if (birdX + birdRadius < left || birdX - birdRadius > right) continue;

        float gapTop = pipeGapY[i] + pipeGapHeight * 0.5f;
        float gapBottom = pipeGapY[i] - pipeGapHeight * 0.5f;
        if (birdY + birdRadius > gapTop || birdY - birdRadius < gapBottom) {
            return true;
        }
    }

    return false;
}

void FlappyBirdMaterial::Update() {
    uint32_t now = millis();
    if (lastUpdateMs == 0) {
        lastUpdateMs = now;
        return;
    }

    float delta = (now - lastUpdateMs) / 1000.0f;
    if (delta > kMaxDelta) delta = kMaxDelta;
    lastUpdateMs = now;

    if (flapCooldown > 0.0f) {
        flapCooldown -= delta;
        if (flapCooldown < 0.0f) flapCooldown = 0.0f;
    }

    float maxX = -INFINITY;
    for (uint8_t i = 0; i < kPipeCount; ++i) {
        pipeX[i] -= pipeSpeed * delta;
        if (pipeX[i] > maxX) maxX = pipeX[i];
    }

    for (uint8_t i = 0; i < kPipeCount; ++i) {
        if (pipeX[i] < -size.X - pipeWidth) {
            maxX += pipeSpacing;
            ResetPipe(i, maxX);
        }
    }

    int8_t nextPipe = FindNextPipeIndex();
    if (nextPipe >= 0) {
        float targetY = pipeGapY[nextPipe] + pipeGapHeight * kAimBias;
        if (birdY < targetY - birdRadius * 0.25f && flapCooldown <= 0.0f) {
            birdVelocity = flapVelocity;
            flapCooldown = kFlapCooldownSeconds;
        }
    }

    birdVelocity += gravity * delta;
    float maxFall = -pipeGapHeight * 3.0f;
    if (birdVelocity < maxFall) birdVelocity = maxFall;
    birdY += birdVelocity * delta;

    if (CheckCollision()) {
        Reset();
    }
}

RGBColor FlappyBirdMaterial::GetRGB(const Vector3D& position, const Vector3D& /*normal*/, const Vector3D& /*uvw*/) {
    Vector2D relative(position.X, position.Y);
    relative = relative - offset;

    if (relative.X < -size.X || relative.X > size.X) return RGBColor();
    if (relative.Y < -size.Y || relative.Y > size.Y) return RGBColor();

    float dx = relative.X - birdX;
    float dy = relative.Y - birdY;
    if ((dx * dx + dy * dy) <= (birdRadius * birdRadius)) {
        if (dx > birdRadius * 0.25f && fabsf(dy) < birdRadius * 0.25f) {
            return birdAccent;
        }
        return birdColor;
    }

    for (uint8_t i = 0; i < kPipeCount; ++i) {
        float left = pipeX[i] - pipeWidth * 0.5f;
        float right = pipeX[i] + pipeWidth * 0.5f;
        if (relative.X < left || relative.X > right) continue;

        float gapTop = pipeGapY[i] + pipeGapHeight * 0.5f;
        float gapBottom = pipeGapY[i] - pipeGapHeight * 0.5f;
        if (relative.Y > gapTop || relative.Y < gapBottom) {
            return pipeColor;
        }
    }

    if (relative.Y < -size.Y + groundHeight) {
        return groundColor;
    }

    return skyColor;
}
