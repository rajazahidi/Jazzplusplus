//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Stereo Mid-Side (M/S) Matrix & Mono Bass Maker
//
// Full bi-directional Mid-Side matrix processing with independent Mid/Side gain,
// stereo width expansion (0% to 200%), Solo Mid/Side audition, and Mono Bass Maker
// (high-pass filtering the Side channel to eliminate low-end phase issues).
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

// Configuration for Mid-Side Processor
struct JZMidSideConfig
{
  float midGainDB;            // -12.0 dB to +12.0 dB (e.g. 0.0 dB)
  float sideGainDB;           // -12.0 dB to +12.0 dB (e.g. 0.0 dB)
  float stereoWidthPercent;   // 0% (Mono) to 100% (Normal) to 200% (Super Wide)
  bool enableMonoBass;        // High-pass filter side channel to keep low-end centered
  float monoBassCutoffHz;     // 20.0 Hz to 300.0 Hz (default 120.0 Hz)
  bool soloMid;               // Audition Mid channel only
  bool soloSide;              // Audition Side channel only

  JZMidSideConfig()
    : midGainDB(0.0f),
      sideGainDB(0.0f),
      stereoWidthPercent(100.0f),
      enableMonoBass(true),
      monoBassCutoffHz(120.0f),
      soloMid(false),
      soloSide(false)
  {
  }
};

// Mid-Side Processor Class
class JZMidSideProcessor
{
public:
  JZMidSideProcessor(float sampleRate = 44100.0f);
  ~JZMidSideProcessor() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  void SetConfig(const JZMidSideConfig& config);
  const JZMidSideConfig& GetConfig() const { return mConfig; }
  JZMidSideConfig& GetConfig() { return mConfig; }

  void Reset();

  // Process Stereo Audio Block in place
  void Process(float* inOutL, float* inOutR, int numSamples);

  // Discrete matrix conversion helpers
  static void LeftRightToMidSide(float L, float R, float& outMid, float& outSide);
  static void MidSideToLeftRight(float Mid, float Side, float& outL, float& outR);

private:
  float mSampleRate;
  JZMidSideConfig mConfig;

  // 2nd-order Butterworth High-Pass filter for Mono Bass on Side channel
  float hp_b0, hp_b1, hp_b2, hp_a1, hp_a2;
  float mSideHpX1, mSideHpX2, mSideHpY1, mSideHpY2;

  void UpdateFilterCoefficients();
};
