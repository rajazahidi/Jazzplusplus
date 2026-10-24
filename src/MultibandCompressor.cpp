//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// 3-Band Studio Multiband Compressor & Dynamics Processor Implementation
//*****************************************************************************

#include "MultibandCompressor.h"

void JZLinkwitzRiley4::SetCrossover(float freqHz, float sampleRate)
{
  float w0 = 3.14159265f * std::max(20.0f, std::min(sampleRate * 0.48f, freqHz)) / sampleRate;
  float k = std::tan(w0);
  float k2 = k * k;
  float sqrt2 = 1.41421356f;

  // 2nd-order Butterworth Low-Pass (Q = 0.7071)
  float normLp = 1.0f / (k2 + sqrt2 * k + 1.0f);
  lp1.b0 = lp2.b0 = k2 * normLp;
  lp1.b1 = lp2.b1 = 2.0f * k2 * normLp;
  lp1.b2 = lp2.b2 = k2 * normLp;
  lp1.a1 = lp2.a1 = 2.0f * (k2 - 1.0f) * normLp;
  lp1.a2 = lp2.a2 = (k2 - sqrt2 * k + 1.0f) * normLp;

  // 2nd-order Butterworth High-Pass
  float normHp = 1.0f / (k2 + sqrt2 * k + 1.0f);
  hp1.b0 = hp2.b0 = 1.0f * normHp;
  hp1.b1 = hp2.b1 = -2.0f * normHp;
  hp1.b2 = hp2.b2 = 1.0f * normHp;
  hp1.a1 = hp2.a1 = 2.0f * (k2 - 1.0f) * normHp;
  hp1.a2 = hp2.a2 = (k2 - sqrt2 * k + 1.0f) * normHp;
}

JZMultibandCompressor::JZMultibandCompressor(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f)
{
  for (int b = 0; b < 3; ++b)
  {
    mEnvelopeDB[b] = 0.0f;
    mCurrentGrDB[b] = 0.0f;
  }
  UpdateCoefficients();
}

void JZMultibandCompressor::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
    UpdateCoefficients();
  }
}

void JZMultibandCompressor::SetConfig(const JZMultibandConfig& config)
{
  mConfig = config;
  UpdateCoefficients();
}

void JZMultibandCompressor::Reset()
{
  mXoverLowMidL.Reset();
  mXoverLowMidR.Reset();
  mXoverMidHighL.Reset();
  mXoverMidHighR.Reset();
  for (int b = 0; b < 3; ++b)
  {
    mEnvelopeDB[b] = 0.0f;
    mCurrentGrDB[b] = 0.0f;
  }
}

void JZMultibandCompressor::UpdateCoefficients()
{
  mXoverLowMidL.SetCrossover(mConfig.crossoverLowMidHz, mSampleRate);
  mXoverLowMidR.SetCrossover(mConfig.crossoverLowMidHz, mSampleRate);
  mXoverMidHighL.SetCrossover(mConfig.crossoverMidHighHz, mSampleRate);
  mXoverMidHighR.SetCrossover(mConfig.crossoverMidHighHz, mSampleRate);

  for (int b = 0; b < 3; ++b)
  {
    float attSec = std::max(0.0001f, mConfig.bands[b].attackMs * 0.001f);
    float relSec = std::max(0.001f, mConfig.bands[b].releaseMs * 0.001f);
    mAlphaAtt[b] = std::exp(-1.0f / (mSampleRate * attSec));
    mAlphaRel[b] = std::exp(-1.0f / (mSampleRate * relSec));
  }
}

float JZMultibandCompressor::GetGainReductionDB(int bandIndex) const
{
  if (bandIndex >= 0 && bandIndex < 3) return mCurrentGrDB[bandIndex];
  return 0.0f;
}

float JZMultibandCompressor::ProcessBandCompression(float inputLevelDB, int b)
{
  float T = mConfig.bands[b].thresholdDB;
  float R = std::max(1.0f, mConfig.bands[b].ratio);
  float targetGrDB = 0.0f;

  if (inputLevelDB > T)
  {
    targetGrDB = -(inputLevelDB - T) * (1.0f - (1.0f / R));
  }

  if (targetGrDB < mEnvelopeDB[b])
  {
    mEnvelopeDB[b] = mAlphaAtt[b] * mEnvelopeDB[b] + (1.0f - mAlphaAtt[b]) * targetGrDB;
  }
  else
  {
    mEnvelopeDB[b] = mAlphaRel[b] * mEnvelopeDB[b] + (1.0f - mAlphaRel[b]) * targetGrDB;
  }

  mCurrentGrDB[b] = mEnvelopeDB[b];
  float totalGainDB = mEnvelopeDB[b] + mConfig.bands[b].makeupGainDB;
  return std::pow(10.0f, totalGainDB / 20.0f);
}

