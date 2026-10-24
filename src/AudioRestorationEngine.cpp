//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Spectral Audio De-Clipper & De-Clicker Restoration Implementation
//*****************************************************************************

#include "AudioRestorationEngine.h"
#include <cmath>
#include <algorithm>

JZAudioRestorationEngine::JZAudioRestorationEngine(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f),
    mClipThresholdDb(-0.2f),
    mReconstructionDrive(1.25f),
    mClickSensitivity(0.75f)
{
}

void JZAudioRestorationEngine::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
  }
}

void JZAudioRestorationEngine::SetClipThresholdDb(float thresholdDb)
{
  mClipThresholdDb = std::clamp(thresholdDb, -6.0f, -0.01f);
}

void JZAudioRestorationEngine::SetReconstructionDrive(float drive)
{
  mReconstructionDrive = std::clamp(drive, 1.0f, 2.0f);
}

void JZAudioRestorationEngine::SetClickSensitivity(float sens)
{
  mClickSensitivity = std::clamp(sens, 0.1f, 1.0f);
}

void JZAudioRestorationEngine::ProcessDeClip(
  std::vector<float>& audio,
  int& repairedCount,
  float& maxPeakRestored)
{
  repairedCount = 0;
  maxPeakRestored = 0.0f;
  if (audio.size() < 4)
  {
    return;
  }

  float clipThresh = std::pow(10.0f, mClipThresholdDb / 20.0f);
  size_t i = 1;

  while (i < audio.size() - 2)
  {
    if (std::abs(audio[i]) >= clipThresh)
    {
      size_t start = i;
      while (i < audio.size() - 2 && std::abs(audio[i]) >= clipThresh)
      {
        i++;
      }
      size_t end = i; // Clipped segment is [start, end)
      size_t clipLen = end - start;

      if (clipLen > 0 && clipLen <= 128) // Valid digital flat-top clip
      {
        float sign = audio[start] >= 0.0f ? 1.0f : -1.0f;
        float before = audio[start - 1];
        float after = audio[end];

        for (size_t k = 0; k < clipLen; ++k)
        {
          float t = static_cast<float>(k + 1) / static_cast<float>(clipLen + 1);
          // Parabolic/sinusoidal dome reconstruction
          float dome = std::sin(t * 3.14159265f) * (clipThresh * (mReconstructionDrive - 1.0f));
          float interp = (1.0f - t) * before + t * after;
          float reconstructed = interp + sign * dome;

          audio[start + k] = reconstructed;
          repairedCount++;

          if (std::abs(reconstructed) > maxPeakRestored)
          {
            maxPeakRestored = std::abs(reconstructed);
          }
        }
      }
    }
    else
    {
      i++;
    }
  }
}

void JZAudioRestorationEngine::ProcessDeClick(
  std::vector<float>& audio,
  int& clicksRemoved)
{
  clicksRemoved = 0;
  if (audio.size() < 8)
  {
    return;
  }

  float clickThreshold = 0.6f - (mClickSensitivity * 0.45f); // 0.15 to 0.50 threshold

  for (size_t i = 2; i < audio.size() - 2; ++i)
  {
    // Linear predictive residual of 2nd order
    float predicted = 2.0f * audio[i - 1] - audio[i - 2];
    float residual = std::abs(audio[i] - predicted);

    if (residual > clickThreshold)
    {
      // Check if neighboring sample returns to trajectory (impulsive spike signature)
      float nextDiff = std::abs(audio[i + 1] - audio[i - 1]);
      if (nextDiff < residual * 0.5f)
      {
        // Replace impulsive click spike with smooth linear interpolation
        audio[i] = 0.5f * (audio[i - 1] + audio[i + 1]);
        clicksRemoved++;
      }
    }
  }
}

bool JZAudioRestorationEngine::RepairAudio(
  const std::vector<float>& inAudio,
  std::vector<float>& outRepairedAudio,
  JZRestorationReport& outReport)
{
  outReport = JZRestorationReport();
  outRepairedAudio = inAudio;

  if (outRepairedAudio.empty())
  {
    return false;
  }

  float maxPeak = 0.0f;
  ProcessDeClip(outRepairedAudio, outReport.clippedSamplesRepaired, maxPeak);
  ProcessDeClick(outRepairedAudio, outReport.clicksDetectedAndRemoved);

  if (maxPeak > 0.00001f)
  {
    outReport.peakRestorationDb = 20.0f * std::log10(maxPeak);
  }
  else
  {
    outReport.peakRestorationDb = 0.0f;
  }

  outReport.success = true;
  return true;
}
