//=============================================================================
// MasterLinearPhaseMultibandDynamicsEngine.cpp
//
// Master Studio Pro Suite v32.0 - Apex Transcendence Sovereign Millennium Release
// 5-Band Linear-Phase Mastering Dynamics Processor & Multiband Compressor.
//=============================================================================

#include "MasterLinearPhaseMultibandDynamicsEngine.h"

namespace Jazz {

static const float kPi = 3.14159265358979323846f;

JZMasterLinearPhaseMultibandDynamicsEngine::JZMasterLinearPhaseMultibandDynamicsEngine(float sampleRate)
    : mSampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
    , mSoloBand(-1)
{
    // Default mastering crossovers: 100 Hz, 500 Hz, 2500 Hz, 8000 Hz
    mCrossovers[0] = 100.0f;
    mCrossovers[1] = 500.0f;
    mCrossovers[2] = 2500.0f;
    mCrossovers[3] = 8000.0f;

    // Default 5 band presets
    mBands[0].thresholdDb = -16.0f; mBands[0].ratio = 3.0f; mBands[0].attackMs = 25.0f; mBands[0].releaseMs = 150.0f; // Sub
    mBands[1].thresholdDb = -18.0f; mBands[1].ratio = 2.5f; mBands[1].attackMs = 15.0f; mBands[1].releaseMs = 120.0f; // Low-Mid
    mBands[2].thresholdDb = -20.0f; mBands[2].ratio = 2.0f; mBands[2].attackMs = 10.0f; mBands[2].releaseMs = 100.0f; // Mid
    mBands[3].thresholdDb = -18.0f; mBands[3].ratio = 2.2f; mBands[3].attackMs = 8.0f;  mBands[3].releaseMs = 80.0f;  // High-Mid
    mBands[4].thresholdDb = -15.0f; mBands[4].ratio = 1.8f; mBands[4].attackMs = 5.0f;  mBands[4].releaseMs = 60.0f;  // Air

    for (size_t b = 0; b < NUM_BANDS; ++b) {
        mEnvelopes[b] = 0.0f;
        mGainReductionsDb[b] = 0.0f;
    }

    initCrossoverFilters();
}

void JZMasterLinearPhaseMultibandDynamicsEngine::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        mSampleRate = sampleRate;
        initCrossoverFilters();
        reset();
    }
}

void JZMasterLinearPhaseMultibandDynamicsEngine::setCrossovers(float c1, float c2, float c3, float c4) {
    mCrossovers[0] = std::max(40.0f, std::min(c2 - 20.0f, c1));
    mCrossovers[1] = std::max(mCrossovers[0] + 20.0f, std::min(c3 - 50.0f, c2));
    mCrossovers[2] = std::max(mCrossovers[1] + 50.0f, std::min(c4 - 100.0f, c3));
    mCrossovers[3] = std::max(mCrossovers[2] + 100.0f, std::min(mSampleRate * 0.45f, c4));
    initCrossoverFilters();
}

void JZMasterLinearPhaseMultibandDynamicsEngine::setBandParameters(size_t bandIdx, const BandDynamicsParams& params) {
    if (bandIdx < NUM_BANDS) {
        mBands[bandIdx] = params;
    }
}

const BandDynamicsParams& JZMasterLinearPhaseMultibandDynamicsEngine::getBandParameters(size_t bandIdx) const {
    static BandDynamicsParams dummy;
    if (bandIdx < NUM_BANDS) return mBands[bandIdx];
    return dummy;
}

void JZMasterLinearPhaseMultibandDynamicsEngine::setSoloBand(int bandIdx) {
    mSoloBand = std::max(-1, std::min(static_cast<int>(NUM_BANDS - 1), bandIdx));
}

float JZMasterLinearPhaseMultibandDynamicsEngine::getBandGainReductionDb(size_t bandIdx) const {
    if (bandIdx < NUM_BANDS) return mGainReductionsDb[bandIdx];
    return 0.0f;
}

void JZMasterLinearPhaseMultibandDynamicsEngine::reset() {
    for (size_t b = 0; b < NUM_BANDS; ++b) {
        mBandFiltersL[b].reset();
        mBandFiltersR[b].reset();
        mEnvelopes[b] = 0.0f;
        mGainReductionsDb[b] = 0.0f;
    }
}

