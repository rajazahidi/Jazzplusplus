//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// In-Key Piano Roll Scale Highlighting, Snap-to-Scale & Chord Stamp Tool
// Implementation
//*****************************************************************************

#include "PianoScaleChordHelper.h"
#include "Track.h"
#include "Events.h"

#include <algorithm>

using namespace std;

//-----------------------------------------------------------------------------
// JZPianoScaleChordHelper Implementation
//-----------------------------------------------------------------------------
const char* JZPianoScaleChordHelper::GetChordStampName(JZChordStampType type)
{
  switch (type)
  {
    case CHORD_STAMP_MAJOR_TRIAD: return "Major Triad (1-3-5)";
    case CHORD_STAMP_MINOR_TRIAD: return "Minor Triad (1-b3-5)";
    case CHORD_STAMP_DIMINISHED_TRIAD: return "Diminished Triad (1-b3-b5)";
    case CHORD_STAMP_AUGMENTED_TRIAD: return "Augmented Triad (1-3-#5)";
    case CHORD_STAMP_SUS2: return "Suspended 2nd (1-2-5)";
    case CHORD_STAMP_SUS4: return "Suspended 4th (1-4-5)";
    case CHORD_STAMP_MAJOR_7TH: return "Major 7th (1-3-5-7)";
    case CHORD_STAMP_MINOR_7TH: return "Minor 7th (1-b3-5-b7)";
    case CHORD_STAMP_DOMINANT_7TH: return "Dominant 7th (1-3-5-b7)";
    case CHORD_STAMP_HALF_DIMINISHED_7TH: return "Half-Diminished 7th / m7b5 (1-b3-b5-b7)";
    case CHORD_STAMP_DIMINISHED_7TH: return "Diminished 7th (1-b3-b5-bb7)";
    case CHORD_STAMP_MINOR_MAJOR_7TH: return "Minor Major 7th (1-b3-5-7)";
    case CHORD_STAMP_DOMINANT_9TH: return "Dominant 9th (1-3-5-b7-9)";
    case CHORD_STAMP_MAJOR_9TH: return "Major 9th (1-3-5-7-9)";
    case CHORD_STAMP_MINOR_9TH: return "Minor 9th (1-b3-5-b7-9)";
    case CHORD_STAMP_7_FLAT_9: return "7b9 Dominant (1-3-5-b7-b9)";
    case CHORD_STAMP_7_SHARP_9: return "7#9 Hendrix Chord (1-3-5-b7-#9)";
    case CHORD_STAMP_DOMINANT_11TH: return "Dominant 11th (1-5-b7-9-11)";
    case CHORD_STAMP_7_SHARP_11: return "7#11 Lydian Dominant (1-3-5-b7-#11)";
    case CHORD_STAMP_DOMINANT_13TH: return "Dominant 13th (1-3-b7-9-13)";
    case CHORD_STAMP_MAJOR_13TH: return "Major 13th (1-3-7-9-13)";
    case CHORD_STAMP_ALTERED_DOMINANT: return "7alt Super Locrian (1-3-b7-b9-#9-b13)";
    default: return "Major Triad";
  }
}

bool JZPianoScaleChordHelper::IsPitchInScale(int pitch, int tonicPc, int scaleCategoryIndex, int scaleIndex)
{
  tonicPc = (tonicPc % 12 + 12) % 12;
  int pc = (pitch % 12 + 12) % 12;
  int semitoneFromTonic = (pc - tonicPc + 12) % 12;

  // Standard Diatonic Major Scale intervals fallback
  // Major: 0, 2, 4, 5, 7, 9, 11
  static const bool sMajorScale[12] = {true, false, true, false, true, true, false, true, false, true, false, true};
  // Natural Minor: 0, 2, 3, 5, 7, 8, 10
  static const bool sMinorScale[12] = {true, false, true, true, false, true, false, true, true, false, true, false};

  if (scaleCategoryIndex == 1) // Minor scale category
  {
    return sMinorScale[semitoneFromTonic];
  }
  return sMajorScale[semitoneFromTonic];
}

