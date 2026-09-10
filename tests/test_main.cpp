//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Core Unit Tests Suite
//*****************************************************************************

#include "ChordScaleData.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

// Test counters
static int gTestsRun = 0;
static int gTestsPassed = 0;
static int gTestsFailed = 0;

#define TEST_ASSERT(condition, msg) \
  do { \
    gTestsRun++; \
    if (condition) { \
      gTestsPassed++; \
    } else { \
      gTestsFailed++; \
      cerr << "  [FAILED] " << msg << " (" << __FILE__ << ":" << __LINE__ << ")" << endl; \
    } \
  } while (0)

//-----------------------------------------------------------------------------
// Test 1: Chord and Scale Counts & Categories
//-----------------------------------------------------------------------------
void TestCountsAndCategories()
{
  cout << "[TEST] Running TestCountsAndCategories..." << endl;

  const vector<JZChordDefinition>& chords = JZChordScaleLibrary::GetChords();
  TEST_ASSERT(chords.size() >= 35, "Chord library should contain at least 35 chords");

  const vector<JZScaleDefinition>& scales = JZChordScaleLibrary::GetScales();
  TEST_ASSERT(scales.size() >= 25, "Scale library should contain at least 25 scales");

  vector<string> chordCats = JZChordScaleLibrary::GetChordCategories();
  TEST_ASSERT(chordCats.size() >= 6, "Should have at least 6 chord categories");

  vector<string> scaleCats = JZChordScaleLibrary::GetScaleCategories();
  TEST_ASSERT(scaleCats.size() >= 5, "Should have at least 5 scale categories");
}

//-----------------------------------------------------------------------------
// Test 2: Power Chords
//-----------------------------------------------------------------------------
void TestPowerChords()
{
  cout << "[TEST] Running TestPowerChords..." << endl;

  const vector<JZChordDefinition>& chords = JZChordScaleLibrary::GetChords();
  bool foundP5 = false;
  bool foundP5Oct = false;
  bool foundP5b = false;
  bool foundP5Sharp = false;

  for (size_t i = 0; i < chords.size(); ++i)
  {
    if (chords[i].mShortName == "5")
    {
      foundP5 = true;
      vector<int> expected = {0, 7};
      TEST_ASSERT(chords[i].mIntervals == expected, "Power chord 5 should have intervals {0, 7}");
      TEST_ASSERT(chords[i].mCategory == "Power Chords", "Category should be Power Chords");
    }
    else if (chords[i].mShortName == "5 (oct)")
    {
      foundP5Oct = true;
      vector<int> expected = {0, 7, 12};
      TEST_ASSERT(chords[i].mIntervals == expected, "Power chord with octave should have {0, 7, 12}");
    }
    else if (chords[i].mShortName == "5b")
    {
      foundP5b = true;
      vector<int> expected = {0, 6};
      TEST_ASSERT(chords[i].mIntervals == expected, "Power chord 5b should have {0, 6}");
    }
    else if (chords[i].mShortName == "5#")
    {
      foundP5Sharp = true;
      vector<int> expected = {0, 8};
      TEST_ASSERT(chords[i].mIntervals == expected, "Power chord 5# should have {0, 8}");
    }
  }

  TEST_ASSERT(foundP5, "Must find Power Chord (5th)");
  TEST_ASSERT(foundP5Oct, "Must find Power Chord with Octave");
  TEST_ASSERT(foundP5b, "Must find Flat 5 Power Chord");
  TEST_ASSERT(foundP5Sharp, "Must find Sharp 5 Power Chord");
}

