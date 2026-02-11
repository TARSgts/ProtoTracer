#include "MergeSortVisualizer.h"

#include <Arduino.h>
#include <cmath>

namespace {
constexpr float kMinPadding = 0.0f;
constexpr float kMaxPadding = 0.9f;
}

MergeSortVisualizer::MergeSortVisualizer(Vector2D dimensions, Vector2D center)
    : size(dimensions.Divide(2.0f)),
      offset(center),
      displayResolution(dimensions),
      stepTimer(75.0f) {
    for (uint8_t i = 0; i < kMaxColumns; ++i) {
        values[i] = 0.0f;
        displayValues[i] = 0.0f;
        scratch[i] = 0.0f;
        valueKeys[i] = 0;
        scratchKeys[i] = 0;
    }

    UpdateResolutionLimit();
    Reset();
}

void MergeSortVisualizer::SetSize(Vector2D dimensions) {
    displayResolution = dimensions;
    size = dimensions.Divide(2.0f);
    UpdateResolutionLimit();
    columns = ClampColumnsToResolution(columns);
}

void MergeSortVisualizer::SetPosition(Vector2D center) {
    offset = center;
}

void MergeSortVisualizer::SetColumnCount(uint8_t columnCount) {
    columns = ClampColumnsToResolution(columnCount);
    randomizeColumns = false;
    shuffleAlgorithm = true;
    Reset();
}

void MergeSortVisualizer::EnableColumnRandomization(bool state) {
    randomizeColumns = state;
}

void MergeSortVisualizer::SetColumnRandomRange(uint8_t minColumns, uint8_t maxColumns) {
    uint8_t clampedMin = ClampColumnsToResolution(minColumns);
    uint8_t clampedMax = ClampColumnsToResolution(maxColumns);
    if (clampedMin > clampedMax) clampedMin = clampedMax;

    randomMinColumns = clampedMin;
    randomMaxColumns = clampedMax;
}

void MergeSortVisualizer::SetColumnPadding(float ratio) {
    if (ratio < kMinPadding) ratio = kMinPadding;
    if (ratio > kMaxPadding) ratio = kMaxPadding;
    columnGapRatio = ratio;
}

void MergeSortVisualizer::SetStepFrequency(float frequency) {
    if (frequency < 5.0f) frequency = 5.0f;
    stepTimer.SetFrequency(frequency);
}

void MergeSortVisualizer::Reset() {
    if (randomizeColumns) {
        columns = GenerateRandomColumnCount();
    } else {
        columns = ClampColumnsToResolution(columns);
    }

    ShuffleBars();
    sortingActive = true;
    holdFrames = 0;
    compareIndexA = compareIndexB = writeIndex = 255;

    if (shuffleAlgorithm) {
        SelectNextAlgorithm();
        shuffleAlgorithm = false;
    }

    radixPhase = RadixPhase::Counting;
    radixDigit = 0;
    radixDivisor = 1;
    radixProcessIndex = 0;
    for (uint8_t i = 0; i < radixBase; ++i) {
        radixCounts[i] = 0;
        radixOffsets[i] = 0;
    }

    PrepareCurrentAlgorithm();
}

void MergeSortVisualizer::ShuffleBars() {
    if (columns == 0) {
        return;
    }

    float denom = (columns > 1) ? float(columns - 1) : 1.0f;
    for (uint8_t i = 0; i < columns; ++i) {
        float value = float(i) / denom;
        uint16_t key = static_cast<uint16_t>(value * 1000.0f);
        values[i] = value;
        displayValues[i] = value;
        scratch[i] = value;
        valueKeys[i] = key;
        scratchKeys[i] = key;
    }

    if (columns < 2) return;

    for (int16_t i = int16_t(columns) - 1; i > 0; --i) {
        uint8_t j = static_cast<uint8_t>(random(0, i + 1));
        SwapColumns(static_cast<uint8_t>(i), j);
    }
}

