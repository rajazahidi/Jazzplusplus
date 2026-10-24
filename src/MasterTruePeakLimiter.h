//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Master True-Peak Brickwall Limiter
//
// Studio Master Limiter with 8x Polyphase Oversampling (ITU-R BS.1770 compliant),
// Lookahead Buffer (0.5ms - 5.0ms), True-Peak Inter-Sample Detection, and
// strict brickwall ceiling (-0.1 dBTP) for commercial streaming compliance.
//*****************************************************************************

#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

class JZMasterTruePeakLimiter
{
public:
  JZMasterTruePeakLimiter(float sampleRate = 44100.0f);
  ~JZMasterTruePeakLimiter() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  // Limiter Controls
  void SetThresholdDb(float thresholdDb);   // Input drive / threshold (e.g. -12.0 to 0.0 dB)
  float GetThresholdDb() const { return mThresholdDb; }

  void SetCeilingDbTP(float ceilingDb);    // Output ceiling (e.g. -0.1 dBTP)
  float GetCeilingDbTP() const { return mCeilingDbTP; }

  void SetReleaseMs(float releaseMs);       // Release time (10ms to 1000ms)
  float GetReleaseMs() const { return mReleaseMs; }

  void SetLookaheadMs(float lookaheadMs);   // Lookahead time (0.5ms to 5.0ms)
  float GetLookaheadMs() const { return mLookaheadMs; }

  void SetOversamplingEnabled(bool enable) { mOversamplingEnabled = enable; }
  bool IsOversamplingEnabled() const { return mOversamplingEnabled; }

  // Stereo Real-Time Processing
  void Process(float& left, float& right);

  // Metering
  float GetMaxTruePeakLinear() const { return mMaxTruePeakLinear; }
  float GetMaxTruePeakDbTP() const;
  float GetCurrentGainReductionDb() const;

  void ResetMeters();

private:
  float mSampleRate;
  float mThresholdDb;
  float mCeilingDbTP;
  float mReleaseMs;
  float mLookaheadMs;
  bool  mOversamplingEnabled;

  // Linear gains
  float mInputGain;
  float mCeilingLinear;
  float mCurrentGain;

  // Lookahead Ring Buffers
  std::vector<float> mDelayBufL;
  std::vector<float> mDelayBufR;
  size_t mDelayWriteIdx;
  size_t mDelaySamples;

  // Metering states
  float mMaxTruePeakLinear;

  void UpdateParameters();
  float DetectInterSampleTruePeak(float s0, float s1);
};
