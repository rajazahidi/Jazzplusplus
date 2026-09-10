//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Copyright (C) 2026 Jazz++ Developers
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//*****************************************************************************

#include "AudioEffects.h"

#include <algorithm>
#include <cmath>
#include <cstring>

using namespace std;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// 1. JZBiquadFilter Implementation
// ============================================================================
JZBiquadFilter::JZBiquadFilter()
  : b0(1.0f), b1(0.0f), b2(0.0f),
    a1(0.0f), a2(0.0f),
    x1(0.0f), x2(0.0f),
    y1(0.0f), y2(0.0f)
{
}

void JZBiquadFilter::Reset()
{
  x1 = x2 = y1 = y2 = 0.0f;
}

void JZBiquadFilter::Configure(
  JZBiquadType type,
  float sampleRate,
  float frequencyHz,
  float Q,
  float gainDB)
{
  if (sampleRate <= 0.0f)
  {
    sampleRate = 44100.0f;
  }
  if (frequencyHz < 10.0f)
  {
    frequencyHz = 10.0f;
  }
  if (frequencyHz > sampleRate * 0.49f)
  {
    frequencyHz = sampleRate * 0.49f;
  }
  if (Q <= 0.01f)
  {
    Q = 0.7071f;
  }

  float omega = static_cast<float>(2.0 * M_PI * frequencyHz / sampleRate);
  float sinOmega = sin(omega);
  float cosOmega = cos(omega);
  float alpha = sinOmega / (2.0f * Q);
  float A = pow(10.0f, gainDB / 40.0f);

  float a0 = 1.0f;

  switch (type)
  {
    case eBiquadLowShelf:
      {
        float twoSqrtAAlpha = 2.0f * sqrt(A) * alpha;
        b0 = A * ((A + 1.0f) - (A - 1.0f) * cosOmega + twoSqrtAAlpha);
        b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosOmega);
        b2 = A * ((A + 1.0f) - (A - 1.0f) * cosOmega - twoSqrtAAlpha);
        a0 = (A + 1.0f) + (A - 1.0f) * cosOmega + twoSqrtAAlpha;
        a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cosOmega);
        a2 = (A + 1.0f) + (A - 1.0f) * cosOmega - twoSqrtAAlpha;
      }
      break;

    case eBiquadHighShelf:
      {
        float twoSqrtAAlpha = 2.0f * sqrt(A) * alpha;
        b0 = A * ((A + 1.0f) + (A - 1.0f) * cosOmega + twoSqrtAAlpha);
        b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosOmega);
        b2 = A * ((A + 1.0f) + (A - 1.0f) * cosOmega - twoSqrtAAlpha);
        a0 = (A + 1.0f) - (A - 1.0f) * cosOmega + twoSqrtAAlpha;
        a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cosOmega);
        a2 = (A + 1.0f) - (A - 1.0f) * cosOmega - twoSqrtAAlpha;
      }
      break;

    case eBiquadPeakingEQ:
      {
        b0 = 1.0f + alpha * A;
        b1 = -2.0f * cosOmega;
        b2 = 1.0f - alpha * A;
        a0 = 1.0f + alpha / A;
        a1 = -2.0f * cosOmega;
        a2 = 1.0f - alpha / A;
      }
      break;

    case eBiquadLowPass:
      {
        b0 = (1.0f - cosOmega) / 2.0f;
        b1 = 1.0f - cosOmega;
        b2 = (1.0f - cosOmega) / 2.0f;
        a0 = 1.0f + alpha;
        a1 = -2.0f * cosOmega;
        a2 = 1.0f - alpha;
      }
      break;

    case eBiquadHighPass:
      {
        b0 = (1.0f + cosOmega) / 2.0f;
        b1 = -(1.0f + cosOmega);
        b2 = (1.0f + cosOmega) / 2.0f;
        a0 = 1.0f + alpha;
        a1 = -2.0f * cosOmega;
        a2 = 1.0f - alpha;
      }
      break;

    case eBiquadBandPass:
      {
        b0 = alpha;
        b1 = 0.0f;
        b2 = -alpha;
        a0 = 1.0f + alpha;
        a1 = -2.0f * cosOmega;
        a2 = 1.0f - alpha;
      }
      break;
  }

  // Normalize by a0
  if (abs(a0) > 1e-9f)
  {
    b0 /= a0;
    b1 /= a0;
    b2 /= a0;
    a1 /= a0;
    a2 /= a0;
  }
}

