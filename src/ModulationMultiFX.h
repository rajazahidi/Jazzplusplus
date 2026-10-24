//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Stereo Modulation Multi-FX Suite (Dimension Chorus, 12-Stage Phaser & Flanger)
//
// Vintage studio modulation multi-effects processor featuring:
// 1. Stereo Dimension Chorus (Roland Dimension D style zero-wobble stereo expander)
// 2. 12-Stage Optical Phaser (deep resonant all-pass phasing with stereo inversion)
// 3. Through-Zero Tape Flanger (tape-crossing zero-flange sweep)
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

// Modulation Multi-FX Mode
enum JZModulationMode
{
  MOD_DIMENSION_CHORUS = 0,   // Dimension D 4-line stereo chorus (lush zero-vibrato width)
  MOD_OPTICAL_PHASER_12,      // 12-Stage optical all-pass phaser
  MOD_THROUGH_ZERO_FLANGER    // Through-zero tape flanger with dramatic phase inversion
};

// Configuration for Modulation Multi-FX
struct JZModulationConfig
{
  JZModulationMode mode;
  float rateHz;               // LFO speed: 0.05 Hz to 8.0 Hz (default 0.45 Hz)
  float depth;                // Modulation intensity: 0.0 to 1.0 (default 0.7)
  float feedback;             // Resonance feedback: -0.90 to +0.90 (default 0.35)
  float stereoSpread;         // LFO phase difference between L and R (0.0 to 1.0, default 0.8)
  float dryWet;               // 0.0 to 1.0 (default 0.5)

  JZModulationConfig()
    : mode(MOD_DIMENSION_CHORUS),
      rateHz(0.45f),
      depth(0.70f),
      feedback(0.35f),
      stereoSpread(0.80f),
      dryWet(0.50f)
  {
  }
};

// 1-Pole All-Pass Filter Stage (for Phaser)
struct JZAllPassStage
{
  float x1, y1;
  JZAllPassStage() : x1(0), y1(0) {}
  void Reset() { x1 = y1 = 0.0f; }

  inline float Process(float in, float a)
  {
    float out = -a * in + x1 + a * y1;
    x1 = in;
    y1 = out;
    return out;
  }
};

// Modulation Multi-FX Class
class JZModulationMultiFX
{
public:
  JZModulationMultiFX(float sampleRate = 44100.0f);
  ~JZModulationMultiFX() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  void SetConfig(const JZModulationConfig& config) { mConfig = config; }
  const JZModulationConfig& GetConfig() const { return mConfig; }
  JZModulationConfig& GetConfig() { return mConfig; }

  void Reset();

  // Process Stereo Audio Block
  void Process(float* inOutL, float* inOutR, int numSamples);

  // Process Mono Audio Block
  void ProcessMono(float* inOut, int numSamples);

private:
  float mSampleRate;
  JZModulationConfig mConfig;

  // Chorus / Flanger delay buffers
  std::vector<float> mDelayL;
  std::vector<float> mDelayR;
  int mDelayWritePos;
  double mLfoPhase;

  // 12 All-Pass stages for Phaser
  JZAllPassStage mPhaserL[12];
  JZAllPassStage mPhaserR[12];
  float mPhaserFeedbackL;
  float mPhaserFeedbackR;
};
