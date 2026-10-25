//=============================================================================
// SpectralFrequencySplitPannerEngine.cpp
//
// Master Studio Pro Suite v30.0 - Apex Colossus Sovereign Diamond Jubilee Release
// 3-Band Linear-Phase Frequency-Split Mid/Side Dynamic Spectral Panner.
//=============================================================================

#include "SpectralFrequencySplitPannerEngine.h"

namespace Jazz {

static const float kPi = 3.14159265358979323846f;

JZSpectralFrequencySplitPannerEngine::JZSpectralFrequencySplitPannerEngine(float sampleRate)
    : mSampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
    , mLowMidHz(120.0f)
    , mMidHighHz(2800.0f)
    , mLowPan(0.0f)
    , mLowWidth(0.0f)
    , mLowGainDb(0.0f)
    , mLowLinearGain(1.0f)
    , mLowMonoLock(true)
    , mMidPan(0.0f)
    , mMidWidth(1.0f)
    , mMidGainDb(0.0f)
    , mMidLinearGain(1.0f)
    , mHighPan(0.0f)
    , mHighWidth(1.25f)
    , mHighGainDb(0.0f)
    , mHighLinearGain(1.0f)
    , mLfoEnabled(false)
    , mLfoRateHz(0.5f)
    , mLfoDepth(0.4f)
    , mLfoTarget(SpectralPannerTargetBand::HIGH_ONLY)
    , mLfoPhase(0.0f)
    , mPhaseGuardEnabled(true)
    , mPhaseCorrelation(1.0f)
    , mCorrSumLR(0.0f)
    , mCorrSumLL(0.0f)
    , mCorrSumRR(0.0f)
{
    updateCrossoverCoefficients();
}

void JZSpectralFrequencySplitPannerEngine::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        mSampleRate = sampleRate;
        updateCrossoverCoefficients();
        reset();
    }
}

void JZSpectralFrequencySplitPannerEngine::setCrossoverFrequencies(float lowMidHz, float midHighHz) {
    mLowMidHz = std::max(40.0f, std::min(midHighHz - 50.0f, lowMidHz));
    mMidHighHz = std::max(mLowMidHz + 50.0f, std::min(mSampleRate * 0.45f, midHighHz));
    updateCrossoverCoefficients();
}

void JZSpectralFrequencySplitPannerEngine::setLowBand(float pan, float width, float gainDb, bool monoLock) {
    mLowPan = std::max(-1.0f, std::min(1.0f, pan));
    mLowWidth = std::max(0.0f, std::min(1.0f, width));
    mLowGainDb = std::max(-24.0f, std::min(12.0f, gainDb));
    mLowLinearGain = std::pow(10.0f, mLowGainDb / 20.0f);
    mLowMonoLock = monoLock;
}

void JZSpectralFrequencySplitPannerEngine::setMidBand(float pan, float width, float gainDb) {
    mMidPan = std::max(-1.0f, std::min(1.0f, pan));
    mMidWidth = std::max(0.0f, std::min(2.0f, width));
    mMidGainDb = std::max(-24.0f, std::min(12.0f, gainDb));
    mMidLinearGain = std::pow(10.0f, mMidGainDb / 20.0f);
}

void JZSpectralFrequencySplitPannerEngine::setHighBand(float pan, float width, float gainDb) {
    mHighPan = std::max(-1.0f, std::min(1.0f, pan));
    mHighWidth = std::max(0.0f, std::min(2.5f, width));
    mHighGainDb = std::max(-24.0f, std::min(12.0f, gainDb));
    mHighLinearGain = std::pow(10.0f, mHighGainDb / 20.0f);
}

void JZSpectralFrequencySplitPannerEngine::setAutoPanLfo(bool enabled, float rateHz, float depth, SpectralPannerTargetBand target) {
    mLfoEnabled = enabled;
    mLfoRateHz = std::max(0.05f, std::min(15.0f, rateHz));
    mLfoDepth = std::max(0.0f, std::min(1.0f, depth));
    mLfoTarget = target;
}

void JZSpectralFrequencySplitPannerEngine::reset() {
    mLowpassL.reset(); mLowpassR.reset();
    mHighpassL.reset(); mHighpassR.reset();
    mMidLowpassL.reset(); mMidLowpassR.reset();
    mMidHighpassL.reset(); mMidHighpassR.reset();

    mLfoPhase = 0.0f;
    mPhaseCorrelation = 1.0f;
    mCorrSumLR = mCorrSumLL = mCorrSumRR = 0.0f;
}