float JZBiquadFilter::Process(float inSample)
{
  float out = b0 * inSample + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
  x2 = x1;
  x1 = inSample;
  y2 = y1;
  y1 = out;
  return out;
}

// ============================================================================
// 2. JZ3BandEqualizer Implementation
// ============================================================================
JZ3BandEqualizer::JZ3BandEqualizer(float sampleRate)
  : mSampleRate(sampleRate)
{
  SetSettings(mSettings);
}

void JZ3BandEqualizer::SetSettings(const JZEqualizerSettings& settings)
{
  mSettings = settings;

  mFilterLowL.Configure(eBiquadLowShelf, mSampleRate, mSettings.bassFreqHz, 0.7071f, mSettings.bassGainDB);
  mFilterLowR.Configure(eBiquadLowShelf, mSampleRate, mSettings.bassFreqHz, 0.7071f, mSettings.bassGainDB);

  mFilterMidL.Configure(eBiquadPeakingEQ, mSampleRate, mSettings.midFreqHz, 1.0f, mSettings.midGainDB);
  mFilterMidR.Configure(eBiquadPeakingEQ, mSampleRate, mSettings.midFreqHz, 1.0f, mSettings.midGainDB);

  mFilterHighL.Configure(eBiquadHighShelf, mSampleRate, mSettings.trebleFreqHz, 0.7071f, mSettings.trebleGainDB);
  mFilterHighR.Configure(eBiquadHighShelf, mSampleRate, mSettings.trebleFreqHz, 0.7071f, mSettings.trebleGainDB);
}

void JZ3BandEqualizer::Process(float* left, float* right, int numSamples)
{
  for (int i = 0; i < numSamples; ++i)
  {
    float sL = left[i];
    sL = mFilterLowL.Process(sL);
    sL = mFilterMidL.Process(sL);
    sL = mFilterHighL.Process(sL);
    left[i] = sL;

    if (right)
    {
      float sR = right[i];
      sR = mFilterLowR.Process(sR);
      sR = mFilterMidR.Process(sR);
      sR = mFilterHighR.Process(sR);
      right[i] = sR;
    }
  }
}

void JZ3BandEqualizer::Process(short* buffer, int numSamples, int channels)
{
  if (!buffer || numSamples <= 0 || channels <= 0)
  {
    return;
  }

  if (channels == 1)
  {
    for (int i = 0; i < numSamples; ++i)
    {
      float s = buffer[i] / 32768.0f;
      s = mFilterLowL.Process(s);
      s = mFilterMidL.Process(s);
      s = mFilterHighL.Process(s);
      float clamped = max(-1.0f, min(1.0f, s));
      buffer[i] = static_cast<short>(clamped * 32767.0f);
    }
  }
  else
  {
    for (int i = 0; i < numSamples; ++i)
    {
      float sL = buffer[i * 2] / 32768.0f;
      float sR = buffer[i * 2 + 1] / 32768.0f;

      sL = mFilterLowL.Process(sL);
      sL = mFilterMidL.Process(sL);
      sL = mFilterHighL.Process(sL);

      sR = mFilterLowR.Process(sR);
      sR = mFilterMidR.Process(sR);
      sR = mFilterHighR.Process(sR);

      buffer[i * 2] = static_cast<short>(max(-1.0f, min(1.0f, sL)) * 32767.0f);
      buffer[i * 2 + 1] = static_cast<short>(max(-1.0f, min(1.0f, sR)) * 32767.0f);
    }
  }
}

// ============================================================================
// 3. JZFreeverb Reverb Implementation
// ============================================================================
JZCombFilter::JZCombFilter()
  : mBufSize(0), mBufIdx(0), mFeedback(0.8f), mFilterStore(0.0f), mDamp1(0.2f), mDamp2(0.8f)
{
}

