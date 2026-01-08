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

// Sprite data (12x12 indexed) for slot symbols
struct SpriteData {
    uint8_t width;
    uint8_t height;
    const uint8_t* mem;
    const uint8_t* pal;
    float scale;
    bool transparentZero;
};

// BAR (15x7 sprite from BarPixelSprite)
static const uint8_t kBarMem[] PROGMEM = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,0,0,2,2,2,0,2,2,0,0,2,2,1,1,2,0,2,0,2,0,2,0,2,0,2,0,2,1,1,2,0,0,2,2,0,0,0,2,0,0,2,2,1,1,2,0,2,0,2,0,2,0,2,0,2,0,2,1,1,2,0,0,2,2,0,2,0,2,0,2,0,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
static const uint8_t kBarPal[] PROGMEM = {255,255,255,255,242,0,0,0,0};

// CHERRY
static const uint8_t kCherryMem[] PROGMEM = {15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,12,15,15,15,15,15,15,15,15,15,15,7,2,4,2,3,7,15,15,15,15,15,15,15,13,1,7,15,15,15,15,15,15,15,15,15,2,15,12,15,15,15,15,15,15,11,0,4,15,15,11,6,13,15,15,15,15,8,5,5,6,0,0,5,10,15,15,15,15,15,9,9,15,8,8,10,10,15,15,15,15,15,15,15,15,15,8,10,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15};
static const uint8_t kCherryPal[] PROGMEM = {253,165,0,22,172,0,9,165,0,2,141,6,0,77,26,245,29,0,145,17,0,0,49,13,211,7,0,189,7,0,158,3,0,88,3,0,0,8,5,0,4,9,26,0,0,0,0,0};

// SEVEN
static const uint8_t kSevenMem[] PROGMEM = {15,15,15,15,15,15,15,15,15,15,15,15,15,10,14,1,1,1,10,15,10,13,15,15,15,7,0,4,4,4,5,5,5,13,15,15,15,7,2,3,3,3,4,3,6,13,15,15,15,8,7,15,15,15,0,7,13,15,15,15,15,15,15,15,12,0,7,9,14,15,15,15,15,15,15,14,8,6,8,15,15,15,15,15,15,15,15,13,0,6,8,15,15,15,15,15,15,15,15,2,0,6,8,15,15,15,15,15,15,15,15,10,10,10,10,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15};
static const uint8_t kSevenPal[] PROGMEM = {253,196,0,194,137,0,228,66,0,254,54,0,253,53,0,251,49,0,252,44,0,247,27,0,229,19,0,207,12,0,184,12,0,105,5,0,9,2,0,28,0,0,2,0,1,0,0,0};

// DIAMOND
static const uint8_t kDiamondMem[] PROGMEM = {15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,13,3,3,2,2,4,7,14,15,15,15,11,2,1,1,1,0,2,4,0,13,15,12,4,5,2,5,5,6,6,9,8,9,15,15,12,8,4,6,0,0,9,5,7,12,15,15,15,12,5,6,0,9,5,9,11,15,15,15,15,15,13,7,7,7,10,12,15,15,15,15,15,15,15,14,8,10,15,15,15,15,15,15,15,15,15,15,14,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15};
static const uint8_t kDiamondPal[] PROGMEM = {0,250,253,0,243,243,0,241,248,0,238,247,0,230,249,0,217,243,0,203,232,0,162,191,0,120,141,0,92,109,0,64,76,0,16,11,0,6,6,0,2,2,0,1,1,0,0,0};

// BELL
static const uint8_t kBellMem[] PROGMEM = {15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,12,0,14,15,15,15,15,15,15,15,15,13,7,2,6,11,15,15,15,15,15,15,15,14,0,1,3,6,15,15,15,15,15,15,15,6,0,1,2,5,15,15,15,15,15,15,12,5,1,2,4,5,14,15,15,15,15,9,3,0,3,3,4,5,6,11,15,15,15,9,3,10,11,0,14,11,10,10,15,15,15,15,14,8,8,4,7,8,8,12,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15};
static const uint8_t kBellPal[] PROGMEM = {254,245,0,252,236,0,252,212,0,249,182,0,236,161,0,226,143,0,141,78,0,46,26,0,23,10,0,3,14,0,10,5,0,6,4,0,2,4,5,1,0,69,1,0,0,0,0,0};

