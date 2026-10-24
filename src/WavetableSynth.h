//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Modern Wavetable Synthesizer & Stereo Unison Engine
//
// Advanced polyphonic wavetable synthesizer featuring 64-frame 2D wavetable
// morphing, 1 to 16 voice stereo unison supersaw spread, multi-mode resonant
// filter, and dual ADSR modulation envelopes.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

// Filter Type
enum JZFilterMode
{
  FILTER_LOWPASS = 0,
  FILTER_BANDPASS,
  FILTER_HIGHPASS
};

// Configuration for Wavetable Synth
struct JZWavetableSynthConfig
{
  float wavetablePosition;    // 0.0 to 63.0 (morphs between 64 wave frames)
  int unisonVoices;           // 1 to 16 unison voices
  float unisonDetune;         // 0.0 to 1.0 (detune spread)
  float unisonStereoSpread;   // 0.0 to 1.0 (pan width)
  
  // Filter
  JZFilterMode filterMode;
  float filterCutoffHz;       // 20 Hz to 20000 Hz
  float filterResonance;      // 0.0 to 0.95
  float filterEnvAmount;      // -1.0 to +1.0

  // Amp Envelope (ADSR)
  float ampAttackMs;
  float ampDecayMs;
  float ampSustainLevel;      // 0.0 to 1.0
  float ampReleaseMs;

  // Master
  float masterGainDB;         // -24.0 dB to +6.0 dB

  JZWavetableSynthConfig()
    : wavetablePosition(0.0f),
      unisonVoices(7),
      unisonDetune(0.25f),
      unisonStereoSpread(0.85f),
      filterMode(FILTER_LOWPASS),
      filterCutoffHz(4500.0f),
      filterResonance(0.35f),
      filterEnvAmount(0.5f),
      ampAttackMs(10.0f),
      ampDecayMs(350.0f),
      ampSustainLevel(0.65f),
      ampReleaseMs(250.0f),
      masterGainDB(-3.0f)
  {
  }
};

// Polyphonic voice state
struct JZWavetableVoice
{
  bool active;
  int note;
  int velocity;
  double phase[16];           // Unison sub-phases
  float envLevel;
  int envStage;               // 0: Idle, 1: Attack, 2: Decay, 3: Sustain, 4: Release
  float filterStateL1, filterStateL2;
  float filterStateR1, filterStateR2;

  JZWavetableVoice()
    : active(false), note(60), velocity(100), envLevel(0.0f), envStage(0),
      filterStateL1(0), filterStateL2(0), filterStateR1(0), filterStateR2(0)
  {
    for (int i = 0; i < 16; ++i) phase[i] = 0.0;
  }
};

// Wavetable Synth Class
class JZWavetableSynth
{
public:
  static const int NUM_FRAMES = 64;
  static const int FRAME_SIZE = 256;
  static const int MAX_POLYPHONY = 16;

  JZWavetableSynth(float sampleRate = 44100.0f);
  ~JZWavetableSynth() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  void SetConfig(const JZWavetableSynthConfig& config);
  const JZWavetableSynthConfig& GetConfig() const { return mConfig; }
  JZWavetableSynthConfig& GetConfig() { return mConfig; }

  void Reset();

  // MIDI Events
  void NoteOn(int note, int velocity);
  void NoteOff(int note);
  void AllNotesOff();

  // Process Stereo Audio Block
  void Process(float* outL, float* outR, int numSamples);

  // Active voice count
  int GetActiveVoiceCount() const;

  // Single-sample wavetable read helper with Hermite frame interpolation
  float SampleWavetable(double phase, float framePos) const;

private:
  float mSampleRate;
  JZWavetableSynthConfig mConfig;

  // 64 frames of 256 samples
  std::vector<float> mWavetableData;
  JZWavetableVoice mVoices[MAX_POLYPHONY];

  void GenerateDefaultWavetable();
  int AllocateVoice();
};