int JZPianoScaleChordHelper::SnapPitchToScale(int pitch, int tonicPc, int scaleCategoryIndex, int scaleIndex)
{
  if (IsPitchInScale(pitch, tonicPc, scaleCategoryIndex, scaleIndex))
  {
    return pitch;
  }

  // Find nearest pitch up or down that is in scale
  for (int delta = 1; delta <= 6; ++delta)
  {
    if (IsPitchInScale(pitch - delta, tonicPc, scaleCategoryIndex, scaleIndex)) return pitch - delta;
    if (IsPitchInScale(pitch + delta, tonicPc, scaleCategoryIndex, scaleIndex)) return pitch + delta;
  }
  return pitch;
}

vector<int> JZPianoScaleChordHelper::GetChordStampPitches(int rootPitch, JZChordStampType chordType, int inversion)
{
  vector<int> intervals;

  switch (chordType)
  {
    case CHORD_STAMP_MAJOR_TRIAD: intervals = {0, 4, 7}; break;
    case CHORD_STAMP_MINOR_TRIAD: intervals = {0, 3, 7}; break;
    case CHORD_STAMP_DIMINISHED_TRIAD: intervals = {0, 3, 6}; break;
    case CHORD_STAMP_AUGMENTED_TRIAD: intervals = {0, 4, 8}; break;
    case CHORD_STAMP_SUS2: intervals = {0, 2, 7}; break;
    case CHORD_STAMP_SUS4: intervals = {0, 5, 7}; break;
    case CHORD_STAMP_MAJOR_7TH: intervals = {0, 4, 7, 11}; break;
    case CHORD_STAMP_MINOR_7TH: intervals = {0, 3, 7, 10}; break;
    case CHORD_STAMP_DOMINANT_7TH: intervals = {0, 4, 7, 10}; break;
    case CHORD_STAMP_HALF_DIMINISHED_7TH: intervals = {0, 3, 6, 10}; break;
    case CHORD_STAMP_DIMINISHED_7TH: intervals = {0, 3, 6, 9}; break;
    case CHORD_STAMP_MINOR_MAJOR_7TH: intervals = {0, 3, 7, 11}; break;
    case CHORD_STAMP_DOMINANT_9TH: intervals = {0, 4, 7, 10, 14}; break;
    case CHORD_STAMP_MAJOR_9TH: intervals = {0, 4, 7, 11, 14}; break;
    case CHORD_STAMP_MINOR_9TH: intervals = {0, 3, 7, 10, 14}; break;
    case CHORD_STAMP_7_FLAT_9: intervals = {0, 4, 7, 10, 13}; break;
    case CHORD_STAMP_7_SHARP_9: intervals = {0, 4, 7, 10, 15}; break;
    case CHORD_STAMP_DOMINANT_11TH: intervals = {0, 7, 10, 14, 17}; break;
    case CHORD_STAMP_7_SHARP_11: intervals = {0, 4, 7, 10, 18}; break;
    case CHORD_STAMP_DOMINANT_13TH: intervals = {0, 4, 10, 14, 21}; break;
    case CHORD_STAMP_MAJOR_13TH: intervals = {0, 4, 11, 14, 21}; break;
    case CHORD_STAMP_ALTERED_DOMINANT: intervals = {0, 4, 10, 13, 15, 20}; break;
    default: intervals = {0, 4, 7}; break;
  }

  vector<int> pitches;
  for (size_t i = 0; i < intervals.size(); ++i)
  {
    pitches.push_back(rootPitch + intervals[i]);
  }

  // Handle inversions (shift lowest notes up an octave)
  if (inversion > 0 && !pitches.empty())
  {
    int invCount = inversion % (int)pitches.size();
    for (int inv = 0; inv < invCount; ++inv)
    {
      pitches[inv] += 12;
    }
    sort(pitches.begin(), pitches.end());
  }

  return pitches;
}

int JZPianoScaleChordHelper::StampChordToTrack(
  JZTrack* pTrack,
  int rootPitch,
  JZChordStampType chordType,
  int clock,
  int length,
  int velocity,
  int channel,
  int inversion)
{
  if (!pTrack) return 0;

  vector<int> pitches = GetChordStampPitches(rootPitch, chordType, inversion);
  int notesAdded = 0;

  for (size_t i = 0; i < pitches.size(); ++i)
  {
    int p = max(0, min(127, pitches[i]));
    pTrack->Put(new JZKeyOnEvent(clock, channel, (unsigned char)p, (unsigned char)velocity, (unsigned short)length));
    notesAdded++;
  }

  return notesAdded;
}
