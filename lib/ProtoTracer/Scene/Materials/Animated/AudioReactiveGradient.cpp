#include "AudioReactiveGradient.h"
#include <Arduino.h>
#include "../../../ExternalDevices/Sensors/Microphone/Utils/AudioFrame.h"

AudioReactiveGradient::AudioReactiveGradient(Vector2D size, Vector2D offset, bool bounce, bool circular) {
    this->size = size.Divide(2.0f);
    this->offset = offset;
    this->bounce = bounce;
    this->circular = circular;
    this->material = &gM;

    if (bounce && !circular) {
        for (uint8_t i = 0; i < 128; i++) {
            bPhy[i] = new BouncePhysics(35.0f, 15.0f);
        }
    }
}

AudioReactiveGradient::~AudioReactiveGradient() {
    for (uint8_t i = 0; i < 128; i++) {
        delete bPhy[i];
    }
}

void AudioReactiveGradient::SetMaterial(Material* material) {
    this->material = material;
}

float* AudioReactiveGradient::GetFourierData() {
    if (bounce || circular) {
        return bounceData;
    } else {
        return data;
    }
}

void AudioReactiveGradient::SetSize(Vector2D size) {
    this->size = size.Divide(2.0f);
}

void AudioReactiveGradient::SetPosition(Vector2D offset) {
    this->offset = offset;
}

void AudioReactiveGradient::SetRotation(float angle) {
    this->angle = angle;
}

void AudioReactiveGradient::SetHueAngle(float hueAngle) {
    this->hueAngle = hueAngle;
}

void AudioReactiveGradient::SetRadius(float radius) {
    this->radius = radius;
}

void AudioReactiveGradient::Update(float* readData) {
    data = readData;

    if (circular) {
        const uint32_t now = millis();
        const uint32_t elapsed = now - lastUpdateMillis;
        lastUpdateMillis = now;
        // Immediate attack, 80 ms release; no ten-frame average or bounce overshoot.
        const float decay = expf(-float(elapsed) / 80.0f);
        for (uint8_t i = 0; i < bins; ++i) {
            const float target = data ? AudioFrame::Unit(data[i]) : 0.0f;
            bounceData[i] = bounce && target < bounceData[i]
                                ? target + (bounceData[i] - target) * decay
                                : target;
        }
        for (uint8_t spoke = 0; spoke < SpokeCount; ++spoke) {
            float peak = 0.0f;
            for (uint8_t bin = 0; bin < bins / SpokeCount; ++bin) {
                peak = Mathematics::Max(peak, bounceData[spoke * (bins / SpokeCount) + bin]);
            }
            spokeData[spoke] = sqrtf(peak);
        }
        return;
    }

    for (uint8_t i = 0; i < 128; i++) {
        if (bounce) {
            bounceData[i] = bPhy[i]->Calculate(data ? data[i] : 0.0f, 0.1f);
        } else {
            bounceData[i] = data ? data[i] : 0.0f;
        }
    }
}

RGBColor AudioReactiveGradient::GetRGB(const Vector3D& position, const Vector3D& normal, const Vector3D& uvw) {
    if (!data || size.X <= 0.0f || size.Y <= 0.0f) return RGBColor();
    Vector2D rPos = Mathematics::IsClose(angle, 0.0f, 0.1f) ? Vector2D(position.X, position.Y) - offset : Vector2D(position.X, position.Y).Rotate(angle, offset) - offset;

    // Outside of size bounds
    if (rPos.X < -size.X || rPos.X > size.X) return RGBColor();
    if (rPos.Y < -size.Y || rPos.Y > size.Y) return RGBColor();

    if (circular) {
        const float distance = sqrtf(rPos.X * rPos.X + rPos.Y * rPos.Y);
        const float diskRadius = Mathematics::Min(size.X, size.Y);
        if (distance > diskRadius) return RGBColor();

        // One turn covers all bins. Wrap the last interpolation pair at the seam.
        float mapped = (0.5f - atan2f(rPos.Y, rPos.X) / (2.0f * Mathematics::MPI)) * float(bins);
        if (mapped >= float(bins)) mapped = 0.0f;
        const uint8_t left = static_cast<uint8_t>(mapped);
        const uint8_t right = (left + 1) % bins;
        const float body = sqrtf(AudioFrame::Unit(Mathematics::CosineInterpolation(
            bounceData[left], bounceData[right], mapped - float(left))));
        const float spokeMapped = mapped * float(SpokeCount) / float(bins);
        const uint8_t spoke = static_cast<uint8_t>(spokeMapped);
        const float tip = 1.0f - fabsf(2.0f * (spokeMapped - float(spoke)) - 1.0f);
        // A continuous body joins 32 tapered frequency spikes. Pool four bins
        // per spoke so narrow spectral peaks remain visible on the small panel.
        const float height = AudioFrame::Unit(body * 0.60f + spokeData[spoke] * tip * 0.40f);

        // Scale the audio movement to this canvas. The old height*150 swallowed
        // the entire disk at ordinary levels on the template's 50-90 unit canvas.
        const float baseCenter = Mathematics::Constrain(radius - 5.0f, diskRadius * 0.25f, diskRadius * 0.75f);
        // Let the radius pulse too; width alone changed by less than a pixel on
        // moderate beats. The square-root curve makes quiet beats visible.
        const float center = baseCenter * (0.40f + 0.60f * height);
        const float maxThickness = Mathematics::Min(center * 0.70f, diskRadius * 0.95f - center);
        if (fabsf(distance - center) >= height * maxThickness) return RGBColor();

        const float yColor = 1.0f - distance / size.Y;
        return material->GetRGB(Vector3D(1.0f - height - yColor, 0, 0), Vector3D(), Vector3D()).HueShift(hueAngle);
    }

    // Convert to polar coordinates
    float tX = rPos.X;
    rPos.X = atan2f(rPos.Y, rPos.X) / (2.0f * Mathematics::MPI) * size.Y;
    rPos.Y = sqrtf(tX * tX + rPos.Y * rPos.Y);

    // Clamped to [0, bins-2] (not just [0, bins-1]) since data[x+1]/bounceData[x+1] below
    // reads one bin ahead for interpolation -- letting x reach bins-1 would read past the
    // end of the 128-element array.
    float xMapped = Mathematics::Map(rPos.X, -size.X, size.X, float(bins), 0.0f);
    uint8_t x = static_cast<uint8_t>(Mathematics::Constrain(xMapped, 0.0f, float(bins - 2)));

    float xDistance = size.X / float(bins) * x - size.X;
    float xDistance2 = size.X / float(bins) * (x + 1) - size.X;
    float ratio = Mathematics::Map(rPos.X, xDistance, xDistance2, 0.0f, 1.0f); // ratio between two bins
    float height = bounce ? Mathematics::CosineInterpolation(bounceData[x], bounceData[x + 1], ratio) : Mathematics::CosineInterpolation(data[x], data[x + 1], ratio); // 0->1.0f of max height of color

    float yColor = Mathematics::Map(rPos.Y, 0.0f, size.Y, 1.0f, 0.0f);

    float inside = 1.0f - (height * 4.0f + 0.15f) - yColor;

    if (inside < 0.0f) {
        return material->GetRGB(Vector3D(1.0f - height - yColor, 0, 0), Vector3D(), Vector3D()).HueShift(hueAngle);
    } else {
        return RGBColor(0, 0, 0);
    }
}
