#include "LavaLampMaterial.h"

#include <Arduino.h>
#include <cmath>

namespace {
constexpr float kPi = 3.14159265f;
}

float LavaLampMaterial::Clamp01(float value) {
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

float LavaLampMaterial::Clamp(float value, float minimum, float maximum) {
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

float LavaLampMaterial::SmoothStep(float value) {
    value = Clamp01(value);
    return value * value * (3.0f - (2.0f * value));
}

float LavaLampMaterial::Random01() {
    return static_cast<float>(random(0, 10001)) / 10000.0f;
}

RGBColor LavaLampMaterial::ScaleColor(const RGBColor& color, float factor) {
    if (factor < 0.0f) factor = 0.0f;
    uint16_t r = static_cast<uint16_t>(color.R * factor);
    uint16_t g = static_cast<uint16_t>(color.G * factor);
    uint16_t b = static_cast<uint16_t>(color.B * factor);
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    return RGBColor(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b));
}

RGBColor LavaLampMaterial::LiftColor(const RGBColor& color, uint8_t amount) {
    uint16_t r = color.R + amount;
    uint16_t g = color.G + amount;
    uint16_t b = color.B + amount;
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    return RGBColor(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b));
}

LavaLampMaterial::LavaLampMaterial(Vector2D dimensions, Vector2D center)
    : size(dimensions),
      offset(center) {
    RecomputeBounds();
    for (uint8_t i = 0; i < kBlobCount; ++i) {
        ResetBlob(i);
    }
}

void LavaLampMaterial::RecomputeBounds() {
    halfWidth = size.X * 0.5f;
    halfHeight = size.Y * 0.5f;
}

void LavaLampMaterial::ResetBlob(uint8_t index) {
    Blob& blob = blobs[index];

    // Reported "still really small" on real hardware -- radius was 5.5-9.2 logical
    // units (~1.8-3.1 physical LED pitches, so barely 4-6 physical pixels across).
    // Roughly doubled to read as genuinely prominent blobs on a 64-wide panel.
    blob.radius = 10.0f + Random01() * 8.0f;
    blob.vx = (Random01() - 0.5f) * 1.5f;
    blob.vy = 3.0f + Random01() * 3.8f;
    blob.phase = Random01() * 2.0f * kPi;
    blob.pulseRate = 0.20f + Random01() * 0.34f;
    blob.driftRate = 0.24f + Random01() * 0.44f;
    blob.rising = Random01() < 0.5f;
    // Per-blob pace so they don't all rise/sink in lockstep -- some noticeably quicker,
    // some more languid, for a livelier, less mechanical-looking lamp.
    blob.riseStrength = 9.0f + Random01() * 6.0f;
    blob.sinkStrength = 7.0f + Random01() * 5.0f;

    // Keep each blob distributed across the panel to avoid center clustering.
    float laneWidth = (halfWidth * 2.0f) / static_cast<float>(kBlobCount);
    float laneCenter = -halfWidth + laneWidth * (static_cast<float>(index) + 0.5f);
    blob.anchorX = laneCenter + (Random01() - 0.5f) * laneWidth * 0.25f;

    float spanX = halfWidth - blob.radius - 1.0f;
    if (spanX < 2.0f) spanX = 2.0f;
    blob.anchorX = Clamp(blob.anchorX, -spanX, spanX);
    blob.x = blob.anchorX + (Random01() - 0.5f) * laneWidth * 0.35f;
    blob.x = Clamp(blob.x, -spanX, spanX);

    float spanY = halfHeight - blob.radius - 1.0f;
    if (spanY < 2.0f) spanY = 2.0f;
    blob.y = (Random01() - 0.5f) * spanY * 2.0f;
    if (Random01() < 0.4f) {
        blob.vy = -blob.vy;
    }
}

void LavaLampMaterial::SetSize(Vector2D dimensions) {
    size = dimensions;
    RecomputeBounds();
    for (uint8_t i = 0; i < kBlobCount; ++i) {
        ResetBlob(i);
    }
}

