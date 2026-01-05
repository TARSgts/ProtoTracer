#include "SmartMatrixGifPlayer.h"

#ifdef ENABLE_GIF_FACE
static bool GetName(SdFileType& f, char* out, size_t outSize) {
    if (!out || outSize == 0) return false;
#ifdef GIF_SD_USE_SDFAT
    return f.getName(out, outSize) > 0;
#else
    const char* n = f.name();
    if (!n) return false;
    strncpy(out, n, outSize - 1);
    out[outSize - 1] = '\0';
    return true;
#endif
}

SmartMatrixGifPlayer* SmartMatrixGifPlayer::instance = nullptr;
#ifdef GIF_SD_USE_SDFAT
SdFat SmartMatrixGifPlayer::sd;
#endif
SdFileType SmartMatrixGifPlayer::file;
DMAMEM GifDecoder<kMatrixWidth, kMatrixHeight, 12> SmartMatrixGifPlayer::decoder;

void SmartMatrixGifPlayer::Initialize() {
    instance = this;

    decoder.setScreenClearCallback(ScreenClearCallback);
    decoder.setUpdateScreenCallback(UpdateScreenCallback);
    decoder.setDrawPixelCallback(DrawPixelCallback);

    decoder.setFileSeekCallback(FileSeekCallback);
    decoder.setFilePositionCallback(FilePositionCallback);
    decoder.setFileReadCallback(FileReadCallback);
    decoder.setFileReadBlockCallback(FileReadBlockCallback);
    decoder.setFileSizeCallback(FileSizeCallback);

    Serial.println("GIF: Initialize");
}

void SmartMatrixGifPlayer::SetActive(bool enabled) {
    if (active == enabled) return;
    active = enabled;

    if (!active) {
        decoding = false;
        if (file) {
            file.close();
        }
    } else {
        nextGif = true;
        displayStartTimeMs = 0;
        decoding = false;
        lastOpenAttemptMs = 0;
    }
}

bool SmartMatrixGifPlayer::IsActive() const {
    return active;
}

bool SmartMatrixGifPlayer::IsDecoding() const {
    return decoding;
}

void SmartMatrixGifPlayer::Update() {
    if (!active) {
        decoding = false;
        return;
    }
    if (!EnsureReady()) {
        decoding = false;
        return;
    }

    unsigned long now = millis();
    if (decoding && ((now - displayStartTimeMs) > kDisplayTimeMs || decoder.getCycleNumber() > kNumberFullCycles)) {
        decoding = false;
        nextGif = true;
    }

    if (nextGif) {
        if (now - lastOpenAttemptMs < kOpenRetryMs) {
            return;
        }
        lastOpenAttemptMs = now;
        lastFrameDecodeMs = now;
        nextGif = false;
        if (OpenGifByIndex(index) >= 0) {
            if (decoder.startDecoding() < 0) {
                Serial.println("GIF: startDecoding failed");
                decoding = false;
                nextGif = true;
                return;
            }
            UpdateGifSize();
            displayStartTimeMs = now;
            decoding = true;
        } else {
            decoding = false;
        }

        if (++index >= numFiles) {
            index = 0;
        }
    }

    if (!decoding) {
        return;
    }

    if (now - lastFrameDecodeMs < kMinFrameMs) {
        return;
    }
    lastFrameDecodeMs = now;

    if (decoder.decodeFrame() < 0) {
        Serial.println("GIF: decodeFrame failed");
        decoding = false;
        nextGif = true;
    }
}

