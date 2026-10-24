//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Vintage Studio Microphone Modeler & Capsule Simulator Implementation
//*****************************************************************************

#include "MicrophoneModeler.h"
#include <cmath>
#include <algorithm>

JZMicrophoneModeler::JZMicrophoneModeler(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f),
    mMicModel(JZMicModel::GermanU87),
    mPolarPattern(JZPolarPattern::Cardioid),
    mProximityDistanceCm(8.0f),
    mTubeDrive(0.3f),
    mProximityLpL(0.0f), mProximityLpR(0.0f),
    mAirFilterL(0.0f), mAirFilterR(0.0f),
    mMidPresenceL(0.0f), mMidPresenceR(0.0f)
{
}

void JZMicrophoneModeler::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
  }
}

void JZMicrophoneModeler::SetProximityDistanceCm(float cm)
{
  mProximityDistanceCm = std::clamp(cm, 0.0f, 30.0f);
}

void JZMicrophoneModeler::SetTubeDrive(float drive)
{
  mTubeDrive = std::clamp(drive, 0.0f, 2.0f);
}

float JZMicrophoneModeler::ApplyProximity(float in, float& state)
{
  if (mPolarPattern == JZPolarPattern::Omni || mProximityDistanceCm >= 28.0f)
  {
    return in; // Pressure omni mics have no proximity effect
  }

  // Low shelf boost below 150Hz
  float proxGain = (1.0f - (mProximityDistanceCm / 30.0f)) * 1.5f; // Up to +6dB at 0cm
  float alpha = std::exp(-2.0f * 3.14159265f * 150.0f / mSampleRate);
  state = alpha * state + (1.0f - alpha) * in;

  return in + state * proxGain;
}

float JZMicrophoneModeler::ApplyCapsuleEq(float in, float& airState, float& midState)
{
  float airCutoff = 10000.0f;
  float midFreq = 3500.0f;

  float airAlpha = std::exp(-2.0f * 3.14159265f * airCutoff / mSampleRate);
  float midAlpha = std::exp(-2.0f * 3.14159265f * midFreq / mSampleRate);

  airState = airAlpha * airState + (1.0f - airAlpha) * in;
  midState = midAlpha * midState + (1.0f - midAlpha) * in;

  float highAir = in - airState;
  float midBand = airState - midState;

  float out = in;

  switch (mMicModel)
  {
    case JZMicModel::GermanU87:
      // Clear 3.5kHz presence peak, balanced air
      out = in + midBand * 0.40f + highAir * 0.15f;
      break;

    case JZMicModel::Vintage251:
      // Silky, open 12kHz+ tube air sparkle
      out = in + highAir * 0.55f - midBand * 0.10f;
      break;

    case JZMicModel::Broadcast7B:
      // Thick, controlled dynamic mid, subdued sibilance
      out = in + midBand * 0.35f - highAir * 0.30f;
      break;

    case JZMicModel::RibbonR121:
      // Gentle roll-off above 10kHz, smooth dark warmth
      out = in * 0.85f + midState * 0.25f - highAir * 0.45f;
      break;
  }

  return out;
}

float JZMicrophoneModeler::ProcessSample(float inSample)
{
  float prox = ApplyProximity(inSample, mProximityLpL);
  float eq = ApplyCapsuleEq(prox, mAirFilterL, mMidPresenceL);

  // Subtle analog transformer / tube saturation
  if (mTubeDrive > 0.001f)
  {
    float driveFactor = 1.0f + mTubeDrive * 0.4f;
    eq = std::tanh(eq * driveFactor) / driveFactor;
  }

  return std::clamp(eq, -1.0f, 1.0f);
}

void JZMicrophoneModeler::Process(float& left, float& right)
{
  float proxL = ApplyProximity(left, mProximityLpL);
  float proxR = ApplyProximity(right, mProximityLpR);

  float eqL = ApplyCapsuleEq(proxL, mAirFilterL, mMidPresenceL);
  float eqR = ApplyCapsuleEq(proxR, mAirFilterR, mMidPresenceR);

  if (mTubeDrive > 0.001f)
  {
    float driveFactor = 1.0f + mTubeDrive * 0.4f;
    eqL = std::tanh(eqL * driveFactor) / driveFactor;
    eqR = std::tanh(eqR * driveFactor) / driveFactor;
  }

  left = std::clamp(eqL, -1.0f, 1.0f);
  right = std::clamp(eqR, -1.0f, 1.0f);
}
