//=============================================================================
// SpatialTransientTonalBalancerEngine.cpp
//
// Master Studio Pro Suite v32.0 - Apex Transcendence Sovereign Millennium Release
// Spatial Mid/Side Transient-Tonal Polar Balancer & Stereo Image Sculptor.
//=============================================================================

#include "SpatialTransientTonalBalancerEngine.h"

namespace Jazz {

static const float kPi = 3.14159265358979323846f;

JZSpatialTransientTonalBalancerEngine::JZSpatialTransientTonalBalancerEngine(float sampleRate)
    : mSampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
    , mTransientWidth(0.5f) // tight focused transients
    , mTransientPan(0.0f)
    , mTransientGainDb(0.0f)
    , mLinearTransientGain(1.0f)
    , mTonalWidth(1.4f)     // expansive wide reverb/body
    , mTonalPan(0.0f)
    , mTonalGainDb(0.0f)
    , mLinearTonalGain(1.0f)
    , mTransientRatio(0.0f)
    , mPhaseCorrelation(1.0f)
    , mCorrLR(0.0f), mCorrLL(0.0f), mCorrRR(0.0f)
    , mFastEnvM(0.0f), mSlowEnvM(0.0f)
    , mFastEnvS(0.0f), mSlowEnvS(0.0f)
{
    setTransientGainDb(mTransientGainDb);
    setTonalGainDb(mTonalGainDb);
}

void JZSpatialTransientTonalBalancerEngine::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        mSampleRate = sampleRate;
        reset();
    }
}

void JZSpatialTransientTonalBalancerEngine::setTransientWidth(float width) {
    mTransientWidth = std::max(0.0f, std::min(2.0f, width));
}

void JZSpatialTransientTonalBalancerEngine::setTransientPan(float pan) {
    mTransientPan = std::max(-1.0f, std::min(1.0f, pan));
}

void JZSpatialTransientTonalBalancerEngine::setTransientGainDb(float db) {
    mTransientGainDb = std::max(-24.0f, std::min(12.0f, db));
    mLinearTransientGain = std::pow(10.0f, mTransientGainDb / 20.0f);
}

void JZSpatialTransientTonalBalancerEngine::setTonalWidth(float width) {
    mTonalWidth = std::max(0.0f, std::min(2.5f, width));
}

void JZSpatialTransientTonalBalancerEngine::setTonalPan(float pan) {
    mTonalPan = std::max(-1.0f, std::min(1.0f, pan));
}

void JZSpatialTransientTonalBalancerEngine::setTonalGainDb(float db) {
    mTonalGainDb = std::max(-24.0f, std::min(12.0f, db));
    mLinearTonalGain = std::pow(10.0f, mTonalGainDb / 20.0f);
}

void JZSpatialTransientTonalBalancerEngine::reset() {
    mFastEnvM = mSlowEnvM = 0.0f;
    mFastEnvS = mSlowEnvS = 0.0f;
    mCorrLR = mCorrLL = mCorrRR = 0.0f;
    mPhaseCorrelation = 1.0f;
    mTransientRatio = 0.0f;
}

void JZSpatialTransientTonalBalancerEngine::process(const float* inL, const float* inR,
                                                  float* outL, float* outR, size_t numSamples) {
    if (!inL || !inR || !outL || !outR || numSamples == 0) return;

    // Fast attack (0.5ms) vs slow decay (25ms) differential envelope coefficients
    const float fastAtt = 0.08f;
    const float fastRel = 0.01f;
    const float slowAtt = 0.005f;
    const float slowRel = 0.001f;

    // Pan gains
    auto calcPan = [](float pan, float& gL, float& gR) {
        float norm = (pan + 1.0f) * 0.5f;
        float angle = norm * (0.5f * kPi);
        gL = std::cos(angle) * 1.4142f;
        gR = std::sin(angle) * 1.4142f;
    };

    float transPanL, transPanR;
    calcPan(mTransientPan, transPanL, transPanR);

    float tonalPanL, tonalPanR;
    calcPan(mTonalPan, tonalPanL, tonalPanR);

    float transEnergyTotal = 0.0f;
    float tonalEnergyTotal = 0.0f;

    for (size_t i = 0; i < numSamples; ++i) {
        float sL = inL[i];
        float sR = inR[i];

        // 1. Convert to Mid / Side
        float mid  = 0.5f * (sL + sR);
        float side = 0.5f * (sR - sL);

        // 2. Mid Envelope Tracking
        float absM = std::abs(mid);
        if (absM > mFastEnvM) mFastEnvM += fastAtt * (absM - mFastEnvM);
        else mFastEnvM -= fastRel * mFastEnvM;

        if (absM > mSlowEnvM) mSlowEnvM += slowAtt * (absM - mSlowEnvM);
        else mSlowEnvM -= slowRel * mSlowEnvM;

        // Transient difference in Mid
        float transDiffM = std::max(0.0f, mFastEnvM - mSlowEnvM);
        float transWeightM = (absM > 1e-4f) ? std::min(1.0f, transDiffM / (absM + 1e-4f)) : 0.0f;

        float midTrans = mid * transWeightM;
        float midTonal = mid - midTrans;

        // 3. Side Envelope Tracking
        float absS = std::abs(side);
        if (absS > mFastEnvS) mFastEnvS += fastAtt * (absS - mFastEnvS);
        else mFastEnvS -= fastRel * mFastEnvS;

        if (absS > mSlowEnvS) mSlowEnvS += slowAtt * (absS - mSlowEnvS);
        else mSlowEnvS -= slowRel * mSlowEnvS;

        float transDiffS = std::max(0.0f, mFastEnvS - mSlowEnvS);
        float transWeightS = (absS > 1e-4f) ? std::min(1.0f, transDiffS / (absS + 1e-4f)) : 0.0f;

        float sideTrans = side * transWeightS;
        float sideTonal = side - sideTrans;

        // 4. Independent Stereo Width Scaling
        float modifiedSideTrans = sideTrans * mTransientWidth;
        float modifiedSideTonal = sideTonal * mTonalWidth;

        // 5. Decode back to L/R for Transient stream and Tonal stream
        float transL = (midTrans - modifiedSideTrans) * mLinearTransientGain * transPanL;
        float transR = (midTrans + modifiedSideTrans) * mLinearTransientGain * transPanR;

        float tonalL = (midTonal - modifiedSideTonal) * mLinearTonalGain * tonalPanL;
        float tonalR = (midTonal + modifiedSideTonal) * mLinearTonalGain * tonalPanR;

        // 6. Recombine
        float finalL = transL + tonalL;
        float finalR = transR + tonalR;

        outL[i] = finalL;
        outR[i] = finalR;

        transEnergyTotal += (transL * transL + transR * transR);
        tonalEnergyTotal += (tonalL * tonalL + tonalR * tonalR);

        // Correlation tracking
        mCorrLR += 0.001f * ((finalL * finalR) - mCorrLR);
        mCorrLL += 0.001f * ((finalL * finalL) - mCorrLL);
        mCorrRR += 0.001f * ((finalR * finalR) - mCorrRR);
    }

    float totalE = transEnergyTotal + tonalEnergyTotal + 1e-6f;
    mTransientRatio = transEnergyTotal / totalE;

    float denom = std::sqrt(std::max(1e-8f, mCorrLL * mCorrRR));
    mPhaseCorrelation = std::max(-1.0f, std::min(1.0f, mCorrLR / denom));
}

} // namespace Jazz
