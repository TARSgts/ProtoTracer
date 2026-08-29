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

    blob.radius = 10.0f + Random01() * 8.0f;
    blob.vx = 0.0f;
    blob.vy = 0.0f;
    blob.phase = Random01() * 2.0f * kPi;
    blob.phase2 = Random01() * 2.0f * kPi;
    blob.pulseRate = 0.10f + Random01() * 0.08f;
    blob.wanderFreqA = 0.05f + Random01() * 0.03f;
    blob.wanderFreqB = 0.11f + Random01() * 0.05f;
    blob.rising = Random01() < 0.5f;
    // Full one-way trip covers roughly 2*halfHeight*0.7 or so at this speed -- taking
    // ~15-30s, not a handful of seconds. A real lava lamp is slow and hypnotic.
    blob.riseSpeed = 3.2f + Random01() * 1.8f;
    blob.sinkSpeed = 2.6f + Random01() * 1.6f;
    blob.nextAnchorChangeTime = timeSeconds + 8.0f + Random01() * 10.0f;

    float laneWidth = (halfWidth * 2.0f) / static_cast<float>(kBlobCount);
    float laneCenter = -halfWidth + laneWidth * (static_cast<float>(index) + 0.5f);
    float spanX = halfWidth - blob.radius - 4.0f;
    if (spanX < 2.0f) spanX = 2.0f;
    blob.anchorX = Clamp(laneCenter, -spanX, spanX);
    blob.x = blob.anchorX;

    float spanY = halfHeight - blob.radius - 4.0f;
    if (spanY < 2.0f) spanY = 2.0f;
    blob.y = (Random01() - 0.5f) * spanY * 2.0f;
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

