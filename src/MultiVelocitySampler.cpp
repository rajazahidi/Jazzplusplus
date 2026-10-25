//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Multi-Velocity Zone Sampler Instrument Implementation
//*****************************************************************************

#include "MultiVelocitySampler.h"
#include <cstdint>

JZMultiVelocitySampler::JZMultiVelocitySampler(float sampleRate, int maxPolyphony)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f),
    mMaxPolyphony(std::max(4, std::min(64, maxPolyphony)))
{
  mVoices.resize(mMaxPolyphony);
}

void JZMultiVelocitySampler::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
  }
}

int JZMultiVelocitySampler::AddZone(const JZSampleZone& zone)
{
  mZones.push_back(zone);
  mZones.back().zoneId = (int)mZones.size() - 1;
  return mZones.back().zoneId;
}

void JZMultiVelocitySampler::ClearZones()
{
  AllNotesOff();
  mZones.clear();
}

int JZMultiVelocitySampler::GetActiveVoiceCount() const
{
  int count = 0;
  for (const auto& v : mVoices)
  {
    if (v.active) count++;
  }
  return count;
}

int JZMultiVelocitySampler::FindBestMatchingZone(int note, int velocity) const
{
  for (size_t i = 0; i < mZones.size(); ++i)
  {
    const auto& z = mZones[i];
    if (note >= z.lowKey && note <= z.highKey &&
        velocity >= z.lowVelocity && velocity <= z.highVelocity)
    {
      return (int)i;
    }
  }
  return -1;
}

int JZMultiVelocitySampler::AllocateVoice()
{
  // 1. Look for free voice
  for (int i = 0; i < mMaxPolyphony; ++i)
  {
    if (!mVoices[i].active) return i;
  }

  // 2. Look for voice in release phase with lowest envelope level
  int bestIdx = 0;
  float minLevel = 999.0f;
  for (int i = 0; i < mMaxPolyphony; ++i)
  {
    if (mVoices[i].envPhase == ENV_RELEASE && mVoices[i].envLevel < minLevel)
    {
      minLevel = mVoices[i].envLevel;
      bestIdx = i;
    }
  }
  return bestIdx;
}

void JZMultiVelocitySampler::NoteOn(int note, int velocity)
{
  if (velocity <= 0)
  {
    NoteOff(note);
    return;
  }

  int zoneIdx = FindBestMatchingZone(note, velocity);
  if (zoneIdx < 0) return;

  const auto& zone = mZones[zoneIdx];
  if (zone.sampleDataL.empty()) return;

  int vIdx = AllocateVoice();
  auto& voice = mVoices[vIdx];

  voice.active = true;
  voice.note = note;
  voice.velocity = velocity;
  voice.zoneIndex = zoneIdx;
  voice.playhead = 0.0;

  // Speed = 2^((note - rootKey) / 12)
  voice.speed = std::pow(2.0, (double)(note - zone.rootKey) / 12.0);

  voice.envPhase = ENV_ATTACK;
  voice.envLevel = 0.0f;
  voice.filterStateL = 0.0f;
  voice.filterStateR = 0.0f;
}

void JZMultiVelocitySampler::NoteOff(int note)
{
  for (auto& v : mVoices)
  {
    if (v.active && v.note == note && v.envPhase != ENV_RELEASE)
    {
      v.envPhase = ENV_RELEASE;
    }
  }
}

void JZMultiVelocitySampler::AllNotesOff()
{
  for (auto& v : mVoices)
  {
    v.active = false;
    v.envPhase = ENV_IDLE;
    v.envLevel = 0.0f;
  }
}

