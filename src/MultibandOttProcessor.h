//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Upward / Downward OTT Multiband Dynamics Processor
//
// Legendary OTT-style 3-band dynamics processor featuring simultaneous
// Upward Compression (lifting quiet micro-dynamics) and Downward Compression
// (taming loud transients) across Low, Mid, and High frequency bands.
//*****************************************************************************

#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

struct JZOttBandState
{
  float crossoverFreq;
  float downThreshDb;         // Downward compression threshold (e.g. -18 dBFS)
  float downRatio;            // e.g. 6.0
  float upThreshDb;           // Upward compression threshold (e.g. -36 dBFS)
  float upRatio;              // e.g. 4.0
  float makeUpGainDb;

  float envFollowerL;
  float envFollowerR;
  float currentGainL;
  float currentGainR;

  float lpStateL, lpStateR;
  float hpStateL, hpStateR;

  JZOttBandState(float cross = 250.0f, float downTh = -18.0f, float upTh = -36.0f)
    : crossoverFreq(cross),
      downThreshDb(downTh),
      downRatio(6.0f),
      upThreshDb(upTh),
      upRatio(3.5f),
      makeUpGainDb(0.0f),
      envFollowerL(0.0f),
      envFollowerR(0.0f),
      currentGainL(1.0f),
      currentGainR(1.0f),
      lpStateL(0.0f), lpStateR(0.0f),
      hpStateL(0.0f), hpStateR(0.0f)
  {
  }
};

class JZMultibandOttProcessor
{
public:
  JZMultibandOttProcessor(float sampleRate = 44100.0f);
  ~JZMultibandOttProcessor() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  // OTT Controls
  void SetDepth(float depthPercent);          // 0% (dry bypass) to 100% (full OTT hyper-dynamics)
  float GetDepth() const { return mDepth; }

  void SetTimeScale(float scale);             // 0.2 (hyper-fast) to 4.0 (gentle)
  float GetTimeScale() const { return mTimeScale; }

  void SetBandGainDb(int bandIdx, float gainDb); // 0=Low, 1=Mid, 2=High
  float GetBandGainDb(int bandIdx) const;

  // Real-Time Stereo Processing
  void Process(float& left, float& right);

private:
  float mSampleRate;
  float mDepth;
  float mTimeScale;

  JZOttBandState mLowBand;
  JZOttBandState mMidBand;
  JZOttBandState mHighBand;

  float ComputeOttGain(float envLinear, const JZOttBandState& band);
};
