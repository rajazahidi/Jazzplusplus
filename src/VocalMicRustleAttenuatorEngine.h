//=============================================================================
// VocalMicRustleAttenuatorEngine.h
//
// Master Studio Pro Suite v32.0 - Apex Transcendence Sovereign Millennium Release
// Real-Time Vocal Microphone Rustle, Clothing Friction & Mechanical Thud Attenuator.
//
// Features:
// - Non-stationary low-mid clothing friction detection (80 Hz to 1.2 kHz)
// - Sub-band variance tracking separating vocal phonemes from abrasive rustle
// - Fast dynamic suppression (-3 dB to -24 dB) with sub-5ms recovery
// - Consonant and fundamental pitch preservation preventing vocal muffling
// - Rustle Delta Solo audition mode
//=============================================================================

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

enum class RustleAttenuatorMode {
    CLEAN_AUDIO = 0,
    DELTA_RUSTLE_SOLO
};

class JZVocalMicRustleAttenuatorEngine {
public:
    explicit JZVocalMicRustleAttenuatorEngine(float sampleRate = 48000.0f);
    ~JZVocalMicRustleAttenuatorEngine() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return mSampleRate; }

    // Configuration
    void setReductionAmount(float percent);        // 0.0% to 100.0% (nominally 70%)
    float getReductionAmount() const { return mReductionAmount; }

    void setSensitivity(float sensitivity);        // 0.1 to 1.0 (nominally 0.75)
    float getSensitivity() const { return mSensitivity; }

    void setFrequencyBand(float minHz, float maxHz); // 60 Hz to 2000 Hz
    float getMinFreqHz() const { return mMinFreqHz; }
    float getMaxFreqHz() const { return mMaxFreqHz; }

    void setMode(RustleAttenuatorMode mode);
    RustleAttenuatorMode getMode() const { return mMode; }

    // Telemetry
    float getCurrentAttenuationDb() const { return mCurrentAttenuationDb; }
    size_t getRustleEventsDetected() const { return mRustleEventsCount; }
    bool isRustleDetected() const { return mIsRustleActive; }

    // Audio Processing
    void process(const float* inL, const float* inR,
                 float* outL, float* outR, size_t numSamples);

    void reset();

private:
    float mSampleRate;
    float mReductionAmount;
    float mSensitivity;
    float mMinFreqHz;
    float mMaxFreqHz;
    RustleAttenuatorMode mMode;

    // Telemetry
    float mCurrentAttenuationDb;
    size_t mRustleEventsCount;
    bool  mIsRustleActive;

    // 8-band friction analysis filter bank covering 80 Hz to 1400 Hz
    static constexpr size_t NUM_BANDS = 8;
    float mBandFreqs[NUM_BANDS];

    struct BiquadBand {
        float b0, b1, b2, a1, a2;
        float x1, x2, y1, y2;
        BiquadBand() : b0(0), b1(0), b2(0), a1(0), a2(0), x1(0), x2(0), y1(0), y2(0) {}
        void reset() { x1 = x2 = y1 = y2 = 0.0f; }
        float process(float in) {
            float out = b0 * in + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = in;
            y2 = y1; y1 = out;
            return out;
        }
    };

    BiquadBand mBandsL[NUM_BANDS];
    BiquadBand mBandsR[NUM_BANDS];

    float mFastEnergy[NUM_BANDS];
    float mVarianceTracker[NUM_BANDS];
    float mBandGains[NUM_BANDS];

    void initFilterBank();
};

} // namespace Jazz
