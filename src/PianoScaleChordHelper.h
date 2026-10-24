//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// In-Key Piano Roll Scale Highlighting, Snap-to-Scale & Chord Stamp Tool
//
// Scale Awareness for 12-TET Piano Roll, Instant Non-Diatonic Lane Dimming,
// Diatonic Snapping, and 1-Click Jazz Chord Stamp Generator (9th, 11th, 13th, Alt).
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include "ChordScaleData.h"

class JZTrack;

// Chord Stamp Template Categories
enum JZChordStampType
{
  CHORD_STAMP_MAJOR_TRIAD = 0,
  CHORD_STAMP_MINOR_TRIAD,
  CHORD_STAMP_DIMINISHED_TRIAD,
  CHORD_STAMP_AUGMENTED_TRIAD,
  CHORD_STAMP_SUS2,
  CHORD_STAMP_SUS4,
  CHORD_STAMP_MAJOR_7TH,
  CHORD_STAMP_MINOR_7TH,
  CHORD_STAMP_DOMINANT_7TH,
  CHORD_STAMP_HALF_DIMINISHED_7TH,
  CHORD_STAMP_DIMINISHED_7TH,
  CHORD_STAMP_MINOR_MAJOR_7TH,
  CHORD_STAMP_DOMINANT_9TH,
  CHORD_STAMP_MAJOR_9TH,
  CHORD_STAMP_MINOR_9TH,
  CHORD_STAMP_7_FLAT_9,
  CHORD_STAMP_7_SHARP_9,
  CHORD_STAMP_DOMINANT_11TH,
  CHORD_STAMP_7_SHARP_11,
  CHORD_STAMP_DOMINANT_13TH,
  CHORD_STAMP_MAJOR_13TH,
  CHORD_STAMP_ALTERED_DOMINANT,
  CHORD_STAMP_COUNT
};

class JZPianoScaleChordHelper
{
  public:

    // Check if MIDI pitch belongs to scale (0 = C, 1 = C#, ... 11 = B)
    static bool IsPitchInScale(int pitch, int tonicPc, int scaleCategoryIndex, int scaleIndex);

    // Snap any arbitrary pitch to closest in-scale pitch
    static int SnapPitchToScale(int pitch, int tonicPc, int scaleCategoryIndex, int scaleIndex);

    // Get chord notes for 1-click Chord Stamp
    static std::vector<int> GetChordStampPitches(int rootPitch, JZChordStampType chordType, int inversion = 0);

    // Stamp chord notes directly into track at timeline clock
    static int StampChordToTrack(
      JZTrack* pTrack,
      int rootPitch,
      JZChordStampType chordType,
      int clock,
      int length = 96,
      int velocity = 85,
      int channel = 0,
      int inversion = 0);

    static const char* GetChordStampName(JZChordStampType type);
};