void MergeSortVisualizer::BeginMergeRange() {
    if (currentWidth >= columns) {
        EnterHoldState();
        return;
    }

    if (leftStart >= columns) {
        leftStart = 0;
    }

    midPoint = leftStart + currentWidth;
    if (midPoint > columns) midPoint = columns;
    rightEnd = leftStart + currentWidth * 2;
    if (rightEnd > columns) rightEnd = columns;

    leftCursor = leftStart;
    rightCursor = midPoint;
    writeCursor = leftStart;
    copyCursor = leftStart;
    copyingBack = false;
}

void MergeSortVisualizer::PrepareCurrentAlgorithm() {
    switch (currentAlgorithm) {
        case SortAlgorithm::MergeSort:
            currentWidth = 1;
            leftStart = 0;
            copyingBack = false;
            BeginMergeRange();
            break;
        case SortAlgorithm::BubbleSort:
            bubbleOuter = 0;
            bubbleInner = 0;
            bubbleSwapped = false;
            break;
        case SortAlgorithm::InsertionSort:
            insertionIndex = 1;
            insertionScan = 0;
            insertionValue = 0.0f;
            insertionKey = 0;
            insertionPlacing = false;
            break;
        case SortAlgorithm::SelectionSort:
            selectionIndex = 0;
            selectionScan = 1;
            selectionMinIndex = 0;
            break;
        case SortAlgorithm::RadixSort:
            radixPhase = RadixPhase::Counting;
            radixDigit = 0;
            radixDivisor = 1;
            radixProcessIndex = 0;
            for (uint8_t i = 0; i < radixBase; ++i) {
                radixCounts[i] = 0;
                radixOffsets[i] = 0;
            }
            break;
        case SortAlgorithm::ShellSort:
            shellGap = columns / 2;
            shellOuter = shellGap;
            shellInner = shellOuter;
            break;
        case SortAlgorithm::HeapSort:
            heapSize = columns;
            heapBuilding = true;
            heapBuildIndex = heapSize > 0 ? int8_t(heapSize / 2) - 1 : -1;
            heapSiftIndex = -1;
            break;
        case SortAlgorithm::QuickSort:
            quickStackTop = -1;
            quickPartitioning = false;
            if (columns > 1) {
                PushQuickRange(0, columns - 1);
            }
            break;
    }

    compareIndexA = compareIndexB = writeIndex = 255;
}

void MergeSortVisualizer::AdvanceSortingStep() {
    if (!sortingActive) {
        if (holdFrames > 0) {
            --holdFrames;
            return;
        }

        shuffleAlgorithm = true;
        Reset();
        return;
    }

    switch (currentAlgorithm) {
        case SortAlgorithm::MergeSort:
            AdvanceMergeSortStep();
            break;
        case SortAlgorithm::BubbleSort:
            AdvanceBubbleSortStep();
            break;
        case SortAlgorithm::InsertionSort:
            AdvanceInsertionSortStep();
            break;
        case SortAlgorithm::SelectionSort:
            AdvanceSelectionSortStep();
            break;
        case SortAlgorithm::RadixSort:
            AdvanceRadixSortStep();
            break;
        case SortAlgorithm::ShellSort:
            AdvanceShellSortStep();
            break;
        case SortAlgorithm::HeapSort:
            AdvanceHeapSortStep();
            break;
        case SortAlgorithm::QuickSort:
            AdvanceQuickSortStep();
            break;
    }
}

