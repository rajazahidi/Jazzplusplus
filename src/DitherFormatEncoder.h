//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Master TPDF Dithering & Multi-Format Audio Encoder
//
// Audiophile master dithering engine featuring TPDF (Triangular Probability
// Density Function), Psychoacoustic Noise-Shaping (Lipshitz/E-Weighted),
// and multi-format export (16-bit CD Master, 24-bit Studio, 32-bit Float).
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <cmath>
#include <algorithm>

// Dither Algorithm Type
enum JZDitherType
{
  DITHER_NONE = 0,            // Truncation (not recommended for 16-bit)
  DITHER_RECTANGULAR_RPDF,    // Uniform noise
  DITHER_TRIANGULAR_TPDF,     // Industry standard TPDF (eliminates distortion & noise modulation)
  DITHER_PSYCHOACOUSTIC_SHAPED// Noise-shaped TPDF (shifts dither energy above 16-18kHz)
};

// Configuration for Master Dither & Encoder
struct JZDitherConfig
{
  JZDitherType ditherType;
  int targetBitDepth;         // 16, 24, 32
  bool autoDitherIfDownsampled;

  JZDitherConfig()
    : ditherType(DITHER_PSYCHOACOUSTIC_SHAPED),
      targetBitDepth(16),
      autoDitherIfDownsampled(true)
  {
  }
};

// Dither & Multi-Format Encoder Class
class JZDitherFormatEncoder
{
public:
  JZDitherFormatEncoder();
  ~JZDitherFormatEncoder() = default;

  void SetConfig(const JZDitherConfig& config) { mConfig = config; }
  const JZDitherConfig& GetConfig() const { return mConfig; }

  void Reset();

  // Quantize 32-bit float audio buffer to 16-bit signed PCM with TPDF Dither & Noise Shaping
  void QuantizeTo16Bit(
    const std::vector<float>& inL,
    const std::vector<float>& inR,
    std::vector<int16_t>& outL,
    std::vector<int16_t>& outR);

  // Quantize 32-bit float audio buffer to 24-bit signed PCM
  void QuantizeTo24Bit(
    const std::vector<float>& inL,
    const std::vector<float>& inR,
    std::vector<int32_t>& outL,
    std::vector<int32_t>& outR);

  // Full master container export to memory buffer
  bool RenderMasterWav(
    const std::vector<float>& inL,
    const std::vector<float>& inR,
    int sampleRate,
    std::vector<uint8_t>& outWavData);

private:
  JZDitherConfig mConfig;

  // PRNG state
  uint32_t mRngState1;
  uint32_t mRngState2;

  // Noise shaping feedback filter history
  float mNoiseShapingErrL1, mNoiseShapingErrL2;
  float mNoiseShapingErrR1, mNoiseShapingErrR2;

  float GenerateTpdfDither();
};
