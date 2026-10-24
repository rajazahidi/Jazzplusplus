//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Automatic Song Key, Scale & Chord Progression Timeline Detector
//
// Harmonic intelligence engine featuring:
// 1. 12-bin Pitch Class Profile (Chromagram) extraction
// 2. Krumhansl-Schmuckler Key-Finding Algorithm (24 Major/Minor keys + confidence score)
// 3. Automated timeline chord progression detection (Triads & 7th chords)
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

struct JZDetectedKeyResult
{
  std::string rootNote;       // e.g. "C", "F#", "A"
  std::string mode;           // "Major" or "Minor"
  std::string fullKeyName;    // e.g. "C Major", "A Minor"
  float confidenceScore;      // 0.0% to 100.0%
  int rootMidiPitch;          // 0 = C, 1 = C#, ... 11 = B
  bool isMajor;

  JZDetectedKeyResult()
    : rootNote("C"),
      mode("Major"),
      fullKeyName("C Major"),
      confidenceScore(0.0f),
      rootMidiPitch(0),
      isMajor(true)
  {
  }
};

struct JZTimelineChord
{
  float startTimeSec;
  float durationSec;
  std::string chordName;      // e.g. "Cmaj7", "Am", "Dm7", "G7"
  int rootPitch;              // 0 to 11
};

class JZKeyChordDetector
{
public:
  JZKeyChordDetector(float sampleRate = 44100.0f);
  ~JZKeyChordDetector() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  // Analyze audio and extract Chromagram + Key & Chords
  bool AnalyzeAudio(const std::vector<float>& audio);

  // Analyze MIDI Note Events directly
  bool AnalyzeMidiNotes(const std::vector<int>& midiPitches);

  // Retrieve Analysis Results
  const JZDetectedKeyResult& GetDetectedKey() const { return mDetectedKey; }
  const std::vector<float>& GetChromagram() const { return mChromagram; }
  const std::vector<JZTimelineChord>& GetChordProgression() const { return mProgression; }

private:
  float mSampleRate;
  JZDetectedKeyResult mDetectedKey;
  std::vector<float> mChromagram; // 12 bins
  std::vector<JZTimelineChord> mProgression;

  void ComputeKeyFromChromagram();
  std::string IdentifyChord(const std::vector<float>& localChroma);
};
