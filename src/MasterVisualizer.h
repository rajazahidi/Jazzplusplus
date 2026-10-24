//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Master Spectrum Analyzer & Stereo Goniometer Visualizer
//
// Real-time 31-band 1/3-octave frequency spectrum analyzer,
// stereo goniometer / Lissajous vector scope, and stereo phase correlation meter.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

// 1/3 Octave ISO Frequency Band
struct JZSpectrumBand
{
  float centerFreqHz;
  std::string label;
  float magnitudeDB;          // Current level (-96.0 dB to +12.0 dB)
  float peakHoldDB;           // Peak hold level
  float peakDecayTimer;

  JZSpectrumBand(float freq = 1000.0f, const std::string& lbl = "1k")
    : centerFreqHz(freq),
      label(lbl),
      magnitudeDB(-96.0f),
      peakHoldDB(-96.0f),
      peakDecayTimer(0.0f)
  {
  }
};

// Goniometer Lissajous Coordinate Point
struct JZGoniometerPoint
{
  float x;                    // Side component: (L - R) / sqrt(2)
  float y;                    // Mid component: (L + R) / sqrt(2)

  JZGoniometerPoint(float xVal = 0.0f, float yVal = 0.0f)
    : x(xVal), y(yVal)
  {
  }
};

// Master Audio Diagnostics Metrics
struct JZMasterMetrics
{
  float phaseCorrelation;     // -1.0 (inverted phase / cancellation) to +1.0 (in-phase mono)
  float rmsLeftDB;
  float rmsRightDB;
  float peakLeftDB;
  float peakRightDB;
  float stereoSpreadPercent;  // 0% (pure mono) to 100% (standard stereo) to 200% (super wide)

  JZMasterMetrics()
    : phaseCorrelation(1.0f),
      rmsLeftDB(-96.0f),
      rmsRightDB(-96.0f),
      peakLeftDB(-96.0f),
      peakRightDB(-96.0f),
      stereoSpreadPercent(100.0f)
  {
  }
};

// Master Visualizer Engine Class
class JZMasterVisualizer
{
public:
  JZMasterVisualizer(float sampleRate = 44100.0f);
  ~JZMasterVisualizer() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  // Analyze audio block (stereo or mono)
  void ProcessBlock(const float* left, const float* right, int numSamples);

  // Results access
  const std::vector<JZSpectrumBand>& GetSpectrumBands() const { return mBands; }
  std::vector<JZGoniometerPoint> GetGoniometerPoints(int maxPoints = 128) const;
  const JZMasterMetrics& GetMetrics() const { return mMetrics; }

  // Controls
  void ResetPeaks();
  void SetReleaseSpeed(float decaySpeedDBPerSec);

private:
  float mSampleRate;
  float mDecaySpeedDBPerSec;
  std::vector<JZSpectrumBand> mBands;
  std::vector<JZGoniometerPoint> mRecentPoints;
  JZMasterMetrics mMetrics;

  void InitializeBands();
  void EstimateBandMagnitudes(const float* mono, int numSamples);
};
