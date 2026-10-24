//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Sub-Harmonic Synthesizer & Bass Reinforcer (dbx 120A Style)
//
// Dual-band sub-harmonic synthesizer dividing incoming low-end fundamentals
// by 2 (f0 / 2) to generate bone-shaking synthesized sub-bass (24Hz to 56Hz)
// without cluttering the musical mid-bass frequency register.
//*****************************************************************************

#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

class JZSubHarmonicSynthesizer
{
public:
  JZSubHarmonicSynthesizer(float sampleRate = 44100.0f);
  ~JZSubHarmonicSynthesizer() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  // Sub-Bass Controls
  void SetSubDrive(float drive);              // 0.0 to 10.0
  float GetSubDrive() const { return mSubDrive; }

  void SetSubLevelDb(float levelDb);          // -60 dB to +6 dB
  float GetSubLevelDb() const { return mSubLevelDb; }

  void SetCleanBassLevelDb(float levelDb);    // -24 dB to +6 dB
  float GetCleanBassLevelDb() const { return mCleanBassLevelDb; }

  void SetSubBandFrequencyHz(float freqHz);   // 28Hz to 56Hz
  float GetSubBandFrequencyHz() const { return mSubBandFrequencyHz; }

  // Real-time Stereo Processing
  void Process(float& left, float& right);

private:
  float mSampleRate;
  float mSubDrive;
  float mSubLevelDb;
  float mCleanBassLevelDb;
  float mSubBandFrequencyHz;

  // Pitch Divider Flip-Flop States
  bool  mPolarityFlipL;
  bool  mPolarityFlipR;
  float mPrevInputL;
  float mPrevInputR;

  // Filter States (Sub-bass bandpass & clean bass crossover)
  float mInputLpL, mInputLpR;
  float mSubLpL, mSubLpR;
  float mSubHpL, mSubHpR;

  float SynthesizeSubOctave(float inSample, bool& flipFlop, float& prevSample, float& lpState, float& hpState);
};
