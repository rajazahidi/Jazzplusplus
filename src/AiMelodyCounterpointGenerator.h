//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// AI MIDI Melody & Counterpoint Generator
//
// Generative musical intelligence engine creating expressive lead melodies
// and Fuxian counterpoint lines based on project chord progression harmonic context,
// stepwise voice leading, tension resolution, and contrary motion counterpoint.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

struct JZChordContext
{
  int rootMidi;               // 0 to 11 (C=0, D=2, etc.)
  bool isMinor;
  int startTick;
  int durationTicks;

  JZChordContext(int root = 0, bool minor = false, int start = 0, int dur = 384)
    : rootMidi(root),
      isMinor(minor),
      startTick(start),
      durationTicks(dur)
  {
  }
};

struct JZMelodyNote
{
  int midiPitch;
  int velocity;
  int startTick;
  int durationTicks;
  bool isCounterpoint;

  JZMelodyNote(int pitch = 60, int vel = 100, int start = 0, int dur = 96, bool cp = false)
    : midiPitch(pitch),
      velocity(vel),
      startTick(start),
      durationTicks(dur),
      isCounterpoint(cp)
  {
  }
};

class JZAiMelodyCounterpointGenerator
{
public:
  JZAiMelodyCounterpointGenerator(int ticksPerQuarter = 96);
  ~JZAiMelodyCounterpointGenerator() = default;

  void SetTicksPerQuarter(int tpq) { mTicksPerQuarter = tpq > 24 ? tpq : 96; }
  int GetTicksPerQuarter() const { return mTicksPerQuarter; }

  // Generation Controls
  void SetStepToLeapRatio(float ratio);       // 0.0 (all leaps) to 1.0 (all stepwise)
  float GetStepToLeapRatio() const { return mStepToLeapRatio; }

  void SetDensity(float notesPerQuarter);     // 0.5 to 4.0 notes per beat
  float GetDensity() const { return mDensity; }

  void SetSyncopation(float syncFactor);      // 0.0 (on-beat) to 1.0 (heavily syncopated)
  float GetSyncopation() const { return mSyncopation; }

  void SetOctaveRange(int minPitch, int maxPitch);

  // Generate Lead Melody & Secondary Counterpoint Line
  bool Generate(
    const std::vector<JZChordContext>& chordProgression,
    std::vector<JZMelodyNote>& outLeadMelody,
    std::vector<JZMelodyNote>& outCounterpoint);

private:
  int mTicksPerQuarter;
  float mStepToLeapRatio;
  float mDensity;
  float mSyncopation;
  int mMinPitch;
  int mMaxPitch;

  int ChooseNextPitch(int currentPitch, const JZChordContext& chord, bool isContraryMotion, int leadStepPitch);
};
