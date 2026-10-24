//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Automatic Song Key, Scale & Chord Timeline Detector Implementation
//*****************************************************************************

#include "KeyChordDetector.h"
#include <numeric>
#include <cmath>
#include <algorithm>

static const char* kPitchNames[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

// Standard Krumhansl-Schmuckler Key Profiles
static const float kMajorProfile[12] = {6.35f, 2.23f, 3.48f, 2.33f, 4.38f, 4.09f, 2.52f, 5.19f, 2.39f, 3.66f, 2.29f, 2.88f};
static const float kMinorProfile[12] = {6.33f, 2.68f, 3.52f, 5.38f, 2.60f, 3.53f, 2.54f, 4.75f, 3.98f, 2.69f, 3.34f, 3.17f};

JZKeyChordDetector::JZKeyChordDetector(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f)
{
  mChromagram.assign(12, 0.0f);
}

void JZKeyChordDetector::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
  }
}

void JZKeyChordDetector::ComputeKeyFromChromagram()
{
  float chromaSum = std::accumulate(mChromagram.begin(), mChromagram.end(), 0.0f);
  if (chromaSum < 0.00001f)
  {
    mDetectedKey = JZDetectedKeyResult();
    return;
  }

  // Normalize chromagram
  std::vector<float> normChroma(12);
  for (int i = 0; i < 12; ++i)
  {
    normChroma[i] = mChromagram[i] / chromaSum;
  }

  float bestCorrelation = -2.0f;
  int bestRoot = 0;
  bool bestIsMajor = true;

  float chromaMean = 1.0f / 12.0f;

  // Test all 12 Major and 12 Minor keys
  for (int mode = 0; mode < 2; ++mode)
  {
    bool isMajorMode = (mode == 0);
    const float* profile = isMajorMode ? kMajorProfile : kMinorProfile;

    float profSum = 0.0f;
    for (int i = 0; i < 12; ++i) profSum += profile[i];
    float profMean = profSum / 12.0f;

    for (int root = 0; root < 12; ++root)
    {
      float num = 0.0f;
      float denChroma = 0.0f;
      float denProf = 0.0f;

      for (int i = 0; i < 12; ++i)
      {
        float cVal = normChroma[i] - chromaMean;
        float pVal = profile[(i - root + 12) % 12] - profMean;

        num += cVal * pVal;
        denChroma += cVal * cVal;
        denProf += pVal * pVal;
      }

      float denom = std::sqrt(denChroma * denProf);
      float r = (denom > 0.000001f) ? (num / denom) : 0.0f;

      if (r > bestCorrelation)
      {
        bestCorrelation = r;
        bestRoot = root;
        bestIsMajor = isMajorMode;
      }
    }
  }

  mDetectedKey.rootMidiPitch = bestRoot;
  mDetectedKey.rootNote = kPitchNames[bestRoot];
  mDetectedKey.mode = bestIsMajor ? "Major" : "Minor";
  mDetectedKey.isMajor = bestIsMajor;
  mDetectedKey.fullKeyName = mDetectedKey.rootNote + " " + mDetectedKey.mode;
  mDetectedKey.confidenceScore = std::clamp((bestCorrelation * 0.5f + 0.5f) * 100.0f, 0.0f, 99.8f);
}

std::string JZKeyChordDetector::IdentifyChord(const std::vector<float>& localChroma)
{
  if (localChroma.size() < 12) return "N/C";

  int root = 0;
  float maxVal = localChroma[0];
  for (int i = 1; i < 12; ++i)
  {
    if (localChroma[i] > maxVal)
    {
      maxVal = localChroma[i];
      root = i;
    }
  }

  // Check 3rd (Major 3rd = root + 4, Minor 3rd = root + 3)
  float maj3 = localChroma[(root + 4) % 12];
  float min3 = localChroma[(root + 3) % 12];

  std::string chord = kPitchNames[root];
  if (maj3 >= min3)
  {
    chord += "maj";
  }
  else
  {
    chord += "m";
  }

  return chord;
}

bool JZKeyChordDetector::AnalyzeAudio(const std::vector<float>& audio)
{
  mChromagram.assign(12, 0.0f);
  mProgression.clear();

  if (audio.size() < static_cast<size_t>(mSampleRate * 0.2f))
  {
    return false;
  }

  // Energy distribution estimation across 12 pitch classes
  size_t blockSize = 1024;
  for (size_t i = 0; i + blockSize <= audio.size(); i += blockSize)
  {
    std::vector<float> blockChroma(12, 0.0001f);
    for (int pc = 0; pc < 12; ++pc)
    {
      float freq = 261.63f * std::pow(2.0f, pc / 12.0f); // C4 octave reference
      float alpha = std::exp(-2.0f * 3.14159265f * freq / mSampleRate);
      float state = 0.0f;
      float energy = 0.0f;

      for (size_t s = 0; s < blockSize; ++s)
      {
        float val = audio[i + s];
        state = alpha * state + (1.0f - alpha) * val;
        energy += state * state;
      }
      blockChroma[pc] = energy;
      mChromagram[pc] += energy;
    }

    // Add timeline chord every 1.5 seconds
    if (i % static_cast<size_t>(mSampleRate * 1.5f) < blockSize)
    {
      JZTimelineChord tc;
      tc.startTimeSec = static_cast<float>(i) / mSampleRate;
      tc.durationSec = 1.5f;
      tc.chordName = IdentifyChord(blockChroma);
      mProgression.push_back(tc);
    }
  }

  ComputeKeyFromChromagram();
  return true;
}

bool JZKeyChordDetector::AnalyzeMidiNotes(const std::vector<int>& midiPitches)
{
  mChromagram.assign(12, 0.0f);
  mProgression.clear();

  if (midiPitches.empty())
  {
    return false;
  }

  for (int p : midiPitches)
  {
    int pc = (p % 12 + 12) % 12;
    mChromagram[pc] += 1.0f;
  }

  ComputeKeyFromChromagram();
  return true;
}
