//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Dynamic Auto-Wah & Vowel Formant Filter
//
// Classic dynamic envelope filter (Mu-Tron III / Boss AW-3 style) featuring
// sweep up/down modes, twin-peak vocal formant filtering ('Ooh-Aah'),
// adjustable sensitivity, fast attack/release ballistics, and screaming resonance Q.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

// Auto-Wah Sweep Mode
enum JZAutoWahMode
{
  AUTOWAH_SWEEP_UP = 0,       // Envelope opens filter upwards (Classic Funk Lead)
  AUTOWAH_SWEEP_DOWN,         // Envelope clamps filter downwards (Dub / Quack Bass)
  AUTOWAH_VOWEL_FORMANT       // Twin-peak vowel formant filter ('Ooh' -> 'Aah')
};

// Configuration for Dynamic Auto-Wah
struct JZAutoWahConfig
{
  JZAutoWahMode mode;
  float sensitivity;          // 0.0 to 1.0 (envelope sensitivity)
  float baseCutoffHz;         // 100 Hz to 1500 Hz (resting frequency)
  float sweepRangeHz;         // 200 Hz to 5000 Hz (modulation depth)
  float resonanceQ;           // 1.5 to 14.0 (filter peak sharpness)
  float attackMs;             // 1.0 ms to 30.0 ms (envelope follower attack)
  float releaseMs;            // 20.0 ms to 400.0 ms (envelope follower release)
  float dryWet;               // 0.0 to 1.0

  JZAutoWahConfig()
    : mode(AUTOWAH_SWEEP_UP),
      sensitivity(0.65f),
      baseCutoffHz(350.0f),
      sweepRangeHz(2800.0f),
      resonanceQ(6.5f),
      attackMs(3.0f),
      releaseMs(120.0f),
      dryWet(1.0f)
  {
  }
};

// State-Variable 2-Pole Resonant Filter
struct JZStateVariableFilter
{
  float low;
  float high;
  float band;

  JZStateVariableFilter() : low(0), high(0), band(0) {}
  void Reset() { low = high = band = 0.0f; }

  inline void Process(float in, float f, float q)
  {
    low += f * band;
    high = in - low - q * band;
    band += f * high;
  }
};

// Dynamic Auto-Wah Class
class JZDynamicAutoWah
{
public:
  JZDynamicAutoWah(float sampleRate = 44100.0f);
  ~JZDynamicAutoWah() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  void SetConfig(const JZAutoWahConfig& config);
  const JZAutoWahConfig& GetConfig() const { return mConfig; }
  JZAutoWahConfig& GetConfig() { return mConfig; }

  void Reset();

  // Process Stereo Audio Block
  void Process(float* inOutL, float* inOutR, int numSamples);

  // Process Mono Audio Block
  void ProcessMono(float* inOut, int numSamples);

  // Live telemetry
  float GetCurrentCutoffHz() const { return mCurrentCutoffHz; }
  float GetEnvelopeLevel() const { return mEnvelopeLevel; }

private:
  float mSampleRate;
  JZAutoWahConfig mConfig;

  float mEnvelopeLevel;
  float mAlphaAttack;
  float mAlphaRelease;
  float mCurrentCutoffHz;

  JZStateVariableFilter mFiltL1, mFiltL2;
  JZStateVariableFilter mFiltR1, mFiltR2;

  void UpdateCoefficients();
};
