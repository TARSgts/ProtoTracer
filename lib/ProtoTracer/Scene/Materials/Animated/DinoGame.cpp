#include "DinoGame.h"

#include <Arduino.h>
#include <cmath>

#include "../../../Utils/Math/Mathematics.h"

namespace {
constexpr float kMaxDelta = 0.05f;
constexpr float kHitboxShrink = 0.82f;
constexpr float kTwoPi = 6.2831853f;
// Real Chrome Dino ramps SPEED(6) to MAX_SPEED(13) at ACCELERATION=0.001/frame@60fps,
// i.e. ~117s. Match that order of magnitude so speed doesn't outpace reaction time.
constexpr float kSpeedRampSeconds = 120.0f;
// Real game sizes the gap to the next obstacle off current speed (minGap = width*speed
// + typeConfig.minGap*GAP_COEFFICIENT, maxGap = minGap*1.5) specifically so the time to
// cross a gap stays roughly constant as speed increases. ComputeGap() mirrors that.
// Bumped from 1.4 to 1.8 for extra breathing room between consecutive obstacles.
constexpr float kGapSeconds = 1.8f;

// Hand-drawn pixel-art sprite (top row first), not derived from proportional rectangle
// math -- three rounds of "cut a fraction of the body away" produced an unrecognizable
// blob at the ~6-10 physical pixels this renders to. Each character is exactly one
// physical LED (see kPixelPitch). '.'=background, 'X'=body, 'E'=eye.
constexpr uint8_t kDinoBodyRows = 9;
constexpr uint8_t kDinoFeetRows = 2;
constexpr uint8_t kDinoRows = kDinoBodyRows + kDinoFeetRows;
constexpr uint8_t kDinoCols = 9;
const char* const kDinoBody[kDinoBodyRows] = {
    "....XXXX.",
    "...XEXXXX",
    "...XXX...",
    "...XXXXXX",
    "X..XXXX..",
    "XX.XXXX..",
    "XXXXXXXXX",
    ".XXXXXX..",
    "..XXXXX..",
};
// Feet alternate between two frames that both keep the legs populated -- shifting
// position, not blacking out a big region -- since a full vanish/reappear read as
// flicker rather than running at this resolution.
const char* const kDinoFeetA[kDinoFeetRows] = {
    ".XX..XX..",
    ".X....X..",
};
const char* const kDinoFeetB[kDinoFeetRows] = {
    "..XXXX...",
    "...XX....",
};
constexpr float kLegSwapSeconds = 0.18f;

// Three hand-drawn cactus variants for visual variety (real Chrome Dino similarly has
// small/large sprites plus small clusters) -- replacing a single fixed cactus, which
// read as "all the same" despite being safely sized. Visual footprint is taller/wider
// than the actual gameplay hitbox in each case (top rows are decorative upward-curving
// arm tips) -- same "visual bigger than hitbox" trick as the dino's width. Collision
// dims are independently verified against the jump apex (see RecalculateDimensions).
constexpr uint8_t kCactusSmallRows = 7;
constexpr uint8_t kCactusSmallCols = 5;
const char* const kCactusSmallBitmap[kCactusSmallRows] = {
    "..X..",
    "X.X..",
    "X.X.X",
    ".XXX.",
    "..X..",
    "..X..",
    "..X..",
};
constexpr float kCactusSmallCollisionWidth = 9.0f;   // 3 physical px.
constexpr float kCactusSmallCollisionHeight = 15.0f; // 5 physical px.

// Taller trunk, still only as wide as Small -- the extra height is what reads as
// "medium", not extra bulk.
constexpr uint8_t kCactusMediumRows = 9;
constexpr uint8_t kCactusMediumCols = 5;
const char* const kCactusMediumBitmap[kCactusMediumRows] = {
    "..X..",
    "..X.X",
    "X.X.X",
    "X.X.X",
    ".XXX.",
    "..X..",
    "..X..",
    "..X..",
    "..X..",
};
// Collision height intentionally matches Small exactly (not the taller visual) -- this
// obstacle being "still too hard to clear" persisted across multiple jump-apex increases
// because a taller hitbox was directly fighting the apex-based fairness fix. Rather than
// keep raising the jump apex to compensate for one obstacle's height, decouple it:
// Medium is purely a visual variant (taller sprite) with the exact same, already-
// comfortable difficulty as Small.
constexpr float kCactusMediumCollisionWidth = 9.0f;
constexpr float kCactusMediumCollisionHeight = 15.0f;

// Two Small cacti side by side, as one obstacle (generated from kCactusSmallBitmap with
// a 1-column gap, kept as a literal table here since C++ has no simple compile-time
// string-concat like the Python simulator's list comprehension -- verified character-
// for-character to match that generation). Combined width roughly doubles the overlap
// window the dino must clear, which only has a safe margin once the game has sped up --
// see kCactusDoubleMinSpeed, checked before this variant is ever chosen.
constexpr uint8_t kCactusDoubleRows = 7;
constexpr uint8_t kCactusDoubleCols = 11;
const char* const kCactusDoubleBitmap[kCactusDoubleRows] = {
    "..X.....X..",
    "..X.X.X.X.X",
    "X.X.X.X.X.X",
    ".XXX...XXX.",
    "..X.....X..",
    "..X.....X..",
    "..X.....X..",
};
constexpr float kCactusDoubleCollisionWidth = 21.0f; // 2x small collision width + 1px gap.
constexpr float kCactusDoubleCollisionHeight = 15.0f;
constexpr float kCactusDoubleMinSpeed = 70.0f; // verified via overlap-window-vs-high-enough-duration.

// Logical units per physical LED -- matches HUB75's real pitch (192 logical units / 64
// physical LEDs, see HUB75DeltaCameras.h) so each bitmap character is exactly one LED.
// Sprites are sized in fixed physical pixels rather than as a fraction of canvas size,
// like real pixel-art sprites would be. Shared by the dino, cactus, and score digits so
// everything sits on the same physical grid.
constexpr float kPixelPitch = 3.0f;
// The drawn dino sprite is sized for legibility, not fairness -- the gameplay hitbox is
// narrower than the visual body (see CheckCollision). Widening the hitbox to match the
// full (bigger, post-bitmap-redesign) sprite made tall/wide obstacles literally
// uncrossable at low speed: verified numerically that the required "stay above obstacle
// height" window exceeded the time actually available above that height.
constexpr float kCollisionWidthFraction = 0.6f;

// Compact 3x5 pixel digit font for the score HUD, scaled kScoreScale-x. Originally
// rendered via the shared TextEngine class (also used by Menu/Clock), but its internal
// Y-axis Map() has an off-by-one at the box boundary (a position mapping to exactly
// lineCount*10 fails the `< lineCount*10` bounds check and renders black), which
// clipped a row of the score. Switched to this hand-drawn bitmap approach instead, same
// technique as the dino/cactus. At 1x scale the thin 1-pixel strokes were reported
// "completely illegible" on real hardware (LED diffusion washes out single-pixel
// detail); 2x fixed that but was then "way too big" (~62% of screen width). 1.5 is the
// middle ground -- works cleanly here since the score box already samples via
// continuous floorf() math rather than literal NxN block replication, so a non-integer
// scale doesn't need special-casing.
constexpr uint8_t kScoreDigitRows = 5;
constexpr uint8_t kScoreDigitCols = 3;
constexpr uint8_t kScoreDigitCount = 5;
constexpr float kScoreScale = 1.5f;
const char* const kScoreDigits[10][kScoreDigitRows] = {
    {"111", "101", "101", "101", "111"}, // 0
    {"010", "110", "010", "010", "111"}, // 1
    {"111", "001", "111", "100", "111"}, // 2
    {"111", "001", "111", "001", "111"}, // 3
    {"101", "101", "111", "001", "001"}, // 4
    {"111", "100", "111", "001", "111"}, // 5
    {"111", "100", "111", "101", "111"}, // 6
    {"111", "001", "001", "001", "001"}, // 7
    {"111", "101", "111", "101", "111"}, // 8
    {"111", "101", "111", "001", "111"}, // 9
};
constexpr float kScoreGapCols = 1.0f; // 1 physical px between digits (before scaling).
}

