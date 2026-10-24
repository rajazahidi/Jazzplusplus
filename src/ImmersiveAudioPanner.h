//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Immersive 7.1.4 Spatial Bed & Height Object Audio Panner (Dolby Atmos Compliant)
//
// Full 3D spatial object panner generating 12 discrete surround & ceiling channels:
// Ear-level bed: L, C, R, LFE, Lss (Side L), Rss (Side R), Lsr (Rear L), Rsr (Rear R)
// Ceiling height: TFL (Top Front L), TFR (Top Front R), TRL (Top Rear L), TRR (Top Rear R)
//*****************************************************************************

#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

enum JZImmersiveChannel
{
  CH_LEFT = 0,
  CH_CENTER,
  CH_RIGHT,
  CH_LFE,
  CH_SIDE_LEFT,
  CH_SIDE_RIGHT,
  CH_REAR_LEFT,
  CH_REAR_RIGHT,
  CH_TOP_FRONT_LEFT,
  CH_TOP_FRONT_RIGHT,
  CH_TOP_REAR_LEFT,
  CH_TOP_REAR_RIGHT,
  CH_TOTAL_COUNT = 12
};

class JZImmersiveAudioPanner
{
public:
  JZImmersiveAudioPanner();
  ~JZImmersiveAudioPanner() = default;

  // 3D Object Coordinates
  void SetAzimuthDegrees(float azimuth);      // 0 = Front Center, 90 = Right, 180 = Rear, 270 = Left
  float GetAzimuthDegrees() const { return mAzimuthDeg; }

  void SetElevationDegrees(float elevation);  // 0 = Ear-level, +90 = Zenith Ceiling
  float GetElevationDegrees() const { return mElevationDeg; }

  void SetDistanceMeters(float meters);       // 0.5m to 20m
  float GetDistanceMeters() const { return mDistanceMeters; }

  void SetLfeSend(float lfeGain) { mLfeGain = std::clamp(lfeGain, 0.0f, 1.0f); }
  float GetLfeSend() const { return mLfeGain; }

  // Compute 12-channel spatial gains and process audio sample
  void ComputeGains(float outGains[12]) const;
  void ProcessSample(float inSample, float out12Channels[12]);

private:
  float mAzimuthDeg;
  float mElevationDeg;
  float mDistanceMeters;
  float mLfeGain;
};