void MergeSortVisualizer::AdvanceMergeSortStep() {
    if (!copyingBack) {
        if (leftCursor < midPoint && rightCursor < rightEnd) {
            compareIndexA = leftCursor;
            compareIndexB = rightCursor;
            if (valueKeys[leftCursor] <= valueKeys[rightCursor]) {
                uint8_t target = writeCursor++;
                uint8_t source = leftCursor++;
                scratch[target] = values[source];
                scratchKeys[target] = valueKeys[source];
            } else {
                uint8_t target = writeCursor++;
                uint8_t source = rightCursor++;
                scratch[target] = values[source];
                scratchKeys[target] = valueKeys[source];
            }
        } else if (leftCursor < midPoint) {
            compareIndexA = leftCursor;
            compareIndexB = 255;
            uint8_t target = writeCursor++;
            uint8_t source = leftCursor++;
            scratch[target] = values[source];
            scratchKeys[target] = valueKeys[source];
        } else if (rightCursor < rightEnd) {
            compareIndexA = 255;
            compareIndexB = rightCursor;
            uint8_t target = writeCursor++;
            uint8_t source = rightCursor++;
            scratch[target] = values[source];
            scratchKeys[target] = valueKeys[source];
        } else {
            copyingBack = true;
            copyCursor = leftStart;
            writeIndex = copyCursor;
            compareIndexA = compareIndexB = 255;
        }
    } else {
        if (copyCursor < rightEnd) {
            values[copyCursor] = scratch[copyCursor];
            valueKeys[copyCursor] = scratchKeys[copyCursor];
            writeIndex = copyCursor;
            ++copyCursor;
        } else {
            copyingBack = false;
            leftStart += currentWidth * 2;
            if (leftStart >= columns) {
                leftStart = 0;
                currentWidth *= 2;
            }

            BeginMergeRange();
        }
    }
}

void MergeSortVisualizer::AdvanceBubbleSortStep() {
    if (columns < 2) {
        EnterHoldState();
        return;
    }

    uint8_t upperBound = columns - bubbleOuter - 1;
    if (bubbleInner >= upperBound) {
        if (!bubbleSwapped) {
            EnterHoldState();
            return;
        }

        bubbleInner = 0;
        ++bubbleOuter;
        bubbleSwapped = false;

        if (bubbleOuter >= columns - 1) {
            EnterHoldState();
            return;
        }

        upperBound = columns - bubbleOuter - 1;
    }

    compareIndexA = bubbleInner;
    compareIndexB = bubbleInner + 1;
    writeIndex = 255;

    if (valueKeys[compareIndexA] > valueKeys[compareIndexB]) {
        SwapColumns(compareIndexA, compareIndexB);
        writeIndex = compareIndexB;
        bubbleSwapped = true;
    }

    ++bubbleInner;
}

void MergeSortVisualizer::AdvanceInsertionSortStep() {
    if (columns < 2) {
        EnterHoldState();
        return;
    }

    if (insertionIndex >= columns) {
        EnterHoldState();
        return;
    }

    if (!insertionPlacing) {
        insertionValue = values[insertionIndex];
        insertionKey = valueKeys[insertionIndex];
        insertionScan = static_cast<int8_t>(insertionIndex) - 1;
        insertionPlacing = true;
    }

    if (insertionScan >= 0 && valueKeys[insertionScan] > insertionKey) {
        values[insertionScan + 1] = values[insertionScan];
        valueKeys[insertionScan + 1] = valueKeys[insertionScan];
        writeIndex = insertionScan + 1;
        compareIndexA = insertionScan;
        compareIndexB = insertionScan + 1;
        --insertionScan;
    } else {
        values[insertionScan + 1] = insertionValue;
        valueKeys[insertionScan + 1] = insertionKey;
        writeIndex = insertionScan + 1;
        compareIndexA = insertionScan >= 0 ? insertionScan : 255;
        compareIndexB = insertionScan + 1;
        insertionPlacing = false;
        ++insertionIndex;
    }
}

void MergeSortVisualizer::AdvanceSelectionSortStep() {
    if (columns < 2) {
        EnterHoldState();
        return;
    }

    if (selectionIndex >= columns - 1) {
        EnterHoldState();
        return;
    }

    if (selectionScan < columns) {
        compareIndexA = selectionScan;
        compareIndexB = selectionMinIndex;
        if (valueKeys[selectionScan] < valueKeys[selectionMinIndex]) {
            selectionMinIndex = selectionScan;
        }
        ++selectionScan;
    } else {
        if (selectionMinIndex != selectionIndex) {
            SwapColumns(selectionIndex, selectionMinIndex);
            writeIndex = selectionIndex;
        } else {
            writeIndex = 255;
        }

        ++selectionIndex;
        if (selectionIndex >= columns - 1) {
            EnterHoldState();
            return;
        }

        selectionMinIndex = selectionIndex;
        selectionScan = selectionIndex + 1;
    }
}

