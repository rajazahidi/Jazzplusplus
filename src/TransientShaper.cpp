//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Studio Audio Transient Shaper & Punch Designer Implementation
//*****************************************************************************

#include "TransientShaper.h"

JZTransientShaper::JZTransientShaper(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f),
    mEnvFastL(0.0f), mEnvSlowL(0.0f),
    mEnvFastR(0.0f), mEnvSlowR(0.0f),
    mCurrentTransientActivity(0.0f),
    mCurrentSustainActivity(0.0f)
{
  UpdateCoefficients();
}

void JZTransientShaper::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
    UpdateCoefficients();
  }
}

void JZTransientShaper::SetConfig(const JZTransientShaperConfig& config)
{
  mConfig = config;
  UpdateCoefficients();
}

void JZTransientShaper::Reset()
{
  mEnvFastL = mEnvSlowL = 0.0f;
  mEnvFastR = mEnvSlowR = 0.0f;
  mCurrentTransientActivity = 0.0f;
  mCurrentSustainActivity = 0.0f;
}

void JZTransientShaper::UpdateCoefficients()
{
  // Fast envelope (attack tracking)
  float fastAttSec = 0.0005f; // 0.5 ms instant attack
  float fastRelSec = std::max(0.001f, mConfig.attackLengthMs * 0.001f);
  mAlphaFastAtt = std::exp(-1.0f / (mSampleRate * fastAttSec));
  mAlphaFastRel = std::exp(-1.0f / (mSampleRate * fastRelSec));

  // Slow envelope (sustain tracking)
  float slowAttSec = std::max(0.005f, mConfig.attackLengthMs * 0.003f);
  float slowRelSec = std::max(0.01f, mConfig.sustainReleaseMs * 0.001f);
  mAlphaSlowAtt = std::exp(-1.0f / (mSampleRate * slowAttSec));
  mAlphaSlowRel = std::exp(-1.0f / (mSampleRate * slowRelSec));
}

void JZTransientShaper::Process(float* inOutL, float* inOutR, int numSamples)
{
  if (!inOutL || numSamples <= 0) return;

  float attTargetLin = std::pow(10.0f, mConfig.attackGainDB / 20.0f);
  float susTargetLin = std::pow(10.0f, mConfig.sustainGainDB / 20.0f);

  for (int i = 0; i < numSamples; ++i)
  {
    float absL = std::fabs(inOutL[i]);
    float absR = inOutR ? std::fabs(inOutR[i]) : absL;

    // 1. Update Fast Envelope
    if (absL > mEnvFastL) mEnvFastL = mAlphaFastAtt * mEnvFastL + (1.0f - mAlphaFastAtt) * absL;
    else                  mEnvFastL = mAlphaFastRel * mEnvFastL + (1.0f - mAlphaFastRel) * absL;

    if (absR > mEnvFastR) mEnvFastR = mAlphaFastAtt * mEnvFastR + (1.0f - mAlphaFastAtt) * absR;
    else                  mEnvFastR = mAlphaFastRel * mEnvFastR + (1.0f - mAlphaFastRel) * absR;

    // 2. Update Slow Envelope
    if (absL > mEnvSlowL) mEnvSlowL = mAlphaSlowAtt * mEnvSlowL + (1.0f - mAlphaSlowAtt) * absL;
    else                  mEnvSlowL = mAlphaSlowRel * mEnvSlowL + (1.0f - mAlphaSlowRel) * absL;

    if (absR > mEnvSlowR) mEnvSlowR = mAlphaSlowAtt * mEnvSlowR + (1.0f - mAlphaSlowAtt) * absR;
    else                  mEnvSlowR = mAlphaSlowRel * mEnvSlowR + (1.0f - mAlphaSlowRel) * absR;

    // 3. Transient & Sustain decomposition for Left
    float transL = std::max(0.0f, mEnvFastL - mEnvSlowL);
    float denomL = mEnvFastL + 1.0e-5f;
    float transRatioL = transL / denomL;
    float susRatioL = std::max(0.0f, 1.0f - transRatioL);

    float gainL = (1.0f + (attTargetLin - 1.0f) * transRatioL) *
                  (1.0f + (susTargetLin - 1.0f) * susRatioL);

    float outL = inOutL[i] * gainL;

    // 4. Decomposition for Right
    float outR = outL;
    if (inOutR)
    {
      float transR = std::max(0.0f, mEnvFastR - mEnvSlowR);
      float denomR = mEnvFastR + 1.0e-5f;
      float transRatioR = transR / denomR;
      float susRatioR = std::max(0.0f, 1.0f - transRatioR);

      float gainR = (1.0f + (attTargetLin - 1.0f) * transRatioR) *
                    (1.0f + (susTargetLin - 1.0f) * susRatioR);
      outR = inOutR[i] * gainR;
    }

    // 5. Soft-clipping protection
    if (mConfig.clipOutput)
    {
      outL = std::tanh(outL);
      if (inOutR) outR = std::tanh(outR);
    }

    inOutL[i] = outL;
    if (inOutR) inOutR[i] = outR;

    mCurrentTransientActivity = transL;
    mCurrentSustainActivity = mEnvSlowL;
  }
}

void JZTransientShaper::ProcessMono(float* inOut, int numSamples)
{
  Process(inOut, nullptr, numSamples);
}