bool SmartMatrixGifPlayer::EnsureReady() {
    if (ready) return true;

    unsigned long now = millis();
    if (now - lastInitAttemptMs < kInitRetryMs) return false;
    lastInitAttemptMs = now;

    #ifdef GIF_SD_USE_SDFAT
    // Teensy 4.x SDIO
    delay(200); // brief settle
    if (!sd.begin(SdioConfig(FIFO_SDIO))) {
        Serial.println("GIF: SdFat SDIO begin failed");
        return false;
    }
    #else
    // Only poke CS if we're using an SPI SD slot. BUILTIN_SDCARD uses SDIO and
    // does not want pinMode on that value.
    if (kSdCs >= 0 && kSdCs != BUILTIN_SDCARD) {
        pinMode(kSdCs, OUTPUT);
    }

    if (!SD.begin(kSdCs)) {
        Serial.println("GIF: SD.begin failed");
        return false;
    }
    #endif

    numFiles = EnumerateGifs(false);
    if (numFiles <= 0) {
        Serial.println("GIF: no GIF files found in /gifs/");
        return false;
    }

    Serial.print("GIF: ready, files=");
    Serial.println(numFiles);
    ready = true;
    nextGif = true;
    index = 0;
    return true;
}

int SmartMatrixGifPlayer::EnumerateGifs(bool displayFilenames) {
    int count = 0;
    #ifdef GIF_SD_USE_SDFAT
    SdFileType directory = sd.open(kGifDirectory);
    #else
    SdFileType directory = SD.open(kGifDirectory);
    #endif
    if (!directory) {
        Serial.println("GIF: cannot open /gifs/");
        return -1;
    }

    SdFileType next;
    while (next = directory.openNextFile()) {
        char nameBuf[64] = {0};
        if (!GetName(next, nameBuf, sizeof(nameBuf))) {
            next.close();
            continue;
        }

        if (IsAnimationFile(nameBuf)) {
            ++count;
            if (displayFilenames) {
                Serial.print(count);
                Serial.print(":");
                Serial.print(nameBuf);
                Serial.print("    size:");
                Serial.println(next.size());
            }
        } else if (displayFilenames) {
            Serial.println(nameBuf);
        }
        next.close();
    }

    directory.close();
    return count;
}

int SmartMatrixGifPlayer::OpenGifByIndex(int targetIndex) {
    if (targetIndex < 0 || targetIndex >= numFiles) {
        return -1;
    }

    #ifdef GIF_SD_USE_SDFAT
    SdFileType directory = sd.open(kGifDirectory);
    #else
    SdFileType directory = SD.open(kGifDirectory);
    #endif
    if (!directory) {
        Serial.println("GIF: failed to open /gifs/");
        return -1;
    }

    char pathname[255] = {0};
    int remaining = targetIndex;
    bool found = false;

    while (remaining >= 0) {
        file = directory.openNextFile();
        if (!file) break;

        char nameBuf[64] = {0};
        if (!GetName(file, nameBuf, sizeof(nameBuf))) {
            file.close();
            continue;
        }

        if (IsAnimationFile(nameBuf)) {
            --remaining;

#if defined(ESP32)
            pathname[0] = 0;
#else
            strcpy(pathname, kGifDirectory);
            int len = strlen(pathname);
            if (len == 0 || pathname[len - 1] != '/') {
                strcat(pathname, "/");
            }
#endif
            strcat(pathname, nameBuf);
            found = true;
        }

        file.close();
    }

    file.close();
    directory.close();

    if (!found) {
        Serial.println("GIF: target GIF not found while iterating");
        return -1;
    }

    if (file) {
        file.close();
    }

    #ifdef GIF_SD_USE_SDFAT
    file = sd.open(pathname);
    #else
    file = SD.open(pathname);
    #endif
    if (!file) {
        Serial.print("GIF: failed to open file ");
        Serial.println(pathname);
        return -1;
    }

    Serial.print("GIF: opened ");
    Serial.println(pathname);
    return 0;
}

void SmartMatrixGifPlayer::ScreenClearCallback() {
    backgroundLayer.fillScreen({0, 0, 0});
}

void SmartMatrixGifPlayer::UpdateScreenCallback() {
    // Use blocking swap to reduce visible tearing during GIF playback.
    backgroundLayer.swapBuffers(true);
}

void SmartMatrixGifPlayer::DrawPixelCallback(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue) {
    if (!instance) return;
    instance->DrawScaledPixel(x, y, red, green, blue);
}

bool SmartMatrixGifPlayer::FileSeekCallback(unsigned long position) {
    return file.seek(position);
}

