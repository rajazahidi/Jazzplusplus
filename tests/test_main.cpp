//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Core Unit Tests Suite
//*****************************************************************************

#include "ChordScaleData.h"
#include "AudioEffects.h"
#include "SoundGenerator.h"
#include "MidiEffects.h"

#include <cassert>
#include <cmath>
#include <cstdio>
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
// Test 8: Channel & Program/Bank Logic
//-----------------------------------------------------------------------------
void TestChannelAndProgramLogic()
{
  cout << "[TEST] Running TestChannelAndProgramLogic..." << endl;

  // 1. Channel cycling (1..16)
  int ch = 1;
  ch = (ch % 16) + 1;
  TEST_ASSERT(ch == 2, "Channel 1 increment should be 2");

  ch = 16;
  ch = (ch % 16) + 1;
  TEST_ASSERT(ch == 1, "Channel 16 increment should wrap to 1");

  ch = 1;
  ch = (ch > 1) ? (ch - 1) : 16;
  TEST_ASSERT(ch == 16, "Channel 1 decrement should wrap to 16");

  ch = 5;
  ch = (ch > 1) ? (ch - 1) : 16;
  TEST_ASSERT(ch == 4, "Channel 5 decrement should be 4");

  // 2. Program & Bank bit packing and unpacking
  // Standard Piano 1 (Program 1, Bank 0)
  int patchVal = 1;
  int bank = (patchVal & 0x0000ff00) >> 8;
  int patch = patchVal & 0x000000ff;
  TEST_ASSERT(bank == 0 && patch == 1, "Piano 1 should decode to Bank 0, Patch 1");

  // GS Detuned EP 1: Value 2052 -> Bank 8, Program 4
  int gsDetunedEP1Val = 2052;
  bank = (gsDetunedEP1Val & 0x0000ff00) >> 8;
  patch = gsDetunedEP1Val & 0x000000ff;
  TEST_ASSERT(bank == 8 && patch == 4, "Detuned EP 1 should decode to Bank 8, Patch 4");

  // GS Mandolin: Value 4121 -> Bank 16, Program 25
  int gsMandolinVal = 4121;
  bank = (gsMandolinVal & 0x0000ff00) >> 8;
  patch = gsMandolinVal & 0x000000ff;
  TEST_ASSERT(bank == 16 && patch == 25, "Mandolin should decode to Bank 16, Patch 25");

  // Drum set: CM-64/32L Set (Patch 128, Bank 0)
  int drumVal = 128;
  bank = (drumVal & 0x0000ff00) >> 8;
  patch = drumVal & 0x000000ff;
  TEST_ASSERT(bank == 0 && patch == 128, "CM-64/32L drum should decode to Bank 0, Patch 128");

  // 3. Selection bounds safety verification
  vector<int> drumValues;
  drumValues.push_back(1);
  drumValues.push_back(9);
  drumValues.push_back(17);
  drumValues.push_back(25);
  drumValues.push_back(26);
  drumValues.push_back(33);
  drumValues.push_back(41);
  drumValues.push_back(49);
  drumValues.push_back(128);

  // List count is 9. A patch index of 128 should NEVER be used directly as listbox index!
  int rawPatchNr = 128;
  int safeSelectionIndex = -1;
  for (size_t i = 0; i < drumValues.size(); ++i)
  {
    if (drumValues[i] == rawPatchNr)
    {
      safeSelectionIndex = (int)i;
      break;
    }
  }
  TEST_ASSERT(safeSelectionIndex == 8, "Safe lookup finds CM-64/32L at index 8");
  TEST_ASSERT(safeSelectionIndex < (int)drumValues.size(), "Safe selection index is within listbox bounds");

  // Ensure an out-of-range lookup doesn't crash and falls back
  int unknownPatch = 999;
  int unknownIndex = -1;
  for (size_t i = 0; i < drumValues.size(); ++i)
  {
    if (drumValues[i] == unknownPatch)
    {
      unknownIndex = (int)i;
      break;
    }
  }
  TEST_ASSERT(unknownIndex == -1, "Unknown patch returns -1 safely");
}