void JZCombFilter::SetBuffer(int size)
{
  mBufSize = size;
  mBuffer.assign(size, 0.0f);
  mBufIdx = 0;
  mFilterStore = 0.0f;
}

void JZCombFilter::SetDamp(float val)
{
  mDamp1 = val;
  mDamp2 = 1.0f - val;
}

void JZCombFilter::SetFeedback(float val)
{
  mFeedback = val;
}

float JZCombFilter::Process(float input)
{
  if (mBufSize <= 0) return input;
  float output = mBuffer[mBufIdx];
  mFilterStore = (output * mDamp2) + (mFilterStore * mDamp1);
  mBuffer[mBufIdx] = input + (mFilterStore * mFeedback);
  if (++mBufIdx >= mBufSize) mBufIdx = 0;
  return output;
}

void JZCombFilter::Reset()
{
  std::fill(mBuffer.begin(), mBuffer.end(), 0.0f);
  mBufIdx = 0;
  mFilterStore = 0.0f;
}

JZAllpassFilter::JZAllpassFilter()
  : mBufSize(0), mBufIdx(0), mFeedback(0.5f)
{
}

void JZAllpassFilter::SetBuffer(int size)
{
  mBufSize = size;
  mBuffer.assign(size, 0.0f);
  mBufIdx = 0;
}

void JZAllpassFilter::SetFeedback(float val)
{
  mFeedback = val;
}

float JZAllpassFilter::Process(float input)
{
  if (mBufSize <= 0) return input;
  float bufout = mBuffer[mBufIdx];
  float output = -input + bufout;
  mBuffer[mBufIdx] = input + (bufout * mFeedback);
  if (++mBufIdx >= mBufSize) mBufIdx = 0;
  return output;
}

void JZAllpassFilter::Reset()
{
  std::fill(mBuffer.begin(), mBuffer.end(), 0.0f);
  mBufIdx = 0;
}

JZFreeverb::JZFreeverb(float sampleRate)
  : mSampleRate(sampleRate)
{
  static const int combTuning[NUM_COMBS] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
  static const int allpassTuning[NUM_ALLPASSES] = { 556, 441, 341, 225 };
  static const int stereoSpread = 23;

  float scale = sampleRate / 44100.0f;

  for (int i = 0; i < NUM_COMBS; ++i)
  {
    mCombL[i].SetBuffer(static_cast<int>(combTuning[i] * scale));
    mCombR[i].SetBuffer(static_cast<int>((combTuning[i] + stereoSpread) * scale));
  }

  for (int i = 0; i < NUM_ALLPASSES; ++i)
  {
    mAllpassL[i].SetBuffer(static_cast<int>(allpassTuning[i] * scale));
    mAllpassR[i].SetBuffer(static_cast<int>((allpassTuning[i] + stereoSpread) * scale));
    mAllpassL[i].SetFeedback(0.5f);
    mAllpassR[i].SetFeedback(0.5f);
  }

  UpdateParameters();
}

void JZFreeverb::SetSettings(const JZReverbSettings& settings)
{
  mSettings = settings;
  UpdateParameters();
}

void JZFreeverb::UpdateParameters()
{
  float room = 0.7f + mSettings.roomSize * 0.28f;
  float damp = mSettings.damping * 0.4f;

  for (int i = 0; i < NUM_COMBS; ++i)
  {
    mCombL[i].SetFeedback(room);
    mCombR[i].SetFeedback(room);
    mCombL[i].SetDamp(damp);
    mCombR[i].SetDamp(damp);
  }
}

void JZFreeverb::Reset()
{
  for (int i = 0; i < NUM_COMBS; ++i)
  {
    mCombL[i].Reset();
    mCombR[i].Reset();
  }
  for (int i = 0; i < NUM_ALLPASSES; ++i)
  {
    mAllpassL[i].Reset();
    mAllpassR[i].Reset();
  }
}

