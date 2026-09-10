//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Chord and Scale Library Implementation
//*****************************************************************************

#include "ChordScaleData.h"

#include <algorithm>
#include <sstream>

using namespace std;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
string JZChordDefinition::GetFormulaString() const
{
  static const char* intervalNames[25] = {
    "R", "b9", "9", "b3", "3", "4", "b5", "5", "#5", "6", "b7", "7",
    "8", "b9", "9", "#9", "3", "11", "#11", "5", "b13", "13", "b7", "7", "8"
  };

  ostringstream oss;
  for (size_t i = 0; i < mIntervals.size(); ++i)
  {
    if (i > 0)
    {
      oss << "-";
    }
    int iv = mIntervals[i];
    if (iv >= 0 && iv < 25)
    {
      oss << intervalNames[iv];
    }
    else
    {
      oss << iv;
    }
  }
  return oss.str();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
string JZScaleDefinition::GetFormulaString() const
{
  static const char* intervalNames[25] = {
    "R", "b2", "2", "b3", "3", "4", "b5", "5", "#5", "6", "b7", "7",
    "8", "b9", "9", "#9", "3", "11", "#11", "5", "b13", "13", "b7", "7", "8"
  };

  ostringstream oss;
  for (size_t i = 0; i < mIntervals.size(); ++i)
  {
    if (i > 0)
    {
      oss << " ";
    }
    int iv = mIntervals[i];
    if (iv >= 0 && iv < 25)
    {
      oss << intervalNames[iv];
    }
    else
    {
      oss << iv;
    }
  }
  return oss.str();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
const vector<JZChordDefinition>& JZChordScaleLibrary::GetChords()
{
  static vector<JZChordDefinition> chords;
  if (!chords.empty())
  {
    return chords;
  }

  // --- Power Chords (Rock / Metal / Pop) ---
  chords.push_back({ "Power Chord (5th)", "5", "Power Chords", {0, 7} });
  chords.push_back({ "Power Chord with Octave (1-5-8)", "5 (oct)", "Power Chords", {0, 7, 12} });
  chords.push_back({ "Flat 5 Power Chord (5b)", "5b", "Power Chords", {0, 6} });
  chords.push_back({ "Sharp 5 Power Chord (5#)", "5#", "Power Chords", {0, 8} });
  chords.push_back({ "Power Chord Double Octave", "5 (2-oct)", "Power Chords", {0, 7, 12, 19} });

  // --- Major & Minor Triads ---
  chords.push_back({ "Major Triad", "maj", "Triads", {0, 4, 7} });
  chords.push_back({ "Minor Triad", "m", "Triads", {0, 3, 7} });
  chords.push_back({ "Diminished Triad", "dim", "Triads", {0, 3, 6} });
  chords.push_back({ "Augmented Triad", "aug", "Triads", {0, 4, 8} });
  chords.push_back({ "Suspended 4th (sus4)", "sus4", "Triads", {0, 5, 7} });
  chords.push_back({ "Suspended 2nd (sus2)", "sus2", "Triads", {0, 2, 7} });

  // --- 6th Chords ---
  chords.push_back({ "Major 6th", "6", "6th Chords", {0, 4, 7, 9} });
  chords.push_back({ "Minor 6th", "m6", "6th Chords", {0, 3, 7, 9} });
  chords.push_back({ "6/9 (Sixth Add 9)", "6/9", "6th Chords", {0, 4, 7, 9, 14} });
  chords.push_back({ "Minor 6/9", "m6/9", "6th Chords", {0, 3, 7, 9, 14} });

  // --- 7th Chords ---
  chords.push_back({ "Dominant 7th", "7", "7th Chords", {0, 4, 7, 10} });
  chords.push_back({ "Major 7th", "maj7", "7th Chords", {0, 4, 7, 11} });
  chords.push_back({ "Minor 7th", "m7", "7th Chords", {0, 3, 7, 10} });
  chords.push_back({ "Minor-Major 7th", "m/maj7", "7th Chords", {0, 3, 7, 11} });
  chords.push_back({ "Half-Diminished 7th (m7b5)", "m7b5", "7th Chords", {0, 3, 6, 10} });
  chords.push_back({ "Diminished 7th (dim7)", "dim7", "7th Chords", {0, 3, 6, 9} });
  chords.push_back({ "Augmented 7th (7#5)", "7#5", "7th Chords", {0, 4, 8, 10} });
  chords.push_back({ "Augmented Major 7th (maj7#5)", "maj7#5", "7th Chords", {0, 4, 8, 11} });
  chords.push_back({ "Dominant 7b5", "7b5", "7th Chords", {0, 4, 6, 10} });
  chords.push_back({ "Major 7b5", "maj7b5", "7th Chords", {0, 4, 6, 11} });
  chords.push_back({ "7sus4", "7sus4", "7th Chords", {0, 5, 7, 10} });
  chords.push_back({ "7sus2", "7sus2", "7th Chords", {0, 2, 7, 10} });

  // --- 9th Chords ---
  chords.push_back({ "Dominant 9th", "9", "9th Chords", {0, 4, 7, 10, 14} });
  chords.push_back({ "Major 9th", "maj9", "9th Chords", {0, 4, 7, 11, 14} });
  chords.push_back({ "Minor 9th", "m9", "9th Chords", {0, 3, 7, 10, 14} });
  chords.push_back({ "7b9 (Dominant Minor 9th)", "7b9", "9th Chords", {0, 4, 7, 10, 13} });
  chords.push_back({ "7#9 (Hendrix Chord)", "7#9", "9th Chords", {0, 4, 7, 10, 15} });
  chords.push_back({ "9sus4", "9sus4", "9th Chords", {0, 5, 7, 10, 14} });
  chords.push_back({ "Add9 (Major Add 9)", "add9", "9th Chords", {0, 4, 7, 14} });
  chords.push_back({ "Minor Add9", "madd9", "9th Chords", {0, 3, 7, 14} });

  // --- 11th Chords ---
  chords.push_back({ "Dominant 11th", "11", "11th Chords", {0, 4, 7, 10, 14, 17} });
  chords.push_back({ "Major 11th", "maj11", "11th Chords", {0, 4, 7, 11, 14, 17} });
  chords.push_back({ "Minor 11th", "m11", "11th Chords", {0, 3, 7, 10, 14, 17} });
  chords.push_back({ "7#11 (Lydian Dominant)", "7#11", "11th Chords", {0, 4, 7, 10, 18} });
  chords.push_back({ "maj7#11 (Lydian Major)", "maj7#11", "11th Chords", {0, 4, 7, 11, 18} });

  // --- 13th Chords ---
  chords.push_back({ "Dominant 13th", "13", "13th Chords", {0, 4, 7, 10, 14, 21} });
  chords.push_back({ "Major 13th", "maj13", "13th Chords", {0, 4, 7, 11, 14, 21} });
  chords.push_back({ "Minor 13th", "m13", "13th Chords", {0, 3, 7, 10, 14, 21} });
  chords.push_back({ "7b13", "7b13", "13th Chords", {0, 4, 7, 10, 20} });

  // --- Altered & Jazz Chords ---
  chords.push_back({ "Altered 7th (7alt)", "7alt", "Jazz & Altered", {0, 4, 10, 13, 15, 20} });
  chords.push_back({ "7#9#5", "7#9#5", "Jazz & Altered", {0, 4, 8, 10, 15} });
  chords.push_back({ "7b9b5", "7b9b5", "Jazz & Altered", {0, 4, 6, 10, 13} });

  return chords;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
const vector<JZScaleDefinition>& JZChordScaleLibrary::GetScales()
{
  static vector<JZScaleDefinition> scales;
  if (!scales.empty())
  {
    return scales;
  }

  // --- Major & Diatonic Modes ---
  scales.push_back({ "Major / Ionian", "Major & Modes", {0, 2, 4, 5, 7, 9, 11} });
  scales.push_back({ "Dorian (2nd mode)", "Major & Modes", {0, 2, 3, 5, 7, 9, 10} });
  scales.push_back({ "Phrygian (3rd mode)", "Major & Modes", {0, 1, 3, 5, 7, 8, 10} });
  scales.push_back({ "Lydian (4th mode)", "Major & Modes", {0, 2, 4, 6, 7, 9, 11} });
  scales.push_back({ "Mixolydian (5th mode)", "Major & Modes", {0, 2, 4, 5, 7, 9, 10} });
  scales.push_back({ "Natural Minor / Aeolian", "Major & Modes", {0, 2, 3, 5, 7, 8, 10} });
  scales.push_back({ "Locrian (7th mode)", "Major & Modes", {0, 1, 3, 5, 6, 8, 10} });

  // --- Minor Variations ---
  scales.push_back({ "Harmonic Minor", "Minor Variations", {0, 2, 3, 5, 7, 8, 11} });
  scales.push_back({ "Melodic Minor (Jazz Minor)", "Minor Variations", {0, 2, 3, 5, 7, 9, 11} });
  scales.push_back({ "Phrygian Dominant (Spanish Phrygian)", "Minor Variations", {0, 1, 4, 5, 7, 8, 10} });
  scales.push_back({ "Lydian Dominant (Mixolydian #11)", "Minor Variations", {0, 2, 4, 6, 7, 9, 10} });
  scales.push_back({ "Super Locrian / Altered Scale", "Minor Variations", {0, 1, 3, 4, 6, 8, 10} });
  scales.push_back({ "Harmonic Major (Ionian b13)", "Minor Variations", {0, 2, 4, 5, 7, 8, 11} });
  scales.push_back({ "Dorian b2 (Melodic 2nd mode)", "Minor Variations", {0, 1, 3, 5, 7, 9, 10} });
  scales.push_back({ "Lydian Augmented (Melodic 3rd mode)", "Minor Variations", {0, 2, 4, 6, 8, 9, 11} });
  scales.push_back({ "Locrian Natural 2 (Melodic 6th mode)", "Minor Variations", {0, 2, 3, 5, 6, 8, 10} });

  // --- Pentatonic & Blues ---
  scales.push_back({ "Major Pentatonic", "Pentatonic & Blues", {0, 2, 4, 7, 9} });
  scales.push_back({ "Minor Pentatonic", "Pentatonic & Blues", {0, 3, 5, 7, 10} });
  scales.push_back({ "Blues Scale (Hexatonic)", "Pentatonic & Blues", {0, 3, 5, 6, 7, 10} });
  scales.push_back({ "Major Blues Scale", "Pentatonic & Blues", {0, 2, 3, 4, 7, 9} });
  scales.push_back({ "Neutral / Suspended Pentatonic", "Pentatonic & Blues", {0, 2, 5, 7, 10} });

  // --- Symmetrical & Bebop ---
  scales.push_back({ "Whole Tone", "Symmetrical & Bebop", {0, 2, 4, 6, 8, 10} });
  scales.push_back({ "Diminished (Half-Whole)", "Symmetrical & Bebop", {0, 1, 3, 4, 6, 7, 9, 10} });
  scales.push_back({ "Diminished (Whole-Half)", "Symmetrical & Bebop", {0, 2, 3, 5, 6, 8, 9, 11} });
  scales.push_back({ "Bebop Dominant", "Symmetrical & Bebop", {0, 2, 4, 5, 7, 9, 10, 11} });
  scales.push_back({ "Bebop Major", "Symmetrical & Bebop", {0, 2, 4, 5, 7, 8, 9, 11} });
  scales.push_back({ "Bebop Minor", "Symmetrical & Bebop", {0, 2, 3, 4, 5, 7, 9, 10} });
  scales.push_back({ "Chromatic", "Symmetrical & Bebop", {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11} });

  // --- World & Traditional ---
  scales.push_back({ "Hungarian Minor (Gypsy Minor)", "World & Traditional", {0, 2, 3, 6, 7, 8, 11} });
  scales.push_back({ "Double Harmonic Major (Byzantine / Arabic)", "World & Traditional", {0, 1, 4, 5, 7, 8, 11} });
  scales.push_back({ "Persian", "World & Traditional", {0, 1, 4, 5, 6, 8, 11} });
  scales.push_back({ "Hirajoshi (Japanese)", "World & Traditional", {0, 2, 3, 7, 8} });
  scales.push_back({ "Insen (Japanese)", "World & Traditional", {0, 1, 5, 7, 10} });
  scales.push_back({ "Kumoi (Japanese)", "World & Traditional", {0, 2, 3, 7, 9} });
  scales.push_back({ "Iwato (Japanese)", "World & Traditional", {0, 1, 5, 6, 10} });
  scales.push_back({ "Pelog (Indonesian)", "World & Traditional", {0, 1, 3, 7, 8} });

  return scales;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
vector<string> JZChordScaleLibrary::GetChordCategories()
{
  const vector<JZChordDefinition>& chords = GetChords();
  vector<string> categories;
  for (size_t i = 0; i < chords.size(); ++i)
  {
    if (find(categories.begin(), categories.end(), chords[i].mCategory) == categories.end())
    {
      categories.push_back(chords[i].mCategory);
    }
  }
  return categories;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
vector<string> JZChordScaleLibrary::GetScaleCategories()
{
  const vector<JZScaleDefinition>& scales = GetScales();
  vector<string> categories;
  for (size_t i = 0; i < scales.size(); ++i)
  {
    if (find(categories.begin(), categories.end(), scales[i].mCategory) == categories.end())
    {
      categories.push_back(scales[i].mCategory);
    }
  }
  return categories;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
const vector<string>& JZChordScaleLibrary::GetRootNoteNames()
{
  static vector<string> names;
  if (names.empty())
  {
    names.push_back("C");
    names.push_back("C# / Db");
    names.push_back("D");
    names.push_back("D# / Eb");
    names.push_back("E");
    names.push_back("F");
    names.push_back("F# / Gb");
    names.push_back("G");
    names.push_back("G# / Ab");
    names.push_back("A");
    names.push_back("A# / Bb");
    names.push_back("B");
  }
  return names;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZChordScaleLibrary::NoteNameToSemitone(const string& Name)
{
  const vector<string>& names = GetRootNoteNames();
  for (size_t i = 0; i < names.size(); ++i)
  {
    if (names[i] == Name)
    {
      return static_cast<int>(i);
    }
  }
  return 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
string JZChordScaleLibrary::PitchToNoteName(int Pitch)
{
  static const char* noteNames[12] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
  };
  int semi = (Pitch % 12 + 12) % 12;
  int oct = (Pitch / 12) - 1;
  ostringstream oss;
  oss << noteNames[semi] << oct;
  return oss.str();
}
