//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Smart Dynamic Spectral Resonance Tamer Implementation
//*****************************************************************************

#include "SpectralResonanceTamer.h"
#include <cmath>
#include <algorithm>

JZSpectralResonanceTamer::JZSpectralResonanceTamer(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f),
    mDepth(50.0f),
    mSharpnessQ(4.0f),
    mSensitivity(0.65f),
    mListenDelta(false),
    mCurrentAttenuationDb(0.0f)
{
  InitNotches();
}

void JZSpectralResonanceTamer::InitNotches()
{
  mNotches.clear();
  mNotches.reserve(16);

  float minFreq = 1000.0f;
  float maxFreq = 11000.0f;
  float minLog = std::log10(minFreq);
  float maxLog = std::log10(maxFreq);
  float step = (maxLog - minLog) / 15.0f;

  for (int i = 0; i < 16; ++i)
  {
    float f = std::pow(10.0f, minLog + i * step);
    mNotches.emplace_back(f);
  }
}

void JZSpectralResonanceTamer::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
    InitNotches();
  }
}

void JZSpectralResonanceTamer::SetDepth(float depthPercent)
{
  mDepth = std::clamp(depthPercent, 0.0f, 100.0f);
}

void JZSpectralResonanceTamer::SetSharpnessQ(float q)
{
  mSharpnessQ = std::clamp(q, 1.0f, 10.0f);
}

void JZSpectralResonanceTamer::SetSensitivity(float sens)
{
  mSensitivity = std::clamp(sens, 0.0f, 1.0f);
}

void JZSpectralResonanceTamer::Process(float& left, float& right)
{
  float origL = left;
  float origR = right;

  if (mDepth <= 0.001f)
  {
    if (mListenDelta)
    {
      left = 0.0f;
      right = 0.0f;
    }
    mCurrentAttenuationDb = 0.0f;
    return;
  }

  float overallRms = std::sqrt(0.5f * (origL * origL + origR * origR));
  float threshold = (1.0f - mSensitivity * 0.7f) * 0.15f;
  float maxAtten = 1.0f;

  float processedL = origL;
  float processedR = origR;

  for (auto& notch : mNotches)
  {
    // Biquad Bandpass filter to isolate narrow resonance band
    float omega = 2.0f * 3.14159265f * notch.centerHz / mSampleRate;
    float alpha = std::sin(omega) / (2.0f * mSharpnessQ);

    // Filter single step (Direct Form 1)
    float bpL = alpha * (origL - notch.state2L);
    notch.state2L = notch.state1L;
    notch.state1L = bpL;

    float bpR = alpha * (origR - notch.state2R);
    notch.state2R = notch.state1R;
    notch.state1R = bpR;

    float bandRms = std::sqrt(0.5f * (bpL * bpL + bpR * bpR));

    // Fast envelope follower
    float envCoeff = (bandRms > notch.envFollower) ? 0.85f : 0.995f;
    notch.envFollower = envCoeff * notch.envFollower + (1.0f - envCoeff) * bandRms;

    // Resonant harshness spike detection
    if (notch.envFollower > threshold && overallRms > 0.0001f)
    {
      float excess = (notch.envFollower - threshold) / (notch.envFollower + 0.01f);
      float targetGain = 1.0f - (mDepth / 100.0f) * std::clamp(excess, 0.0f, 0.85f);
      notch.dynamicGain = 0.9f * notch.dynamicGain + 0.1f * targetGain;
    }
    else
    {
      notch.dynamicGain = 0.99f * notch.dynamicGain + 0.01f * 1.0f; // Smooth return
    }

    if (notch.dynamicGain < maxAtten)
    {
      maxAtten = notch.dynamicGain;
    }

    // Apply notch cut dynamically
    float cutAmount = 1.0f - notch.dynamicGain;
    processedL -= bpL * cutAmount;
    processedR -= bpR * cutAmount;
  }

  mCurrentAttenuationDb = (maxAtten < 0.999f) ? (20.0f * std::log10(maxAtten)) : 0.0f;

  if (mListenDelta)
  {
    left = origL - processedL;
    right = origR - processedR;
  }
  else
  {
    left = std::clamp(processedL, -1.0f, 1.0f);
    right = std::clamp(processedR, -1.0f, 1.0f);
  }
}
