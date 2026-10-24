//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Analog Tape Machine & Vintage Console Warmer
//
// Authentic analog saturation modeler: Magnetic hysteresis tape saturation,
// tube/transformer odd and even harmonic distortion, 15/30 IPS tape speed selection,
// low-end head bump resonance, and subtle wow & flutter pitch modulation.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

// Tape Speed
enum JZTapeSpeedIPS
{
  TAPE_SPEED_15_IPS = 0,      // Warm, punchy low-end head bump (60-80Hz), gentle HF roll-off
  TAPE_SPEED_30_IPS           // Extended flat high-frequency response, ultra-low distortion
};

// Configuration for Analog Tape Warmer
struct JZTapeConsoleConfig
{
  JZTapeSpeedIPS tapeSpeed;
  float driveDB;              // 0.0 dB (clean) to +18.0 dB (heavy saturation)
  float biasTrimDB;           // -3.0 dB to +3.0 dB (high frequency tilt)
  float tubeHarmonicsEven;    // 0.0 to 1.0 (2nd harmonic warmth)
  float tapeHarmonicsOdd;     // 0.0 to 1.0 (3rd harmonic punch)
  float wowFlutterDepth;      // 0.0 (off) to 1.0 (vintage motor fluctuation)
  float dryWet;               // 0.0 to 1.0

  JZTapeConsoleConfig()
    : tapeSpeed(TAPE_SPEED_15_IPS),
      driveDB(4.5f),
      biasTrimDB(0.0f),
      tubeHarmonicsEven(0.25f),
      tapeHarmonicsOdd(0.35f),
      wowFlutterDepth(0.15f),
      dryWet(1.0f)
  {
  }
};

// Tape Console Warmer Class
class JZTapeConsoleWarmer
{
public:
  JZTapeConsoleWarmer(float sampleRate = 44100.0f);
  ~JZTapeConsoleWarmer() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  void SetConfig(const JZTapeConsoleConfig& config);
  const JZTapeConsoleConfig& GetConfig() const { return mConfig; }
  JZTapeConsoleConfig& GetConfig() { return mConfig; }

  void Reset();

  // Process Stereo Audio Block
  void Process(float* inOutL, float* inOutR, int numSamples);

  // Process Mono Audio Block
  void ProcessMono(float* inOut, int numSamples);

private:
  float mSampleRate;
  JZTapeConsoleConfig mConfig;

  // Wow & Flutter delay line & LFO
  std::vector<float> mDelayL;
  std::vector<float> mDelayR;
  int mDelayWritePos;
  float mLfoPhase;

  // Low-end head bump 2nd-order peak filter
  float hb_b0, hb_b1, hb_b2, hb_a1, hb_a2;
  float mHbX1L, mHbX2L, mHbY1L, mHbY2L;
  float mHbX1R, mHbX2R, mHbY1R, mHbY2R;

  void UpdateFilterCoefficients();
  float SaturateSample(float x, float driveLin) const;
};
