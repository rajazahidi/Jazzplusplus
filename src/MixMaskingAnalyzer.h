//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Intelligent Cross-Track Frequency Masking Analyzer
//
// Multi-track spectral clash detector comparing frequency overlaps across
// 32 logarithmic bands, calculating masking severity indexes, generating
// collision heatmaps, and providing smart unmasking EQ recommendations.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <map>
#include <cmath>
#include <algorithm>

// Single Masking Collision Report
struct JZMaskingClash
{
  int maskedTrackId;          // Track being obscured (e.g. Lead Vocal)
  std::string maskedTrackName;
  int maskerTrackId;          // Track causing collision (e.g. Electric Guitars)
  std::string maskerTrackName;
  float clashFreqHz;          // Frequency center of clash
  float severityPercent;      // 0% to 100% collision severity
  float recommendedCutDB;     // Suggested EQ notch cut on masker track
  std::string advice;

  JZMaskingClash()
    : maskedTrackId(0),
      maskerTrackId(0),
      clashFreqHz(1000.0f),
      severityPercent(0.0f),
      recommendedCutDB(-2.5f)
  {
  }
};

// Track Spectral Profile
struct JZTrackSpectrum
{
  int trackId;
  std::string trackName;
  int priorityLevel;          // 1: Highest (Vocal/Lead), 2: High (Bass/Kick), 3: Normal, 4: Background
  float bandEnergiesDB[32];

  JZTrackSpectrum()
    : trackId(0),
      trackName("Track"),
      priorityLevel(3)
  {
    for (int i = 0; i < 32; ++i) bandEnergiesDB[i] = -96.0f;
  }
};

// Mix Masking Analyzer Class
class JZMixMaskingAnalyzer
{
public:
  static const int NUM_ANALYSIS_BANDS = 32;

  JZMixMaskingAnalyzer(float sampleRate = 44100.0f);
  ~JZMixMaskingAnalyzer() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  void Reset();

  // Register track for multi-track collision tracking
  void RegisterTrack(int trackId, const std::string& name, int priority = 3);
  void RemoveTrack(int trackId);

  // Feed block of audio for track
  void FeedAudioBlock(int trackId, const float* audio, int numSamples);

  // Run cross-track spectral masking analysis
  std::vector<JZMaskingClash> AnalyzeClashes(float minSeverityThreshold = 25.0f);

  // Get active track count
  int GetTrackCount() const { return (int)mTracks.size(); }

  // Static helper to map band index (0 to 31) to center frequency
  static float BandIndexToFreq(int bandIndex, float sampleRate);

private:
  float mSampleRate;
  std::map<int, JZTrackSpectrum> mTracks;
};
