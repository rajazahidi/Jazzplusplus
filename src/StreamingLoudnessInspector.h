//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Streaming Loudness Target & True-Peak Inspector
//
// Industry compliance analyzer comparing mix loudness against official streaming targets:
// Spotify (-14 LUFS), Apple Music (-16 LUFS), YouTube (-14 LUFS), Tidal (-14 LUFS),
// and EBU R128 Broadcast (-23 LUFS), with gain penalty prediction and one-click normalization.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

enum class JZStreamingPlatform
{
  Spotify = 0,
  AppleMusic,
  YouTube,
  Tidal,
  BroadcastEbu
};

struct JZPlatformCompliance
{
  std::string platformName;
  float targetLufs;
  float targetMaxTruePeakDbTP;
  float gainPenaltyDb;        // Negative means platform turns track DOWN
  bool isCompliant;
  float recommendedGainAdjustmentDb;

  JZPlatformCompliance(const std::string& name = "Spotify", float lufs = -14.0f, float tp = -1.0f)
    : platformName(name),
      targetLufs(lufs),
      targetMaxTruePeakDbTP(tp),
      gainPenaltyDb(0.0f),
      isCompliant(true),
      recommendedGainAdjustmentDb(0.0f)
  {
  }
};

class JZStreamingLoudnessInspector
{
public:
  JZStreamingLoudnessInspector(float sampleRate = 44100.0f);
  ~JZStreamingLoudnessInspector() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  // Analyze audio stream
  void Reset();
  void ProcessBlock(const std::vector<float>& left, const std::vector<float>& right);

  // Metering Results
  float GetIntegratedLufs() const;
  float GetShortTermLufs() const { return mShortTermLufs; }
  float GetLoudnessRangeLra() const { return mLoudnessRangeLra; }
  float GetMaxTruePeakDbTP() const { return mMaxTruePeakDbTP; }

  // Platform Compliance Inspection
  std::vector<JZPlatformCompliance> GenerateComplianceReport() const;
  float GetOneClickNormalizationGain(JZStreamingPlatform platform) const;

private:
  float mSampleRate;
  double mTotalSumSqL;
  double mTotalSumSqR;
  size_t mTotalSampleCount;

  float mShortTermLufs;
  float mLoudnessRangeLra;
  float mMaxTruePeakDbTP;

  // K-Weighting Filter States
  float mStage1L_x1, mStage1L_x2, mStage1L_y1, mStage1L_y2;
  float mStage1R_x1, mStage1R_x2, mStage1R_y1, mStage1R_y2;
  float mStage2L_x1, mStage2L_y1;
  float mStage2R_x1, mStage2R_y1;

  float ApplyKWeighting(float in, float& x1, float& x2, float& y1, float& y2, float& x1_2, float& y1_2);
};
