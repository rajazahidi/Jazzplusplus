//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Copyright (C) 2026 Raja Zahidi
// Procedural Sound FX & Drum Synthesizer — Retro SFX, 808 Analog Drums
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>

class JZSample;
class JZTrack;
class JZSong;

// ============================================================================
// 1. Procedural Retro Game Sound Effects (8-bit / 16-bit Chiptune SFX)
// ============================================================================
enum JZRetroSFXType
{
  eSFXLaser = 0,
  eSFXExplosion,
  eSFXPowerUp,
  eSFXCoin,
  eSFXJump,
  eSFXHurt,
  eSFXSweep,
  eSFXRandom
};

enum JZWaveform
{
  eWaveSquare = 0,
  eWaveSawtooth,
  eWaveSine,
  eWaveTriangle,
  eWaveNoise
};

struct JZRetroSFXParams
{
  JZWaveform waveform;
  float startFrequency;      // Hz (e.g. 800.0f)
  float endFrequency;        // Hz (e.g. 100.0f)
  float duration;            // seconds (e.g. 0.25f)
  float attack;              // seconds (e.g. 0.005f)
  float decay;               // seconds (e.g. 0.2f)
  float dutyCycle;           // 0.1f to 0.9f for square wave (default 0.5f)
  float frequencyJumpTime;   // 0.0 to 1.0 (fraction of duration)
  float frequencyJumpFactor; // frequency multiplier at jump time (e.g. 1.33f)
  float vibratoDepth;        // Hz
  float vibratoSpeed;        // Hz
  float volume;              // 0.0 to 1.0

  JZRetroSFXParams();
};

class JZRetroSFXGenerator
{
  public:
    static JZRetroSFXParams GetPreset(JZRetroSFXType type);
    static std::vector<short> Generate(const JZRetroSFXParams& params, int sampleRate = 44100);
};

// ============================================================================
// 2. Vintage Analog Drum Synthesizer (808 / Vintage Drum Machine)
// ============================================================================
enum JZSynthDrumType
{
  eDrum808Kick = 0,
  eDrum808Snare,
  eDrumHiHatClosed,
  eDrumHiHatOpen,
  eDrum808Clap,
  eDrum808Tom
};

struct JZSynthDrumParams
{
  float baseFrequency;      // Hz
  float pitchDropHz;        // Hz to sweep downwards
  float pitchDropDecay;     // seconds for pitch sweep
  float duration;           // total duration in seconds
  float attack;             // attack time in seconds
  float decay;              // amplitude decay in seconds
  float toneMix;            // 0.0 (pure noise) to 1.0 (pure tone)
  float clickTransient;     // 0.0 to 1.0 transient click amount
  float noiseFilterCutoff;  // Hz filter cutoff for noise component
  float noiseFilterQ;       // Q factor
  float overdrive;          // 0.0 to 1.0 analog warmth
  float volume;             // 0.0 to 1.0

  JZSynthDrumParams();
};

class JZSynthDrumGenerator
{
  public:
    static JZSynthDrumParams GetPreset(JZSynthDrumType type);
    static std::vector<short> Generate(const JZSynthDrumParams& params, int sampleRate = 44100);
};

// ============================================================================
// 3. Audio & Sample Utilities
// ============================================================================
class JZSoundIO
{
  public:
    // Writes raw 16-bit mono or stereo PCM to a standard RIFF/WAVE file
    static bool SaveWav(
      const std::string& filePath,
      const std::vector<short>& samples,
      int sampleRate = 44100,
      int channels = 1);

    // Play WAV file asynchronously using wxSound
    static bool PlayWav(const std::string& filePath);

    // Insert or load samples into a JZSample in Jazz++
    static bool AssignToSample(
      JZSample& destSample,
      const std::vector<short>& samples,
      const std::string& label,
      int sampleRate = 44100);

    // Insert NoteOn event into a track at clock tick
    static bool InsertNoteTrigger(
      JZTrack* pTrack,
      long clock,
      int midiKey,
      int velocity = 100,
      int durationTicks = 120);
};
