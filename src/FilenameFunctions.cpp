/*
 * Animated GIF filename helpers adapted from the SmartMatrix AnimatedGifs example.
 */

#if defined(PROJECT_SMARTMATRIX_GIF)
#include "FilenameFunctions.h"

#include <Arduino.h>
#include <SD.h>

static File file;
static int numberOfFiles;

bool fileSeekCallback(unsigned long position) {
    return file.seek(position);
}

unsigned long filePositionCallback(void) {
    return file.position();
}

int fileReadCallback(void) {
    return file.read();
}

int fileReadBlockCallback(void* buffer, int numberOfBytes) {
    return file.read(static_cast<uint8_t*>(buffer), numberOfBytes);
}

int fileSizeCallback(void) {
    return file.size();
}

int initFileSystem(int chipSelectPin) {
    if (chipSelectPin >= 0) {
        pinMode(chipSelectPin, OUTPUT);
    }
    if (!SD.begin(chipSelectPin)) {
        return -1;
    }
    return 0;
}

static bool isAnimationFile(const char filename[]) {
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

int enumerateGIFFiles(const char* directoryName, bool displayFilenames) {
    numberOfFiles = 0;

    File directory = SD.open(directoryName);
    if (!directory) {
        return -1;
    }

    File next;
    while (next = directory.openNextFile()) {
        if (isAnimationFile(next.name())) {
            ++numberOfFiles;
            if (displayFilenames) {
                Serial.print(numberOfFiles);
                Serial.print(":");
                Serial.print(next.name());
                Serial.print("    size:");
                Serial.println(next.size());
            }
        } else if (displayFilenames) {
            Serial.println(next.name());
        }
        next.close();
    }

    directory.close();
    return numberOfFiles;
}

void getGIFFilenameByIndex(const char* directoryName, int index, char* pnBuffer) {
    if ((index < 0) || (index >= numberOfFiles)) {
        return;
    }

    File directory = SD.open(directoryName);
    if (!directory) {
        return;
    }

    while (index >= 0) {
        file = directory.openNextFile();
        if (!file) break;

        if (isAnimationFile(file.name())) {
            --index;

#if defined(ESP32)
            pnBuffer[0] = 0;
#else
            strcpy(pnBuffer, directoryName);
            int len = strlen(pnBuffer);
            if (len == 0 || pnBuffer[len - 1] != '/') {
                strcat(pnBuffer, "/");
            }
#endif

            strcat(pnBuffer, file.name());
        }

        file.close();
    }

    file.close();
    directory.close();
}

int openGifFilenameByIndex(const char* directoryName, int index) {
    char pathname[255];

    getGIFFilenameByIndex(directoryName, index, pathname);

    Serial.print("Pathname: ");
    Serial.println(pathname);

    if (file) {
        file.close();
    }

    file = SD.open(pathname);
    if (!file) {
        Serial.println("Error opening GIF file");
        return -1;
    }

    return 0;
}
#endif