void JZFreeverb::Process(float* inL, float* inR, float* outL, float* outR, int numSamples)
{
  float wet1 = mSettings.wet * (mSettings.width / 2.0f + 0.5f);
  float wet2 = mSettings.wet * ((1.0f - mSettings.width) / 2.0f);
  float dry = mSettings.dry;

  for (int i = 0; i < numSamples; ++i)
  {
    float input = (inL[i] + (inR ? inR[i] : inL[i])) * 0.015f;
    float outSampleL = 0.0f;
    float outSampleR = 0.0f;

    for (int j = 0; j < NUM_COMBS; ++j)
    {
      outSampleL += mCombL[j].Process(input);
      outSampleR += mCombR[j].Process(input);
    }

    for (int j = 0; j < NUM_ALLPASSES; ++j)
    {
      outSampleL = mAllpassL[j].Process(outSampleL);
      outSampleR = mAllpassR[j].Process(outSampleR);
    }

    outL[i] = outSampleL * wet1 + outSampleR * wet2 + inL[i] * dry;
    if (outR)
    {
      outR[i] = outSampleR * wet1 + outSampleL * wet2 + (inR ? inR[i] : inL[i]) * dry;
    }
  }
}

void JZFreeverb::Process(short* buffer, int numSamples, int channels)
{
  if (!buffer || numSamples <= 0 || channels <= 0) return;

  vector<float> inL(numSamples), inR(numSamples);
  vector<float> outL(numSamples), outR(numSamples);

  if (channels == 1)
  {
    for (int i = 0; i < numSamples; ++i)
    {
      inL[i] = buffer[i] / 32768.0f;
    }
    Process(&inL[0], 0, &outL[0], 0, numSamples);
    for (int i = 0; i < numSamples; ++i)
    {
      buffer[i] = static_cast<short>(max(-1.0f, min(1.0f, outL[i])) * 32767.0f);
    }
  }
  else
  {
    for (int i = 0; i < numSamples; ++i)
    {
      inL[i] = buffer[i * 2] / 32768.0f;
      inR[i] = buffer[i * 2 + 1] / 32768.0f;
    }
    Process(&inL[0], &inR[0], &outL[0], &outR[0], numSamples);
    for (int i = 0; i < numSamples; ++i)
    {
      buffer[i * 2] = static_cast<short>(max(-1.0f, min(1.0f, outL[i])) * 32767.0f);
      buffer[i * 2 + 1] = static_cast<short>(max(-1.0f, min(1.0f, outR[i])) * 32767.0f);
    }
  }
}

// ============================================================================
// 4. JZStereoDelay Implementation
// ============================================================================
JZStereoDelay::JZStereoDelay(float sampleRate)
  : mSampleRate(sampleRate),
    mWriteIdxL(0),
    mWriteIdxR(0),
    mFilterStoreL(0.0f),
    mFilterStoreR(0.0f)
{
  int maxBuf = static_cast<int>(sampleRate * 2.5f); // 2.5 seconds buffer
  mBufferL.assign(maxBuf, 0.0f);
  mBufferR.assign(maxBuf, 0.0f);
}

void JZStereoDelay::SetSettings(const JZDelaySettings& settings)
{
  mSettings = settings;
}

void JZStereoDelay::Reset()
{
  std::fill(mBufferL.begin(), mBufferL.end(), 0.0f);
  std::fill(mBufferR.begin(), mBufferR.end(), 0.0f);
  mWriteIdxL = mWriteIdxR = 0;
  mFilterStoreL = mFilterStoreR = 0.0f;
}

void JZStereoDelay::Process(float* inL, float* inR, float* outL, float* outR, int numSamples)
{
  int bufLen = (int)mBufferL.size();
  int delaySampL = static_cast<int>(mSettings.delayTimeMsL * 0.001f * mSampleRate);
  int delaySampR = static_cast<int>(mSettings.delayTimeMsR * 0.001f * mSampleRate);

  delaySampL = max(1, min(bufLen - 1, delaySampL));
  delaySampR = max(1, min(bufLen - 1, delaySampR));

  float damp = mSettings.damp;
  float feedback = min(0.95f, max(0.0f, mSettings.feedback));

  for (int i = 0; i < numSamples; ++i)
  {
    int readIdxL = (mWriteIdxL - delaySampL + bufLen) % bufLen;
    float delayedL = mBufferL[readIdxL];
    mFilterStoreL = (delayedL * (1.0f - damp)) + (mFilterStoreL * damp);
    mBufferL[mWriteIdxL] = inL[i] + (mFilterStoreL * feedback);

    outL[i] = inL[i] * mSettings.dry + delayedL * mSettings.wet;
    if (++mWriteIdxL >= bufLen) mWriteIdxL = 0;

    if (outR)
    {
      float curInR = inR ? inR[i] : inL[i];
      int readIdxR = (mWriteIdxR - delaySampR + bufLen) % bufLen;
      float delayedR = mBufferR[readIdxR];
      mFilterStoreR = (delayedR * (1.0f - damp)) + (mFilterStoreR * damp);
      mBufferR[mWriteIdxR] = curInR + (mFilterStoreR * feedback);

      outR[i] = curInR * mSettings.dry + delayedR * mSettings.wet;
      if (++mWriteIdxR >= bufLen) mWriteIdxR = 0;
    }
  }
}