unsigned long SmartMatrixGifPlayer::FilePositionCallback(void) {
    return file.position();
}

int SmartMatrixGifPlayer::FileReadCallback(void) {
    return file.read();
}

int SmartMatrixGifPlayer::FileReadBlockCallback(void* buffer, int numberOfBytes) {
    return file.read(static_cast<uint8_t*>(buffer), numberOfBytes);
}

int SmartMatrixGifPlayer::FileSizeCallback(void) {
    return file.size();
}

bool SmartMatrixGifPlayer::IsAnimationFile(const char* filename) {
    String filenameString(filename);

#if defined(ESP32)
    int pathindex = filenameString.lastIndexOf("/");
    if (pathindex >= 0) {
        filenameString.remove(0, pathindex + 1);
    }
#endif

    if ((filenameString[0] == '_') || (filenameString[0] == '~') || (filenameString[0] == '.')) {
        return false;
    }

    filenameString.toUpperCase();
    return filenameString.endsWith(".GIF") == 1;
}

void SmartMatrixGifPlayer::UpdateGifSize() {
    uint16_t width = 0;
    uint16_t height = 0;
    decoder.getSize(&width, &height);
    if (width == 0 || height == 0) {
        gifWidth = kMatrixWidth;
        gifHeight = kMatrixHeight;
    } else {
        gifWidth = width;
        gifHeight = height;
    }
    targetWidth = 64;
    targetHeight = 32;

    Serial.print("GIF: size ");
    Serial.print(gifWidth);
    Serial.print("x");
    Serial.println(gifHeight);
}

void SmartMatrixGifPlayer::DrawScaledPixel(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue) {
    if (x < 0 || y < 0) return;
    if (gifWidth == 0 || gifHeight == 0 || targetWidth == 0 || targetHeight == 0) return;

    uint16_t srcX = static_cast<uint16_t>(x);
    uint16_t srcY = static_cast<uint16_t>(y);
    if (srcX >= gifWidth || srcY >= gifHeight) return;

    uint32_t destX0 = (static_cast<uint32_t>(srcX) * targetWidth) / gifWidth;
    uint32_t destX1 = (static_cast<uint32_t>(srcX + 1) * targetWidth) / gifWidth;
    uint32_t destY0 = (static_cast<uint32_t>(srcY) * targetHeight) / gifHeight;
    uint32_t destY1 = (static_cast<uint32_t>(srcY + 1) * targetHeight) / gifHeight;

    if (destX1 <= destX0) destX1 = destX0 + 1;
    if (destY1 <= destY0) destY1 = destY0 + 1;

    if (destX0 >= targetWidth || destY0 >= targetHeight) return;
    if (destX1 > targetWidth) destX1 = targetWidth;
    if (destY1 > targetHeight) destY1 = targetHeight;

    const rgb24 color = {red, green, blue};
    // Fast path: 1:1 mapping, no block fill needed.
    if ((destX1 - destX0 == 1) && (destY1 - destY0 == 1)) {
        uint16_t dx = static_cast<uint16_t>(destX0);
        uint16_t dy = static_cast<uint16_t>(destY0);
        backgroundLayer.drawPixel(static_cast<int16_t>(dx), static_cast<int16_t>(dy), color);
        if (kDuplicateToBothPanels) {
            backgroundLayer.drawPixel(static_cast<int16_t>((kMatrixWidth - 1) - dx),
                                      static_cast<int16_t>(dy + targetHeight),
                                      color);
        }
        return;
    }

    for (uint32_t yy = destY0; yy < destY1; ++yy) {
        for (uint32_t xx = destX0; xx < destX1; ++xx) {
            backgroundLayer.drawPixel(static_cast<int16_t>(xx),
                                      static_cast<int16_t>(yy),
                                      color);
            if (kDuplicateToBothPanels) {
                backgroundLayer.drawPixel(static_cast<int16_t>((kMatrixWidth - 1) - xx),
                                          static_cast<int16_t>(yy + targetHeight),
                                          color);
            }
        }
    }
}

#endif