void MergeSortVisualizer::AdvanceRadixSortStep() {
    if (columns < 2) {
        EnterHoldState();
        return;
    }

    switch (radixPhase) {
        case RadixPhase::Counting: {
            if (radixProcessIndex >= columns) {
                radixPhase = RadixPhase::Prefix;
                compareIndexA = compareIndexB = writeIndex = 255;
                return;
            }

            compareIndexA = radixProcessIndex;
            uint16_t key = valueKeys[radixProcessIndex];
            uint8_t digit = static_cast<uint8_t>((key / radixDivisor) % radixBase);
            ++radixCounts[digit];
            ++radixProcessIndex;
            break;
        }
        case RadixPhase::Prefix: {
            uint8_t running = 0;
            for (uint8_t i = 0; i < radixBase; ++i) {
                running += radixCounts[i];
                radixOffsets[i] = running;
            }
            radixProcessIndex = columns;
            radixPhase = RadixPhase::Distribute;
            compareIndexA = compareIndexB = writeIndex = 255;
            break;
        }
        case RadixPhase::Distribute: {
            if (radixProcessIndex == 0) {
                radixPhase = RadixPhase::CopyBack;
                radixProcessIndex = 0;
                compareIndexA = compareIndexB = writeIndex = 255;
                break;
            }

            --radixProcessIndex;
            uint16_t key = valueKeys[radixProcessIndex];
            uint8_t digit = static_cast<uint8_t>((key / radixDivisor) % radixBase);
            uint8_t dest = --radixOffsets[digit];
            scratch[dest] = values[radixProcessIndex];
            scratchKeys[dest] = key;
            compareIndexA = radixProcessIndex;
            compareIndexB = digit;
            writeIndex = dest;
            break;
        }
        case RadixPhase::CopyBack: {
            if (radixProcessIndex >= columns) {
                ++radixDigit;
                radixDivisor = static_cast<uint16_t>(radixDivisor * radixBase);
                for (uint8_t i = 0; i < radixBase; ++i) {
                    radixCounts[i] = 0;
                    radixOffsets[i] = 0;
                }

                if (radixDigit >= radixMaxDigits || radixDivisor > 10000) {
                    EnterHoldState();
                    return;
                }

                radixPhase = RadixPhase::Counting;
                radixProcessIndex = 0;
                compareIndexA = compareIndexB = writeIndex = 255;
                break;
            }

            values[radixProcessIndex] = scratch[radixProcessIndex];
            valueKeys[radixProcessIndex] = scratchKeys[radixProcessIndex];
            writeIndex = radixProcessIndex;
            compareIndexA = radixProcessIndex;
            ++radixProcessIndex;
            break;
        }
    }
}

void MergeSortVisualizer::AdvanceShellSortStep() {
    if (columns < 2) {
        EnterHoldState();
        return;
    }

    if (shellGap < 1) {
        EnterHoldState();
        return;
    }

    if (shellOuter >= columns) {
        shellGap /= 2;
        if (shellGap < 1) {
            EnterHoldState();
            return;
        }
        shellOuter = shellGap;
        shellInner = shellOuter;
    }

    uint8_t currentIndex = shellInner < 0 ? 0 : static_cast<uint8_t>(shellInner);
    uint8_t compareTarget = (shellInner >= shellGap) ? static_cast<uint8_t>(shellInner - shellGap) : 255;
    compareIndexA = currentIndex;
    compareIndexB = compareTarget;

    if (shellInner >= shellGap && valueKeys[currentIndex] < valueKeys[currentIndex - shellGap]) {
        SwapColumns(currentIndex, currentIndex - shellGap);
        writeIndex = currentIndex - shellGap;
        shellInner -= shellGap;
    } else {
        writeIndex = 255;
        ++shellOuter;
        shellInner = shellOuter;
    }
}

