//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Polymetric & Polyrhythmic Drum Machine Sequencer
//
// Advanced multi-track drum sequencer with independent per-track step loop lengths
// (polymeter 1 to 64 steps), clock ratios (polyrhythm 3:2, 5:4), step probabilities,
// velocity humanization, and micro-burst ratchet rolls.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

struct JZDrumStep
{
  bool active;
  int velocity;             // 1 to 127
  int probability;          // 0% to 100%
  int ratchetCount;         // 1 (normal) to 4 (rapid burst roll)

  JZDrumStep()
    : active(false),
      velocity(100),
      probability(100),
      ratchetCount(1)
  {
  }
};

struct JZDrumTrack
{
  std::string name;
  int midiNote;             // General MIDI drum note (36=Kick, 38=Snare, 42=ClosedHat, etc.)
  int stepCount;            // Independent track step length (e.g. 16, 14, 12, 7)
  int currentStep;          // Playhead index
  float clockMultiplier;    // 1.0 = standard, 1.5 = 3:2 polyrhythm, 0.75 = 3:4
  std::vector<JZDrumStep> steps;

  JZDrumTrack(const std::string& trackName = "Drum", int note = 36, int defaultSteps = 16)
    : name(trackName),
      midiNote(note),
      stepCount(defaultSteps),
      currentStep(0),
      clockMultiplier(1.0f)
  {
    steps.resize(64);
  }
};

struct JZTriggeredDrumNote
{
  int midiNote;
  int velocity;
  int subTickOffset;        // Timing offset for ratchets/swing
};

class JZPolymetricDrumSequencer
{
public:
  JZPolymetricDrumSequencer();
  ~JZPolymetricDrumSequencer() = default;

  void SetTrackCount(size_t count);
  size_t GetTrackCount() const { return mTracks.size(); }

  JZDrumTrack& GetTrack(size_t index);
  const JZDrumTrack& GetTrack(size_t index) const;

  void SetGlobalSwing(float swingPercent); // 0% (straight) to 75% (heavy swing)
  float GetGlobalSwing() const { return mGlobalSwing; }

  // Step Sequencer Clock Advance
  void ResetPlayhead();
  void AdvanceStep(std::vector<JZTriggeredDrumNote>& outTriggeredNotes);

  // Helper to load modern polyrhythmic genre presets
  void LoadNeoSoulPolymeterPreset();

private:
  std::vector<JZDrumTrack> mTracks;
  float mGlobalSwing;
  int mGlobalStepTick;
};
