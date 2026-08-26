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

    blob.radius = 5.5f + Random01() * 3.7f;
    blob.vx = (Random01() - 0.5f) * 1.5f;
    blob.vy = 3.0f + Random01() * 3.8f;
    blob.phase = Random01() * 2.0f * kPi;
    blob.pulseRate = 0.20f + Random01() * 0.34f;
    blob.driftRate = 0.24f + Random01() * 0.44f;

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
    if (energy < 12) {
        backgroundColor = RGBColor(0, 0, 0);
        shellColor = RGBColor(150, 55, 18);
        coreColor = RGBColor(240, 145, 55);
        return;
    }

    backgroundColor = RGBColor(0, 0, 0);
    shellColor = LiftColor(ScaleColor(baseColor, 0.62f), 12);
    coreColor = LiftColor(ScaleColor(baseColor, 1.0f), 42);
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

        float yNorm = blob.y / (halfHeight + 0.001f);
        float bottomHeat = Clamp01((-yNorm - 0.10f) / 0.90f);
        float topCool = Clamp01((yNorm - 0.15f) / 0.85f);
        float thermalWave = sinf(timeSeconds * (0.55f + blob.pulseRate) + blob.phase) * 0.32f;
        float buoyancy = 7.0f * bottomHeat - 6.4f * topCool + thermalWave;
        float sizeDrag = (blob.radius - 7.0f) * 0.10f;

        blob.vy += (buoyancy - sizeDrag) * dt;
        blob.vy *= (1.0f - 0.07f * dt);
        blob.vy = Clamp(blob.vy, -6.8f, 6.8f);

        float springToLane = (blob.anchorX - blob.x) * 0.9f;
        float sideDrift = sinf(timeSeconds * blob.driftRate + blob.phase) * 0.42f;
        blob.vx += (springToLane + sideDrift) * dt;
        blob.vx *= (1.0f - 0.06f * dt);
        blob.vx = Clamp(blob.vx, -2.2f, 2.2f);

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

        if (fabsf(blob.vy) < 0.9f && (blob.y > top * 0.90f || blob.y < bottom * 0.90f)) {
            blob.vy += (blob.y >= 0.0f) ? -0.5f : 0.5f;
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
        float stretch = (blob.vy >= 0.0f) ? 0.82f : 1.08f;
        float distSq = dx * dx + (dy * stretch) * (dy * stretch) + radius * 0.85f;
        field += (radius * radius) / distSq;
    }

    // Small reservoirs to keep classic lava-lamp pooling without dominating the panel.
    float bottomY = -halfHeight * 0.80f;
    float bottomRx = halfWidth * 0.38f;
    float bottomRy = halfHeight * 0.09f;
    float bx = x / (bottomRx + 0.001f);
    float by = (y - bottomY) / (bottomRy + 0.001f);
    field += (1.0f / (bx * bx + by * by + 0.35f)) * 0.28f;

    float topY = halfHeight * 0.80f;
    float topRx = halfWidth * 0.22f;
    float topRy = halfHeight * 0.06f;
    float tx = x / (topRx + 0.001f);
    float ty = (y - topY) / (topRy + 0.001f);
    field += (1.0f / (tx * tx + ty * ty + 0.45f)) * 0.08f;

    float outer = outerThreshold - sinf(timeSeconds * 0.55f) * 0.015f;
    float inner = innerThreshold - sinf(timeSeconds * 0.55f + 0.7f) * 0.02f;
    float core = coreThreshold - sinf(timeSeconds * 0.55f + 1.2f) * 0.025f;

    if (field < outer) {
        return backgroundColor;
    }

    if (field < inner) {
        float t = SmoothStep((field - outer) / (inner - outer));
        return RGBColor::InterpolateColors(backgroundColor, shellColor, t);
    }

    if (field < core) {
        float t = SmoothStep((field - inner) / (core - inner));
        return RGBColor::InterpolateColors(shellColor, coreColor, t);
    }

    return LiftColor(coreColor, 20);
}

