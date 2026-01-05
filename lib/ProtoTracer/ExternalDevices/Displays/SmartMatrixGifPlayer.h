#pragma once

#include "Arduino.h"
#include "../../Examples/UserConfiguration.h"
#include "../../Controller/SmartMatrixHUB75.h"
#include <GifDecoder.h>
#include <SD.h> // default path; optionally use SdFat if GIF_SD_USE_SDFAT is defined

using SdFileType = File;

#ifdef ENABLE_GIF_FACE

class SmartMatrixGifPlayer {
public:
    void Initialize();
    void SetActive(bool enabled);
    void Update();
    bool IsActive() const;
    bool IsDecoding() const;

private:
    static SmartMatrixGifPlayer* instance;
    #ifdef GIF_SD_USE_SDFAT
    static SdFat sd;
    #endif
    static SdFileType file;

    static GifDecoder<kMatrixWidth, kMatrixHeight, 12> decoder;
    bool active = false;
    bool ready = false;
    bool decoding = false;
    bool nextGif = true;
    unsigned long lastInitAttemptMs = 0;
    unsigned long lastOpenAttemptMs = 0;
    unsigned long displayStartTimeMs = 0;
    unsigned long lastFrameDecodeMs = 0;
    int numFiles = 0;
    int index = 0;
    uint16_t gifWidth = kMatrixWidth;
    uint16_t gifHeight = kMatrixHeight;
    uint16_t targetWidth = 64;   // render to a single 64x32 panel
    uint16_t targetHeight = 32;  // panel height
    static constexpr bool kDuplicateToBothPanels = true; // draw same GIF on both 64x32 panels

    static constexpr unsigned long kInitRetryMs = 2000;
    static constexpr unsigned long kOpenRetryMs = 500;
    static constexpr unsigned long kDisplayTimeMs = 600000; // allow long clips (10 minutes)
    static constexpr int kNumberFullCycles = 1000;          // plenty of loops before advancing
    static constexpr unsigned long kMinFrameMs = 30; // throttle decode for smoother playback

#if defined(ESP32)
    static constexpr int kSdCs = 5;
    static constexpr const char* kGifDirectory = "/gifs";
#else
    static constexpr int kSdCs = BUILTIN_SDCARD;
    static constexpr const char* kGifDirectory = "/gifs/";
#endif

    bool EnsureReady();
    int EnumerateGifs(bool displayFilenames);
    int OpenGifByIndex(int index);

    static void ScreenClearCallback();
    static void UpdateScreenCallback();
    static void DrawPixelCallback(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue);

    static bool FileSeekCallback(unsigned long position);
    static unsigned long FilePositionCallback(void);
    static int FileReadCallback(void);
    static int FileReadBlockCallback(void* buffer, int numberOfBytes);
    static int FileSizeCallback(void);

    static bool IsAnimationFile(const char* filename);

    void UpdateGifSize();
    void DrawScaledPixel(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue);
};

#endif
