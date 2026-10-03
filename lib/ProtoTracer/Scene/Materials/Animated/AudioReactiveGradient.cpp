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
        // The active template supplies the accepted spectrum analyzer's processed
        // data. Do not add another bounce, temporal filter or whole-circle pulse.
        for (uint8_t i = 0; i < bins; ++i) {
            bounceData[i] = data ? AudioFrame::Unit(data[i]) : 0.0f;
        }
        for (uint8_t bar = 0; bar < BarCount; ++bar) {
            float peak = 0.0f;
            for (uint8_t bin = 0; bin < bins / BarCount; ++bin) {
                peak = Mathematics::Max(peak, bounceData[bar * (bins / BarCount) + bin]);
            }
            barData[bar] = peak;
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

        // Bend the spectrum's horizontal bins around a fixed inner circle.
        // Flat-topped, separate radial bars follow their own frequency levels.
        float mapped = (0.5f - atan2f(rPos.Y, rPos.X) / (2.0f * Mathematics::MPI)) * float(BarCount);
        if (mapped >= float(BarCount)) mapped = 0.0f;
        const uint8_t bar = static_cast<uint8_t>(mapped);
        const float fraction = mapped - float(bar);
        const float inner = Mathematics::Constrain(radius * 0.50f, diskRadius * 0.32f, diskRadius * 0.46f);
        const float rimHalfWidth = diskRadius * 0.025f;
        if (distance < inner - rimHalfWidth) return RGBColor();
        if (distance > inner + rimHalfWidth) {
            if (fraction < 0.075f || fraction > 0.925f) return RGBColor();
            const float outer = inner + barData[bar] * (diskRadius * 0.93f - inner);
            if (distance > outer) return RGBColor();
        }
        return material->GetRGB(Vector3D(mapped / float(BarCount), 0, 0), Vector3D(), Vector3D()).HueShift(hueAngle);
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
