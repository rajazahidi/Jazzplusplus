//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// ITU-R BS.1770-4 / EBU R128 Loudness Meter & True Peak Engine
//
// Professional broadcast and commercial streaming loudness meter:
// K-weighting pre-filtering (acoustic head diffraction + RLB high-pass),
// Momentary (400ms), Short-Term (3s), and Integrated (gated) LUFS metering,
// Loudness Range (LRA), and 4x oversampled True Peak (dBTP) detection.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

// Target streaming platform presets
enum JZLoudnessTargetPreset
{
  LOUDNESS_SPOTIFY = 0,       // Target: -14.0 LUFS, Max TP: -1.0 dBTP
  LOUDNESS_APPLE_MUSIC,       // Target: -16.0 LUFS, Max TP: -1.0 dBTP
  LOUDNESS_YOUTUBE,           // Target: -14.0 LUFS, Max TP: -1.0 dBTP
  LOUDNESS_EBU_R128,          // Target: -23.0 LUFS, Max TP: -1.0 dBTP
  LOUDNESS_ATSC_A85           // Target: -24.0 LUFS, Max TP: -2.0 dBTP
};

// Loudness Analysis Results
struct JZLoudnessMetrics
{
  float momentaryLUFS;        // 400ms window
  float shortTermLUFS;        // 3.0s window
  float integratedLUFS;       // Program loudness (gated)
  float loudnessRangeLU;      // LRA in Loudness Units
  float truePeakDBTP;         // True Peak (4x oversampled inter-sample peak)
  float maxShortTermLUFS;
  float maxMomentaryLUFS;

  bool isTargetCompliant;
  float targetOffsetLU;       // Difference from target (e.g. +1.5 LU means too loud)

  JZLoudnessMetrics()
    : momentaryLUFS(-70.0f),
      shortTermLUFS(-70.0f),
      integratedLUFS(-70.0f),
      loudnessRangeLU(0.0f),
      truePeakDBTP(-96.0f),
      maxShortTermLUFS(-70.0f),
      maxMomentaryLUFS(-70.0f),
      isTargetCompliant(true),
      targetOffsetLU(0.0f)
  {
  }
};

// EBU R128 / ITU-R BS.1770-4 Loudness Meter Class
class JZLoudnessMeterEBU
{
public:
  JZLoudnessMeterEBU(float sampleRate = 44100.0f);
  ~JZLoudnessMeterEBU() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  void SetTargetPreset(JZLoudnessTargetPreset preset);
  JZLoudnessTargetPreset GetTargetPreset() const { return mTargetPreset; }
  float GetTargetLUFS() const;

  void Reset();

  // Process Stereo Audio Block
  void ProcessBlock(const float* left, const float* right, int numSamples);

  // Retrieve current loudness metrics
  const JZLoudnessMetrics& GetMetrics() const { return mMetrics; }

  // Target preset label
  static std::string GetPresetName(JZLoudnessTargetPreset preset);

private:
  float mSampleRate;
  JZLoudnessTargetPreset mTargetPreset;
  JZLoudnessMetrics mMetrics;

  // K-Weighting Stage 1: High-Shelf Filter
  float hs_b0, hs_b1, hs_b2, hs_a1, hs_a2;
  float mHsX1L, mHsX2L, mHsY1L, mHsY2L;
  float mHsX1R, mHsX2R, mHsY1R, mHsY2R;

  // K-Weighting Stage 2: RLB High-Pass Filter
  float hp_b0, hp_b1, hp_b2, hp_a1, hp_a2;
  float mHpX1L, mHpX2L, mHpY1L, mHpY2L;
  float mHpX1R, mHpX2R, mHpY1R, mHpY2R;

  // Sliding buffers for Momentary (400ms) and Short-Term (3s)
  std::vector<float> mMomentaryBuffer;
  std::vector<float> mShortTermBuffer;
  int mMomentaryPos;
  int mShortTermPos;
  int mMomentarySamples;
  int mShortTermSamples;

  // Gated integrated loudness accumulation
  std::vector<float> mBlockEnergies;

  void UpdateFilterCoefficients();
  float FilterSample(float in, float b0, float b1, float b2, float a1, float a2,
                     float& x1, float& x2, float& y1, float& y2);
  void ComputeIntegratedLoudness();
  float DetectTruePeak4x(float s0, float s1);
};
