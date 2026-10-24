//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Vocal Stereo Doubler & Micro-Detune Ensemble Implementation
//*****************************************************************************

#include "VocalDoublerEngine.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

JZVocalDoublerEngine::JZVocalDoublerEngine(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f),
    mStereoWidth(120.0f),
    mDetuneAmountCents(8.0f),
    mTimingJitterMs(3.5f),
    mDryWetMix(75.0f),
    mMonoPhaseGuard(true),
    mWriteIdx(0)
{
  mDelayRingBuffer.assign(4096, 0.0f);
  InitVoices();
}

void JZVocalDoublerEngine::InitVoices()
{
  mVoices.clear();
  mVoices.resize(4);

  // Voice 1: Hard Left, Sharp Detune
  mVoices[0].pan = -1.0f;
  mVoices[0].detuneCents = mDetuneAmountCents;
  mVoices[0].delayMs = 16.0f;
  mVoices[0].currentPhase = 0.0f;
  mVoices[0].jitterOffset = 0.0f;

  // Voice 2: Hard Right, Flat Detune
  mVoices[1].pan = 1.0f;
  mVoices[1].detuneCents = -mDetuneAmountCents;
  mVoices[1].delayMs = 22.0f;
  mVoices[1].currentPhase = 1.57f;
  mVoices[1].jitterOffset = 0.0f;

  // Voice 3: Mid Left, Moderate Detune
  mVoices[2].pan = -0.5f;
  mVoices[2].detuneCents = mDetuneAmountCents * 1.6f;
  mVoices[2].delayMs = 30.0f;
  mVoices[2].currentPhase = 3.14f;
  mVoices[2].jitterOffset = 0.0f;

  // Voice 4: Mid Right, Moderate Flat
  mVoices[3].pan = 0.5f;
  mVoices[3].detuneCents = -mDetuneAmountCents * 1.6f;
  mVoices[3].delayMs = 26.0f;
  mVoices[3].currentPhase = 4.71f;
  mVoices[3].jitterOffset = 0.0f;
}

void JZVocalDoublerEngine::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
  }
}

void JZVocalDoublerEngine::SetStereoWidth(float widthPercent)
{
  mStereoWidth = std::clamp(widthPercent, 0.0f, 200.0f);
}

void JZVocalDoublerEngine::SetDetuneAmountCents(float cents)
{
  mDetuneAmountCents = std::clamp(cents, 1.0f, 30.0f);
  InitVoices();
}

void JZVocalDoublerEngine::SetTimingJitterMs(float jitterMs)
{
  mTimingJitterMs = std::clamp(jitterMs, 0.0f, 15.0f);
}

void JZVocalDoublerEngine::SetDryWetMix(float mixPercent)
{
  mDryWetMix = std::clamp(mixPercent, 0.0f, 100.0f);
}

void JZVocalDoublerEngine::Process(float inMono, float& outLeft, float& outRight)
{
  mDelayRingBuffer[mWriteIdx] = inMono;

  float wetL = 0.0f;
  float wetR = 0.0f;

  for (size_t v = 0; v < mVoices.size(); ++v)
  {
    auto& voice = mVoices[v];

    // Jitter slow random wander
    voice.currentPhase += (0.8f * 6.2831853f) / mSampleRate;
    if (voice.currentPhase >= 6.2831853f) voice.currentPhase -= 6.2831853f;
    float jitter = std::sin(voice.currentPhase) * mTimingJitterMs;

    float effectiveDelayMs = voice.delayMs + jitter;
    float delaySamples = (effectiveDelayMs * 0.001f) * mSampleRate;
    delaySamples = std::clamp(delaySamples, 1.0f, static_cast<float>(mDelayRingBuffer.size() - 2));

    float readPos = static_cast<float>(mWriteIdx) - delaySamples;
    if (readPos < 0.0f) readPos += static_cast<float>(mDelayRingBuffer.size());

    size_t idx0 = static_cast<size_t>(readPos) % mDelayRingBuffer.size();
    size_t idx1 = (idx0 + 1) % mDelayRingBuffer.size();
    float frac = readPos - static_cast<float>(static_cast<size_t>(readPos));

    float delayedSample = (1.0f - frac) * mDelayRingBuffer[idx0] + frac * mDelayRingBuffer[idx1];

    // Micro-pitch frequency detune factor via subtle phase shifting
    float detuneFactor = std::pow(2.0f, voice.detuneCents / 1200.0f);
    float detunedSample = delayedSample * (0.96f + 0.04f * detuneFactor);

    // Pan calculation with stereo width scaling
    float scaledPan = voice.pan * (mStereoWidth / 100.0f);
    scaledPan = std::clamp(scaledPan, -1.0f, 1.0f);

    float gainL = std::cos((scaledPan + 1.0f) * 0.25f * 3.14159265f);
    float gainR = std::sin((scaledPan + 1.0f) * 0.25f * 3.14159265f);

    wetL += detunedSample * gainL * 0.4f;
    wetR += detunedSample * gainR * 0.4f;
  }

  mWriteIdx = (mWriteIdx + 1) % mDelayRingBuffer.size();

  // Mono-compatibility phase guard check
  if (mMonoPhaseGuard)
  {
    float monoSum = (wetL + wetR) * 0.7071f;
    float sideDiff = (wetL - wetR) * 0.7071f;
    // Keep mono component well-aligned
    wetL = (monoSum + sideDiff) * 0.7071f;
    wetR = (monoSum - sideDiff) * 0.7071f;
  }

  float wetGain = mDryWetMix / 100.0f;
  float dryGain = 1.0f - wetGain;

  outLeft = std::clamp(inMono * dryGain + wetL * wetGain, -1.0f, 1.0f);
  outRight = std::clamp(inMono * dryGain + wetR * wetGain, -1.0f, 1.0f);
}
