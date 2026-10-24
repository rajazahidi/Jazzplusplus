//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// 3-Band Studio Multiband Compressor & Dynamics Processor
//
// Phase-linear 4th-order Linkwitz-Riley crossover splitting (Low, Mid, High),
// independent per-band threshold, ratio, attack/release ballistics, makeup gain,
// solo/mute audition switches, and real-time per-band gain reduction metering.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

// Per-band Compressor Configuration
struct JZBandDynamicsConfig
{
  float thresholdDB;          // -50.0 dB to 0.0 dB
  float ratio;                // 1.0 to 20.0
  float attackMs;             // 0.5 ms to 100.0 ms
  float releaseMs;            // 10.0 ms to 1000.0 ms
  float makeupGainDB;         // -12.0 dB to +18.0 dB
  bool isMuted;
  bool isSoloed;

  JZBandDynamicsConfig(float thresh = -18.0f, float rat = 3.0f, float att = 15.0f, float rel = 100.0f)
    : thresholdDB(thresh),
      ratio(rat),
      attackMs(att),
      releaseMs(rel),
      makeupGainDB(0.0f),
      isMuted(false),
      isSoloed(false)
  {
  }
};

// Full Multiband Processor Configuration
struct JZMultibandConfig
{
  float crossoverLowMidHz;    // Default 140 Hz (Sub/Bass vs Mid)
  float crossoverMidHighHz;   // Default 2500 Hz (Mid vs High/Air)
  JZBandDynamicsConfig bands[3]; // 0: Low, 1: Mid, 2: High
  float masterDryWet;         // 0.0 (Dry) to 1.0 (100% Wet)
  float masterOutputGainDB;   // Master trim

  JZMultibandConfig()
    : crossoverLowMidHz(140.0f),
      crossoverMidHighHz(2500.0f),
      masterDryWet(1.0f),
      masterOutputGainDB(0.0f)
  {
    bands[0] = JZBandDynamicsConfig(-16.0f, 4.0f, 25.0f, 150.0f); // Low: tighter bass
    bands[1] = JZBandDynamicsConfig(-20.0f, 2.5f, 15.0f, 80.0f);  // Mid: musical control
    bands[2] = JZBandDynamicsConfig(-22.0f, 2.0f, 5.0f, 60.0f);   // High: transparent air
  }
};

// Biquad 2nd-order filter section
struct JZBiquad
{
  float b0, b1, b2, a1, a2;
  float x1, x2, y1, y2;

  JZBiquad() : b0(1), b1(0), b2(0), a1(0), a2(0), x1(0), x2(0), y1(0), y2(0) {}
  void Reset() { x1 = x2 = y1 = y2 = 0.0f; }

  inline float Process(float in)
  {
    float out = b0 * in + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1; x1 = in;
    y2 = y1; y1 = out;
    return out;
  }
};

// 4th-order Linkwitz-Riley Crossover (2 cascaded 2nd-order Butterworth filters)
struct JZLinkwitzRiley4
{
  JZBiquad lp1, lp2;
  JZBiquad hp1, hp2;

  void Reset()
  {
    lp1.Reset(); lp2.Reset();
    hp1.Reset(); hp2.Reset();
  }

  void SetCrossover(float freqHz, float sampleRate);
};

// Multiband Compressor Class
class JZMultibandCompressor
{
public:
  JZMultibandCompressor(float sampleRate = 44100.0f);
  ~JZMultibandCompressor() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  void SetConfig(const JZMultibandConfig& config);
  const JZMultibandConfig& GetConfig() const { return mConfig; }
  JZMultibandConfig& GetConfig() { return mConfig; }

  void Reset();

  // Process Stereo Audio
  void Process(float* inOutL, float* inOutR, int numSamples);

  // Metering
  float GetGainReductionDB(int bandIndex) const;

private:
  float mSampleRate;
  JZMultibandConfig mConfig;

  // Crossover filters for Left and Right channels
  JZLinkwitzRiley4 mXoverLowMidL, mXoverLowMidR;
  JZLinkwitzRiley4 mXoverMidHighL, mXoverMidHighR;

  // Smoothing states per band
  float mEnvelopeDB[3];
  float mCurrentGrDB[3];
  float mAlphaAtt[3];
  float mAlphaRel[3];

  void UpdateCoefficients();
  float ProcessBandCompression(float inputLevelDB, int band);
};
