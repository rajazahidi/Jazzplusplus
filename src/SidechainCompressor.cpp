//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Studio Sidechain Compressor & Dynamic Ducking Engine Implementation
//*****************************************************************************

#include "SidechainCompressor.h"

JZSidechainCompressor::JZSidechainCompressor(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f),
    mEnvelopeDB(0.0f),
    mCurrentGainReductionDB(0.0f),
    mCurrentLinearGain(1.0f),
    mHpPrevInL(0.0f),
    mHpPrevOutL(0.0f),
    mHpPrevInR(0.0f),
    mHpPrevOutR(0.0f),
    mAlphaAttack(0.0f),
    mAlphaRelease(0.0f),
    mHpAlpha(0.0f)
{
  UpdateCoefficients();
}

void JZSidechainCompressor::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
    UpdateCoefficients();
  }
}

void JZSidechainCompressor::SetConfig(const JZSidechainConfig& config)
{
  mConfig = config;
  UpdateCoefficients();
}

void JZSidechainCompressor::Reset()
{
  mEnvelopeDB = 0.0f;
  mCurrentGainReductionDB = 0.0f;
  mCurrentLinearGain = 1.0f;
  mHpPrevInL = 0.0f;
  mHpPrevOutL = 0.0f;
  mHpPrevInR = 0.0f;
  mHpPrevOutR = 0.0f;
}

void JZSidechainCompressor::UpdateCoefficients()
{
  // Attack / Release time constants (ms to seconds)
  float attSec = std::max(0.0001f, mConfig.attackMs * 0.001f);
  float relSec = std::max(0.001f, mConfig.releaseMs * 0.001f);

  mAlphaAttack = std::exp(-1.0f / (mSampleRate * attSec));
  mAlphaRelease = std::exp(-1.0f / (mSampleRate * relSec));

  // High-Pass Filter cutoff for detector (1-pole high-pass)
  // alpha = 1 / (1 + 2 * pi * fc / fs)
  float w = 2.0f * 3.14159265358979323846f * std::max(10.0f, mConfig.sidechainHpFreq) / mSampleRate;
  mHpAlpha = 1.0f / (1.0f + w);
}

float JZSidechainCompressor::ProcessHpFilter(float in, float& prevIn, float& prevOut) const
{
  // 1-pole high-pass: y[n] = alpha * (y[n-1] + x[n] - x[n-1])
  float out = mHpAlpha * (prevOut + in - prevIn);
  prevIn = in;
  prevOut = out;
  return out;
}

float JZSidechainCompressor::ComputeStaticGainReduction(float inputDB) const
{
  float T = mConfig.thresholdDB;
  float R = std::max(1.0f, mConfig.ratio);
  float W = std::max(0.0f, mConfig.kneeDB);

  // Soft-knee characteristic curve
  if (W > 0.001f)
  {
    if (2.0f * (inputDB - T) < -W)
    {
      // Fully below knee
      return 0.0f;
    }
    else if (2.0f * std::fabs(inputDB - T) <= W)
    {
      // Within soft knee transition curve
      float diff = inputDB - T + (W * 0.5f);
      float yG = inputDB + ((1.0f / R - 1.0f) * diff * diff) / (2.0f * W);
      return yG - inputDB;
    }
    else
    {
      // Above knee
      float yG = T + (inputDB - T) / R;
      return yG - inputDB;
    }
  }
  else
  {
    // Hard knee
    if (inputDB > T)
    {
      float yG = T + (inputDB - T) / R;
      return yG - inputDB;
    }
    return 0.0f;
  }
}

void JZSidechainCompressor::Process(float* inOutL, float* inOutR, int numSamples,
                                   const float* sidechainInL,
                                   const float* sidechainInR)
{
  if (!inOutL || numSamples <= 0) return;

  float makeupLinear = std::pow(10.0f, mConfig.makeupGainDB / 20.0f);

  for (int i = 0; i < numSamples; ++i)
  {
    // 1. Determine detector input signal
    float detL = 0.0f;
    float detR = 0.0f;

    if (mConfig.useExternalSidechain && sidechainInL)
    {
      detL = sidechainInL[i];
      detR = sidechainInR ? sidechainInR[i] : sidechainInL[i];
    }
    else
    {
      detL = inOutL[i];
      detR = inOutR ? inOutR[i] : inOutL[i];
    }

    // 2. High-pass filter detector if enabled (removes low-end pumping)
    if (mConfig.enableSidechainFilter)
    {
      detL = ProcessHpFilter(detL, mHpPrevInL, mHpPrevOutL);
      if (inOutR)
      {
        detR = ProcessHpFilter(detR, mHpPrevInR, mHpPrevOutR);
      }
    }

    // 3. Peak level detection (maximum of stereo pair)
    float peak = std::max(std::fabs(detL), std::fabs(detR));
    float inputDB = 20.0f * std::log10(std::max(peak, 1.0e-6f));

    // 4. Calculate static target gain reduction in dB
    float targetGrDB = ComputeStaticGainReduction(inputDB);

    // 5. Ballistics (Attack / Release smoothing)
    if (targetGrDB < mEnvelopeDB)
    {
      // Compressing (Attack phase)
      mEnvelopeDB = mAlphaAttack * mEnvelopeDB + (1.0f - mAlphaAttack) * targetGrDB;
    }
    else
    {
      // Releasing
      mEnvelopeDB = mAlphaRelease * mEnvelopeDB + (1.0f - mAlphaRelease) * targetGrDB;
    }

    mCurrentGainReductionDB = mEnvelopeDB;

    // 6. Convert gain reduction to linear multiplier + makeup gain
    float grLinear = std::pow(10.0f, mEnvelopeDB / 20.0f);
    mCurrentLinearGain = grLinear * makeupLinear;

    // 7. Apply gain reduction
    inOutL[i] *= mCurrentLinearGain;
    if (inOutR)
    {
      inOutR[i] *= mCurrentLinearGain;
    }
  }
}

void JZSidechainCompressor::ProcessMono(float* inOut, int numSamples, const float* sidechainIn)
{
  Process(inOut, nullptr, numSamples, sidechainIn, nullptr);
}