void MergeSortVisualizer::AdvanceHeapSortStep() {
    if (columns < 2) {
        EnterHoldState();
        return;
    }

    if (heapSize <= 1) {
        EnterHoldState();
        return;
    }

    if (heapBuilding) {
        if (heapBuildIndex < 0) {
            heapBuilding = false;
            heapSiftIndex = -1;
            return;
        }

        if (heapSiftIndex < 0) {
            heapSiftIndex = heapBuildIndex;
        }

        uint8_t current = static_cast<uint8_t>(heapSiftIndex);
        bool continueSift = SiftDown(heapSize, current);
        if (continueSift) {
            heapSiftIndex = current;
        } else {
            heapSiftIndex = -1;
            --heapBuildIndex;
            if (heapBuildIndex < 0) {
                heapBuilding = false;
            }
        }
        return;
    }

    if (heapSize <= 1) {
        EnterHoldState();
        return;
    }

    if (heapSiftIndex < 0) {
        SwapColumns(0, heapSize - 1);
        writeIndex = heapSize - 1;
        --heapSize;
        heapSiftIndex = 0;
        if (heapSize <= 1) {
            heapSiftIndex = -1;
            EnterHoldState();
            return;
        }
    }

    uint8_t current = static_cast<uint8_t>(heapSiftIndex);
    bool continueSift = SiftDown(heapSize, current);
    if (continueSift) {
        heapSiftIndex = current;
    } else {
        heapSiftIndex = -1;
    }
}

void MergeSortVisualizer::AdvanceQuickSortStep() {
    if (columns < 2) {
        EnterHoldState();
        return;
    }

    while (!quickPartitioning) {
        int16_t start = 0;
        int16_t end = 0;
        if (!PopQuickRange(start, end)) {
            EnterHoldState();
            return;
        }

        if (start >= end) {
            continue;
        }

        quickPartitionStart = start;
        quickPartitionEnd = end;
        quickPivotKey = valueKeys[end];
        quickStoreIndex = start;
        quickIterator = start;
        quickPartitioning = true;
    }

    if (quickIterator < quickPartitionEnd) {
        uint8_t current = static_cast<uint8_t>(quickIterator);
        uint8_t pivotIdx = static_cast<uint8_t>(quickPartitionEnd);
        compareIndexA = current;
        compareIndexB = pivotIdx;

        if (valueKeys[current] <= quickPivotKey) {
            if (quickIterator != quickStoreIndex) {
                SwapColumns(current, static_cast<uint8_t>(quickStoreIndex));
                writeIndex = static_cast<uint8_t>(quickStoreIndex);
            } else {
                writeIndex = 255;
            }
            ++quickStoreIndex;
        } else {
            writeIndex = 255;
        }

        ++quickIterator;
        return;
    }

    uint8_t storeIdx = static_cast<uint8_t>(quickStoreIndex);
    uint8_t pivotIdx = static_cast<uint8_t>(quickPartitionEnd);
    SwapColumns(storeIdx, pivotIdx);
    writeIndex = storeIdx;
    quickPartitioning = false;
    PushQuickRange(quickPartitionStart, quickStoreIndex - 1);
    PushQuickRange(quickStoreIndex + 1, quickPartitionEnd);
}

void MergeSortVisualizer::SmoothDisplayedValues() {
    for (uint8_t i = 0; i < columns; ++i) {
        float target = values[i];
        displayValues[i] += (target - displayValues[i]) * smoothingAmount;
    }
}

void MergeSortVisualizer::Update() {
    if (stepTimer.IsReady()) {
        AdvanceSortingStep();
    }

    SmoothDisplayedValues();
}