void JZStereoDelay::Process(short* buffer, int numSamples, int channels)
{
  if (!buffer || numSamples <= 0 || channels <= 0) return;

  vector<float> inL(numSamples), inR(numSamples);
  vector<float> outL(numSamples), outR(numSamples);

  if (channels == 1)
  {
    for (int i = 0; i < numSamples; ++i) inL[i] = buffer[i] / 32768.0f;
    Process(&inL[0], 0, &outL[0], 0, numSamples);
    for (int i = 0; i < numSamples; ++i)
    {
      buffer[i] = static_cast<short>(max(-1.0f, min(1.0f, outL[i])) * 32767.0f);
    }
  }
  else
  {
    for (int i = 0; i < numSamples; ++i)
    {
      inL[i] = buffer[i * 2] / 32768.0f;
      inR[i] = buffer[i * 2 + 1] / 32768.0f;
    }
    Process(&inL[0], &inR[0], &outL[0], &outR[0], numSamples);
    for (int i = 0; i < numSamples; ++i)
    {
      buffer[i * 2] = static_cast<short>(max(-1.0f, min(1.0f, outL[i])) * 32767.0f);
      buffer[i * 2 + 1] = static_cast<short>(max(-1.0f, min(1.0f, outR[i])) * 32767.0f);
    }
  }
}

// ============================================================================
// 5. JZStereoChorus Implementation
// ============================================================================
JZStereoChorus::JZStereoChorus(float sampleRate)
  : mSampleRate(sampleRate),
    mWriteIdx(0),
    mLfoPhase(0.0f)
{
  int maxBuf = static_cast<int>(sampleRate * 0.05f); // 50ms buffer
  mBufferL.assign(maxBuf, 0.0f);
  mBufferR.assign(maxBuf, 0.0f);
}

void JZStereoChorus::SetSettings(const JZChorusSettings& settings)
{
  mSettings = settings;
}

void JZStereoChorus::Reset()
{
  std::fill(mBufferL.begin(), mBufferL.end(), 0.0f);
  std::fill(mBufferR.begin(), mBufferR.end(), 0.0f);
  mWriteIdx = 0;
  mLfoPhase = 0.0f;
}

void JZStereoChorus::Process(float* left, float* right, int numSamples)
{
  int bufLen = (int)mBufferL.size();
  float baseDelay = 0.015f * mSampleRate; // 15ms
  float depthSamples = (mSettings.depthMs * 0.001f * mSampleRate);
  float lfoInc = (2.0f * (float)M_PI * mSettings.rateHz) / mSampleRate;

  for (int i = 0; i < numSamples; ++i)
  {
    float modL = sin(mLfoPhase) * depthSamples;
    float modR = cos(mLfoPhase) * depthSamples; // 90 degree stereo offset
    mLfoPhase += lfoInc;
    if (mLfoPhase >= 2.0f * (float)M_PI) mLfoPhase -= 2.0f * (float)M_PI;

    float readPosL = (float)mWriteIdx - (baseDelay + modL);
    while (readPosL < 0) readPosL += bufLen;
    int idxL = (int)readPosL % bufLen;
    float delayedL = mBufferL[idxL];

    mBufferL[mWriteIdx] = left[i] + delayedL * mSettings.feedback;
    left[i] = left[i] * (1.0f - mSettings.mix) + delayedL * mSettings.mix;

    if (right)
    {
      float readPosR = (float)mWriteIdx - (baseDelay + modR);
      while (readPosR < 0) readPosR += bufLen;
      int idxR = (int)readPosR % bufLen;
      float delayedR = mBufferR[idxR];

      mBufferR[mWriteIdx] = right[i] + delayedR * mSettings.feedback;
      right[i] = right[i] * (1.0f - mSettings.mix) + delayedR * mSettings.mix;
    }

    if (++mWriteIdx >= bufLen) mWriteIdx = 0;
  }
}

