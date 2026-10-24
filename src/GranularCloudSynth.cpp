//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Stereo Granular Cloud Texture Synthesizer Implementation
//*****************************************************************************

#include "GranularCloudSynth.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

JZGranularCloudSynth::JZGranularCloudSynth(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f),
    mGrainDensity(24.0f),      // 24 grains per second
    mGrainSizeMs(45.0f),       // 45ms grain duration
    mStereoSpray(75.0f),       // 75% stereo widening
    mPitchJitterCents(12.0f),  // +/- 12 cents micro-detuning
    mReverseProbability(15.0f),// 15% grains play in reverse
    mFreeze(false),
    mPlayheadPosition(0.0f),
    mSpawnAccumulator(0.0f)
{
  mGrains.resize(64); // Max 64 simultaneous overlapping grains

  // Default atmospheric harmonic source wave
  mSourceAudio.resize(static_cast<size_t>(mSampleRate * 2.0f));
  for (size_t i = 0; i < mSourceAudio.size(); ++i)
  {
    float t = static_cast<float>(i) / mSampleRate;
    mSourceAudio[i] = std::sin(2.0f * 3.14159265f * 220.0f * t) * 0.5f +
                      std::sin(2.0f * 3.14159265f * 440.0f * t) * 0.25f +
                      std::sin(2.0f * 3.14159265f * 660.0f * t) * 0.15f;
  }
}

void JZGranularCloudSynth::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
  }
}

void JZGranularCloudSynth::LoadSourceAudio(const std::vector<float>& pcmAudio)
{
  if (!pcmAudio.empty())
  {
    mSourceAudio = pcmAudio;
    mPlayheadPosition = 0.0f;
  }
}

void JZGranularCloudSynth::SetGrainDensity(float grainsPerSec)
{
  mGrainDensity = std::clamp(grainsPerSec, 1.0f, 64.0f);
}

void JZGranularCloudSynth::SetGrainSizeMs(float sizeMs)
{
  mGrainSizeMs = std::clamp(sizeMs, 5.0f, 150.0f);
}

void JZGranularCloudSynth::SetStereoSpray(float sprayPercent)
{
  mStereoSpray = std::clamp(sprayPercent, 0.0f, 100.0f);
}

void JZGranularCloudSynth::SetPitchJitterCents(float cents)
{
  mPitchJitterCents = std::clamp(cents, 0.0f, 100.0f);
}

void JZGranularCloudSynth::SetReverseProbability(float prob)
{
  mReverseProbability = std::clamp(prob, 0.0f, 50.0f);
}

void JZGranularCloudSynth::SpawnGrain()
{
  if (mSourceAudio.empty()) return;

  // Find inactive grain
  JZMicroGrain* targetGrain = nullptr;
  for (auto& g : mGrains)
  {
    if (!g.active)
    {
      targetGrain = &g;
      break;
    }
  }
  if (!targetGrain) targetGrain = &mGrains[0]; // Reuse oldest

  targetGrain->lengthSamples = static_cast<size_t>(mGrainSizeMs * 0.001f * mSampleRate);
  if (targetGrain->lengthSamples < 16) targetGrain->lengthSamples = 16;
  targetGrain->currentAge = 0;

  // Random position jitter around current playhead
  float posJitter = (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) - 0.5f) * 0.05f * mSampleRate;
  float pos = mPlayheadPosition + posJitter;
  if (pos < 0.0f) pos += static_cast<float>(mSourceAudio.size());
  if (pos >= static_cast<float>(mSourceAudio.size() - 2)) pos = 0.0f;
  targetGrain->sourceReadPos = pos;

  // Pitch detune jitter
  float cents = (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) - 0.5f) * 2.0f * mPitchJitterCents;
  targetGrain->playbackSpeed = std::pow(2.0f, cents / 1200.0f);

  // Stereo Spray Pan
  float pan = (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) - 0.5f) * 2.0f * (mStereoSpray / 100.0f);
  pan = std::clamp(pan, -1.0f, 1.0f);
  targetGrain->panL = std::cos((pan + 1.0f) * 0.25f * 3.14159265f);
  targetGrain->panR = std::sin((pan + 1.0f) * 0.25f * 3.14159265f);

  // Reverse probability
  float revRoll = (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * 100.0f;
  targetGrain->isReversed = (revRoll < mReverseProbability);

  targetGrain->active = true;
}

void JZGranularCloudSynth::SynthesizeStereo(float& outL, float& outR)
{
  outL = 0.0f;
  outR = 0.0f;

  if (mSourceAudio.empty()) return;

  // Advance playhead unless frozen
  if (!mFreeze)
  {
    mPlayheadPosition += 1.0f;
    if (mPlayheadPosition >= static_cast<float>(mSourceAudio.size() - 2))
    {
      mPlayheadPosition = 0.0f;
    }
  }

  // Grain spawning rate accumulator
  mSpawnAccumulator += mGrainDensity / mSampleRate;
  if (mSpawnAccumulator >= 1.0f)
  {
    SpawnGrain();
    mSpawnAccumulator -= 1.0f;
  }

  // Render all active grains
  int activeCount = 0;
  for (auto& g : mGrains)
  {
    if (!g.active) continue;

    // Hanning window amplitude
    float phase = static_cast<float>(g.currentAge) / static_cast<float>(g.lengthSamples);
    float window = 0.5f * (1.0f - std::cos(2.0f * 3.14159265f * phase));

    // Calculate source sample read position
    float readOffset = (g.isReversed) ?
      (static_cast<float>(g.lengthSamples - g.currentAge) * g.playbackSpeed) :
      (static_cast<float>(g.currentAge) * g.playbackSpeed);

    float readPos = g.sourceReadPos + readOffset;
    if (readPos < 0.0f) readPos += static_cast<float>(mSourceAudio.size());
    size_t sIdx = static_cast<size_t>(readPos) % mSourceAudio.size();
    size_t sNext = (sIdx + 1) % mSourceAudio.size();
    float frac = readPos - static_cast<float>(static_cast<size_t>(readPos));

    float sampleVal = (1.0f - frac) * mSourceAudio[sIdx] + frac * mSourceAudio[sNext];
    float grainSample = sampleVal * window;

    outL += grainSample * g.panL;
    outR += grainSample * g.panR;

    g.currentAge++;
    if (g.currentAge >= g.lengthSamples)
    {
      g.active = false;
    }
    activeCount++;
  }

  float normFactor = (activeCount > 4) ? (2.0f / std::sqrt(static_cast<float>(activeCount))) : 0.65f;
  outL = std::clamp(outL * normFactor, -1.0f, 1.0f);
  outR = std::clamp(outR * normFactor, -1.0f, 1.0f);
}

void JZGranularCloudSynth::ProcessBuffer(
  std::vector<float>& outL,
  std::vector<float>& outR,
  size_t numSamples)
{
  outL.resize(numSamples);
  outR.resize(numSamples);
  for (size_t i = 0; i < numSamples; ++i)
  {
    SynthesizeStereo(outL[i], outR[i]);
  }
}
