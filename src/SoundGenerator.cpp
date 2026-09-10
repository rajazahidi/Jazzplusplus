//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Copyright (C) 2026 Jazz++ Developers
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//*****************************************************************************

#include "SoundGenerator.h"
#ifndef JAZZ_UNIT_TEST
#include "Sample.h"
#include "Track.h"
#include "Song.h"
#include "Events.h"

#include <wx/sound.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#endif

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <algorithm>

namespace
{
  const float kPi = 3.14159265358979323846f;

  inline float ClampFloat(float val, float minVal, float maxVal)
  {
    if (val < minVal) return minVal;
    if (val > maxVal) return maxVal;
    return val;
  }

  inline short FloatToShort(float val)
  {
    float clamped = ClampFloat(val, -1.0f, 1.0f);
    return static_cast<short>(clamped * 32767.0f);
  }

  inline float RandomFloat()
  {
    return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
  }

  inline float RandomRange(float minVal, float maxVal)
  {
    return minVal + RandomFloat() * (maxVal - minVal);
  }
}

// ============================================================================
// 1. Procedural Retro Game Sound Effects Implementation
// ============================================================================

JZRetroSFXParams::JZRetroSFXParams()
  : waveform(eWaveSquare),
    startFrequency(440.0f),
    endFrequency(220.0f),
    duration(0.25f),
    attack(0.005f),
    decay(0.24f),
    dutyCycle(0.5f),
    frequencyJumpTime(0.0f),
    frequencyJumpFactor(1.0f),
    vibratoDepth(0.0f),
    vibratoSpeed(0.0f),
    volume(0.85f)
{
}

JZRetroSFXParams JZRetroSFXGenerator::GetPreset(JZRetroSFXType type)
{
  JZRetroSFXParams p;

  switch (type)
  {
    case eSFXLaser:
      p.waveform = eWaveSawtooth;
      p.startFrequency = 1400.0f;
      p.endFrequency = 80.0f;
      p.duration = 0.18f;
      p.attack = 0.002f;
      p.decay = 0.17f;
      p.volume = 0.85f;
      break;

    case eSFXExplosion:
      p.waveform = eWaveNoise;
      p.startFrequency = 250.0f;
      p.endFrequency = 30.0f;
      p.duration = 0.65f;
      p.attack = 0.005f;
      p.decay = 0.62f;
      p.volume = 0.90f;
      break;

    case eSFXPowerUp:
      p.waveform = eWaveTriangle;
      p.startFrequency = 220.0f;
      p.endFrequency = 880.0f;
      p.duration = 0.38f;
      p.attack = 0.01f;
      p.decay = 0.35f;
      p.frequencyJumpTime = 0.45f;
      p.frequencyJumpFactor = 1.33484f; // Ascend by a perfect fourth
      p.volume = 0.85f;
      break;

    case eSFXCoin:
      p.waveform = eWaveSquare;
      p.dutyCycle = 0.5f;
      p.startFrequency = 987.77f; // B5
      p.endFrequency = 987.77f;
      p.duration = 0.35f;
      p.attack = 0.005f;
      p.decay = 0.25f;
      p.frequencyJumpTime = 0.10f;
      p.frequencyJumpFactor = 1.33484f; // Step to E6 (1318.5 Hz)
      p.volume = 0.80f;
      break;

    case eSFXJump:
      p.waveform = eWaveSquare;
      p.dutyCycle = 0.4f;
      p.startFrequency = 160.0f;
      p.endFrequency = 640.0f;
      p.duration = 0.22f;
      p.attack = 0.005f;
      p.decay = 0.20f;
      p.volume = 0.85f;
      break;

    case eSFXHurt:
      p.waveform = eWaveSawtooth;
      p.startFrequency = 380.0f;
      p.endFrequency = 70.0f;
      p.duration = 0.20f;
      p.attack = 0.002f;
      p.decay = 0.18f;
      p.vibratoDepth = 45.0f;
      p.vibratoSpeed = 30.0f;
      p.volume = 0.85f;
      break;

    case eSFXSweep:
      p.waveform = eWaveSine;
      p.startFrequency = 880.0f;
      p.endFrequency = 180.0f;
      p.duration = 0.45f;
      p.attack = 0.02f;
      p.decay = 0.40f;
      p.vibratoDepth = 50.0f;
      p.vibratoSpeed = 14.0f;
      p.volume = 0.85f;
      break;

    case eSFXRandom:
    default:
      p.waveform = static_cast<JZWaveform>(rand() % 5);
      p.startFrequency = RandomRange(150.0f, 1500.0f);
      p.endFrequency = RandomRange(60.0f, 1200.0f);
      p.duration = RandomRange(0.12f, 0.50f);
      p.attack = RandomRange(0.001f, 0.03f);
      p.decay = p.duration - p.attack;
      p.dutyCycle = RandomRange(0.2f, 0.8f);
      if (rand() % 2 == 0)
      {
        p.frequencyJumpTime = RandomRange(0.1f, 0.5f);
        p.frequencyJumpFactor = RandomRange(0.6f, 1.6f);
      }
      if (rand() % 3 == 0)
      {
        p.vibratoDepth = RandomRange(10.0f, 50.0f);
        p.vibratoSpeed = RandomRange(5.0f, 25.0f);
      }
      p.volume = 0.85f;
      break;
  }

  return p;
}

