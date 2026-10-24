//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Automatic Vocal Timing Aligner Engine (VocAlign Style)
//
// Automated vocal double & harmony alignment engine: analyzes Guide vocal
// energy trajectory, aligns Dub vocal consonant transients via Dynamic Time
// Warping (DTW), and locks backing vocals tight in sync without pitch artifacts.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

// Vocal Alignment Results Summary
struct JZVocalAlignReport
{
  float initialDriftMs;       // Timing offset before alignment
  float finalDriftMs;         // Remaining timing offset after alignment (near 0ms)
  float alignmentAccuracy;    // 0% to 100%
  int warpSegmentsCount;
  bool success;

  JZVocalAlignReport()
    : initialDriftMs(0.0f),
      finalDriftMs(0.0f),
      alignmentAccuracy(0.0f),
      warpSegmentsCount(0),
      success(false)
  {
  }
};

// Vocal Aligner Class
class JZVocalAligner
{
public:
  JZVocalAligner(float sampleRate = 44100.0f);
  ~JZVocalAligner() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  // Align Dub vocal audio buffer to match Guide vocal audio timing
  bool AlignVocals(
    const std::vector<float>& guideAudio,
    const std::vector<float>& dubAudio,
    std::vector<float>& outAlignedDub,
    JZVocalAlignReport& outReport);

  // Energy envelope extraction helper
  static void ExtractEnvelope(
    const std::vector<float>& audio,
    int windowSamples,
    std::vector<float>& outEnvelope);

private:
  float mSampleRate;
};
