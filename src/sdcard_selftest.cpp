#ifdef PROJECT_SD_SELFTEST
#include <Arduino.h>
#include <SD.h>
#include <SdFat.h>

// Try the Teensy 4.1 built-in SD slot first, then a few common SPI CS pins.
static const int kCandidates[] = {
    BUILTIN_SDCARD, // Teensy 4.1 underside slot (SDIO)
    10,             // Common CS for shield/board SPI SD sockets
    4,              // Alternate CS sometimes used on shields
};

static void PrintFlashID() {
#ifdef TEENSYDUINO
    // Simple sanity: print the Teensy flash ID to prove USB is up.
    Serial.print("Chip ID: 0x");
    Serial.println(HW_OCOTP_CFG1, HEX);
#endif
}

static void ListRoot() {
    File root = SD.open("/");
    if (!root) {
        Serial.println("  open / failed");
        return;
    }

    File f;
    while (f = root.openNextFile()) {
        Serial.print("  ");
        Serial.print(f.name());
        if (f.isDirectory()) {
            Serial.print("/");
        } else {
            Serial.print(" size=");
            Serial.print(f.size());
            Serial.print(" bytes");
        }
        Serial.println();
        f.close();
    }
    root.close();
}

static void TryMount(int cs) {
    Serial.print("Trying SD.begin with CS=");
    if (cs == BUILTIN_SDCARD) {
        Serial.println("BUILTIN_SDCARD");
    } else {
        Serial.println(cs);
    }

    // Avoid poking SDIO with pinMode; only set OUTPUT for SPI CS pins.
    if (cs != BUILTIN_SDCARD && cs >= 0) {
        pinMode(cs, OUTPUT);
    }

    bool ok = SD.begin(cs);
    if (!ok) {
        Serial.println("  SD.begin FAILED");
        // If builtin SDIO failed and we're on Teensy, try SdFat directly.
        #if defined(ARDUINO_TEENSY41) || defined(__IMXRT1062__)
        if (cs == BUILTIN_SDCARD) {
            SdFat sd;
            delay(750);
            Serial.println("  Trying SdFat.begin(SdioConfig(FIFO_SDIO))");
            if (sd.begin(SdioConfig(FIFO_SDIO))) {
                Serial.println("  SdFat SDIO OK");
                File root = sd.open("/");
                if (root) {
                    File f;
                    while (f = root.openNextFile()) {
                        Serial.print("  ");
                        Serial.print(f.name());
                        if (f.isDirectory()) {
                            Serial.println("/");
                        } else {
                            Serial.print(" size=");
                            Serial.print(f.size());
                            Serial.println(" bytes");
                        }
                        f.close();
                    }
                    root.close();
                }
                return;
            } else {
                Serial.println("  SdFat SDIO FAILED");
            }
        }
        #endif
        return;
    }

    Serial.println("  SD.begin OK");
    ListRoot();
}

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 4000) {
        // wait for USB serial
    }

    Serial.println();
    Serial.println("=== SD Self-Test ===");
    Serial.println("Tries BUILTIN_SDCARD then common SPI CS pins.");
    PrintFlashID();

    for (unsigned i = 0; i < sizeof(kCandidates) / sizeof(kCandidates[0]); ++i) {
        TryMount(kCandidates[i]);
    }

    Serial.println("Waiting 5s between retries...");
}

void loop() {
    static unsigned long last = 0;
    unsigned long now = millis();
    if (now - last >= 5000) {
        last = now;
        Serial.println();
        Serial.println("=== SD Self-Test retry ===");
        for (unsigned i = 0; i < sizeof(kCandidates) / sizeof(kCandidates[0]); ++i) {
            TryMount(kCandidates[i]);
        }
    }
}
#endif
