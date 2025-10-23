/**
 * @file SingleButtonMorseHandler.h
 * @brief Dual-button handler used when MORSEBUTTON is defined.
 *
 * The original Morse-input implementation has been replaced with a simpler two-button schema:
 * one button advances menu items (short press) while the other advances the current menu (long press substitute).
 */

#pragma once

#include <Arduino.h>
#include <EEPROM.h>

template <uint8_t menuCount>
class MenuHandler {
private:
    static bool previousShortState;
    static bool previousLongState;
    static uint8_t currentMenu;
    static uint8_t currentValue[menuCount];
    static uint8_t maxValue[menuCount];
    static uint8_t shortPressPin;
    static uint8_t longPressPin;
    static unsigned long lastInteraction;

    static uint8_t ReadEEPROM(uint16_t index);
    static void WriteEEPROM(uint16_t index, uint8_t value);

public:
    static void Begin();
    static bool Initialize(uint8_t shortPressPin, uint8_t longPressPin, uint16_t holdingTime);
    static void SetDefaultValue(uint16_t menu, uint8_t value);
    static void SetInitialized();
    static void SetMenuMax(uint8_t menu, uint8_t maxValue);
    static void Update();
    static uint8_t GetMenuValue(uint8_t menu);
    static uint8_t GetCurrentMenu();
};

#include "SingleButtonMorseHandler.tpp"
