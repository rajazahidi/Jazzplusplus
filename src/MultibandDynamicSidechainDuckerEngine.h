//=============================================================================
// MultibandDynamicSidechainDuckerEngine.h
//
// Master Studio Pro Suite v30.0 - Apex Colossus Sovereign Diamond Jubilee Release
// 16-Band Sub-Band Dynamic Sidechain Carving Ducker & Frequency Isolation Engine.
//
// Features:
// - 16-Band logarithmic bandpass filter bank (40 Hz to 16 kHz)
// - Sub-band inverse spectral subtraction (Trackspacer / dynamic carving style)
// - Ducks ONLY the colliding frequency bands of the sidechain signal
// - Frequency focus range controls (minFreqHz to maxFreqHz)
// - Ultra-fast transient attack (< 2ms) and program-dependent release
// - Audition modes: Carve, Delta Solo, and Sidechain Listen
// - Real-time gain reduction telemetry per band
//=============================================================================

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

enum class SidechainDuckerMode {
    CARVE = 0,
    DELTA_SOLO,
    SIDECHAIN_LISTEN
};

class JZMultibandDynamicSidechainDuckerEngine {
public:
    explicit JZMultibandDynamicSidechainDuckerEngine(float sampleRate = 48000.0f);
    ~JZMultibandDynamicSidechainDuckerEngine() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return mSampleRate; }

    // Ducker Configuration
    void setCarveAmount(float percent);          // 0.0% to 100.0% (nominally 40%)
    float getCarveAmount() const { return mCarveAmount; }

    void setFrequencyFocus(float minHz, float maxHz);
    float getMinFreqHz() const { return mMinFreqHz; }
    float getMaxFreqHz() const { return mMaxFreqHz; }

    void setAttackTimeMs(float attackMs);        // 0.2 ms to 20.0 ms
    float getAttackTimeMs() const { return mAttackMs; }

    void setReleaseTimeMs(float releaseMs);      // 10.0 ms to 500.0 ms
    float getReleaseTimeMs() const { return mReleaseMs; }

    void setMode(SidechainDuckerMode mode);
    SidechainDuckerMode getMode() const { return mMode; }

    // Telemetry
    float getMaxGainReductionDb() const { return mMaxGainReductionDb; }
    size_t getActiveCarvedBandsCount() const { return mActiveCarvedBands; }
    const std::vector<float>& getBandGainReductionsDb() const { return mBandGainReductionsDb; }

    // Processing
    void process(const float* mainInL, const float* mainInR,
                 const float* sideInL, const float* sideInR,
                 float* outL, float* outR, size_t numSamples);

    void reset();

private:
    float mSampleRate;
    float mCarveAmount;
    float mMinFreqHz;
    float mMaxFreqHz;
    float mAttackMs;
    float mReleaseMs;
    SidechainDuckerMode mMode;

    // Telemetry
    float mMaxGainReductionDb;
    size_t mActiveCarvedBands;
    std::vector<float> mBandGainReductionsDb;

    static constexpr size_t NUM_BANDS = 16;
    float mBandFrequencies[NUM_BANDS];

    // Bandpass filter biquad
    struct BandpassBiquad {
        float b0, b1, b2, a1, a2;
        float x1, x2, y1, y2;
        BandpassBiquad() : b0(0), b1(0), b2(0), a1(0), a2(0), x1(0), x2(0), y1(0), y2(0) {}
        void reset() { x1 = x2 = y1 = y2 = 0.0f; }
        float process(float in) {
            float out = b0 * in + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = in;
            y2 = y1; y1 = out;
            return out;
        }
    };

    BandpassBiquad mMainFiltersL[NUM_BANDS];
    BandpassBiquad mMainFiltersR[NUM_BANDS];
    BandpassBiquad mSideFiltersL[NUM_BANDS];
    BandpassBiquad mSideFiltersR[NUM_BANDS];

    float mSideEnvelopes[NUM_BANDS];
    float mBandDynamicGains[NUM_BANDS];

    void initFilterBands();
    void updateAttackReleaseCoefficients(float& attackCoeff, float& releaseCoeff);
};

} // namespace Jazz
