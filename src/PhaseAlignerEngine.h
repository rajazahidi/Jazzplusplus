//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Dual-Mic Dynamic Phase Aligner & Auto-Correlator
//
// Solves comb filtering and phase cancellation in multi-microphone tracking
// (e.g. Snare Top/Bottom, Kick In/Out, Dual-Mic Guitar Cabs, Stereo Overhead pairs)
// via cross-correlation delay detection, sub-sample micro-timing delay,
// continuous 0-360 degree all-pass phase rotation, and polarity flip.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

struct JZPhaseAlignReport
{
  float detectedDelayMs;      // Time offset between guide and target
  float initialCorrelation;   // -1.0 (inverted) to +1.0 (in phase)
  float alignedCorrelation;   // Correlation post-alignment (~ +0.95 to +1.0)
  float appliedDelayMs;
  float appliedPhaseDegrees;
  bool  polarityInverted;
  bool  success;

  JZPhaseAlignReport()
    : detectedDelayMs(0.0f),
      initialCorrelation(0.0f),
      alignedCorrelation(0.0f),
      appliedDelayMs(0.0f),
      appliedPhaseDegrees(0.0f),
      polarityInverted(false),
      success(false)
  {
  }
};

class JZPhaseAlignerEngine
{
public:
  JZPhaseAlignerEngine(float sampleRate = 44100.0f);
  ~JZPhaseAlignerEngine() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  // Manual Phase Controls
  void SetDelayMs(float delayMs);             // 0.0ms to 25.0ms
  float GetDelayMs() const { return mDelayMs; }

  void SetPhaseDegrees(float degrees);        // 0.0 to 360.0 degrees
  float GetPhaseDegrees() const { return mPhaseDegrees; }

  void SetInvertPolarity(bool invert) { mInvertPolarity = invert; }
  bool IsPolarityInverted() const { return mInvertPolarity; }

  // Automatic Cross-Correlation Alignment
  bool AutoAlign(
    const std::vector<float>& guideAudio,
    const std::vector<float>& targetAudio,
    JZPhaseAlignReport& outReport);

  // Real-Time Processing for Target Track
  float ProcessSample(float targetSample);
  void ProcessBuffer(const std::vector<float>& inTarget, std::vector<float>& outTarget);

private:
  float mSampleRate;
  float mDelayMs;
  float mPhaseDegrees;
  bool  mInvertPolarity;

  // Fractional Delay Line Ring Buffer
  std::vector<float> mDelayBuffer;
  size_t mWriteIdx;

  // All-pass phase rotator state
  float mAllPassState1;
  float mAllPassState2;

  float ComputeCorrelation(const std::vector<float>& a, const std::vector<float>& b, int lagSamples);
};