std::vector<short> JZRetroSFXGenerator::Generate(
  const JZRetroSFXParams& params,
  int sampleRate)
{
  int totalSamples = static_cast<int>(params.duration * sampleRate);
  if (totalSamples <= 0)
  {
    totalSamples = sampleRate / 10;
  }

  std::vector<short> output(totalSamples, 0);
  float phase = 0.0f;
  float filterState = 0.0f;

  for (int i = 0; i < totalSamples; ++i)
  {
    float t = static_cast<float>(i) / static_cast<float>(sampleRate);
    float tFrac = t / params.duration;

    // 1. Calculate base frequency with sweep
    float curFreq;
    if (params.startFrequency > 0.0f && params.endFrequency > 0.0f)
    {
      // Exponential frequency sweep
      curFreq = params.startFrequency * std::pow(params.endFrequency / params.startFrequency, tFrac);
    }
    else
    {
      curFreq = params.startFrequency + (params.endFrequency - params.startFrequency) * tFrac;
    }

    // 2. Frequency jump check
    if (params.frequencyJumpTime > 0.0f && tFrac >= params.frequencyJumpTime)
    {
      curFreq *= params.frequencyJumpFactor;
    }

    // 3. Vibrato modulation
    if (params.vibratoDepth > 0.0f && params.vibratoSpeed > 0.0f)
    {
      curFreq += params.vibratoDepth * std::sin(2.0f * kPi * params.vibratoSpeed * t);
    }

    curFreq = ClampFloat(curFreq, 10.0f, sampleRate * 0.45f);

    // 4. Update phase
    phase += curFreq / static_cast<float>(sampleRate);
    while (phase >= 1.0f)
    {
      phase -= 1.0f;
    }

    // 5. Generate raw oscillator wave
    float raw = 0.0f;
    switch (params.waveform)
    {
      case eWaveSquare:
        raw = (phase < params.dutyCycle) ? 1.0f : -1.0f;
        break;

      case eWaveSawtooth:
        raw = 2.0f * phase - 1.0f;
        break;

      case eWaveSine:
        raw = std::sin(2.0f * kPi * phase);
        break;

      case eWaveTriangle:
        raw = 4.0f * std::fabs(phase - 0.5f) - 1.0f;
        break;

      case eWaveNoise:
      {
        float noise = RandomRange(-1.0f, 1.0f);
        // Apply single-pole low-pass filter tracking the frequency
        float cutoffNorm = ClampFloat(curFreq * 4.0f / sampleRate, 0.01f, 0.99f);
        filterState += cutoffNorm * (noise - filterState);
        raw = filterState;
        break;
      }
    }

    // 6. Calculate amplitude envelope
    float env = 1.0f;
    if (t < params.attack && params.attack > 0.0f)
    {
      env = t / params.attack;
    }
    else
    {
      float decayTime = params.duration - params.attack;
      if (decayTime > 0.0f)
      {
        float elapsedDecay = t - params.attack;
        env = ClampFloat(1.0f - (elapsedDecay / decayTime), 0.0f, 1.0f);
        // Exponential fade-out curve for natural chiptune sound
        env = env * env;
      }
    }

    output[i] = FloatToShort(raw * env * params.volume);
  }

  return output;
}

