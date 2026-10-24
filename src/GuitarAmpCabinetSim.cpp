//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Analog Guitar Tube Amp & Speaker Cabinet IR Simulator Implementation
//*****************************************************************************

#include "GuitarAmpCabinetSim.h"
#include <cmath>
#include <algorithm>

JZGuitarAmpCabinetSim::JZGuitarAmpCabinetSim(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f),
    mAmpModel(JZAmpModel::BritishCrunch),
    mCabinetModel(JZCabinetModel::Cab4x12HighGain),
    mGain(5.0f),
    mMasterVolume(7.0f),
    mBass(5.0f),
    mMiddle(5.0f),
    mTreble(6.0f),
    mPresence(5.0f),
    mLowState(0.0f),
    mMidState(0.0f),
    mHighState(0.0f),
    mCabLpState(0.0f),
    mCabHpState(0.0f),
    mCabNotchState(0.0f)
{
}

void JZGuitarAmpCabinetSim::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
  }
}

void JZGuitarAmpCabinetSim::SetAmpModel(JZAmpModel model)
{
  mAmpModel = model;
}

void JZGuitarAmpCabinetSim::SetGain(float gain)
{
  mGain = std::clamp(gain, 0.0f, 10.0f);
}

void JZGuitarAmpCabinetSim::SetMasterVolume(float master)
{
  mMasterVolume = std::clamp(master, 0.0f, 10.0f);
}

void JZGuitarAmpCabinetSim::SetCabinetModel(JZCabinetModel cab)
{
  mCabinetModel = cab;
}

float JZGuitarAmpCabinetSim::TubeTriodeSaturation(float in, float drive)
{
  float x = in * drive;
  // Asymmetric DC bias injection creates authentic 2nd harmonic warmth
  float biasedX = x + 0.14f * (x * x);

  // Soft saturation tanh curve
  float saturated = std::tanh(biasedX);

  // Soft power-stage compression
  if (std::abs(saturated) > 0.8f)
  {
    float excess = std::abs(saturated) - 0.8f;
    float sign = saturated >= 0.0f ? 1.0f : -1.0f;
    saturated = sign * (0.8f + excess * 0.4f);
  }

  return saturated;
}

float JZGuitarAmpCabinetSim::ApplyToneStack(float in)
{
  // Simple interactive tone stack approximation
  float bassGain = 0.4f + (mBass / 10.0f) * 1.2f;
  float midGain = 0.3f + (mMiddle / 10.0f) * 1.0f;
  float trebleGain = 0.3f + (mTreble / 10.0f) * 1.4f;

  float lpBassCoeff = std::exp(-2.0f * 3.14159265f * 200.0f / mSampleRate);
  float lpTrebleCoeff = std::exp(-2.0f * 3.14159265f * 2500.0f / mSampleRate);

  mLowState = lpBassCoeff * mLowState + (1.0f - lpBassCoeff) * in;
  mHighState = lpTrebleCoeff * mHighState + (1.0f - lpTrebleCoeff) * in;
  float midBand = in - (mLowState + (in - mHighState));

  float out = mLowState * bassGain + midBand * midGain + (in - mHighState) * trebleGain;
  return out;
}

float JZGuitarAmpCabinetSim::ApplyCabinetSimulation(float in)
{
  if (mCabinetModel == JZCabinetModel::Bypass)
  {
    return in;
  }

  // Guitar speakers roll off sharply above ~5kHz and below ~80Hz
  float cutoffLp = 4800.0f;
  float cutoffHp = 85.0f;

  if (mCabinetModel == JZCabinetModel::Cab1x12Vintage)
  {
    cutoffLp = 4200.0f;
    cutoffHp = 100.0f;
  }
  else if (mCabinetModel == JZCabinetModel::Cab4x12HighGain)
  {
    cutoffLp = 5400.0f;
    cutoffHp = 75.0f;
  }

  float lpCoeff = std::exp(-2.0f * 3.14159265f * cutoffLp / mSampleRate);
  float hpCoeff = std::exp(-2.0f * 3.14159265f * cutoffHp / mSampleRate);

  mCabLpState = lpCoeff * mCabLpState + (1.0f - lpCoeff) * in;
  mCabHpState = hpCoeff * mCabHpState + (1.0f - hpCoeff) * mCabLpState;
  float cabOut = mCabLpState - mCabHpState;

  // Add 3kHz speaker cone resonance bite
  mCabNotchState = 0.85f * mCabNotchState + 0.15f * cabOut;
  return cabOut + 0.2f * mCabNotchState;
}

float JZGuitarAmpCabinetSim::ProcessSample(float inSample)
{
  // Drive factor depending on amp model
  float driveBase = 1.0f;
  switch (mAmpModel)
  {
    case JZAmpModel::AmericanClean:
      driveBase = 1.2f + (mGain / 10.0f) * 2.5f;
      break;
    case JZAmpModel::BritishCrunch:
      driveBase = 2.5f + (mGain / 10.0f) * 6.0f;
      break;
    case JZAmpModel::ModernHighGain:
      driveBase = 4.0f + (mGain / 10.0f) * 12.0f;
      break;
  }

  float saturated = TubeTriodeSaturation(inSample, driveBase);
  float toned = ApplyToneStack(saturated);
  float cabToned = ApplyCabinetSimulation(toned);

  float masterGain = (mMasterVolume / 10.0f) * 0.7f;
  return std::clamp(cabToned * masterGain, -1.0f, 1.0f);
}

void JZGuitarAmpCabinetSim::Process(float& left, float& right)
{
  left = ProcessSample(left);
  right = ProcessSample(right);
}
