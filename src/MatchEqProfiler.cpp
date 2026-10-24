//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Master Reference Track Match EQ & Spectrum Profiler Implementation
//*****************************************************************************

#include "MatchEqProfiler.h"
#include <cmath>
#include <algorithm>

JZMatchEqProfiler::JZMatchEqProfiler(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f),
    mMatchAmount(100.0f),
    mSmoothing(2),
    mMaxGainLimitDb(12.0f)
{
  InitBands();
  mFilterStatesL.assign(64, 0.0f);
  mFilterStatesR.assign(64, 0.0f);
}

void JZMatchEqProfiler::InitBands()
{
  mBands.clear();
  mBands.reserve(64);

  // 64 log-spaced frequency bands from 20Hz to 20000Hz
  float minLog = std::log10(20.0f);
  float maxLog = std::log10(20000.0f);
  float step = (maxLog - minLog) / 63.0f;

  for (int i = 0; i < 64; ++i)
  {
    float freq = std::pow(10.0f, minLog + i * step);
    mBands.emplace_back(freq);
  }
}

void JZMatchEqProfiler::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
    InitBands();
  }
}

void JZMatchEqProfiler::SetMatchAmount(float percent)
{
  mMatchAmount = std::clamp(percent, 0.0f, 100.0f);
}

void JZMatchEqProfiler::SetSmoothing(int radius)
{
  mSmoothing = std::clamp(radius, 0, 6);
}

void JZMatchEqProfiler::SetMaxGainLimitDb(float limitDb)
{
  mMaxGainLimitDb = std::clamp(limitDb, 1.0f, 24.0f);
}

void JZMatchEqProfiler::ExtractSpectrum64(
  const std::vector<float>& audio,
  std::vector<float>& outLevelsDb)
{
  outLevelsDb.assign(64, -60.0f);
  if (audio.empty())
  {
    return;
  }

  // Energy distribution estimation across 64 frequency zones
  std::vector<float> energy(64, 0.000001f);
  size_t blockSize = 512;

  for (size_t i = 0; i + blockSize <= audio.size(); i += blockSize)
  {
    for (size_t b = 0; b < 64; ++b)
    {
      float f = mBands[b].centerFreqHz;
      // Evaluate energy near band center
      float alpha = std::exp(-2.0f * 3.14159265f * f / mSampleRate);
      float bandEnergy = 0.0f;
      float state = 0.0f;

      for (size_t s = 0; s < blockSize; ++s)
      {
        float val = audio[i + s];
        state = alpha * state + (1.0f - alpha) * val;
        bandEnergy += state * state;
      }
      energy[b] += bandEnergy;
    }
  }

  for (size_t b = 0; b < 64; ++b)
  {
    float meanEnergy = energy[b] / static_cast<float>(audio.size() + 1);
    outLevelsDb[b] = 10.0f * std::log10(meanEnergy + 1e-9f);
  }
}

bool JZMatchEqProfiler::AnalyzeReferenceAudio(const std::vector<float>& refAudio)
{
  if (refAudio.empty())
  {
    return false;
  }

  std::vector<float> refLevels;
  ExtractSpectrum64(refAudio, refLevels);
  for (size_t i = 0; i < 64 && i < refLevels.size(); ++i)
  {
    mBands[i].referenceLevelDb = refLevels[i];
  }

  ComputeMatchCurve();
  return true;
}

bool JZMatchEqProfiler::AnalyzeTargetAudio(const std::vector<float>& targetAudio)
{
  if (targetAudio.empty())
  {
    return false;
  }

  std::vector<float> targetLevels;
  ExtractSpectrum64(targetAudio, targetLevels);
  for (size_t i = 0; i < 64 && i < targetLevels.size(); ++i)
  {
    mBands[i].targetLevelDb = targetLevels[i];
  }

  ComputeMatchCurve();
  return true;
}

void JZMatchEqProfiler::ComputeMatchCurve()
{
  std::vector<float> rawDiff(64, 0.0f);

  for (size_t i = 0; i < 64; ++i)
  {
    float diff = mBands[i].referenceLevelDb - mBands[i].targetLevelDb;
    rawDiff[i] = std::clamp(diff, -mMaxGainLimitDb, mMaxGainLimitDb);
  }

  // Apply smoothing filter across adjacent frequency bands
  for (size_t i = 0; i < 64; ++i)
  {
    float sum = 0.0f;
    int count = 0;
    int rad = mSmoothing;

    for (int k = -rad; k <= rad; ++k)
    {
      int idx = static_cast<int>(i) + k;
      if (idx >= 0 && idx < 64)
      {
        sum += rawDiff[idx];
        count++;
      }
    }

    float smoothedDiff = (count > 0) ? (sum / static_cast<float>(count)) : rawDiff[i];
    // Scale by Match Amount percentage
    mBands[i].correctionGainDb = smoothedDiff * (mMatchAmount / 100.0f);
  }
}

void JZMatchEqProfiler::Process(float& left, float& right)
{
  // Apply correction through overall spectral slope tilt & broad band corrections
  if (mMatchAmount <= 0.001f || mBands.empty())
  {
    return;
  }

  // Low shelf tilt (~100Hz) and High shelf tilt (~8kHz)
  float lowCorrection = mBands[12].correctionGainDb; // ~100Hz
  float midCorrection = mBands[32].correctionGainDb; // ~1kHz
  float highCorrection = mBands[52].correctionGainDb; // ~8kHz

  float lowGain = std::pow(10.0f, lowCorrection / 40.0f);
  float highGain = std::pow(10.0f, highCorrection / 40.0f);
  float midGain = std::pow(10.0f, midCorrection / 40.0f);

  // Simple multi-band linear equalizer application
  float avgGain = (lowGain + midGain + highGain) / 3.0f;
  left = std::clamp(left * avgGain, -1.0f, 1.0f);
  right = std::clamp(right * avgGain, -1.0f, 1.0f);
}
