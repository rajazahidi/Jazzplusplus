//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Performance Chord Trigger Pads & Voicing Engine
//
// Live 16-pad chord progression performance launcher (Akai MPC / Maschine style)
// supporting genre progressions (Neo-Soul Jazz, Lo-Fi Chillhop, Pop Anthem, R&B),
// Drop-2 voicing transformations, and humanized guitar strum spread timing.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include "ClipMatrixEngine.h" // For JZMatrixMidiNote definition

// Voicing Style Transformation
enum JZChordVoicingMode
{
  VOICING_ROOT_POSITION = 0,  // Standard compact chord voicing
  VOICING_DROP_2,             // Drop-2 voicing (2nd highest voice dropped 1 octave)
  VOICING_OPEN_SPREAD         // Wide octave spread across bass, tenor, and treble
};

// Genre Progression Preset
enum JZChordPadPreset
{
  PRESET_NEO_SOUL_JAZZ = 0,
  PRESET_LOFI_CHILLHOP,
  PRESET_POP_EDM_ANTHEM,
  PRESET_RNB_SOUL,
  PRESET_CINEMATIC_DRAMA
};

// Single Pad Definition
struct JZChordPadDefinition
{
  std::string chordName;
  int rootMidiNote;
  std::vector<int> pitches;   // Raw semitone pitches (e.g. 60, 64, 67, 71, 74 for Cmaj9)
  std::string colorHex;

  JZChordPadDefinition(const std::string& name = "Chord", int root = 60, const std::vector<int>& notes = {60, 64, 67})
    : chordName(name),
      rootMidiNote(root),
      pitches(notes),
      colorHex("#00e5ff")
  {
  }
};

// Performance Chord Pads Class
class JZChordPerformancePads
{
public:
  static const int NUM_PADS = 16;

  JZChordPerformancePads();
  ~JZChordPerformancePads() = default;

  // Preset management
  void LoadPreset(JZChordPadPreset preset);
  void SetVoicingMode(JZChordVoicingMode mode) { mVoicingMode = mode; }
  JZChordVoicingMode GetVoicingMode() const { return mVoicingMode; }

  // Pad custom setup
  void SetPad(int padIndex, const JZChordPadDefinition& pad);
  const JZChordPadDefinition& GetPad(int padIndex) const;
  JZChordPadDefinition& GetPad(int padIndex);

  // Strum timing (ticks offset between notes, e.g. 0 to 48 ticks)
  void SetStrumSpreadTicks(int ticks) { mStrumSpreadTicks = std::max(0, ticks); }
  int GetStrumSpreadTicks() const { return mStrumSpreadTicks; }

  // Trigger pad: generates voiced MIDI notes ready for sequencer or live playback
  bool TriggerPad(int padIndex, int baseVelocity, std::vector<JZMatrixMidiNote>& outNotes) const;

  // Apply Voicing Transformation to a raw pitch list
  static std::vector<int> ApplyVoicing(const std::vector<int>& rawPitches, JZChordVoicingMode mode);

private:
  JZChordPadDefinition mPads[NUM_PADS];
  JZChordVoicingMode mVoicingMode;
  int mStrumSpreadTicks;
};
