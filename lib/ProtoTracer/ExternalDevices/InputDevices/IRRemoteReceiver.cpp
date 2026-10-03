#include "IRRemoteReceiver.h"

#if defined(ENABLE_IR_REMOTE) && !defined(NEOTRELLISMENU) && !defined(MORSEBUTTON)

// Even DISABLE_LED_FEEDBACK configures LED_BUILTIN in IRremote::begin().
// Teensy pin 13 is HUB75 lower-half blue; leave its display pin mux untouched.
#ifndef NO_LED_FEEDBACK_CODE
#define NO_LED_FEEDBACK_CODE
#endif
#include <IRremote.hpp>

IRRemoteReceiver::Mapping IRRemoteReceiver::mappings[IRRemoteReceiver::kMaxMappings];
uint8_t IRRemoteReceiver::mappingCount = 0;
bool IRRemoteReceiver::initialized = false;
uint32_t IRRemoteReceiver::lastCode = 0;
unsigned long IRRemoteReceiver::lastDispatchMillis = 0;

#if defined(ENABLE_IR_REMOTE_DEBUG)
namespace {
    void LogCode(uint32_t code) {
        if (code == 0) return;
        auto& stream = IR_REMOTE_DEBUG_STREAM;
        if (!stream) return;

        stream.print(F("[IR] 0x"));
        stream.println(code, HEX);
    }
}
#endif

void IRRemoteReceiver::Initialize(uint8_t receiverPin) {
    if (initialized) return;

    IrReceiver.begin(receiverPin, DISABLE_LED_FEEDBACK);
    initialized = true;
}

bool IRRemoteReceiver::RegisterCode(uint32_t code, Callback callback) {
    if (!initialized || callback == nullptr) return false;
    if (code == IR_REMOTE_UNUSED_CODE) return false;
    if (mappingCount >= kMaxMappings) return false;

    mappings[mappingCount++] = {code, callback};
    return true;
}

void IRRemoteReceiver::Update() {
    if (!initialized) return;
    if (!IrReceiver.decode()) return;

    const auto& data = IrReceiver.decodedIRData;
    uint32_t receivedCode = data.decodedRawData;
    const bool isRepeat = (data.flags & IRDATA_FLAGS_IS_REPEAT);

    if (isRepeat) {
        receivedCode = lastCode;
    } else if (receivedCode != 0) {
        lastCode = receivedCode;
    }

    if (receivedCode == 0) {
        IrReceiver.resume();
        return;
    }

    const unsigned long now = millis();
    if (isRepeat && receivedCode == lastCode &&
        (now - lastDispatchMillis) < IR_REMOTE_REPEAT_DEBOUNCE_MS) {
        IrReceiver.resume();
        return;
    }

#if defined(ENABLE_IR_REMOTE_DEBUG)
    LogCode(receivedCode);
#endif

    Dispatch(receivedCode);
    lastDispatchMillis = now;
    IrReceiver.resume();
}

void IRRemoteReceiver::Dispatch(uint32_t code) {
    if (code == 0) return;

    for (uint8_t i = 0; i < mappingCount; ++i) {
        if (mappings[i].code == code && mappings[i].callback != nullptr) {
            mappings[i].callback();
        }
    }
}

#endif
