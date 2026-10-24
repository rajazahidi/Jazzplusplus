//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Physical Modeling Wind & Brass Tube Resonator
//
// Acoustic digital waveguide physical model simulating wind & brass instruments
// (flutes, clarinets, saxophones, brass horns) with non-linear reed table,
// breath pressure modulation, air turbulence noise, and bell radiation damping.
//*****************************************************************************

#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

enum class JZWindInstrumentType
{
  Flute = 0,          // Jet-excited open cylinder
  Clarinet,           // Single-reed closed cylinder (odd harmonics)
  Saxophone,          // Conical single-reed bore
  BrassHorn           // Lip-reed brass horn
};

class JZWindTubePhysicalModel
{
public:
  JZWindTubePhysicalModel(float sampleRate = 44100.0f);
  ~JZWindTubePhysicalModel() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  // Wind Physics Controls
  void SetInstrumentType(JZWindInstrumentType type);
  JZWindInstrumentType GetInstrumentType() const { return mType; }

  void SetFrequencyHz(float freqHz);          // Musical note pitch (40Hz to 2000Hz)
  float GetFrequencyHz() const { return mFrequencyHz; }

  void SetBreathPressure(float pressure);     // 0.0 to 1.0 (blowing intensity)
  float GetBreathPressure() const { return mBreathPressure; }

  void SetBreathNoise(float noiseAmount);     // 0.0 to 0.5 (air turbulence breathiness)
  float GetBreathNoise() const { return mBreathNoise; }

  void SetBellDamping(float damping);         // 0.0 to 1.0 (bell high-frequency radiation)
  float GetBellDamping() const { return mBellDamping; }

  // Audio Synthesis
  float SynthesizeSample();
  void ProcessBuffer(std::vector<float>& outBuffer, size_t numSamples);

private:
  float mSampleRate;
  JZWindInstrumentType mType;
  float mFrequencyHz;
  float mBreathPressure;
  float mBreathNoise;
  float mBellDamping;

  // Waveguide Bore Delay Lines
  std::vector<float> mForwardDelay;
  std::vector<float> mBackwardDelay;
  size_t mWriteIdx;
  size_t mDelayLength;

  // Bell radiation filter state
  float mBellFilterState;

  void UpdateDelayLength();
  float NonLinearReed(float deltaPressure);
};
