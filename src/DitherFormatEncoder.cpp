//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Master TPDF Dithering & Multi-Format Audio Encoder Implementation
//*****************************************************************************

#include "DitherFormatEncoder.h"
#include "StemsExporter.h" // For WAV header utility

JZDitherFormatEncoder::JZDitherFormatEncoder()
  : mRngState1(123456789),
    mRngState2(987654321),
    mNoiseShapingErrL1(0.0f), mNoiseShapingErrL2(0.0f),
    mNoiseShapingErrR1(0.0f), mNoiseShapingErrR2(0.0f)
{
}

void JZDitherFormatEncoder::Reset()
{
  mRngState1 = 123456789;
  mRngState2 = 987654321;
  mNoiseShapingErrL1 = mNoiseShapingErrL2 = 0.0f;
  mNoiseShapingErrR1 = mNoiseShapingErrR2 = 0.0f;
}

float JZDitherFormatEncoder::GenerateTpdfDither()
{
  // Fast Linear Congruential Generator
  mRngState1 = mRngState1 * 1664525u + 1013904223u;
  mRngState2 = mRngState2 * 1103515245u + 12345u;

  float r1 = ((float)(mRngState1 & 0x7FFFFFFF) / 2147483648.0f) - 0.5f;
  float r2 = ((float)(mRngState2 & 0x7FFFFFFF) / 2147483648.0f) - 0.5f;

  // Triangular PDF: difference between two uniform variables in [-0.5, 0.5]
  return r1 - r2;
}

void JZDitherFormatEncoder::QuantizeTo16Bit(
  const std::vector<float>& inL,
  const std::vector<float>& inR,
  std::vector<int16_t>& outL,
  std::vector<int16_t>& outR)
{
  int n = (int)std::min(inL.size(), inR.size());
  outL.resize(n);
  outR.resize(n);

  const float scale16 = 32767.0f;
  const float invScale16 = 1.0f / scale16;

  for (int i = 0; i < n; ++i)
  {
    float sampleL = inL[i] * scale16;
    float sampleR = inR[i] * scale16;

    // Apply Noise Shaping error feedback
    if (mConfig.ditherType == DITHER_PSYCHOACOUSTIC_SHAPED)
    {
      // 2nd-order high-pass feedback: e[n] - 1.62 * e[n-1] + 0.81 * e[n-2]
      sampleL += (1.62f * mNoiseShapingErrL1 - 0.81f * mNoiseShapingErrL2);
      sampleR += (1.62f * mNoiseShapingErrR1 - 0.81f * mNoiseShapingErrR2);
    }

    // Add Dither noise
    float ditherL = 0.0f;
    float ditherR = 0.0f;

    if (mConfig.ditherType == DITHER_TRIANGULAR_TPDF || mConfig.ditherType == DITHER_PSYCHOACOUSTIC_SHAPED)
    {
      ditherL = GenerateTpdfDither();
      ditherR = GenerateTpdfDither();
    }
    else if (mConfig.ditherType == DITHER_RECTANGULAR_RPDF)
    {
      mRngState1 = mRngState1 * 1664525u + 1013904223u;
      ditherL = ((float)(mRngState1 & 0x7FFFFFFF) / 2147483648.0f) - 0.5f;
      mRngState2 = mRngState2 * 1103515245u + 12345u;
      ditherR = ((float)(mRngState2 & 0x7FFFFFFF) / 2147483648.0f) - 0.5f;
    }

    float quantL = std::floor(sampleL + ditherL + 0.5f);
    float quantR = std::floor(sampleR + ditherR + 0.5f);

    // Save error for noise shaping
    mNoiseShapingErrL2 = mNoiseShapingErrL1;
    mNoiseShapingErrL1 = quantL - sampleL;
    mNoiseShapingErrR2 = mNoiseShapingErrR1;
    mNoiseShapingErrR1 = quantR - sampleR;

    int16_t outSampleL = (int16_t)std::max(-32768.0f, std::min(32767.0f, quantL));
    int16_t outSampleR = (int16_t)std::max(-32768.0f, std::min(32767.0f, quantR));

    outL[i] = outSampleL;
    outR[i] = outSampleR;
  }
}

