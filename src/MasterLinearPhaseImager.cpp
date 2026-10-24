//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Master Linear Phase FIR Crossover & Psychoacoustic Stereo Imager Implementation
//*****************************************************************************

#include "MasterLinearPhaseImager.h"
#include <cmath>
#include <algorithm>

JZMasterLinearPhaseImager::JZMasterLinearPhaseImager(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f),
    mEllipticalSubMono(true),
    mWriteIdx(0)
{
  mBands.reserve(4);
  mBands.emplace_back(120.0f, 0.0f);     // Band 0: Sub (Default Elliptical Mono)
  mBands.emplace_back(1200.0f, 100.0f);  // Band 1: Low-Mid (100% natural)
  mBands.emplace_back(6000.0f, 130.0f);  // Band 2: High-Mid (130% widened presence)
  mBands.emplace_back(15000.0f, 150.0f); // Band 3: Air (150% super-wide air sparkle)

  mDelayL.assign(kFirTaps, 0.0f);
  mDelayR.assign(kFirTaps, 0.0f);
}

void JZMasterLinearPhaseImager::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
  }
}

void JZMasterLinearPhaseImager::SetBandWidthPercent(int bandIdx, float widthPercent)
{
  if (bandIdx >= 0 && bandIdx < static_cast<int>(mBands.size()))
  {
    mBands[bandIdx].widthPercent = std::clamp(widthPercent, 0.0f, 200.0f);
  }
}

float JZMasterLinearPhaseImager::GetBandWidthPercent(int bandIdx) const
{
  if (bandIdx >= 0 && bandIdx < static_cast<int>(mBands.size()))
  {
    return mBands[bandIdx].widthPercent;
  }
  return 100.0f;
}

void JZMasterLinearPhaseImager::Filter4Bands(
  float inL, float inR,
  float outSub[2], float outLowMid[2], float outHighMid[2], float outAir[2])
{
  mDelayL[mWriteIdx] = inL;
  mDelayR[mWriteIdx] = inR;

  // Symmetric 31-tap Linear Phase FIR Crossover kernel
  float sumSubL = 0.0f, sumSubR = 0.0f;
  float sumLowMidL = 0.0f, sumLowMidR = 0.0f;
  float sumHighMidL = 0.0f, sumHighMidR = 0.0f;

  for (size_t t = 0; t < kFirTaps; ++t)
  {
    size_t idx = (mWriteIdx + kFirTaps - t) % kFirTaps;
    float sL = mDelayL[idx];
    float sR = mDelayR[idx];

    // Sinc filter coefficients with Blackman-Harris windowing
    float dist = static_cast<float>(t) - 15.0f;
    float win = 0.35875f - 0.48829f * std::cos(2.0f * 3.14159265f * t / 30.0f) + 0.14128f * std::cos(4.0f * 3.14159265f * t / 30.0f);

    float fSub = 120.0f / mSampleRate;
    float fLowMid = 1200.0f / mSampleRate;
    float fHighMid = 6000.0f / mSampleRate;

    float sincSub = (dist == 0.0f) ? (2.0f * fSub) : (std::sin(2.0f * 3.14159265f * fSub * dist) / (3.14159265f * dist));
    float sincLowMid = (dist == 0.0f) ? (2.0f * fLowMid) : (std::sin(2.0f * 3.14159265f * fLowMid * dist) / (3.14159265f * dist));
    float sincHighMid = (dist == 0.0f) ? (2.0f * fHighMid) : (std::sin(2.0f * 3.14159265f * fHighMid * dist) / (3.14159265f * dist));

    sumSubL += sL * (sincSub * win);
    sumSubR += sR * (sincSub * win);
    sumLowMidL += sL * (sincLowMid * win);
    sumLowMidR += sR * (sincLowMid * win);
    sumHighMidL += sL * (sincHighMid * win);
    sumHighMidR += sR * (sincHighMid * win);
  }

  // Exact center sample (zero-phase linear delay tap 15)
  size_t centerIdx = (mWriteIdx + kFirTaps - 15) % kFirTaps;
  float centerL = mDelayL[centerIdx];
  float centerR = mDelayR[centerIdx];

  mWriteIdx = (mWriteIdx + 1) % kFirTaps;

  // Perfect reconstruction band separation
  outSub[0] = sumSubL;
  outSub[1] = sumSubR;

  outLowMid[0] = sumLowMidL - sumSubL;
  outLowMid[1] = sumLowMidR - sumSubR;

  outHighMid[0] = sumHighMidL - sumLowMidL;
  outHighMid[1] = sumHighMidR - sumLowMidR;

  outAir[0] = centerL - sumHighMidL;
  outAir[1] = centerR - sumHighMidR;
}

void JZMasterLinearPhaseImager::Process(float& left, float& right)
{
  float sub[2], lowMid[2], highMid[2], air[2];
  Filter4Bands(left, right, sub, lowMid, highMid, air);

  auto processBandMS = [&](float bandL, float bandR, float widthPct, bool forceMono) -> std::pair<float, float> {
    const float invSqrt2 = 0.70710678f;
    float mid = (bandL + bandR) * invSqrt2;
    float side = (bandL - bandR) * invSqrt2;

    if (forceMono)
    {
      side = 0.0f; // Elliptical mono constraint
    }
    else
    {
      side *= (widthPct / 100.0f);
    }

    float recL = (mid + side) * invSqrt2;
    float recR = (mid - side) * invSqrt2;
    return {recL, recR};
  };

  auto pSub = processBandMS(sub[0], sub[1], mBands[0].widthPercent, mEllipticalSubMono);
  auto pLowMid = processBandMS(lowMid[0], lowMid[1], mBands[1].widthPercent, false);
  auto pHighMid = processBandMS(highMid[0], highMid[1], mBands[2].widthPercent, false);
  auto pAir = processBandMS(air[0], air[1], mBands[3].widthPercent, false);

  left = std::clamp(pSub.first + pLowMid.first + pHighMid.first + pAir.first, -1.0f, 1.0f);
  right = std::clamp(pSub.second + pLowMid.second + pHighMid.second + pAir.second, -1.0f, 1.0f);
}
