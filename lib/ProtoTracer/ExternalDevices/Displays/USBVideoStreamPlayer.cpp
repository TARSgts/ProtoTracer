#include "USBVideoStreamPlayer.h"

#ifdef ENABLE_USB_VIDEO_FACE

void USBVideoStreamPlayer::Initialize() {
    if (!serialReady) {
        Serial.begin(kSerialBaud);
        serialReady = true;
    }
    streamModeRequested = false;
    streamLive = false;
    lastFrameMs = 0;
    lastSignalMs = 0;
    ResetParser();
}

bool USBVideoStreamPlayer::IsStreamingModeActive() const {
    if (!streamModeRequested) return false;

    uint32_t now = millis();
    if (streamLive && (now - lastFrameMs) <= kFrameTimeoutMs) {
        return true;
    }

    return (lastSignalMs != 0) && ((now - lastSignalMs) <= kModeTimeoutMs);
}

bool USBVideoStreamPlayer::IsRendering() const {
    if (!streamModeRequested || !streamLive) return false;
    return (millis() - lastFrameMs) <= kFrameTimeoutMs;
}

void USBVideoStreamPlayer::Update() {
    if (!serialReady) {
        Serial.begin(kSerialBaud);
        serialReady = true;
    }

    while (Serial.available() > 0) {
        int incoming = Serial.read();
        if (incoming < 0) break;
        ProcessIncomingByte(static_cast<uint8_t>(incoming));
    }

    uint32_t now = millis();
    if (streamLive && (now - lastFrameMs) > kFrameTimeoutMs) {
        streamLive = false;
        backgroundLayer.fillScreen({0, 0, 0});
        backgroundLayer.swapBuffers(false);
    }

    if (streamModeRequested && (now - lastSignalMs) > kModeTimeoutMs) {
        DeactivateStreamingMode(false);
    }
}

void USBVideoStreamPlayer::ResetParser() {
    rxState = RxState::WaitMagic0;
    headerIndex = 0;
    payloadIndex = 0;
    expectedPayloadLength = 0;
}

void USBVideoStreamPlayer::ProcessIncomingByte(uint8_t value) {
    switch (rxState) {
        case RxState::WaitMagic0:
            if (value == kMagic0) {
                rxState = RxState::WaitMagic1;
            }
            break;

        case RxState::WaitMagic1:
            if (value == kMagic1) {
                headerIndex = 0;
                rxState = RxState::ReadHeader;
            } else if (value != kMagic0) {
                rxState = RxState::WaitMagic0;
            }
            break;

        case RxState::ReadHeader:
            header[headerIndex++] = value;
            if (headerIndex >= sizeof(header)) {
                uint8_t version = header[0];
                flags = header[1];
                sequence = static_cast<uint16_t>(header[2]) | (static_cast<uint16_t>(header[3]) << 8);
                uint16_t payloadLength = static_cast<uint16_t>(header[4]) | (static_cast<uint16_t>(header[5]) << 8);
                bool controlPacket = (flags & kFlagControlPacket) != 0;

                if (version != kProtocolVersion) {
                    ResetParser();
                } else if (controlPacket && payloadLength != kControlPayloadLength) {
                    ResetParser();
                } else if (!controlPacket && payloadLength != kFramePayloadLength) {
                    ResetParser();
                } else {
                    payloadIndex = 0;
                    expectedPayloadLength = payloadLength;
                    rxState = RxState::ReadPayload;
                }
            }
            break;

        case RxState::ReadPayload:
            payload[payloadIndex++] = value;
            if (payloadIndex >= expectedPayloadLength) {
                HandlePacketPayload();
            }
            break;
    }
}

void USBVideoStreamPlayer::HandlePacketPayload() {
    if ((flags & kFlagControlPacket) != 0) {
        HandleControlCommand(payload[0]);
        ResetParser();
        return;
    }

    if (!streamModeRequested) {
        streamModeRequested = true;
    }

    PresentFrame();
    streamLive = true;
    lastFrameMs = millis();
    lastSignalMs = lastFrameMs;
    ResetParser();
}

void USBVideoStreamPlayer::HandleControlCommand(uint8_t command) {
    switch (command) {
        case kCommandStartStreaming:
            streamModeRequested = true;
            streamLive = false;
            lastFrameMs = 0;
            lastSignalMs = millis();
            backgroundLayer.fillScreen({0, 0, 0});
            backgroundLayer.swapBuffers(false);
            break;

        case kCommandStopStreaming:
            DeactivateStreamingMode(false);
            break;

        default:
            break;
    }
}

void USBVideoStreamPlayer::DeactivateStreamingMode(bool clearFrame) {
    streamModeRequested = false;
    streamLive = false;
    lastFrameMs = 0;
    lastSignalMs = 0;
    ResetParser();

    if (clearFrame) {
        backgroundLayer.fillScreen({0, 0, 0});
        backgroundLayer.swapBuffers(false);
    }
}

void USBVideoStreamPlayer::PresentFrame() {
    (void)sequence;
    bool mirrorToLowerPanel = kMirrorToLowerPanel;
    if ((flags & kFlagSinglePanel) != 0) {
        mirrorToLowerPanel = false;
    }
    if ((flags & kFlagMirrorLowerPanel) != 0) {
        mirrorToLowerPanel = true;
    }

    uint16_t index = 0;
    for (uint16_t y = 0; y < kStreamHeight; ++y) {
        for (uint16_t x = 0; x < kStreamWidth; ++x) {
            uint16_t pixel565 = static_cast<uint16_t>(payload[index]) |
                                (static_cast<uint16_t>(payload[index + 1]) << 8);
            index += 2;

            rgb24 color = RGB565ToRgb24(pixel565);
            backgroundLayer.drawPixel(static_cast<int16_t>(x), static_cast<int16_t>(y), color);

            if (mirrorToLowerPanel) {
                backgroundLayer.drawPixel(static_cast<int16_t>((kStreamWidth - 1) - x),
                                          static_cast<int16_t>(y + kStreamHeight),
                                          color);
            }
        }
    }

    backgroundLayer.swapBuffers(false);
}

rgb24 USBVideoStreamPlayer::RGB565ToRgb24(uint16_t value) {
    uint8_t r5 = static_cast<uint8_t>((value >> 11) & 0x1F);
    uint8_t g6 = static_cast<uint8_t>((value >> 5) & 0x3F);
    uint8_t b5 = static_cast<uint8_t>(value & 0x1F);

    rgb24 color;
    color.red = static_cast<uint8_t>((r5 << 3) | (r5 >> 2));
    color.green = static_cast<uint8_t>((g6 << 2) | (g6 >> 4));
    color.blue = static_cast<uint8_t>((b5 << 3) | (b5 >> 2));
    return color;
}

#endif
