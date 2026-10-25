//=============================================================================
// MasteringTargetCurveMatcherEngine.cpp
//
// Master Studio Pro Suite v31.0 - Apex Transcendence Sovereign Pinnacle Release
// Mastering Target Curve Spectrum Matcher & 32-Band Morphing Equalizer.
//=============================================================================

#include "MasteringTargetCurveMatcherEngine.h"

namespace Jazz {

static const float kPi = 3.14159265358979323846f;

JZMasteringTargetCurveMatcherEngine::JZMasteringTargetCurveMatcherEngine(float sampleRate)
    : mSampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
    , mTargetCurve(MasteringTargetCurve::MODERN_POP_URBAN)
    , mMatchDepth(100.0f)
    , mSmoothingOctaves(0.33f)
    , mMaxCorrectionDb(12.0f)
    , mRmsErrorDb(0.0f)
    , mMeasuredSpectrumDb(NUM_BANDS, -20.0f)
    , mTargetSpectrumDb(NUM_BANDS, 0.0f)
    , mCorrectionGainsDb(NUM_BANDS, 0.0f)
    , mLinearCorrectionGains(NUM_BANDS, 1.0f)
{
    initBandFrequencies();
    updateTargetCurve();
    updateCorrectionFilters();
}

void JZMasteringTargetCurveMatcherEngine::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        mSampleRate = sampleRate;
        updateCorrectionFilters();
        reset();
    }
}

void JZMasteringTargetCurveMatcherEngine::initBandFrequencies() {
    // 32 ISO 1/3-Octave Center Frequencies
    const float freqs[NUM_BANDS] = {
        20.0f, 25.0f, 31.5f, 40.0f, 50.0f, 63.0f, 80.0f, 100.0f,
        125.0f, 160.0f, 200.0f, 250.0f, 315.0f, 400.0f, 500.0f, 630.0f,
        800.0f, 1000.0f, 1250.0f, 1600.0f, 2000.0f, 2500.0f, 3150.0f, 4000.0f,
        5000.0f, 6300.0f, 8000.0f, 10000.0f, 12500.0f, 15000.0f, 17500.0f, 20000.0f
    };
    for (size_t b = 0; b < NUM_BANDS; ++b) {
        mBandCentersHz[b] = freqs[b];
        mBandEnergyAccum[b] = 0.001f;
    }
}

void JZMasteringTargetCurveMatcherEngine::setTargetCurve(MasteringTargetCurve curve) {
    mTargetCurve = curve;
    updateTargetCurve();
    updateCorrectionFilters();
}

std::string JZMasteringTargetCurveMatcherEngine::getTargetCurveName() const {
    switch (mTargetCurve) {
        case MasteringTargetCurve::PINK_NOISE_REFERENCE: return "Pink Noise (-4.5 dB/octave Natural Reference)";
        case MasteringTargetCurve::MODERN_POP_URBAN:     return "Modern Pop / Urban Commercial Master";
        case MasteringTargetCurve::WARM_ACOUSTIC:        return "Warm Acoustic & Folk Master";
        case MasteringTargetCurve::CLASSICAL_HI_FI:      return "Classical & Audiophile Hi-Fi Flat";
        case MasteringTargetCurve::EDM_CLUB_SUB:         return "EDM / Club Festival Heavy Sub";
        default: return "Target Curve";
    }
}

void JZMasteringTargetCurveMatcherEngine::updateTargetCurve() {
    for (size_t b = 0; b < NUM_BANDS; ++b) {
        float f = mBandCentersHz[b];
        float octDiff = std::log2(f / 1000.0f); // Octave relative to 1 kHz

        switch (mTargetCurve) {
            case MasteringTargetCurve::PINK_NOISE_REFERENCE:
                // -4.5 dB/octave slope
                mTargetSpectrumDb[b] = -4.5f * octDiff;
                break;

            case MasteringTargetCurve::MODERN_POP_URBAN:
                // Boosted sub (<80 Hz) and air (>8 kHz), flat mids
                if (f < 80.0f) mTargetSpectrumDb[b] = 3.5f * (1.0f - f / 80.0f);
                else if (f > 8000.0f) mTargetSpectrumDb[b] = 2.5f * ((f - 8000.0f) / 12000.0f);
                else mTargetSpectrumDb[b] = 0.0f;
                break;

            case MasteringTargetCurve::WARM_ACOUSTIC:
                // Low-mid bump ~250 Hz, smooth high rolloff
                mTargetSpectrumDb[b] = 2.0f * std::exp(-std::pow(octDiff + 2.0f, 2.0f)) - 1.5f * std::max(0.0f, octDiff);
                break;

            case MasteringTargetCurve::CLASSICAL_HI_FI:
                mTargetSpectrumDb[b] = 0.0f; // Linear flat
                break;

            case MasteringTargetCurve::EDM_CLUB_SUB:
                if (f < 90.0f) mTargetSpectrumDb[b] = 6.0f;
                else if (f >= 3000.0f && f <= 6000.0f) mTargetSpectrumDb[b] = 2.0f;
                else mTargetSpectrumDb[b] = 0.0f;
                break;
        }
    }
}

