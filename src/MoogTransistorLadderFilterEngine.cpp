//=============================================================================
// MoogTransistorLadderFilterEngine.cpp
//
// Master Studio Pro Suite v32.0 - Apex Transcendence Sovereign Millennium Release
// Historic Robert Moog 4-Pole 24dB/Octave Non-Linear Transistor Ladder Filter Engine.
//=============================================================================

#include "MoogTransistorLadderFilterEngine.h"

namespace Jazz {

static const float kPi = 3.14159265358979323846f;

JZMoogTransistorLadderFilterEngine::JZMoogTransistorLadderFilterEngine(float sampleRate)
    : mSampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
    , mCutoffHz(1000.0f)
    , mResonance(0.35f)
    , mDriveDb(3.0f)
    , mLinearDrive(1.4125f)
    , mThdPercent(0.0f)
{
    for (int i = 0; i < 4; ++i) {
        mStageL[i] = mStageR[i] = 0.0f;
        mPrevStageL[i] = mPrevStageR[i] = 0.0f;
    }
    setDriveDb(mDriveDb);
}

void JZMoogTransistorLadderFilterEngine::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        mSampleRate = sampleRate;
        reset();
    }
}

void JZMoogTransistorLadderFilterEngine::setCutoffHz(float cutoffHz) {
    mCutoffHz = std::max(20.0f, std::min(mSampleRate * 0.48f, cutoffHz));
}

void JZMoogTransistorLadderFilterEngine::setResonance(float res) {
    mResonance = std::max(0.0f, std::min(1.0f, res));
}

void JZMoogTransistorLadderFilterEngine::setDriveDb(float driveDb) {
    mDriveDb = std::max(0.0f, std::min(24.0f, driveDb));
    mLinearDrive = std::pow(10.0f, mDriveDb / 20.0f);
}

void JZMoogTransistorLadderFilterEngine::reset() {
    for (int i = 0; i < 4; ++i) {
        mStageL[i] = mStageR[i] = 0.0f;
        mPrevStageL[i] = mPrevStageR[i] = 0.0f;
    }
    mThdPercent = 0.0f;
}

float JZMoogTransistorLadderFilterEngine::processLadder(float inSample, float stages[4],
                                                      float prevStages[4], float g, float k) {
    // 2x Oversampled trapezoidal integration with non-linear differential transistor saturation
    float input = inSample * mLinearDrive;

    for (int sub = 0; sub < 2; ++sub) {
        // Feedback from 4th stage with resonance gain k
        float feedback = k * stages[3];
        float driveInput = input - feedback;

        // Differential transistor pair clipping: V_t ~ 26mV
        float u = std::tanh(driveInput * 0.5f);

        // 4 Cascaded 1-pole integrator stages
        float stageIn = u;
        for (int p = 0; p < 4; ++p) {
            float tanhIn = std::tanh(stageIn);
            float tanhStage = std::tanh(stages[p]);
            // Trapezoidal integration
            stages[p] += g * (tanhIn - tanhStage);
            stageIn = stages[p];
        }
    }

    return stages[3];
}

void JZMoogTransistorLadderFilterEngine::process(const float* inL, const float* inR,
                                               float* outL, float* outR, size_t numSamples) {
    if (!inL || !inR || !outL || !outR || numSamples == 0) return;

    // Cutoff tuning coefficient
    float fc = mCutoffHz;
    float w0 = 2.0f * kPi * fc / (mSampleRate * 2.0f); // adjusted for 2x sub-step
    float g = 1.0f - std::exp(-w0);

    // Resonance gain: k in [0, 4.0]
    float k = mResonance * 3.98f;

    float distEnergy = 0.0f;
    float totalEnergy = 0.0f;

    for (size_t i = 0; i < numSamples; ++i) {
        float inSampleL = inL[i];
        float inSampleR = inR[i];

        float filteredL = processLadder(inSampleL, mStageL, mPrevStageL, g, k);
        float filteredR = processLadder(inSampleR, mStageR, mPrevStageR, g, k);

        outL[i] = filteredL;
        outR[i] = filteredR;

        float diff = std::abs(filteredL - inSampleL);
        distEnergy += diff * diff;
        totalEnergy += filteredL * filteredL + 1e-6f;
    }

    mThdPercent = std::min(100.0f, (std::sqrt(distEnergy / totalEnergy) * 100.0f));
}

} // namespace Jazz