void JZMultiVelocitySampler::Process(float* outL, float* outR, int numSamples)
{
  if (!outL || numSamples <= 0) return;

  for (int s = 0; s < numSamples; ++s)
  {
    float mixL = 0.0f;
    float mixR = 0.0f;

    for (auto& v : mVoices)
    {
      if (!v.active || v.zoneIndex < 0 || v.zoneIndex >= (int)mZones.size()) continue;

      const auto& zone = mZones[v.zoneIndex];
      int sampleCount = (int)zone.sampleDataL.size();
      if (sampleCount == 0) { v.active = false; continue; }

      // 1. Envelope Update
      float attStep = 1000.0f / (std::max(0.1f, zone.attackMs) * mSampleRate);
      float decStep = 1000.0f / (std::max(0.1f, zone.decayMs) * mSampleRate);
      float relStep = 1000.0f / (std::max(0.1f, zone.releaseMs) * mSampleRate);

      if (v.envPhase == ENV_ATTACK)
      {
        v.envLevel += attStep;
        if (v.envLevel >= 1.0f)
        {
          v.envLevel = 1.0f;
          v.envPhase = ENV_DECAY;
        }
      }
      else if (v.envPhase == ENV_DECAY)
      {
        v.envLevel -= decStep;
        if (v.envLevel <= zone.sustainLevel)
        {
          v.envLevel = zone.sustainLevel;
          v.envPhase = ENV_SUSTAIN;
        }
      }
      else if (v.envPhase == ENV_RELEASE)
      {
        v.envLevel -= relStep;
        if (v.envLevel <= 0.0001f)
        {
          v.envLevel = 0.0f;
          v.active = false;
          continue;
        }
      }

      // 2. Linear Interpolated Sample Read
      int idx0 = (int)v.playhead;
      int idx1 = idx0 + 1;
      float frac = (float)(v.playhead - idx0);

      if (idx0 >= sampleCount - 1)
      {
        v.active = false;
        continue;
      }

      float s0L = zone.sampleDataL[idx0];
      float s1L = (idx1 < sampleCount) ? zone.sampleDataL[idx1] : 0.0f;
      float sampL = s0L + frac * (s1L - s0L);

      float sampR = sampL;
      if (!zone.sampleDataR.empty())
      {
        float s0R = zone.sampleDataR[idx0];
        float s1R = (idx1 < (int)zone.sampleDataR.size()) ? zone.sampleDataR[idx1] : 0.0f;
        sampR = s0R + frac * (s1R - s0R);
      }

      // 3. Filter processing (1-pole low pass filter with envelope/velocity scaling)
      float velScale = (float)v.velocity / 127.0f;
      float cutoff = std::min(mSampleRate * 0.45f, zone.filterCutoffHz * (0.3f + 0.7f * velScale));
      float alpha = 2.0f * 3.14159265f * cutoff / mSampleRate;
      alpha = std::max(0.01f, std::min(0.99f, alpha));

      v.filterStateL += alpha * (sampL - v.filterStateL);
      v.filterStateR += alpha * (sampR - v.filterStateR);

      float outSampL = v.filterStateL * v.envLevel * velScale;
      float outSampR = v.filterStateR * v.envLevel * velScale;

      mixL += outSampL;
      mixR += outSampR;

      // 4. Advance playhead
      v.playhead += v.speed;
    }

    outL[s] = mixL;
    if (outR) outR[s] = mixR;
  }
}

