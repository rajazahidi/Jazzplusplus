//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Upward / Downward OTT Multiband Dynamics Implementation
//*****************************************************************************

#include "MultibandOttProcessor.h"
#include <cmath>
#include <algorithm>

JZMultibandOttProcessor::JZMultibandOttProcessor(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f),
    mDepth(100.0f),
    mTimeScale(1.0f),
    mLowBand(140.0f, -16.0f, -34.0f),
    mMidBand(2500.0f, -14.0f, -32.0f),
    mHighBand(12000.0f, -15.0f, -33.0f)
{
}

void JZMultibandOttProcessor::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
  }
}

void JZMultibandOttProcessor::SetDepth(float depthPercent)
{
  mDepth = std::clamp(depthPercent, 0.0f, 100.0f);
}

void JZMultibandOttProcessor::SetTimeScale(float scale)
{
  mTimeScale = std::clamp(scale, 0.1f, 5.0f);
}

void JZMultibandOttProcessor::SetBandGainDb(int bandIdx, float gainDb)
{
  float g = std::clamp(gainDb, -24.0f, 24.0f);
  if (bandIdx == 0) mLowBand.makeUpGainDb = g;
  else if (bandIdx == 1) mMidBand.makeUpGainDb = g;
  else if (bandIdx == 2) mHighBand.makeUpGainDb = g;
}

float JZMultibandOttProcessor::GetBandGainDb(int bandIdx) const
{
  if (bandIdx == 0) return mLowBand.makeUpGainDb;
  if (bandIdx == 1) return mMidBand.makeUpGainDb;
  if (bandIdx == 2) return mHighBand.makeUpGainDb;
  return 0.0f;
}

float JZMultibandOttProcessor::ComputeOttGain(float envLinear, const JZOttBandState& band)
{
  float envDb = 20.0f * std::log10(std::max(0.00001f, envLinear));
  float downCutDb = 0.0f;
  float upBoostDb = 0.0f;

  // 1. Downward Compression (Taming loud parts)
  if (envDb > band.downThreshDb)
  {
    float excess = envDb - band.downThreshDb;
    downCutDb = -excess * (1.0f - 1.0f / band.downRatio);
  }

  // 2. Upward Compression (Boosting quiet details)
  if (envDb < band.upThreshDb && envDb > -65.0f)
  {
    float deficit = band.upThreshDb - envDb;
    upBoostDb = deficit * (1.0f - 1.0f / band.upRatio);
    upBoostDb = std::min(upBoostDb, 18.0f); // Guard max upward expansion
  }

  float netGainDb = (downCutDb + upBoostDb + band.makeUpGainDb) * (mDepth / 100.0f);
  return std::pow(10.0f, netGainDb / 20.0f);
}

void JZMultibandOttProcessor::Process(float& left, float& right)
{
  if (mDepth <= 0.001f)
  {
    return;
  }

  float origL = left;
  float origR = right;

  // 1. Crossover Filtering
  // Low-pass at 140Hz
  float lowAlpha = std::exp(-2.0f * 3.14159265f * 140.0f / mSampleRate);
  mLowBand.lpStateL = lowAlpha * mLowBand.lpStateL + (1.0f - lowAlpha) * origL;
  mLowBand.lpStateR = lowAlpha * mLowBand.lpStateR + (1.0f - lowAlpha) * origR;
  float lowL = mLowBand.lpStateL;
  float lowR = mLowBand.lpStateR;

  // High-pass at 2500Hz
  float highAlpha = std::exp(-2.0f * 3.14159265f * 2500.0f / mSampleRate);
  mHighBand.hpStateL = highAlpha * mHighBand.hpStateL + (1.0f - highAlpha) * origL;
  mHighBand.hpStateR = highAlpha * mHighBand.hpStateR + (1.0f - highAlpha) * origR;
  float highL = origL - mHighBand.hpStateL;
  float highR = origR - mHighBand.hpStateR;

  // Mid Band
  float midL = origL - (lowL + highL);
  float midR = origR - (lowR + highR);

  // 2. Attack / Release envelope time constants
  float attackCoeff = std::exp(-1.0f / (0.005f * mTimeScale * mSampleRate)); // 5ms
  float releaseCoeff = std::exp(-1.0f / (0.050f * mTimeScale * mSampleRate)); // 50ms

  auto updateEnv = [&](float sample, float& env) {
    float absVal = std::abs(sample);
    if (absVal > env) env = attackCoeff * env + (1.0f - attackCoeff) * absVal;
    else env = releaseCoeff * env + (1.0f - releaseCoeff) * absVal;
  };

  updateEnv(lowL, mLowBand.envFollowerL);
  updateEnv(lowR, mLowBand.envFollowerR);
  updateEnv(midL, mMidBand.envFollowerL);
  updateEnv(midR, mMidBand.envFollowerR);
  updateEnv(highL, mHighBand.envFollowerL);
  updateEnv(highR, mHighBand.envFollowerR);

  // 3. Compute OTT dual gains
  float gainLowL = ComputeOttGain(mLowBand.envFollowerL, mLowBand);
  float gainLowR = ComputeOttGain(mLowBand.envFollowerR, mLowBand);
  float gainMidL = ComputeOttGain(mMidBand.envFollowerL, mMidBand);
  float gainMidR = ComputeOttGain(mMidBand.envFollowerR, mMidBand);
  float gainHighL = ComputeOttGain(mHighBand.envFollowerL, mHighBand);
  float gainHighR = ComputeOttGain(mHighBand.envFollowerR, mHighBand);

  // 4. Recombine compressed bands
  float outL = lowL * gainLowL + midL * gainMidL + highL * gainHighL;
  float outR = lowR * gainLowR + midR * gainMidR + highR * gainHighR;

  left = std::clamp(outL, -1.0f, 1.0f);
  right = std::clamp(outR, -1.0f, 1.0f);
}
