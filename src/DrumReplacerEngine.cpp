//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Phase-Locked Transient Drum Replacer Implementation
//*****************************************************************************

#include "DrumReplacerEngine.h"
#include <cmath>
#include <algorithm>

JZDrumReplacerEngine::JZDrumReplacerEngine(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f),
    mThresholdDb(-20.0f),
    mSensitivity(0.80f),
    mRetriggerIntervalMs(50.0f),
    mLeakageRejection(0.65f),
    mDryWetMix(100.0f) // 100% replacement by default
{
  // Generate high-quality synthesized drum transient samples as defaults
  size_t sampleLen = static_cast<size_t>(mSampleRate * 0.35f); // 350ms decay
  mSampleSoft.resize(sampleLen);
  mSampleMed.resize(sampleLen);
  mSampleHard.resize(sampleLen);
  mSampleRim.resize(sampleLen);

  for (size_t i = 0; i < sampleLen; ++i)
  {
    float t = static_cast<float>(i) / mSampleRate;
    float decay = std::exp(-t * 22.0f);
    float f = 180.0f * std::exp(-t * 35.0f) + 55.0f; // Snare body sweep
    float osc = std::sin(2.0f * 3.14159265f * f * t);

    mSampleSoft[i] = osc * decay * 0.45f;
    mSampleMed[i]  = osc * decay * 0.75f;
    mSampleHard[i] = osc * decay * 1.0f;
    mSampleRim[i]  = (osc * 0.7f + std::sin(2.0f * 3.14159265f * 950.0f * t) * 0.3f) * decay * 1.05f;
  }
}

void JZDrumReplacerEngine::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
  }
}

void JZDrumReplacerEngine::SetThresholdDb(float thresholdDb)
{
  mThresholdDb = std::clamp(thresholdDb, -60.0f, 0.0f);
}

void JZDrumReplacerEngine::SetSensitivity(float sens)
{
  mSensitivity = std::clamp(sens, 0.05f, 1.0f);
}

void JZDrumReplacerEngine::SetRetriggerIntervalMs(float ms)
{
  mRetriggerIntervalMs = std::clamp(ms, 15.0f, 300.0f);
}

void JZDrumReplacerEngine::SetLeakageRejection(float rejection)
{
  mLeakageRejection = std::clamp(rejection, 0.0f, 1.0f);
}

void JZDrumReplacerEngine::SetDryWetMix(float mixPercent)
{
  mDryWetMix = std::clamp(mixPercent, 0.0f, 100.0f);
}

void JZDrumReplacerEngine::LoadReplacementSample(
  JZDrumHitType type,
  const std::vector<float>& pcmAudio)
{
  if (pcmAudio.empty()) return;
  switch (type)
  {
    case JZDrumHitType::Soft: mSampleSoft = pcmAudio; break;
    case JZDrumHitType::Medium: mSampleMed = pcmAudio; break;
    case JZDrumHitType::Hard: mSampleHard = pcmAudio; break;
    case JZDrumHitType::Rimshot: mSampleRim = pcmAudio; break;
  }
}

bool JZDrumReplacerEngine::DetectPhaseLockedHit(
  const std::vector<float>& audio,
  size_t idx,
  float thresholdLinear,
  float& outVelocity,
  JZDrumHitType& outHitType)
{
  if (idx < 2 || idx >= audio.size() - 2) return false;

  float s0 = audio[idx];
  float sPrev = audio[idx - 1];
  float absVal = std::abs(s0);

  if (absVal < thresholdLinear) return false;

  // Onset transient detection (rising zero-crossing or peak sharpness)
  float delta = absVal - std::abs(sPrev);
  if (delta > thresholdLinear * 0.4f * (1.0f - mSensitivity * 0.5f))
  {
    // Leakage rejection: check high frequency ratio vs low body
    float highBleedRatio = std::abs(s0 - 2.0f * sPrev + audio[idx - 2]);
    if (mLeakageRejection > 0.01f && highBleedRatio > absVal * (2.2f - mLeakageRejection))
    {
      return false; // Hi-hat / cymbal bleed rejected
    }

    outVelocity = std::clamp(absVal * 1.15f, 0.1f, 1.0f);

    if (outVelocity < 0.45f) outHitType = JZDrumHitType::Soft;
    else if (outVelocity < 0.75f) outHitType = JZDrumHitType::Medium;
    else if (outVelocity < 0.92f) outHitType = JZDrumHitType::Hard;
    else outHitType = JZDrumHitType::Rimshot;

    return true;
  }

  return false;
}

bool JZDrumReplacerEngine::ProcessAudio(
  const std::vector<float>& acousticDrumTrack,
  std::vector<float>& outReplacedAudio,
  std::vector<JZTriggeredSampleEvent>& outTriggerEvents)
{
  outTriggerEvents.clear();
  outReplacedAudio.assign(acousticDrumTrack.size(), 0.0f);

  if (acousticDrumTrack.empty())
  {
    return false;
  }

  float threshLinear = std::pow(10.0f, mThresholdDb / 20.0f);
  size_t retriggerSamples = static_cast<size_t>(mRetriggerIntervalMs * 0.001f * mSampleRate);
  size_t lastHitSample = 0;

  for (size_t i = 2; i < acousticDrumTrack.size() - 2; ++i)
  {
    if ((i - lastHitSample) >= retriggerSamples || lastHitSample == 0)
    {
      float vel = 1.0f;
      JZDrumHitType hitType = JZDrumHitType::Hard;

      if (DetectPhaseLockedHit(acousticDrumTrack, i, threshLinear, vel, hitType))
      {
        float peakDb = 20.0f * std::log10(std::max(0.00001f, std::abs(acousticDrumTrack[i])));
        outTriggerEvents.emplace_back(i, vel, hitType, peakDb);
        lastHitSample = i;

        // Choose replacement sample layer
        const std::vector<float>* repSample = &mSampleHard;
        if (hitType == JZDrumHitType::Soft) repSample = &mSampleSoft;
        else if (hitType == JZDrumHitType::Medium) repSample = &mSampleMed;
        else if (hitType == JZDrumHitType::Rimshot) repSample = &mSampleRim;

        // Mix phase-aligned sample into output
        for (size_t s = 0; s < repSample->size() && (i + s) < outReplacedAudio.size(); ++s)
        {
          outReplacedAudio[i + s] += (*repSample)[s] * vel;
        }
      }
    }
  }

  // Apply dry / wet blending
  float wet = mDryWetMix / 100.0f;
  float dry = 1.0f - wet;

  for (size_t i = 0; i < outReplacedAudio.size(); ++i)
  {
    outReplacedAudio[i] = std::clamp(acousticDrumTrack[i] * dry + outReplacedAudio[i] * wet, -1.0f, 1.0f);
  }

  return true;
}
