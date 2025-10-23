#pragma once

template <uint8_t menuCount>
bool MenuHandler<menuCount>::previousShortState;

template <uint8_t menuCount>
bool MenuHandler<menuCount>::previousLongState;

template <uint8_t menuCount>
uint8_t MenuHandler<menuCount>::currentMenu;

template <uint8_t menuCount>
uint8_t MenuHandler<menuCount>::currentValue[menuCount];

template <uint8_t menuCount>
uint8_t MenuHandler<menuCount>::maxValue[menuCount];

template <uint8_t menuCount>
uint8_t MenuHandler<menuCount>::shortPressPin;

template <uint8_t menuCount>
uint8_t MenuHandler<menuCount>::longPressPin;

template <uint8_t menuCount>
unsigned long MenuHandler<menuCount>::lastInteraction;

template <uint8_t menuCount>
void MenuHandler<menuCount>::Begin() {
    previousShortState = false;
    previousLongState = false;
    lastInteraction = millis();
}

template <uint8_t menuCount>
void MenuHandler<menuCount>::Update() {
    const unsigned long now = millis();
    const bool dualButtonMode = shortPressPin != longPressPin;
    const bool shortPressed = !digitalRead(shortPressPin);
    const bool longPressed = dualButtonMode ? !digitalRead(longPressPin) : false;
    constexpr unsigned long inactivityTimeout = 30000UL;

    if (!shortPressed && (!dualButtonMode || !longPressed) && currentMenu != 0 &&
        (now - lastInteraction) >= inactivityTimeout) {
        WriteEEPROM(currentMenu, currentValue[currentMenu]);
        currentMenu = 0;
        lastInteraction = now;
    }

    if (shortPressed && !previousShortState) {
        lastInteraction = now;
    } else if (!shortPressed && previousShortState) {
        if (maxValue[currentMenu] > 0) {
            currentValue[currentMenu] = (currentValue[currentMenu] + 1) % maxValue[currentMenu];
            if (currentMenu != 0) {
                WriteEEPROM(currentMenu, currentValue[currentMenu]);
            }
        }
        lastInteraction = now;
    }

    if (dualButtonMode) {
        if (longPressed && !previousLongState) {
            lastInteraction = now;
        } else if (!longPressed && previousLongState) {
            WriteEEPROM(currentMenu, currentValue[currentMenu]);
            currentMenu = (currentMenu + 1) % menuCount;
            lastInteraction = now;
        }
    }

    previousShortState = shortPressed;
    previousLongState = dualButtonMode ? longPressed : false;
}

template <uint8_t menuCount>
uint8_t MenuHandler<menuCount>::ReadEEPROM(uint16_t index) {
    return EEPROM.read(index);
}

template <uint8_t menuCount>
void MenuHandler<menuCount>::WriteEEPROM(uint16_t index, uint8_t value) {
    EEPROM.write(index, value);
}

template <uint8_t menuCount>
bool MenuHandler<menuCount>::Initialize(uint8_t shortPressPin, uint8_t longPressPin, uint16_t /*holdingTime*/) {
    MenuHandler::shortPressPin = shortPressPin;
    MenuHandler::longPressPin = longPressPin;

    pinMode(shortPressPin, INPUT_PULLUP);
    if (longPressPin != shortPressPin) {
        pinMode(longPressPin, INPUT_PULLUP);
    }

    for (uint8_t i = 0; i < menuCount; i++) {
        currentValue[i] = ReadEEPROM(i);
    }

    currentMenu = 0;
    previousShortState = false;
    previousLongState = false;
    lastInteraction = millis();

    return ReadEEPROM(menuCount + 1) != 255;
}

template <uint8_t menuCount>
void MenuHandler<menuCount>::SetDefaultValue(uint16_t menu, uint8_t value) {
    if (menu >= menuCount) return;

    currentValue[menu] = value;

    WriteEEPROM(menu, value);
}

template <uint8_t menuCount>
void MenuHandler<menuCount>::SetInitialized() {
    WriteEEPROM(menuCount + 1, 0);
}

template <uint8_t menuCount>
void MenuHandler<menuCount>::SetMenuMax(uint8_t menu, uint8_t maxValue) {
    if (menu >= menuCount) return;

    MenuHandler::maxValue[menu] = maxValue;
}

template <uint8_t menuCount>
uint8_t MenuHandler<menuCount>::GetMenuValue(uint8_t menu) {
    return currentValue[menu];
}

template <uint8_t menuCount>
uint8_t MenuHandler<menuCount>::GetCurrentMenu() {
    return currentMenu;
}