DinoGameMaterial::DinoGameMaterial(Vector2D dimensions, Vector2D center)
    : size(dimensions.Divide(2.0f)),
      offset(center) {
    RecalculateDimensions();
    ResetClouds();
    Reset();
}

void DinoGameMaterial::SetSize(Vector2D dimensions) {
    size = dimensions.Divide(2.0f);
    RecalculateDimensions();
    ResetClouds();
}

void DinoGameMaterial::SetPosition(Vector2D center) {
    offset = center;
}

void DinoGameMaterial::RecalculateDimensions() {
    groundHeight = Mathematics::Max(2.0f, size.Y * 0.08f);
    dinoWidth = kDinoCols * kPixelPitch;
    dinoHeight = kDinoRows * kPixelPitch;
    minObstacleSpeed = Mathematics::Max(35.0f, size.X * 0.5f);
    maxObstacleSpeed = minObstacleSpeed * 2.2f;
    gravity = -Mathematics::Max(150.0f, size.Y * 4.5f);
    jumpVelocity = Mathematics::Max(90.0f, size.Y * 2.35f);
    // Peak jump height reachable with the above gravity/velocity; each fixed cactus
    // size is verified against this so it's always jumpable with margin.
    apexHeight = (jumpVelocity * jumpVelocity) / (2.0f * fabsf(gravity));

    speedRampPerSecond = (maxObstacleSpeed - minObstacleSpeed) / kSpeedRampSeconds;
    minGapPixels = size.X * 0.35f;

    // Hills: a prominent background dune/mountain line (peak reaches ~38% of half-height).
    hillBaseHeight = Mathematics::Max(4.0f, size.Y * 0.14f);
    hillAmplitude = Mathematics::Max(6.0f, size.Y * 0.24f);
    hillSpeed = minObstacleSpeed * 0.35f;
    hillFrequency = 3.5f / Mathematics::Max(1.0f, size.X);

    // Clouds: big, wide puffs, drifting slowly.
    cloudWidth = Mathematics::Max(20.0f, size.X * 0.30f);
    cloudHeight = cloudWidth * 0.55f;
    cloudSpeed = minObstacleSpeed * 0.18f;
}

