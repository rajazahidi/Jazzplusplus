//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Stereo Mid-Side (M/S) Matrix & Mono Bass Maker Implementation
//*****************************************************************************

#include "MidSideProcessor.h"

JZMidSideProcessor::JZMidSideProcessor(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f),
    hp_b0(1), hp_b1(0), hp_b2(0), hp_a1(0), hp_a2(0),
    mSideHpX1(0), mSideHpX2(0), mSideHpY1(0), mSideHpY2(0)
{
  UpdateFilterCoefficients();
}

void JZMidSideProcessor::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
    UpdateFilterCoefficients();
  }
}

void JZMidSideProcessor::SetConfig(const JZMidSideConfig& config)
{
  bool cutoffChanged = std::fabs(mConfig.monoBassCutoffHz - config.monoBassCutoffHz) > 0.5f;
  mConfig = config;
  if (cutoffChanged)
  {
    UpdateFilterCoefficients();
  }
}

void JZMidSideProcessor::Reset()
{
  mSideHpX1 = mSideHpX2 = mSideHpY1 = mSideHpY2 = 0.0f;
}

void JZMidSideProcessor::UpdateFilterCoefficients()
{
  // 2nd-order Butterworth High-Pass for Side channel
  float f0 = std::max(10.0f, std::min(mSampleRate * 0.45f, mConfig.monoBassCutoffHz));
  float w0 = 3.14159265f * f0 / mSampleRate;
  float k = std::tan(w0);
  float k2 = k * k;
  float sqrt2 = 1.41421356f;
  float norm = 1.0f / (k2 + sqrt2 * k + 1.0f);

  hp_b0 = 1.0f * norm;
  hp_b1 = -2.0f * norm;
  hp_b2 = 1.0f * norm;
  hp_a1 = 2.0f * (k2 - 1.0f) * norm;
  hp_a2 = (k2 - sqrt2 * k + 1.0f) * norm;
}

void JZMidSideProcessor::LeftRightToMidSide(float L, float R, float& outMid, float& outSide)
{
  const float invSqrt2 = 0.70710678f;
  outMid = (L + R) * invSqrt2;
  outSide = (L - R) * invSqrt2;
}

void JZMidSideProcessor::MidSideToLeftRight(float Mid, float Side, float& outL, float& outR)
{
  const float invSqrt2 = 0.70710678f;
  outL = (Mid + Side) * invSqrt2;
  outR = (Mid - Side) * invSqrt2;
}

void JZMidSideProcessor::Process(float* inOutL, float* inOutR, int numSamples)
{
  if (!inOutL || !inOutR || numSamples <= 0) return;

  float midGainLin = std::pow(10.0f, mConfig.midGainDB / 20.0f);
  float sideGainLin = std::pow(10.0f, mConfig.sideGainDB / 20.0f);
  float widthScale = mConfig.stereoWidthPercent * 0.01f;

  for (int i = 0; i < numSamples; ++i)
  {
    float l = inOutL[i];
    float r = inOutR[i];

    // 1. Convert L/R to Mid-Side
    float mid = 0.0f, side = 0.0f;
    LeftRightToMidSide(l, r, mid, side);

    // 2. Apply Mono Bass high-pass filter on Side channel
    if (mConfig.enableMonoBass)
    {
      float filteredSide = hp_b0 * side + hp_b1 * mSideHpX1 + hp_b2 * mSideHpX2 - hp_a1 * mSideHpY1 - hp_a2 * mSideHpY2;
      mSideHpX2 = mSideHpX1;
      mSideHpX1 = side;
      mSideHpY2 = mSideHpY1;
      mSideHpY1 = filteredSide;
      side = filteredSide;
    }

    // 3. Apply Gain and Width
    mid *= midGainLin;
    side *= (sideGainLin * widthScale);

    // 4. Solo modes
    if (mConfig.soloMid)
    {
      side = 0.0f;
    }
    else if (mConfig.soloSide)
    {
      mid = 0.0f;
    }

    // 5. Convert Mid-Side back to L/R
    float outL = 0.0f, outR = 0.0f;
    MidSideToLeftRight(mid, side, outL, outR);

    inOutL[i] = outL;
    inOutR[i] = outR;
  }
}
