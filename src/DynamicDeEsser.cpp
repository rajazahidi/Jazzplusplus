//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Dynamic Vocal De-Esser & Sibilance Suppressor Implementation
//*****************************************************************************

#include "DynamicDeEsser.h"

JZDynamicDeEsser::JZDynamicDeEsser(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f),
    b0(0.0f), b1(0.0f), b2(0.0f), a1(0.0f), a2(0.0f),
    mBpInL1(0.0f), mBpInL2(0.0f), mBpOutL1(0.0f), mBpOutL2(0.0f),
    mBpInR1(0.0f), mBpInR2(0.0f), mBpOutR1(0.0f), mBpOutR2(0.0f),
    mEnvelopeDB(-96.0f),
    mCurrentGainReductionDB(0.0f),
    mCurrentSibilanceDB(-96.0f),
    mAlphaAttack(0.0f),
    mAlphaRelease(0.0f)
{
  UpdateCoefficients();
}

void JZDynamicDeEsser::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
    UpdateCoefficients();
  }
}

void JZDynamicDeEsser::SetConfig(const JZDeEsserConfig& config)
{
  mConfig = config;
  UpdateCoefficients();
}

void JZDynamicDeEsser::Reset()
{
  mBpInL1 = mBpInL2 = mBpOutL1 = mBpOutL2 = 0.0f;
  mBpInR1 = mBpInR2 = mBpOutR1 = mBpOutR2 = 0.0f;
  mEnvelopeDB = -96.0f;
  mCurrentGainReductionDB = 0.0f;
  mCurrentSibilanceDB = -96.0f;
}

void JZDynamicDeEsser::UpdateCoefficients()
{
  // Attack / Release time constants
  float attSec = std::max(0.0001f, mConfig.attackMs * 0.001f);
  float relSec = std::max(0.001f, mConfig.releaseMs * 0.001f);
  mAlphaAttack = std::exp(-1.0f / (mSampleRate * attSec));
  mAlphaRelease = std::exp(-1.0f / (mSampleRate * relSec));

  // Biquad Bandpass Filter (constant skirt gain, peak gain = Q)
  float w0 = 2.0f * 3.14159265f * std::max(1000.0f, std::min(mSampleRate * 0.45f, mConfig.centerFreqHz)) / mSampleRate;
  float Q = std::max(0.1f, mConfig.bandwidthQ);
  float alpha = std::sin(w0) / (2.0f * Q);

  float a0 = 1.0f + alpha;
  b0 = (std::sin(w0) * 0.5f) / a0;
  b1 = 0.0f;
  b2 = (-std::sin(w0) * 0.5f) / a0;
  a1 = (-2.0f * std::cos(w0)) / a0;
  a2 = (1.0f - alpha) / a0;
}

float JZDynamicDeEsser::ProcessBandpass(float in, float& in1, float& in2, float& out1, float& out2)
{
  float out = b0 * in + b1 * in1 + b2 * in2 - a1 * out1 - a2 * out2;
  in2 = in1;
  in1 = in;
  out2 = out1;
  out1 = out;
  return out;
}

void JZDynamicDeEsser::Process(float* inOutL, float* inOutR, int numSamples)
{
  if (!inOutL || numSamples <= 0) return;

  float T = mConfig.thresholdDB;
  float R = std::max(1.0f, mConfig.ratio);

  for (int i = 0; i < numSamples; ++i)
  {
    float inL = inOutL[i];
    float inR = inOutR ? inOutR[i] : inL;

    // 1. Isolate sibilance band via biquad bandpass filter
    float sibilanceL = ProcessBandpass(inL, mBpInL1, mBpInL2, mBpOutL1, mBpOutL2);
    float sibilanceR = inOutR ? ProcessBandpass(inR, mBpInR1, mBpInR2, mBpOutR1, mBpOutR2) : sibilanceL;

    // 2. Measure level of sibilance
    float peakSib = std::max(std::fabs(sibilanceL), std::fabs(sibilanceR));
    float sibDB = (peakSib > 1e-5f) ? (20.0f * std::log10(peakSib)) : -96.0f;
    mCurrentSibilanceDB = sibDB;

    // 3. Compute target gain reduction in dB
    float targetGrDB = 0.0f;
    if (sibDB > T)
    {
      float overshoot = sibDB - T;
      targetGrDB = -(overshoot * (1.0f - (1.0f / R)));
    }

    // 4. Ballistics smoothing
    if (targetGrDB < mEnvelopeDB)
    {
      mEnvelopeDB = mAlphaAttack * mEnvelopeDB + (1.0f - mAlphaAttack) * targetGrDB;
    }
    else
    {
      mEnvelopeDB = mAlphaRelease * mEnvelopeDB + (1.0f - mAlphaRelease) * targetGrDB;
    }

    mCurrentGainReductionDB = mEnvelopeDB;
    float grLinear = std::pow(10.0f, mEnvelopeDB / 20.0f);

    // 5. Audition mode
    if (mConfig.listenMode)
    {
      inOutL[i] = sibilanceL;
      if (inOutR) inOutR[i] = sibilanceR;
      continue;
    }

    // 6. Apply gain reduction
    if (mConfig.splitBandMode)
    {
      // Split-band: attenuate only the sibilance band, leaving the rest untouched
      float remainderL = inL - sibilanceL;
      inOutL[i] = remainderL + (sibilanceL * grLinear);

      if (inOutR)
      {
        float remainderR = inR - sibilanceR;
        inOutR[i] = remainderR + (sibilanceR * grLinear);
      }
    }
    else
    {
      // Wideband: attenuate full signal
      inOutL[i] = inL * grLinear;
      if (inOutR) inOutR[i] = inR * grLinear;
    }
  }
}

void JZDynamicDeEsser::ProcessMono(float* inOut, int numSamples)
{
  Process(inOut, nullptr, numSamples);
}
