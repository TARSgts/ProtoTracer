#include "AudioReactiveGradient.h"

AudioReactiveGradient::AudioReactiveGradient(Vector2D size, Vector2D offset, bool bounce, bool circular) {
    this->size = size.Divide(2.0f);
    this->offset = offset;
    this->bounce = bounce;
    this->circular = circular;
    this->material = &gM;

    if (bounce) {
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
    if (bounce) {
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

    for (uint8_t i = 0; i < 128; i++) {
        if (bounce) {
            bounceData[i] = bPhy[i]->Calculate(data[i], 0.1f);
        } else {
            bounceData[i] = data[i];
        }
    }
}

RGBColor AudioReactiveGradient::GetRGB(const Vector3D& position, const Vector3D& normal, const Vector3D& uvw) {
    Vector2D rPos = Mathematics::IsClose(angle, 0.0f, 0.1f) ? Vector2D(position.X, position.Y) - offset : Vector2D(position.X, position.Y).Rotate(angle, offset) - offset;

    // Outside of size bounds
    if (rPos.X < -size.X || rPos.X > size.X) return RGBColor();
    if (rPos.Y < -size.Y || rPos.Y > size.Y) return RGBColor();

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

    if (!circular && inside < 0.0f) {
        return material->GetRGB(Vector3D(1.0f - height - yColor, 0, 0), Vector3D(), Vector3D()).HueShift(hueAngle);
    } else if (circular && rPos.Y + height * 150.0f > radius - 5.0f && rPos.Y - height * 150.0f < radius - 5.0f) {
        if (rPos.Y - height * 50.0f > 75.0f) {
            return material->GetRGB(Vector3D(1.0f + height - yColor, 0, 0), Vector3D(), Vector3D()).HueShift(hueAngle);
        } else if (rPos.Y - height * 50.0f < 125.0f) {
            return material->GetRGB(Vector3D(1.0f - height - yColor, 0, 0), Vector3D(), Vector3D()).HueShift(hueAngle);
        } else {
            return RGBColor(0, 0, 0);
        }
    } else {
        return RGBColor(0, 0, 0);
    }
}
