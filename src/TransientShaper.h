//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Studio Audio Transient Shaper & Punch Designer
//
// Threshold-independent dynamics processor providing independent Attack boost/cut
// (+-15dB) and Sustain tail resonance control (+-15dB) with dual fast/slow
// envelope followers and analog-style soft-clipping protection.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

// Configuration for Transient Shaper
struct JZTransientShaperConfig
{
  float attackGainDB;         // -15.0 dB to +15.0 dB (e.g. +4.0 dB for extra punch)
  float sustainGainDB;        // -15.0 dB to +15.0 dB (e.g. +2.0 dB for room ambience)
  float attackLengthMs;       // Fast envelope time window (1.0 ms to 40.0 ms, default 5.0 ms)
  float sustainReleaseMs;     // Slow envelope time window (20.0 ms to 500.0 ms, default 120.0 ms)
  bool clipOutput;            // Soft-saturation ceiling protection (tanh)

  JZTransientShaperConfig()
    : attackGainDB(0.0f),
      sustainGainDB(0.0f),
      attackLengthMs(5.0f),
      sustainReleaseMs(120.0f),
      clipOutput(true)
  {
  }
};

// Transient Shaper Class
class JZTransientShaper
{
public:
  JZTransientShaper(float sampleRate = 44100.0f);
  ~JZTransientShaper() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  void SetConfig(const JZTransientShaperConfig& config);
  const JZTransientShaperConfig& GetConfig() const { return mConfig; }
  JZTransientShaperConfig& GetConfig() { return mConfig; }

  void Reset();

  // Process Stereo Audio
  void Process(float* inOutL, float* inOutR, int numSamples);

  // Process Mono Audio
  void ProcessMono(float* inOut, int numSamples);

  // Real-time activity telemetry
  float GetTransientActivity() const { return mCurrentTransientActivity; }
  float GetSustainActivity() const { return mCurrentSustainActivity; }

private:
  float mSampleRate;
  JZTransientShaperConfig mConfig;

  // Envelope followers
  float mEnvFastL;
  float mEnvSlowL;
  float mEnvFastR;
  float mEnvSlowR;

  float mAlphaFastAtt;
  float mAlphaFastRel;
  float mAlphaSlowAtt;
  float mAlphaSlowRel;

  float mCurrentTransientActivity;
  float mCurrentSustainActivity;

  void UpdateCoefficients();
};
