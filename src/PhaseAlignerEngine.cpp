//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Dual-Mic Dynamic Phase Aligner & Auto-Correlator Implementation
//*****************************************************************************

#include "PhaseAlignerEngine.h"
#include <cmath>
#include <algorithm>

JZPhaseAlignerEngine::JZPhaseAlignerEngine(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f),
    mDelayMs(0.0f),
    mPhaseDegrees(0.0f),
    mInvertPolarity(false),
    mWriteIdx(0),
    mAllPassState1(0.0f),
    mAllPassState2(0.0f)
{
  mDelayBuffer.assign(4096, 0.0f);
}

void JZPhaseAlignerEngine::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
  }
}

void JZPhaseAlignerEngine::SetDelayMs(float delayMs)
{
  mDelayMs = std::clamp(delayMs, 0.0f, 25.0f);
}

void JZPhaseAlignerEngine::SetPhaseDegrees(float degrees)
{
  mPhaseDegrees = std::fmod(degrees, 360.0f);
  if (mPhaseDegrees < 0.0f)
  {
    mPhaseDegrees += 360.0f;
  }
}

float JZPhaseAlignerEngine::ComputeCorrelation(
  const std::vector<float>& a,
  const std::vector<float>& b,
  int lagSamples)
{
  int n = static_cast<int>(std::min(a.size(), b.size()));
  if (n < 64)
  {
    return 0.0f;
  }

  int start = std::max(0, -lagSamples);
  int end = std::min(n, n - lagSamples);
  if (end <= start)
  {
    return 0.0f;
  }

  float sumProd = 0.0f;
  float sumSqA = 0.0f;
  float sumSqB = 0.0f;

  for (int i = start; i < end; ++i)
  {
    float valA = a[i];
    float valB = b[i + lagSamples];

    sumProd += valA * valB;
    sumSqA += valA * valA;
    sumSqB += valB * valB;
  }

  float denom = std::sqrt(sumSqA * sumSqB);
  return (denom > 0.00001f) ? (sumProd / denom) : 0.0f;
}

bool JZPhaseAlignerEngine::AutoAlign(
  const std::vector<float>& guideAudio,
  const std::vector<float>& targetAudio,
  JZPhaseAlignReport& outReport)
{
  outReport = JZPhaseAlignReport();
  if (guideAudio.empty() || targetAudio.empty())
  {
    return false;
  }

  outReport.initialCorrelation = ComputeCorrelation(guideAudio, targetAudio, 0);

  // Scan lag range up to 20ms
  int maxLag = static_cast<int>(mSampleRate * 0.020f);
  int bestLag = 0;
  float bestCorr = -2.0f;
  bool shouldInvert = false;

  for (int lag = -maxLag; lag <= maxLag; lag += 2)
  {
    float corr = ComputeCorrelation(guideAudio, targetAudio, lag);
    if (corr > bestCorr)
    {
      bestCorr = corr;
      bestLag = lag;
      shouldInvert = false;
    }
    // Check if inverted phase produces better match (e.g. Snare bottom 180 deg out)
    if (-corr > bestCorr)
    {
      bestCorr = -corr;
      bestLag = lag;
      shouldInvert = true;
    }
  }

  float delaySec = static_cast<float>(bestLag) / mSampleRate;
  mDelayMs = std::max(0.0f, delaySec * 1000.0f);
  mInvertPolarity = shouldInvert;
  mPhaseDegrees = (shouldInvert) ? 180.0f : 0.0f;

  outReport.detectedDelayMs = (static_cast<float>(bestLag) / mSampleRate) * 1000.0f;
  outReport.appliedDelayMs = mDelayMs;
  outReport.appliedPhaseDegrees = mPhaseDegrees;
  outReport.polarityInverted = mInvertPolarity;
  outReport.alignedCorrelation = bestCorr;
  outReport.success = true;

  return true;
}

float JZPhaseAlignerEngine::ProcessSample(float targetSample)
{
  mDelayBuffer[mWriteIdx] = targetSample;

  float delaySamples = mDelayMs * 0.001f * mSampleRate;
  if (delaySamples > static_cast<float>(mDelayBuffer.size() - 2))
  {
    delaySamples = static_cast<float>(mDelayBuffer.size() - 2);
  }

  float readPos = static_cast<float>(mWriteIdx) - delaySamples;
  if (readPos < 0.0f)
  {
    readPos += static_cast<float>(mDelayBuffer.size());
  }

  size_t idx0 = static_cast<size_t>(readPos) % mDelayBuffer.size();
  size_t idx1 = (idx0 + 1) % mDelayBuffer.size();
  float frac = readPos - static_cast<float>(static_cast<size_t>(readPos));

  // Linear interpolation
  float delayed = (1.0f - frac) * mDelayBuffer[idx0] + frac * mDelayBuffer[idx1];
  mWriteIdx = (mWriteIdx + 1) % mDelayBuffer.size();

  // All-pass phase rotator approximation (0 to 360 deg)
  float phaseRad = mPhaseDegrees * (3.14159265f / 180.0f);
  float alpha = std::tan(phaseRad * 0.25f);
  float apOut = -alpha * delayed + mAllPassState1;
  mAllPassState1 = delayed + alpha * apOut;

  // Polarity flip
  if (mInvertPolarity)
  {
    apOut = -apOut;
  }

  return apOut;
}

void JZPhaseAlignerEngine::ProcessBuffer(
  const std::vector<float>& inTarget,
  std::vector<float>& outTarget)
{
  outTarget.resize(inTarget.size());
  for (size_t i = 0; i < inTarget.size(); ++i)
  {
    outTarget[i] = ProcessSample(inTarget[i]);
  }
}
