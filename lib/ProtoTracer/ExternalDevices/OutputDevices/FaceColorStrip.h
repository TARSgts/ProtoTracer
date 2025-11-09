/**
 * @file FaceColorStrip.h
 * @brief Thin wrapper that mirrors the active face color on an external WS2812 strip.
 */

#pragma once

#include <Arduino.h>
#include <WS2812Serial.h>

#include "../../Utils/RGBColor.h"

/**
 * @class FaceColorStrip
 * @brief Provides gradient/rainbow helpers backed by the non-blocking WS2812Serial driver.
 */
class FaceColorStrip {
public:
    FaceColorStrip(uint8_t pin, uint16_t ledCount, uint8_t brightness = 255);
    ~FaceColorStrip();

    void Initialize();
    void SetSolidColor(const RGBColor& color);
    void SetGradient(const RGBColor& start, const RGBColor& end);
    void ShowRainbow(float animationRatio, float speedMultiplier = 1.0f);
    void Clear();

private:
    WS2812Serial* strip = nullptr;
    uint16_t ledCount;
    uint8_t brightness;
    bool initialized = false;
    bool usingSolidColor = false;
    bool usingGradient = false;
    bool stripDirty = false;
    RGBColor lastSolidColor;
    RGBColor lastGradientStart;
    RGBColor lastGradientEnd;

    RGBColor ApplyBrightness(const RGBColor& color) const;
    void FillStrip(const RGBColor& color);
    void WriteBuffer();
};