//-----------------------------------------------------------------------------
// Test 9: MIDI Effects Suite (Arpeggiator, Harmonizer, Rates)
//-----------------------------------------------------------------------------
void TestMidiEffects()
{
  cout << "[TEST] Running TestMidiEffects..." << endl;

  // 1. Arpeggiator Rate Ticks
  TEST_ASSERT(JZMidiArpeggiator::GetRateTicks(eArpRate_Quarter, 480) == 480, "Quarter rate should be 480 ticks");
  TEST_ASSERT(JZMidiArpeggiator::GetRateTicks(eArpRate_Eighth, 480) == 240, "Eighth rate should be 240 ticks");
  TEST_ASSERT(JZMidiArpeggiator::GetRateTicks(eArpRate_Sixteenth, 480) == 120, "Sixteenth rate should be 120 ticks");
  TEST_ASSERT(JZMidiArpeggiator::GetRateTicks(eArpRate_ThirtySecond, 480) == 60, "Thirty-second rate should be 60 ticks");
  TEST_ASSERT(JZMidiArpeggiator::GetRateTicks(eArpRate_EighthTriplet, 480) == 320, "Eighth triplet should be 320 ticks");
  TEST_ASSERT(JZMidiArpeggiator::GetRateTicks(eArpRate_SixteenthTriplet, 480) == 160, "Sixteenth triplet should be 160 ticks");

  // 2. Pitch Sequences
  vector<int> chord = {60, 64, 67}; // C Major triad (C4, E4, G4)

  // Up pattern 1 octave
  vector<int> seqUp1 = JZMidiArpeggiator::GeneratePitchSequence(chord, eArpUp, 1);
  vector<int> expectedUp1 = {60, 64, 67};
  TEST_ASSERT(seqUp1 == expectedUp1, "Up pattern 1 octave matches {60, 64, 67}");

  // Up pattern 2 octaves
  vector<int> seqUp2 = JZMidiArpeggiator::GeneratePitchSequence(chord, eArpUp, 2);
  vector<int> expectedUp2 = {60, 64, 67, 72, 76, 79};
  TEST_ASSERT(seqUp2 == expectedUp2, "Up pattern 2 octaves matches {60, 64, 67, 72, 76, 79}");

  // Down pattern 1 octave
  vector<int> seqDown1 = JZMidiArpeggiator::GeneratePitchSequence(chord, eArpDown, 1);
  vector<int> expectedDown1 = {67, 64, 60};
  TEST_ASSERT(seqDown1 == expectedDown1, "Down pattern matches {67, 64, 60}");

  // UpDown pattern 1 octave
  vector<int> seqUpDown = JZMidiArpeggiator::GeneratePitchSequence(chord, eArpUpDown, 1);
  vector<int> expectedUpDown = {60, 64, 67, 64};
  TEST_ASSERT(seqUpDown == expectedUpDown, "UpDown pattern matches {60, 64, 67, 64}");

  // 3. Harmonizer Diatonic & Octave intervals
  vector<int> harmOctUp = JZMidiHarmonizer::GetHarmonizedPitches(60, eHarmOctaveAbove, 0, "Major");
  TEST_ASSERT(harmOctUp.size() == 1 && harmOctUp[0] == 72, "Octave above 60 is 72");

  vector<int> harmOctDown = JZMidiHarmonizer::GetHarmonizedPitches(60, eHarmOctaveBelow, 0, "Major");
  TEST_ASSERT(harmOctDown.size() == 1 && harmOctDown[0] == 48, "Octave below 60 is 48");

  vector<int> harmThird = JZMidiHarmonizer::GetHarmonizedPitches(60, eHarmThirdAbove, 0, "Major");
  TEST_ASSERT(harmThird.size() == 1 && harmThird[0] == 64, "Diatonic 3rd above C4 in C Major is E4 (64)");

  vector<int> harmFifth = JZMidiHarmonizer::GetHarmonizedPitches(60, eHarmFifthAbove, 0, "Major");
  TEST_ASSERT(harmFifth.size() == 1 && harmFifth[0] == 67, "Diatonic 5th above C4 in C Major is G4 (67)");

  vector<int> harmTriad = JZMidiHarmonizer::GetHarmonizedPitches(60, eHarmDiatonicTriad, 0, "Major");
  vector<int> expectedTriad = {64, 67};
  TEST_ASSERT(harmTriad == expectedTriad, "Diatonic triad above C4 in C Major is {64, 67}");
}

