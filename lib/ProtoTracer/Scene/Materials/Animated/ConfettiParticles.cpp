#include "ConfettiParticles.h"

#include <Arduino.h>
#include <cmath>

#include "../../../Utils/Math/Mathematics.h"

namespace {
constexpr float kMaxDelta = 0.05f;
}

ConfettiParticles::ConfettiParticles(Vector2D dimensions, Vector2D center)
    : size(dimensions.Divide(2.0f)),
      offset(center) {}

void ConfettiParticles::SetSize(Vector2D dimensions) {
    size = dimensions.Divide(2.0f);
}

void ConfettiParticles::SetPosition(Vector2D center) {
    offset = center;
}

RGBColor ConfettiParticles::ScaleColor(const RGBColor& color, float scale) const {
    if (scale <= 0.0f) return RGBColor();
    if (scale > 1.0f) scale = 1.0f;
    return RGBColor(
        static_cast<uint8_t>(color.R * scale),
        static_cast<uint8_t>(color.G * scale),
        static_cast<uint8_t>(color.B * scale)
    );
}

void ConfettiParticles::SpawnParticle(const Vector2D& origin, const Vector2D& direction) {
    Particle& particle = particles[nextIndex];
    nextIndex = (nextIndex + 1) % kMaxParticles;

    float speed = float(random(45, 90));
    particle.position = origin;
    particle.velocity = direction.UnitCircle().Multiply(speed);
    particle.radius = float(random(2, 4));
    particle.maxLife = float(random(60, 110)) / 100.0f;
    particle.life = particle.maxLife;
    particle.color = palette[random(0, 6)];
    particle.active = true;
}

void ConfettiParticles::Trigger(const Vector2D& origin, const Vector2D& direction, float spreadDeg, uint8_t count) {
    if (count == 0) return;
    Vector2D baseDirection = direction.Magnitude() > 0.0f ? direction.UnitCircle() : Vector2D(0.0f, 1.0f);
    float halfSpread = spreadDeg * 0.5f;

    for (uint8_t i = 0; i < count; i++) {
        float angleDeg = float(random(-int(halfSpread), int(halfSpread) + 1));
        float angleRad = Mathematics::DegreesToRadians(angleDeg);
        float cosA = cosf(angleRad);
        float sinA = sinf(angleRad);
        Vector2D rotated(
            baseDirection.X * cosA - baseDirection.Y * sinA,
            baseDirection.X * sinA + baseDirection.Y * cosA
        );
        SpawnParticle(origin, rotated);
    }
}

void ConfettiParticles::Update() {
    uint32_t now = millis();
    if (lastUpdateMs == 0) {
        lastUpdateMs = now;
        return;
    }

    float delta = (now - lastUpdateMs) / 1000.0f;
    if (delta > kMaxDelta) delta = kMaxDelta;
    lastUpdateMs = now;

    for (uint8_t i = 0; i < kMaxParticles; i++) {
        Particle& particle = particles[i];
        if (!particle.active) continue;

        particle.velocity = particle.velocity.Multiply(drag);
        particle.velocity.Y += gravity * delta;
        particle.position = particle.position + particle.velocity * delta;
        particle.life -= delta;
        if (particle.life <= 0.0f) {
            particle.active = false;
        }
    }
}

RGBColor ConfettiParticles::GetRGB(const Vector3D& position, const Vector3D& /*normal*/, const Vector3D& /*uvw*/) {
    Vector2D relative(position.X, position.Y);
    relative = relative - offset;

    if (relative.X < -size.X || relative.X > size.X || relative.Y < -size.Y || relative.Y > size.Y) {
        return RGBColor();
    }

    RGBColor output;
    float bestAlpha = 0.0f;

    for (uint8_t i = 0; i < kMaxParticles; i++) {
        const Particle& particle = particles[i];
        if (!particle.active) continue;

        float dx = relative.X - particle.position.X;
        float dy = relative.Y - particle.position.Y;
        float dist2 = dx * dx + dy * dy;
        float radius2 = particle.radius * particle.radius;
        if (dist2 <= radius2) {
            float alpha = particle.life / particle.maxLife;
            if (alpha > bestAlpha) {
                bestAlpha = alpha;
                output = ScaleColor(particle.color, alpha);
            }
        }
    }

    return output;
}
