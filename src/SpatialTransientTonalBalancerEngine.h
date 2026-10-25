//=============================================================================
// SpatialTransientTonalBalancerEngine.h
//
// Master Studio Pro Suite v32.0 - Apex Transcendence Sovereign Millennium Release
// Spatial Mid/Side Transient-Tonal Polar Balancer & Stereo Image Sculptor.
//
// Features:
// - Dual-envelope differential transient vs tonal sustain decomposition
// - Independent Mid/Side stereo width for transients (0.0x - 2.0x) and tonal body (0.0x - 2.5x)
// - Independent spatial polar angle panning for transient attack and sustained decay
// - Transient centering lock preventing low-end and kick/snare punch dispersion
// - Broadcast mono-compatibility phase correlation guard
//=============================================================================

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

class JZSpatialTransientTonalBalancerEngine {
public:
    explicit JZSpatialTransientTonalBalancerEngine(float sampleRate = 48000.0f);
    ~JZSpatialTransientTonalBalancerEngine() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return mSampleRate; }

    // Transient Stream Controls
    void setTransientWidth(float width);         // 0.0x (mono) to 2.0x (wide)
    float getTransientWidth() const { return mTransientWidth; }

    void setTransientPan(float pan);             // -1.0 to +1.0
    float getTransientPan() const { return mTransientPan; }

    void setTransientGainDb(float db);           // -12.0 dB to +12.0 dB
    float getTransientGainDb() const { return mTransientGainDb; }

    // Tonal Body Controls
    void setTonalWidth(float width);             // 0.0x to 2.5x
    float getTonalWidth() const { return mTonalWidth; }

    void setTonalPan(float pan);                 // -1.0 to +1.0
    float getTonalPan() const { return mTonalPan; }

    void setTonalGainDb(float db);               // -12.0 dB to +12.0 dB
    float getTonalGainDb() const { return mTonalGainDb; }

    // Telemetry
    float getTransientRatio() const { return mTransientRatio; }
    float getPhaseCorrelation() const { return mPhaseCorrelation; }

    // Processing
    void process(const float* inL, const float* inR,
                 float* outL, float* outR, size_t numSamples);

    void reset();

private:
    float mSampleRate;
    float mTransientWidth;
    float mTransientPan;
    float mTransientGainDb;
    float mLinearTransientGain;

    float mTonalWidth;
    float mTonalPan;
    float mTonalGainDb;
    float mLinearTonalGain;

    // Telemetry
    float mTransientRatio;
    float mPhaseCorrelation;
    float mCorrLR;
    float mCorrLL;
    float mCorrRR;

    // Envelope followers for transient/tonal separation
    float mFastEnvM, mSlowEnvM;
    float mFastEnvS, mSlowEnvS;
};

} // namespace Jazz
