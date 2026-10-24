//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Audio Flex-Time Warping & Multi-Marker Transient Quantizer
//
// Non-destructive elastic audio warping engine with movable anchor pins,
// automatic transient detection, grid rhythm quantization (1/8, 1/16, 1/32),
// and piecewise phase-preserving audio time mapping.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

// Audio Warp Marker Pin
struct JZWarpMarker
{
  int originalSample;         // Original transient position in source audio
  int warpedSample;           // New aligned position on project grid
  bool pinned;                // Fixed anchor marker

  JZWarpMarker(int orig = 0, int warped = 0, bool isPinned = false)
    : originalSample(orig),
      warpedSample(warped),
      pinned(isPinned)
  {
  }
};

// Audio Flex-Time Warping Class
class JZAudioFlexTimeWarp
{
public:
  JZAudioFlexTimeWarp(float sampleRate = 44100.0f);
  ~JZAudioFlexTimeWarp() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  // Marker Management
  void AddWarpMarker(int originalSample, int warpedSample, bool pinned = false);
  void ClearWarpMarkers();
  int GetMarkerCount() const { return (int)mMarkers.size(); }
  const std::vector<JZWarpMarker>& GetMarkers() const { return mMarkers; }

  // 1-Click Audio Quantize: Auto-detects audio transients and creates warp markers to snap to tempo grid
  int AutoQuantizeTransients(
    const std::vector<float>& audioBuffer,
    float bpm,
    int gridSubdivision = 16,
    float quantizeStrength = 1.0f);

  // Process and warp stereo audio buffers according to current warp markers
  bool ProcessWarp(
    const std::vector<float>& inL,
    const std::vector<float>& inR,
    std::vector<float>& outL,
    std::vector<float>& outR);

  // Map warped output sample position to original input sample position
  double MapOutputToInputSample(int outputSample, int maxInputSample) const;

private:
  float mSampleRate;
  std::vector<JZWarpMarker> mMarkers;

  void SortAndDeduplicateMarkers();
};
