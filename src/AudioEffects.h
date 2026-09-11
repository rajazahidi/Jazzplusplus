//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Copyright (C) 2026 Raja Zahidi
// Audio DSP Engine — 3-Band EQ, Reverb, Delay, Chorus, Limiter/Overdrive
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//*****************************************************************************

#pragma once

#include <vector>

// ============================================================================
// 1. RBJ Biquad Filter (Equalizer building block)
// ============================================================================
enum JZBiquadType
{
  eBiquadLowShelf = 0,
  eBiquadHighShelf,
  eBiquadPeakingEQ,
  eBiquadLowPass,
  eBiquadHighPass,
  eBiquadBandPass
};

class JZBiquadFilter
{
  public:
    JZBiquadFilter();

    void Configure(
      JZBiquadType type,
      float sampleRate,
      float frequencyHz,
      float Q,
      float gainDB = 0.0f);

    float Process(float inSample);
    void Reset();

  private:
    float b0, b1, b2, a1, a2;
    float x1, x2, y1, y2;
};

// ============================================================================
// 2. 3-Band Parametric Equalizer
// ============================================================================
struct JZEqualizerSettings
{
  float bassGainDB;    // -18.0 to +18.0 dB (e.g. at 100 Hz)
  float midGainDB;     // -18.0 to +18.0 dB (e.g. at 1000 Hz)
  float trebleGainDB;  // -18.0 to +18.0 dB (e.g. at 8000 Hz)
  float bassFreqHz;    // default 120 Hz
  float midFreqHz;     // default 1200 Hz
  float trebleFreqHz;  // default 7000 Hz

  JZEqualizerSettings()
    : bassGainDB(0.0f),
      midGainDB(0.0f),
      trebleGainDB(0.0f),
      bassFreqHz(120.0f),
      midFreqHz(1200.0f),
      trebleFreqHz(7000.0f)
  {
  }
};

class JZ3BandEqualizer
{
  public:
    JZ3BandEqualizer(float sampleRate = 44100.0f);

    void SetSettings(const JZEqualizerSettings& settings);

    void Process(float* left, float* right, int numSamples);
    void Process(short* buffer, int numSamples, int channels);

  private:
    float mSampleRate;
    JZEqualizerSettings mSettings;
    JZBiquadFilter mFilterLowL, mFilterLowR;
    JZBiquadFilter mFilterMidL, mFilterMidR;
    JZBiquadFilter mFilterHighL, mFilterHighR;
};

// ============================================================================
// 3. Algorithmic Freeverb Reverb
// ============================================================================
struct JZReverbSettings
{
  float roomSize; // 0.0 to 1.0 (internal scale 0.7 to 0.98)
  float damping;  // 0.0 to 1.0
  float width;    // 0.0 to 1.0 (stereo spread)
  float wet;      // 0.0 to 1.0
  float dry;      // 0.0 to 1.0

  JZReverbSettings()
    : roomSize(0.5f),
      damping(0.3f),
      width(1.0f),
      wet(0.35f),
      dry(0.75f)
  {
  }
};

class JZCombFilter
{
  public:
    JZCombFilter();
    void SetBuffer(int size);
    void SetDamp(float val);
    void SetFeedback(float val);
    float Process(float input);
    void Reset();

  private:
    std::vector<float> mBuffer;
    int mBufSize;
    int mBufIdx;
    float mFeedback;
    float mFilterStore;
    float mDamp1;
    float mDamp2;
};

class JZAllpassFilter
{
  public:
    JZAllpassFilter();
    void SetBuffer(int size);
    void SetFeedback(float val);
    float Process(float input);
    void Reset();

  private:
    std::vector<float> mBuffer;
    int mBufSize;
    int mBufIdx;
    float mFeedback;
};

class JZFreeverb
{
  public:
    JZFreeverb(float sampleRate = 44100.0f);

    void SetSettings(const JZReverbSettings& settings);
    void Process(float* inL, float* inR, float* outL, float* outR, int numSamples);
    void Process(short* buffer, int numSamples, int channels);
    void Reset();

  private:
    float mSampleRate;
    JZReverbSettings mSettings;

    enum { NUM_COMBS = 8, NUM_ALLPASSES = 4 };
    JZCombFilter mCombL[NUM_COMBS];
    JZCombFilter mCombR[NUM_COMBS];
    JZAllpassFilter mAllpassL[NUM_ALLPASSES];
    JZAllpassFilter mAllpassR[NUM_ALLPASSES];

    void UpdateParameters();
};

// ============================================================================
// 4. Stereo Delay & Echo
// ============================================================================
struct JZDelaySettings
{
  float delayTimeMsL; // 1 to 2000 ms
  float delayTimeMsR; // 1 to 2000 ms
  float feedback;     // 0.0 to 0.95
  float damp;         // 0.0 to 1.0 (high frequency damping)
  float wet;          // 0.0 to 1.0
  float dry;          // 0.0 to 1.0

  JZDelaySettings()
    : delayTimeMsL(350.0f),
      delayTimeMsR(450.0f),
      feedback(0.5f),
      damp(0.2f),
      wet(0.4f),
      dry(0.8f)
  {
  }
};

class JZStereoDelay
{
  public:
    JZStereoDelay(float sampleRate = 44100.0f);

    void SetSettings(const JZDelaySettings& settings);
    void Process(float* inL, float* inR, float* outL, float* outR, int numSamples);
    void Process(short* buffer, int numSamples, int channels);
    void Reset();

  private:
    float mSampleRate;
    JZDelaySettings mSettings;
    std::vector<float> mBufferL;
    std::vector<float> mBufferR;
    int mWriteIdxL;
    int mWriteIdxR;
    float mFilterStoreL;
    float mFilterStoreR;
};

// ============================================================================
// 5. Modulation (Chorus / Flanger)
// ============================================================================
struct JZChorusSettings
{
  float rateHz;   // 0.1 to 5.0 Hz
  float depthMs;  // 1.0 to 15.0 ms
  float feedback; // 0.0 to 0.8
  float mix;      // 0.0 to 1.0

  JZChorusSettings()
    : rateHz(1.2f),
      depthMs(4.0f),
      feedback(0.2f),
      mix(0.5f)
  {
  }
};

class JZStereoChorus
{
  public:
    JZStereoChorus(float sampleRate = 44100.0f);

    void SetSettings(const JZChorusSettings& settings);
    void Process(float* left, float* right, int numSamples);
    void Process(short* buffer, int numSamples, int channels);
    void Reset();

  private:
    float mSampleRate;
    JZChorusSettings mSettings;
    std::vector<float> mBufferL;
    std::vector<float> mBufferR;
    int mWriteIdx;
    float mLfoPhase;
};

// ============================================================================
// 6. Overdrive / Soft-Clipper & Master Limiter
// ============================================================================
struct JZLimiterSettings
{
  float drive;         // 1.0 (clean) to 10.0 (distorted)
  float ceilingDB;     // -12.0 to 0.0 dB
  bool enableOverdrive;// true = warm saturation / distortion

  JZLimiterSettings()
    : drive(1.0f),
      ceilingDB(-0.3f),
      enableOverdrive(false)
  {
  }
};

class JZAudioLimiterDistortion
{
  public:
    JZAudioLimiterDistortion();

    void SetSettings(const JZLimiterSettings& settings);
    void Process(float* left, float* right, int numSamples);
    void Process(short* buffer, int numSamples, int channels);

  private:
    JZLimiterSettings mSettings;
};
