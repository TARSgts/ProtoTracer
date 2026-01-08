/**
 * @file UserConfiguration.h
 * @brief User-configurable settings for the project.
 *
 * This file defines compile-time macros and settings that enable or disable
 * various features and behaviors of the project. Adjust the definitions below
 * to customize the functionality as needed.
 *
 * @date 22/12/2024
 * @author Coela Can't
 */

#pragma once

/**
 * @def HUB75_RBG
 * @brief Define this macro to use a modified Matrix Hardware file for panels with RBG color order.
 */
//#define HUB75_RBG
#if DOXYGEN
#define HUB75_RBG
#endif

/**
 * @def PRINTINFO
 * @brief Define this macro to enable printing live stats, such as FPS and other information.
 */
#define PRINTINFO
#if DOXYGEN
#define PRINTINFO
#endif

/**
 * @def DEBUG
 * @brief Define this macro to enable debug information output.
 */
//#define DEBUG
#if DOXYGEN
#define DEBUG
#endif

/**
 * @def NEOTRELLISMENU
 * @brief Define this macro to enable the NeoTrellis controller for menu navigation.
 *
 * If this macro is undefined, the button controller is used instead.
 */
//#define NEOTRELLISMENU
#if DOXYGEN
#define NEOTRELLISMENU
#endif

/**
 * @def MORSEBUTTON
 * @brief Define this macro to enable Morse code input using a button.
 *
 * Note: This feature cannot be used simultaneously with NEOTRELLISMENU.
 */
//#define MORSEBUTTON
#if DOXYGEN
#define MORSEBUTTON
#endif

/**
 * @def ENABLE_IR_REMOTE
 * @brief Define to allow an IR receiver + remote to mirror the single-button menu actions.
 *
 * The remote provides short-press (value increment) and long-press (menu advance) actions in parallel
 * with the physical buttons. This helper currently targets the standard single-button handler.
 */
#define ENABLE_IR_REMOTE
#if DOXYGEN
#define ENABLE_IR_REMOTE
#endif

#if defined(ENABLE_IR_REMOTE)
#ifndef IR_REMOTE_RECEIVER_PIN
#define IR_REMOTE_RECEIVER_PIN 17
#endif

#ifndef IR_REMOTE_UNUSED_CODE
#define IR_REMOTE_UNUSED_CODE 0xFFFFFFFFUL
#endif

#ifndef IR_REMOTE_CODE_INCREMENT
#define IR_REMOTE_CODE_INCREMENT 0xBC43FF00UL
#endif

#ifndef IR_REMOTE_CODE_NEXT_MENU
#define IR_REMOTE_CODE_NEXT_MENU 0xBA45FF00UL
#endif

#ifndef IR_REMOTE_CODE_DECREMENT
#define IR_REMOTE_CODE_DECREMENT 0xBB44FF00UL
#endif

#ifndef IR_REMOTE_CODE_FAN_INCREMENT
#define IR_REMOTE_CODE_FAN_INCREMENT 0xF609FF00UL
#endif

#ifndef IR_REMOTE_CODE_FAN_DECREMENT
#define IR_REMOTE_CODE_FAN_DECREMENT 0xF807FF00UL
#endif

#ifndef IR_REMOTE_COLOR_CODE_0
#define IR_REMOTE_COLOR_CODE_0 0xE916FF00UL
#endif
#ifndef IR_REMOTE_COLOR_CODE_1
#define IR_REMOTE_COLOR_CODE_1 0xF30CFF00UL
#endif
#ifndef IR_REMOTE_COLOR_CODE_2
#define IR_REMOTE_COLOR_CODE_2 0xE718FF00UL
#endif
#ifndef IR_REMOTE_COLOR_CODE_3
#define IR_REMOTE_COLOR_CODE_3 0xA15EFF00UL
#endif
#ifndef IR_REMOTE_COLOR_CODE_4
#define IR_REMOTE_COLOR_CODE_4 0xF708FF00UL
#endif
#ifndef IR_REMOTE_COLOR_CODE_5
#define IR_REMOTE_COLOR_CODE_5 0xE31CFF00UL
#endif
#ifndef IR_REMOTE_COLOR_CODE_6
#define IR_REMOTE_COLOR_CODE_6 0xA55AFF00UL
#endif
#ifndef IR_REMOTE_COLOR_CODE_7
#define IR_REMOTE_COLOR_CODE_7 0xBD42FF00UL
#endif
#ifndef IR_REMOTE_COLOR_CODE_8
#define IR_REMOTE_COLOR_CODE_8 0xAD52FF00UL
#endif
#ifndef IR_REMOTE_COLOR_CODE_9
#define IR_REMOTE_COLOR_CODE_9 0xB54AFF00UL
#endif

#ifndef IR_REMOTE_REPEAT_DEBOUNCE_MS
#define IR_REMOTE_REPEAT_DEBOUNCE_MS 250
#endif