void JZMasteringTargetCurveMatcherEngine::setMatchDepthPercent(float percent) {
    mMatchDepth = std::max(0.0f, std::min(200.0f, percent));
    updateCorrectionFilters();
}

void JZMasteringTargetCurveMatcherEngine::setSmoothingOctaves(float octaves) {
    mSmoothingOctaves = std::max(0.05f, std::min(2.0f, octaves));
    updateCorrectionFilters();
}

void JZMasteringTargetCurveMatcherEngine::setMaxCorrectionDb(float maxDb) {
    mMaxCorrectionDb = std::max(1.0f, std::min(24.0f, maxDb));
    updateCorrectionFilters();
}

void JZMasteringTargetCurveMatcherEngine::updateCorrectionFilters() {
    float depthFactor = mMatchDepth * 0.01f;
    float sigma = mSmoothingOctaves * 3.0f; // Gaussian width in bands

    std::vector<float> rawErrors(NUM_BANDS, 0.0f);
    float sumSq = 0.0f;
    for (size_t b = 0; b < NUM_BANDS; ++b) {
        float err = mTargetSpectrumDb[b] - mMeasuredSpectrumDb[b];
        rawErrors[b] = err;
        sumSq += err * err;
    }
    mRmsErrorDb = std::sqrt(sumSq / static_cast<float>(NUM_BANDS));

    // Apply Gaussian Spectral Smoothing across bands
    for (size_t b = 0; b < NUM_BANDS; ++b) {
        float smoothedErr = 0.0f;
        float weightSum = 0.0f;

        for (int k = 0; k < static_cast<int>(NUM_BANDS); ++k) {
            float dist = static_cast<float>(b) - static_cast<float>(k);
            float weight = std::exp(-(dist * dist) / (2.0f * sigma * sigma));
            smoothedErr += rawErrors[k] * weight;
            weightSum += weight;
        }

        if (weightSum > 1e-4f) smoothedErr /= weightSum;

        // Scale by match depth and clamp
        float gainDb = smoothedErr * depthFactor;
        gainDb = std::max(-mMaxCorrectionDb, std::min(mMaxCorrectionDb, gainDb));

        mCorrectionGainsDb[b] = gainDb;
        mLinearCorrectionGains[b] = std::pow(10.0f, gainDb / 20.0f);

        // Update peaking biquad filter coefficients
        float f0 = mBandCentersHz[b];
        if (f0 >= mSampleRate * 0.48f) f0 = mSampleRate * 0.48f;

        float w0 = 2.0f * kPi * f0 / mSampleRate;
        float A = std::pow(10.0f, gainDb / 40.0f);
        float Q = 4.318f; // 1/3-octave Q
        float alpha = std::sin(w0) / (2.0f * Q);

        float b0 = 1.0f + alpha * A;
        float b1 = -2.0f * std::cos(w0);
        float b2 = 1.0f - alpha * A;
        float a0 = 1.0f + alpha / A;
        float a1 = -2.0f * std::cos(w0);
        float a2 = 1.0f - alpha / A;

        auto setupFilter = [&](BandFilter& f) {
            f.b0 = b0 / a0;
            f.b1 = b1 / a0;
            f.b2 = b2 / a0;
            f.a1 = a1 / a0;
            f.a2 = a2 / a0;
        };

        setupFilter(mBandFiltersL[b]);
        setupFilter(mBandFiltersR[b]);
    }
}

void JZMasteringTargetCurveMatcherEngine::reset() {
    for (size_t b = 0; b < NUM_BANDS; ++b) {
        mBandFiltersL[b].reset();
        mBandFiltersR[b].reset();
        mBandEnergyAccum[b] = 0.001f;
    }
    std::fill(mMeasuredSpectrumDb.begin(), mMeasuredSpectrumDb.end(), -20.0f);
    mRmsErrorDb = 0.0f;
    updateCorrectionFilters();
}

void JZMasteringTargetCurveMatcherEngine::process(const float* inL, const float* inR,
                                                float* outL, float* outR, size_t numSamples) {
    if (!inL || !inR || !outL || !outR || numSamples == 0) return;

    for (size_t i = 0; i < numSamples; ++i) {
        float sL = inL[i];
        float sR = inR[i];

        // Cascade through 32 peaking correction filters
        for (size_t b = 0; b < NUM_BANDS; ++b) {
            sL = mBandFiltersL[b].process(sL);
            sR = mBandFiltersR[b].process(sR);
        }

        outL[i] = sL;
        outR[i] = sR;

        // Spectrum energy tracking (slow decay)
        float midSample = 0.5f * (std::abs(sL) + std::abs(sR));
        mBandEnergyAccum[i % NUM_BANDS] += 0.002f * (midSample - mBandEnergyAccum[i % NUM_BANDS]);
    }

    // Refresh measured spectrum telemetry
    for (size_t b = 0; b < NUM_BANDS; ++b) {
        float energy = std::max(1e-5f, mBandEnergyAccum[b]);
        mMeasuredSpectrumDb[b] = 20.0f * std::log10(energy);
    }
}

} // namespace Jazz
