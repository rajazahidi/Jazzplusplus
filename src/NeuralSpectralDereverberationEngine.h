//=============================================================================
// NeuralSpectralDereverberationEngine.h
//
// Master Studio Pro Suite v31.0 - Apex Transcendence Sovereign Pinnacle Release
// Real-Time Neural Spectral Dereverberation & Early Reflection Stripper.
//
// Features:
// - Spectral phase-coherence cross-correlation & early reflection cancellation (< 40ms)
// - 16-band sub-band diffuse reverberant tail subtraction
// - Unvoiced consonant transient protection preventing vocal muffling
// - Room acoustic mode de-boxing (120 Hz to 650 Hz boxiness damping)
// - Reverb Tail Delta Solo audition mode
//=============================================================================

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

enum class DereverbMode {
    DEREVERB_CLEAN = 0,
    DELTA_REVERB_SOLO
};

class JZNeuralSpectralDereverberationEngine {
public:
    explicit JZNeuralSpectralDereverberationEngine(float sampleRate = 48000.0f);
    ~JZNeuralSpectralDereverberationEngine() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return mSampleRate; }

    // Configuration
    void setDereverbAmount(float percent);                // 0.0% to 100.0% (nominally 60%)
    float getDereverbAmount() const { return mDereverbAmount; }

    void setEarlyReflectionSuppressionDb(float db);      // -24.0 dB to 0.0 dB
    float getEarlyReflectionSuppressionDb() const { return mEarlySuppressionDb; }

    void setRoomSizeEstimate(float meters);              // 2.0 to 25.0 meters
    float getRoomSizeEstimate() const { return mRoomSizeMeters; }

    void setTransientPreservation(float amount);          // 0.0 to 1.0 (default 0.85)
    float getTransientPreservation() const { return mTransientPreservation; }

    void setMode(DereverbMode mode);
    DereverbMode getMode() const { return mMode; }

    // Telemetry
    float getEstimatedRt60Seconds() const { return mEstimatedRt60Sec; }
    float getCurrentSuppressionDb() const { return mCurrentSuppressionDb; }
    bool isConsonantProtected() const { return mIsConsonantActive; }

    // Audio Processing
    void process(const float* inL, const float* inR,
                 float* outL, float* outR, size_t numSamples);

    void reset();

private:
    float mSampleRate;
    float mDereverbAmount;
    float mEarlySuppressionDb;
    float mRoomSizeMeters;
    float mTransientPreservation;
    DereverbMode mMode;

    // Telemetry
    float mEstimatedRt60Sec;
    float mCurrentSuppressionDb;
    bool  mIsConsonantActive;

    // 16-band filter bank for sub-band spectral decay tracking
    static constexpr size_t NUM_BANDS = 16;
    float mBandFrequencies[NUM_BANDS];

    struct BandFilter {
        float b0, b1, b2, a1, a2;
        float x1, x2, y1, y2;
        BandFilter() : b0(0), b1(0), b2(0), a1(0), a2(0), x1(0), x2(0), y1(0), y2(0) {}
        void reset() { x1 = x2 = y1 = y2 = 0.0f; }
        float process(float in) {
            float out = b0 * in + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = in;
            y2 = y1; y1 = out;
            return out;
        }
    };

    BandFilter mFiltersL[NUM_BANDS];
    BandFilter mFiltersR[NUM_BANDS];

    // Sub-band envelope and decay trackers
    float mBandFastEnvelopes[NUM_BANDS];
    float mBandSlowEnvelopes[NUM_BANDS];
    float mBandGains[NUM_BANDS];

    // Circular delay buffer for early reflection comb cancellation (up to 50ms)
    std::vector<float> mEarlyDelayBufL;
    std::vector<float> mEarlyDelayBufR;
    size_t mDelayWriteIdx;
    size_t mDelayLengthSamples;

    // Transient attack detector
    float mPrevSampleL;
    float mPrevSampleR;

    void initFilterBank();
    void updateDelayLength();
};

} // namespace Jazz
