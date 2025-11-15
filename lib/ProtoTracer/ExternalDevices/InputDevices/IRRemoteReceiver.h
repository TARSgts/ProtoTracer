#pragma once

#include <Arduino.h>

#include "../../Examples/UserConfiguration.h"

#if defined(ENABLE_IR_REMOTE) && !defined(NEOTRELLISMENU) && !defined(MORSEBUTTON)

/**
 * @brief Lightweight dispatcher that maps IR remote codes to callback functions.
 *
 * The Menu system registers callbacks that mimic the physical button actions. The helper keeps
 * a tiny static lookup table to avoid heap usage and is only compiled when IR remote support
 * is enabled alongside the single-button input mode.
 */
class IRRemoteReceiver {
public:
    using Callback = void (*)();

    /**
     * @brief Initializes the global IR receiver instance.
     *
     * @param receiverPin The GPIO pin connected to the IR receiver module.
     */
    static void Initialize(uint8_t receiverPin);

    /**
     * @brief Registers a callback to be invoked when the specified code is received.
     *
     * @param code The 32-bit raw code reported by IRremote (protocol agnostic).
     * @param callback The function to invoke when the code is seen.
     * @return True on success; false if the table is full or parameters are invalid.
     */
    static bool RegisterCode(uint32_t code, Callback callback);

    /**
     * @brief Polls for IR messages and dispatches callbacks when a mapping matches.
     */
    static void Update();

private:
    struct Mapping {
        uint32_t code;
        Callback callback;
    };

    static constexpr uint8_t kMaxMappings = 6;

    static Mapping mappings[kMaxMappings];
    static uint8_t mappingCount;
    static bool initialized;
    static uint32_t lastCode;
    static unsigned long lastDispatchMillis;

    static void Dispatch(uint32_t code);
};

#endif