// STAR
static const uint8_t kStarMem[] PROGMEM = {15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,8,15,15,15,15,15,15,15,15,15,15,12,2,9,15,15,15,15,15,15,15,15,15,12,0,5,13,15,15,15,15,15,14,5,0,0,0,3,3,2,7,14,15,15,15,15,6,1,1,4,6,8,15,15,15,15,15,15,9,2,8,7,6,15,15,15,15,15,15,13,4,4,11,9,5,9,15,15,15,15,15,11,6,12,15,12,11,8,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15};
static const uint8_t kStarPal[] PROGMEM = {252,247,0,255,236,0,250,226,0,255,219,0,251,210,0,253,198,0,249,184,0,229,157,0,225,133,0,142,88,0,73,38,0,22,14,0,2,4,2,23,0,1,0,0,1,0,0,0};

inline RGBColor SampleSprite(const SpriteData& sprite, float u, float v) {
    float us = u * sprite.scale;
    float vs = v * sprite.scale;
    float gx = (us + 1.0f) * 0.5f * sprite.width;
    float gy = ((-vs) + 1.0f) * 0.5f * sprite.height; // flip vertically to match display
    if (gx < 0.0f || gy < 0.0f || gx >= sprite.width || gy >= sprite.height) return RGBColor();
    uint8_t ix = static_cast<uint8_t>(gx);
    uint8_t iy = static_cast<uint8_t>(gy);
    uint16_t idx = static_cast<uint16_t>(iy) * sprite.width + ix;
    uint8_t palIndex = pgm_read_byte_near(sprite.mem + idx);
    if (palIndex >= 15) return RGBColor(); // transparent
    if (sprite.transparentZero && palIndex == 0) return RGBColor();
    uint16_t palPos = static_cast<uint16_t>(palIndex) * 3;
    uint8_t r = pgm_read_byte_near(sprite.pal + palPos + 0);
    uint8_t g = pgm_read_byte_near(sprite.pal + palPos + 1);
    uint8_t b = pgm_read_byte_near(sprite.pal + palPos + 2);
    return RGBColor(r, g, b);
}

RGBColor DrawSymbol(uint8_t symbol, float u, float v, const RGBColor* /*palette*/) {
    // u, v are normalized to [-1, 1] inside the cell (0,0 = center)
    static const SpriteData sprites[6] = {
        {15, 7,  kBarMem,     kBarPal,     1.0f,  false},
        {12, 12, kCherryMem,  kCherryPal,  0.86f, false},
        {12, 12, kSevenMem,   kSevenPal,   0.86f, false},
        {12, 12, kDiamondMem, kDiamondPal, 0.86f, false},
        {12, 12, kBellMem,    kBellPal,    0.86f, false},
        {12, 12, kStarMem,    kStarPal,    0.86f, false}
    };
    const SpriteData& sprite = sprites[symbol % 6];
    return SampleSprite(sprite, u, v);
}
}

SlotMachineMaterial::SlotMachineMaterial(Vector2D dimensions, Vector2D center)
    : size(dimensions.Divide(2.0f)),
      offset(center) {
    RecalculateDimensions();
    spinning = false;
    stopping = false;
    spinTimer = 0.0f;
    confettiActive = false;
    confettiTimer = 0.0f;
    forceWinPlanned = false;
    forceWinSymbol = 0;
    stopStartMs = 0;
    for (uint8_t i = 0; i < kReels; ++i) {
        reelOffset[i] = static_cast<float>(random(kSymbols));
        reelSpeed[i] = 0.0f;
        snapPending[i] = false;
        reelStopping[i] = false;
        reelStopped[i] = true;
        snapLerping[i] = false;
        snapProgress[i] = 0.0f;
    }
    lastUpdateMs = 0;
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

    forceWinPlanned = false;
    forceWinSymbol = 0;

    for (uint8_t i = 0; i < kReels; ++i) {
        reelSpeed[i] = spinSpeed * (1.0f - i * 0.15f);
        snapPending[i] = false;
        reelStopping[i] = false;
        reelStopped[i] = false;
        snapLerping[i] = false;
        snapProgress[i] = 0.0f;
    }

    lastUpdateMs = millis();
}

void SlotMachineMaterial::PullLever() {
    if (spinning || stopping) return;
    ResetSpin();
}