// ============================================================================
// 2. Vintage Analog Drum Synthesizer Implementation
// ============================================================================

JZSynthDrumParams::JZSynthDrumParams()
  : baseFrequency(55.0f),
    pitchDropHz(110.0f),
    pitchDropDecay(0.04f),
    duration(0.45f),
    attack(0.002f),
    decay(0.35f),
    toneMix(0.9f),
    clickTransient(0.3f),
    noiseFilterCutoff(2500.0f),
    noiseFilterQ(1.5f),
    overdrive(0.2f),
    volume(0.9f)
{
}

JZSynthDrumParams JZSynthDrumGenerator::GetPreset(JZSynthDrumType type)
{
  JZSynthDrumParams p;

  switch (type)
  {
    case eDrum808Kick:
      p.baseFrequency = 48.0f;
      p.pitchDropHz = 125.0f;
      p.pitchDropDecay = 0.038f;
      p.duration = 0.55f;
      p.attack = 0.002f;
      p.decay = 0.48f;
      p.toneMix = 0.96f;
      p.clickTransient = 0.35f;
      p.overdrive = 0.30f;
      p.volume = 0.92f;
      break;

    case eDrum808Snare:
      p.baseFrequency = 175.0f;
      p.pitchDropHz = 70.0f;
      p.pitchDropDecay = 0.025f;
      p.duration = 0.28f;
      p.attack = 0.001f;
      p.decay = 0.22f;
      p.toneMix = 0.35f;
      p.clickTransient = 0.25f;
      p.noiseFilterCutoff = 3800.0f;
      p.noiseFilterQ = 1.8f;
      p.overdrive = 0.20f;
      p.volume = 0.90f;
      break;

    case eDrumHiHatClosed:
      p.baseFrequency = 350.0f;
      p.pitchDropHz = 0.0f;
      p.pitchDropDecay = 0.01f;
      p.duration = 0.07f;
      p.attack = 0.001f;
      p.decay = 0.05f;
      p.toneMix = 0.0f;
      p.clickTransient = 0.15f;
      p.noiseFilterCutoff = 7500.0f;
      p.noiseFilterQ = 2.0f;
      p.overdrive = 0.0f;
      p.volume = 0.80f;
      break;

    case eDrumHiHatOpen:
      p.baseFrequency = 350.0f;
      p.pitchDropHz = 0.0f;
      p.pitchDropDecay = 0.01f;
      p.duration = 0.42f;
      p.attack = 0.001f;
      p.decay = 0.38f;
      p.toneMix = 0.0f;
      p.clickTransient = 0.12f;
      p.noiseFilterCutoff = 7200.0f;
      p.noiseFilterQ = 2.0f;
      p.overdrive = 0.0f;
      p.volume = 0.80f;
      break;

    case eDrum808Clap:
      p.baseFrequency = 120.0f;
      p.pitchDropHz = 0.0f;
      p.pitchDropDecay = 0.01f;
      p.duration = 0.35f;
      p.attack = 0.001f;
      p.decay = 0.30f;
      p.toneMix = 0.0f;
      p.clickTransient = 0.0f;
      p.noiseFilterCutoff = 2400.0f;
      p.noiseFilterQ = 1.4f;
      p.overdrive = 0.15f;
      p.volume = 0.90f;
      break;

    case eDrum808Tom:
      p.baseFrequency = 95.0f;
      p.pitchDropHz = 50.0f;
      p.pitchDropDecay = 0.06f;
      p.duration = 0.35f;
      p.attack = 0.003f;
      p.decay = 0.30f;
      p.toneMix = 0.90f;
      p.clickTransient = 0.20f;
      p.overdrive = 0.20f;
      p.volume = 0.88f;
      break;
  }

  return p;
}

