//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Studio Sidechain Compressor & Dynamic Ducking Engine
//
// Multi-channel compressor with external sidechain input routing,
// adjustable soft/hard knee, high-pass sidechain filter, attack/release ballistics,
// and real-time gain reduction metering.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

// Configuration for Sidechain Compressor
struct JZSidechainConfig
{
  float thresholdDB;          // -60.0 dB to 0.0 dB (e.g. -18.0 dB)
  float ratio;                // 1.0 to 30.0 (e.g. 4.0:1)
  float attackMs;             // 0.1 ms to 100.0 ms (e.g. 10.0 ms)
  float releaseMs;            // 10.0 ms to 1500.0 ms (e.g. 120.0 ms)
  float kneeDB;               // 0.0 dB (hard knee) to 20.0 dB (soft knee)
  float makeupGainDB;         // 0.0 dB to +24.0 dB
  float sidechainHpFreq;      // 20.0 Hz to 500.0 Hz (high-pass filter for detector)
  bool enableSidechainFilter; // Filter sub frequencies out of detection
  bool useExternalSidechain;  // When true, uses external sidechain aux bus

  JZSidechainConfig()
    : thresholdDB(-18.0f),
      ratio(4.0f),
      attackMs(10.0f),
      releaseMs(120.0f),
      kneeDB(4.0f),
      makeupGainDB(0.0f),
      sidechainHpFreq(80.0f),
      enableSidechainFilter(true),
      useExternalSidechain(false)
  {
  }
};

// Sidechain Compressor Class
class JZSidechainCompressor
{
public:
  JZSidechainCompressor(float sampleRate = 44100.0f);
  ~JZSidechainCompressor() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  void SetConfig(const JZSidechainConfig& config);
  const JZSidechainConfig& GetConfig() const { return mConfig; }
  JZSidechainConfig& GetConfig() { return mConfig; }

  void Reset();

  // Process Stereo Audio with optional External Sidechain inputs
  void Process(float* inOutL, float* inOutR, int numSamples,
               const float* sidechainInL = nullptr,
               const float* sidechainInR = nullptr);

  // Process Mono Audio with optional External Sidechain input
  void ProcessMono(float* inOut, int numSamples,
                   const float* sidechainIn = nullptr);

  // Metering
  float GetGainReductionDB() const { return mCurrentGainReductionDB; }
  float GetLinearGainReduction() const { return mCurrentLinearGain; }

private:
  float mSampleRate;
  JZSidechainConfig mConfig;

  // Smoothing states
  float mEnvelopeDB;
  float mCurrentGainReductionDB;
  float mCurrentLinearGain;

  // 1-Pole High-Pass Filter state for sidechain detector
  float mHpPrevInL;
  float mHpPrevOutL;
  float mHpPrevInR;
  float mHpPrevOutR;

  void UpdateCoefficients();
  float mAlphaAttack;
  float mAlphaRelease;
  float mHpAlpha;

  float ComputeStaticGainReduction(float inputDB) const;
  float ProcessHpFilter(float in, float& prevIn, float& prevOut) const;
};
