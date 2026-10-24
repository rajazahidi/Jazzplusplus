//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Analog Tape Machine & Vintage Console Warmer Implementation
//*****************************************************************************

#include "TapeConsoleWarmer.h"

JZTapeConsoleWarmer::JZTapeConsoleWarmer(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f),
    mDelayWritePos(0),
    mLfoPhase(0.0f),
    mHbX1L(0), mHbX2L(0), mHbY1L(0), mHbY2L(0),
    mHbX1R(0), mHbX2R(0), mHbY1R(0), mHbY2R(0)
{
  mDelayL.resize(512, 0.0f);
  mDelayR.resize(512, 0.0f);
  UpdateFilterCoefficients();
}

void JZTapeConsoleWarmer::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
    UpdateFilterCoefficients();
  }
}

void JZTapeConsoleWarmer::SetConfig(const JZTapeConsoleConfig& config)
{
  bool speedChanged = (mConfig.tapeSpeed != config.tapeSpeed);
  mConfig = config;
  if (speedChanged)
  {
    UpdateFilterCoefficients();
  }
}

void JZTapeConsoleWarmer::Reset()
{
  std::fill(mDelayL.begin(), mDelayL.end(), 0.0f);
  std::fill(mDelayR.begin(), mDelayR.end(), 0.0f);
  mDelayWritePos = 0;
  mLfoPhase = 0.0f;
  mHbX1L = mHbX2L = mHbY1L = mHbY2L = 0.0f;
  mHbX1R = mHbX2R = mHbY1R = mHbY2R = 0.0f;
}

void JZTapeConsoleWarmer::UpdateFilterCoefficients()
{
  // 15 IPS low-frequency tape head bump resonance (~70Hz, +2.5dB peak)
  float f0 = (mConfig.tapeSpeed == TAPE_SPEED_15_IPS) ? 72.0f : 120.0f;
  float gainDB = (mConfig.tapeSpeed == TAPE_SPEED_15_IPS) ? 2.5f : 0.8f;
  float Q = 1.2f;

  float w0 = 2.0f * 3.14159265f * f0 / mSampleRate;
  float A = std::pow(10.0f, gainDB / 40.0f);
  float alpha = std::sin(w0) / (2.0f * Q);

  float a0 = 1.0f + alpha / A;
  hb_b0 = (1.0f + alpha * A) / a0;
  hb_b1 = (-2.0f * std::cos(w0)) / a0;
  hb_b2 = (1.0f - alpha * A) / a0;
  hb_a1 = (-2.0f * std::cos(w0)) / a0;
  hb_a2 = (1.0f - alpha / A) / a0;
}

float JZTapeConsoleWarmer::SaturateSample(float x, float driveLin) const
{
  float driven = x * driveLin;

  // 1. Core magnetic soft saturation (tanh curve)
  float sat = std::tanh(driven);

  // 2. Add Even harmonics (tube warmth: x^2)
  float evenHarm = mConfig.tubeHarmonicsEven * 0.15f * (driven * driven);
  if (driven < 0.0f) evenHarm = -evenHarm; // preserve polarity

  // 3. Add Odd harmonics (tape saturation: x^3)
  float oddHarm = mConfig.tapeHarmonicsOdd * 0.12f * (driven * driven * driven);

  float combined = sat + evenHarm - oddHarm;
  // Normalize output volume
  return combined / (1.0f + (driveLin - 1.0f) * 0.25f);
}

void JZTapeConsoleWarmer::Process(float* inOutL, float* inOutR, int numSamples)
{
  if (!inOutL || numSamples <= 0) return;

  float driveLin = std::pow(10.0f, mConfig.driveDB / 20.0f);
  int bufLen = (int)mDelayL.size();
  float lfoInc = 2.0f * 3.14159265f * 1.2f / mSampleRate; // 1.2 Hz wow flutter motor flutter

  for (int i = 0; i < numSamples; ++i)
  {
    float rawL = inOutL[i];
    float rawR = inOutR ? inOutR[i] : rawL;

    // 1. Apply low-end head bump filter
    float bumpL = hb_b0 * rawL + hb_b1 * mHbX1L + hb_b2 * mHbX2L - hb_a1 * mHbY1L - hb_a2 * mHbY2L;
    mHbX2L = mHbX1L; mHbX1L = rawL; mHbY2L = mHbY1L; mHbY1L = bumpL;

    float bumpR = bumpL;
    if (inOutR)
    {
      bumpR = hb_b0 * rawR + hb_b1 * mHbX1R + hb_b2 * mHbX2R - hb_a1 * mHbY1R - hb_a2 * mHbY2R;
      mHbX2R = mHbX1R; mHbX1R = rawR; mHbY2R = mHbY1R; mHbY1R = bumpR;
    }

    // 2. Magnetic Tape Hysteresis & Harmonic Saturation
    float satL = SaturateSample(bumpL, driveLin);
    float satR = SaturateSample(bumpR, driveLin);

    // 3. Wow & Flutter Modulation
    if (mConfig.wowFlutterDepth > 0.001f)
    {
      mDelayL[mDelayWritePos] = satL;
      mDelayR[mDelayWritePos] = satR;

      float lfo = std::sin(mLfoPhase) * (mConfig.wowFlutterDepth * 3.0f);
      mLfoPhase += lfoInc;
      if (mLfoPhase >= 6.2831853f) mLfoPhase -= 6.2831853f;

      float delaySamples = 10.0f + lfo;
      float rPos = (float)mDelayWritePos - delaySamples;
      if (rPos < 0.0f) rPos += (float)bufLen;
      int idx0 = (int)rPos;
      int idx1 = (idx0 + 1) % bufLen;
      float frac = rPos - (float)idx0;

      satL = mDelayL[idx0] + frac * (mDelayL[idx1] - mDelayL[idx0]);
      satR = mDelayR[idx0] + frac * (mDelayR[idx1] - mDelayR[idx0]);

      mDelayWritePos = (mDelayWritePos + 1) % bufLen;
    }

    // 4. Dry / Wet mix
    inOutL[i] = (rawL * (1.0f - mConfig.dryWet)) + (satL * mConfig.dryWet);
    if (inOutR)
    {
      inOutR[i] = (rawR * (1.0f - mConfig.dryWet)) + (satR * mConfig.dryWet);
    }
  }
}

void JZTapeConsoleWarmer::ProcessMono(float* inOut, int numSamples)
{
  Process(inOut, nullptr, numSamples);
}
