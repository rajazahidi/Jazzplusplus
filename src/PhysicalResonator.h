//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Karplus-Strong Physical Acoustic Resonator & Tuned Comb Filter Harp
//
// Acoustic physical resonator modeling plucked strings, metallic bells, and tuned
// harp resonance from dry clicks or noise bursts via 4 feedback comb delay lines,
// chord tuning structures, adjustable decay time, and high-frequency damping.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

// Resonator Chord Structure Tuning
enum JZResonatorChordMode
{
  RESONATOR_UNISON = 0,       // All lines tuned to root pitch
  RESONATOR_MAJOR,            // Root, Major 3rd (+4st), 5th (+7st), Octave (+12st)
  RESONATOR_MINOR,            // Root, Minor 3rd (+3st), 5th (+7st), Octave (+12st)
  RESONATOR_SUS4,             // Root, 4th (+5st), 5th (+7st), Octave (+12st)
  RESONATOR_PENTATONIC        // Root, Minor 3rd (+3st), 4th (+5st), Minor 7th (+10st)
};

// Configuration for Physical Resonator
struct JZResonatorConfig
{
  int rootMidiNote;           // 24 to 96 (default 48 = C3)
  JZResonatorChordMode chordMode;
  float decayTimeSec;         // 0.1s to 8.0s (resonance duration)
  float damping;              // 0.0 (metallic bright) to 1.0 (warm wooden string damping)
  float stereoSpread;         // 0.0 to 1.0 (panning width across the 4 resonator strings)
  float dryWet;               // 0.0 to 1.0

  JZResonatorConfig()
    : rootMidiNote(48),
      chordMode(RESONATOR_MAJOR),
      decayTimeSec(2.5f),
      damping(0.35f),
      stereoSpread(0.75f),
      dryWet(0.5f)
  {
  }
};

// Single Comb Filter String Line
struct JZCombString
{
  std::vector<float> buffer;
  int writePos;
  float filterState;

  JZCombString() : writePos(0), filterState(0.0f) { buffer.resize(2048, 0.0f); }
  void Reset() { std::fill(buffer.begin(), buffer.end(), 0.0f); writePos = 0; filterState = 0.0f; }
};

// Physical Resonator Class
class JZPhysicalResonator
{
public:
  static const int NUM_STRINGS = 4;

  JZPhysicalResonator(float sampleRate = 44100.0f);
  ~JZPhysicalResonator() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  void SetConfig(const JZResonatorConfig& config);
  const JZResonatorConfig& GetConfig() const { return mConfig; }
  JZResonatorConfig& GetConfig() { return mConfig; }

  void Reset();

  // Process Stereo Audio Block
  void Process(float* inOutL, float* inOutR, int numSamples);

  // Process Mono Audio Block
  void ProcessMono(float* inOut, int numSamples);

private:
  float mSampleRate;
  JZResonatorConfig mConfig;
  JZCombString mStrings[NUM_STRINGS];

  void GetChordIntervals(int intervals[NUM_STRINGS]) const;
};