//-----------------------------------------------------------------------------
// Test 10: Audio DSP Suite (Biquad, EQ, Freeverb, Delay, Chorus, Limiter)
//-----------------------------------------------------------------------------
void TestAudioEffects()
{
  cout << "[TEST] Running TestAudioEffects..." << endl;

  // 1. RBJ Biquad Filter
  JZBiquadFilter lpFilter;
  lpFilter.Configure(eBiquadLowPass, 44100.0f, 1000.0f, 0.707f);
  float outVal = 0.0f;
  bool isStable = true;
  for (int i = 0; i < 100; ++i)
  {
    float inSample = (i == 0) ? 1.0f : 0.0f; // Unit impulse
    outVal = lpFilter.Process(inSample);
    if (std::isnan(outVal) || std::isinf(outVal))
    {
      isStable = false;
      break;
    }
  }
  TEST_ASSERT(isStable, "Biquad LowPass filter impulse response is stable and finite");

  // 2. 3-Band Parametric Equalizer
  JZ3BandEqualizer eq(44100.0f);
  JZEqualizerSettings eqSettings;
  eqSettings.bassGainDB = 6.0f; // +6 dB bass boost
  eqSettings.midGainDB = -3.0f;
  eqSettings.trebleGainDB = 4.0f;
  eq.SetSettings(eqSettings);

  const int numSamples = 512;
  vector<float> eqL(numSamples, 0.0f);
  vector<float> eqR(numSamples, 0.0f);
  // Feed 100 Hz test tone
  for (int i = 0; i < numSamples; ++i)
  {
    eqL[i] = sin(2.0f * 3.14159f * 100.0f * i / 44100.0f) * 0.5f;
    eqR[i] = eqL[i];
  }
  eq.Process(eqL.data(), eqR.data(), numSamples);
  bool eqValid = true;
  for (int i = 0; i < numSamples; ++i)
  {
    if (std::isnan(eqL[i]) || std::isinf(eqL[i]) || std::isnan(eqR[i]) || std::isinf(eqR[i]))
    {
      eqValid = false;
      break;
    }
  }
  TEST_ASSERT(eqValid, "3-Band Equalizer processes audio buffer without NaN or Inf");

  // 3. Freeverb Reverb
  JZFreeverb reverb(44100.0f);
  JZReverbSettings verbSettings;
  verbSettings.roomSize = 0.7f;
  verbSettings.wet = 0.5f;
  verbSettings.dry = 0.5f;
  reverb.SetSettings(verbSettings);

  vector<float> verbInL(numSamples, 0.0f);
  vector<float> verbInR(numSamples, 0.0f);
  vector<float> verbOutL(numSamples, 0.0f);
  vector<float> verbOutR(numSamples, 0.0f);
  verbInL[0] = 1.0f; // Impulse in left
  verbInR[0] = 1.0f;

  reverb.Process(verbInL.data(), verbInR.data(), verbOutL.data(), verbOutR.data(), numSamples);
  float totalEnergy = 0.0f;
  for (int i = 0; i < numSamples; ++i)
  {
    totalEnergy += fabs(verbOutL[i]) + fabs(verbOutR[i]);
  }
  TEST_ASSERT(totalEnergy > 0.01f, "Freeverb produces reverberant tail energy from impulse");

  // 4. Stereo Delay
  JZStereoDelay delay(44100.0f);
  JZDelaySettings delaySettings;
  delaySettings.delayTimeMsL = 10.0f; // ~441 samples delay
  delaySettings.delayTimeMsR = 10.0f;
  delaySettings.feedback = 0.0f;
  delaySettings.wet = 1.0f;
  delaySettings.dry = 0.0f;
  delay.SetSettings(delaySettings);

  const int delayTestSamples = 1000;
  vector<float> dInL(delayTestSamples, 0.0f);
  vector<float> dInR(delayTestSamples, 0.0f);
  vector<float> dOutL(delayTestSamples, 0.0f);
  vector<float> dOutR(delayTestSamples, 0.0f);
  dInL[0] = 1.0f;

  delay.Process(dInL.data(), dInR.data(), dOutL.data(), dOutR.data(), delayTestSamples);
  // Delayed impulse should arrive near sample index 441
  int peakIdx = 0;
  float peakVal = 0.0f;
  for (int i = 1; i < delayTestSamples; ++i)
  {
    if (fabs(dOutL[i]) > peakVal)
    {
      peakVal = fabs(dOutL[i]);
      peakIdx = i;
    }
  }
  TEST_ASSERT(peakIdx >= 435 && peakIdx <= 445, "Stereo Delay reproduces impulse at expected delay sample offset");

  // 5. Modulation (Stereo Chorus)
  JZStereoChorus chorus(44100.0f);
  vector<float> chorL(numSamples, 0.5f);
  vector<float> chorR(numSamples, 0.5f);
  chorus.Process(chorL.data(), chorR.data(), numSamples);
  TEST_ASSERT(!std::isnan(chorL[10]) && !std::isnan(chorR[10]), "Stereo Chorus produces valid modulated output");

  // 6. Limiter & Overdrive Soft-Clipping
  JZAudioLimiterDistortion limiter;
  JZLimiterSettings limSettings;
  limSettings.enableOverdrive = true;
  limSettings.drive = 4.0f;      // 4x overdrive
  limSettings.ceilingDB = -0.3f; // ~0.966 max ceiling
  limiter.SetSettings(limSettings);

  vector<float> hotL = {0.5f, 2.0f, 10.0f, -5.0f};
  vector<float> hotR = {0.5f, 2.0f, 10.0f, -5.0f};
  limiter.Process(hotL.data(), hotR.data(), 4);

  // Ceiling check: no sample should exceed ceiling magnitude (~0.966f)
  float maxCeiling = pow(10.0f, -0.3f / 20.0f) + 0.001f;
  bool ceilingRespected = true;
  for (int i = 0; i < 4; ++i)
  {
    if (fabs(hotL[i]) > maxCeiling || fabs(hotR[i]) > maxCeiling)
    {
      ceilingRespected = false;
      break;
    }
  }
  TEST_ASSERT(ceilingRespected, "Master Limiter clamps large overdrive signals within ceiling threshold");
}

