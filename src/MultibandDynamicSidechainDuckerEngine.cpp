//=============================================================================
// MultibandDynamicSidechainDuckerEngine.cpp
//
// Master Studio Pro Suite v30.0 - Apex Colossus Sovereign Diamond Jubilee Release
// 16-Band Sub-Band Dynamic Sidechain Carving Ducker & Frequency Isolation Engine.
//=============================================================================

#include "MultibandDynamicSidechainDuckerEngine.h"

namespace Jazz {

static const float kPi = 3.14159265358979323846f;

JZMultibandDynamicSidechainDuckerEngine::JZMultibandDynamicSidechainDuckerEngine(float sampleRate)
    : mSampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
    , mCarveAmount(50.0f)
    , mMinFreqHz(40.0f)
    , mMaxFreqHz(16000.0f)
    , mAttackMs(1.5f)
    , mReleaseMs(80.0f)
    , mMode(SidechainDuckerMode::CARVE)
    , mMaxGainReductionDb(0.0f)
    , mActiveCarvedBands(0)
    , mBandGainReductionsDb(NUM_BANDS, 0.0f)
{
    // 16 Logarithmic Centers
    const float centers[NUM_BANDS] = {
        50.0f,   75.0f,  110.0f,  160.0f,  240.0f,  360.0f,  540.0f,  800.0f,
        1200.0f, 1800.0f, 2700.0f, 4000.0f, 6000.0f, 8500.0f, 11500.0f, 15000.0f
    };
    for (size_t b = 0; b < NUM_BANDS; ++b) {
        mBandFrequencies[b] = centers[b];
        mSideEnvelopes[b] = 0.0f;
        mBandDynamicGains[b] = 1.0f;
    }

    initFilterBands();
}

void JZMultibandDynamicSidechainDuckerEngine::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        mSampleRate = sampleRate;
        initFilterBands();
        reset();
    }
}

void JZMultibandDynamicSidechainDuckerEngine::setCarveAmount(float percent) {
    mCarveAmount = std::max(0.0f, std::min(100.0f, percent));
}

void JZMultibandDynamicSidechainDuckerEngine::setFrequencyFocus(float minHz, float maxHz) {
    mMinFreqHz = std::max(20.0f, std::min(maxHz - 20.0f, minHz));
    mMaxFreqHz = std::max(mMinFreqHz + 20.0f, std::min(mSampleRate * 0.48f, maxHz));
}

void JZMultibandDynamicSidechainDuckerEngine::setAttackTimeMs(float attackMs) {
    mAttackMs = std::max(0.1f, std::min(50.0f, attackMs));
}

void JZMultibandDynamicSidechainDuckerEngine::setReleaseTimeMs(float releaseMs) {
    mReleaseMs = std::max(5.0f, std::min(1000.0f, releaseMs));
}

void JZMultibandDynamicSidechainDuckerEngine::setMode(SidechainDuckerMode mode) {
    mMode = mode;
}

void JZMultibandDynamicSidechainDuckerEngine::reset() {
    for (size_t b = 0; b < NUM_BANDS; ++b) {
        mMainFiltersL[b].reset();
        mMainFiltersR[b].reset();
        mSideFiltersL[b].reset();
        mSideFiltersR[b].reset();
        mSideEnvelopes[b] = 0.0f;
        mBandDynamicGains[b] = 1.0f;
        mBandGainReductionsDb[b] = 0.0f;
    }
    mMaxGainReductionDb = 0.0f;
    mActiveCarvedBands = 0;
}

void JZMultibandDynamicSidechainDuckerEngine::initFilterBands() {
    const float Q = 2.2f; // Overlapping constant-Q bandpass biquads

    for (size_t b = 0; b < NUM_BANDS; ++b) {
        float f0 = mBandFrequencies[b];
        if (f0 >= mSampleRate * 0.48f) {
            f0 = mSampleRate * 0.48f;
        }

        float w0 = 2.0f * kPi * f0 / mSampleRate;
        float alpha = std::sin(w0) / (2.0f * Q);

        float b0 = alpha;
        float b1 = 0.0f;
        float b2 = -alpha;
        float a0 = 1.0f + alpha;
        float a1 = -2.0f * std::cos(w0);
        float a2 = 1.0f - alpha;

        auto setupBiquad = [&](BandpassBiquad& f) {
            f.b0 = b0 / a0;
            f.b1 = b1 / a0;
            f.b2 = b2 / a0;
            f.a1 = a1 / a0;
            f.a2 = a2 / a0;
            f.reset();
        };

        setupBiquad(mMainFiltersL[b]);
        setupBiquad(mMainFiltersR[b]);
        setupBiquad(mSideFiltersL[b]);
        setupBiquad(mSideFiltersR[b]);
    }
}

