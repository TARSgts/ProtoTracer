/**
 * @file MergeSortVisualizer.h
 * @brief Material that visualizes a merge sort pass as colorful bars.
 *
 * The MergeSortVisualizer renders an array of bars that are progressively sorted
 * using an in-place, iterative merge sort. Bars animate smoothly between states
 * and the current compare/write indices are highlighted so the algorithm's
 * progress is easy to follow on the face.
 */

#pragma once

#include "../Material.h"
#include "../../../Utils/Math/Vector2D.h"
#include "../../../Utils/Math/Mathematics.h"
#include "../../../Utils/Time/TimeStep.h"

/**
 * @class MergeSortVisualizer
 * @brief Animated material that renders a merge sort visualizer.
 */
class MergeSortVisualizer : public Material {
public:
    static constexpr uint8_t kMaxColumns = 32; ///< Maximum number of bars rendered.
    static constexpr uint8_t kMinColumns = 4;  ///< Minimum number of bars that still read clearly.

    /**
     * @brief Construct the visualizer with a size and offset.
     * @param size Bounding size of the visualization in scene units.
     * @param offset Position of the visualization center.
     */
    MergeSortVisualizer(Vector2D dimensions, Vector2D center);

    /**
     * @brief Update the internal merge sort state.
     *
     * Should be called once per frame.
     */
    void Update();

    /**
     * @brief Change the visualizer size.
     * @param size New bounding size.
     */
    void SetSize(Vector2D dimensions);

    /**
     * @brief Change the visualizer position.
     * @param offset New visualization center.
     */
    void SetPosition(Vector2D center);

    /**
     * @brief Adjust how many bars are rendered and disable randomization.
     * @param columnCount Desired bar count (clamped to valid range).
     */
    void SetColumnCount(uint8_t columnCount);

    /**
     * @brief Enable or disable automatic column randomization on reset.
     */
    void EnableColumnRandomization(bool state);

    /**
     * @brief Configure the range used when randomizing column counts.
     */
    void SetColumnRandomRange(uint8_t minColumns, uint8_t maxColumns);

    /**
     * @brief Sets how much of each column width becomes padding (0.0-0.9).
     */
    void SetColumnPadding(float ratio);

    /**
     * @brief Configure how quickly merge steps advance.
     * @param frequency Steps per second.
     */
    void SetStepFrequency(float frequency);

    /**
     * @brief Reset and randomize the merge sort data.
     */
    void Reset();

    /**
     * @brief Sample the material color at the requested position.
     */
    RGBColor GetRGB(const Vector3D& position, const Vector3D& normal, const Vector3D& uvw) override;

private:
    void ShuffleBars();
    void BeginMergeRange();
    void AdvanceSortingStep();
    void SmoothDisplayedValues();

    enum class SortAlgorithm : uint8_t {
        MergeSort = 0,
        BubbleSort,
        InsertionSort,
        SelectionSort,
        RadixSort
    };

    enum class RadixPhase : uint8_t {
        Counting,
        Prefix,
        Distribute,
        CopyBack
    };

    static constexpr uint8_t kAlgorithmCount = 5;

    Vector2D size;
    Vector2D offset;
    Vector2D displayResolution;

    uint8_t columns = 24;
    float values[kMaxColumns];
    float displayValues[kMaxColumns];
    float scratch[kMaxColumns];
    uint16_t valueKeys[kMaxColumns];
    uint16_t scratchKeys[kMaxColumns];

    // Merge sort state
    uint8_t currentWidth = 1;
    uint8_t leftStart = 0;
    uint8_t midPoint = 0;
    uint8_t rightEnd = 0;
    uint8_t leftCursor = 0;
    uint8_t rightCursor = 0;
    uint8_t writeCursor = 0;
    uint8_t copyCursor = 0;
    bool copyingBack = false;

    // Bubble sort state
    uint8_t bubbleOuter = 0;
    uint8_t bubbleInner = 0;
    bool bubbleSwapped = false;

    // Insertion sort state
    uint8_t insertionIndex = 1;
    int8_t insertionScan = 0;
    float insertionValue = 0.0f;
    uint16_t insertionKey = 0;
    bool insertionPlacing = false;

    // Selection sort state
    uint8_t selectionIndex = 0;
    uint8_t selectionScan = 1;
    uint8_t selectionMinIndex = 0;
    // Radix sort state
    RadixPhase radixPhase = RadixPhase::Counting;
    uint8_t radixDigit = 0;
    uint8_t radixMaxDigits = 4;
    uint16_t radixDivisor = 1;
    uint8_t radixBase = 10;
    int16_t radixProcessIndex = 0;
    uint8_t radixCounts[10] = {0};
    uint8_t radixOffsets[10] = {0};

    bool sortingActive = false;
    uint16_t holdFrames = 0;
    static constexpr uint16_t kHoldDuration = 120;

    uint8_t compareIndexA = 255;
    uint8_t compareIndexB = 255;
    uint8_t writeIndex = 255;

    float smoothingAmount = 0.18f;
    float columnGapRatio = 0.0f;
    TimeStep stepTimer;

    bool randomizeColumns = true;
    uint8_t randomMinColumns = 10;
    uint8_t randomMaxColumns = kMaxColumns;
    uint8_t resolutionColumnLimit = kMaxColumns;

    SortAlgorithm currentAlgorithm = SortAlgorithm::MergeSort;
    bool shuffleAlgorithm = true;

    void PrepareCurrentAlgorithm();
    void AdvanceMergeSortStep();
    void AdvanceBubbleSortStep();
    void AdvanceInsertionSortStep();
    void AdvanceSelectionSortStep();
    void AdvanceRadixSortStep();
    void SelectNextAlgorithm();
    uint8_t ClampColumnsToResolution(uint8_t desired) const;
    uint8_t GenerateRandomColumnCount() const;
    void UpdateResolutionLimit();
    void EnterHoldState();
};
