//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Stereo Modulation Multi-FX Suite Implementation
//*****************************************************************************

#include "ModulationMultiFX.h"

JZModulationMultiFX::JZModulationMultiFX(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f),
    mDelayWritePos(0),
    mLfoPhase(0.0),
    mPhaserFeedbackL(0.0f),
    mPhaserFeedbackR(0.0f)
{
  mDelayL.resize(4096, 0.0f);
  mDelayR.resize(4096, 0.0f);
  Reset();
}

void JZModulationMultiFX::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
    Reset();
  }
}

void JZModulationMultiFX::Reset()
{
  std::fill(mDelayL.begin(), mDelayL.end(), 0.0f);
  std::fill(mDelayR.begin(), mDelayR.end(), 0.0f);
  mDelayWritePos = 0;
  mLfoPhase = 0.0;
  mPhaserFeedbackL = mPhaserFeedbackR = 0.0f;
  for (int i = 0; i < 12; ++i)
  {
    mPhaserL[i].Reset();
    mPhaserR[i].Reset();
  }
}

void JZModulationMultiFX::Process(float* inOutL, float* inOutR, int numSamples)
{
  if (!inOutL || numSamples <= 0) return;

  int bufLen = (int)mDelayL.size();
  double lfoInc = 2.0 * 3.141592653589793 * std::max(0.01f, mConfig.rateHz) / mSampleRate;
  float spreadOffset = mConfig.stereoSpread * 3.14159265f;

  for (int i = 0; i < numSamples; ++i)
  {
    float rawL = inOutL[i];
    float rawR = inOutR ? inOutR[i] : rawL;

    float wetL = 0.0f;
    float wetR = 0.0f;

    // --- MODE 1: STEREO DIMENSION CHORUS ---
    if (mConfig.mode == MOD_DIMENSION_CHORUS)
    {
      mDelayL[mDelayWritePos] = rawL;
      mDelayR[mDelayWritePos] = rawR;

      // Dual anti-phase LFOs (Roland Dimension D style)
      float lfoL = (float)std::sin(mLfoPhase);
      float lfoR = (float)std::sin(mLfoPhase + spreadOffset);

      // 4 cross-delayed lines (7ms and 15ms base centers)
      float delayL1 = 0.007f * mSampleRate + lfoL * (0.003f * mSampleRate * mConfig.depth);
      float delayR1 = 0.007f * mSampleRate + lfoR * (0.003f * mSampleRate * mConfig.depth);
      float delayL2 = 0.015f * mSampleRate - lfoL * (0.004f * mSampleRate * mConfig.depth);
      float delayR2 = 0.015f * mSampleRate - lfoR * (0.004f * mSampleRate * mConfig.depth);

      auto readDelay = [&](const std::vector<float>& buf, float d) {
        float rPos = (float)mDelayWritePos - d;
        if (rPos < 0.0f) rPos += (float)bufLen;
        int idx0 = (int)rPos;
        int idx1 = (idx0 + 1) % bufLen;
        float frac = rPos - (float)idx0;
        return buf[idx0] + frac * (buf[idx1] - buf[idx0]);
      };

      float cL = readDelay(mDelayL, delayL1) + 0.7f * readDelay(mDelayR, delayL2);
      float cR = readDelay(mDelayR, delayR1) + 0.7f * readDelay(mDelayL, delayR2);

      wetL = cL * 0.6f;
      wetR = cR * 0.6f;
    }
    // --- MODE 2: 12-STAGE OPTICAL PHASER ---
    else if (mConfig.mode == MOD_OPTICAL_PHASER_12)
    {
      float lfoL = 0.5f + 0.5f * (float)std::sin(mLfoPhase);
      float lfoR = 0.5f + 0.5f * (float)std::sin(mLfoPhase + spreadOffset);

      // Sweep all-pass cutoff from 200 Hz to 4500 Hz
      float fMin = 200.0f, fMax = 4500.0f;
      float fL = fMin * std::pow(fMax / fMin, lfoL * mConfig.depth);
      float fR = fMin * std::pow(fMax / fMin, lfoR * mConfig.depth);

      float tL = std::tan(3.14159265f * fL / mSampleRate);
      float aL = (tL - 1.0f) / (tL + 1.0f);

      float tR = std::tan(3.14159265f * fR / mSampleRate);
      float aR = (tR - 1.0f) / (tR + 1.0f);

      float sigL = rawL + mPhaserFeedbackL * mConfig.feedback;
      float sigR = rawR + mPhaserFeedbackR * mConfig.feedback;

      for (int s = 0; s < 12; ++s)
      {
        sigL = mPhaserL[s].Process(sigL, aL);
        sigR = mPhaserR[s].Process(sigR, aR);
      }

      mPhaserFeedbackL = sigL;
      mPhaserFeedbackR = sigR;

      wetL = sigL;
      wetR = sigR;
    }
    // --- MODE 3: THROUGH-ZERO TAPE FLANGER ---
    else
    {
      mDelayL[mDelayWritePos] = rawL;
      mDelayR[mDelayWritePos] = rawR;

      // Flanger delay sweeps down through zero (crossing point cancellation)
      float lfo = 0.5f + 0.5f * (float)std::sin(mLfoPhase);
      float delayMs = lfo * (4.5f * mConfig.depth); // 0.0ms to 4.5ms
      float delaySamples = delayMs * 0.001f * mSampleRate;

      float rPos = (float)mDelayWritePos - delaySamples;
      if (rPos < 0.0f) rPos += (float)bufLen;
      int idx0 = (int)rPos;
      int idx1 = (idx0 + 1) % bufLen;
      float frac = rPos - (float)idx0;

      float flgL = mDelayL[idx0] + frac * (mDelayL[idx1] - mDelayL[idx0]);
      float flgR = mDelayR[idx0] + frac * (mDelayR[idx1] - mDelayR[idx0]);

      // Phase inversion through zero creates complete cancellation notches
      wetL = (rawL - flgL) * 0.7f;
      wetR = (rawR - flgR) * 0.7f;
    }

    mDelayWritePos = (mDelayWritePos + 1) % bufLen;
    mLfoPhase += lfoInc;
    if (mLfoPhase >= 6.283185307179586) mLfoPhase -= 6.283185307179586;

    // Dry / Wet mix
    inOutL[i] = (rawL * (1.0f - mConfig.dryWet)) + (wetL * mConfig.dryWet);
    if (inOutR)
    {
      inOutR[i] = (rawR * (1.0f - mConfig.dryWet)) + (wetR * mConfig.dryWet);
    }
  }
}

void JZModulationMultiFX::ProcessMono(float* inOut, int numSamples)
{
  Process(inOut, nullptr, numSamples);
}
