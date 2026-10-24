//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Studio Spectral Audio Denoiser & Noise Repair Engine
//
// Profile-based spectral subtraction denoiser with background noise fingerprinting,
// adjustable noise reduction (0 to 30dB), sensitivity threshold, spectral floor,
// and audition noise-only listen mode.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

// Configuration for Spectral Denoiser
struct JZSpectralDenoiseConfig
{
  float reductionDB;          // Noise reduction amount: 0.0 dB (bypass) to 30.0 dB (default 12.0 dB)
  float thresholdOffsetDB;    // Noise detection sensitivity offset: -12.0 dB to +12.0 dB
  float spectralFloor;        // Minimum floor to avoid musical noise artifacts (0.001 to 0.1, default 0.02)
  bool listenNoiseOnly;       // Audition mode: listen only to subtracted noise
  bool learnProfileMode;      // When true, updates noise fingerprint from incoming audio

  JZSpectralDenoiseConfig()
    : reductionDB(12.0f),
      thresholdOffsetDB(0.0f),
      spectralFloor(0.02f),
      listenNoiseOnly(false),
      learnProfileMode(false)
  {
  }
};

// Spectral Audio Denoiser Class
class JZSpectralDenoise
{
public:
  static const int NUM_BANDS = 64;

  JZSpectralDenoise(float sampleRate = 44100.0f);
  ~JZSpectralDenoise() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  void SetConfig(const JZSpectralDenoiseConfig& config);
  const JZSpectralDenoiseConfig& GetConfig() const { return mConfig; }
  JZSpectralDenoiseConfig& GetConfig() { return mConfig; }

  void Reset();

  // Noise fingerprint capture
  void StartLearningProfile();
  void StopLearningProfile();
  bool IsLearningProfile() const { return mConfig.learnProfileMode; }
  void ClearProfile();

  // Process Stereo Audio
  void Process(float* inOutL, float* inOutR, int numSamples);

  // Process Mono Audio
  void ProcessMono(float* inOut, int numSamples);

  // Real-time telemetry
  float GetNoiseLevelDB() const { return mCurrentNoiseLevelDB; }
  const std::vector<float>& GetNoiseProfile() const { return mNoiseProfile; }

private:
  float mSampleRate;
  JZSpectralDenoiseConfig mConfig;

  std::vector<float> mNoiseProfile;     // Captured noise energy per frequency band
  int mProfileFramesCount;
  float mCurrentNoiseLevelDB;

  // Smoothing filters per band
  std::vector<float> mBandGainL;
  std::vector<float> mBandGainR;

  void InitializeBands();
  void EstimateBandEnergies(const float* signal, int numSamples, std::vector<float>& bandEnergies) const;
};
