/**
 * @file SpectrumAnalyzer.h
 * @brief A material for visualizing audio data as a colorful spectrum analyzer.
 *
 * This file defines the SpectrumAnalyzer class, which uses audio data to generate
 * a visually dynamic spectrum effect, with options for mirroring, flipping, and bouncing.
 *
 * @date 22/12/2024
 * @author Coela Can't
 */

#pragma once

#include "../Material.h" // Base material class.
#include "../Static/GradientMaterial.h" // For managing gradient colors.
#include "../../../Physics/Utils/BouncePhysics.h" // For adding bounce physics to spectrum data.

/**
 * @class SpectrumAnalyzer
 * @brief A material that visualizes audio data as a spectrum.
 *
 * The SpectrumAnalyzer class processes audio data to create a colorful and dynamic
 * spectrum visualization with options for customization, including mirroring and flipping.
 */
class SpectrumAnalyzer : public Material {
private:
    BouncePhysics* bPhy[128] = {}; ///< Array of bounce physics objects for each frequency bin.
    Vector2D size; ///< Size of the visualization area.
    Vector2D offset; ///< Offset position of the visualization area.
    float angle = 0.0f; ///< Rotation angle of the visualization.
    float hueAngle = 0.0f; ///< Hue adjustment angle for the spectrum colors.
    float* data = nullptr; ///< Pointer to the input audio data.
    float bounceData[128] = {}; ///< Processed bounce data for visualization.
    float processedData[128] = {0.0f}; ///< Locally processed spectrum values.
    float smoothedData[128] = {0.0f}; ///< Smoothed magnitudes for stability.
    float peakHoldData[128] = {0.0f}; ///< Peak hold values to keep spikes visible.
    uint8_t peakHoldTimer[128] = {0}; ///< Number of frames to keep each peak before decay.
    float visualData[128] = {0.0f}; ///< Smoothed data prepared for rendering.
    bool visualReady = false; ///< Indicates whether visualData is up to date.
    float noiseFloor = 0.02f; ///< Estimated idle noise floor.
    uint8_t idleFrames = 0; ///< Count of consecutive quiet frames.
    float autoGain = 1.0f; ///< Adaptive gain factor.
    uint8_t bins = 128; ///< Number of frequency bins.
    bool mirrorY = false; ///< Whether to mirror the visualization along the Y-axis.
    bool flipY = false; ///< Whether to flip the visualization along the Y-axis.
    bool bounce = false; ///< Whether to apply bouncing animation to the spectrum.
    float bassEnvelope = 0.0f; ///< Slow envelope follower for bass energy.
    float beatPulse = 0.0f; ///< Beat pulse intensity for visual accents.
    float beatHueOffset = 0.0f; ///< Temporary hue offset applied on beats.

    RGBColor rainbowSpectrum[6] = {
        RGBColor(255, 0, 0), 
        RGBColor(255, 255, 0), 
        RGBColor(0, 255, 0), 
        RGBColor(0, 255, 255), 
        RGBColor(0, 0, 255), 
        RGBColor(255, 0, 255)
    }; ///< Predefined rainbow color gradient.

    GradientMaterial<6> gM = GradientMaterial<6>(rainbowSpectrum, 1.0f, false); ///< Gradient material for coloring the spectrum.
    Material* material; ///< Optional sub-material for additional effects.
    bool usePeakHoldBlend = false; ///< Blends peak-hold data into display bars when true.
    float peakHoldBlend = 0.25f; ///< Amount of peak-hold contribution (0-1).
    uint8_t smoothingRadius = 0; ///< Kernel radius for visual smoothing (0-2). Default keeps single-bin peaks crisp.
    bool interpolateColumns = false; ///< When false, renders discrete bins without interpolation blur.
    bool frequencyRemapEnabled = false; ///< Enables non-linear remapping of frequency bins.
    float frequencyRemapExponent = 1.35f; ///< Exponent used when remapping bins (values > 1 emphasize bass).

public:
    /**
     * @brief Constructor for SpectrumAnalyzer.
     *
     * @param size The size of the visualization area.
     * @param offset The offset position of the visualization.
     * @param bounce Enables bouncing animation for the spectrum.
     * @param flipY Enables flipping the visualization along the Y-axis.
     * @param mirrorY Enables mirroring the visualization along the Y-axis.
     */
    SpectrumAnalyzer(Vector2D size, Vector2D offset, bool bounce = false, bool flipY = false, bool mirrorY = false);

    /**
     * @brief Destructor for SpectrumAnalyzer.
     */
    ~SpectrumAnalyzer();

    /**
     * @brief Sets the mirroring state for the visualization.
     *
     * @param state True to enable mirroring along the Y-axis, false to disable.
     */
    void SetMirrorYState(bool state);

    /**
     * @brief Sets the flipping state for the visualization.
     *
     * @param state True to enable flipping along the Y-axis, false to disable.
     */
    void SetFlipYState(bool state);

    /**
     * @brief Sets a custom material for additional effects.
     *
     * @param material Pointer to the custom material.
     */
    void SetMaterial(Material* material);

    /**
     * @brief Retrieves the Fourier data used for visualization.
     *
     * @return Pointer to the array of Fourier data.
     */
    float* GetFourierData();

    /**
     * @brief Sets the size of the visualization area.
     *
     * @param size The new size as a Vector2D.
     */
    void SetSize(Vector2D size);

    /**
     * @brief Sets the position of the visualization area.
     *
     * @param offset The new position as a Vector2D.
     */
    void SetPosition(Vector2D offset);

    /**
     * @brief Sets the rotation angle of the visualization.
     *
     * @param angle The rotation angle in degrees.
     */
    void SetRotation(float angle);

    /**
     * @brief Sets the hue adjustment angle for the spectrum colors.
     *
     * @param hueAngle The hue adjustment angle in degrees.
     */
    void SetHueAngle(float hueAngle);

    /**
     * @brief Enables blending of peak-hold data into the visible bars.
     *
     * @param enable True to blend peak values, false to show live data only.
     * @param blendRatio Blend ratio (0.0 = live only, 1.0 = hold only).
     */
    void EnablePeakHoldBlend(bool enable, float blendRatio = 0.25f);

    /**
     * @brief Sets the smoothing radius applied when building visual data.
     *
     * @param radius Kernel radius (0 = none, 1 = subtle, 2 = legacy wide blur).
     */
    void SetSmoothingRadius(uint8_t radius);

    /**
     * @brief Enables non-linear remapping of bins along the horizontal axis.
     *
     * @param enable True to remap bins, false to use native FFT ordering.
     * @param exponent Curve exponent ( >1 boosts bass, <1 boosts treble ).
     */
    void EnableFrequencyRemap(bool enable, float exponent = 1.35f);

    /**
     * @brief Enables smooth interpolation between bins when sampling final columns.
     *
     * @param enable True to interpolate, false to sample discrete bins.
     */
    void EnableColumnInterpolation(bool enable);

    /**
     * @brief Updates the spectrum visualization with new audio data.
     *
     * @param readData Pointer to the new audio data.
     */
    void Update(float* readData);

    /**
     * @brief Computes the color at a given position in the material.
     *
     * @param position The position in 3D space.
     * @param normal The surface normal vector.
     * @param uvw The texture coordinates.
     * @return The computed color as an RGBColor.
     */
    RGBColor GetRGB(const Vector3D& position, const Vector3D& normal, const Vector3D& uvw) override;

private:
    void BuildVisualData(const float* source);
    float SampleFrequency(float index) const;
};
