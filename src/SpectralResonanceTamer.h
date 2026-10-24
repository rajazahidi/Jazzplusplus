//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Smart Dynamic Spectral Resonance Tamer (Soothe-Style Harshness Suppressor)
//
// Transparent harshness suppressor that tracks and dynamically reduces
// sharp resonant frequency spikes across the mid and high frequencies (1kHz to 12kHz)
// without compromising the overall musical frequency balance.
//*****************************************************************************

#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

class JZSpectralResonanceTamer
{
public:
  JZSpectralResonanceTamer(float sampleRate = 44100.0f);
  ~JZSpectralResonanceTamer() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  // Processing Controls
  void SetDepth(float depthPercent);    // 0% (bypass) to 100% (aggressive reduction)
  float GetDepth() const { return mDepth; }

  void SetSharpnessQ(float q);          // 1.0 (broad) to 8.0 (pinpoint narrow notch)
  float GetSharpnessQ() const { return mSharpnessQ; }

  void SetSensitivity(float sens);      // 0.0 to 1.0 (detection threshold)
  float GetSensitivity() const { return mSensitivity; }

  void SetListenDelta(bool enable) { mListenDelta = enable; }
  bool IsListenDelta() const { return mListenDelta; }

  // Real-time stereo processing
  void Process(float& left, float& right);

  // Metering
  float GetCurrentAttenuationDb() const { return mCurrentAttenuationDb; }

private:
  float mSampleRate;
  float mDepth;
  float mSharpnessQ;
  float mSensitivity;
  bool  mListenDelta;

  // Filter Tracking States across 16 narrow resonant detection sub-bands
  struct JZResonantNotch
  {
    float centerHz;
    float envFollower;
    float dynamicGain;
    float state1L, state2L;
    float state1R, state2R;

    JZResonantNotch(float f = 2000.0f)
      : centerHz(f),
        envFollower(0.0f),
        dynamicGain(1.0f),
        state1L(0.0f), state2L(0.0f),
        state1R(0.0f), state2R(0.0f)
    {
    }
  };

  std::vector<JZResonantNotch> mNotches;
  float mCurrentAttenuationDb;

  void InitNotches();
};