void LavaLampMaterial::SetPosition(Vector2D center) {
    offset = center;
}

void LavaLampMaterial::SetPalette(const RGBColor& baseColor) {
    uint16_t energy = baseColor.R + baseColor.G + baseColor.B;
    RGBColor base = (energy < 12) ? RGBColor(200, 90, 30) : baseColor;

    backgroundColor = RGBColor(0, 0, 0);
    // Hot = a bright, lifted version of the palette color (a white-hot glow near the
    // heat source); cool = a darker, deeper version hue-shifted toward blue/violet,
    // mimicking how molten material visibly darkens and cools as it rises away from
    // the heat source. Blended per-blob by height in GetRGB().
    hotColor = LiftColor(ScaleColor(base, 1.15f), 55);
    coolColor = LiftColor(ScaleColor(base, 0.30f), 6).HueShift(-60.0f);
}

void LavaLampMaterial::ResolveBlobSeparation(float dt) {
    for (uint8_t i = 0; i < kBlobCount; ++i) {
        for (uint8_t j = i + 1; j < kBlobCount; ++j) {
            Blob& a = blobs[i];
            Blob& b = blobs[j];

            float dx = b.x - a.x;
            float dy = b.y - a.y;
            float distSq = dx * dx + dy * dy + 0.0001f;
            float dist = sqrtf(distSq);
            float minDist = (a.radius + b.radius) * 1.08f;

            if (dist < minDist) {
                float push = (minDist - dist) * 0.5f;
                float nx = dx / dist;
                float ny = dy / dist;

                a.x -= nx * push;
                a.y -= ny * push;
                b.x += nx * push;
                b.y += ny * push;

                float vPush = 0.22f;
                a.vx -= nx * vPush * dt;
                a.vy -= ny * vPush * dt;
                b.vx += nx * vPush * dt;
                b.vy += ny * vPush * dt;
            }
        }
    }
}

void LavaLampMaterial::Update() {
    uint32_t now = millis();
    if (lastUpdateMs == 0) {
        lastUpdateMs = now;
        return;
    }

    float dt = (now - lastUpdateMs) / 1000.0f;
    if (dt > 0.05f) dt = 0.05f;
    lastUpdateMs = now;
    timeSeconds += dt;

    for (uint8_t i = 0; i < kBlobCount; ++i) {
        Blob& blob = blobs[i];

        // Bistable thermal cycle instead of a continuous force balance: a blob is either
        // decisively heating and rising, or decisively cooling and sinking, and flips
        // state on reaching the top/bottom of its travel range. This guarantees a full,
        // lively rise-and-fall every cycle -- the old buoyancy formula had a wide dead
        // zone around vertical center (both its heat and cool terms were exactly 0 there)
        // where a blob could stall and just hover mid-screen instead of completing a trip.
        float topFlip = halfHeight * 0.80f;
        float bottomFlip = -halfHeight * 0.80f;
        if (blob.rising && blob.y > topFlip) {
            blob.rising = false;
        } else if (!blob.rising && blob.y < bottomFlip) {
            blob.rising = true;
        }

        float thermalWave = sinf(timeSeconds * (0.55f + blob.pulseRate) + blob.phase) * 0.9f;
        float buoyancy = (blob.rising ? blob.riseStrength : -blob.sinkStrength) + thermalWave;
        float sizeDrag = (blob.radius - 7.0f) * 0.10f;

        blob.vy += (buoyancy - sizeDrag) * dt;
        blob.vy *= (1.0f - 0.045f * dt);
        blob.vy = Clamp(blob.vy, -10.5f, 10.5f);

        // A weak homing bias toward each blob's own lane (was 0.9, a tight leash) --
        // strong enough to still avoid all blobs drifting into a single center clump,
        // but loose enough to let blobs actually wander into a neighbor's territory and
        // pass close enough to merge, which the original value made all but impossible
        // (verified by simulation: 0% of run time had any two blobs within visual-merge
        // distance at 0.9, vs ~9% at 0.12, with no meaningful difference in overall
        // horizontal spread between the two).
        float springToLane = (blob.anchorX - blob.x) * 0.12f;
        float sideDrift = sinf(timeSeconds * blob.driftRate + blob.phase) * 0.65f;
        blob.vx += (springToLane + sideDrift) * dt;
        blob.vx *= (1.0f - 0.045f * dt);
        blob.vx = Clamp(blob.vx, -3.0f, 3.0f);

        blob.x += blob.vx * dt;
        blob.y += blob.vy * dt;

        float left = -halfWidth + blob.radius + 1.0f;
        float right = halfWidth - blob.radius - 1.0f;
        float bottom = -halfHeight + blob.radius + 1.0f;
        float top = halfHeight - blob.radius - 1.0f;

        if (blob.x > right) {
            float overshoot = blob.x - right;
            blob.x = right - overshoot;
            blob.vx = -fabsf(blob.vx) * 0.95f;
        } else if (blob.x < left) {
            float overshoot = left - blob.x;
            blob.x = left + overshoot;
            blob.vx = fabsf(blob.vx) * 0.95f;
        }

        if (blob.y > top) {
            float overshoot = blob.y - top;
            blob.y = top - overshoot;
            blob.vy = -fabsf(blob.vy) * 0.96f;
        } else if (blob.y < bottom) {
            float overshoot = bottom - blob.y;
            blob.y = bottom + overshoot;
            blob.vy = fabsf(blob.vy) * 0.96f;
        }

    }

    ResolveBlobSeparation(dt);
}