void JZSpectralFrequencySplitPannerEngine::updateCrossoverCoefficients() {
    const float Q = 0.7071067811865475f; // Butterworth Q

    // 1. Low Crossover at mLowMidHz (Lowpass and Highpass)
    float w0_low = 2.0f * kPi * mLowMidHz / mSampleRate;
    float cos_w0_low = std::cos(w0_low);
    float alpha_low = std::sin(w0_low) / (2.0f * Q);

    // LP Stage coefficients
    float lp_b0 = (1.0f - cos_w0_low) * 0.5f;
    float lp_b1 = 1.0f - cos_w0_low;
    float lp_b2 = (1.0f - cos_w0_low) * 0.5f;
    float a0_lp = 1.0f + alpha_low;
    float lp_a1 = -2.0f * cos_w0_low;
    float lp_a2 = 1.0f - alpha_low;

    // HP Stage coefficients
    float hp_b0 = (1.0f + cos_w0_low) * 0.5f;
    float hp_b1 = -(1.0f + cos_w0_low);
    float hp_b2 = (1.0f + cos_w0_low) * 0.5f;
    float a0_hp = 1.0f + alpha_low;
    float hp_a1 = -2.0f * cos_w0_low;
    float hp_a2 = 1.0f - alpha_low;

    // Set Lowpass LR4 (two identical stages)
    auto setLR4 = [](BiquadLR4& filter, float b0, float b1, float b2, float a0, float a1, float a2) {
        filter.b0_1 = filter.b0_2 = b0 / a0;
        filter.b1_1 = filter.b1_2 = b1 / a0;
        filter.b2_1 = filter.b2_2 = b2 / a0;
        filter.a1_1 = filter.a1_2 = a1 / a0;
        filter.a2_1 = filter.a2_2 = a2 / a0;
    };

    setLR4(mLowpassL, lp_b0, lp_b1, lp_b2, a0_lp, lp_a1, lp_a2);
    setLR4(mLowpassR, lp_b0, lp_b1, lp_b2, a0_lp, lp_a1, lp_a2);
    setLR4(mMidHighpassL, hp_b0, hp_b1, hp_b2, a0_hp, hp_a1, hp_a2);
    setLR4(mMidHighpassR, hp_b0, hp_b1, hp_b2, a0_hp, hp_a1, hp_a2);

    // 2. High Crossover at mMidHighHz (Lowpass and Highpass)
    float w0_high = 2.0f * kPi * mMidHighHz / mSampleRate;
    float cos_w0_high = std::cos(w0_high);
    float alpha_high = std::sin(w0_high) / (2.0f * Q);

    float lp2_b0 = (1.0f - cos_w0_high) * 0.5f;
    float lp2_b1 = 1.0f - cos_w0_high;
    float lp2_b2 = (1.0f - cos_w0_high) * 0.5f;
    float a0_lp2 = 1.0f + alpha_high;
    float lp2_a1 = -2.0f * cos_w0_high;
    float lp2_a2 = 1.0f - alpha_high;

    float hp2_b0 = (1.0f + cos_w0_high) * 0.5f;
    float hp2_b1 = -(1.0f + cos_w0_high);
    float hp2_b2 = (1.0f + cos_w0_high) * 0.5f;
    float a0_hp2 = 1.0f + alpha_high;
    float hp2_a1 = -2.0f * cos_w0_high;
    float hp2_a2 = 1.0f - alpha_high;

    setLR4(mMidLowpassL, lp2_b0, lp2_b1, lp2_b2, a0_lp2, lp2_a1, lp2_a2);
    setLR4(mMidLowpassR, lp2_b0, lp2_b1, lp2_b2, a0_lp2, lp2_a1, lp2_a2);
    setLR4(mHighpassL, hp2_b0, hp2_b1, hp2_b2, a0_hp2, hp2_a1, hp2_a2);
    setLR4(mHighpassR, hp2_b0, hp2_b1, hp2_b2, a0_hp2, hp2_a1, hp2_a2);
}

