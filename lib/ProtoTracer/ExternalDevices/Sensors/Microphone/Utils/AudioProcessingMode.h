#pragma once

// Select one audio path per rendered frame. Only a committed mode change
// restarts sampling; requesting the same mode repeatedly keeps capture intact.
class AudioProcessingMode {
    bool spectrum = false;
    bool requested = false;
public:
    void BeginFrame() { requested = false; }
    void RequestSpectrum() { requested = true; }
    bool HasChange() const { return spectrum != requested; }
    void Commit() { spectrum = requested; }
    bool IsSpectrum() const { return spectrum; }
    unsigned SampleCount(unsigned fftSize) const { return spectrum ? fftSize / 2 : fftSize; }
};
