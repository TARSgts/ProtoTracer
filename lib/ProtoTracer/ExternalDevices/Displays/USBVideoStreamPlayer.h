#pragma once

#include "Arduino.h"
#include "../../Examples/UserConfiguration.h"
#include "../../Controller/SmartMatrixHUB75.h"

#ifdef ENABLE_USB_VIDEO_FACE

class USBVideoStreamPlayer {
public:
    static constexpr uint16_t kStreamWidth = 64;
    static constexpr uint16_t kStreamHeight = 32;

    void Initialize();
    bool IsStreamingModeActive() const;
    bool IsRendering() const;
    void Update();

private:
    enum class RxState : uint8_t {
        WaitMagic0 = 0,
        WaitMagic1,
        ReadHeader,
        ReadPayload
    };

    static constexpr uint8_t kMagic0 = 'P';
    static constexpr uint8_t kMagic1 = 'T';
    static constexpr uint8_t kProtocolVersion = 1;
    static constexpr uint8_t kFlagMirrorLowerPanel = 0x01;
    static constexpr uint8_t kFlagSinglePanel = 0x02;
    static constexpr uint8_t kFlagControlPacket = 0x80;
    static constexpr uint8_t kCommandStartStreaming = 0x01;
    static constexpr uint8_t kCommandStopStreaming = 0x02;
    static constexpr uint32_t kSerialBaud = 2000000;
    static constexpr uint16_t kFramePayloadLength = kStreamWidth * kStreamHeight * 2;
    static constexpr uint16_t kControlPayloadLength = 1;
    static constexpr uint32_t kFrameTimeoutMs = 750;
    static constexpr uint32_t kModeTimeoutMs = 1500;
    static constexpr bool kMirrorToLowerPanel = true;

    bool serialReady = false;
    bool streamModeRequested = false;
    bool streamLive = false;
    uint32_t lastFrameMs = 0;
    uint32_t lastSignalMs = 0;

    RxState rxState = RxState::WaitMagic0;
    uint8_t header[6] = {0};
    uint8_t headerIndex = 0;
    uint16_t payloadIndex = 0;
    uint16_t expectedPayloadLength = 0;
    uint8_t payload[kFramePayloadLength] = {0};

    uint8_t flags = 0;
    uint16_t sequence = 0;

    void ResetParser();
    void ProcessIncomingByte(uint8_t value);
    void HandlePacketPayload();
    void HandleControlCommand(uint8_t command);
    void DeactivateStreamingMode(bool clearFrame);
    void PresentFrame();
    static rgb24 RGB565ToRgb24(uint16_t value);
};

#endif