void JZSpectralFrequencySplitPannerEngine::processBandStereo(float inL, float inR,
                                                           float pan, float width, float gain,
                                                           float& outL, float& outR) {
    // 1. Mid/Side Stereo Width Expansion/Narrowing
    float mid  = 0.5f * (inL + inR);
    float side = 0.5f * (inR - inL);

    // Apply stereo width (0.0 = pure mono, 1.0 = unity, >1.0 = wide)
    float modifiedSide = side * width;

    // Phase Guard Protection: if correlation is negative, dampen out-of-phase side content
    if (mPhaseGuardEnabled && mPhaseCorrelation < 0.0f) {
        float guardDamping = std::max(0.0f, 1.0f + mPhaseCorrelation);
        modifiedSide *= guardDamping;
    }

    float widthL = mid - modifiedSide;
    float widthR = mid + modifiedSide;

    // 2. Constant Power Panning Law
    // Pan: -1.0 (hard left) to +1.0 (hard right)
    float panNorm = (pan + 1.0f) * 0.5f; // 0.0 to 1.0
    float angle = panNorm * (0.5f * kPi);
    float gainL = std::cos(angle) * 1.41421356f;
    float gainR = std::sin(angle) * 1.41421356f;

    outL = widthL * gainL * gain;
    outR = widthR * gainR * gain;
}

void JZSpectralFrequencySplitPannerEngine::process(const float* inL, const float* inR,
                                                  float* outL, float* outR, size_t numSamples) {
    if (!inL || !inR || !outL || !outR || numSamples == 0) return;

    float lfoPhaseInc = 2.0f * kPi * mLfoRateHz / mSampleRate;

    for (size_t i = 0; i < numSamples; ++i) {
        float sL = inL[i];
        float sR = inR[i];

        // 1. Crossover Band Splitting
        // Low Band (< mLowMidHz)
        float lowL = mLowpassL.process(sL);
        float lowR = mLowpassR.process(sR);

        // Above Low Band
        float aboveLowL = mMidHighpassL.process(sL);
        float aboveLowR = mMidHighpassR.process(sR);

        // Mid Band (mLowMidHz to mMidHighHz)
        float midL = mMidLowpassL.process(aboveLowL);
        float midR = mMidLowpassR.process(aboveLowR);

        // High Band (> mMidHighHz)
        float highL = mHighpassL.process(aboveLowL);
        float highR = mHighpassR.process(aboveLowR);

        // 2. LFO Auto-Pan Modulation
        float lfoVal = 0.0f;
        if (mLfoEnabled) {
            lfoVal = std::sin(mLfoPhase) * mLfoDepth;
            mLfoPhase += lfoPhaseInc;
            if (mLfoPhase >= 2.0f * kPi) {
                mLfoPhase -= 2.0f * kPi;
            }
        }

        float effectiveMidPan = mMidPan;
        float effectiveHighPan = mHighPan;
        if (mLfoEnabled) {
            if (mLfoTarget == SpectralPannerTargetBand::MID_ONLY ||
                mLfoTarget == SpectralPannerTargetBand::MID_AND_HIGH) {
                effectiveMidPan = std::max(-1.0f, std::min(1.0f, mMidPan + lfoVal));
            }
            if (mLfoTarget == SpectralPannerTargetBand::HIGH_ONLY ||
                mLfoTarget == SpectralPannerTargetBand::MID_AND_HIGH) {
                effectiveHighPan = std::max(-1.0f, std::min(1.0f, mHighPan + lfoVal));
            }
        }

        // 3. Process Bands
        // Low Band (Enforce mono if lock is enabled)
        float effectiveLowWidth = mLowMonoLock ? 0.0f : mLowWidth;
        float outLowL = 0.0f, outLowR = 0.0f;
        processBandStereo(lowL, lowR, mLowPan, effectiveLowWidth, mLowLinearGain, outLowL, outLowR);

        // Mid Band
        float outMidL = 0.0f, outMidR = 0.0f;
        processBandStereo(midL, midR, effectiveMidPan, mMidWidth, mMidLinearGain, outMidL, outMidR);

        // High Band
        float outHighL = 0.0f, outHighR = 0.0f;
        processBandStereo(highL, highR, effectiveHighPan, mHighWidth, mHighLinearGain, outHighL, outHighR);

        // 4. Recombine Bands
        float resL = outLowL + outMidL + outHighL;
        float resR = outLowR + outMidR + outHighR;

        outL[i] = resL;
        outR[i] = resR;

        // Correlation tracking (low-pass smoothed)
        const float corrAlpha = 0.001f;
        mCorrSumLR += corrAlpha * ((resL * resR) - mCorrSumLR);
        mCorrSumLL += corrAlpha * ((resL * resL) - mCorrSumLL);
        mCorrSumRR += corrAlpha * ((resR * resR) - mCorrSumRR);
    }

    float denom = std::sqrt(std::max(1e-8f, mCorrSumLL * mCorrSumRR));
    mPhaseCorrelation = std::max(-1.0f, std::min(1.0f, mCorrSumLR / denom));
}

} // namespace Jazz