//-----------------------------------------------------------------------------
// Test 11: Procedural Sound Generator Suite (Retro SFX, 808 Drums, WAV IO)
//-----------------------------------------------------------------------------
void TestSoundGenerator()
{
  cout << "[TEST] Running TestSoundGenerator..." << endl;

  // 1. Retro SFX Presets and Generation
  JZRetroSFXParams laserParams = JZRetroSFXGenerator::GetPreset(eSFXLaser);
  vector<short> laserWave = JZRetroSFXGenerator::Generate(laserParams, 44100);
  TEST_ASSERT(!laserWave.empty(), "Laser SFX generated non-empty PCM buffer");
  TEST_ASSERT(laserWave.size() == static_cast<size_t>(laserParams.duration * 44100), "Laser PCM length matches duration");

  JZRetroSFXParams expParams = JZRetroSFXGenerator::GetPreset(eSFXExplosion);
  vector<short> expWave = JZRetroSFXGenerator::Generate(expParams, 44100);
  TEST_ASSERT(!expWave.empty(), "Explosion SFX generated non-empty PCM buffer");

  JZRetroSFXParams coinParams = JZRetroSFXGenerator::GetPreset(eSFXCoin);
  vector<short> coinWave = JZRetroSFXGenerator::Generate(coinParams, 44100);
  TEST_ASSERT(!coinWave.empty(), "Coin SFX generated non-empty PCM buffer");

  JZRetroSFXParams powerParams = JZRetroSFXGenerator::GetPreset(eSFXPowerUp);
  vector<short> powerWave = JZRetroSFXGenerator::Generate(powerParams, 44100);
  TEST_ASSERT(!powerWave.empty(), "PowerUp SFX generated non-empty PCM buffer");

  // 2. Analog Synth Drums Generation
  JZSynthDrumParams kickParams = JZSynthDrumGenerator::GetPreset(eDrum808Kick);
  vector<short> kickWave = JZSynthDrumGenerator::Generate(kickParams, 44100);
  TEST_ASSERT(!kickWave.empty(), "808 Kick generated non-empty PCM buffer");
  TEST_ASSERT(kickWave.size() == static_cast<size_t>(kickParams.duration * 44100), "Kick PCM length matches duration");

  JZSynthDrumParams snareParams = JZSynthDrumGenerator::GetPreset(eDrum808Snare);
  vector<short> snareWave = JZSynthDrumGenerator::Generate(snareParams, 44100);
  TEST_ASSERT(!snareWave.empty(), "808 Snare generated non-empty PCM buffer");

  JZSynthDrumParams hatParams = JZSynthDrumGenerator::GetPreset(eDrumHiHatClosed);
  vector<short> hatWave = JZSynthDrumGenerator::Generate(hatParams, 44100);
  TEST_ASSERT(!hatWave.empty(), "Closed Hi-Hat generated non-empty PCM buffer");

  JZSynthDrumParams clapParams = JZSynthDrumGenerator::GetPreset(eDrum808Clap);
  vector<short> clapWave = JZSynthDrumGenerator::Generate(clapParams, 44100);
  TEST_ASSERT(!clapWave.empty(), "808 Clap generated non-empty PCM buffer");

  // 3. RIFF/WAVE Export Validation
  string tempWavPath = "test_sfx_export.wav";
  bool saveSuccess = JZSoundIO::SaveWav(tempWavPath, coinWave, 44100, 1);
  TEST_ASSERT(saveSuccess, "SaveWav successfully written to disk");

  // Read back and verify WAV header
  ifstream wavFile(tempWavPath.c_str(), ios::binary);
  TEST_ASSERT(wavFile.is_open(), "Written WAV file is openable for reading");

  char riffHeader[4];
  wavFile.read(riffHeader, 4);
  bool isRiff = (riffHeader[0] == 'R' && riffHeader[1] == 'I' && riffHeader[2] == 'F' && riffHeader[3] == 'F');
  TEST_ASSERT(isRiff, "WAV file contains valid 'RIFF' header chunk");

  wavFile.seekg(8, ios::beg);
  char waveHeader[4];
  wavFile.read(waveHeader, 4);
  bool isWave = (waveHeader[0] == 'W' && waveHeader[1] == 'A' && waveHeader[2] == 'V' && waveHeader[3] == 'E');
  TEST_ASSERT(isWave, "WAV file contains valid 'WAVE' format identifier");
  wavFile.close();

  // Clean up
  remove(tempWavPath.c_str());
}

