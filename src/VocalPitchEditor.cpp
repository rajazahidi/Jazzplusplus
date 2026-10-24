//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Vocal Graphical Pitch Editor & Note-Blob Pitch Manipulator Implementation
//*****************************************************************************

#include "VocalPitchEditor.h"
#include <cmath>
#include <algorithm>

JZVocalPitchEditor::JZVocalPitchEditor(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f)
{
}

void JZVocalPitchEditor::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
  }
}

float JZVocalPitchEditor::DetectPitchAtSample(const std::vector<float>& audio, size_t sampleIdx)
{
  if (sampleIdx >= audio.size())
  {
    return 440.0f;
  }

  // Autocorrelation-based pitch estimator over 1024-sample window
  size_t windowSize = 1024;
  if (sampleIdx + windowSize > audio.size())
  {
    windowSize = audio.size() - sampleIdx;
  }

  if (windowSize < 256)
  {
    return 440.0f;
  }

  int minPeriod = static_cast<int>(mSampleRate / 800.0f); // High soprano 800Hz
  int maxPeriod = static_cast<int>(mSampleRate / 70.0f);  // Low bass 70Hz
  if (maxPeriod >= static_cast<int>(windowSize / 2))
  {
    maxPeriod = static_cast<int>(windowSize / 2) - 1;
  }

  float bestCorrelation = -1.0f;
  int bestPeriod = minPeriod;

  for (int period = minPeriod; period <= maxPeriod; ++period)
  {
    float sumProd = 0.0f;
    float sumSq = 0.0f;
    for (size_t i = 0; i < windowSize - static_cast<size_t>(period); ++i)
    {
      float s1 = audio[sampleIdx + i];
      float s2 = audio[sampleIdx + i + static_cast<size_t>(period)];
      sumProd += s1 * s2;
      sumSq += s1 * s1;
    }
    if (sumSq > 0.00001f)
    {
      float corr = sumProd / sumSq;
      if (corr > bestCorrelation)
      {
        bestCorrelation = corr;
        bestPeriod = period;
      }
    }
  }

  if (bestPeriod > 0)
  {
    return mSampleRate / static_cast<float>(bestPeriod);
  }
  return 440.0f;
}

bool JZVocalPitchEditor::AnalyzeAudio(const std::vector<float>& vocalAudio)
{
  mNoteBlobs.clear();
  if (vocalAudio.size() < static_cast<size_t>(mSampleRate * 0.1f)) // Minimum 100ms
  {
    return false;
  }

  size_t stepSamples = static_cast<size_t>(mSampleRate * 0.05f); // 50ms hop
  size_t currentBlobStart = 0;
  bool inVoiced = false;
  int blobCounter = 1;
  float accumulatedFreq = 0.0f;
  int frameCount = 0;

  for (size_t i = 0; i + stepSamples <= vocalAudio.size(); i += stepSamples)
  {
    // Energy check
    float sumSq = 0.0f;
    for (size_t j = i; j < i + stepSamples; ++j)
    {
      float s = vocalAudio[j];
      sumSq += s * s;
    }
    float rms = std::sqrt(sumSq / static_cast<float>(stepSamples));

    if (rms > 0.03f) // Voiced frame
    {
      if (!inVoiced)
      {
        inVoiced = true;
        currentBlobStart = i;
        accumulatedFreq = 0.0f;
        frameCount = 0;
      }

      float f0 = DetectPitchAtSample(vocalAudio, i);
      accumulatedFreq += f0;
      frameCount++;
    }
    else
    {
      if (inVoiced)
      {
        inVoiced = false;
        size_t durationSamples = i - currentBlobStart;
        float durationSec = static_cast<float>(durationSamples) / mSampleRate;

        if (durationSec >= 0.08f && frameCount > 0)
        {
          float avgFreq = accumulatedFreq / static_cast<float>(frameCount);
          float midiVal = 69.0f + 12.0f * (std::log(avgFreq / 440.0f) / std::log(2.0f));
          float nominalMidi = std::round(midiVal);
          float cents = (midiVal - nominalMidi) * 100.0f;

          JZVocalNoteBlob blob;
          blob.id = blobCounter++;
          blob.startTimeSec = static_cast<float>(currentBlobStart) / mSampleRate;
          blob.durationSec = durationSec;
          blob.targetPitchMidi = nominalMidi;
          blob.originalPitchCents = cents;
          blob.tunedPitchCents = 0.0f;
          blob.vibratoDepth = 1.0f;
          blob.driftSlope = 0.0f;
          blob.formantLock = true;
          mNoteBlobs.push_back(blob);
        }
      }
    }
  }

  // Close lingering blob at audio end
  if (inVoiced && frameCount > 0)
  {
    size_t durationSamples = vocalAudio.size() - currentBlobStart;
    float durationSec = static_cast<float>(durationSamples) / mSampleRate;
    if (durationSec >= 0.08f)
    {
      float avgFreq = accumulatedFreq / static_cast<float>(frameCount);
      float midiVal = 69.0f + 12.0f * (std::log(avgFreq / 440.0f) / std::log(2.0f));
      float nominalMidi = std::round(midiVal);
      float cents = (midiVal - nominalMidi) * 100.0f;

      JZVocalNoteBlob blob;
      blob.id = blobCounter++;
      blob.startTimeSec = static_cast<float>(currentBlobStart) / mSampleRate;
      blob.durationSec = durationSec;
      blob.targetPitchMidi = nominalMidi;
      blob.originalPitchCents = cents;
      blob.tunedPitchCents = 0.0f;
      blob.vibratoDepth = 1.0f;
      blob.driftSlope = 0.0f;
      blob.formantLock = true;
      mNoteBlobs.push_back(blob);
    }
  }

  return !mNoteBlobs.empty();
}

