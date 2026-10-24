//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// 4-Operator Frequency Modulation (FM) Synthesizer
//
// Classical 4-operator FM synthesis engine with 8 routing algorithms
// (Yamaha DX7 / Ableton Operator style), variable frequency ratios,
// operator 4 self-feedback loop, per-operator ADSR envelopes, and polyphony.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

// Single FM Operator Configuration
struct JZFmOperatorConfig
{
  float ratio;                // Frequency ratio (0.5 to 16.0, default 1.0)
  float detuneHz;             // Micro detune in Hz (-5.0 to +5.0)
  float outputLevel;          // Output level / Modulation index (0.0 to 1.0)
  float feedback;             // Self-feedback (used for Operator 4: 0.0 to 1.0)
  
  // ADSR Envelope
  float attackMs;
  float decayMs;
  float sustainLevel;         // 0.0 to 1.0
  float releaseMs;

  JZFmOperatorConfig(float r = 1.0f, float lvl = 1.0f, float att = 5.0f, float dec = 250.0f, float sus = 0.7f, float rel = 200.0f)
    : ratio(r),
      detuneHz(0.0f),
      outputLevel(lvl),
      feedback(0.0f),
      attackMs(att),
      decayMs(dec),
      sustainLevel(sus),
      releaseMs(rel)
  {
  }
};

// Full 4-Operator FM Synthesizer Configuration
struct JZFmSynthConfig
{
  int routingAlgorithm;       // 1 to 8
  JZFmOperatorConfig ops[4];  // 0: Op 1, 1: Op 2, 2: Op 3, 3: Op 4
  float masterGainDB;         // Output volume trim

  JZFmSynthConfig()
    : routingAlgorithm(1),
      masterGainDB(-3.0f)
  {
    // Default: DX7 Style Electric Piano
    ops[0] = JZFmOperatorConfig(1.0f, 1.0f, 2.0f, 400.0f, 0.4f, 300.0f);  // Carrier
    ops[1] = JZFmOperatorConfig(1.0f, 0.75f, 2.0f, 200.0f, 0.0f, 150.0f); // Modulator 1
    ops[2] = JZFmOperatorConfig(3.0f, 0.45f, 1.0f, 120.0f, 0.0f, 100.0f); // Tine Modulator
    ops[3] = JZFmOperatorConfig(14.0f, 0.25f, 1.0f, 60.0f, 0.0f, 50.0f);  // Bell Modulator
    ops[3].feedback = 0.25f;
  }
};

// Polyphonic voice state
struct JZFmVoice
{
  bool active;
  int note;
  int velocity;
  double phase[4];
  float envLevel[4];
  int envStage[4];            // 0: Idle, 1: Attack, 2: Decay, 3: Sustain, 4: Release
  float prevOp4Out;

  JZFmVoice()
    : active(false), note(60), velocity(100), prevOp4Out(0.0f)
  {
    for (int i = 0; i < 4; ++i) { phase[i] = 0.0; envLevel[i] = 0.0f; envStage[i] = 0; }
  }
};

// FM Synthesizer Class
class JZFmSynthesizer
{
public:
  static const int MAX_POLYPHONY = 16;

  JZFmSynthesizer(float sampleRate = 44100.0f);
  ~JZFmSynthesizer() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  void SetConfig(const JZFmSynthConfig& config);
  const JZFmSynthConfig& GetConfig() const { return mConfig; }
  JZFmSynthConfig& GetConfig() { return mConfig; }

  void Reset();

  // MIDI Events
  void NoteOn(int note, int velocity);
  void NoteOff(int note);
  void AllNotesOff();

  // Process Stereo Audio Block
  void Process(float* outL, float* outR, int numSamples);

  // Active voice count
  int GetActiveVoiceCount() const;

  // Preset Factory
  void LoadPreset(int presetId); // 0: E-Piano, 1: Lately Bass, 2: Crystal Bell, 3: Reese Bass

private:
  float mSampleRate;
  JZFmSynthConfig mConfig;
  JZFmVoice mVoices[MAX_POLYPHONY];

  int AllocateVoice();
  float ComputeVoiceSample(JZFmVoice& voice, float baseFreq);
};
