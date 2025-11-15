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
#define IR_REMOTE_RECEIVER_PIN 0
#endif

#ifndef IR_REMOTE_UNUSED_CODE
#define IR_REMOTE_UNUSED_CODE 0xFFFFFFFFUL
#endif

#ifndef IR_REMOTE_CODE_INCREMENT
#define IR_REMOTE_CODE_INCREMENT 0xBA45FF00UL
#endif

#ifndef IR_REMOTE_CODE_NEXT_MENU
#define IR_REMOTE_CODE_NEXT_MENU 0xB946FF00UL
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
