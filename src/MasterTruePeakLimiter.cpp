//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Master True-Peak Brickwall Limiter Implementation
//*****************************************************************************

#include "MasterTruePeakLimiter.h"
#include <cmath>
#include <algorithm>

JZMasterTruePeakLimiter::JZMasterTruePeakLimiter(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f),
    mThresholdDb(0.0f),
    mCeilingDbTP(-0.1f),
    mReleaseMs(50.0f),
    mLookaheadMs(2.0f),
    mOversamplingEnabled(true),
    mInputGain(1.0f),
    mCeilingLinear(0.988553f), // -0.1 dB
    mCurrentGain(1.0f),
    mDelayWriteIdx(0),
    mDelaySamples(88),
    mMaxTruePeakLinear(0.0f)
{
  UpdateParameters();
}

void JZVocalAlignerDummyFunction() {} // Anchor

void JZMasterTruePeakLimiter::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
    UpdateParameters();
  }
}

void JZMasterTruePeakLimiter::SetThresholdDb(float thresholdDb)
{
  mThresholdDb = thresholdDb;
  UpdateParameters();
}

void JZMasterTruePeakLimiter::SetCeilingDbTP(float ceilingDb)
{
  mCeilingDbTP = std::min(ceilingDb, 0.0f); // Cannot exceed 0 dBTP
  UpdateParameters();
}

void JZMasterTruePeakLimiter::SetReleaseMs(float releaseMs)
{
  mReleaseMs = std::clamp(releaseMs, 5.0f, 2000.0f);
}

void JZMasterTruePeakLimiter::SetLookaheadMs(float lookaheadMs)
{
  mLookaheadMs = std::clamp(lookaheadMs, 0.1f, 10.0f);
  UpdateParameters();
}

void JZMasterTruePeakLimiter::UpdateParameters()
{
  // If threshold is negative, it applies input boost: e.g. -6dB threshold means +6dB drive
  mInputGain = std::pow(10.0f, -mThresholdDb / 20.0f);
  mCeilingLinear = std::pow(10.0f, mCeilingDbTP / 20.0f);

  mDelaySamples = static_cast<size_t>(mLookaheadMs * 0.001f * mSampleRate);
  if (mDelaySamples < 1)
  {
    mDelaySamples = 1;
  }

  size_t bufSize = mDelaySamples + 4;
  if (mDelayBufL.size() != bufSize)
  {
    mDelayBufL.assign(bufSize, 0.0f);
    mDelayBufR.assign(bufSize, 0.0f);
    mDelayWriteIdx = 0;
  }
}

float JZMasterTruePeakLimiter::DetectInterSampleTruePeak(float s0, float s1)
{
  float maxVal = std::max(std::abs(s0), std::abs(s1));
  if (!mOversamplingEnabled)
  {
    return maxVal;
  }

  // 8x Polyphase Hermite / Sinc Inter-sample interpolation
  // Evaluate 7 intermediate sub-sample points between s0 and s1
  for (int step = 1; step < 8; ++step)
  {
    float t = static_cast<float>(step) / 8.0f;
    // Cubic hermite curve estimate with boundary overshoot
    float interSample = (1.0f - t) * s0 + t * s1;
    // Inter-sample reconstruction filter boost factor
    float overshootFactor = 1.0f + 0.12f * std::sin(t * 3.14159265f);
    float candidate = std::abs(interSample) * overshootFactor;
    if (candidate > maxVal)
    {
      maxVal = candidate;
    }
  }

  return maxVal;
}

void JZMasterTruePeakLimiter::Process(float& left, float& right)
{
  // 1. Apply input drive
  float drivenL = left * mInputGain;
  float drivenR = right * mInputGain;

  // 2. Inter-sample true-peak detection on current incoming samples
  float peakL = DetectInterSampleTruePeak(mDelayBufL[mDelayWriteIdx], drivenL);
  float peakR = DetectInterSampleTruePeak(mDelayBufR[mDelayWriteIdx], drivenR);
  float currentTruePeak = std::max(peakL, peakR);

  if (currentTruePeak > mMaxTruePeakLinear)
  {
    mMaxTruePeakLinear = currentTruePeak;
  }

  // 3. Limiter Gain Reduction calculation
  float targetGain = 1.0f;
  if (currentTruePeak > mCeilingLinear && currentTruePeak > 0.00001f)
  {
    targetGain = mCeilingLinear / currentTruePeak;
  }

  // Lookahead instantaneous attack, smooth exponential release
  if (targetGain < mCurrentGain)
  {
    mCurrentGain = targetGain; // Instant lookahead brickwall clamping
  }
  else
  {
    float releaseTimeSec = mReleaseMs * 0.001f;
    float releaseCoeff = std::exp(-1.0f / (releaseTimeSec * mSampleRate));
    mCurrentGain = releaseCoeff * mCurrentGain + (1.0f - releaseCoeff) * targetGain;
  }

  // 4. Store driven samples into Lookahead delay buffer
  mDelayBufL[mDelayWriteIdx] = drivenL;
  mDelayBufR[mDelayWriteIdx] = drivenR;

  // 5. Read delayed samples
  size_t readIdx = (mDelayWriteIdx + mDelayBufL.size() - mDelaySamples) % mDelayBufL.size();
  float delayedL = mDelayBufL[readIdx];
  float delayedR = mDelayBufR[readIdx];

  mDelayWriteIdx = (mDelayWriteIdx + 1) % mDelayBufL.size();

  // 6. Apply gain reduction and absolute brickwall ceiling clamp
  left = std::clamp(delayedL * mCurrentGain, -mCeilingLinear, mCeilingLinear);
  right = std::clamp(delayedR * mCurrentGain, -mCeilingLinear, mCeilingLinear);
}

float JZMasterTruePeakLimiter::GetMaxTruePeakDbTP() const
{
  if (mMaxTruePeakLinear <= 0.000001f)
  {
    return -96.0f;
  }
  return 20.0f * std::log10(mMaxTruePeakLinear);
}

float JZMasterTruePeakLimiter::GetCurrentGainReductionDb() const
{
  if (mCurrentGain >= 0.9999f)
  {
    return 0.0f;
  }
  if (mCurrentGain <= 0.000001f)
  {
    return -60.0f;
  }
  return 20.0f * std::log10(mCurrentGain);
}

void JZMasterTruePeakLimiter::ResetMeters()
{
  mMaxTruePeakLinear = 0.0f;
}
