//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Sub-Harmonic Synthesizer & Bass Reinforcer Implementation
//*****************************************************************************

#include "SubHarmonicSynthesizer.h"
#include <cmath>
#include <algorithm>

JZSubHarmonicSynthesizer::JZSubHarmonicSynthesizer(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f),
    mSubDrive(5.0f),
    mSubLevelDb(0.0f),
    mCleanBassLevelDb(0.0f),
    mSubBandFrequencyHz(36.0f), // Deep 36Hz sub fundamental
    mPolarityFlipL(false),
    mPolarityFlipR(false),
    mPrevInputL(0.0f),
    mPrevInputR(0.0f),
    mInputLpL(0.0f), mInputLpR(0.0f),
    mSubLpL(0.0f), mSubLpR(0.0f),
    mSubHpL(0.0f), mSubHpR(0.0f)
{
}

void JZSubHarmonicSynthesizer::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
  }
}

void JZSubHarmonicSynthesizer::SetSubDrive(float drive)
{
  mSubDrive = std::clamp(drive, 0.0f, 10.0f);
}

void JZSubHarmonicSynthesizer::SetSubLevelDb(float levelDb)
{
  mSubLevelDb = std::clamp(levelDb, -60.0f, 6.0f);
}

void JZSubHarmonicSynthesizer::SetCleanBassLevelDb(float levelDb)
{
  mCleanBassLevelDb = std::clamp(levelDb, -24.0f, 6.0f);
}

void JZSubHarmonicSynthesizer::SetSubBandFrequencyHz(float freqHz)
{
  mSubBandFrequencyHz = std::clamp(freqHz, 24.0f, 60.0f);
}

float JZSubHarmonicSynthesizer::SynthesizeSubOctave(
  float inSample,
  bool& flipFlop,
  float& prevSample,
  float& lpState,
  float& hpState)
{
  // 1. Low-pass filter to isolate low fundamental (< 110Hz)
  float inputLpCutoff = 110.0f;
  float alphaIn = std::exp(-2.0f * 3.14159265f * inputLpCutoff / mSampleRate);
  float filteredLow = alphaIn * inSample + (1.0f - alphaIn) * prevSample;

  // 2. Zero-crossing pitch divider (divide-by-2 flip-flop logic)
  if (prevSample <= 0.0f && filteredLow > 0.0f)
  {
    flipFlop = !flipFlop;
  }
  prevSample = filteredLow;

  float subSquare = flipFlop ? 1.0f : -1.0f;
  // Modulate sub amplitude by incoming bass envelope
  float subDriven = subSquare * std::abs(filteredLow) * (1.0f + mSubDrive * 0.4f);

  // 3. Sub-bass bandpass filter (24Hz to 60Hz) to reshape into clean sine sub wave
  float lpCoeff = std::exp(-2.0f * 3.14159265f * mSubBandFrequencyHz * 1.5f / mSampleRate);
  float hpCoeff = std::exp(-2.0f * 3.14159265f * 22.0f / mSampleRate);

  lpState = lpCoeff * lpState + (1.0f - lpCoeff) * subDriven;
  hpState = hpCoeff * hpState + (1.0f - hpCoeff) * lpState;
  float subSineWave = lpState - hpState;

  return std::tanh(subSineWave * 1.6f);
}

void JZSubHarmonicSynthesizer::Process(float& left, float& right)
{
  float subL = SynthesizeSubOctave(left, mPolarityFlipL, mPrevInputL, mSubLpL, mSubHpL);
  float subR = SynthesizeSubOctave(right, mPolarityFlipR, mPrevInputR, mSubLpR, mSubHpR);

  float cleanGain = std::pow(10.0f, mCleanBassLevelDb / 20.0f);
  float subGain = std::pow(10.0f, mSubLevelDb / 20.0f);

  // Reinforce low end with synthesized f0/2 sub octave
  left = std::clamp(left * cleanGain + subL * subGain, -1.0f, 1.0f);
  right = std::clamp(right * cleanGain + subR * subGain, -1.0f, 1.0f);
}