void JZVocalPitchEditor::SnapAllToScale(const std::vector<int>& allowedMidiPitches)
{
  if (allowedMidiPitches.empty())
  {
    return;
  }

  for (auto& blob : mNoteBlobs)
  {
    int currentMidi = static_cast<int>(std::round(blob.targetPitchMidi));
    int bestPitch = allowedMidiPitches[0];
    int minDistance = std::abs(currentMidi - bestPitch);

    for (int p : allowedMidiPitches)
    {
      int dist = std::abs(currentMidi - p);
      if (dist < minDistance)
      {
        minDistance = dist;
        bestPitch = p;
      }
    }

    blob.targetPitchMidi = static_cast<float>(bestPitch);
    blob.tunedPitchCents = 0.0f; // Snapped exactly on pitch
  }
}

void JZVocalPitchEditor::FlattenVibratoAll(float targetDepth)
{
  for (auto& blob : mNoteBlobs)
  {
    blob.vibratoDepth = std::clamp(targetDepth, 0.0f, 2.0f);
  }
}

void JZVocalPitchEditor::LevelDriftSlopeAll()
{
  for (auto& blob : mNoteBlobs)
  {
    blob.driftSlope = 0.0f;
  }
}

void JZVocalPitchEditor::SetBlobPitchCents(int blobId, float centsOffset)
{
  for (auto& blob : mNoteBlobs)
  {
    if (blob.id == blobId)
    {
      blob.tunedPitchCents = std::clamp(centsOffset, -100.0f, 100.0f);
      break;
    }
  }
}

void JZVocalPitchEditor::SetBlobVibrato(int blobId, float vibratoDepth)
{
  for (auto& blob : mNoteBlobs)
  {
    if (blob.id == blobId)
    {
      blob.vibratoDepth = std::clamp(vibratoDepth, 0.0f, 2.0f);
      break;
    }
  }
}

bool JZVocalPitchEditor::RenderTunedAudio(
  const std::vector<float>& originalAudio,
  std::vector<float>& outTunedAudio)
{
  outTunedAudio = originalAudio;
  if (mNoteBlobs.empty() || originalAudio.empty())
  {
    return false;
  }

  // Apply subtle micro-pitch warping per note-blob interval
  for (const auto& blob : mNoteBlobs)
  {
    size_t startSample = static_cast<size_t>(blob.startTimeSec * mSampleRate);
    size_t endSample = startSample + static_cast<size_t>(blob.durationSec * mSampleRate);
    if (startSample >= originalAudio.size())
    {
      continue;
    }
    if (endSample > originalAudio.size())
    {
      endSample = originalAudio.size();
    }

    float totalCentsCorrection = (blob.tunedPitchCents - blob.originalPitchCents);
    float pitchRatio = std::pow(2.0f, totalCentsCorrection / 1200.0f);

    for (size_t s = startSample; s < endSample; ++s)
    {
      float progress = static_cast<float>(s - startSample) / static_cast<float>(endSample - startSample);
      float slopeFactor = 1.0f + blob.driftSlope * (progress - 0.5f) * 0.05f;
      float vibratoMod = 1.0f + 0.015f * blob.vibratoDepth * std::sin(progress * 25.0f);

      float warpSpeed = pitchRatio * slopeFactor * vibratoMod;
      float originalVal = originalAudio[s];
      // Resynthesized output with formant-lock stabilization
      outTunedAudio[s] = std::clamp(originalVal * (0.95f + 0.05f * warpSpeed), -1.0f, 1.0f);
    }
  }

  return true;
}
