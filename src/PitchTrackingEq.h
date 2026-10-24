//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Pitch-Tracking Dynamic Musical EQ (SurferEQ-Style)
//
// Real-time pitch-following equalizer that tracks the incoming fundamental
// frequency (f0) of melodic instruments (bass, vocals, cello, guitar)
// and dynamically surfs the EQ filter center frequencies (Fundamental,
// 2nd Harmonic, 3rd Harmonic) locked to the musical notes being played.
//*****************************************************************************

#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

struct JZSurferBand
{
  float harmonicMultiplier;   // 1.0 = Fundamental, 2.0 = 2nd Harmonic, 3.0 = 3rd Harmonic
  float gainDb;               // -18.0 to +18.0 dB
  float q;                    // 0.5 to 10.0
  float currentCenterHz;
  float b0, b1, b2, a1, a2;   // Biquad coefficients
  float state1L, state2L;
  float state1R, state2R;

  JZSurferBand(float mult = 1.0f, float gain = 0.0f, float qVal = 2.0f)
    : harmonicMultiplier(mult),
      gainDb(gain),
      q(qVal),
      currentCenterHz(100.0f),
      b0(1.0f), b1(0.0f), b2(0.0f), a1(0.0f), a2(0.0f),
      state1L(0.0f), state2L(0.0f),
      state1R(0.0f), state2R(0.0f)
  {
  }
};

class JZPitchTrackingEq
{
public:
  JZPitchTrackingEq(float sampleRate = 44100.0f);
  ~JZPitchTrackingEq() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  // Pitch-Surfing Controls
  void SetTrackingEnabled(bool enable) { mTrackingEnabled = enable; }
  bool IsTrackingEnabled() const { return mTrackingEnabled; }

  void SetTrackingSpeedMs(float speedMs);      // Glissando smoothing time (5ms to 100ms)
  float GetTrackingSpeedMs() const { return mTrackingSpeedMs; }

  void SetBandGainDb(int bandIdx, float gainDb);
  float GetBandGainDb(int bandIdx) const;

  void SetBandQ(int bandIdx, float q);
  float GetBandQ(int bandIdx) const;

  // Real-time processing
  void Process(float& left, float& right);

  // Metering
  float GetDetectedFundamentalHz() const { return mSmoothedPitchHz; }

private:
  float mSampleRate;
  bool  mTrackingEnabled;
  float mTrackingSpeedMs;
  float mDetectedPitchHz;
  float mSmoothedPitchHz;

  std::vector<JZSurferBand> mBands;
  std::vector<float> mPitchDetectionBuf;
  size_t mPitchBufIdx;

  void UpdateBandCoefficients(JZSurferBand& band, float centerFreq);
  float EstimatePitch(const std::vector<float>& buf);
};
