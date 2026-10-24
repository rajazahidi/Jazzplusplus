//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Vintage Lo-Fi Cassette Degradation & Pitch Wobble Implementation
//*****************************************************************************

#include "VintageLofiProcessor.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

JZVintageLofiProcessor::JZVintageLofiProcessor(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f),
    mWowDepth(0.35f),
    mFlutterDepth(0.20f),
    mCassetteType(JZCassetteType::TypeIFerric),
    mHissLevelDb(-54.0f),
    mAzimuthLoss(0.40f),
    mDropoutsEnabled(true),
    mWowPhase(0.0f),
    mFlutterPhase(0.0f),
    mWriteIdx(0),
    mAzimuthLpL(0.0f),
    mAzimuthLpR(0.0f),
    mDropoutGain(1.0f)
{
  mDelayBufL.assign(2048, 0.0f);
  mDelayBufR.assign(2048, 0.0f);
}

void JZVintageLofiProcessor::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
  }
}

void JZVintageLofiProcessor::SetWowDepth(float depth)
{
  mWowDepth = std::clamp(depth, 0.0f, 1.0f);
}

void JZVintageLofiProcessor::SetFlutterDepth(float depth)
{
  mFlutterDepth = std::clamp(depth, 0.0f, 1.0f);
}

void JZVintageLofiProcessor::SetHissLevelDb(float hissDb)
{
  mHissLevelDb = std::clamp(hissDb, -96.0f, -24.0f);
}

void JZVintageLofiProcessor::SetAzimuthLoss(float loss)
{
  mAzimuthLoss = std::clamp(loss, 0.0f, 1.0f);
}

float JZVintageLofiProcessor::GenerateHissSample()
{
  if (mHissLevelDb <= -90.0f)
  {
    return 0.0f;
  }
  float noise = (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * 2.0f - 1.0f;
  float hissLinear = std::pow(10.0f, mHissLevelDb / 20.0f);
  return noise * hissLinear;
}

void JZVintageLofiProcessor::Process(float& left, float& right)
{
  // 1. Write incoming samples to ring delay buffer
  mDelayBufL[mWriteIdx] = left;
  mDelayBufR[mWriteIdx] = right;

  // 2. Advance LFO phases (Wow ~0.55 Hz, Flutter ~11.8 Hz)
  const float twoPi = 6.2831853f;
  mWowPhase += (0.55f * twoPi) / mSampleRate;
  if (mWowPhase >= twoPi) mWowPhase -= twoPi;

  mFlutterPhase += (11.8f * twoPi) / mSampleRate;
  if (mFlutterPhase >= twoPi) mFlutterPhase -= twoPi;

  // 3. Compute fractional pitch warble delay
  float baseDelay = 30.0f;
  float wowMod = mWowDepth * 18.0f * std::sin(mWowPhase);
  float flutterMod = mFlutterDepth * 4.5f * std::sin(mFlutterPhase);
  float totalDelay = baseDelay + wowMod + flutterMod;

  if (totalDelay < 1.0f) totalDelay = 1.0f;
  if (totalDelay > 1000.0f) totalDelay = 1000.0f;

  float readPos = static_cast<float>(mWriteIdx) - totalDelay;
  if (readPos < 0.0f) readPos += static_cast<float>(mDelayBufL.size());

  size_t idx0 = static_cast<size_t>(readPos) % mDelayBufL.size();
  size_t idx1 = (idx0 + 1) % mDelayBufL.size();
  float frac = readPos - static_cast<float>(static_cast<size_t>(readPos));

  // Linear interpolation reading
  float delayedL = (1.0f - frac) * mDelayBufL[idx0] + frac * mDelayBufL[idx1];
  float delayedR = (1.0f - frac) * mDelayBufR[idx0] + frac * mDelayBufR[idx1];

  mWriteIdx = (mWriteIdx + 1) % mDelayBufL.size();

  // 4. Azimuth high-frequency loss low-pass filter
  float azimuthCutoff = 13000.0f - (mAzimuthLoss * 8500.0f); // 4.5kHz to 13kHz
  float alpha = std::exp(-twoPi * azimuthCutoff / mSampleRate);
  mAzimuthLpL = alpha * mAzimuthLpL + (1.0f - alpha) * delayedL;
  mAzimuthLpR = alpha * mAzimuthLpR + (1.0f - alpha) * delayedR;

  // 5. Tape Dropouts Simulation
  if (mDropoutsEnabled)
  {
    if ((std::rand() % 40000) == 0) // Random oxide drop
    {
      mDropoutGain = 0.65f;
    }
    mDropoutGain = 0.9995f * mDropoutGain + 0.0005f * 1.0f; // Recover smoothly
  }
  else
  {
    mDropoutGain = 1.0f;
  }

  // 6. Analog Tape Hiss injection
  float hissL = GenerateHissSample();
  float hissR = GenerateHissSample();

  left = std::clamp(mAzimuthLpL * mDropoutGain + hissL, -1.0f, 1.0f);
  right = std::clamp(mAzimuthLpR * mDropoutGain + hissR, -1.0f, 1.0f);
}