void JZStereoChorus::Process(short* buffer, int numSamples, int channels)
{
  if (!buffer || numSamples <= 0 || channels <= 0) return;

  vector<float> left(numSamples), right(numSamples);

  if (channels == 1)
  {
    for (int i = 0; i < numSamples; ++i) left[i] = buffer[i] / 32768.0f;
    Process(&left[0], 0, numSamples);
    for (int i = 0; i < numSamples; ++i)
    {
      buffer[i] = static_cast<short>(max(-1.0f, min(1.0f, left[i])) * 32767.0f);
    }
  }
  else
  {
    for (int i = 0; i < numSamples; ++i)
    {
      left[i] = buffer[i * 2] / 32768.0f;
      right[i] = buffer[i * 2 + 1] / 32768.0f;
    }
    Process(&left[0], &right[0], numSamples);
    for (int i = 0; i < numSamples; ++i)
    {
      buffer[i * 2] = static_cast<short>(max(-1.0f, min(1.0f, left[i])) * 32767.0f);
      buffer[i * 2 + 1] = static_cast<short>(max(-1.0f, min(1.0f, right[i])) * 32767.0f);
    }
  }
}

// ============================================================================
// 6. JZAudioLimiterDistortion Implementation
// ============================================================================
JZAudioLimiterDistortion::JZAudioLimiterDistortion()
{
}

void JZAudioLimiterDistortion::SetSettings(const JZLimiterSettings& settings)
{
  mSettings = settings;
}

void JZAudioLimiterDistortion::Process(float* left, float* right, int numSamples)
{
  float ceiling = pow(10.0f, mSettings.ceilingDB / 20.0f);
  ceiling = min(1.0f, max(0.1f, ceiling));
  float drive = max(1.0f, mSettings.drive);
  float norm = tanh(drive);

  for (int i = 0; i < numSamples; ++i)
  {
    float sL = left[i];
    if (mSettings.enableOverdrive && drive > 1.0f)
    {
      sL = tanh(sL * drive) / norm;
    }
    // Brickwall ceiling clamp
    left[i] = max(-ceiling, min(ceiling, sL));

    if (right)
    {
      float sR = right[i];
      if (mSettings.enableOverdrive && drive > 1.0f)
      {
        sR = tanh(sR * drive) / norm;
      }
      right[i] = max(-ceiling, min(ceiling, sR));
    }
  }
}

void JZAudioLimiterDistortion::Process(short* buffer, int numSamples, int channels)
{
  if (!buffer || numSamples <= 0 || channels <= 0) return;

  vector<float> left(numSamples), right(numSamples);

  if (channels == 1)
  {
    for (int i = 0; i < numSamples; ++i) left[i] = buffer[i] / 32768.0f;
    Process(&left[0], 0, numSamples);
    for (int i = 0; i < numSamples; ++i)
    {
      buffer[i] = static_cast<short>(max(-1.0f, min(1.0f, left[i])) * 32767.0f);
    }
  }
  else
  {
    for (int i = 0; i < numSamples; ++i)
    {
      left[i] = buffer[i * 2] / 32768.0f;
      right[i] = buffer[i * 2 + 1] / 32768.0f;
    }
    Process(&left[0], &right[0], numSamples);
    for (int i = 0; i < numSamples; ++i)
    {
      buffer[i * 2] = static_cast<short>(max(-1.0f, min(1.0f, left[i])) * 32767.0f);
      buffer[i * 2 + 1] = static_cast<short>(max(-1.0f, min(1.0f, right[i])) * 32767.0f);
    }
  }
}
