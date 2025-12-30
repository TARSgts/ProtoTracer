#pragma once

#include "Arduino.h"
#include "SD.h"
#include "../../../Scene/Materials/Material.h"
#include "../../../Utils/Math/Vector2D.h"
#include "../../../Utils/Math/Mathematics.h"

class BapleFullSequenceSD : public Material {
private:
    const char* path;
    File file;
    bool sdReady = false;
    bool fileReady = false;
    bool active = false;
    unsigned long lastSdAttemptMs = 0;
    unsigned long nextRetryMs = 0;

    Vector2D size;
    Vector2D offset;
    float angle = 0.0f;

    unsigned long startTime = 0;
    unsigned int imageCount = 3110;
    float fps = 18.0f;
    float frameTime = 0.0f;
    unsigned int currentFrame = 0;
    unsigned int nextFrame = 1;
    float frameFrac = 0.0f;
    const uint8_t* cachedFrameA = nullptr;
    const uint8_t* cachedFrameB = nullptr;

    static constexpr unsigned int kWidth = 64;
    static constexpr unsigned int kHeight = 32;
    static constexpr unsigned int kFrameBytes = (kWidth * kHeight) / 8;

    uint8_t cache[2][kFrameBytes];
    unsigned int cacheIndex[2] = {0xFFFFFFFFu, 0xFFFFFFFFu};

    bool EnsureSD() {
        if (sdReady) return true;
        if (!SD.mediaPresent()) return false;
        unsigned long now = millis();
        if (now < nextRetryMs) return false;
        if (now - lastSdAttemptMs < 1000) return false;
        lastSdAttemptMs = now;
        sdReady = SD.begin(BUILTIN_SDCARD);
        if (!sdReady) {
            nextRetryMs = now + 2000;
        }
        return sdReady;
    }

    bool EnsureFile() {
        if (fileReady) return true;
        if (!EnsureSD()) return false;
        file = SD.open(path, FILE_READ);
        fileReady = file;
        if (!fileReady) {
            nextRetryMs = millis() + 2000;
        }
        return fileReady;
    }

    bool LoadFrame(unsigned int index, uint8_t* dest) {
        if (!EnsureFile()) return false;
        uint32_t offsetBytes = static_cast<uint32_t>(index) * kFrameBytes;
        if (!file.seek(offsetBytes)) return false;
        return file.read(dest, kFrameBytes) == static_cast<int>(kFrameBytes);
    }

    const uint8_t* GetFrame(unsigned int index) {
        if (cacheIndex[0] == index) return cache[0];
        if (cacheIndex[1] == index) return cache[1];

        if (LoadFrame(index, cache[0])) {
            cacheIndex[0] = index;
            return cache[0];
        }
        return nullptr;
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
            cachedFrameA = nullptr;
            cachedFrameB = nullptr;
        } else {
            Reset();
        }
    }

    void Reset() {
        startTime = millis();
        currentFrame = 0;
        nextFrame = 1;
        frameFrac = 0.0f;
        cacheIndex[0] = 0xFFFFFFFFu;
        cacheIndex[1] = 0xFFFFFFFFu;
        cachedFrameA = nullptr;
        cachedFrameB = nullptr;
    }

    void Update() {
        if (!active) return;
        if (frameTime <= 0.0f) return;
        if (!fileReady && !EnsureFile()) return;
        if (startTime == 0) startTime = millis();
        unsigned long elapsed = millis() - startTime;

        currentFrame = static_cast<unsigned int>((elapsed / frameTime)) % imageCount;
        nextFrame = (currentFrame + 1) % imageCount;
        frameFrac = static_cast<float>(elapsed % static_cast<unsigned long>(frameTime)) / frameTime;

        // Preload current and next frames into cache.
        if (cacheIndex[0] != currentFrame && cacheIndex[1] != currentFrame) {
            if (!LoadFrame(currentFrame, cache[0])) return;
            cacheIndex[0] = currentFrame;
        }
        if (cacheIndex[0] != nextFrame && cacheIndex[1] != nextFrame) {
            if (!LoadFrame(nextFrame, cache[1])) return;
            cacheIndex[1] = nextFrame;
        }

        cachedFrameA = GetFrame(currentFrame);
        cachedFrameB = GetFrame(nextFrame);
    }

    RGBColor GetRGB(const Vector3D& position, const Vector3D& normal, const Vector3D& uvw) override {
        if (!active) return RGBColor();
        (void)normal;
        (void)uvw;
        Vector2D rPos = angle != 0.0f ? Vector2D(position.X, position.Y).Rotate(angle, offset) - offset
                                        : Vector2D(position.X, position.Y) - offset;

        unsigned int x = (unsigned int)Mathematics::Map(rPos.X, size.X / -2.0f, size.X / 2.0f, float(kWidth), 0.0f);
        unsigned int y = (unsigned int)Mathematics::Map(rPos.Y, size.Y / -2.0f, size.Y / 2.0f, float(kHeight), 0.0f);

        if (x <= 1 || x >= kWidth || y <= 1 || y >= kHeight) return RGBColor();

        const uint8_t* frameA = cachedFrameA;
        const uint8_t* frameB = cachedFrameB;
        if (!frameA || !frameB) return RGBColor();

        uint32_t index = x + y * kWidth;
        uint8_t byteA = frameA[index >> 3];
        uint8_t byteB = frameB[index >> 3];
        bool bitA = (byteA >> (7 - (index & 7))) & 0x1;
        bool bitB = (byteB >> (7 - (index & 7))) & 0x1;

        bool bit = bitA;
        if (bitA != bitB) {
            uint8_t pattern = static_cast<uint8_t>(((x & 1) << 1) | (y & 1));
            float threshold = (static_cast<float>(pattern) + 0.5f) * 0.25f;
            bit = (frameFrac >= threshold) ? bitB : bitA;
        }

        return bit ? RGBColor(255, 255, 255) : RGBColor(0, 0, 0);
    }
};
