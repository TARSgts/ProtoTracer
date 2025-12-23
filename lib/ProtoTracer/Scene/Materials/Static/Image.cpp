#include "Image.h"

Image::Image(const uint8_t* data, const uint8_t* rgbColors, unsigned int xPixels, unsigned int yPixels, uint8_t colors) {
    this->data = data;
    this->rgbColors = rgbColors;
    this->xPixels = xPixels;
    this->yPixels = yPixels;
    this->colors = colors;
}

void Image::SetData(const uint8_t* data) {
    this->data = data;
}

void Image::SetColorPalette(const uint8_t* rgbColors) {
    this->rgbColors = rgbColors;
}

void Image::SetSize(Vector2D size) {
    this->size = size;
}

void Image::SetPosition(Vector2D offset) {
    this->offset = offset;
}

void Image::SetRotation(float angle) {
    this->angle = angle;
}

void Image::SetHueAngle(float hueAngle) {
    this->hueAngle = hueAngle;
}

void Image::SetTintColor(const RGBColor& color) {
    tintColor = color;
    tintEnabled = true;
}

void Image::DisableTint() {
    tintEnabled = false;
}

RGBColor Image::GetRGB(const Vector3D& position, const Vector3D& normal, const Vector3D& uvw) {
    Vector2D rPos = angle != 0.0f ? Vector2D(position.X, position.Y).Rotate(angle, offset) - offset : Vector2D(position.X, position.Y) - offset;

    unsigned int x = (unsigned int)Mathematics::Map(rPos.X, size.X / -2.0f, size.X / 2.0f, float(xPixels), 0.0f);
    unsigned int y = (unsigned int)Mathematics::Map(rPos.Y, size.Y / -2.0f, size.Y / 2.0f, float(yPixels), 0.0f);

    if (x <= 1 || x >= xPixels || y <= 1 || y >= yPixels) return RGBColor();

    uint8_t colorIndex = data[x + y * xPixels];
    if (colorIndex >= colors) return RGBColor();
    unsigned int pos = static_cast<unsigned int>(colorIndex) * 3;

    RGBColor sample(rgbColors[pos], rgbColors[pos + 1], rgbColors[pos + 2]);

    if (tintEnabled) {
        sample.R = static_cast<uint8_t>((static_cast<uint16_t>(sample.R) * tintColor.R) / 255);
        sample.G = static_cast<uint8_t>((static_cast<uint16_t>(sample.G) * tintColor.G) / 255);
        sample.B = static_cast<uint8_t>((static_cast<uint16_t>(sample.B) * tintColor.B) / 255);
    }

    return sample.HueShift(hueAngle);
}
