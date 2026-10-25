//=============================================================================
// NeuralSpectralDereverberationEngine.cpp
//
// Master Studio Pro Suite v31.0 - Apex Transcendence Sovereign Pinnacle Release
// Real-Time Neural Spectral Dereverberation & Early Reflection Stripper.
//=============================================================================

#include "NeuralSpectralDereverberationEngine.h"

namespace Jazz {

static const float kPi = 3.14159265358979323846f;

JZNeuralSpectralDereverberationEngine::JZNeuralSpectralDereverberationEngine(float sampleRate)
    : mSampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
    , mDereverbAmount(60.0f)
    , mEarlySuppressionDb(-12.0f)
    , mRoomSizeMeters(6.5f)
    , mTransientPreservation(0.85f)
    , mMode(DereverbMode::DEREVERB_CLEAN)
    , mEstimatedRt60Sec(0.65f)
    , mCurrentSuppressionDb(0.0f)
    , mIsConsonantActive(false)
    , mDelayWriteIdx(0)
    , mDelayLengthSamples(0)
    , mPrevSampleL(0.0f)
    , mPrevSampleR(0.0f)
{
    // 16 Logarithmic Centers from 60 Hz to 14 kHz
    const float freqs[NUM_BANDS] = {
        60.0f, 95.0f, 150.0f, 240.0f, 380.0f, 600.0f, 950.0f, 1500.0f,
        2400.0f, 3800.0f, 5500.0f, 7500.0f, 9500.0f, 11500.0f, 13500.0f, 15500.0f
    };
    for (size_t b = 0; b < NUM_BANDS; ++b) {
        mBandFrequencies[b] = freqs[b];
        mBandFastEnvelopes[b] = 0.0f;
        mBandSlowEnvelopes[b] = 0.0f;
        mBandGains[b] = 1.0f;
    }

    size_t maxDelay = static_cast<size_t>(mSampleRate * 0.06f); // 60ms buffer
    mEarlyDelayBufL.assign(maxDelay, 0.0f);
    mEarlyDelayBufR.assign(maxDelay, 0.0f);

    updateDelayLength();
    initFilterBank();
}

void JZNeuralSpectralDereverberationEngine::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        mSampleRate = sampleRate;
        size_t maxDelay = static_cast<size_t>(mSampleRate * 0.06f);
        mEarlyDelayBufL.assign(maxDelay, 0.0f);
        mEarlyDelayBufR.assign(maxDelay, 0.0f);
        updateDelayLength();
        initFilterBank();
        reset();
    }
}

void JZNeuralSpectralDereverberationEngine::setDereverbAmount(float percent) {
    mDereverbAmount = std::max(0.0f, std::min(100.0f, percent));
}

void JZNeuralSpectralDereverberationEngine::setEarlyReflectionSuppressionDb(float db) {
    mEarlySuppressionDb = std::max(-30.0f, std::min(0.0f, db));
}

void JZNeuralSpectralDereverberationEngine::setRoomSizeEstimate(float meters) {
    mRoomSizeMeters = std::max(1.5f, std::min(30.0f, meters));
    updateDelayLength();
}

void JZNeuralSpectralDereverberationEngine::setTransientPreservation(float amount) {
    mTransientPreservation = std::max(0.0f, std::min(1.0f, amount));
}

void JZNeuralSpectralDereverberationEngine::setMode(DereverbMode mode) {
    mMode = mode;
}

void JZNeuralSpectralDereverberationEngine::updateDelayLength() {
    // Speed of sound c ~ 343 m/s
    // Round-trip early wall reflection time ~ 2 * roomSize / 343
    float delaySec = (2.0f * mRoomSizeMeters) / 343.0f;
    delaySec = std::max(0.005f, std::min(0.045f, delaySec)); // clamp 5ms to 45ms
    mDelayLengthSamples = static_cast<size_t>(delaySec * mSampleRate);
    if (mDelayLengthSamples >= mEarlyDelayBufL.size()) {
        mDelayLengthSamples = mEarlyDelayBufL.size() - 1;
    }
}

void JZNeuralSpectralDereverberationEngine::initFilterBank() {
    const float Q = 2.0f;

    for (size_t b = 0; b < NUM_BANDS; ++b) {
        float f0 = mBandFrequencies[b];
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

        setup(mFiltersL[b]);
        setup(mFiltersR[b]);
    }
}

void JZNeuralSpectralDereverberationEngine::reset() {
    for (size_t b = 0; b < NUM_BANDS; ++b) {
        mFiltersL[b].reset();
        mFiltersR[b].reset();
        mBandFastEnvelopes[b] = 0.0f;
        mBandSlowEnvelopes[b] = 0.0f;
        mBandGains[b] = 1.0f;
    }
    std::fill(mEarlyDelayBufL.begin(), mEarlyDelayBufL.end(), 0.0f);
    std::fill(mEarlyDelayBufR.begin(), mEarlyDelayBufR.end(), 0.0f);
    mDelayWriteIdx = 0;
    mPrevSampleL = mPrevSampleR = 0.0f;
    mCurrentSuppressionDb = 0.0f;
    mIsConsonantActive = false;
}

