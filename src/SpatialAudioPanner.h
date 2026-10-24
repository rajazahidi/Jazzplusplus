//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// 3D Spatial Audio & Surround Sound Panner
//
// 3D acoustic positioning engine: Azimuth (0-360 deg), Elevation (-90 to +90 deg),
// Distance (0.1m - 30m), Interaural Time Difference (ITD), Binaural 3D Headphone
// rendering, Quadraphonic 4-ch, and 5.1 Surround Sound bus panning.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

// Spatial Panning Mode
enum JZSpatialPannerMode
{
  SPATIAL_BINAURAL_3D = 0,    // 2-channel 3D immersive headphone render
  SPATIAL_QUADRAPHONIC,       // 4-channel surround (FL, FR, RL, RR)
  SPATIAL_SURROUND_5_1        // 6-channel surround (L, C, R, LFE, Ls, Rs)
};

// 3D Coordinate Position
struct JZSpatialPosition
{
  float azimuthDeg;           // 0 deg = Center Front, 90 deg = Right, 180 deg = Behind, 270 deg = Left
  float elevationDeg;         // -90 deg (Below) to +90 deg (Above)
  float distanceMeters;       // 0.2m to 30.0m
  bool airAbsorption;         // Distance-based high-frequency attenuation

  JZSpatialPosition(float az = 0.0f, float el = 0.0f, float dist = 1.5f)
    : azimuthDeg(az),
      elevationDeg(el),
      distanceMeters(dist),
      airAbsorption(true)
  {
  }
};

// 5.1 Surround output channel buffers
struct JZSurround51Buffer
{
  std::vector<float> L;       // Front Left
  std::vector<float> C;       // Center
  std::vector<float> R;       // Front Right
  std::vector<float> LFE;     // Low Frequency Effects (Subwoofer)
  std::vector<float> Ls;      // Surround Left
  std::vector<float> Rs;      // Surround Right

  void Resize(int size)
  {
    L.assign(size, 0.0f);
    C.assign(size, 0.0f);
    R.assign(size, 0.0f);
    LFE.assign(size, 0.0f);
    Ls.assign(size, 0.0f);
    Rs.assign(size, 0.0f);
  }
};

// 3D Spatial Audio Panner Class
class JZSpatialAudioPanner
{
public:
  JZSpatialAudioPanner(float sampleRate = 44100.0f);
  ~JZSpatialAudioPanner() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  void SetMode(JZSpatialPannerMode mode) { mMode = mode; }
  JZSpatialPannerMode GetMode() const { return mMode; }

  void SetPosition(const JZSpatialPosition& pos);
  const JZSpatialPosition& GetPosition() const { return mPos; }

  void Reset();

  // Process mono input to Binaural 3D Stereo output
  void ProcessBinaural(const float* inMono, float* outL, float* outR, int numSamples);

  // Process mono input to 5.1 Surround output
  void ProcessSurround51(const float* inMono, JZSurround51Buffer& outSurround, int numSamples);

private:
  float mSampleRate;
  JZSpatialPannerMode mMode;
  JZSpatialPosition mPos;

  // ITD fractional delay line
  std::vector<float> mDelayLineL;
  std::vector<float> mDelayLineR;
  int mDelayWritePos;

  // Head shadow 1-pole filter states
  float mShadowStateL;
  float mShadowStateR;

  // Distance low-pass air absorption
  float mAirStateL;
  float mAirStateR;
};
