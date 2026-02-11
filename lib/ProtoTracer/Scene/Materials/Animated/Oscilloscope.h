/**
 * @file Oscilloscope.h
 * @brief A dynamic oscilloscope material for visualizing audio signals.
 *
 * This file defines the Oscilloscope class, which visualizes audio signals in real-time
 * using a dynamic gradient and configurable physics-based bouncing behavior.
 *
 * @date 22/12/2024
 * @author Coela Can't
 */

#pragma once

#include "../Material.h" // Base class for material types.
#include "../Static/GradientMaterial.h" // Include for gradient materials.
#include "../../../Utils/Math/Mathematics.h" // Include for mathematical utilities.

/**
 * @class Oscilloscope
 * @brief A dynamic oscilloscope material for visualizing audio signals.
 *
 * The Oscilloscope class provides real-time visualization of audio signals,
 * featuring bouncing physics, gradient colors, and customizable attributes such as size,
 * position, and rotation.
 */
class Oscilloscope : public Material {
private:
    static constexpr uint16_t kInputSamples = 512; ///< Amount of microphone history used per frame.
    Vector2D size; ///< The size of the oscilloscope visualization.
    Vector2D offset; ///< The position offset of the visualization.
    float angle = 0.0f; ///< Rotation angle of the visualization.
    float hueAngle = 0.0f; ///< Hue angle for color adjustments.
    float* data = nullptr; ///< Pointer to the display-ready waveform bins.
    float scopedData[128] = {0.5f}; ///< Preprocessed and normalized oscilloscope bins (0..1).
    float filteredWave[128] = {0.0f}; ///< Internal signed smoothing buffer (-1..1).
    float displayAmplitude = 0.35f; ///< Smoothed auto-gain level for stable trace height.
    bool hasData = false; ///< Indicates whether scopedData has valid values.
    uint8_t bins = 128; ///< Number of bins for audio data processing.

    RGBColor rainbowSpectrum[6] = {RGBColor(255, 0, 0), RGBColor(255, 255, 0), RGBColor(0, 255, 0), RGBColor(0, 255, 255), RGBColor(0, 0, 255), RGBColor(255, 0, 255)}; ///< Predefined rainbow colors.
    GradientMaterial<6> gM = GradientMaterial<6>(rainbowSpectrum, 1.0f, false); ///< Gradient material for the rainbow spectrum.

    Material* material; ///< Pointer to an additional material for effects.

public:
    /**
     * @brief Constructs an Oscilloscope with the specified size and offset.
     *
     * @param size The size of the oscilloscope visualization.
     * @param offset The position offset of the visualization.
     */
    Oscilloscope(Vector2D size, Vector2D offset);

    /**
     * @brief Destroys the Oscilloscope instance.
     */
    ~Oscilloscope();

    /**
     * @brief Sets an additional material for visual effects.
     *
     * @param material Pointer to the Material instance.
     */
    void SetMaterial(Material* material);

    /**
     * @brief Retrieves the current sample data.
     *
     * @return Pointer to the audio sample data array.
     */
    float* GetSampleData();

    /**
     * @brief Sets the size of the oscilloscope visualization.
     *
     * @param size The new size as a Vector2D.
     */
    void SetSize(Vector2D size);

    /**
     * @brief Sets the position of the oscilloscope visualization.
     *
     * @param offset The new position offset as a Vector2D.
     */
    void SetPosition(Vector2D offset);

    /**
     * @brief Sets the rotation angle of the visualization.
     *
     * @param angle The new rotation angle in degrees.
     */
    void SetRotation(float angle);

    /**
     * @brief Sets the hue angle for color adjustments.
     *
     * @param hueAngle The new hue angle in degrees.
     */
    void SetHueAngle(float hueAngle);

    /**
     * @brief Updates the oscilloscope visualization based on new audio data.
     *
     * @param data Pointer to the audio sample data array.
     */
    void Update(float* data);

    /**
     * @brief Computes the color at a given position in the visualization.
     *
     * @param position The position in 3D space.
     * @param normal The surface normal vector.
     * @param uvw The texture coordinates.
     * @return The computed color as an RGBColor.
     */
    RGBColor GetRGB(const Vector3D& position, const Vector3D& normal, const Vector3D& uvw) override;
};
