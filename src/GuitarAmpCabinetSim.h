//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Analog Guitar Tube Amp & Speaker Cabinet IR Simulator
//
// High-fidelity tube amplifier modeler featuring non-linear 12AX7 triode pre-amp,
// EL34/6L6 power stage saturation, interactive 3-band tone stack (British/American),
// and speaker cabinet impulse response (IR) convolution filtering.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

enum class JZAmpModel
{
  AmericanClean = 0,    // Warm Twin/Blackface clean tone
  BritishCrunch,        // Classic Plexi/JCM crunch tone
  ModernHighGain        // Rectified high-gain lead tone
};

enum class JZCabinetModel
{
  Bypass = 0,
  Cab1x12Vintage,       // 1x12 Vintage Tweed combo
  Cab2x12British,       // 2x12 British Open-back
  Cab4x12HighGain       // 4x12 High-gain closed stack
};

class JZGuitarAmpCabinetSim
{
public:
  JZGuitarAmpCabinetSim(float sampleRate = 44100.0f);
  ~JZGuitarAmpCabinetSim() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  // Amp Controls
  void SetAmpModel(JZAmpModel model);
  JZAmpModel GetAmpModel() const { return mAmpModel; }

  void SetGain(float gain); // 0.0 to 10.0 (clean to ultra-saturated)
  float GetGain() const { return mGain; }

  void SetMasterVolume(float master); // 0.0 to 10.0
  float GetMasterVolume() const { return mMasterVolume; }

  // Tone Stack (0.0 to 10.0)
  void SetBass(float bass) { mBass = std::clamp(bass, 0.0f, 10.0f); }
  void SetMiddle(float mid) { mMiddle = std::clamp(mid, 0.0f, 10.0f); }
  void SetTreble(float treble) { mTreble = std::clamp(treble, 0.0f, 10.0f); }
  void SetPresence(float presence) { mPresence = std::clamp(presence, 0.0f, 10.0f); }

  // Cabinet IR Model
  void SetCabinetModel(JZCabinetModel cab);
  JZCabinetModel GetCabinetModel() const { return mCabinetModel; }

  // Real-Time Audio Processing
  float ProcessSample(float inSample);
  void Process(float& left, float& right);

private:
  float mSampleRate;
  JZAmpModel mAmpModel;
  JZCabinetModel mCabinetModel;

  float mGain;
  float mMasterVolume;
  float mBass;
  float mMiddle;
  float mTreble;
  float mPresence;

  // Filter states for Tone Stack & Cabinet simulation
  float mLowState;
  float mMidState;
  float mHighState;
  float mCabLpState;
  float mCabHpState;
  float mCabNotchState;

  float TubeTriodeSaturation(float in, float drive);
  float ApplyToneStack(float in);
  float ApplyCabinetSimulation(float in);
};