float DinoGameMaterial::ComputeGap() const {
    float base = Mathematics::Max(minGapPixels, obstacleSpeed * kGapSeconds);
    float extra = base * (static_cast<float>(random(0, 5000)) / 10000.0f);
    return base + extra;
}

DinoGameMaterial::CactusType DinoGameMaterial::RandomObstacleType() const {
    // "Double" (two small cacti side by side) roughly doubles the horizontal span the
    // dino must clear in one jump -- only safe once the game has sped up enough
    // (verified: overlap-window-vs-high-enough-duration goes negative below this).
    uint8_t choiceCount = (obstacleSpeed >= kCactusDoubleMinSpeed) ? 3 : 2;
    uint8_t pick = static_cast<uint8_t>(random(0, choiceCount));
    if (pick == 0) return CactusType::Small;
    if (pick == 1) return CactusType::Medium;
    return CactusType::Double;
}

void DinoGameMaterial::ResetObstacle(uint8_t index, float xPosition) {
    if (index >= kObstacleCount) return;
    obstacleX[index] = xPosition;
    obstacleType[index] = RandomObstacleType();
}

void DinoGameMaterial::ResetClouds() {
    float skyTop = size.Y;
    float skyBottom = -size.Y + groundHeight + hillBaseHeight + hillAmplitude + cloudHeight;
    for (uint8_t i = 0; i < kCloudCount; ++i) {
        float xt = static_cast<float>(random(0, 10000)) / 10000.0f;
        float yt = static_cast<float>(random(0, 10000)) / 10000.0f;
        cloudX[i] = -size.X + xt * (size.X * 2.0f);
        cloudY[i] = skyBottom + yt * Mathematics::Max(1.0f, (skyTop - skyBottom));
    }
    cloudsInitialized = true;
}

void DinoGameMaterial::UpdateClouds(float delta) {
    float skyTop = size.Y;
    float skyBottom = -size.Y + groundHeight + hillBaseHeight + hillAmplitude + cloudHeight;
    for (uint8_t i = 0; i < kCloudCount; ++i) {
        cloudX[i] -= cloudSpeed * delta;
        if (cloudX[i] < -size.X - cloudWidth) {
            cloudX[i] = size.X + cloudWidth;
            float yt = static_cast<float>(random(0, 10000)) / 10000.0f;
            cloudY[i] = skyBottom + yt * Mathematics::Max(1.0f, (skyTop - skyBottom));
        }
    }
}