std::vector<short> JZSynthDrumGenerator::Generate(
  const JZSynthDrumParams& params,
  int sampleRate)
{
  int totalSamples = static_cast<int>(params.duration * sampleRate);
  if (totalSamples <= 0)
  {
    totalSamples = sampleRate / 10;
  }

  std::vector<short> output(totalSamples, 0);
  float sinePhase = 0.0f;

  // Simple bandpass filter state for noise
  float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
  float x1 = 0.0f, x2 = 0.0f, y1 = 0.0f, y2 = 0.0f;

  // Calculate Bandpass filter coefficients for noise component
  {
    float w0 = 2.0f * kPi * params.noiseFilterCutoff / static_cast<float>(sampleRate);
    w0 = ClampFloat(w0, 0.01f, 3.1f);
    float alpha = std::sin(w0) / (2.0f * std::max(0.1f, params.noiseFilterQ));
    float a0 = 1.0f + alpha;

    b0 = alpha / a0;
    b1 = 0.0f;
    b2 = -alpha / a0;
    a1 = (-2.0f * std::cos(w0)) / a0;
    a2 = (1.0f - alpha) / a0;
  }

  for (int i = 0; i < totalSamples; ++i)
  {
    float t = static_cast<float>(i) / static_cast<float>(sampleRate);

    // 1. Pitch sweep for tonal body
    float curPitch = params.baseFrequency +
      params.pitchDropHz * std::exp(-t / std::max(0.001f, params.pitchDropDecay));
    curPitch = ClampFloat(curPitch, 15.0f, sampleRate * 0.45f);

    sinePhase += curPitch / static_cast<float>(sampleRate);
    while (sinePhase >= 1.0f)
    {
      sinePhase -= 1.0f;
    }
    float tone = std::sin(2.0f * kPi * sinePhase);

    // 2. Filtered noise layer
    float whiteNoise = RandomRange(-1.0f, 1.0f);
    float filteredNoise = b0 * whiteNoise + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1;
    x1 = whiteNoise;
    y2 = y1;
    y1 = filteredNoise;

    // 3. Transient click (first 4 ms)
    float click = 0.0f;
    if (t < 0.004f && params.clickTransient > 0.0f)
    {
      click = (1.0f - t / 0.004f) * RandomRange(-1.0f, 1.0f);
    }

    // 4. Clap burst logic: 3 micro-bursts for 808 clap simulation
    float clapBurstMod = 1.0f;
    if (params.toneMix < 0.05f && params.baseFrequency > 100.0f && params.baseFrequency < 140.0f)
    {
      // Clap mode
      if (t < 0.010f)
        clapBurstMod = 1.0f - (t / 0.010f);
      else if (t < 0.013f)
        clapBurstMod = 0.05f;
      else if (t < 0.023f)
        clapBurstMod = 1.0f - ((t - 0.013f) / 0.010f);
      else if (t < 0.026f)
        clapBurstMod = 0.05f;
      else if (t < 0.036f)
        clapBurstMod = 1.0f - ((t - 0.026f) / 0.010f);
    }

    // 5. Mix tone and noise
    float mixed = tone * params.toneMix +
                  filteredNoise * (1.0f - params.toneMix) * clapBurstMod +
                  click * params.clickTransient;

    // 6. Amplitude envelope
    float env = 1.0f;
    if (t < params.attack && params.attack > 0.0f)
    {
      env = t / params.attack;
    }
    else
    {
      float decayT = std::max(0.001f, params.decay);
      env = std::exp(-(t - params.attack) * 4.0f / decayT);
    }

    float sampleVal = mixed * env * params.volume;

    // 7. Soft saturation / analog overdrive
    if (params.overdrive > 0.0f)
    {
      float drive = 1.0f + params.overdrive * 2.0f;
      sampleVal = std::tanh(sampleVal * drive);
    }

    output[i] = FloatToShort(sampleVal);
  }

  return output;
}

// ============================================================================
// 3. Audio & Sample Utilities Implementation
// ============================================================================