void JZDitherFormatEncoder::QuantizeTo24Bit(
  const std::vector<float>& inL,
  const std::vector<float>& inR,
  std::vector<int32_t>& outL,
  std::vector<int32_t>& outR)
{
  int n = (int)std::min(inL.size(), inR.size());
  outL.resize(n);
  outR.resize(n);

  const float scale24 = 8388607.0f;

  for (int i = 0; i < n; ++i)
  {
    float sampleL = inL[i] * scale24;
    float sampleR = inR[i] * scale24;

    float ditherL = 0.0f;
    float ditherR = 0.0f;
    if (mConfig.ditherType != DITHER_NONE)
    {
      ditherL = GenerateTpdfDither();
      ditherR = GenerateTpdfDither();
    }

    float qL = std::floor(sampleL + ditherL + 0.5f);
    float qR = std::floor(sampleR + ditherR + 0.5f);

    outL[i] = (int32_t)std::max(-8388608.0f, std::min(8388607.0f, qL));
    outR[i] = (int32_t)std::max(-8388608.0f, std::min(8388607.0f, qR));
  }
}

bool JZDitherFormatEncoder::RenderMasterWav(
  const std::vector<float>& inL,
  const std::vector<float>& inR,
  int sampleRate,
  std::vector<uint8_t>& outWavData)
{
  int numSamples = (int)std::min(inL.size(), inR.size());
  if (numSamples <= 0) return false;

  int bits = mConfig.targetBitDepth;
  if (bits != 16 && bits != 24 && bits != 32) bits = 16;

  int bytesPerSample = bits / 8;
  int numChannels = 2;
  int dataBytes = numSamples * numChannels * bytesPerSample;

  std::vector<uint8_t> header;
  JZStemsExporter::BuildWavHeader(header, numChannels, sampleRate, bits, dataBytes);

  outWavData.clear();
  outWavData.reserve(header.size() + dataBytes);
  outWavData.insert(outWavData.end(), header.begin(), header.end());

  if (bits == 16)
  {
    std::vector<int16_t> qL, qR;
    QuantizeTo16Bit(inL, inR, qL, qR);
    for (int i = 0; i < numSamples; ++i)
    {
      uint8_t* pl = (uint8_t*)&qL[i];
      uint8_t* pr = (uint8_t*)&qR[i];
      outWavData.push_back(pl[0]); outWavData.push_back(pl[1]);
      outWavData.push_back(pr[0]); outWavData.push_back(pr[1]);
    }
  }
  else if (bits == 24)
  {
    std::vector<int32_t> qL, qR;
    QuantizeTo24Bit(inL, inR, qL, qR);
    for (int i = 0; i < numSamples; ++i)
    {
      int32_t l = qL[i];
      int32_t r = qR[i];
      outWavData.push_back((uint8_t)(l & 0xFF));
      outWavData.push_back((uint8_t)((l >> 8) & 0xFF));
      outWavData.push_back((uint8_t)((l >> 16) & 0xFF));
      outWavData.push_back((uint8_t)(r & 0xFF));
      outWavData.push_back((uint8_t)((r >> 8) & 0xFF));
      outWavData.push_back((uint8_t)((r >> 16) & 0xFF));
    }
  }
  else // 32-bit float
  {
    for (int i = 0; i < numSamples; ++i)
    {
      float l = inL[i];
      float r = inR[i];
      uint8_t* pl = (uint8_t*)&l;
      uint8_t* pr = (uint8_t*)&r;
      for (int b = 0; b < 4; ++b) outWavData.push_back(pl[b]);
      for (int b = 0; b < 4; ++b) outWavData.push_back(pr[b]);
    }
  }

  return true;
}
