//=============================================================================
// DigitalTruePeakDeClipperEngine.h
//
// Master Studio Pro Suite v31.0 - Apex Transcendence Sovereign Pinnacle Release
// Digital True-Peak De-Clipper & Cubic Hermite Spline Waveform Inpainter.
//
// Features:
// - Real-time flat-top digital saturation detection (consecutive clipped samples)
// - Cubic Hermite Spline inpainting reconstructing lost peak trajectories
// - Odd-harmonic de-harshing & peak profile organic synthesis
// - Automatic post-repair headroom dynamic protection preventing D/A converter overs
// - Real-time clipped events counter and true-peak crest restoration telemetry
//=============================================================================

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

class JZDigitalTruePeakDeClipperEngine {
public:
    explicit JZDigitalTruePeakDeClipperEngine(float sampleRate = 48000.0f);
    ~JZDigitalTruePeakDeClipperEngine() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return mSampleRate; }

    // Configuration
    void setClippingThresholdDbFS(float dbFS); // -2.0 dBFS to 0.0 dBFS (nominally -0.1 dBFS)
    float getClippingThresholdDbFS() const { return mThresholdDbFS; }

    void setAutoHeadroomEnabled(bool enabled) { mAutoHeadroom = enabled; }
    bool isAutoHeadroomEnabled() const { return mAutoHeadroom; }

    void setReconstructionPurity(float purity); // 0.0 to 1.0 (default 0.85)
    float getReconstructionPurity() const { return mReconstructionPurity; }

    // Telemetry
    size_t getDetectedClippedEvents() const { return mClippedEventsCount; }
    size_t getRepairedPeaksCount() const { return mRepairedPeaksCount; }
    float getMaxReconstructedPeakDbFS() const { return mMaxReconstructedPeakDbFS; }
    float getHeadroomCompensationDb() const { return mHeadroomGainDb; }

    // Processing
    void process(const float* inL, const float* inR,
                 float* outL, float* outR, size_t numSamples);

    void reset();

private:
    float mSampleRate;
    float mThresholdDbFS;
    float mLinearThreshold;
    bool  mAutoHeadroom;
    float mReconstructionPurity;

    // Telemetry
    size_t mClippedEventsCount;
    size_t mRepairedPeaksCount;
    float  mMaxReconstructedPeakDbFS;
    float  mHeadroomGainDb;
    float  mLinearHeadroomGain;

    // Lookahead ring buffer for cubic Hermite boundary analysis (32 samples)
    static constexpr size_t LOOKAHEAD_SIZE = 32;
    std::vector<float> mBufL;
    std::vector<float> mBufR;
    size_t mWriteIdx;

    void processChannel(const std::vector<float>& inBuf, std::vector<float>& outBuf,
                        size_t channelIdx, size_t numSamples);
    float inpaintCubicHermite(float p0, float p1, float m0, float m1, float t);
};

} // namespace Jazz