RGBColor MergeSortVisualizer::GetRGB(const Vector3D& position, const Vector3D& /*normal*/, const Vector3D& /*uvw*/) {
    static const RGBColor kBackgroundColor(3, 3, 6);

    Vector2D relative(position.X, position.Y);
    relative = relative - offset;

    if (relative.X < -size.X || relative.X > size.X) return RGBColor();
    if (relative.Y < -size.Y || relative.Y > size.Y) return RGBColor();

    float widthF = size.X * 2.0f;
    float heightF = size.Y * 2.0f;
    if (widthF <= 0.0f || heightF <= 0.0f || columns == 0) return RGBColor();

    int16_t widthPx = static_cast<int16_t>(floorf(widthF + 0.5f));
    int16_t heightPx = static_cast<int16_t>(floorf(heightF + 0.5f));
    if (widthPx <= 0 || heightPx <= 0) return RGBColor();

    int16_t pixelX = static_cast<int16_t>(floorf(relative.X + size.X));
    int16_t pixelY = static_cast<int16_t>(floorf(relative.Y + size.Y));
    if (pixelX < 0 || pixelX >= widthPx) return RGBColor();
    if (pixelY < 0 || pixelY >= heightPx) return RGBColor();

    uint8_t column = static_cast<uint8_t>((static_cast<uint32_t>(pixelX) * columns) / static_cast<uint32_t>(widthPx));
    if (column >= columns) column = columns - 1;
    float columnRatio = columns > 1 ? (float)column / float(columns - 1) : 0.0f;

    uint16_t columnStartPx = static_cast<uint16_t>((static_cast<uint32_t>(column) * widthPx) / columns);
    uint16_t columnEndPx = static_cast<uint16_t>((static_cast<uint32_t>(column + 1) * widthPx) / columns);
    if (columnEndPx <= columnStartPx) {
        columnEndPx = columnStartPx + 1;
    }
    uint16_t columnWidthPx = columnEndPx - columnStartPx;
    uint16_t columnLocalPx = static_cast<uint16_t>(pixelX) - columnStartPx;

    if (columnGapRatio > 0.0f) {
        uint16_t gapPixels = static_cast<uint16_t>(floorf(float(columnWidthPx) * columnGapRatio * 0.5f));
        if (gapPixels * 2 >= columnWidthPx) {
            gapPixels = (columnWidthPx > 1) ? (columnWidthPx - 1) / 2 : 0;
        }
        if (columnLocalPx < gapPixels || columnLocalPx >= (columnWidthPx - gapPixels)) {
            return kBackgroundColor;
        }
    }

    float barHeight = displayValues[column];
    if (barHeight < 0.02f) barHeight = 0.02f;
    if (barHeight > 1.0f) barHeight = 1.0f;

    float yRatio = (static_cast<float>(pixelY) + 0.5f) / static_cast<float>(heightPx);

    static const RGBColor coolColor(48, 120, 255);
    static const RGBColor warmColor(255, 80, 120);
    RGBColor color = RGBColor::InterpolateColors(coolColor, warmColor, columnRatio);

    if (column == compareIndexA || column == compareIndexB) {
        color = RGBColor(255, 204, 0);
    } else if (column == writeIndex) {
        color = RGBColor(0, 255, 180);
    }

    if (yRatio > barHeight) {
        return kBackgroundColor;
    }

    float depth = barHeight - yRatio;
    float brightness = 0.4f + depth * 0.6f;
    uint8_t r = static_cast<uint8_t>(float(color.R) * brightness);
    uint8_t g = static_cast<uint8_t>(float(color.G) * brightness);
    uint8_t b = static_cast<uint8_t>(float(color.B) * brightness);
    return RGBColor(r, g, b);
}

