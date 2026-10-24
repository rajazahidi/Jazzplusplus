//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Vintage Studio Microphone Modeler & Capsule Simulator
//
// Accurately models the acoustic frequency response, impedance loading,
// proximity effect, and non-linear capsule saturation of legendary studio microphones:
// 1. German U87 (Large-diaphragm studio condenser)
// 2. Vintage 251 / C12 (Silky tube condenser)
// 3. Broadcast 7B (Warm broadcast dynamic)
// 4. Ribbon R121 (Smooth vintage ribbon)
//*****************************************************************************

#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

enum class JZMicModel
{
  GermanU87 = 0,      // Versatile studio vocal condenser
  Vintage251,         // Silky airy high-end tube mic
  Broadcast7B,        // Thick mid-range dynamic broadcast
  RibbonR121          // Smooth natural ribbon microphone
};

enum class JZPolarPattern
{
  Cardioid = 0,
  Omni,
  Figure8
};

class JZMicrophoneModeler
{
public:
  JZMicrophoneModeler(float sampleRate = 44100.0f);
  ~JZMicrophoneModeler() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  // Mic Selection & Controls
  void SetMicModel(JZMicModel model) { mMicModel = model; }
  JZMicModel GetMicModel() const { return mMicModel; }

  void SetPolarPattern(JZPolarPattern pattern) { mPolarPattern = pattern; }
  JZPolarPattern GetPolarPattern() const { return mPolarPattern; }

  void SetProximityDistanceCm(float cm);      // 0cm (heavy bass boost) to 30cm (flat)
  float GetProximityDistanceCm() const { return mProximityDistanceCm; }

  void SetTubeDrive(float drive);             // 0.0 to 2.0 (subtle transformer/tube saturation)
  float GetTubeDrive() const { return mTubeDrive; }

  // Real-Time Audio Processing
  float ProcessSample(float inSample);
  void Process(float& left, float& right);

private:
  float mSampleRate;
  JZMicModel mMicModel;
  JZPolarPattern mPolarPattern;
  float mProximityDistanceCm;
  float mTubeDrive;

  // Filter States
  float mProximityLpL, mProximityLpR;
  float mAirFilterL, mAirFilterR;
  float mMidPresenceL, mMidPresenceR;

  float ApplyProximity(float in, float& state);
  float ApplyCapsuleEq(float in, float& airState, float& midState);
};