void SlotMachineMaterial::SetLeverPulled(bool pulled) {
    if (pulled && !leverLatched) {
        PullLever();
    }
    leverLatched = pulled;
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
            // roll chance for forced win ahead of snapping so we can ease to it (20% => 1 in 5)
            forceWinPlanned = (random(5) == 0);
            forceWinSymbol = static_cast<uint8_t>(random(kSymbols));
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
        float currentSpeed = reelSpeed[i];
        reelOffset[i] += currentSpeed * delta;
        if (reelOffset[i] > 10000.0f) reelOffset[i] = fmodf(reelOffset[i], static_cast<float>(kSymbols));

        if (stopping && !reelStopped[i] && !snapLerping[i]) {
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
            reelSpeed[i] = 0.0f;
            snapPending[i] = false;
            snapLerping[i] = true;
            snapProgress[i] = 0.0f;
            snapStart[i] = reelOffset[i];
            float naturalSnap = roundf(reelOffset[i]);
            float target = naturalSnap;
            if (forceWinPlanned) target = static_cast<float>(forceWinSymbol);
            snapTarget[i] = target;
        }

        if (snapLerping[i]) {
            snapProgress[i] += delta * 5.0f; // ~0.2s ease
            float t = Clamp01(snapProgress[i]);
            float ease = t * t * (3.0f - 2.0f * t); // smoothstep
            reelOffset[i] = snapStart[i] + (snapTarget[i] - snapStart[i]) * ease;
            if (t >= 1.0f) {
                reelOffset[i] = snapTarget[i];
                snapLerping[i] = false;
                reelStopped[i] = true;
            }
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
        if (!matched && forceWinPlanned) {
            mid0 = mid1 = mid2 = forceWinSymbol;
            forcedWin = true;
        }
        confettiActive = matched || forcedWin;
        confettiTimer = confettiActive ? confettiDuration : 0.0f;
        forceWinPlanned = false;
    }
}

RGBColor SlotMachineMaterial::GetRGB(const Vector3D& position, const Vector3D& /*normal*/, const Vector3D& /*uvw*/) {
    Vector2D relative(position.X, position.Y);
    relative = relative - offset;

    if (relative.X < -size.X || relative.X > size.X) return RGBColor();
    if (relative.Y < -size.Y || relative.Y > size.Y) return RGBColor();

    float innerLeft = -gridHalfW;
    float innerBottom = -gridHalfH;
    float minAxis = Mathematics::Min(cellWidth, cellHeight);

    // outer/background and frame (1px-style margin)
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

    // smooth reel scrolling: derive symbol index and vertical phase from continuous offset
    float cellCenterX = innerLeft + (col + 0.5f) * cellWidth;
    float relCells = (relative.Y - innerBottom) / cellHeight; // 0..3 across visible window
    float symbolFloat = reelOffset[col] + relCells - 1.0f;    // center row aligns to 0.5 phase at stop
    int symbolIdx = static_cast<int>(floorf(symbolFloat));
    float vPhase = symbolFloat - static_cast<float>(symbolIdx); // 0..1 inside current symbol vertically
    float vNorm = (vPhase - 0.5f) * 2.0f; // -1..1 bottom->top
    int wrapped = symbolIdx % kSymbols;
    if (wrapped < 0) wrapped += kSymbols;
    uint8_t symbolId = static_cast<uint8_t>(wrapped);

    // normalize u/v; BAR uses full cell width/height, others keep square aspect via minAxis
    float u = (relative.X - cellCenterX) / (minAxis * 0.5f);
    float v = vNorm * (cellHeight / minAxis);
    if (symbolId == 0) {
        u = (relative.X - cellCenterX) / (cellWidth * 0.5f);
        v = vNorm;
    }

    RGBColor symbol = DrawSymbol(symbolId, u, v, symbolColors);
    if (symbol.R | symbol.G | symbol.B) return symbol;

    // confetti overlay on win
    if (confettiActive) {
        // sprinkle using a quick hash on position + time
        uint32_t t = millis();
        uint32_t seed = static_cast<uint32_t>((int32_t)(relative.X * 7.0f) * 73856093u) ^
                        static_cast<uint32_t>((int32_t)(relative.Y * 13.0f) * 19349663u) ^
                        (t * 2654435761u);
        float flash = ((seed >> 24) & 0xFF) / 255.0f;
        if (flash > 0.6f) { // denser, more chaotic
            uint8_t idx = seed % 6u;
            RGBColor c = symbolColors[idx];
            float alpha = Clamp01(confettiTimer / confettiDuration);
            alpha = 0.6f + alpha * 0.8f; // boost brightness
            return RGBColor(uint8_t(c.R * alpha), uint8_t(c.G * alpha), uint8_t(c.B * alpha));
        }
    }

    return reelColor;
}
