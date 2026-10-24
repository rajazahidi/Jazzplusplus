//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Convolution Reverb & Impulse Response (IR) Cabinet Simulator Implementation
//*****************************************************************************

#include "ConvolutionEngine.h"

JZConvolutionEngine::JZConvolutionEngine(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f),
    mHistoryPos(0),
    mPreDelayWritePos(0)
{
  mPreDelayBufL.resize(8820, 0.0f); // Up to 200ms pre-delay
  mPreDelayBufR.resize(8820, 0.0f);
  RebuildImpulse();
}

void JZConvolutionEngine::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
    RebuildImpulse();
  }
}

void JZConvolutionEngine::SetConfig(const JZConvolutionConfig& config)
{
  bool needRebuild = (mConfig.irType != config.irType ||
                      std::fabs(mConfig.decayScale - config.decayScale) > 0.01f);
  mConfig = config;
  if (needRebuild)
  {
    RebuildImpulse();
  }
}

void JZConvolutionEngine::LoadPreset(JZImpulseType type)
{
  mConfig.irType = type;
  RebuildImpulse();
}

void JZConvolutionEngine::Reset()
{
  std::fill(mHistoryL.begin(), mHistoryL.end(), 0.0f);
  std::fill(mHistoryR.begin(), mHistoryR.end(), 0.0f);
  std::fill(mPreDelayBufL.begin(), mPreDelayBufL.end(), 0.0f);
  std::fill(mPreDelayBufR.begin(), mPreDelayBufR.end(), 0.0f);
  mHistoryPos = 0;
  mPreDelayWritePos = 0;
}

void JZConvolutionEngine::LoadCustomIR(const std::vector<float>& irL, const std::vector<float>& irR)
{
  mConfig.irType = IR_CUSTOM;
  mIrLeft = irL;
  mIrRight = !irR.empty() ? irR : irL;

  int irLen = (int)mIrLeft.size();
  mHistoryL.assign(irLen, 0.0f);
  mHistoryR.assign(irLen, 0.0f);
  mHistoryPos = 0;
}

void JZConvolutionEngine::RebuildImpulse()
{
  if (mConfig.irType == IR_CUSTOM && !mIrLeft.empty())
  {
    return;
  }

  GenerateImpulseResponse(mConfig.irType, mSampleRate, mConfig.decayScale, mIrLeft, mIrRight);

  int irLen = (int)mIrLeft.size();
  mHistoryL.assign(irLen, 0.0f);
  mHistoryR.assign(irLen, 0.0f);
  mHistoryPos = 0;
}

