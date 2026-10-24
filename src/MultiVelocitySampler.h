//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Multi-Velocity Zone Sampler Instrument
//
// Polyphonic sample playback engine with multi-velocity layering,
// key-zone split mapping, per-zone ADSR envelopes, resonant low-pass filtering,
// and procedural acoustic instrument generation.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <memory>
#include <cmath>
#include <algorithm>

// Sample Zone Definition
struct JZSampleZone
{
  int zoneId;
  std::string name;
  int rootKey;                // Center MIDI pitch (e.g. 60 = C4)
  int lowKey;                 // Key range start
  int highKey;                // Key range end
  int lowVelocity;            // Velocity range start (1 to 127)
  int highVelocity;           // Velocity range end (1 to 127)
  
  // ADSR Envelope
  float attackMs;
  float decayMs;
  float sustainLevel;         // 0.0 to 1.0
  float releaseMs;

  // Filter
  float filterCutoffHz;       // 100 Hz to 20000 Hz
  float filterResonance;      // 0.0 to 0.95

  // Audio sample buffer
  std::vector<float> sampleDataL;
  std::vector<float> sampleDataR;

  JZSampleZone()
    : zoneId(0),
      name("Zone"),
      rootKey(60),
      lowKey(0),
      highKey(127),
      lowVelocity(1),
      highVelocity(127),
      attackMs(5.0f),
      decayMs(200.0f),
      sustainLevel(0.7f),
      releaseMs(250.0f),
      filterCutoffHz(14000.0f),
      filterResonance(0.1f)
  {
  }
};

// Polyphonic voice state
enum JZEnvPhase
{
  ENV_IDLE = 0,
  ENV_ATTACK,
  ENV_DECAY,
  ENV_SUSTAIN,
  ENV_RELEASE
};

struct JZActiveVoice
{
  bool active;
  int note;
  int velocity;
  int zoneIndex;
  double playhead;
  double speed;
  
  JZEnvPhase envPhase;
  float envLevel;
  
  float filterStateL;
  float filterStateR;

  JZActiveVoice()
    : active(false),
      note(60),
      velocity(100),
      zoneIndex(-1),
      playhead(0.0),
      speed(1.0),
      envPhase(ENV_IDLE),
      envLevel(0.0f),
      filterStateL(0.0f),
      filterStateR(0.0f)
  {
  }
};

// Multi-Velocity Sampler Class
class JZMultiVelocitySampler
{
public:
  JZMultiVelocitySampler(float sampleRate = 44100.0f, int maxPolyphony = 32);
  ~JZMultiVelocitySampler() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  // Zone Management
  int AddZone(const JZSampleZone& zone);
  void ClearZones();
  int GetZoneCount() const { return (int)mZones.size(); }
  const std::vector<JZSampleZone>& GetZones() const { return mZones; }

  // MIDI Events
  void NoteOn(int note, int velocity);
  void NoteOff(int note);
  void AllNotesOff();

  // Audio Block Rendering
  void Process(float* outL, float* outR, int numSamples);

  // Active voice count for telemetry
  int GetActiveVoiceCount() const;

  // Preset Factory: Built-in procedural acoustic instruments
  void LoadAcousticPianoKit();
  void LoadStudioDrumKit();

private:
  float mSampleRate;
  int mMaxPolyphony;
  std::vector<JZSampleZone> mZones;
  std::vector<JZActiveVoice> mVoices;

  int FindBestMatchingZone(int note, int velocity) const;
  int AllocateVoice();
};