void DinoGameMaterial::Reset() {
    dinoX = -size.X * 0.6f;
    dinoY = 0.0f;
    dinoVelocity = 0.0f;
    onGround = true;
    legPhase = false;
    legTimer = 0.0f;
    survivalTime = 0.0f;
    distance = 0.0f;
    score = 0;
    obstacleSpeed = minObstacleSpeed;

    float x = size.X + ComputeGap() * 0.5f;
    for (uint8_t i = 0; i < kObstacleCount; ++i) {
        ResetObstacle(i, x);
        x += ComputeGap();
    }

    lastUpdateMs = millis();
}

void DinoGameMaterial::SetJumpPressed(bool pressed) {
    if (pressed && !jumpLatched && onGround) {
        dinoVelocity = jumpVelocity;
        onGround = false;
    }
    jumpLatched = pressed;
}

bool DinoGameMaterial::CheckCollision() const {
    float halfWidth = dinoWidth * 0.5f * kHitboxShrink * kCollisionWidthFraction;
    float dinoBottom = -size.Y + groundHeight + dinoY;
    float dinoTop = dinoBottom + dinoHeight * kHitboxShrink;
    float dinoLeft = dinoX - halfWidth;
    float dinoRight = dinoX + halfWidth;

    for (uint8_t i = 0; i < kObstacleCount; ++i) {
        float collisionWidth, collisionHeight;
        switch (obstacleType[i]) {
            case CactusType::Medium:
                collisionWidth = kCactusMediumCollisionWidth;
                collisionHeight = kCactusMediumCollisionHeight;
                break;
            case CactusType::Double:
                collisionWidth = kCactusDoubleCollisionWidth;
                collisionHeight = kCactusDoubleCollisionHeight;
                break;
            case CactusType::Small:
            default:
                collisionWidth = kCactusSmallCollisionWidth;
                collisionHeight = kCactusSmallCollisionHeight;
                break;
        }

        float left = obstacleX[i] - collisionWidth * 0.5f;
        float right = obstacleX[i] + collisionWidth * 0.5f;
        if (dinoRight < left || dinoLeft > right) continue;

        float obstacleBottom = -size.Y + groundHeight;
        float obstacleTop = obstacleBottom + collisionHeight;
        if (dinoTop > obstacleBottom && dinoBottom < obstacleTop) {
            return true;
        }
    }

    return false;
}

void DinoGameMaterial::Update() {
    uint32_t now = millis();
    if (lastUpdateMs == 0) {
        lastUpdateMs = now;
        return;
    }

    float delta = (now - lastUpdateMs) / 1000.0f;
    if (delta > kMaxDelta) delta = kMaxDelta;
    lastUpdateMs = now;

    survivalTime += delta;
    obstacleSpeed = Mathematics::Min(maxObstacleSpeed, minObstacleSpeed + survivalTime * speedRampPerSecond);

    distance += obstacleSpeed * delta;
    score = static_cast<uint32_t>(distance) % 100000UL;

    if (!onGround) {
        dinoVelocity += gravity * delta;
        dinoY += dinoVelocity * delta;
        if (dinoY <= 0.0f) {
            dinoY = 0.0f;
            dinoVelocity = 0.0f;
            onGround = true;
        }
    } else {
        legTimer += delta;
        if (legTimer >= kLegSwapSeconds) {
            legTimer = 0.0f;
            legPhase = !legPhase;
        }
    }

    // Widest possible visual footprint (Double) is used for the off-screen check so a
    // wide cluster doesn't visually pop out of existence before it's fully gone.
    float cactusVisualWidth = kCactusDoubleCols * kPixelPitch;
    float maxX = obstacleX[0];
    for (uint8_t i = 0; i < kObstacleCount; ++i) {
        obstacleX[i] -= obstacleSpeed * delta;
        if (obstacleX[i] > maxX) maxX = obstacleX[i];
    }

    for (uint8_t i = 0; i < kObstacleCount; ++i) {
        if (obstacleX[i] < -size.X - cactusVisualWidth) {
            maxX += ComputeGap();
            ResetObstacle(i, maxX);
        }
    }

    UpdateClouds(delta);
    hillPhase += hillSpeed * delta;
    float hillPeriod = kTwoPi / Mathematics::Max(0.0001f, hillFrequency);
    hillPhase = fmodf(hillPhase, hillPeriod);

    if (CheckCollision()) {
        Reset();
    }
}

