/**
 * @file HUB75Controller.h
 * @brief Declares the HUB75Controller class for managing HUB75 LED matrices.
 *
 * This file defines the HUB75Controller class, which extends the Controller base class.
 * It provides functionality for controlling and displaying content on HUB75 LED matrices.
 *
 * @date 22/12/2024
 * @author Coela Can't
 */

#pragma once

#include <stdint.h> // Include for fixed-width integer types.
#include "Controller.h" // Include for base controller functionality.
#include "SmartMatrixHUB75.h" // Include for HUB75-specific matrix utilities.
#include "../Camera/CameraManager/CameraManager.h" // Include for camera management.
#include "../Camera/Pixels/PixelGroup.h" // Include for pixel group management.

/**
 * @class HUB75Controller
 * @brief Manages HUB75 LED matrices with camera integration.
 *
 * The HUB75Controller class extends the Controller base class to provide specific
 * functionality for initializing, controlling, and displaying content on HUB75 LED matrices.
 */
class HUB75Controller : public Controller {
public:
    /**
     * @brief Constructs a HUB75Controller with specified parameters.
     *
     * @param cameras Pointer to the CameraManager for managing camera data.
     * @param maxBrightness Maximum brightness for the LED matrix.
     * @param maxAccentBrightness Maximum brightness for accent lighting.
     */
    HUB75Controller(CameraManager* cameras, uint8_t maxBrightness, uint8_t maxAccentBrightness);

    /**
     * @brief Initializes the HUB75Controller and sets up the LED matrix.
     */
    void Initialize() override;

    /**
     * @brief Updates and displays the content on the LED matrix.
     */
    void Display() override;

    /**
     * @brief Sets the maximum brightness of the LED matrix.
     *
     * @param maxBrightness The maximum brightness value (0-255).
     */
    void SetBrightness(uint8_t maxBrightness) override;

    /**
     * @brief Sets the maximum accent brightness of the secondary display.
     *
     * @param maxAccentBrightness The maximum accent brightness value (0-255).
     */
    void SetAccentBrightness(uint8_t maxAccentBrightness) override;

    /**
     * @brief Enables/disables an external frame provider (e.g., SmartMatrix GIF face).
     *
     * When enabled, the controller skips copying ProtoTracer pixels into the SmartMatrix
     * background layer, allowing external code to render directly.
     */
    void SetExternalFrameProvider(bool enabled);

    /**
     * @brief Checks if an external frame provider is active.
     */
    bool IsExternalFrameProvider() const;

    // Optional top-down 64x32 indexed startup frame. Null restores camera output.
    void SetStartupFrame(const uint8_t* indices, const uint8_t* rgbPalette);

    // USB diagnostics; flag counts represent reporting windows, not individual events.
    static void PrintDisplayStats();

private:
    bool externalFrameProvider = false;
    const uint8_t* startupIndices = nullptr;
    const uint8_t* startupPalette = nullptr;
};
