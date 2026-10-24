//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Real-Time Audio Time-Stretching & Pitch-Shifting Engine
//
// WSOLA (Waveform Similarity Overlap-Add) High-Fidelity Time-Stretch,
// Granular Pitch Shifting, and Automatic BPM Tempo Warping.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

// Time-Stretch Quality Mode
enum JZTimeStretchMode
{
  STRETCH_MODE_WSOLA_HIGH_QUALITY = 0, // High-quality WSOLA overlap-add (rhythmic & melodic)
  STRETCH_MODE_SOLA_FAST,              // Fast SOLA for preview
  STRETCH_MODE_GRANULAR_TEXTURE        // Granular windowing for ambient / sound design
};

// Audio Time-Stretch & Pitch-Shift Configuration
struct JZTimeStretchConfig
{
  double timeRatio;           // 1.0 = normal, 0.5 = 2x faster, 2.0 = 2x slower (0.25 to 4.0)
  double pitchShiftSemitones; // -12.0 to +12.0 semitones
  double sourceBpm;           // Original loop BPM (e.g. 120.0)
  double targetBpm;           // Project destination BPM (e.g. 135.0)
  bool autoBpmWarp;           // When true, calculates timeRatio from targetBpm / sourceBpm
  JZTimeStretchMode mode;

  JZTimeStretchConfig()
    : timeRatio(1.0),
      pitchShiftSemitones(0.0),
      sourceBpm(120.0),
      targetBpm(120.0),
      autoBpmWarp(false),
      mode(STRETCH_MODE_WSOLA_HIGH_QUALITY)
  {
  }

  double GetEffectiveStretchRatio() const
  {
    if (autoBpmWarp && sourceBpm > 10.0 && targetBpm > 10.0)
    {
      return targetBpm / sourceBpm;
    }
    return std::max(0.25, std::min(4.0, timeRatio));
  }
};

class JZAudioTimeStretcher
{
  public:

    JZAudioTimeStretcher();
    ~JZAudioTimeStretcher();

    // Process mono or stereo audio buffer with WSOLA time-stretch
    // Returns number of output frames generated
    static int ProcessWSOLA(
      const float* pInput,
      int inputFrames,
      float* pOutput,
      int maxOutputFrames,
      double stretchRatio,
      int channels = 1,
      int sampleRate = 44100);

    // Process independent pitch-shift in semitones (-12 to +12)
    static int ProcessPitchShift(
      const float* pInput,
      int inputFrames,
      float* pOutput,
      int maxOutputFrames,
      double semitones,
      int channels = 1,
      int sampleRate = 44100);

    // Auto-warp an entire audio buffer from source BPM to target BPM
    static bool WarpAudioBpm(
      const std::vector<float>& inputBuffer,
      std::vector<float>& outputBuffer,
      double sourceBpm,
      double targetBpm,
      int channels = 2,
      int sampleRate = 44100);

  private:

    static float ComputeCrossCorrelation(
      const float* pRef,
      const float* pCandidate,
      int length,
      int channels);
};
