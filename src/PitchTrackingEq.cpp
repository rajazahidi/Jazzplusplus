//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Pitch-Tracking Dynamic Musical EQ Implementation
//*****************************************************************************

#include "PitchTrackingEq.h"
#include <cmath>
#include <algorithm>

JZPitchTrackingEq::JZPitchTrackingEq(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f),
    mTrackingEnabled(true),
    mTrackingSpeedMs(25.0f),
    mDetectedPitchHz(110.0f), // A2 default
    mSmoothedPitchHz(110.0f),
    mPitchBufIdx(0)
{
  mPitchDetectionBuf.assign(1024, 0.0f);

  // 3 Harmonic Surfing Bands: Fundamental (1x), 2nd Harmonic (2x), 3rd Harmonic (3x)
  mBands.reserve(3);
  mBands.emplace_back(1.0f, 3.5f, 2.5f);  // Band 0: Fundamental +3.5dB
  mBands.emplace_back(2.0f, -2.0f, 3.0f); // Band 1: 2nd Harmonic -2.0dB
  mBands.emplace_back(3.0f, 1.5f, 3.5f);  // Band 2: 3rd Harmonic +1.5dB

  for (auto& band : mBands)
  {
    UpdateBandCoefficients(band, mSmoothedPitchHz * band.harmonicMultiplier);
  }
}

void JZPitchTrackingEq::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
    for (auto& band : mBands)
    {
      UpdateBandCoefficients(band, mSmoothedPitchHz * band.harmonicMultiplier);
    }
  }
}

void JZPitchTrackingEq::SetTrackingSpeedMs(float speedMs)
{
  mTrackingSpeedMs = std::clamp(speedMs, 2.0f, 200.0f);
}

void JZPitchTrackingEq::SetBandGainDb(int bandIdx, float gainDb)
{
  if (bandIdx >= 0 && bandIdx < static_cast<int>(mBands.size()))
  {
    mBands[bandIdx].gainDb = std::clamp(gainDb, -24.0f, 24.0f);
    UpdateBandCoefficients(mBands[bandIdx], mSmoothedPitchHz * mBands[bandIdx].harmonicMultiplier);
  }
}

float JZPitchTrackingEq::GetBandGainDb(int bandIdx) const
{
  if (bandIdx >= 0 && bandIdx < static_cast<int>(mBands.size()))
  {
    return mBands[bandIdx].gainDb;
  }
  return 0.0f;
}

void JZPitchTrackingEq::SetBandQ(int bandIdx, float q)
{
  if (bandIdx >= 0 && bandIdx < static_cast<int>(mBands.size()))
  {
    mBands[bandIdx].q = std::clamp(q, 0.5f, 12.0f);
    UpdateBandCoefficients(mBands[bandIdx], mSmoothedPitchHz * mBands[bandIdx].harmonicMultiplier);
  }
}

float JZPitchTrackingEq::GetBandQ(int bandIdx) const
{
  if (bandIdx >= 0 && bandIdx < static_cast<int>(mBands.size()))
  {
    return mBands[bandIdx].q;
  }
  return 2.0f;
}

void JZPitchTrackingEq::UpdateBandCoefficients(JZSurferBand& band, float centerFreq)
{
  float f = std::clamp(centerFreq, 20.0f, mSampleRate * 0.45f);
  band.currentCenterHz = f;

  float A = std::pow(10.0f, band.gainDb / 40.0f);
  float omega = 2.0f * 3.14159265f * f / mSampleRate;
  float alpha = std::sin(omega) / (2.0f * band.q);
  float cosw = std::cos(omega);

  float a0 = 1.0f + alpha / A;
  band.b0 = (1.0f + alpha * A) / a0;
  band.b1 = (-2.0f * cosw) / a0;
  band.b2 = (1.0f - alpha * A) / a0;
  band.a1 = (-2.0f * cosw) / a0;
  band.a2 = (1.0f - alpha / A) / a0;
}

float JZPitchTrackingEq::EstimatePitch(const std::vector<float>& buf)
{
  int minPeriod = static_cast<int>(mSampleRate / 700.0f); // 700Hz
  int maxPeriod = static_cast<int>(mSampleRate / 55.0f);  // 55Hz (A1)
  if (maxPeriod >= static_cast<int>(buf.size() / 2))
  {
    maxPeriod = static_cast<int>(buf.size() / 2) - 1;
  }

  float bestCorr = -1.0f;
  int bestPeriod = minPeriod;

  for (int p = minPeriod; p <= maxPeriod; p += 2)
  {
    float sumProd = 0.0f;
    float sumSq = 0.0f;
    for (size_t i = 0; i < buf.size() - static_cast<size_t>(p); i += 2)
    {
      float s1 = buf[i];
      float s2 = buf[i + static_cast<size_t>(p)];
      sumProd += s1 * s2;
      sumSq += s1 * s1;
    }
    if (sumSq > 0.001f)
    {
      float corr = sumProd / sumSq;
      if (corr > bestCorr)
      {
        bestCorr = corr;
        bestPeriod = p;
      }
    }
  }

  if (bestCorr > 0.4f && bestPeriod > 0)
  {
    return mSampleRate / static_cast<float>(bestPeriod);
  }
  return mSmoothedPitchHz; // Fallback to current
}

void JZPitchTrackingEq::Process(float& left, float& right)
{
  float monoIn = 0.5f * (left + right);
  mPitchDetectionBuf[mPitchBufIdx] = monoIn;
  mPitchBufIdx = (mPitchBufIdx + 1) % mPitchDetectionBuf.size();

  // Periodic pitch evaluation every 256 samples
  if (mPitchBufIdx % 256 == 0 && mTrackingEnabled)
  {
    float detected = EstimatePitch(mPitchDetectionBuf);
    if (detected > 40.0f && detected < 1000.0f)
    {
      mDetectedPitchHz = detected;
    }
  }

  // Smooth glissando pitch following
  float smoothAlpha = std::exp(-1.0f / (mTrackingSpeedMs * 0.001f * mSampleRate));
  mSmoothedPitchHz = smoothAlpha * mSmoothedPitchHz + (1.0f - smoothAlpha) * mDetectedPitchHz;

  // Process audio through surfing biquads
  float outL = left;
  float outR = right;

  for (auto& band : mBands)
  {
    if (mTrackingEnabled)
    {
      UpdateBandCoefficients(band, mSmoothedPitchHz * band.harmonicMultiplier);
    }

    // Direct Form 1 biquad filter
    float filteredL = band.b0 * outL + band.state1L;
    band.state1L = band.b1 * outL - band.a1 * filteredL + band.state2L;
    band.state2L = band.b2 * outL - band.a2 * filteredL;
    outL = filteredL;

    float filteredR = band.b0 * outR + band.state1R;
    band.state1R = band.b1 * outR - band.a1 * filteredR + band.state2R;
    band.state2R = band.b2 * outR - band.a2 * filteredR;
    outR = filteredR;
  }

  left = std::clamp(outL, -1.0f, 1.0f);
  right = std::clamp(outR, -1.0f, 1.0f);
}