//-----------------------------------------------------------------------------
// Test 3: Triads (Major, Minor, Diminished, Augmented, Sus4, Sus2)
//-----------------------------------------------------------------------------
void TestTriads()
{
  cout << "[TEST] Running TestTriads..." << endl;

  const vector<JZChordDefinition>& chords = JZChordScaleLibrary::GetChords();
  int triadCount = 0;

  for (size_t i = 0; i < chords.size(); ++i)
  {
    if (chords[i].mShortName == "maj")
    {
      triadCount++;
      vector<int> expected = {0, 4, 7};
      TEST_ASSERT(chords[i].mIntervals == expected, "Major triad should be {0, 4, 7}");
    }
    else if (chords[i].mShortName == "m")
    {
      triadCount++;
      vector<int> expected = {0, 3, 7};
      TEST_ASSERT(chords[i].mIntervals == expected, "Minor triad should be {0, 3, 7}");
    }
    else if (chords[i].mShortName == "dim")
    {
      triadCount++;
      vector<int> expected = {0, 3, 6};
      TEST_ASSERT(chords[i].mIntervals == expected, "Diminished triad should be {0, 3, 6}");
    }
    else if (chords[i].mShortName == "aug")
    {
      triadCount++;
      vector<int> expected = {0, 4, 8};
      TEST_ASSERT(chords[i].mIntervals == expected, "Augmented triad should be {0, 4, 8}");
    }
    else if (chords[i].mShortName == "sus4")
    {
      triadCount++;
      vector<int> expected = {0, 5, 7};
      TEST_ASSERT(chords[i].mIntervals == expected, "Sus4 triad should be {0, 5, 7}");
    }
    else if (chords[i].mShortName == "sus2")
    {
      triadCount++;
      vector<int> expected = {0, 2, 7};
      TEST_ASSERT(chords[i].mIntervals == expected, "Sus2 triad should be {0, 2, 7}");
    }
  }

  TEST_ASSERT(triadCount == 6, "All 6 standard triads must be present");
}

//-----------------------------------------------------------------------------
// Test 4: Seventh Chords
//-----------------------------------------------------------------------------
void TestSeventhChords()
{
  cout << "[TEST] Running TestSeventhChords..." << endl;

  const vector<JZChordDefinition>& chords = JZChordScaleLibrary::GetChords();
  bool found7 = false;
  bool foundMaj7 = false;
  bool foundMin7 = false;
  bool foundM7b5 = false;
  bool foundDim7 = false;

  for (size_t i = 0; i < chords.size(); ++i)
  {
    if (chords[i].mShortName == "7")
    {
      found7 = true;
      vector<int> expected = {0, 4, 7, 10};
      TEST_ASSERT(chords[i].mIntervals == expected, "Dominant 7th should be {0, 4, 7, 10}");
    }
    else if (chords[i].mShortName == "maj7")
    {
      foundMaj7 = true;
      vector<int> expected = {0, 4, 7, 11};
      TEST_ASSERT(chords[i].mIntervals == expected, "Major 7th should be {0, 4, 7, 11}");
    }
    else if (chords[i].mShortName == "m7")
    {
      foundMin7 = true;
      vector<int> expected = {0, 3, 7, 10};
      TEST_ASSERT(chords[i].mIntervals == expected, "Minor 7th should be {0, 3, 7, 10}");
    }
    else if (chords[i].mShortName == "m7b5")
    {
      foundM7b5 = true;
      vector<int> expected = {0, 3, 6, 10};
      TEST_ASSERT(chords[i].mIntervals == expected, "Half-diminished 7th should be {0, 3, 6, 10}");
    }
    else if (chords[i].mShortName == "dim7")
    {
      foundDim7 = true;
      vector<int> expected = {0, 3, 6, 9};
      TEST_ASSERT(chords[i].mIntervals == expected, "Diminished 7th should be {0, 3, 6, 9}");
    }
  }

  TEST_ASSERT(found7, "Must find Dominant 7th");
  TEST_ASSERT(foundMaj7, "Must find Major 7th");
  TEST_ASSERT(foundMin7, "Must find Minor 7th");
  TEST_ASSERT(foundM7b5, "Must find Half-Diminished 7th");
  TEST_ASSERT(foundDim7, "Must find Diminished 7th");
}