void JZMultiVelocitySampler::LoadAcousticPianoKit()
{
  ClearZones();

  // Zone 1: Pianissimo / Soft (Vel 1-45)
  JZSampleZone softZone;
  softZone.name = "ConcertGrand_Pianissimo";
  softZone.rootKey = 60;
  softZone.lowKey = 21; // A0
  softZone.highKey = 108; // C8
  softZone.lowVelocity = 1;
  softZone.highVelocity = 45;
  softZone.attackMs = 3.0f;
  softZone.decayMs = 800.0f;
  softZone.sustainLevel = 0.4f;
  softZone.releaseMs = 300.0f;
  softZone.filterCutoffHz = 4000.0f;

  // Synthesize soft strike harmonic waveform
  int sampleLen = (int)(mSampleRate * 1.5f);
  softZone.sampleDataL.resize(sampleLen);
  float f0 = 261.63f; // C4
  for (int i = 0; i < sampleLen; ++i)
  {
    float t = (float)i / mSampleRate;
    float decay = std::exp(-2.5f * t);
    // Soft tone: mostly fundamental and 2nd harmonic
    float val = (std::sin(2.0f * 3.14159f * f0 * t) + 0.3f * std::sin(4.0f * 3.14159f * f0 * t)) * decay;
    softZone.sampleDataL[i] = val * 0.7f;
  }
  softZone.sampleDataR = softZone.sampleDataL;
  AddZone(softZone);

  // Zone 2: Mezzo-Forte / Medium (Vel 46-95)
  JZSampleZone medZone;
  medZone.name = "ConcertGrand_MezzoForte";
  medZone.rootKey = 60;
  medZone.lowKey = 21;
  medZone.highKey = 108;
  medZone.lowVelocity = 46;
  medZone.highVelocity = 95;
  medZone.attackMs = 2.0f;
  medZone.decayMs = 1200.0f;
  medZone.sustainLevel = 0.6f;
  medZone.releaseMs = 350.0f;
  medZone.filterCutoffHz = 9000.0f;

  medZone.sampleDataL.resize(sampleLen);
  for (int i = 0; i < sampleLen; ++i)
  {
    float t = (float)i / mSampleRate;
    float decay = std::exp(-2.0f * t);
    // Richer harmonics
    float val = (std::sin(2.0f * 3.14159f * f0 * t) +
                 0.6f * std::sin(4.0f * 3.14159f * f0 * t) +
                 0.4f * std::sin(6.0f * 3.14159f * f0 * t)) * decay;
    medZone.sampleDataL[i] = val * 0.8f;
  }
  medZone.sampleDataR = medZone.sampleDataL;
  AddZone(medZone);

  // Zone 3: Fortissimo / Hard Strike (Vel 96-127)
  JZSampleZone hardZone;
  hardZone.name = "ConcertGrand_Fortissimo";
  hardZone.rootKey = 60;
  hardZone.lowKey = 21;
  hardZone.highKey = 108;
  hardZone.lowVelocity = 96;
  hardZone.highVelocity = 127;
  hardZone.attackMs = 1.0f;
  hardZone.decayMs = 1500.0f;
  hardZone.sustainLevel = 0.75f;
  hardZone.releaseMs = 400.0f;
  hardZone.filterCutoffHz = 16000.0f;

  hardZone.sampleDataL.resize(sampleLen);
  for (int i = 0; i < sampleLen; ++i)
  {
    float t = (float)i / mSampleRate;
    float decay = std::exp(-1.8f * t);
    // Bright punchy piano string resonance
    float val = (std::sin(2.0f * 3.14159f * f0 * t) +
                 0.8f * std::sin(4.0f * 3.14159f * f0 * t) +
                 0.6f * std::sin(6.0f * 3.14159f * f0 * t) +
                 0.4f * std::sin(8.0f * 3.14159f * f0 * t) +
                 0.25f * std::sin(10.0f * 3.14159f * f0 * t)) * decay;
    hardZone.sampleDataL[i] = val * 0.9f;
  }
  hardZone.sampleDataR = hardZone.sampleDataL;
  AddZone(hardZone);
}

void JZMultiVelocitySampler::LoadStudioDrumKit()
{
  ClearZones();

  // Kick (Note 36)
  JZSampleZone kick;
  kick.name = "Studio_Kick";
  kick.rootKey = 36;
  kick.lowKey = 36;
  kick.highKey = 36;
  kick.lowVelocity = 1;
  kick.highVelocity = 127;
  kick.attackMs = 1.0f;
  kick.decayMs = 300.0f;
  kick.sustainLevel = 0.0f;
  kick.releaseMs = 50.0f;

  int kickLen = (int)(mSampleRate * 0.4f);
  kick.sampleDataL.resize(kickLen);
  for (int i = 0; i < kickLen; ++i)
  {
    float t = (float)i / mSampleRate;
    float pitchEnv = 150.0f * std::exp(-35.0f * t) + 48.0f;
    float ampEnv = std::exp(-12.0f * t);
    kick.sampleDataL[i] = std::sin(2.0f * 3.14159f * pitchEnv * t) * ampEnv;
  }
  kick.sampleDataR = kick.sampleDataL;
  AddZone(kick);

  // Snare (Note 38)
  JZSampleZone snare;
  snare.name = "Studio_Snare";
  snare.rootKey = 38;
  snare.lowKey = 38;
  snare.highKey = 38;
  snare.lowVelocity = 1;
  snare.highVelocity = 127;
  snare.attackMs = 1.0f;
  snare.decayMs = 250.0f;
  snare.sustainLevel = 0.0f;
  snare.releaseMs = 40.0f;

  int snareLen = (int)(mSampleRate * 0.35f);
  snare.sampleDataL.resize(snareLen);
  uint32_t seed = 42;
  for (int i = 0; i < snareLen; ++i)
  {
    float t = (float)i / mSampleRate;
    seed = seed * 1664525u + 1013904223u;
    float noise = ((float)(seed & 0xFFFF) / 32768.0f) - 1.0f;
    float tone = std::sin(2.0f * 3.14159f * 185.0f * t);
    float ampEnv = std::exp(-15.0f * t);
    snare.sampleDataL[i] = (tone * 0.5f + noise * 0.5f) * ampEnv;
  }
  snare.sampleDataR = snare.sampleDataL;
  AddZone(snare);
}