#pragma pack(push, 1)
struct WavHeader
{
  char riff[4];        // "RIFF"
  unsigned int fileSize;
  char wave[4];        // "WAVE"
  char fmt[4];         // "fmt "
  unsigned int fmtSize;// 16 for PCM
  unsigned short audioFormat; // 1 = PCM
  unsigned short numChannels; // 1 = mono, 2 = stereo
  unsigned int sampleRate;
  unsigned int byteRate;
  unsigned short blockAlign;
  unsigned short bitsPerSample; // 16
  char data[4];        // "data"
  unsigned int dataSize;
};
#pragma pack(pop)

bool JZSoundIO::SaveWav(
  const std::string& filePath,
  const std::vector<short>& samples,
  int sampleRate,
  int channels)
{
  if (samples.empty() || sampleRate <= 0 || channels <= 0)
  {
    return false;
  }

  std::ofstream outFile(filePath.c_str(), std::ios::binary | std::ios::trunc);
  if (!outFile.is_open())
  {
    return false;
  }

  WavHeader header;
  header.riff[0] = 'R'; header.riff[1] = 'I'; header.riff[2] = 'F'; header.riff[3] = 'F';
  header.wave[0] = 'W'; header.wave[1] = 'A'; header.wave[2] = 'V'; header.wave[3] = 'E';
  header.fmt[0]  = 'f'; header.fmt[1]  = 'm'; header.fmt[2]  = 't'; header.fmt[3]  = ' ';
  header.fmtSize = 16;
  header.audioFormat = 1; // PCM
  header.numChannels = static_cast<unsigned short>(channels);
  header.sampleRate = static_cast<unsigned int>(sampleRate);
  header.bitsPerSample = 16;
  header.blockAlign = static_cast<unsigned short>(channels * (header.bitsPerSample / 8));
  header.byteRate = header.sampleRate * header.blockAlign;
  header.data[0] = 'd'; header.data[1] = 'a'; header.data[2] = 't'; header.data[3] = 'a';
  header.dataSize = static_cast<unsigned int>(samples.size() * sizeof(short));
  header.fileSize = sizeof(WavHeader) - 8 + header.dataSize;

  outFile.write(reinterpret_cast<const char*>(&header), sizeof(WavHeader));
  outFile.write(reinterpret_cast<const char*>(samples.data()), header.dataSize);

  return outFile.good();
}

bool JZSoundIO::PlayWav(const std::string& filePath)
{
#ifndef JAZZ_UNIT_TEST
#if wxUSE_SOUND
  wxSound sound(wxString::FromUTF8(filePath.c_str()));
  if (sound.IsOk())
  {
    return sound.Play(wxSOUND_ASYNC);
  }
#endif
#endif
  (void)filePath;
  return false;
}

#ifndef JAZZ_UNIT_TEST
bool JZSoundIO::AssignToSample(
  JZSample& destSample,
  const std::vector<short>& samples,
  const std::string& label,
  int sampleRate)
{
  wxString tempDir = wxStandardPaths::Get().GetTempDir();
  wxFileName tempWav(tempDir, wxString::FromUTF8(label.c_str()), "wav");
  std::string tempPath = std::string(tempWav.GetFullPath().mb_str());

  if (!SaveWav(tempPath, samples, sampleRate, 1))
  {
    return false;
  }

  destSample.SetFileName(tempPath);
  destSample.SetLabel(label);
  destSample.LoadWav();
  return true;
}

bool JZSoundIO::InsertNoteTrigger(
  JZTrack* pTrack,
  long clock,
  int midiKey,
  int velocity,
  int durationTicks)
{
  if (!pTrack)
  {
    return false;
  }

  // Create KeyOn event
  JZKeyOnEvent* pKeyOn = new JZKeyOnEvent(
    clock,
    0, // Channel
    midiKey,
    velocity,
    durationTicks);

  pTrack->Put(pKeyOn);
  return true;
}
#else
bool JZSoundIO::AssignToSample(
  JZSample&,
  const std::vector<short>&,
  const std::string&,
  int)
{
  return true;
}

bool JZSoundIO::InsertNoteTrigger(
  JZTrack*,
  long,
  int,
  int,
  int)
{
  return true;
}
#endif