void JZMultibandDynamicSidechainDuckerEngine::updateAttackReleaseCoefficients(float& attackCoeff, float& releaseCoeff) {
    attackCoeff = 1.0f - std::exp(-1.0f / (mAttackMs * 0.001f * mSampleRate));
    releaseCoeff = 1.0f - std::exp(-1.0f / (mReleaseMs * 0.001f * mSampleRate));
}

void JZMultibandDynamicSidechainDuckerEngine::process(const float* mainInL, const float* mainInR,
                                                    const float* sideInL, const float* sideInR,
                                                    float* outL, float* outR, size_t numSamples) {
    if (!mainInL || !mainInR || !outL || !outR || numSamples == 0) return;

    // Direct listen mode
    if (mMode == SidechainDuckerMode::SIDECHAIN_LISTEN) {
        for (size_t i = 0; i < numSamples; ++i) {
            outL[i] = sideInL ? sideInL[i] : 0.0f;
            outR[i] = sideInR ? sideInR[i] : 0.0f;
        }
        return;
    }

    float attackCoeff = 0.0f, releaseCoeff = 0.0f;
    updateAttackReleaseCoefficients(attackCoeff, releaseCoeff);

    float maxGrDb = 0.0f;
    size_t activeCount = 0;

    for (size_t i = 0; i < numSamples; ++i) {
        float inL = mainInL[i];
        float inR = mainInR[i];
        float sL = sideInL ? sideInL[i] : 0.0f;
        float sR = sideInR ? sideInR[i] : 0.0f;
        float sMono = 0.5f * (std::abs(sL) + std::abs(sR));

        float carvedL = 0.0f;
        float carvedR = 0.0f;

        // Process across 16 bands
        for (size_t b = 0; b < NUM_BANDS; ++b) {
            // Filter main signal
            float mBandL = mMainFiltersL[b].process(inL);
            float mBandR = mMainFiltersR[b].process(inR);

            // Filter sidechain signal to detect energy in this band
            float sideBand = mSideFiltersL[b].process(sMono);
            float sideAbs = std::abs(sideBand);

            // Sidechain envelope follower
            if (sideAbs > mSideEnvelopes[b]) {
                mSideEnvelopes[b] += attackCoeff * (sideAbs - mSideEnvelopes[b]);
            } else {
                mSideEnvelopes[b] -= releaseCoeff * mSideEnvelopes[b];
            }

            // Check if band is within active frequency focus
            float freq = mBandFrequencies[b];
            float bandGain = 1.0f;
            float grDb = 0.0f;

            if (freq >= mMinFreqHz && freq <= mMaxFreqHz) {
                // Calculate dynamic gain reduction proportional to sidechain level
                float sideDb = (mSideEnvelopes[b] > 1e-4f) ? (20.0f * std::log10(mSideEnvelopes[b])) : -80.0f;
                // Threshold around -36 dB
                if (sideDb > -36.0f) {
                    float overDb = sideDb + 36.0f;
                    // Max reduction up to -24 dB when carve is 100%
                    grDb = -(overDb * (mCarveAmount * 0.01f) * 0.65f);
                    grDb = std::max(-24.0f, grDb);
                    bandGain = std::pow(10.0f, grDb / 20.0f);
                }
            }

            mBandDynamicGains[b] = bandGain;
            mBandGainReductionsDb[b] = grDb;

            if (grDb < -0.5f) {
                activeCount++;
                maxGrDb = std::min(maxGrDb, grDb);
            }

            carvedL += mBandL * bandGain;
            carvedR += mBandR * bandGain;
        }

        // Output mode selection
        if (mMode == SidechainDuckerMode::CARVE) {
            outL[i] = carvedL;
            outR[i] = carvedR;
        } else if (mMode == SidechainDuckerMode::DELTA_SOLO) {
            // Delta: output what was removed by the ducker
            outL[i] = inL - carvedL;
            outR[i] = inR - carvedR;
        }
    }

    mMaxGainReductionDb = maxGrDb;
    mActiveCarvedBands = (activeCount > 0) ? (activeCount / numSamples) : 0;
}

} // namespace Jazz