void JZMultibandCompressor::Process(float* inOutL, float* inOutR, int numSamples)
{
  if (!inOutL || numSamples <= 0) return;

  bool anySolo = mConfig.bands[0].isSoloed || mConfig.bands[1].isSoloed || mConfig.bands[2].isSoloed;
  float masterGainLinear = std::pow(10.0f, mConfig.masterOutputGainDB / 20.0f);

  for (int i = 0; i < numSamples; ++i)
  {
    float dryL = inOutL[i];
    float dryR = inOutR ? inOutR[i] : dryL;

    // 1. Crossover Split: Low vs AboveLow (Mid + High)
    float lowL = mXoverLowMidL.lp2.Process(mXoverLowMidL.lp1.Process(dryL));
    float aboveLowL = mXoverLowMidL.hp2.Process(mXoverLowMidL.hp1.Process(dryL));

    float lowR = inOutR ? mXoverLowMidR.lp2.Process(mXoverLowMidR.lp1.Process(dryR)) : lowL;
    float aboveLowR = inOutR ? mXoverLowMidR.hp2.Process(mXoverLowMidR.hp1.Process(dryR)) : aboveLowL;

    // 2. Second Crossover Split on aboveLow: Mid vs High
    float midL = mXoverMidHighL.lp2.Process(mXoverMidHighL.lp1.Process(aboveLowL));
    float highL = mXoverMidHighL.hp2.Process(mXoverMidHighL.hp1.Process(aboveLowL));

    float midR = inOutR ? mXoverMidHighR.lp2.Process(mXoverMidHighR.lp1.Process(aboveLowR)) : midL;
    float highR = inOutR ? mXoverMidHighR.hp2.Process(mXoverMidHighR.hp1.Process(aboveLowR)) : highL;

    // 3. Peak detection & compression per band
    float peakLow = std::max(std::fabs(lowL), std::fabs(lowR));
    float peakMid = std::max(std::fabs(midL), std::fabs(midR));
    float peakHigh = std::max(std::fabs(highL), std::fabs(highR));

    float dbLow = (peakLow > 1e-5f) ? (20.0f * std::log10(peakLow)) : -96.0f;
    float dbMid = (peakMid > 1e-5f) ? (20.0f * std::log10(peakMid)) : -96.0f;
    float dbHigh = (peakHigh > 1e-5f) ? (20.0f * std::log10(peakHigh)) : -96.0f;

    float gainLow = ProcessBandCompression(dbLow, 0);
    float gainMid = ProcessBandCompression(dbMid, 1);
    float gainHigh = ProcessBandCompression(dbHigh, 2);

    // Apply solo / mute logic
    auto isBandAudible = [&](int b) {
      if (anySolo) return mConfig.bands[b].isSoloed;
      return !mConfig.bands[b].isMuted;
    };

    float compLowL = isBandAudible(0) ? (lowL * gainLow) : 0.0f;
    float compLowR = isBandAudible(0) ? (lowR * gainLow) : 0.0f;

    float compMidL = isBandAudible(1) ? (midL * gainMid) : 0.0f;
    float compMidR = isBandAudible(1) ? (midR * gainMid) : 0.0f;

    float compHighL = isBandAudible(2) ? (highL * gainHigh) : 0.0f;
    float compHighR = isBandAudible(2) ? (highR * gainHigh) : 0.0f;

    // 4. Sum bands back together
    float wetL = (compLowL + compMidL + compHighL) * masterGainLinear;
    float wetR = (compLowR + compMidR + compHighR) * masterGainLinear;

    // 5. Dry / Wet blend
    inOutL[i] = (dryL * (1.0f - mConfig.masterDryWet)) + (wetL * mConfig.masterDryWet);
    if (inOutR)
    {
      inOutR[i] = (dryR * (1.0f - mConfig.masterDryWet)) + (wetR * mConfig.masterDryWet);
    }
  }
}