void MergeSortVisualizer::SelectNextAlgorithm() {
    uint8_t last = static_cast<uint8_t>(currentAlgorithm);
    uint8_t next = last;

    for (uint8_t attempt = 0; attempt < kAlgorithmCount; ++attempt) {
        next = static_cast<uint8_t>(random(0, kAlgorithmCount));
        if (kAlgorithmCount == 1 || next != last) {
            break;
        }
    }

    currentAlgorithm = static_cast<SortAlgorithm>(next);
}

uint8_t MergeSortVisualizer::ClampColumnsToResolution(uint8_t desired) const {
    if (desired < kMinColumns) desired = kMinColumns;
    if (desired > resolutionColumnLimit) desired = resolutionColumnLimit;
    if (desired > kMaxColumns) desired = kMaxColumns;
    return desired;
}

uint8_t MergeSortVisualizer::GenerateRandomColumnCount() const {
    uint8_t minRange = ClampColumnsToResolution(randomMinColumns);
    uint8_t maxRange = ClampColumnsToResolution(randomMaxColumns);
    if (minRange > maxRange) minRange = maxRange;
    return static_cast<uint8_t>(random(minRange, maxRange + 1));
}

void MergeSortVisualizer::UpdateResolutionLimit() {
    float width = displayResolution.X;
    float estimatedColumns = width / 6.0f;
    if (estimatedColumns < static_cast<float>(kMinColumns)) {
        estimatedColumns = static_cast<float>(kMinColumns);
    }
    if (estimatedColumns > static_cast<float>(kMaxColumns)) {
        estimatedColumns = static_cast<float>(kMaxColumns);
    }

    resolutionColumnLimit = static_cast<uint8_t>(estimatedColumns);
    if (resolutionColumnLimit < kMinColumns) {
        resolutionColumnLimit = kMinColumns;
    }
}

void MergeSortVisualizer::EnterHoldState() {
    sortingActive = false;
    holdFrames = kHoldDuration;
    compareIndexA = compareIndexB = writeIndex = 255;
}

void MergeSortVisualizer::SwapColumns(uint8_t a, uint8_t b) {
    if (a == b || a >= columns || b >= columns) return;

    float tempValue = values[a];
    values[a] = values[b];
    values[b] = tempValue;

    float tempDisplay = displayValues[a];
    displayValues[a] = displayValues[b];
    displayValues[b] = tempDisplay;

    float tempScratch = scratch[a];
    scratch[a] = scratch[b];
    scratch[b] = tempScratch;

    uint16_t tempKey = valueKeys[a];
    valueKeys[a] = valueKeys[b];
    valueKeys[b] = tempKey;

    uint16_t tempScratchKey = scratchKeys[a];
    scratchKeys[a] = scratchKeys[b];
    scratchKeys[b] = tempScratchKey;
}

void MergeSortVisualizer::PushQuickRange(int16_t start, int16_t end) {
    if (start >= end) return;
    if (quickStackTop >= static_cast<int8_t>(kQuickStackSize) - 1) return;
    ++quickStackTop;
    quickStackStart[quickStackTop] = start;
    quickStackEnd[quickStackTop] = end;
}

bool MergeSortVisualizer::PopQuickRange(int16_t& start, int16_t& end) {
    if (quickStackTop < 0) return false;
    start = quickStackStart[quickStackTop];
    end = quickStackEnd[quickStackTop];
    --quickStackTop;
    return true;
}

bool MergeSortVisualizer::SiftDown(uint8_t limit, uint8_t& current) {
    uint8_t left = current * 2 + 1;
    if (left >= limit) {
        compareIndexA = current;
        compareIndexB = 255;
        writeIndex = 255;
        return false;
    }

    uint8_t right = left + 1;
    uint8_t largest = left;
    if (right < limit && valueKeys[right] > valueKeys[largest]) largest = right;

    compareIndexA = current;
    compareIndexB = largest;

    if (valueKeys[current] < valueKeys[largest]) {
        SwapColumns(current, largest);
        writeIndex = largest;
        current = largest;
        return true;
    } else {
        writeIndex = 255;
        return false;
    }
}
