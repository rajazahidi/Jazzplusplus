//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Dynamic Auto-Wah & Vowel Formant Filter Implementation
//*****************************************************************************

#include "DynamicAutoWah.h"

JZDynamicAutoWah::JZDynamicAutoWah(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f),
    mEnvelopeLevel(0.0f),
    mAlphaAttack(0.0f),
    mAlphaRelease(0.0f),
    mCurrentCutoffHz(350.0f)
{
  UpdateCoefficients();
  Reset();
}

void JZDynamicAutoWah::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
    UpdateCoefficients();
    Reset();
  }
}

void JZDynamicAutoWah::SetConfig(const JZAutoWahConfig& config)
{
  mConfig = config;
  UpdateCoefficients();
}

void JZDynamicAutoWah::Reset()
{
  mEnvelopeLevel = 0.0f;
  mCurrentCutoffHz = mConfig.baseCutoffHz;
  mFiltL1.Reset(); mFiltL2.Reset();
  mFiltR1.Reset(); mFiltR2.Reset();
}

void JZDynamicAutoWah::UpdateCoefficients()
{
  float attSec = std::max(0.0005f, mConfig.attackMs * 0.001f);
  float relSec = std::max(0.005f, mConfig.releaseMs * 0.001f);
  mAlphaAttack = std::exp(-1.0f / (mSampleRate * attSec));
  mAlphaRelease = std::exp(-1.0f / (mSampleRate * relSec));
}

void JZDynamicAutoWah::Process(float* inOutL, float* inOutR, int numSamples)
{
  if (!inOutL || numSamples <= 0) return;

  float q = 1.0f / std::max(0.5f, mConfig.resonanceQ);

  for (int i = 0; i < numSamples; ++i)
  {
    float rawL = inOutL[i];
    float rawR = inOutR ? inOutR[i] : rawL;
    float peakIn = std::max(std::fabs(rawL), std::fabs(rawR));

    // 1. Envelope follower with sensitivity scaling
    float scaledInput = peakIn * mConfig.sensitivity * 2.5f;
    if (scaledInput > mEnvelopeLevel)
    {
      mEnvelopeLevel = mAlphaAttack * mEnvelopeLevel + (1.0f - mAlphaAttack) * scaledInput;
    }
    else
    {
      mEnvelopeLevel = mAlphaRelease * mEnvelopeLevel + (1.0f - mAlphaRelease) * scaledInput;
    }

    float envClamped = std::min(1.0f, mEnvelopeLevel);

    // 2. Modulate filter cutoff frequency
    float cutoff = mConfig.baseCutoffHz;
    if (mConfig.mode == AUTOWAH_SWEEP_UP)
    {
      cutoff += envClamped * mConfig.sweepRangeHz;
    }
    else if (mConfig.mode == AUTOWAH_SWEEP_DOWN)
    {
      cutoff = std::max(80.0f, cutoff + (1.0f - envClamped) * mConfig.sweepRangeHz);
    }
    else // AUTOWAH_VOWEL_FORMANT ('Ooh' 400Hz to 'Aah' 1200Hz)
    {
      cutoff = 400.0f + envClamped * 1000.0f;
    }

    cutoff = std::max(40.0f, std::min(mSampleRate * 0.45f, cutoff));
    mCurrentCutoffHz = cutoff;

    // State-variable filter tuning coefficient
    float f = 2.0f * std::sin(3.14159265f * cutoff / mSampleRate);

    // 3. Process Left channel through resonant bandpass / lowpass wah filter
    mFiltL1.Process(rawL, f, q);
    float wahOutL = (mConfig.mode == AUTOWAH_VOWEL_FORMANT) ? (mFiltL1.band * 2.2f) : (mFiltL1.low * 0.5f + mFiltL1.band * 1.5f);

    float wahOutR = wahOutL;
    if (inOutR)
    {
      mFiltR1.Process(rawR, f, q);
      wahOutR = (mConfig.mode == AUTOWAH_VOWEL_FORMANT) ? (mFiltR1.band * 2.2f) : (mFiltR1.low * 0.5f + mFiltR1.band * 1.5f);
    }

    // 4. Dry / Wet mix
    inOutL[i] = (rawL * (1.0f - mConfig.dryWet)) + (wahOutL * mConfig.dryWet);
    if (inOutR)
    {
      inOutR[i] = (rawR * (1.0f - mConfig.dryWet)) + (wahOutR * mConfig.dryWet);
    }
  }
}

void JZDynamicAutoWah::ProcessMono(float* inOut, int numSamples)
{
  Process(inOut, nullptr, numSamples);
}
