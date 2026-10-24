//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Audio Flex-Time Warping & Multi-Marker Transient Quantizer Implementation
//*****************************************************************************

#include "AudioFlexTimeWarp.h"

JZAudioFlexTimeWarp::JZAudioFlexTimeWarp(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f)
{
}

void JZAudioFlexTimeWarp::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
  }
}

void JZAudioFlexTimeWarp::AddWarpMarker(int originalSample, int warpedSample, bool pinned)
{
  mMarkers.emplace_back(std::max(0, originalSample), std::max(0, warpedSample), pinned);
  SortAndDeduplicateMarkers();
}

void JZAudioFlexTimeWarp::ClearWarpMarkers()
{
  mMarkers.clear();
}

void JZAudioFlexTimeWarp::SortAndDeduplicateMarkers()
{
  std::sort(mMarkers.begin(), mMarkers.end(), [](const JZWarpMarker& a, const JZWarpMarker& b) {
    return a.warpedSample < b.warpedSample;
  });

  // Remove duplicate points at same warped sample
  auto it = std::unique(mMarkers.begin(), mMarkers.end(), [](const JZWarpMarker& a, const JZWarpMarker& b) {
    return a.warpedSample == b.warpedSample;
  });
  mMarkers.erase(it, mMarkers.end());
}

int JZAudioFlexTimeWarp::AutoQuantizeTransients(
  const std::vector<float>& audioBuffer,
  float bpm,
  int gridSubdivision,
  float quantizeStrength)
{
  if (audioBuffer.empty() || bpm <= 10.0f) return 0;

  ClearWarpMarkers();
  int totalSamples = (int)audioBuffer.size();

  // Grid step in samples (e.g. 1/16th note at BPM)
  float beatDurationSec = 60.0f / bpm;
  float gridStepSec = (beatDurationSec * 4.0f) / (float)std::max(1, gridSubdivision);
  int gridStepSamples = (int)(gridStepSec * mSampleRate);
  if (gridStepSamples <= 0) return 0;

  // Always pin start and end
  AddWarpMarker(0, 0, true);

  // Transient energy detector (10ms window)
  int winSize = (int)(mSampleRate * 0.01f);
  std::vector<int> detectedOnsets;
  float prevEnergy = 0.0f;

  for (int i = 0; i < totalSamples - winSize; i += winSize / 2)
  {
    float currentEnergy = 0.0f;
    for (int j = 0; j < winSize; ++j)
    {
      currentEnergy += audioBuffer[i + j] * audioBuffer[i + j];
    }
    currentEnergy /= (float)winSize;

    if (currentEnergy > 0.005f && currentEnergy > prevEnergy * 2.2f)
    {
      detectedOnsets.push_back(i);
      i += winSize * 3; // Refractory holdoff to avoid re-triggering on same transient
    }
    prevEnergy = currentEnergy;
  }

  // Snap each transient to nearest grid step with user strength
  float str = std::max(0.0f, std::min(1.0f, quantizeStrength));
  int quantCount = 0;

  for (int onset : detectedOnsets)
  {
    int nearestGrid = (int)(std::round((float)onset / (float)gridStepSamples) * gridStepSamples);
    int targetSample = (int)(onset + (nearestGrid - onset) * str);
    targetSample = std::max(0, std::min(totalSamples - 1, targetSample));

    AddWarpMarker(onset, targetSample, false);
    quantCount++;
  }

  AddWarpMarker(totalSamples - 1, totalSamples - 1, true);
  return quantCount;
}

double JZAudioFlexTimeWarp::MapOutputToInputSample(int outputSample, int maxInputSample) const
{
  if (mMarkers.size() < 2)
  {
    return (double)outputSample;
  }

  // Find bounding markers [m0, m1] around outputSample
  if (outputSample <= mMarkers.front().warpedSample)
  {
    return (double)mMarkers.front().originalSample;
  }
  if (outputSample >= mMarkers.back().warpedSample)
  {
    return (double)mMarkers.back().originalSample;
  }

  for (size_t i = 0; i < mMarkers.size() - 1; ++i)
  {
    const auto& m0 = mMarkers[i];
    const auto& m1 = mMarkers[i + 1];

    if (outputSample >= m0.warpedSample && outputSample <= m1.warpedSample)
    {
      double spanWarp = (double)(m1.warpedSample - m0.warpedSample);
      if (spanWarp <= 0.0001) return (double)m0.originalSample;

      double frac = (double)(outputSample - m0.warpedSample) / spanWarp;
      double spanOrig = (double)(m1.originalSample - m0.originalSample);
      double mapped = (double)m0.originalSample + frac * spanOrig;
      return std::max(0.0, std::min((double)maxInputSample, mapped));
    }
  }

  return (double)outputSample;
}

bool JZAudioFlexTimeWarp::ProcessWarp(
  const std::vector<float>& inL,
  const std::vector<float>& inR,
  std::vector<float>& outL,
  std::vector<float>& outR)
{
  int inCount = (int)inL.size();
  if (inCount <= 0) return false;

  if (mMarkers.size() < 2)
  {
    outL = inL;
    outR = inR;
    return true;
  }

  int outCount = mMarkers.back().warpedSample + 1;
  outCount = std::max(1, outCount);

  outL.resize(outCount, 0.0f);
  outR.resize(outCount, 0.0f);

  for (int outIdx = 0; outIdx < outCount; ++outIdx)
  {
    double inPos = MapOutputToInputSample(outIdx, inCount - 1);
    int idx0 = (int)inPos;
    int idx1 = std::min(inCount - 1, idx0 + 1);
    float frac = (float)(inPos - idx0);

    float lVal = inL[idx0] + frac * (inL[idx1] - inL[idx0]);
    outL[outIdx] = lVal;

    if (!inR.empty())
    {
      float rVal = inR[idx0] + frac * (inR[idx1] - inR[idx0]);
      outR[outIdx] = rVal;
    }
    else
    {
      outR[outIdx] = lVal;
    }
  }

  return true;
}
