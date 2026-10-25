//=============================================================================
// SpectralFrequencySplitPannerEngine.h
//
// Master Studio Pro Suite v30.0 - Apex Colossus Sovereign Diamond Jubilee Release
// 3-Band Linear-Phase Frequency-Split Mid/Side Dynamic Spectral Panner.
//
// Features:
// - 3-Band 24 dB/octave Linkwitz-Riley Crossover (Low, Mid, High)
// - Sub-bass Low-End Mono Centering Lock (< 120 Hz)
// - Independent Pan, Gain, and Mid/Side Width per frequency band
// - Dynamic Spectral Auto-Pan LFO modulation on high/mid frequency transients
// - Broadcast Phase Correlation Guard with anti-phase side energy damping
//=============================================================================

#pragma once

#include <vector>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

enum class SpectralPannerTargetBand {
    HIGH_ONLY = 0,
    MID_ONLY,
    MID_AND_HIGH
};

class JZSpectralFrequencySplitPannerEngine {
public:
    explicit JZSpectralFrequencySplitPannerEngine(float sampleRate = 48000.0f);
    ~JZSpectralFrequencySplitPannerEngine() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return mSampleRate; }

    // Crossover Points
    void setCrossoverFrequencies(float lowMidHz, float midHighHz);
    float getLowMidCrossoverHz() const { return mLowMidHz; }
    float getMidHighCrossoverHz() const { return mMidHighHz; }

    // Band Controls
    void setLowBand(float pan, float width, float gainDb, bool monoLock);
    void setMidBand(float pan, float width, float gainDb);
    void setHighBand(float pan, float width, float gainDb);

    float getLowPan() const { return mLowPan; }
    float getLowWidth() const { return mLowWidth; }
    bool isLowMonoLockEnabled() const { return mLowMonoLock; }

    float getMidPan() const { return mMidPan; }
    float getMidWidth() const { return mMidWidth; }

    float getHighPan() const { return mHighPan; }
    float getHighWidth() const { return mHighWidth; }

    // Auto-Pan LFO Modulation
    void setAutoPanLfo(bool enabled, float rateHz, float depth, SpectralPannerTargetBand target);
    bool isAutoPanEnabled() const { return mLfoEnabled; }
    float getLfoRateHz() const { return mLfoRateHz; }
    float getLfoDepth() const { return mLfoDepth; }

    // Phase Correlation Guard
    void setPhaseGuardEnabled(bool enabled) { mPhaseGuardEnabled = enabled; }
    bool isPhaseGuardEnabled() const { return mPhaseGuardEnabled; }
    float getPhaseCorrelation() const { return mPhaseCorrelation; }

    // Processing
    void process(const float* inL, const float* inR,
                 float* outL, float* outR, size_t numSamples);

    void reset();

private:
    float mSampleRate;
    float mLowMidHz;
    float mMidHighHz;

    // Low Band
    float mLowPan;       // -1.0 to +1.0
    float mLowWidth;     // 0.0 to 1.0 (0.0 = pure mono)
    float mLowGainDb;
    float mLowLinearGain;
    bool  mLowMonoLock;

    // Mid Band
    float mMidPan;       // -1.0 to +1.0
    float mMidWidth;     // 0.0 to 2.0
    float mMidGainDb;
    float mMidLinearGain;

    // High Band
    float mHighPan;      // -1.0 to +1.0
    float mHighWidth;    // 0.0 to 2.5
    float mHighGainDb;
    float mHighLinearGain;

    // LFO State
    bool mLfoEnabled;
    float mLfoRateHz;
    float mLfoDepth;
    SpectralPannerTargetBand mLfoTarget;
    float mLfoPhase;

    // Phase Guard
    bool mPhaseGuardEnabled;
    float mPhaseCorrelation;
    float mCorrSumLR;
    float mCorrSumLL;
    float mCorrSumRR;

    // 4th-order Linkwitz-Riley Crossover filters (two cascaded 2nd-order Butterworth biquads)
    struct BiquadLR4 {
        float b0_1, b1_1, b2_1, a1_1, a2_1;
        float b0_2, b1_2, b2_2, a1_2, a2_2;
        float x1_1, x2_1, y1_1, y2_1;
        float x1_2, x2_2, y1_2, y2_2;

        BiquadLR4() { reset(); }
        void reset() {
            x1_1 = x2_1 = y1_1 = y2_1 = 0.0f;
            x1_2 = x2_2 = y1_2 = y2_2 = 0.0f;
        }

        float process(float in) {
            // Stage 1
            float out1 = b0_1 * in + b1_1 * x1_1 + b2_1 * x2_1 - a1_1 * y1_1 - a2_1 * y2_1;
            x2_1 = x1_1; x1_1 = in;
            y2_1 = y1_1; y1_1 = out1;

            // Stage 2
            float out2 = b0_2 * out1 + b1_2 * x1_2 + b2_2 * x2_2 - a1_2 * y1_2 - a2_2 * y2_2;
            x2_2 = x1_2; x1_2 = out1;
            y2_2 = y1_2; y2_2 = out2;

            return out2;
        }
    };

    BiquadLR4 mLowpassL, mLowpassR;
    BiquadLR4 mHighpassL, mHighpassR;
    BiquadLR4 mMidLowpassL, mMidLowpassR;
    BiquadLR4 mMidHighpassL, mMidHighpassR;

    void updateCrossoverCoefficients();
    void processBandStereo(float inL, float inR, float pan, float width, float gain,
                           float& outL, float& outR);
};

} // namespace Jazz
