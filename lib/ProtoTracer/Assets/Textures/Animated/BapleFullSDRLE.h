#pragma once

#include "Arduino.h"
#include "SD.h"
#include "../../../Scene/Materials/Material.h"
#include "../../../Utils/Math/Vector2D.h"
#include "../../../Utils/Math/Mathematics.h"

#ifndef BAD_APPLE_SD_BOOT_DELAY_MS
#define BAD_APPLE_SD_BOOT_DELAY_MS 2000
#endif

class BapleFullSequenceSD : public Material {
private:
    const char* path;
    File file;
    bool sdReady = false;
    bool fileReady = false;
    bool active = false;
    bool frameReady = false;
    unsigned long lastSdAttemptMs = 0;
    unsigned long nextRetryMs = 0;
    unsigned long bootReadyMs = 0;
    bool warnedNoCard = false;
    bool warnedSdInit = false;
    bool warnedNoFile = false;
    bool warnedReadFail = false;

    Vector2D size;
    Vector2D offset;
    float angle = 0.0f;

    unsigned int imageCount = 3110;
    float fps = 18.0f;
    float frameTime = 0.0f;
    unsigned int currentFrame = 0;
    unsigned long lastFrameMs = 0;

    static constexpr unsigned int kWidth = 64;
    static constexpr unsigned int kHeight = 32;
    static constexpr unsigned int kFrameBytes = (kWidth * kHeight) / 8;
    static constexpr unsigned long kBootDelayMs = BAD_APPLE_SD_BOOT_DELAY_MS;

    uint8_t frameBits[kFrameBytes];

    bool IsCardPresent() {
#if defined(ARDUINO_TEENSY41)
        constexpr uint8_t kDetectPin = 46;
#elif defined(ARDUINO_TEENSY40)
        constexpr uint8_t kDetectPin = 38;
#elif defined(ARDUINO_TEENSY_MICROMOD)
        constexpr uint8_t kDetectPin = 39;
#else
        return true;
#endif
        pinMode(kDetectPin, INPUT_PULLDOWN);
        return digitalRead(kDetectPin) != 0;
    }

    void LogOnce(bool& flag, const char* message) {
        if (flag) return;
        Serial.println(message);
        flag = true;
    }

    void ResetWarnings() {
        warnedNoCard = false;
        warnedSdInit = false;
        warnedNoFile = false;
        warnedReadFail = false;
    }

    void CloseFile() {
        if (file) {
            file.close();
        }
        fileReady = false;
    }

    bool EnsureSD() {
        if (bootReadyMs != 0 && millis() < bootReadyMs) return false;
        if (sdReady) return true;
        unsigned long now = millis();
        bool present = IsCardPresent();
        if (!present) {
            CloseFile();
            sdReady = false;
            frameReady = false;
            LogOnce(warnedNoCard, "SD: card not detected");
            nextRetryMs = now + 2000;
            return false;
        } else {
            warnedNoCard = false;
        }
        if (now < nextRetryMs) return false;
        if (now - lastSdAttemptMs < 1000) return false;
        lastSdAttemptMs = now;
        sdReady = SD.begin(BUILTIN_SDCARD);
        if (!sdReady) {
            nextRetryMs = now + 2000;
            frameReady = false;
            LogOnce(warnedSdInit, "SD: init failed");
        }
        if (sdReady) {
            warnedSdInit = false;
        }
        return sdReady;
    }

    bool EnsureFile() {
        if (fileReady) return true;
        unsigned long now = millis();
        if (now < nextRetryMs) return false;
        if (!EnsureSD()) return false;
        file = SD.open(path, FILE_READ);
        fileReady = file;
        if (!fileReady) {
            nextRetryMs = millis() + 2000;
            frameReady = false;
            LogOnce(warnedNoFile, "SD: open failed");
        }
        if (fileReady) {
            warnedNoFile = false;
        }
        return fileReady;
    }

    bool ReadByte(uint8_t& out) {
        int value = file.read();
        if (value < 0) {
            CloseFile();
            sdReady = false;
            frameReady = false;
            LogOnce(warnedReadFail, "SD: read failed");
            return false;
        }
        out = static_cast<uint8_t>(value);
        return true;
    }

