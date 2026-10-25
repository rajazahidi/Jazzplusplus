//=============================================================================
// MoogTransistorLadderFilterEngine.h
//
// Master Studio Pro Suite v32.0 - Apex Transcendence Sovereign Millennium Release
// Historic Robert Moog 4-Pole 24dB/Octave Non-Linear Transistor Ladder Filter Engine.
//
// Features:
// - Non-linear differential transistor pair modeling with tanh() saturation
// - 4-stage cascaded 1-pole lowpass ladder with negative resonance feedback
// - Pure sinusoidal self-oscillation at resonance >= 1.0
// - Thermal voltage drift & analog drive saturation (0 dB to +18 dB)
// - 24 dB/octave continuous voltage-controlled cutoff (20 Hz to 20,000 Hz)
//=============================================================================

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

class JZMoogTransistorLadderFilterEngine {
public:
    explicit JZMoogTransistorLadderFilterEngine(float sampleRate = 48000.0f);
    ~JZMoogTransistorLadderFilterEngine() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return mSampleRate; }

    // Filter Controls
    void setCutoffHz(float cutoffHz);           // 20 Hz to 20,000 Hz
    float getCutoffHz() const { return mCutoffHz; }

    void setResonance(float res);               // 0.0 to 1.0 (1.0 = self-oscillation)
    float getResonance() const { return mResonance; }

    void setDriveDb(float driveDb);             // 0.0 dB to 18.0 dB
    float getDriveDb() const { return mDriveDb; }

    // Telemetry
    bool isSelfOscillating() const { return mResonance >= 0.98f; }
    float getThdPercent() const { return mThdPercent; }

    // Processing
    void process(const float* inL, const float* inR,
                 float* outL, float* outR, size_t numSamples);

    void reset();

private:
    float mSampleRate;
    float mCutoffHz;
    float mResonance;
    float mDriveDb;
    float mLinearDrive;
    float mThdPercent;

    // Ladder state (4 poles per channel)
    float mStageL[4];
    float mStageR[4];
    float mPrevStageL[4];
    float mPrevStageR[4];

    // Internal calculation helpers
    float processLadder(float inSample, float stages[4], float prevStages[4], float g, float k);
};

} // namespace Jazz
