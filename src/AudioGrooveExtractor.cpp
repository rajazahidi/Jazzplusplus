//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Audio Transient Groove Extractor & Swing Quantize Implementation
//*****************************************************************************

#include "AudioGrooveExtractor.h"
#include <cmath>
#include <algorithm>

JZAudioGrooveExtractor::JZAudioGrooveExtractor(float sampleRate, int ticksPerQuarter)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f),
    mTicksPerQuarter(ticksPerQuarter > 24 ? ticksPerQuarter : 96)
{
}

void JZAudioGrooveExtractor::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
  }
}

void JZAudioGrooveExtractor::DetectTransients(
  const std::vector<float>& audio,
  std::vector<size_t>& outTransientSamples)
{
  outTransientSamples.clear();
  if (audio.size() < 128) return;

  size_t minSpacing = static_cast<size_t>(mSampleRate * 0.04f); // 40ms refractory period
  size_t lastHit = 0;

  float env = 0.0f;
  float prevVal = audio[0];

  for (size_t i = 1; i < audio.size(); ++i)
  {
    float diff = std::abs(audio[i] - prevVal);
    prevVal = audio[i];

    env = 0.95f * env + 0.05f * diff;
    if (diff > 0.15f && diff > env * 2.5f && (i - lastHit) >= minSpacing)
    {
      outTransientSamples.push_back(i);
      lastHit = i;
    }
  }
}

bool JZAudioGrooveExtractor::ExtractGrooveFromAudio(
  const std::vector<float>& audioLoop,
  float knownOrEstimatedBpm,
  JZGrooveTemplate& outGroove)
{
  outGroove = JZGrooveTemplate();
  outGroove.detectedBpm = std::clamp(knownOrEstimatedBpm, 40.0f, 300.0f);

  if (audioLoop.empty())
  {
    return false;
  }

  std::vector<size_t> transients;
  DetectTransients(audioLoop, transients);

  float secondsPerBeat = 60.0f / outGroove.detectedBpm;
  float samplesPerBar = secondsPerBeat * 4.0f * mSampleRate;
  float samplesPer16th = samplesPerBar / 16.0f;
  float samplesPerTick = samplesPer16th / (mTicksPerQuarter / 4.0f);

  float accumulatedSwingOffset = 0.0f;
  int swingCount = 0;

  for (int step = 0; step < 16; ++step)
  {
    float idealSample = step * samplesPer16th;
    float closestDist = 999999.0f;
    size_t closestTransient = 0;
    bool found = false;

    for (size_t tSample : transients)
    {
      float dist = std::abs(static_cast<float>(tSample) - idealSample);
      if (dist < closestDist && dist < samplesPer16th * 0.6f)
      {
        closestDist = dist;
        closestTransient = tSample;
        found = true;
      }
    }

    if (found)
    {
      float sampleDiff = static_cast<float>(closestTransient) - idealSample;
      int tickDiff = static_cast<int>(std::round(sampleDiff / samplesPerTick));
      tickDiff = std::clamp(tickDiff, -24, 24);
      outGroove.timingOffsetsTicks[step] = tickDiff;

      // Extract local RMS velocity accent
      size_t winStart = closestTransient;
      size_t winEnd = std::min(winStart + 256, audioLoop.size());
      float sumSq = 0.0f;
      for (size_t k = winStart; k < winEnd; ++k) sumSq += audioLoop[k] * audioLoop[k];
      float rms = std::sqrt(sumSq / static_cast<float>(winEnd - winStart));

      outGroove.velocityScalers[step] = std::clamp(rms * 4.0f, 0.4f, 1.6f);

      // Swing analysis on off-beat 16ths (steps 1, 3, 5, 7, 9, 11, 13, 15)
      if (step % 2 != 0)
      {
        accumulatedSwingOffset += tickDiff;
        swingCount++;
      }
    }
  }

  if (swingCount > 0)
  {
    float avgSwingShiftTicks = accumulatedSwingOffset / static_cast<float>(swingCount);
    // 50% = 0 shift, 66% (triplet) = +8 ticks on 96 TPQ (24 ticks per 16th)
    outGroove.swingPercent = 50.0f + (avgSwingShiftTicks / 8.0f) * 16.0f;
    outGroove.swingPercent = std::clamp(outGroove.swingPercent, 50.0f, 75.0f);
  }

  return true;
}

int JZAudioGrooveExtractor::ApplyGrooveToTick(
  int rawTick,
  const JZGrooveTemplate& groove,
  float grooveAmountPercent) const
{
  int ticksPer16th = mTicksPerQuarter / 4;
  int stepIdx = (rawTick / ticksPer16th) % 16;
  float factor = std::clamp(grooveAmountPercent, 0.0f, 100.0f) / 100.0f;

  int offset = static_cast<int>(groove.timingOffsetsTicks[stepIdx] * factor);
  return std::max(0, rawTick + offset);
}

int JZAudioGrooveExtractor::ApplyGrooveToVelocity(
  int rawVelocity,
  int tick,
  const JZGrooveTemplate& groove,
  float grooveAmountPercent) const
{
  int ticksPer16th = mTicksPerQuarter / 4;
  int stepIdx = (tick / ticksPer16th) % 16;
  float factor = std::clamp(grooveAmountPercent, 0.0f, 100.0f) / 100.0f;

  float targetScale = groove.velocityScalers[stepIdx];
  float effectiveScale = (1.0f - factor) + factor * targetScale;

  int vel = static_cast<int>(rawVelocity * effectiveScale);
  return std::clamp(vel, 1, 127);
}
