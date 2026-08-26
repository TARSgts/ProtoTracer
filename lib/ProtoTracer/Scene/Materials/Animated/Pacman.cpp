#include "Pacman.h"

#include <Arduino.h>
#include <cmath>

#include "../../../Utils/Math/Mathematics.h"

namespace {
constexpr float kMaxDelta = 0.05f;
constexpr float kMinMouthAngle = 0.18f;
constexpr float kMaxMouthAngle = 0.45f;
}

PacmanMaterial::PacmanMaterial(Vector2D dimensions, Vector2D center)
    : size(dimensions.Divide(2.0f)),
      offset(center) {
    RecalculateDimensions();
    Reset();
}

void PacmanMaterial::SetSize(Vector2D dimensions) {
    size = dimensions.Divide(2.0f);
    RecalculateDimensions();
    Reset();
}

void PacmanMaterial::SetPosition(Vector2D center) {
    offset = center;
}

void PacmanMaterial::RecalculateDimensions() {
    float minAxis = Mathematics::Min(size.X, size.Y);
    pacmanRadius = Mathematics::Max(2.0f, minAxis * 0.08f);
    ghostRadius = pacmanRadius * 0.9f;
    pelletRadius = Mathematics::Max(1.0f, pacmanRadius * 0.25f);
    pelletSpacing = pacmanRadius * 2.6f;
    pacmanSpeed = Mathematics::Max(30.0f, size.X * 0.7f);
    ghostSpeed = pacmanSpeed * 0.85f;
    ghostSpacing = pacmanRadius * 5.0f;
    mouthSpeed = 6.0f;

    leftBound = -size.X + pacmanRadius;
    rightBound = size.X - pacmanRadius;

    SetupPellets();
}

void PacmanMaterial::SetupPellets() {
    float available = rightBound - leftBound;
    if (available <= pelletSpacing) {
        pelletCount = 0;
        return;
    }
    float countF = Mathematics::Min(available / pelletSpacing, static_cast<float>(kMaxPellets));
    pelletCount = static_cast<uint8_t>(countF);

    float startX = leftBound + pacmanRadius;
    for (uint8_t i = 0; i < pelletCount; ++i) {
        pelletX[i] = startX + pelletSpacing * i;
    }
    ResetPellets();
}

void PacmanMaterial::ResetPellets() {
    for (uint8_t i = 0; i < pelletCount; ++i) {
        pelletActive[i] = true;
    }
}

bool PacmanMaterial::AllPelletsEaten() const {
    for (uint8_t i = 0; i < pelletCount; ++i) {
        if (pelletActive[i]) return false;
    }
    return true;
}

void PacmanMaterial::Reset() {
    pacmanX = leftBound - pacmanRadius;
    pacmanY = 0.0f;
    ghostX = pacmanX + ghostSpacing;
    ghostY = 0.0f;
    mouthPhase = 0.0f;
    ResetPellets();
    lastUpdateMs = millis();
}

void PacmanMaterial::Update() {
    uint32_t now = millis();
    if (lastUpdateMs == 0) {
        lastUpdateMs = now;
        return;
    }

    float delta = (now - lastUpdateMs) / 1000.0f;
    if (delta > kMaxDelta) delta = kMaxDelta;
    lastUpdateMs = now;

    pacmanX += pacmanSpeed * delta;
    ghostX += ghostSpeed * delta;
    mouthPhase += mouthSpeed * delta;

    if (pacmanX > rightBound + pacmanRadius) {
        Reset();
        return;
    }

    float catchDistance = pacmanRadius * 0.4f + ghostRadius * 0.6f;
    if (pacmanX >= ghostX - catchDistance) {
        Reset();
        return;
    }

    for (uint8_t i = 0; i < pelletCount; ++i) {
        if (!pelletActive[i]) continue;
        if (fabsf(pacmanX - pelletX[i]) <= pacmanRadius * 0.6f) {
            pelletActive[i] = false;
        }
    }

    if (AllPelletsEaten()) {
        Reset();
    }
}

RGBColor PacmanMaterial::GetRGB(const Vector3D& position, const Vector3D& /*normal*/, const Vector3D& /*uvw*/) {
    Vector2D relative(position.X, position.Y);
    relative = relative - offset;

    if (relative.X < -size.X || relative.X > size.X) return RGBColor();
    if (relative.Y < -size.Y || relative.Y > size.Y) return RGBColor();

    float dxP = relative.X - pacmanX;
    float dyP = relative.Y - pacmanY;
    float pacmanDist2 = dxP * dxP + dyP * dyP;
    if (pacmanDist2 <= pacmanRadius * pacmanRadius) {
        float mouthAnim = 0.5f + 0.5f * sinf(mouthPhase);
        float mouthAngle = (kMinMouthAngle + (kMaxMouthAngle - kMinMouthAngle) * mouthAnim) * 3.1415926f;
        if (dxP > 0.0f) {
            float angle = fabsf(atan2f(dyP, dxP));
            if (angle < mouthAngle) {
                return backgroundColor;
            }
        }
        return pacmanColor;
    }

    float dxG = relative.X - ghostX;
    float dyG = relative.Y - ghostY;
    float ghostDist2 = dxG * dxG + dyG * dyG;
    if (ghostDist2 <= ghostRadius * ghostRadius ||
        (fabsf(dxG) <= ghostRadius && dyG >= -ghostRadius && dyG <= ghostRadius * 0.6f)) {
        return ghostColor;
    }

    for (uint8_t i = 0; i < pelletCount; ++i) {
        if (!pelletActive[i]) continue;
        float dx = relative.X - pelletX[i];
        float dy = relative.Y - pacmanY;
        if (dx * dx + dy * dy <= pelletRadius * pelletRadius) {
            return pelletColor;
        }
    }

    return backgroundColor;
}