void JZConvolutionEngine::GenerateImpulseResponse(
  JZImpulseType type,
  float sampleRate,
  float decayScale,
  std::vector<float>& outIrL,
  std::vector<float>& outIrR)
{
  int lengthSamples = 1024;
  float decayRate = 1.0f;
  float earlyGain = 0.5f;

  switch (type)
  {
    case IR_CONCERT_HALL:
      lengthSamples = (int)(sampleRate * 2.2f * std::max(0.2f, decayScale));
      decayRate = 3.5f;
      break;

    case IR_JAZZ_CLUB:
      lengthSamples = (int)(sampleRate * 0.85f * std::max(0.2f, decayScale));
      decayRate = 6.0f;
      break;

    case IR_VINTAGE_SPRING:
      lengthSamples = (int)(sampleRate * 1.2f * std::max(0.2f, decayScale));
      decayRate = 4.5f;
      break;

    case IR_GUITAR_CAB_4X12:
      // Speaker cabinet IRs are tight and punchy (15ms - 40ms)
      lengthSamples = (int)(sampleRate * 0.035f);
      decayRate = 50.0f;
      break;

    case IR_BASS_CAB_8X10:
      // Bass cab IR with strong low-end resonance
      lengthSamples = (int)(sampleRate * 0.045f);
      decayRate = 40.0f;
      break;

    case IR_CUSTOM:
    default:
      lengthSamples = (int)(sampleRate * 1.0f);
      decayRate = 5.0f;
      break;
  }

  // Limit maximum convolution length for direct processing performance (max 4096 samples)
  lengthSamples = std::max(128, std::min(4096, lengthSamples));

  outIrL.resize(lengthSamples, 0.0f);
  outIrR.resize(lengthSamples, 0.0f);

  // Direct impulse at t=0
  outIrL[0] = 1.0f;
  outIrR[0] = 1.0f;

  float energySum = 1.0f;

  if (type == IR_GUITAR_CAB_4X12 || type == IR_BASS_CAB_8X10)
  {
    // Speaker cabinet acoustic resonant profile: resonant bandpass around 90-120Hz and 2.5-4kHz
    float resonanceFreq = (type == IR_GUITAR_CAB_4X12) ? 2800.0f : 110.0f;
    float w0 = 2.0f * 3.14159265f * resonanceFreq / sampleRate;

    for (int i = 1; i < lengthSamples; ++i)
    {
      float t = (float)i / sampleRate;
      float env = std::exp(-decayRate * t * 100.0f);
      float osc = std::sin(w0 * (float)i);

      outIrL[i] = env * osc * 0.4f;
      outIrR[i] = env * std::sin(w0 * (float)i + 0.15f) * 0.4f;
      energySum += outIrL[i] * outIrL[i];
    }
  }
  else
  {
    // Reverb diffuse tail with exponential decay and pseudo-random scattering
    uint32_t seedL = 12345;
    uint32_t seedR = 67890;

    auto nextRand = [](uint32_t& s) -> float {
      s = s * 1664525u + 1013904223u;
      return ((float)(s & 0xFFFF) / 32768.0f) - 1.0f;
    };

    for (int i = 1; i < lengthSamples; ++i)
    {
      float t = (float)i / (float)lengthSamples;
      float envelope = std::exp(-decayRate * t);

      // Low pass damping
      float damping = 1.0f - (0.5f * t);

      float noiseL = nextRand(seedL) * envelope * damping;
      float noiseR = nextRand(seedR) * envelope * damping;

      // Add discrete early reflection peaks at fixed intervals
      if (i == (int)(sampleRate * 0.012f) || i == (int)(sampleRate * 0.024f) || i == (int)(sampleRate * 0.038f))
      {
        noiseL += earlyGain * envelope;
        noiseR += earlyGain * envelope * 0.85f;
      }

      outIrL[i] = noiseL;
      outIrR[i] = noiseR;
      energySum += (noiseL * noiseL + noiseR * noiseR) * 0.5f;
    }
  }

  // Energy normalization
  float normFactor = 1.0f / std::sqrt(std::max(1e-4f, energySum));
  for (int i = 0; i < lengthSamples; ++i)
  {
    outIrL[i] *= normFactor;
    outIrR[i] *= normFactor;
  }
}

void JZConvolutionEngine::Process(float* inOutL, float* inOutR, int numSamples)
{
  if (!inOutL || numSamples <= 0 || mIrLeft.empty()) return;

  int irLen = (int)mIrLeft.size();
  int preDelaySamples = (int)(mSampleRate * std::max(0.0f, mConfig.preDelayMs) * 0.001f);
  preDelaySamples = std::max(0, std::min((int)mPreDelayBufL.size() - 1, preDelaySamples));

  for (int i = 0; i < numSamples; ++i)
  {
    float inL = inOutL[i];
    float inR = inOutR ? inOutR[i] : inL;

    // 1. Write input to pre-delay buffer
    mPreDelayBufL[mPreDelayWritePos] = inL;
    mPreDelayBufR[mPreDelayWritePos] = inR;

    // 2. Read delayed sample for convolution
    int readPos = (mPreDelayWritePos - preDelaySamples + (int)mPreDelayBufL.size()) % (int)mPreDelayBufL.size();
    float delayedL = mPreDelayBufL[readPos];
    float delayedR = mPreDelayBufR[readPos];

    mPreDelayWritePos = (mPreDelayWritePos + 1) % (int)mPreDelayBufL.size();

    // 3. Store into circular convolution history buffer
    mHistoryL[mHistoryPos] = delayedL;
    mHistoryR[mHistoryPos] = delayedR;

    // 4. Time-domain convolution sum: y[n] = sum( x[n-k] * h[k] )
    float convOutL = 0.0f;
    float convOutR = 0.0f;

    const float* hL = mIrLeft.data();
    const float* hR = mIrRight.data();
    int hPos = mHistoryPos;

    for (int k = 0; k < irLen; ++k)
    {
      float xL = mHistoryL[hPos];
      float xR = mHistoryR[hPos];

      convOutL += xL * hL[k];
      convOutR += xR * hR[k];

      hPos--;
      if (hPos < 0) hPos += irLen;
    }

    mHistoryPos = (mHistoryPos + 1) % irLen;

    // 5. Mix Dry and Wet signals
    inOutL[i] = (inL * mConfig.dryGain) + (convOutL * mConfig.wetGain);
    if (inOutR)
    {
      inOutR[i] = (inR * mConfig.dryGain) + (convOutR * mConfig.wetGain);
    }
  }
}

void JZConvolutionEngine::ProcessMono(float* inOut, int numSamples)
{
  Process(inOut, nullptr, numSamples);
}
