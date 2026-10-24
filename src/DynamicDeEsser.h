//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Dynamic Vocal De-Esser & Sibilance Suppressor
//
// Frequency-selective sibilance compression engine (4kHz - 9kHz) with
// Split-Band / Wideband modes, audition listen switch, lookahead smoothing,
// and real-time sibilance reduction metering.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

// Configuration for Dynamic De-Esser
struct JZDeEsserConfig
{
  float centerFreqHz;         // Center frequency of sibilance detector (4000 to 9000 Hz, default 6500 Hz)
  float bandwidthQ;           // Filter Q factor (0.7 to 3.0, default 1.4)
  float thresholdDB;          // -40.0 dB to 0.0 dB (default -20.0 dB)
  float ratio;                // 2.0 to 12.0 (default 6.0:1)
  float attackMs;             // 0.5 ms to 20.0 ms (default 2.0 ms)
  float releaseMs;            // 10.0 ms to 200.0 ms (default 60.0 ms)
  bool splitBandMode;         // True: compress only sibilance band; False: compress full audio
  bool listenMode;            // Audition isolated sibilance detector signal

  JZDeEsserConfig()
    : centerFreqHz(6500.0f),
      bandwidthQ(1.4f),
      thresholdDB(-20.0f),
      ratio(6.0f),
      attackMs(2.0f),
      releaseMs(60.0f),
      splitBandMode(true),
      listenMode(false)
  {
  }
};

// Dynamic De-Esser Class
class JZDynamicDeEsser
{
public:
  JZDynamicDeEsser(float sampleRate = 44100.0f);
  ~JZDynamicDeEsser() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  void SetConfig(const JZDeEsserConfig& config);
  const JZDeEsserConfig& GetConfig() const { return mConfig; }
  JZDeEsserConfig& GetConfig() { return mConfig; }

  void Reset();

  // Process Stereo Audio
  void Process(float* inOutL, float* inOutR, int numSamples);

  // Process Mono Audio
  void ProcessMono(float* inOut, int numSamples);

  // Metering
  float GetGainReductionDB() const { return mCurrentGainReductionDB; }
  float GetSibilanceLevelDB() const { return mCurrentSibilanceDB; }

private:
  float mSampleRate;
  JZDeEsserConfig mConfig;

  // 2nd-order Biquad Bandpass Filter for sibilance detection
  float b0, b1, b2, a1, a2;
  float mBpInL1, mBpInL2, mBpOutL1, mBpOutL2;
  float mBpInR1, mBpInR2, mBpOutR1, mBpOutR2;

  // Smoothing states
  float mEnvelopeDB;
  float mCurrentGainReductionDB;
  float mCurrentSibilanceDB;

  float mAlphaAttack;
  float mAlphaRelease;

  void UpdateCoefficients();
  float ProcessBandpass(float in, float& in1, float& in2, float& out1, float& out2);
};