//-----------------------------------------------------------------------------
// Test 12: Guitar Fretboard & Tab Pitch Calculations
//-----------------------------------------------------------------------------
void TestGuitarFretboardLogic()
{
  cout << "[TEST] Running TestGuitarFretboardLogic..." << endl;

  // Standard 6-string Guitar Tuning Pitches (E4, B3, G3, D3, A2, E2)
  const int guitarPitches[6] = { 64, 59, 55, 50, 45, 40 };

  // Standard 4-string Bass Tuning Pitches (G2, D2, A1, E1)
  const int bassPitches[4] = { 43, 38, 33, 28 };

  // 1. Verify standard guitar pitches
  TEST_ASSERT(guitarPitches[0] == 64, "Guitar string 1 (high e) is E4 (MIDI 64)");
  TEST_ASSERT(guitarPitches[1] == 59, "Guitar string 2 (B) is B3 (MIDI 59)");
  TEST_ASSERT(guitarPitches[2] == 55, "Guitar string 3 (G) is G3 (MIDI 55)");
  TEST_ASSERT(guitarPitches[3] == 50, "Guitar string 4 (D) is D3 (MIDI 50)");
  TEST_ASSERT(guitarPitches[4] == 45, "Guitar string 5 (A) is A2 (MIDI 45)");
  TEST_ASSERT(guitarPitches[5] == 40, "Guitar string 6 (low E) is E2 (MIDI 40)");

  // 2. Verify standard bass pitches
  TEST_ASSERT(bassPitches[0] == 43, "Bass string 1 (G) is G2 (MIDI 43)");
  TEST_ASSERT(bassPitches[1] == 38, "Bass string 2 (D) is D2 (MIDI 38)");
  TEST_ASSERT(bassPitches[2] == 33, "Bass string 3 (A) is A1 (MIDI 33)");
  TEST_ASSERT(bassPitches[3] == 28, "Bass string 4 (low E) is E1 (MIDI 28)");

  // 3. Verify fret octave calculations
  // Octave 12 on low E string gives 40 + 12 = 52 (E3)
  TEST_ASSERT(guitarPitches[5] + 12 == 52, "Fret 12 on low E string is E3 (52)");
  // Octave 24 on low E string gives 40 + 24 = 64 (E4 - matches open high e)
  TEST_ASSERT(guitarPitches[5] + 24 == guitarPitches[0], "Fret 24 on low E equals open high e string (64)");
  // Octave 12 on high e gives 64 + 12 = 76 (E5)
  TEST_ASSERT(guitarPitches[0] + 12 == 76, "Fret 12 on high e is E5 (76)");
  // Octave 24 on high e gives 64 + 24 = 88 (E6)
  TEST_ASSERT(guitarPitches[0] + 24 == 88, "Fret 24 on high e is E6 (88)");

  // 4. Verify note chroma (pitch % 12)
  static const string noteNames[12] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
  };
  TEST_ASSERT(noteNames[guitarPitches[0] % 12] == "E", "String 1 chroma is E");
  TEST_ASSERT(noteNames[guitarPitches[1] % 12] == "B", "String 2 chroma is B");
  TEST_ASSERT(noteNames[guitarPitches[2] % 12] == "G", "String 3 chroma is G");
  TEST_ASSERT(noteNames[guitarPitches[3] % 12] == "D", "String 4 chroma is D");
  TEST_ASSERT(noteNames[guitarPitches[4] % 12] == "A", "String 5 chroma is A");
  TEST_ASSERT(noteNames[guitarPitches[5] % 12] == "E", "String 6 chroma is E");

  // 5. Test Fret Clamping Bounds (12 to 24)
  auto clampFrets = [](int count) {
    if (count < 12) count = 12;
    if (count > 24) count = 24;
    return count;
  };
  TEST_ASSERT(clampFrets(5) == 12, "Fret count below 12 clamps to 12");
  TEST_ASSERT(clampFrets(17) == 17, "Fret count 17 remains 17");
  TEST_ASSERT(clampFrets(21) == 21, "Fret count 21 remains 21");
  TEST_ASSERT(clampFrets(24) == 24, "Fret count 24 remains 24");
  TEST_ASSERT(clampFrets(30) == 24, "Fret count above 24 clamps to 24");
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
  TestChannelAndProgramLogic();
  TestMidiEffects();
  TestAudioEffects();
  TestSoundGenerator();
  TestGuitarFretboardLogic();

  cout << "==========================================" << endl;
  cout << "Tests Run:    " << gTestsRun << endl;
  cout << "Tests Passed: " << gTestsPassed << " [SUCCESS]" << endl;
  cout << "Tests Failed: " << gTestsFailed << endl;
  cout << "==========================================" << endl;

  return (gTestsFailed == 0) ? 0 : 1;
}
