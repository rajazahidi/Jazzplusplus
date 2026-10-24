//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Immersive 7.1.4 Spatial Bed & Height Object Audio Panner Implementation
//*****************************************************************************

#include "ImmersiveAudioPanner.h"
#include <cmath>
#include <algorithm>

JZImmersiveAudioPanner::JZImmersiveAudioPanner()
  : mAzimuthDeg(0.0f),
    mElevationDeg(0.0f),
    mDistanceMeters(1.5f),
    mLfeGain(0.1f)
{
}

void JZImmersiveAudioPanner::SetAzimuthDegrees(float azimuth)
{
  mAzimuthDeg = std::fmod(azimuth, 360.0f);
  if (mAzimuthDeg < 0.0f)
  {
    mAzimuthDeg += 360.0f;
  }
}

void JZImmersiveAudioPanner::SetElevationDegrees(float elevation)
{
  mElevationDeg = std::clamp(elevation, -30.0f, 90.0f);
}

void JZImmersiveAudioPanner::SetDistanceMeters(float meters)
{
  mDistanceMeters = std::clamp(meters, 0.5f, 20.0f);
}

void JZImmersiveAudioPanner::ComputeGains(float outGains[12]) const
{
  for (int i = 0; i < 12; ++i)
  {
    outGains[i] = 0.0f;
  }

  const float deg2rad = 3.14159265f / 180.0f;
  float azRad = mAzimuthDeg * deg2rad;
  float elRad = std::max(0.0f, mElevationDeg) * deg2rad;

  // 3D Cartesian coordinates
  float x = std::cos(elRad) * std::sin(azRad); // -1 (Left) to +1 (Right)
  float y = std::cos(elRad) * std::cos(azRad); // +1 (Front) to -1 (Rear)
  float z = std::sin(elRad);                   // 0 (Bed) to +1 (Ceiling)

  float heightWeight = z;
  float bedWeight = std::cos(elRad);

  // 1. Four Ceiling Height Channels
  if (heightWeight > 0.0001f)
  {
    float frontFrac = std::clamp((y + 1.0f) * 0.5f, 0.0f, 1.0f);
    float rearFrac = 1.0f - frontFrac;
    float rightFrac = std::clamp((x + 1.0f) * 0.5f, 0.0f, 1.0f);
    float leftFrac = 1.0f - rightFrac;

    outGains[CH_TOP_FRONT_LEFT]  = heightWeight * std::sqrt(frontFrac * leftFrac);
    outGains[CH_TOP_FRONT_RIGHT] = heightWeight * std::sqrt(frontFrac * rightFrac);
    outGains[CH_TOP_REAR_LEFT]   = heightWeight * std::sqrt(rearFrac * leftFrac);
    outGains[CH_TOP_REAR_RIGHT]  = heightWeight * std::sqrt(rearFrac * rightFrac);
  }

  // 2. Seven Ear-Level Bed Channels
  if (bedWeight > 0.0001f)
  {
    // Center channel focus (when in front +/- 30 degrees)
    float centerFocus = 0.0f;
    if (y > 0.707f && std::abs(x) < 0.5f)
    {
      centerFocus = (1.0f - std::abs(x) * 2.0f) * (y - 0.707f) * 3.41f;
      centerFocus = std::clamp(centerFocus, 0.0f, 1.0f);
    }
    outGains[CH_CENTER] = bedWeight * centerFocus * 0.9f;

    // Front Left & Front Right
    float remainingFront = 1.0f - centerFocus * 0.6f;
    if (y >= 0.0f)
    {
      outGains[CH_LEFT]  = bedWeight * remainingFront * std::clamp(-x, 0.0f, 1.0f) * y;
      outGains[CH_RIGHT] = bedWeight * remainingFront * std::clamp(x, 0.0f, 1.0f) * y;
    }

    // Side Surrounds (Lss, Rss)
    float sideWeight = 1.0f - std::abs(y);
    if (sideWeight > 0.0f)
    {
      outGains[CH_SIDE_LEFT]  = bedWeight * sideWeight * std::clamp(-x, 0.0f, 1.0f);
      outGains[CH_SIDE_RIGHT] = bedWeight * sideWeight * std::clamp(x, 0.0f, 1.0f);
    }

    // Rear Surrounds (Lsr, Rsr)
    if (y < 0.0f)
    {
      float rearAmount = -y;
      outGains[CH_REAR_LEFT]  = bedWeight * rearAmount * std::clamp(-x, 0.0f, 1.0f);
      outGains[CH_REAR_RIGHT] = bedWeight * rearAmount * std::clamp(x, 0.0f, 1.0f);
    }
  }

  // LFE Subwoofer Channel
  outGains[CH_LFE] = mLfeGain;

  // Inverse distance law attenuation
  float distAtten = 1.0f / std::sqrt(std::max(1.0f, mDistanceMeters));
  for (int i = 0; i < 12; ++i)
  {
    outGains[i] *= distAtten;
  }
}

void JZImmersiveAudioPanner::ProcessSample(float inSample, float out12Channels[12])
{
  float gains[12];
  ComputeGains(gains);

  for (int i = 0; i < 12; ++i)
  {
    out12Channels[i] = inSample * gains[i];
  }
}
