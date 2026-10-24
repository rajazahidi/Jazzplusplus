//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Master Reference Track Match EQ & Spectrum Profiler
//
// Analyzes long-term 64-band spectral balance of commercial reference tracks,
// compares against target mix project, and generates an automated multi-band
// matching EQ filter curve with smoothing and adjustable match percentage.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

struct JZMatchBand
{
  float centerFreqHz;
  float referenceLevelDb;
  float targetLevelDb;
  float correctionGainDb;

  JZMatchBand(float freq = 1000.0f)
    : centerFreqHz(freq),
      referenceLevelDb(-40.0f),
      targetLevelDb(-40.0f),
      correctionGainDb(0.0f)
  {
  }
};

class JZMatchEqProfiler
{
public:
  JZMatchEqProfiler(float sampleRate = 44100.0f);
  ~JZMatchEqProfiler() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  // Spectral Profiling
  bool AnalyzeReferenceAudio(const std::vector<float>& refAudio);
  bool AnalyzeTargetAudio(const std::vector<float>& targetAudio);

  // Match Calculation
  void ComputeMatchCurve();
  const std::vector<JZMatchBand>& GetBands() const { return mBands; }

  // Matching Controls
  void SetMatchAmount(float percent); // 0% (bypass) to 100% (exact match)
  float GetMatchAmount() const { return mMatchAmount; }

  void SetSmoothing(int radius);      // 0 to 6
  int GetSmoothing() const { return mSmoothing; }

  void SetMaxGainLimitDb(float limitDb); // e.g. 12.0 dB
  float GetMaxGainLimitDb() const { return mMaxGainLimitDb; }

  // Real-time Master Audio Processing
  void Process(float& left, float& right);

private:
  float mSampleRate;
  float mMatchAmount;
  int mSmoothing;
  float mMaxGainLimitDb;

  std::vector<JZMatchBand> mBands;
  std::vector<float> mFilterStatesL;
  std::vector<float> mFilterStatesR;

  void InitBands();
  void ExtractSpectrum64(const std::vector<float>& audio, std::vector<float>& outLevelsDb);
};