/**
 * @def ENABLE_IR_REMOTE_DEBUG
 * @brief Define to stream every received IR code to the serial monitor.
 *
 * Useful while learning the button mappings on a new remote. Disable once configured to
 * keep the log clean.
 */
#define ENABLE_IR_REMOTE_DEBUG
#if DOXYGEN
#define ENABLE_IR_REMOTE_DEBUG
#endif

#ifndef IR_REMOTE_DEBUG_STREAM
#define IR_REMOTE_DEBUG_STREAM Serial
#endif
#endif

/**
 * @def ENABLE_FACE_COLOR_STRIP
 * @brief Mirrors the current face color onto an external WS2812 strip.
 */
#define ENABLE_FACE_COLOR_STRIP
#if DOXYGEN
#define ENABLE_FACE_COLOR_STRIP
#endif

/**
 * @def ENABLE_GIF_FACE
 * @brief Enables the SmartMatrix SD GIF face (uses GifDecoder + AnimatedGIF).
 */
//#define ENABLE_GIF_FACE
#if DOXYGEN
#define ENABLE_GIF_FACE
#endif

/**
 * @def ENABLE_SORTING_FACE
 * @brief Enables the sorting algorithm (merge sort) face.
 */
#define ENABLE_SORTING_FACE
#if DOXYGEN
#define ENABLE_SORTING_FACE
#endif

/**
 * @def ENABLE_PONG_FACE
 * @brief Enables the Pong face.
 */
#define ENABLE_PONG_FACE
#if DOXYGEN
#define ENABLE_PONG_FACE
#endif

/**
 * @def ENABLE_SPACE_INVADERS_FACE
 * @brief Enables the Space Invaders face.
 */
#define ENABLE_SPACE_INVADERS_FACE
#if DOXYGEN
#define ENABLE_SPACE_INVADERS_FACE
#endif

/**
 * @def ENABLE_FLAPPY_BIRD_FACE
 * @brief Enables the Flappy Bird face.
 */
#define ENABLE_FLAPPY_BIRD_FACE
#if DOXYGEN
#define ENABLE_FLAPPY_BIRD_FACE
#endif

/**
 * @def ENABLE_SNAKE_FACE
 * @brief Enables the Snake face.
 */
#define ENABLE_SNAKE_FACE
#if DOXYGEN
#define ENABLE_SNAKE_FACE
#endif

/**
 * @def ENABLE_GAME_OF_LIFE_FACE
 * @brief Enables the Conway's Game of Life face.
 */
#define ENABLE_GAME_OF_LIFE_FACE
#if DOXYGEN
#define ENABLE_GAME_OF_LIFE_FACE
#endif

/**
 * @def ENABLE_SLOT_MACHINE_FACE
 * @brief Enables the slot machine face.
 */
//#define ENABLE_SLOT_MACHINE_FACE
#if DOXYGEN
#define ENABLE_SLOT_MACHINE_FACE
#endif

#ifndef FACE_COLOR_STRIP_PIN
#define FACE_COLOR_STRIP_PIN 20
#endif

#ifndef FACE_COLOR_STRIP_LENGTH
#define FACE_COLOR_STRIP_LENGTH 30
#endif

#ifndef FACE_COLOR_STRIP_MAX_LENGTH
#define FACE_COLOR_STRIP_MAX_LENGTH FACE_COLOR_STRIP_LENGTH
#endif

#ifndef FACE_COLOR_STRIP_BRIGHTNESS
#define FACE_COLOR_STRIP_BRIGHTNESS 128
#endif

/**
 * @def FAN_PWM_PIN
 * @brief PWM output pin for the cooling fan.
 */
#ifndef FAN_PWM_PIN
#define FAN_PWM_PIN 15
#endif

/**
 * @def FAN_DEFAULT_MENU_VALUE
 * @brief Default fan speed menu value (0-10). 10 = 100%.
 */
#ifndef FAN_DEFAULT_MENU_VALUE
#define FAN_DEFAULT_MENU_VALUE 6
#endif


/**
 * @def TTP223_PIN
 * @brief Digital input pin for the TTP223 touch sensor.
 */
#ifndef TTP223_PIN
#define TTP223_PIN 16
#endif

/**
 * @def TTP223_PIN_MODE
 * @brief Pin mode for the TTP223 input (INPUT, INPUT_PULLUP, INPUT_PULLDOWN).
 */
#ifndef TTP223_PIN_MODE
#define TTP223_PIN_MODE INPUT
#endif

/**
 * @def TTP223_ACTIVE_HIGH
 * @brief Set to 1 if the TTP223 outputs HIGH when touched, 0 if LOW.
 */
#ifndef TTP223_ACTIVE_HIGH
#define TTP223_ACTIVE_HIGH 1
#endif