//-----------------------------------------------------------------------------
// Test 5: Scales (Modes, Pentatonics, Blues, Bebop, World)
//-----------------------------------------------------------------------------
void TestScales()
{
  cout << "[TEST] Running TestScales..." << endl;

  const vector<JZScaleDefinition>& scales = JZChordScaleLibrary::GetScales();

  bool foundMajor = false;
  bool foundNaturalMinor = false;
  bool foundHarmonicMinor = false;
  bool foundMelodicMinor = false;
  bool foundMinorPent = false;
  bool foundBlues = false;
  bool foundBebopDom = false;
  bool foundByzantine = false;

  for (size_t i = 0; i < scales.size(); ++i)
  {
    const JZScaleDefinition& s = scales[i];
    TEST_ASSERT(!s.mIntervals.empty(), "Scale must have intervals");
    TEST_ASSERT(s.mIntervals[0] == 0, "Scale root must be 0");

    if (s.mName.find("Major / Ionian") != string::npos)
    {
      foundMajor = true;
      vector<int> expected = {0, 2, 4, 5, 7, 9, 11};
      TEST_ASSERT(s.mIntervals == expected, "Major scale intervals incorrect");
    }
    else if (s.mName.find("Natural Minor / Aeolian") != string::npos)
    {
      foundNaturalMinor = true;
      vector<int> expected = {0, 2, 3, 5, 7, 8, 10};
      TEST_ASSERT(s.mIntervals == expected, "Natural Minor scale intervals incorrect");
    }
    else if (s.mName.find("Harmonic Minor") != string::npos)
    {
      foundHarmonicMinor = true;
      vector<int> expected = {0, 2, 3, 5, 7, 8, 11};
      TEST_ASSERT(s.mIntervals == expected, "Harmonic Minor intervals incorrect");
    }
    else if (s.mName.find("Melodic Minor (Jazz Minor)") != string::npos)
    {
      foundMelodicMinor = true;
      vector<int> expected = {0, 2, 3, 5, 7, 9, 11};
      TEST_ASSERT(s.mIntervals == expected, "Melodic Minor intervals incorrect");
    }
    else if (s.mName.find("Minor Pentatonic") != string::npos)
    {
      foundMinorPent = true;
      vector<int> expected = {0, 3, 5, 7, 10};
      TEST_ASSERT(s.mIntervals == expected, "Minor Pentatonic intervals incorrect");
    }
    else if (s.mName.find("Blues Scale (Hexatonic)") != string::npos)
    {
      foundBlues = true;
      vector<int> expected = {0, 3, 5, 6, 7, 10};
      TEST_ASSERT(s.mIntervals == expected, "Blues scale intervals incorrect");
    }
    else if (s.mName.find("Bebop Dominant") != string::npos)
    {
      foundBebopDom = true;
      vector<int> expected = {0, 2, 4, 5, 7, 9, 10, 11};
      TEST_ASSERT(s.mIntervals == expected, "Bebop Dominant intervals incorrect");
    }
    else if (s.mName.find("Byzantine") != string::npos)
    {
      foundByzantine = true;
      vector<int> expected = {0, 1, 4, 5, 7, 8, 11};
      TEST_ASSERT(s.mIntervals == expected, "Byzantine intervals incorrect");
    }
  }

  TEST_ASSERT(foundMajor, "Must find Major scale");
  TEST_ASSERT(foundNaturalMinor, "Must find Natural Minor scale");
  TEST_ASSERT(foundHarmonicMinor, "Must find Harmonic Minor scale");
  TEST_ASSERT(foundMelodicMinor, "Must find Melodic Minor scale");
  TEST_ASSERT(foundMinorPent, "Must find Minor Pentatonic scale");
  TEST_ASSERT(foundBlues, "Must find Blues scale");
  TEST_ASSERT(foundBebopDom, "Must find Bebop Dominant scale");
  TEST_ASSERT(foundByzantine, "Must find Byzantine (Double Harmonic) scale");
}

