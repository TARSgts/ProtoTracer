#include "SlotMachine.h"

#include <Arduino.h>
#include <cmath>

#include "../../../Utils/Math/Mathematics.h"

namespace {
constexpr float kMaxDelta = 0.05f;
constexpr float kSnapThreshold = 0.1f;
constexpr float kConfettiInterval = 0.02f;

inline float Clamp01(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

inline bool InCircle(float x, float y, float r) {
    return (x * x + y * y) <= (r * r);
}

inline bool InRect(float x, float y, float hw, float hh) {
    return fabsf(x) <= hw && fabsf(y) <= hh;
}

RGBColor DrawSymbol(uint8_t symbol, float u, float v, const RGBColor* palette) {
    // u, v are normalized to [-1, 1] inside the cell (0,0 = center)
    switch(symbol % 6) {
        case 0: { // BAR (gold)
            if (InRect(u, v, 0.55f, 0.22f)) {
                float shade = 0.7f + 0.3f * (1.0f - (v + 1.0f) * 0.5f);
                RGBColor c = palette[0];
                return RGBColor(uint8_t(c.R * shade), uint8_t(c.G * shade), uint8_t(c.B * shade));
            }
            break;
        }
        case 1: { // CHERRY (two orbs + stem)
            float stemX = u + 0.15f;
            float stemY = v + 0.2f;
            if (InRect(stemX, stemY, 0.05f, 0.25f)) return RGBColor(90, 200, 90);
            if (InRect(stemX - 0.08f, stemY - 0.12f, 0.05f, 0.16f)) return RGBColor(90, 200, 90);
            if (InCircle(u - 0.15f, v - 0.1f, 0.25f)) return palette[1];
            if (InCircle(u + 0.1f, v - 0.05f, 0.22f)) return palette[1];
            break;
        }
        case 2: { // SEVEN
            if (InRect(u, v + 0.4f, 0.55f, 0.1f)) return palette[2];
            if (InRect(u + 0.15f, v, 0.12f, 0.6f)) return palette[2];
            if (InRect(u - 0.05f, v - 0.05f, 0.12f, 0.45f)) return palette[2];
            break;
        }
        case 3: { // DIAMOND
            float d = fabsf(u) + fabsf(v * 0.9f);
            if (d <= 0.9f) {
                float t = 1.0f - d / 0.9f;
                RGBColor c = palette[3];
                return RGBColor(uint8_t(c.R * (0.7f + 0.3f * t)), uint8_t(c.G * (0.7f + 0.3f * t)), uint8_t(c.B * (0.7f + 0.3f * t)));
            }
            break;
        }
        case 4: { // BELL
            if (InCircle(u, v - 0.05f, 0.5f)) return palette[4];
            if (InRect(u, v + 0.45f, 0.25f, 0.12f)) return palette[4];
            if (InRect(u, v + 0.62f, 0.12f, 0.06f)) return palette[4];
            break;
        }
        case 5: { // STAR / SHERIFF BADGE
            if (InRect(u, v, 0.16f, 0.7f)) return palette[5];
            if (InRect(u, v, 0.7f, 0.16f)) return palette[5];
            if (InRect(u + v, u - v, 0.4f, 0.1f)) return palette[5];
            if (InRect(u - v, -(u + v), 0.4f, 0.1f)) return palette[5];
            break;
        }
        default: break;
    }
    return RGBColor();
}
}

SlotMachineMaterial::SlotMachineMaterial(Vector2D dimensions, Vector2D center)
    : size(dimensions.Divide(2.0f)),
      offset(center) {
    RecalculateDimensions();
    ResetSpin();
}

void SlotMachineMaterial::SetSize(Vector2D dimensions) {
    size = dimensions.Divide(2.0f);
    RecalculateDimensions();
}

void SlotMachineMaterial::SetPosition(Vector2D center) {
    offset = center;
}

void SlotMachineMaterial::RecalculateDimensions() {
    float innerW = size.X * 2.0f - gutter * 2.0f;
    float innerH = size.Y * 2.0f - gutter * 2.0f;

    cellWidth = innerW / static_cast<float>(kReels);
    cellHeight = innerH / 3.0f;
    gridHalfW = innerW * 0.5f;
    gridHalfH = innerH * 0.5f;

    float scale = Mathematics::Max(0.7f, Mathematics::Min(1.3f, innerH / 60.0f));
    spinSpeed = 9.5f * scale;
    baseSpeed = 2.5f * scale;
    idlePause = 5.0f;
}

void SlotMachineMaterial::ResetSpin() {
    spinning = true;
    stopping = false;
    spinTimer = 0.0f;
    spinDuration = 1.8f + static_cast<float>(random(0, 80)) / 100.0f; // 1.8 - 2.6s
    confettiActive = false;
    confettiTimer = 0.0f;

    for (uint8_t i = 0; i < kReels; ++i) {
        reelSpeed[i] = spinSpeed * (1.0f - i * 0.15f);
        snapPending[i] = false;
        reelStopping[i] = false;
        reelStopped[i] = false;
    }

    lastUpdateMs = millis();
}

uint8_t SlotMachineMaterial::GetSymbol(uint8_t reelIndex, int8_t row) const {
    float pos = reelOffset[reelIndex] + static_cast<float>(row);
    int32_t idx = static_cast<int32_t>(floorf(pos));
    idx %= kSymbols;
    if (idx < 0) idx += kSymbols;
    return static_cast<uint8_t>(idx);
}

void SlotMachineMaterial::Update() {
    uint32_t now = millis();
    if (lastUpdateMs == 0) {
        lastUpdateMs = now;
        return;
    }

    float delta = (now - lastUpdateMs) / 1000.0f;
    if (delta > kMaxDelta) delta = kMaxDelta;
    lastUpdateMs = now;

    if (spinning) {
        spinTimer += delta;
        if (spinTimer >= spinDuration) {
            // initiate staggered stop
            spinning = false;
            stopping = true;
            spinTimer = 0.0f;
            stopStartMs = now;
            confettiActive = false;
            confettiTimer = 0.0f;
        }
    } else if (!stopping) {
        spinTimer += delta;
        if (spinTimer >= idlePause) {
            ResetSpin();
        }
    }

    if (confettiActive) {
        confettiTimer -= delta;
        if (confettiTimer <= 0.0f) {
            confettiActive = false;
            confettiTimer = 0.0f;
        }
    }

    bool allStopped = true;
    for (uint8_t i = 0; i < kReels; ++i) {
        float targetSpeed = spinning ? reelSpeed[i] : reelSpeed[i];
        reelOffset[i] += targetSpeed * delta;
        if (reelOffset[i] > 10000.0f) reelOffset[i] = fmodf(reelOffset[i], static_cast<float>(kSymbols));

        if (stopping && !reelStopped[i]) {
            uint32_t reelStart = stopStartMs + static_cast<uint32_t>(i * (stopDelay * 1000.0f));
            if (now >= reelStart) {
                reelStopping[i] = true;
            }

            if (reelStopping[i]) {
                // exponential damp toward zero
                reelSpeed[i] -= reelSpeed[i] * stopDamp * delta;
                if (reelSpeed[i] < stopMinSpeed) {
                    reelSpeed[i] = 0.0f;
                    snapPending[i] = true;
                }
            }
        }

        if (snapPending[i]) {
            reelOffset[i] = roundf(reelOffset[i]);
            reelSpeed[i] = 0.0f;
            reelStopped[i] = true;
            snapPending[i] = false;
        }

        allStopped &= reelStopped[i];
    }

    if (stopping && allStopped) {
        stopping = false;
        spinTimer = 0.0f;
        uint8_t mid0 = GetSymbol(0, 0);
        uint8_t mid1 = GetSymbol(1, 0);
        uint8_t mid2 = GetSymbol(2, 0);
        bool matched = (mid0 == mid1 && mid1 == mid2);
        bool forcedWin = false;
        if (!matched && random(10) == 0) {
            uint8_t winSymbol = static_cast<uint8_t>(random(kSymbols));
            for (uint8_t i = 0; i < kReels; ++i) {
                reelOffset[i] = static_cast<float>(winSymbol);
                reelStopped[i] = true;
            }
            mid0 = mid1 = mid2 = winSymbol;
            forcedWin = true;
        }
        confettiActive = matched || forcedWin;
        confettiTimer = confettiActive ? confettiDuration : 0.0f;
    }
}

RGBColor SlotMachineMaterial::GetRGB(const Vector3D& position, const Vector3D& /*normal*/, const Vector3D& /*uvw*/) {
    Vector2D relative(position.X, position.Y);
    relative = relative - offset;

    if (relative.X < -size.X || relative.X > size.X) return RGBColor();
    if (relative.Y < -size.Y || relative.Y > size.Y) return RGBColor();

    float innerLeft = -gridHalfW;
    float innerBottom = -gridHalfH;

    // frame border
    if (fabsf(relative.X) > gridHalfW + gutter * 0.5f || fabsf(relative.Y) > gridHalfH + gutter * 0.5f) {
        return backgroundColor;
    }
    if (fabsf(relative.X) > gridHalfW || fabsf(relative.Y) > gridHalfH) {
        return frameColor;
    }

    // reel background gradient
    float reelV = (relative.Y + gridHalfH) / (gridHalfH * 2.0f); // 0..1
    // background around symbols is black per request
    RGBColor reelColor = backgroundColor;

    // determine cell
    float localX = relative.X - innerLeft;
    float localY = relative.Y - innerBottom;
    uint8_t col = static_cast<uint8_t>(localX / cellWidth);
    uint8_t row = static_cast<uint8_t>(localY / cellHeight);
    if (col >= kReels) col = kReels - 1;
    if (row >= 3) row = 2;

    // borders between reels/rows
    float colBoundary = fmodf(localX, cellWidth);
    float rowBoundary = fmodf(localY, cellHeight);
    float borderPx = cellWidth * borderThickness;
    float borderPy = cellHeight * borderThickness;
    if (colBoundary <= borderPx || (cellWidth - colBoundary) <= borderPx ||
        rowBoundary <= borderPy || (cellHeight - rowBoundary) <= borderPy) {
        return frameColor;
    }

    // local coords normalized to [-1,1] inside cell
    float cellCenterX = innerLeft + (col + 0.5f) * cellWidth;
    float cellCenterY = innerBottom + (row + 0.5f) * cellHeight;
    float minAxis = Mathematics::Min(cellWidth, cellHeight);
    float u = (relative.X - cellCenterX) / (minAxis * 0.5f);
    float v = (relative.Y - cellCenterY) / (minAxis * 0.5f);

    RGBColor symbol = DrawSymbol(GetSymbol(col, static_cast<int8_t>(row - 1)), u, v, symbolColors);
    if (symbol.R | symbol.G | symbol.B) return symbol;

    // confetti overlay on win
    if (confettiActive) {
        // sprinkle using a quick hash on position + time
        uint32_t t = millis();
        uint32_t seed = static_cast<uint32_t>((int32_t)(relative.X * 7.0f) * 73856093u) ^
                        static_cast<uint32_t>((int32_t)(relative.Y * 13.0f) * 19349663u) ^
                        (t * 2654435761u);
        float flash = ((seed >> 24) & 0xFF) / 255.0f;
        if (flash > 0.82f) {
            uint8_t idx = seed & 5u;
            RGBColor c = symbolColors[idx];
            float alpha = Clamp01(confettiTimer / confettiDuration);
            return RGBColor(uint8_t(c.R * alpha), uint8_t(c.G * alpha), uint8_t(c.B * alpha));
        }
    }

    return reelColor;
}
