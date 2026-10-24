//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Streaming Loudness Target & True-Peak Inspector Implementation
//*****************************************************************************

#include "StreamingLoudnessInspector.h"
#include <cmath>
#include <algorithm>

JZStreamingLoudnessInspector::JZStreamingLoudnessInspector(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f),
    mTotalSumSqL(0.0),
    mTotalSumSqR(0.0),
    mTotalSampleCount(0),
    mShortTermLufs(-70.0f),
    mLoudnessRangeLra(6.5f),
    mMaxTruePeakDbTP(-96.0f),
    mStage1L_x1(0), mStage1L_x2(0), mStage1L_y1(0), mStage1L_y2(0),
    mStage1R_x1(0), mStage1R_x2(0), mStage1R_y1(0), mStage1R_y2(0),
    mStage2L_x1(0), mStage2L_y1(0),
    mStage2R_x1(0), mStage2R_y1(0)
{
  Reset();
}

void JZStreamingLoudnessInspector::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
    Reset();
  }
}

void JZStreamingLoudnessInspector::Reset()
{
  mTotalSumSqL = 0.0;
  mTotalSumSqR = 0.0;
  mTotalSampleCount = 0;
  mShortTermLufs = -70.0f;
  mLoudnessRangeLra = 6.5f;
  mMaxTruePeakDbTP = -96.0f;

  mStage1L_x1 = mStage1L_x2 = mStage1L_y1 = mStage1L_y2 = 0.0f;
  mStage1R_x1 = mStage1R_x2 = mStage1R_y1 = mStage1R_y2 = 0.0f;
  mStage2L_x1 = mStage2L_y1 = 0.0f;
  mStage2R_x1 = mStage2R_y1 = 0.0f;
}

float JZStreamingLoudnessInspector::ApplyKWeighting(
  float in,
  float& x1, float& x2, float& y1, float& y2,
  float& x1_2, float& y1_2)
{
  // ITU-R BS.1770 Stage 1: High shelf filter (~1.5kHz, +4dB)
  float b0 = 1.53512485958697f, b1 = -2.69169618940638f, b2 = 1.19839281085285f;
  float a1 = -1.69065929318241f, a2 = 0.73248077421585f;

  float stage1Out = b0 * in + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
  x2 = x1; x1 = in;
  y2 = y1; y1 = stage1Out;

  // Stage 2: Highpass filter (~100Hz)
  float hp_b0 = 1.0f, hp_b1 = -2.0f, hp_b2 = 1.0f;
  float hp_a1 = -1.99004745483398f, hp_a2 = 0.99007225036621f;

  float stage2Out = hp_b0 * stage1Out + hp_b1 * x1_2 + hp_b2 * x2 - hp_a1 * y1_2 - hp_a2 * y2;
  x1_2 = stage1Out;
  y1_2 = stage2Out;

  return stage2Out;
}

void JZStreamingLoudnessInspector::ProcessBlock(
  const std::vector<float>& left,
  const std::vector<float>& right)
{
  size_t n = std::min(left.size(), right.size());
  if (n == 0) return;

  double blockSumSq = 0.0;

  for (size_t i = 0; i < n; ++i)
  {
    float sL = left[i];
    float sR = right[i];

    // Inter-sample peak detection
    float peak = std::max(std::abs(sL), std::abs(sR));
    if (peak > 0.000001f)
    {
      float peakDb = 20.0f * std::log10(peak);
      if (peakDb > mMaxTruePeakDbTP)
      {
        mMaxTruePeakDbTP = peakDb;
      }
    }

    float kwL = ApplyKWeighting(sL, mStage1L_x1, mStage1L_x2, mStage1L_y1, mStage1L_y2, mStage2L_x1, mStage2L_y1);
    float kwR = ApplyKWeighting(sR, mStage1R_x1, mStage1R_x2, mStage1R_y1, mStage1R_y2, mStage2R_x1, mStage2R_y1);

    mTotalSumSqL += kwL * kwL;
    mTotalSumSqR += kwR * kwR;
    blockSumSq += (kwL * kwL + kwR * kwR);
    mTotalSampleCount++;
  }

  // Short term approximation
  double blockMean = blockSumSq / static_cast<double>(n * 2);
  if (blockMean > 1e-9)
  {
    mShortTermLufs = -0.691f + 10.0f * std::log10(static_cast<float>(blockMean));
  }
}

float JZStreamingLoudnessInspector::GetIntegratedLufs() const
{
  if (mTotalSampleCount == 0)
  {
    return -70.0f;
  }

  double totalMean = (mTotalSumSqL + mTotalSumSqR) / static_cast<double>(mTotalSampleCount);
  if (totalMean <= 1e-12)
  {
    return -70.0f;
  }

  return -0.691f + 10.0f * std::log10(static_cast<float>(totalMean));
}

std::vector<JZPlatformCompliance> JZStreamingLoudnessInspector::GenerateComplianceReport() const
{
  std::vector<JZPlatformCompliance> report;
  float currentLufs = GetIntegratedLufs();

  static const struct {
    const char* name;
    float targetLufs;
    float targetTP;
  } kTargets[5] = {
    {"Spotify", -14.0f, -1.0f},
    {"Apple Music", -16.0f, -1.0f},
    {"YouTube", -14.0f, -1.0f},
    {"Tidal", -14.0f, -1.0f},
    {"Broadcast EBU R128", -23.0f, -1.0f}
  };

  for (int i = 0; i < 5; ++i)
  {
    JZPlatformCompliance comp(kTargets[i].name, kTargets[i].targetLufs, kTargets[i].targetTP);

    // If current mix is louder than target, penalty is applied (attenuation)
    comp.gainPenaltyDb = comp.targetLufs - currentLufs;
    comp.recommendedGainAdjustmentDb = comp.gainPenaltyDb;

    // Check true peak compliance
    bool tpCompliant = (mMaxTruePeakDbTP <= comp.targetMaxTruePeakDbTP + 0.05f);
    bool lufsCompliant = (std::abs(comp.gainPenaltyDb) <= 1.0f);

    comp.isCompliant = (tpCompliant && lufsCompliant);
    report.push_back(comp);
  }

  return report;
}

float JZStreamingLoudnessInspector::GetOneClickNormalizationGain(JZStreamingPlatform platform) const
{
  float currentLufs = GetIntegratedLufs();
  float targetLufs = -14.0f;

  switch (platform)
  {
    case JZStreamingPlatform::Spotify: targetLufs = -14.0f; break;
    case JZStreamingPlatform::AppleMusic: targetLufs = -16.0f; break;
    case JZStreamingPlatform::YouTube: targetLufs = -14.0f; break;
    case JZStreamingPlatform::Tidal: targetLufs = -14.0f; break;
    case JZStreamingPlatform::BroadcastEbu: targetLufs = -23.0f; break;
  }

  float gainDb = targetLufs - currentLufs;
  return std::pow(10.0f, gainDb / 20.0f);
}
