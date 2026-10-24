//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Spectral Audio De-Clipper & De-Clicker Restoration Engine
//
// Advanced digital audio repair suite featuring:
// 1. De-Clipper: Polynomial cubic spline reconstruction of clipped analog peaks
// 2. De-Clicker: Linear predictive transient filter to detect and eliminate
//    vinyl clicks, mouth clicks, and digital dropouts transparently.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

struct JZRestorationReport
{
  int clippedSamplesRepaired;
  int clicksDetectedAndRemoved;
  float peakRestorationDb;
  bool success;

  JZRestorationReport()
    : clippedSamplesRepaired(0),
      clicksDetectedAndRemoved(0),
      peakRestorationDb(0.0f),
      success(false)
  {
  }
};

class JZAudioRestorationEngine
{
public:
  JZAudioRestorationEngine(float sampleRate = 44100.0f);
  ~JZAudioRestorationEngine() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  // De-Clipper Controls
  void SetClipThresholdDb(float thresholdDb); // e.g. -0.2 dBFS
  float GetClipThresholdDb() const { return mClipThresholdDb; }

  void SetReconstructionDrive(float drive);   // 1.0 to 1.5 headroom
  float GetReconstructionDrive() const { return mReconstructionDrive; }

  // De-Clicker Controls
  void SetClickSensitivity(float sens);       // 0.0 to 1.0 (threshold sensitivity)
  float GetClickSensitivity() const { return mClickSensitivity; }

  // Processing Methods
  bool RepairAudio(
    const std::vector<float>& inAudio,
    std::vector<float>& outRepairedAudio,
    JZRestorationReport& outReport);

private:
  float mSampleRate;
  float mClipThresholdDb;
  float mReconstructionDrive;
  float mClickSensitivity;

  void ProcessDeClip(std::vector<float>& audio, int& repairedCount, float& maxPeakRestored);
  void ProcessDeClick(std::vector<float>& audio, int& clicksRemoved);
};
