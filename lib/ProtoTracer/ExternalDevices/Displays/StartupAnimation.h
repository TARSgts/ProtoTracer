#pragma once
#include <Arduino.h>
#include "../../Assets/Textures/Startup/SteamDeckStartup.h"

// A one-shot, time-based player. Decodes at most one 64x32 frame per update;
// delayed updates skip old frames instead of delaying normal input/audio work.
class StartupAnimation {
public:
    static bool DecodeRle(const uint8_t* data, size_t size, uint8_t* output, size_t pixels) {
        if (!data || !output || size % 2) return false;
        size_t written = 0;
        for (size_t pos = 0; pos < size; pos += 2) {
            const uint8_t count = data[pos];
            if (!count || count > pixels - written) return false;
            for (uint8_t n = 0; n < count; ++n) output[written++] = data[pos + 1];
        }
        return written == pixels;
    }

    bool Update(uint32_t now) {
        if (finished) return false;
        if (!started) { started = true; startedAt = now; }
        const uint32_t elapsed = now - startedAt; // Safe across millis() wrap.
        if (elapsed >= StartupClip::kDurationMs) { finished = true; return false; }
        const uint16_t index = elapsed * StartupClip::kFramesPerSecond / 1000;
        if (index != frameIndex) {
            if (index >= StartupClip::kFrameCount) return Fail();
            const uint32_t begin = StartupClip::kOffsets[index];
            const uint32_t end = StartupClip::kOffsets[index + 1];
            if (end < begin || end > sizeof(StartupClip::kData) ||
                !DecodeRle(StartupClip::kData + begin, end - begin, frame, sizeof(frame))) return Fail();
            frameIndex = index;
            ++decodedFrames;
        }
        return true;
    }

    const uint8_t* GetFrame() const { return frame; }
    uint16_t GetFrameIndex() const { return frameIndex; }
    uint16_t GetDecodedFrames() const { return decodedFrames; }
    bool IsStarted() const { return started; }
    bool IsFinished() const { return finished; }
    bool HasFailed() const { return failed; }

private:
    bool Fail() { failed = true; finished = true; return false; }
    uint8_t frame[StartupClip::kPixelCount] = {};
    uint32_t startedAt = 0;
    uint16_t frameIndex = StartupClip::kFrameCount;
    uint16_t decodedFrames = 0;
    bool started = false, finished = false, failed = false;
};
