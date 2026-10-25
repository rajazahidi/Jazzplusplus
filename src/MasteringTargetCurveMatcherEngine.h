//=============================================================================
// MasteringTargetCurveMatcherEngine.h
//
// Master Studio Pro Suite v31.0 - Apex Transcendence Sovereign Pinnacle Release
// Mastering Target Curve Spectrum Matcher & 32-Band Morphing Equalizer.
//
// Features:
// - Long-Term Average Spectrum (LTAS) analysis across 32 ISO frequency bands
// - Standard industry target curves (Pink Noise -4.5dB/oct, Modern Pop, Warm Acoustic, Classical Hi-Fi, EDM Sub)
// - 32-band linear-phase morphing correction filter
// - Gaussian spectral curve smoothing preventing comb filter ringing
// - Continuous match depth scaling (0% to 200%) and max gain clamping (+/- 12 dB)
//=============================================================================

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

enum class MasteringTargetCurve {
    PINK_NOISE_REFERENCE = 0,
    MODERN_POP_URBAN,
    WARM_ACOUSTIC,
    CLASSICAL_HI_FI,
    EDM_CLUB_SUB
};

class JZMasteringTargetCurveMatcherEngine {
public:
    explicit JZMasteringTargetCurveMatcherEngine(float sampleRate = 48000.0f);
    ~JZMasteringTargetCurveMatcherEngine() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return mSampleRate; }

    // Target Curve Selection
    void setTargetCurve(MasteringTargetCurve curve);
    MasteringTargetCurve getTargetCurve() const { return mTargetCurve; }
    std::string getTargetCurveName() const;

    // Controls
    void setMatchDepthPercent(float percent);     // 0.0% to 200.0% (nominally 100%)
    float getMatchDepthPercent() const { return mMatchDepth; }

    void setSmoothingOctaves(float octaves);      // 0.1 to 1.0 octaves (nominally 0.33)
    float getSmoothingOctaves() const { return mSmoothingOctaves; }

    void setMaxCorrectionDb(float maxDb);         // 3.0 dB to 18.0 dB (nominally 12 dB)
    float getMaxCorrectionDb() const { return mMaxCorrectionDb; }

    // Telemetry & Analysis Query
    const std::vector<float>& getMeasuredSpectrumDb() const { return mMeasuredSpectrumDb; }
    const std::vector<float>& getTargetSpectrumDb() const { return mTargetSpectrumDb; }
    const std::vector<float>& getCorrectionGainsDb() const { return mCorrectionGainsDb; }
    float getSpectralRmsErrorDb() const { return mRmsErrorDb; }

    // Processing
    void process(const float* inL, const float* inR,
                 float* outL, float* outR, size_t numSamples);

    void reset();

private:
    float mSampleRate;
    MasteringTargetCurve mTargetCurve;
    float mMatchDepth;
    float mSmoothingOctaves;
    float mMaxCorrectionDb;
    float mRmsErrorDb;

    static constexpr size_t NUM_BANDS = 32;
    float mBandCentersHz[NUM_BANDS];

    std::vector<float> mMeasuredSpectrumDb;
    std::vector<float> mTargetSpectrumDb;
    std::vector<float> mCorrectionGainsDb;
    std::vector<float> mLinearCorrectionGains;

    // 32-Band IIR Peaking Filter Bank
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

    BandFilter mBandFiltersL[NUM_BANDS];
    BandFilter mBandFiltersR[NUM_BANDS];

    float mBandEnergyAccum[NUM_BANDS];

    void initBandFrequencies();
    void updateTargetCurve();
    void updateCorrectionFilters();
};

} // namespace Jazz