void JZNeuralSpectralDereverberationEngine::process(const float* inL, const float* inR,
                                                  float* outL, float* outR, size_t numSamples) {
    if (!inL || !inR || !outL || !outR || numSamples == 0) return;

    const float fastAttack = 0.05f;
    const float fastRelease = 0.008f;
    const float slowAttack = 0.002f;
    const float slowRelease = 0.0005f;

    float earlyReflLinearAtten = std::pow(10.0f, mEarlySuppressionDb / 20.0f);
    float dereverbDepth = mDereverbAmount * 0.01f;
    size_t bufSize = mEarlyDelayBufL.size();

    float maxSuppDb = 0.0f;
    bool consonantFound = false;

    for (size_t i = 0; i < numSamples; ++i) {
        float sL = inL[i];
        float sR = inR[i];

        // 1. Transient attack detection (protects unvoiced consonants like s, t, p, k)
        float diffL = std::abs(sL - mPrevSampleL);
        float diffR = std::abs(sR - mPrevSampleR);
        mPrevSampleL = sL;
        mPrevSampleR = sR;

        bool isTransient = (diffL > 0.15f || diffR > 0.15f);
        if (isTransient) consonantFound = true;

        // 2. Early reflection comb cancellation
        mEarlyDelayBufL[mDelayWriteIdx] = sL;
        mEarlyDelayBufR[mDelayWriteIdx] = sR;

        size_t readIdx = (mDelayWriteIdx + bufSize - mDelayLengthSamples) % bufSize;
        float delayedL = mEarlyDelayBufL[readIdx];
        float delayedR = mEarlyDelayBufR[readIdx];
        mDelayWriteIdx = (mDelayWriteIdx + 1) % bufSize;

        // Subtract phase-aligned wall reflection tap
        float earlyCleanL = sL - (delayedL * (1.0f - earlyReflLinearAtten) * dereverbDepth * 0.35f);
        float earlyCleanR = sR - (delayedR * (1.0f - earlyReflLinearAtten) * dereverbDepth * 0.35f);

        // 3. Multi-band diffuse reverberant tail subtraction
        float cleanAccumL = 0.0f;
        float cleanAccumR = 0.0f;

        for (size_t b = 0; b < NUM_BANDS; ++b) {
            float bL = mFiltersL[b].process(earlyCleanL);
            float bR = mFiltersR[b].process(earlyCleanR);
            float absB = 0.5f * (std::abs(bL) + std::abs(bR));

            // Fast envelope (direct sound + transients)
            if (absB > mBandFastEnvelopes[b]) {
                mBandFastEnvelopes[b] += fastAttack * (absB - mBandFastEnvelopes[b]);
            } else {
                mBandFastEnvelopes[b] -= fastRelease * mBandFastEnvelopes[b];
            }

            // Slow envelope (diffuse room tail accumulation)
            if (absB > mBandSlowEnvelopes[b]) {
                mBandSlowEnvelopes[b] += slowAttack * (absB - mBandSlowEnvelopes[b]);
            } else {
                mBandSlowEnvelopes[b] -= slowRelease * mBandSlowEnvelopes[b];
            }

            // Dereverberation gain calculation:
            // When slow envelope (reverberation) dominates fast envelope, attenuate diffuse tail
            float directVsRevRatio = (mBandSlowEnvelopes[b] > 1e-5f)
                                     ? (mBandFastEnvelopes[b] / mBandSlowEnvelopes[b])
                                     : 1.0f;

            float bandGain = 1.0f;
            if (directVsRevRatio < 1.0f) {
                // Tail suppression factor
                float sup = std::max(0.15f, directVsRevRatio);
                bandGain = 1.0f - dereverbDepth * (1.0f - sup);
            }

            // Consonant protection bypass
            if (isTransient) {
                bandGain = bandGain * (1.0f - mTransientPreservation) + mTransientPreservation;
            }

            mBandGains[b] = bandGain;
            float grDb = (bandGain > 1e-4f) ? (20.0f * std::log10(bandGain)) : -30.0f;
            maxSuppDb = std::min(maxSuppDb, grDb);

            cleanAccumL += bL * bandGain;
            cleanAccumR += bR * bandGain;
        }

        if (mMode == DereverbMode::DEREVERB_CLEAN) {
            outL[i] = cleanAccumL;
            outR[i] = cleanAccumR;
        } else {
            // DELTA_REVERB_SOLO: output what was removed
            outL[i] = sL - cleanAccumL;
            outR[i] = sR - cleanAccumR;
        }
    }

    mCurrentSuppressionDb = maxSuppDb;
    mIsConsonantActive = consonantFound;
}

} // namespace Jazz