    bool ReadFrameRle() {
        if (!EnsureFile()) return false;
        unsigned int decoded = 0;
        while (decoded < kFrameBytes) {
            uint8_t ctrl = 0;
            if (!ReadByte(ctrl)) return false;

            if (ctrl & 0x80) {
                unsigned int count = static_cast<unsigned int>(ctrl & 0x7F) + 1;
                uint8_t value = 0;
                if (!ReadByte(value)) return false;
                while (count-- && decoded < kFrameBytes) {
                    frameBits[decoded++] = value;
                }
            } else {
                unsigned int count = static_cast<unsigned int>(ctrl) + 1;
                while (count-- && decoded < kFrameBytes) {
                    uint8_t value = 0;
                    if (!ReadByte(value)) return false;
                    frameBits[decoded++] = value;
                }
            }
        }

        warnedReadFail = false;
        frameReady = true;
        return true;
    }

    bool SeekStart() {
        if (!EnsureFile()) return false;
        if (!file.seek(0)) return false;
        return true;
    }

public:
    BapleFullSequenceSD(const char* path, Vector2D size, Vector2D offset, float fps)
        : path(path) {
        SetSize(size);
        SetPosition(offset);
        SetFPS(fps);
    }

    void SetFPS(float fps) {
        this->fps = fps;
        frameTime = fps > 0.0f ? (1000.0f / fps) : 0.0f;
    }

    void SetSize(Vector2D size) {
        this->size = size;
    }

    void SetPosition(Vector2D offset) {
        this->offset = offset;
    }

    void SetRotation(float angle) {
        this->angle = angle;
    }

    void SetActive(bool enabled) {
        if (active == enabled) return;
        active = enabled;
        if (!active) {
            CloseFile();
            frameReady = false;
        } else {
            if (kBootDelayMs > 0) {
                bootReadyMs = millis() + kBootDelayMs;
            } else {
                bootReadyMs = 0;
            }
            Reset();
            ResetWarnings();
        }
    }

    void Reset() {
        currentFrame = 0;
        lastFrameMs = 0;
        frameReady = false;
        memset(frameBits, 0, sizeof(frameBits));
    }

    bool HasFrame() const {
        return frameReady;
    }

    void Update() {
        if (!active) return;
        if (frameTime <= 0.0f) return;
        if (!fileReady && !EnsureFile()) return;

        unsigned long now = millis();
        if (lastFrameMs == 0) {
            lastFrameMs = now;
            if (!SeekStart()) return;
            if (!ReadFrameRle()) return;
            currentFrame = 0;
            return;
        }

        unsigned long elapsed = now - lastFrameMs;
        if (elapsed < static_cast<unsigned long>(frameTime)) return;

        unsigned int framesToAdvance = static_cast<unsigned int>(elapsed / frameTime);
        if (framesToAdvance >= imageCount) {
            framesToAdvance %= imageCount;
            if (framesToAdvance == 0) framesToAdvance = 1;
        }

        lastFrameMs += static_cast<unsigned long>(framesToAdvance * frameTime);

        for (unsigned int step = 0; step < framesToAdvance; ++step) {
            currentFrame++;
            if (currentFrame >= imageCount) {
                currentFrame = 0;
                if (!SeekStart()) return;
            }

            if (!ReadFrameRle()) return;
        }
    }

    RGBColor GetRGB(const Vector3D& position, const Vector3D& normal, const Vector3D& uvw) override {
        if (!active || !frameReady) return RGBColor();
        (void)normal;
        (void)uvw;
        Vector2D rPos = angle != 0.0f ? Vector2D(position.X, position.Y).Rotate(angle, offset) - offset
                                        : Vector2D(position.X, position.Y) - offset;

        unsigned int x = (unsigned int)Mathematics::Map(rPos.X, size.X / -2.0f, size.X / 2.0f, float(kWidth), 0.0f);
        unsigned int y = (unsigned int)Mathematics::Map(rPos.Y, size.Y / -2.0f, size.Y / 2.0f, float(kHeight), 0.0f);

        if (x <= 1 || x >= kWidth || y <= 1 || y >= kHeight) return RGBColor();

        uint32_t index = x + y * kWidth;
        uint8_t byteValue = frameBits[index >> 3];
        bool bit = (byteValue >> (7 - (index & 7))) & 0x1;
        return bit ? RGBColor(255, 255, 255) : RGBColor(0, 0, 0);
    }
};