RGBColor DinoGameMaterial::GetRGB(const Vector3D& position, const Vector3D& /*normal*/, const Vector3D& /*uvw*/) {
    Vector2D relative(position.X, position.Y);
    relative = relative - offset;

    if (relative.X < -size.X || relative.X > size.X) return RGBColor();
    if (relative.Y < -size.Y || relative.Y > size.Y) return RGBColor();

    // --- Force the bottom-most physical row to black, unconditionally. Reported as a
    // persistent flicker at the very bottom edge of the display -- suspected cause is a
    // boundary instability in how the shared camera/rasterizer samples the last
    // physical row (the PixelGroup for HUB75 is declared as a 96-logical-unit-tall area
    // but the face canvas only uses 94 of them, see HUB75DeltaCameras.h) rather than
    // anything specific to this face's own rendering logic. This is a direct,
    // unconditional workaround requested in place of chasing that root cause, so it
    // takes priority over everything else drawn below (score included). ---
    if (relative.Y < -size.Y + kPixelPitch) {
        return RGBColor();
    }

    // --- Score HUD: top-left corner, drawn as a fixed pixel-digit bitmap (bypasses
    // TextEngine's fragile Y-axis boundary mapping, which was clipping a row). Anti-
    // aliased via 4x supersampling + coverage blending -- the same technique
    // TextEngine.tpp uses internally -- scoped to just this HUD so the dino/cactus/
    // ground stay crisp blocky pixel art while the score gets smoothed edges. ---
    {
        float scaledPitch = kPixelPitch * kScoreScale;
        float scoreLeft = -size.X + kPixelPitch;
        float scoreTop = size.Y - kPixelPitch;
        float cellWidth = (kScoreDigitCols + kScoreGapCols) * scaledPitch;
        float scoreWidth = kScoreDigitCount * cellWidth;
        float scoreHeight = kScoreDigitRows * scaledPitch;
        if (relative.X >= scoreLeft && relative.X < scoreLeft + scoreWidth &&
            relative.Y <= scoreTop && relative.Y > scoreTop - scoreHeight) {
            int cellCols = kScoreDigitCols + static_cast<int>(kScoreGapCols);
            static const float kSampleOffsets[4][2] = {
                {-0.25f, -0.25f}, {0.25f, -0.25f}, {-0.25f, 0.25f}, {0.25f, 0.25f}
            };
            float coverage = 0.0f;
            for (uint8_t s = 0; s < 4; ++s) {
                float sampleX = relative.X + kSampleOffsets[s][0] * kPixelPitch;
                float sampleY = relative.Y + kSampleOffsets[s][1] * kPixelPitch;
                if (sampleX < scoreLeft || sampleX >= scoreLeft + scoreWidth ||
                    sampleY > scoreTop || sampleY <= scoreTop - scoreHeight) {
                    continue; // sample lands outside the score box -> background
                }
                int totalCol = static_cast<int>(floorf((sampleX - scoreLeft) / scaledPitch));
                int row = static_cast<int>(floorf((scoreTop - sampleY) / scaledPitch));
                int digitIndex = totalCol / cellCols;
                int colInDigit = totalCol % cellCols;
                if (digitIndex < 0 || digitIndex >= kScoreDigitCount || colInDigit >= kScoreDigitCols ||
                    row < 0 || row >= kScoreDigitRows) {
                    continue; // inter-digit gap or outside glyph rows -> background
                }
                uint32_t divisor = 1;
                for (int d = kScoreDigitCount - 1; d > digitIndex; --d) divisor *= 10;
                uint8_t digitValue = (score / divisor) % 10;
                if (kScoreDigits[digitValue][row][colInDigit] == '1') {
                    coverage += 0.25f;
                }
            }
            return RGBColor::InterpolateColors(RGBColor(), scoreColor, coverage);
        }
    }

    float groundY = -size.Y + groundHeight;

    // --- Dino: hand-drawn pixel-art bitmap, looked up one physical LED at a time. ---
    float dinoBottom = groundY + dinoY;
    float dinoLeft = dinoX - dinoWidth * 0.5f;
    float localX = relative.X - dinoLeft;
    float localY = relative.Y - dinoBottom;
    int col = static_cast<int>(floorf(localX / kPixelPitch));
    int rowFromBottom = static_cast<int>(floorf(localY / kPixelPitch));
    int row = (kDinoRows - 1) - rowFromBottom;
    if (row >= 0 && row < kDinoRows && col >= 0 && col < kDinoCols) {
        char cell;
        if (row < kDinoBodyRows) {
            cell = kDinoBody[row][col];
        } else {
            const char* const* feet = legPhase ? kDinoFeetA : kDinoFeetB;
            cell = feet[row - kDinoBodyRows][col];
        }
        if (cell == 'X') return dinoColor;
        if (cell == 'E') return dinoEyeColor;
    }

    // --- Obstacles: cacti, hand-drawn pixel-art bitmaps with upward-curving arms. Each
    // obstacle picked its variant (small/medium/double) in ResetObstacle, so there's
    // real visual variety instead of every cactus looking identical. ---
    for (uint8_t i = 0; i < kObstacleCount; ++i) {
        const char* const* bitmap;
        uint8_t rows, cols;
        switch (obstacleType[i]) {
            case CactusType::Medium:
                bitmap = kCactusMediumBitmap;
                rows = kCactusMediumRows;
                cols = kCactusMediumCols;
                break;
            case CactusType::Double:
                bitmap = kCactusDoubleBitmap;
                rows = kCactusDoubleRows;
                cols = kCactusDoubleCols;
                break;
            case CactusType::Small:
            default:
                bitmap = kCactusSmallBitmap;
                rows = kCactusSmallRows;
                cols = kCactusSmallCols;
                break;
        }

        float cactusLeft = obstacleX[i] - (cols * kPixelPitch) * 0.5f;
        float localCactusX = relative.X - cactusLeft;
        float localCactusY = relative.Y - groundY;
        int cCol = static_cast<int>(floorf(localCactusX / kPixelPitch));
        int cRowFromBottom = static_cast<int>(floorf(localCactusY / kPixelPitch));
        int cRow = (rows - 1) - cRowFromBottom;
        if (cRow >= 0 && cRow < rows && cCol >= 0 && cCol < cols) {
            if (bitmap[cRow][cCol] == 'X') return cactusColor;
        }
    }

    // --- Ground: flat fill below the horizon line, anti-aliased only right at that edge
    // via the same 4-sample supersampling/coverage-blending technique used for the score
    // HUD -- blends toward the hill color immediately above. Hills always reach the ground
    // line here (hillBaseHeight is set well above this narrow edge band in
    // RecalculateDimensions), so "just above the ground line" is always hill, never sky
    // or cloud -- safe to blend toward hillColor unconditionally. ---
    float groundEdgeBand = kPixelPitch * 0.5f;
    if (fabsf(relative.Y - groundY) <= groundEdgeBand) {
        static const float kGroundSampleOffsetsY[4] = {-0.25f, 0.25f, -0.25f, 0.25f};
        float coverage = 0.0f;
        for (uint8_t s = 0; s < 4; ++s) {
            float sampleY = relative.Y + kGroundSampleOffsetsY[s] * kPixelPitch;
            if (sampleY < groundY) {
                coverage += 0.25f;
            }
        }
        return RGBColor::InterpolateColors(hillColor, groundColor, coverage);
    }

    if (relative.Y < groundY) {
        return groundColor;
    }

    // --- Rolling hill horizon, scrolling slower than the obstacles for a parallax feel. ---
    float hillWave = sinf((relative.X + hillPhase) * hillFrequency) * 0.5f + 0.5f;
    float hillHeight = hillBaseHeight + hillAmplitude * hillWave;
    if (relative.Y <= groundY + hillHeight) {
        return hillColor;
    }

    // --- Clouds drifting through the upper sky: a flat base plus three puff lobes. ---
    for (uint8_t i = 0; i < kCloudCount; ++i) {
        float cx = cloudX[i];
        float cy = cloudY[i];
        float w = cloudWidth;
        float h = cloudHeight;
        float dx = relative.X - cx;
        float dy = relative.Y - cy;

        bool inBase = dx >= -w * 0.5f && dx <= w * 0.5f && dy >= -h * 0.35f && dy <= h * 0.05f;
        bool inLobeLeft = dx >= -w * 0.36f && dx <= -w * 0.02f && dy >= -h * 0.05f && dy <= h * 0.32f;
        bool inLobeMid = dx >= -w * 0.20f && dx <= w * 0.20f && dy >= h * 0.05f && dy <= h * 0.48f;
        bool inLobeRight = dx >= w * 0.02f && dx <= w * 0.38f && dy >= -h * 0.08f && dy <= h * 0.30f;
        if (inBase || inLobeLeft || inLobeMid || inLobeRight) {
            return cloudColor;
        }
    }

    return skyColor;
}