void LavaLampMaterial::ResolveBlobSeparation() {
    for (uint8_t i = 0; i < kBlobCount; ++i) {
        for (uint8_t j = i + 1; j < kBlobCount; ++j) {
            Blob& a = blobs[i];
            Blob& b = blobs[j];

            float dx = b.x - a.x;
            float dy = b.y - a.y;
            float distSq = dx * dx + dy * dy + 0.0001f;
            float dist = sqrtf(distSq);
            // 1.15x (not the classic ~1.0x "solid body" separation) so blobs can overlap
            // enough for their fields to visibly bridge/merge before being pushed apart --
            // this is what makes merging read as fluid coalescence instead of two solid
            // balls bumping into each other.
            float minDist = (a.radius + b.radius) * 1.15f;

            if (dist < minDist) {
                float push = (minDist - dist) * 0.5f;
                float nx = dx / dist;
                float ny = dy / dist;

                a.x -= nx * push;
                a.y -= ny * push;
                b.x += nx * push;
                b.y += ny * push;
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

        // Both thresholds are derived from THIS blob's own radius, with the flip
        // trigger comfortably inside the hard wall (7 units of margin vs. the wall's 4)
        // -- guarantees the flip always fires before the wall, regardless of radius.
        // A fixed fraction of halfHeight is NOT safe here: for this lamp's blob radii
        // (10-18), a fraction-based threshold can sit BEYOND where a large blob can
        // even physically reach, so "rising" would never flip and the blob would drive
        // straight into the wall and stay there forever (caught by simulation before
        // this shipped: some seeds showed a blob frozen at a wall for 15-26+ seconds).
        float topFlip = halfHeight - blob.radius - 7.0f;
        float bottomFlip = -halfHeight + blob.radius + 7.0f;
        if (blob.rising && blob.y > topFlip) {
            blob.rising = false;
        } else if (!blob.rising && blob.y < bottomFlip) {
            blob.rising = true;
        }

        // Ease vy toward a target speed (a simple low-pass filter) instead of the old
        // force+damping model -- much easier to keep BOTH slow and smooth, with no risk
        // of oscillation or overshoot around the target.
        float targetVy = blob.rising ? blob.riseSpeed : -blob.sinkSpeed;
        const float kVerticalEaseRate = 0.6f; // per second; lower = more languid
        blob.vy += (targetVy - blob.vy) * kVerticalEaseRate * dt;

        if (timeSeconds >= blob.nextAnchorChangeTime) {
            float spanX = halfWidth - blob.radius - 4.0f;
            if (spanX < 2.0f) spanX = 2.0f;
            blob.anchorX = Clamp((Random01() - 0.5f) * 2.0f * halfWidth, -spanX, spanX);
            blob.nextAnchorChangeTime = timeSeconds + 8.0f + Random01() * 10.0f;
        }

        // Smooth pseudo-noise (two slow sines at incommensurate frequencies) instead of
        // a per-frame random walk -- real fluid motion drifts smoothly; a per-frame
        // random kick reads as jittery/glitchy rather than alive, no matter how small.
        float noiseX = sinf(timeSeconds * blob.wanderFreqA + blob.phase) * 0.6f
                     + sinf(timeSeconds * blob.wanderFreqB + blob.phase2) * 0.4f;
        float targetVx = (blob.anchorX - blob.x) * 0.05f + noiseX * 0.8f;
        blob.vx += (targetVx - blob.vx) * 4.0f * dt;

        blob.x += blob.vx * dt;
        blob.y += blob.vy * dt;

        float left = -halfWidth + blob.radius + 4.0f;
        float right = halfWidth - blob.radius - 4.0f;
        float bottom = -halfHeight + blob.radius + 4.0f;
        float top = halfHeight - blob.radius - 4.0f;

        // Position-only clamp -- deliberately does NOT touch velocity. Zeroing vy/vx on
        // wall contact (an earlier version of this rework did) could permanently freeze
        // a blob: ResolveBlobSeparation() below runs afterward and can shove an
        // overlapping blob back past a wall on every single frame, so the NEXT frame's
        // clamp would re-fire and wipe the velocity again before it ever built up enough
        // to escape. Confirmed by simulation before this shipped (some seeds showed a
        // multi-second freeze); leaving velocity alone means it keeps easing toward its
        // rise/sink target regardless of position hiccups from nearby blobs.
        if (blob.x > right) {
            blob.x = right;
        } else if (blob.x < left) {
            blob.x = left;
        }
        if (blob.y > top) {
            blob.y = top;
        } else if (blob.y < bottom) {
            blob.y = bottom;
        }
    }

    ResolveBlobSeparation();
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

        // Gentle radius "breathing" -- slow and subtle, not tied to wall proximity
        // (the old heatInflation term compounded with the wall clamp in confusing ways
        // across this file's history; a plain, slow, always-on pulse is simpler and
        // avoids that whole class of bug).
        float pulse = 0.97f + 0.05f * sinf(timeSeconds * blob.pulseRate + blob.phase);
        float radius = blob.radius * pulse;

        float dx = x - blob.x;
        float dy = y - blob.y;

        // Teardrop asymmetry: the edge trailing behind the direction of travel
        // stretches into a tail; the leading edge stays blunt/rounded. This is what
        // actually reads as "molten blob" rather than a squashed ellipse -- real
        // rising/falling wax drips and blobs are asymmetric, not symmetric ovals.
        float speedFactor = Clamp(fabsf(blob.vy) * 0.13f, 0.0f, 0.6f);
        bool movingUp = blob.vy >= 0.0f;
        // dy>0 means the sample point is above the blob's center. If rising, "above" is
        // the leading (blunt) edge and "below" is the trailing tail; if sinking, it's
        // the other way around.
        float stretchAbove = movingUp ? (1.0f + speedFactor * 0.4f) : (1.0f - speedFactor);
        float stretchBelow = movingUp ? (1.0f - speedFactor) : (1.0f + speedFactor * 0.4f);
        float stretch = (dy >= 0.0f) ? stretchAbove : stretchBelow;

        // Low-frequency angular wobble on top of the isotropic falloff -- a subtle,
        // slow-churning organic texture. Kept modest: at this display's physical pixel
        // pitch, anything higher-frequency or stronger reads as noise, not a blobby
        // shape.
        float angle = atan2f(dy * stretch, dx);
        float wobble = 1.0f + 0.10f * sinf(angle * 2.0f + blob.phase + timeSeconds * 0.25f);
        float distSq = (dx * dx + (dy * stretch) * (dy * stretch)) / (wobble * wobble) + radius * 0.85f;
        float blobField = (radius * radius) / distSq;
        field += blobField;

        // Color temperature: a blob's color is derived from where it currently is, not
        // fixed -- hot (bright) near the bottom heat source, cooling toward the dark/
        // hue-shifted end as it rises. Recomputed from blob.y every frame so a blob's
        // color genuinely shifts as it rises and falls, matching real lava lamp wax.
        // Contrast-boosted so blobs read as clearly hot/cool well before they reach the
        // very top/bottom edge -- the raw linear version left most blobs (which spend
        // most of their time in the middle third of the lamp) sitting close to a flat
        // 50/50 blend, muting the gradient into a single muddy mid-tone instead of a
        // visible hot/cool contrast.
        float rawHeatT = Clamp01((halfHeight - blob.y) / (2.0f * halfHeight));
        float heatT = Clamp01(0.5f + (rawHeatT - 0.5f) * 1.8f);
        RGBColor blobColor = RGBColor::InterpolateColors(coolColor, hotColor, heatT);
        colorR += blobColor.R * blobField;
        colorG += blobColor.G * blobField;
        colorB += blobColor.B * blobField;
    }

    // Small reservoirs to keep classic lava-lamp pooling without dominating the panel --
    // the bottom pool is always hot and the top pool always cooled, matching their fixed
    // positions.
    float bottomY = -halfHeight * 0.82f;
    float bottomRx = halfWidth * 0.42f;
    float bottomRy = halfHeight * 0.10f;
    float bx = x / (bottomRx + 0.001f);
    float by = (y - bottomY) / (bottomRy + 0.001f);
    float bottomField = (1.0f / (bx * bx + by * by + 0.35f)) * 0.30f;
    field += bottomField;
    colorR += hotColor.R * bottomField;
    colorG += hotColor.G * bottomField;
    colorB += hotColor.B * bottomField;

    float topY = halfHeight * 0.82f;
    float topRx = halfWidth * 0.24f;
    float topRy = halfHeight * 0.07f;
    float tx = x / (topRx + 0.001f);
    float ty = (y - topY) / (topRy + 0.001f);
    float topField = (1.0f / (tx * tx + ty * ty + 0.45f)) * 0.09f;
    field += topField;
    colorR += coolColor.R * topField;
    colorG += coolColor.G * topField;
    colorB += coolColor.B * topField;

    if (field < outerThreshold) {
        return backgroundColor;
    }

    // Normalize the weighted color sum back to a single 0-255 color -- this is the
    // blended "local temperature color" for this pixel.
    float invField = field > 0.0001f ? 1.0f / field : 0.0f;
    RGBColor blendedColor(
        static_cast<uint8_t>(Clamp(colorR * invField, 0.0f, 255.0f)),
        static_cast<uint8_t>(Clamp(colorG * invField, 0.0f, 255.0f)),
        static_cast<uint8_t>(Clamp(colorB * invField, 0.0f, 255.0f)));
    RGBColor shellTone = ScaleColor(blendedColor, 0.62f);

    if (field < innerThreshold) {
        float t = SmoothStep((field - outerThreshold) / (innerThreshold - outerThreshold));
        return RGBColor::InterpolateColors(backgroundColor, shellTone, t);
    }

    if (field < coreThreshold) {
        float t = SmoothStep((field - innerThreshold) / (coreThreshold - innerThreshold));
        return RGBColor::InterpolateColors(shellTone, blendedColor, t);
    }

    return LiftColor(blendedColor, 20);
}