RGBColor LavaLampMaterial::GetRGB(const Vector3D& position, const Vector3D&, const Vector3D&) {
    float x = position.X - offset.X;
    float y = position.Y - offset.Y;

    if (x < -halfWidth || x > halfWidth || y < -halfHeight || y > halfHeight) {
        return backgroundColor;
    }

    float field = 0.0f;
    // Weighted running sum of each contributor's own color, weighted by how strongly it
    // influences this pixel (its field contribution) -- the standard "colored metaball"
    // technique, so overlapping blobs of different temperatures blend smoothly instead
    // of the whole lamp sharing one fixed color.
    float colorR = 0.0f, colorG = 0.0f, colorB = 0.0f;

    for (uint8_t i = 0; i < kBlobCount; ++i) {
        const Blob& blob = blobs[i];

        float pulse = 0.94f + 0.12f * sinf(timeSeconds * blob.pulseRate + blob.phase);
        float heatInflation = 1.0f + 0.10f * Clamp01((-blob.y) / (halfHeight + 0.001f));
        float radius = blob.radius * pulse * heatInflation;

        float centerX = blob.x + sinf(timeSeconds * blob.driftRate + blob.phase) * 1.7f;
        float centerY = blob.y + sinf(timeSeconds * (blob.driftRate * 0.48f) + blob.phase * 1.3f) * 0.7f;
        centerX = Clamp(centerX, -halfWidth + radius + 1.0f, halfWidth - radius - 1.0f);
        centerY = Clamp(centerY, -halfHeight + radius + 1.0f, halfHeight - radius - 1.0f);

        float dx = x - centerX;
        float dy = y - centerY;

        // Squash/stretch scales with current speed, not just direction, so a blob
        // moving fast reads as visibly elongated (classic animation liveliness) while a
        // near-stationary one (e.g. right at a rise/sink flip) looks closer to round.
        float speedFactor = Clamp(fabsf(blob.vy) * 0.030f, 0.0f, 0.28f);
        float stretch = (blob.vy >= 0.0f) ? (1.0f - speedFactor) : (1.0f + speedFactor);

        // Low-frequency angular wobble on top of the isotropic falloff -- deforms the
        // silhouette into an organic, non-circular "amoeba" outline that slowly churns
        // over time, instead of every blob rendering as a plain soft-edged circle.
        // Kept to 2-3 lobes and modest amplitude: at this display's physical pixel pitch,
        // anything higher-frequency or stronger reads as noise rather than a blobby shape.
        float angle = atan2f(dy * stretch, dx);
        float wobble = 1.0f
            + 0.16f * sinf(angle * 2.0f + blob.phase + timeSeconds * 0.5f)
            + 0.10f * sinf(angle * 3.0f - blob.phase * 1.6f + timeSeconds * 0.7f);
        float distSq = (dx * dx + (dy * stretch) * (dy * stretch)) / (wobble * wobble) + radius * 0.85f;
        float blobField = (radius * radius) / distSq;
        field += blobField;

        // Color temperature: a blob's color is derived from where it currently is, not
        // fixed -- hot (bright) near the bottom heat source, cooling toward the dark/
        // hue-shifted end as it rises. Recomputed from centerY every frame so a blob's
        // color genuinely shifts as it rises and falls, matching real lava lamp wax.
        // Contrast-boosted so blobs read as clearly hot/cool well before they reach the
        // very top/bottom edge -- the raw linear version left most blobs (which spend
        // most of their time in the middle third of the lamp) sitting close to a flat
        // 50/50 blend, muting the gradient into a single muddy mid-tone instead of a
        // visible hot/cool contrast.
        float rawHeatT = Clamp01((halfHeight - centerY) / (2.0f * halfHeight));
        float heatT = Clamp01(0.5f + (rawHeatT - 0.5f) * 1.8f);
        RGBColor blobColor = RGBColor::InterpolateColors(coolColor, hotColor, heatT);
        colorR += blobColor.R * blobField;
        colorG += blobColor.G * blobField;
        colorB += blobColor.B * blobField;
    }

    // Small reservoirs to keep classic lava-lamp pooling without dominating the panel --
    // the bottom pool is always hot and the top pool always cooled, matching their fixed
    // positions.
    float bottomY = -halfHeight * 0.80f;
    float bottomRx = halfWidth * 0.38f;
    float bottomRy = halfHeight * 0.09f;
    float bx = x / (bottomRx + 0.001f);
    float by = (y - bottomY) / (bottomRy + 0.001f);
    float bottomField = (1.0f / (bx * bx + by * by + 0.35f)) * 0.28f;
    field += bottomField;
    colorR += hotColor.R * bottomField;
    colorG += hotColor.G * bottomField;
    colorB += hotColor.B * bottomField;

    float topY = halfHeight * 0.80f;
    float topRx = halfWidth * 0.22f;
    float topRy = halfHeight * 0.06f;
    float tx = x / (topRx + 0.001f);
    float ty = (y - topY) / (topRy + 0.001f);
    float topField = (1.0f / (tx * tx + ty * ty + 0.45f)) * 0.08f;
    field += topField;
    colorR += coolColor.R * topField;
    colorG += coolColor.G * topField;
    colorB += coolColor.B * topField;

    float outer = outerThreshold - sinf(timeSeconds * 0.55f) * 0.015f;
    float inner = innerThreshold - sinf(timeSeconds * 0.55f + 0.7f) * 0.02f;
    float core = coreThreshold - sinf(timeSeconds * 0.55f + 1.2f) * 0.025f;

    if (field < outer) {
        return backgroundColor;
    }

    // Normalize the weighted color sum back to a single 0-255 color -- this is the
    // blended "local temperature color" for this pixel, replacing the old fixed
    // shellColor/coreColor.
    float invField = field > 0.0001f ? 1.0f / field : 0.0f;
    RGBColor blendedColor(
        static_cast<uint8_t>(Clamp(colorR * invField, 0.0f, 255.0f)),
        static_cast<uint8_t>(Clamp(colorG * invField, 0.0f, 255.0f)),
        static_cast<uint8_t>(Clamp(colorB * invField, 0.0f, 255.0f)));
    RGBColor shellTone = ScaleColor(blendedColor, 0.62f);

    if (field < inner) {
        float t = SmoothStep((field - outer) / (inner - outer));
        return RGBColor::InterpolateColors(backgroundColor, shellTone, t);
    }

    if (field < core) {
        float t = SmoothStep((field - inner) / (core - inner));
        return RGBColor::InterpolateColors(shellTone, blendedColor, t);
    }

    return LiftColor(blendedColor, 20);
}