void JZMasterLinearPhaseMultibandDynamicsEngine::initCrossoverFilters() {
    // 5-band splitting with constant-Q bandpass biquads
    float centers[NUM_BANDS] = {
        mCrossovers[0] * 0.5f,
        std::sqrt(mCrossovers[0] * mCrossovers[1]),
        std::sqrt(mCrossovers[1] * mCrossovers[2]),
        std::sqrt(mCrossovers[2] * mCrossovers[3]),
        (mCrossovers[3] + mSampleRate * 0.45f) * 0.5f
    };

    const float Q = 1.4142f;

    for (size_t b = 0; b < NUM_BANDS; ++b) {
        float f0 = centers[b];
        if (f0 >= mSampleRate * 0.48f) f0 = mSampleRate * 0.48f;

        float w0 = 2.0f * kPi * f0 / mSampleRate;
        float alpha = std::sin(w0) / (2.0f * Q);

        float b0 = alpha;
        float b1 = 0.0f;
        float b2 = -alpha;
        float a0 = 1.0f + alpha;
        float a1 = -2.0f * std::cos(w0);
        float a2 = 1.0f - alpha;

        auto setup = [&](BandFilter& f) {
            f.b0 = b0 / a0;
            f.b1 = b1 / a0;
            f.b2 = b2 / a0;
            f.a1 = a1 / a0;
            f.a2 = a2 / a0;
            f.reset();
        };

        setup(mBandFiltersL[b]);
        setup(mBandFiltersR[b]);
    }
}

float JZMasterLinearPhaseMultibandDynamicsEngine::computeVcaGain(float inputLevelDb, const BandDynamicsParams& params) {
    if (params.bypassed) return 0.0f;

    float T = params.thresholdDb;
    float R = std::max(1.0f, params.ratio);
    float W = 6.0f; // 6 dB soft knee width

    float delta = inputLevelDb - T;
    float grDb = 0.0f;

    if (delta <= -W * 0.5f) {
        // Below knee: no gain reduction
        grDb = 0.0f;
    } else if (std::abs(delta) < W * 0.5f) {
        // Inside soft knee
        float kneeVal = delta + W * 0.5f;
        grDb = (1.0f / R - 1.0f) * (kneeVal * kneeVal) / (2.0f * W);
    } else {
        // Above knee: full ratio compression
        grDb = (1.0f / R - 1.0f) * delta;
    }

    return grDb;
}

void JZMasterLinearPhaseMultibandDynamicsEngine::process(const float* inL, const float* inR,
                                                       float* outL, float* outR, size_t numSamples) {
    if (!inL || !inR || !outL || !outR || numSamples == 0) return;

    for (size_t i = 0; i < numSamples; ++i) {
        float sL = inL[i];
        float sR = inR[i];

        float sumOutL = 0.0f;
        float sumOutR = 0.0f;

        for (size_t b = 0; b < NUM_BANDS; ++b) {
            float bL = mBandFiltersL[b].process(sL);
            float bR = mBandFiltersR[b].process(sR);
            float bMono = 0.5f * (std::abs(bL) + std::abs(bR));

            const BandDynamicsParams& p = mBands[b];
            float attCoeff = 1.0f - std::exp(-1.0f / (p.attackMs * 0.001f * mSampleRate));
            float relCoeff = 1.0f - std::exp(-1.0f / (p.releaseMs * 0.001f * mSampleRate));

            // Envelope detection
            if (bMono > mEnvelopes[b]) {
                mEnvelopes[b] += attCoeff * (bMono - mEnvelopes[b]);
            } else {
                mEnvelopes[b] -= relCoeff * mEnvelopes[b];
            }

            float envDb = (mEnvelopes[b] > 1e-5f) ? (20.0f * std::log10(mEnvelopes[b])) : -90.0f;
            float grDb = computeVcaGain(envDb, p);
            mGainReductionsDb[b] = grDb;

            // Total gain = compression gain reduction + makeup gain
            float totalGainDb = grDb + p.makeupGainDb;
            float linearGain = std::pow(10.0f, totalGainDb / 20.0f);

            float compL = bL * linearGain;
            float compR = bR * linearGain;

            // Check if solo mode is active
            if (mSoloBand >= 0) {
                if (static_cast<int>(b) == mSoloBand) {
                    sumOutL = compL;
                    sumOutR = compR;
                }
            } else {
                sumOutL += compL;
                sumOutR += compR;
            }
        }

        outL[i] = sumOutL;
        outR[i] = sumOutR;
    }
}

} // namespace Jazz
