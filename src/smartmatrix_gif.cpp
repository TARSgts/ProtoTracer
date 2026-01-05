#if defined(PROJECT_SMARTMATRIX_GIF)
#include <Arduino.h>

#include "Controller/SmartMatrixHUB75.h"
#include <GifDecoder.h>
#include "FilenameFunctions.h"

#define DISPLAY_TIME_SECONDS 10
#define NUMBER_FULL_CYCLES 100
#define ENABLE_SCROLLING 0

const int defaultBrightness = 255;
const rgb24 COLOR_BLACK = {0, 0, 0};

SMARTMATRIX_ALLOCATE_BUFFERS(matrix, kMatrixWidth, kMatrixHeight, kRefreshDepth, kDmaBufferRows, kPanelType, kMatrixOptions);
SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(backgroundLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kBackgroundLayerOptions);
#if (ENABLE_SCROLLING == 1)
const uint8_t kScrollingLayerOptions = (SM_SCROLLING_OPTIONS_NONE);
SMARTMATRIX_ALLOCATE_SCROLLING_LAYER(scrollingLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kScrollingLayerOptions);
#endif

GifDecoder<kMatrixWidth, kMatrixHeight, 12> decoder;

#if defined(ESP32)
#define SD_CS 5
#define GIF_DIRECTORY "/gifs"
#else
#define SD_CS BUILTIN_SDCARD
#define GIF_DIRECTORY "/gifs/"
#endif

int numFiles = 0;

void screenClearCallback(void) {
    backgroundLayer.fillScreen(COLOR_BLACK);
}

void updateScreenCallback(void) {
    backgroundLayer.swapBuffers();
}

void drawPixelCallback(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue) {
    if (x < 0 || y < 0 || x >= kMatrixWidth || y >= kMatrixHeight) {
        return;
    }
    backgroundLayer.drawPixel(x, y, {red, green, blue});
}

void setup() {
    decoder.setScreenClearCallback(screenClearCallback);
    decoder.setUpdateScreenCallback(updateScreenCallback);
    decoder.setDrawPixelCallback(drawPixelCallback);

    decoder.setFileSeekCallback(fileSeekCallback);
    decoder.setFilePositionCallback(filePositionCallback);
    decoder.setFileReadCallback(fileReadCallback);
    decoder.setFileReadBlockCallback(fileReadBlockCallback);
    decoder.setFileSizeCallback(fileSizeCallback);

    Serial.begin(115200);
    delay(1000);
    Serial.println("Starting SmartMatrix GIF player");

    matrix.addLayer(&backgroundLayer);
#if (ENABLE_SCROLLING == 1)
    matrix.addLayer(&scrollingLayer);
#endif
    matrix.setBrightness(defaultBrightness);
    matrix.begin();

    backgroundLayer.fillScreen(COLOR_BLACK);
    backgroundLayer.swapBuffers();

    if (initFileSystem(SD_CS) < 0) {
#if (ENABLE_SCROLLING == 1)
        scrollingLayer.start("No SD card", -1);
#endif
        Serial.println("No SD card");
        while (1) { delay(100); }
    }

    numFiles = enumerateGIFFiles(GIF_DIRECTORY, true);
    if (numFiles < 0) {
#if (ENABLE_SCROLLING == 1)
        scrollingLayer.start("No gifs directory", -1);
#endif
        Serial.println("No gifs directory");
        while (1) { delay(100); }
    }

    if (!numFiles) {
#if (ENABLE_SCROLLING == 1)
        scrollingLayer.start("Empty gifs directory", -1);
#endif
        Serial.println("Empty gifs directory");
        while (1) { delay(100); }
    }
}

void loop() {
    static unsigned long displayStartTimeMs = 0;
    static int nextGif = 1;
    static int index = 0;

    unsigned long now = millis();

    if ((now - displayStartTimeMs) > (DISPLAY_TIME_SECONDS * 1000UL) || decoder.getCycleNumber() > NUMBER_FULL_CYCLES) {
        nextGif = 1;
    }

    if (nextGif) {
        nextGif = 0;

        if (openGifFilenameByIndex(GIF_DIRECTORY, index) >= 0) {
            if (decoder.startDecoding() < 0) {
                nextGif = 1;
                return;
            }
            displayStartTimeMs = now;
        }

        if (++index >= numFiles) {
            index = 0;
        }
    }

    if (decoder.decodeFrame() < 0) {
        nextGif = 1;
    }
}
#endif
