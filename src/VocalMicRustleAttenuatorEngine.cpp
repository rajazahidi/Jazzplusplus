//=============================================================================
// VocalMicRustleAttenuatorEngine.cpp
//
// Master Studio Pro Suite v32.0 - Apex Transcendence Sovereign Millennium Release
// Real-Time Vocal Microphone Rustle, Clothing Friction & Mechanical Thud Attenuator.
//=============================================================================

#include "VocalMicRustleAttenuatorEngine.h"

namespace Jazz {

static const float kPi = 3.14159265358979323846f;

JZVocalMicRustleAttenuatorEngine::JZVocalMicRustleAttenuatorEngine(float sampleRate)
    : mSampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
    , mReductionAmount(70.0f)
    , mSensitivity(0.75f)
    , mMinFreqHz(80.0f)
    , mMaxFreqHz(1200.0f)
    , mMode(RustleAttenuatorMode::CLEAN_AUDIO)
    , mCurrentAttenuationDb(0.0f)
    , mRustleEventsCount(0)
    , mIsRustleActive(false)
{
    // 8 Sub-bands in friction zone: 85 Hz to 1250 Hz
    const float freqs[NUM_BANDS] = {
        85.0f, 130.0f, 200.0f, 310.0f, 480.0f, 720.0f, 980.0f, 1250.0f
    };
    for (size_t b = 0; b < NUM_BANDS; ++b) {
        mBandFreqs[b] = freqs[b];
        mFastEnergy[b] = 0.0f;
        mVarianceTracker[b] = 0.0f;
        mBandGains[b] = 1.0f;
    }
    initFilterBank();
}

void JZVocalMicRustleAttenuatorEngine::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        mSampleRate = sampleRate;
        initFilterBank();
        reset();
    }
}

void JZVocalMicRustleAttenuatorEngine::setReductionAmount(float percent) {
    mReductionAmount = std::max(0.0f, std::min(100.0f, percent));
}

void JZVocalMicRustleAttenuatorEngine::setSensitivity(float sensitivity) {
    mSensitivity = std::max(0.1f, std::min(1.0f, sensitivity));
}

void JZVocalMicRustleAttenuatorEngine::setFrequencyBand(float minHz, float maxHz) {
    mMinFreqHz = std::max(40.0f, std::min(maxHz - 20.0f, minHz));
    mMaxFreqHz = std::max(mMinFreqHz + 20.0f, std::min(4000.0f, maxHz));
}

void JZVocalMicRustleAttenuatorEngine::setMode(RustleAttenuatorMode mode) {
    mMode = mode;
}

void JZVocalMicRustleAttenuatorEngine::reset() {
    for (size_t b = 0; b < NUM_BANDS; ++b) {
        mBandsL[b].reset();
        mBandsR[b].reset();
        mFastEnergy[b] = 0.0f;
        mVarianceTracker[b] = 0.0f;
        mBandGains[b] = 1.0f;
    }
    mCurrentAttenuationDb = 0.0f;
    mRustleEventsCount = 0;
    mIsRustleActive = false;
}

void JZVocalMicRustleAttenuatorEngine::initFilterBank() {
    const float Q = 2.0f;

    for (size_t b = 0; b < NUM_BANDS; ++b) {
        float f0 = mBandFreqs[b];
        if (f0 >= mSampleRate * 0.48f) f0 = mSampleRate * 0.48f;

        float w0 = 2.0f * kPi * f0 / mSampleRate;
        float alpha = std::sin(w0) / (2.0f * Q);

        float b0 = alpha;
        float b1 = 0.0f;
        float b2 = -alpha;
        float a0 = 1.0f + alpha;
        float a1 = -2.0f * std::cos(w0);
        float a2 = 1.0f - alpha;

        auto setup = [&](BiquadBand& f) {
            f.b0 = b0 / a0;
            f.b1 = b1 / a0;
            f.b2 = b2 / a0;
            f.a1 = a1 / a0;
            f.a2 = a2 / a0;
            f.reset();
        };

        setup(mBandsL[b]);
        setup(mBandsR[b]);
    }
}

void JZVocalMicRustleAttenuatorEngine::process(const float* inL, const float* inR,
                                             float* outL, float* outR, size_t numSamples) {
    if (!inL || !inR || !outL || !outR || numSamples == 0) return;

    const float attackCoeff = 0.015f;  // sub-2ms fast response to friction spikes
    const float releaseCoeff = 0.003f; // ~15ms smooth release
    float maxSuppDb = 0.0f;
    bool rustleFound = false;

    float depthFactor = (mReductionAmount * 0.01f);

    for (size_t i = 0; i < numSamples; ++i) {
        float sL = inL[i];
        float sR = inR[i];

        float outBandSumL = 0.0f;
        float outBandSumR = 0.0f;

        for (size_t b = 0; b < NUM_BANDS; ++b) {
            float bL = mBandsL[b].process(sL);
            float bR = mBandsR[b].process(sR);
            float absB = 0.5f * (std::abs(bL) + std::abs(bR));

            // Fast envelope
            if (absB > mFastEnergy[b]) {
                mFastEnergy[b] += attackCoeff * (absB - mFastEnergy[b]);
            } else {
                mFastEnergy[b] -= releaseCoeff * mFastEnergy[b];
            }

            // Energy variance tracking: friction exhibits chaotic high-frequency noise bursts
            float diff = std::abs(absB - mFastEnergy[b]);
            mVarianceTracker[b] += 0.005f * (diff - mVarianceTracker[b]);

            // Rustle signature: high localized energy variance in the 80 - 1200 Hz region
            float freq = mBandFreqs[b];
            float bandGain = 1.0f;

            if (freq >= mMinFreqHz && freq <= mMaxFreqHz) {
                float frictionMetric = mVarianceTracker[b] * 15.0f * mSensitivity;
                if (frictionMetric > 0.08f) {
                    rustleFound = true;
                    // Compute suppression up to -20 dB
                    float attenDb = -std::min(20.0f, frictionMetric * 28.0f * depthFactor);
                    bandGain = std::pow(10.0f, attenDb / 20.0f);
                    maxSuppDb = std::min(maxSuppDb, attenDb);
                }
            }

            mBandGains[b] = bandGain;
            outBandSumL += bL * bandGain;
            outBandSumR += bR * bandGain;
        }

        // Add back audio outside of the 8 sub-bands (preserving upper highs and sub-bass)
        float flatResidualL = sL;
        float flatResidualR = sR;
        for (size_t b = 0; b < NUM_BANDS; ++b) {
            flatResidualL -= mBandsL[b].process(sL);
            flatResidualR -= mBandsR[b].process(sR);
        }

        float cleanL = outBandSumL + flatResidualL;
        float cleanR = outBandSumR + flatResidualR;

        if (mMode == RustleAttenuatorMode::CLEAN_AUDIO) {
            outL[i] = cleanL;
            outR[i] = cleanR;
        } else {
            // Delta solo: output what was removed
            outL[i] = sL - cleanL;
            outR[i] = sR - cleanR;
        }
    }

    mCurrentAttenuationDb = maxSuppDb;
    mIsRustleActive = rustleFound;
    if (rustleFound) mRustleEventsCount++;
}

} // namespace Jazz
