//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Master Linear Phase FIR Crossover & Psychoacoustic Stereo Imager
//
// Mastering-grade 4-band zero-phase crossover (Sub, Low-Mid, High-Mid, Air)
// with independent psychoacoustic stereo width scaling and zero-phase
// elliptical sub-bass mono centering.
//*****************************************************************************

#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

struct JZLinearBandWidth
{
  float crossoverFreq;
  float widthPercent;         // 0% (mono) to 200% (super-wide)

  JZLinearBandWidth(float freq = 1000.0f, float width = 100.0f)
    : crossoverFreq(freq),
      widthPercent(width)
  {
  }
};

class JZMasterLinearPhaseImager
{
public:
  JZMasterLinearPhaseImager(float sampleRate = 44100.0f);
  ~JZMasterLinearPhaseImager() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  // Band Width Controls
  void SetBandWidthPercent(int bandIdx, float widthPercent); // 0=Sub, 1=LowMid, 2=HighMid, 3=Air
  float GetBandWidthPercent(int bandIdx) const;

  void SetEllipticalSubMono(bool enable) { mEllipticalSubMono = enable; }
  bool IsEllipticalSubMono() const { return mEllipticalSubMono; }

  // Stereo Real-Time Processing
  void Process(float& left, float& right);

private:
  float mSampleRate;
  bool  mEllipticalSubMono;
  std::vector<JZLinearBandWidth> mBands;

  // Zero-phase symmetric FIR coefficients & delay line
  static const size_t kFirTaps = 31;
  std::vector<float> mDelayL;
  std::vector<float> mDelayR;
  size_t mWriteIdx;

  void Filter4Bands(
    float inL, float inR,
    float outSub[2], float outLowMid[2], float outHighMid[2], float outAir[2]);
};
