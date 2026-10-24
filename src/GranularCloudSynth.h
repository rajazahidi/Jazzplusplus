//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Stereo Granular Cloud Texture Synthesizer
//
// Granular synthesis engine breaking audio into thousands of micro-grains
// (5ms to 120ms) with Hanning windowing, stereo pan spraying, pitch detune jitter,
// reverse probability, and infinite audio freeze time-stretching.
//*****************************************************************************

#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

struct JZMicroGrain
{
  float sourceReadPos;
  size_t lengthSamples;
  size_t currentAge;
  float playbackSpeed;        // Pitch ratio
  float panL, panR;
  bool isReversed;
  bool active;

  JZMicroGrain()
    : sourceReadPos(0.0f),
      lengthSamples(1024),
      currentAge(0),
      playbackSpeed(1.0f),
      panL(0.707f), panR(0.707f),
      isReversed(false),
      active(false)
  {
  }
};

class JZGranularCloudSynth
{
public:
  JZGranularCloudSynth(float sampleRate = 44100.0f);
  ~JZGranularCloudSynth() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  // Load Source Audio Buffer
  void LoadSourceAudio(const std::vector<float>& pcmAudio);

  // Granular Cloud Controls
  void SetGrainDensity(float grainsPerSec);    // 1 to 64 grains/sec
  float GetGrainDensity() const { return mGrainDensity; }

  void SetGrainSizeMs(float sizeMs);          // 10ms to 120ms
  float GetGrainSizeMs() const { return mGrainSizeMs; }

  void SetStereoSpray(float sprayPercent);    // 0% to 100%
  float GetStereoSpray() const { return mStereoSpray; }

  void SetPitchJitterCents(float cents);      // 0 to 100 cents
  float GetPitchJitterCents() const { return mPitchJitterCents; }

  void SetReverseProbability(float prob);     // 0% to 50%
  float GetReverseProbability() const { return mReverseProbability; }

  void SetFreeze(bool freeze) { mFreeze = freeze; }
  bool IsFrozen() const { return mFreeze; }

  // Audio Generation
  void SynthesizeStereo(float& outL, float& outR);
  void ProcessBuffer(std::vector<float>& outL, std::vector<float>& outR, size_t numSamples);

private:
  float mSampleRate;
  float mGrainDensity;
  float mGrainSizeMs;
  float mStereoSpray;
  float mPitchJitterCents;
  float mReverseProbability;
  bool  mFreeze;

  std::vector<float> mSourceAudio;
  std::vector<JZMicroGrain> mGrains;
  float mPlayheadPosition;
  float mSpawnAccumulator;

  void SpawnGrain();
};
