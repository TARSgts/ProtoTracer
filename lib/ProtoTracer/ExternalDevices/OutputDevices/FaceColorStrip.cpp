#include "FaceColorStrip.h"

#include <math.h>

#ifndef FACE_COLOR_STRIP_MAX_LENGTH
#define FACE_COLOR_STRIP_MAX_LENGTH 60
#endif

static_assert(FACE_COLOR_STRIP_MAX_LENGTH > 0, "FACE_COLOR_STRIP_MAX_LENGTH must be greater than zero");

DMAMEM static uint8_t faceColorStripDisplayMemory[FACE_COLOR_STRIP_MAX_LENGTH * 12] = {};
static uint8_t faceColorStripDrawingMemory[FACE_COLOR_STRIP_MAX_LENGTH * 3] = {};

FaceColorStrip::FaceColorStrip(uint8_t pin, uint16_t ledCount, uint8_t brightness)
    : ledCount(ledCount),
      brightness(brightness) {
    if (this->ledCount > FACE_COLOR_STRIP_MAX_LENGTH) {
        this->ledCount = FACE_COLOR_STRIP_MAX_LENGTH;
    }

    strip = new WS2812Serial(this->ledCount,
                             faceColorStripDisplayMemory,
                             faceColorStripDrawingMemory,
                             pin,
                             WS2812_GRB);
}

FaceColorStrip::~FaceColorStrip() {
    if (strip != nullptr) {
        delete strip;
        strip = nullptr;
    }
}

void FaceColorStrip::Initialize() {
    if (strip == nullptr) return;

    strip->begin();
    strip->show();
    initialized = true;
    usingSolidColor = false;
    usingGradient = false;
    stripDirty = false;
    lastSolidColor = RGBColor();
    lastGradientStart = RGBColor();
    lastGradientEnd = RGBColor();
}

RGBColor FaceColorStrip::ApplyBrightness(const RGBColor& color) const {
    if (brightness >= 255) return color;

    RGBColor adjusted = color;
    adjusted.R = uint8_t((uint16_t(color.R) * brightness) / 255);
    adjusted.G = uint8_t((uint16_t(color.G) * brightness) / 255);
    adjusted.B = uint8_t((uint16_t(color.B) * brightness) / 255);
    return adjusted;
}

void FaceColorStrip::FillStrip(const RGBColor& color) {
    if (!initialized || strip == nullptr) return;

    const RGBColor scaled = ApplyBrightness(color);
    for (uint16_t i = 0; i < ledCount; ++i) {
        strip->setPixel(i, scaled.R, scaled.G, scaled.B);
    }
    stripDirty = true;
}

void FaceColorStrip::WriteBuffer() {
    if (!initialized || strip == nullptr || !stripDirty) return;
    strip->show();
    stripDirty = false;
}

void FaceColorStrip::SetSolidColor(const RGBColor& color) {
    if (!initialized) return;

    if (usingSolidColor &&
        color.R == lastSolidColor.R &&
        color.G == lastSolidColor.G &&
        color.B == lastSolidColor.B) {
        return;
    }

    usingSolidColor = true;
    usingGradient = false;
    lastSolidColor = color;

    FillStrip(color);
    WriteBuffer();
}

void FaceColorStrip::SetGradient(const RGBColor& start, const RGBColor& end) {
    if (!initialized || strip == nullptr) return;

    if (usingGradient &&
        start.R == lastGradientStart.R &&
        start.G == lastGradientStart.G &&
        start.B == lastGradientStart.B &&
        end.R == lastGradientEnd.R &&
        end.G == lastGradientEnd.G &&
        end.B == lastGradientEnd.B) {
        return;
    }

    usingSolidColor = false;
    usingGradient = true;
    lastGradientStart = start;
    lastGradientEnd = end;

    const float denom = ledCount > 1 ? float(ledCount - 1) : 1.0f;

    for (uint16_t i = 0; i < ledCount; ++i) {
        const float ratio = (ledCount == 1) ? 0.0f : float(i) / denom;
        const RGBColor color = RGBColor::InterpolateColors(start, end, ratio);
        const RGBColor scaled = ApplyBrightness(color);
        strip->setPixel(i, scaled.R, scaled.G, scaled.B);
    }

    stripDirty = true;
    WriteBuffer();
}

void FaceColorStrip::ShowRainbow(float animationRatio, float speedMultiplier) {
    if (!initialized || strip == nullptr) return;

    usingSolidColor = false;
    usingGradient = false;

    const float rolling = fmodf(animationRatio * speedMultiplier, 1.0f);
    const float hueStep = ledCount > 0 ? 360.0f / float(ledCount) : 0.0f;

    for (uint16_t i = 0; i < ledCount; ++i) {
        const float hue = fmodf(rolling * 360.0f + hueStep * i, 360.0f);
        RGBColor color = RGBColor(255, 0, 0).HueShift(hue);
        color = ApplyBrightness(color);
        strip->setPixel(i, color.R, color.G, color.B);
    }

    stripDirty = true;
    WriteBuffer();
}

void FaceColorStrip::Clear() {
    if (!initialized) return;

    usingSolidColor = false;
    usingGradient = false;
    FillStrip(RGBColor(0, 0, 0));
    WriteBuffer();
}
