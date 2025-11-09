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
