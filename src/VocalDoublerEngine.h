//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Vocal Stereo Doubler & Micro-Detune Ensemble
//
// Generates a rich, dimensional stereo vocal double from a single mono vocal track
// via 4 independent micro-detuned voices, human timing jitter, wide stereo pan matrix,
// and mono-compatibility phase protection.
//*****************************************************************************

#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

struct JZDoublerVoice
{
  float pan;                  // -1.0 (Left) to +1.0 (Right)
  float detuneCents;          // Micro-pitch detune
  float delayMs;              // Base timing offset
  float currentPhase;         // Pitch resynthesis phase
  float jitterOffset;         // Random human timing fluctuation
};

class JZVocalDoublerEngine
{
public:
  JZVocalDoublerEngine(float sampleRate = 44100.0f);
  ~JZVocalDoublerEngine() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  // Doubler Controls
  void SetStereoWidth(float widthPercent);    // 0% (mono) to 200% (super-wide)
  float GetStereoWidth() const { return mStereoWidth; }

  void SetDetuneAmountCents(float cents);     // 2 to 30 cents
  float GetDetuneAmountCents() const { return mDetuneAmountCents; }

  void SetTimingJitterMs(float jitterMs);     // 0 to 15 ms
  float GetTimingJitterMs() const { return mTimingJitterMs; }

  void SetDryWetMix(float mixPercent);        // 0% to 100%
  float GetDryWetMix() const { return mDryWetMix; }

  void SetMonoPhaseGuard(bool enable) { mMonoPhaseGuard = enable; }
  bool IsMonoPhaseGuard() const { return mMonoPhaseGuard; }

  // Real-Time Processing
  void Process(float inMono, float& outLeft, float& outRight);

private:
  float mSampleRate;
  float mStereoWidth;
  float mDetuneAmountCents;
  float mTimingJitterMs;
  float mDryWetMix;
  bool  mMonoPhaseGuard;

  std::vector<JZDoublerVoice> mVoices;
  std::vector<float> mDelayRingBuffer;
  size_t mWriteIdx;

  void InitVoices();
};