//-----------------------------------------------------------------------------
// Test 6: Pitch and Note Name Conversions
//-----------------------------------------------------------------------------
void TestPitchAndNoteConversions()
{
  cout << "[TEST] Running TestPitchAndNoteConversions..." << endl;

  TEST_ASSERT(JZChordScaleLibrary::NoteNameToSemitone("C") == 0, "C should be semitone 0");
  TEST_ASSERT(JZChordScaleLibrary::NoteNameToSemitone("D") == 2, "D should be semitone 2");
  TEST_ASSERT(JZChordScaleLibrary::NoteNameToSemitone("E") == 4, "E should be semitone 4");
  TEST_ASSERT(JZChordScaleLibrary::NoteNameToSemitone("F") == 5, "F should be semitone 5");
  TEST_ASSERT(JZChordScaleLibrary::NoteNameToSemitone("G") == 7, "G should be semitone 7");
  TEST_ASSERT(JZChordScaleLibrary::NoteNameToSemitone("A") == 9, "A should be semitone 9");
  TEST_ASSERT(JZChordScaleLibrary::NoteNameToSemitone("B") == 11, "B should be semitone 11");

  TEST_ASSERT(JZChordScaleLibrary::PitchToNoteName(60) == "C4", "Pitch 60 should be C4");
  TEST_ASSERT(JZChordScaleLibrary::PitchToNoteName(69) == "A4", "Pitch 69 should be A4");
  TEST_ASSERT(JZChordScaleLibrary::PitchToNoteName(72) == "C5", "Pitch 72 should be C5");
  TEST_ASSERT(JZChordScaleLibrary::PitchToNoteName(48) == "C3", "Pitch 48 should be C3");
}

//-----------------------------------------------------------------------------
// Test 7: MIDI Files Integrity (Standard MThd header verification)
//-----------------------------------------------------------------------------
void TestMidiFilesIntegrity()
{
  cout << "[TEST] Running TestMidiFilesIntegrity..." << endl;

  const char* demoFiles[] = {
    "song/demo1.mid",
    "song/demo2.mid",
    "song/demo3.mid",
    "song/demo4.mid",
    "conf/jazz.mid"
  };

  for (size_t i = 0; i < sizeof(demoFiles) / sizeof(demoFiles[0]); ++i)
  {
    ifstream file(demoFiles[i], ios::binary);
    if (!file.is_open())
    {
      string altPath = string("../") + demoFiles[i];
      file.open(altPath, ios::binary);
    }
    TEST_ASSERT(file.is_open(), string("File should exist: ") + demoFiles[i]);

    if (file.is_open())
    {
      char header[4];
      file.read(header, 4);
      bool isMThd = (header[0] == 'M' && header[1] == 'T' && header[2] == 'h' && header[3] == 'd');
      TEST_ASSERT(isMThd, string("Header should be MThd for: ") + demoFiles[i]);
    }
  }
}

//-----------------------------------------------------------------------------
// Main Runner
//-----------------------------------------------------------------------------
int main()
{
  cout << "==========================================" << endl;
  cout << "       Jazz++ Core Unit Tests Runner      " << endl;
  cout << "==========================================" << endl;

  TestCountsAndCategories();
  TestPowerChords();
  TestTriads();
  TestSeventhChords();
  TestScales();
  TestPitchAndNoteConversions();
  TestMidiFilesIntegrity();

  cout << "==========================================" << endl;
  cout << "Tests Run:    " << gTestsRun << endl;
  cout << "Tests Passed: " << gTestsPassed << " [SUCCESS]" << endl;
  cout << "Tests Failed: " << gTestsFailed << endl;
  cout << "==========================================" << endl;

  return (gTestsFailed == 0) ? 0 : 1;
}
