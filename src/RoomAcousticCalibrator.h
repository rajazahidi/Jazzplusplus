//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Room Acoustic Mode Calibrator & Monitor Correction
//
// Studio monitor acoustic calibration suite (Sonarworks SoundID style):
// 1. Logarithmic sine chirp test sweep generation (20Hz to 20kHz)
// 2. Room impulse response measurement & standing wave resonance detection
// 3. Automated inverse minimum-phase correction filter for flat reference monitoring.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

struct JZRoomModePeak
{
  float frequencyHz;
  float deviationDb;          // Measured room deviation (e.g. +8.5 dB resonant room mode)
  float correctionGainDb;     // Inverse correction (e.g. -8.5 dB)

  JZRoomModePeak(float f = 100.0f, float dev = 0.0f)
    : frequencyHz(f),
      deviationDb(dev),
      correctionGainDb(-dev)
  {
  }
};

class JZRoomAcousticCalibrator
{
public:
  JZRoomAcousticCalibrator(float sampleRate = 44100.0f);
  ~JZRoomAcousticCalibrator() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  // Test Sweep Generator
  void GenerateChirpSweep(std::vector<float>& outChirpSweep, float durationSec = 3.0f);

  // Impulse Response Analysis & Inverse EQ Generation
  bool AnalyzeRoomResponse(const std::vector<float>& recordedSweepAudio);

  // Calibration Controls
  void SetEnabled(bool enable) { mEnabled = enable; }
  bool IsEnabled() const { return mEnabled; }

  void SetCorrectionAmount(float percent);    // 0% (bypass) to 100% (flat reference)
  float GetCorrectionAmount() const { return mCorrectionAmount; }

  void SetMaxBoostLimitDb(float maxDb);       // e.g. 6.0 to 12.0 dB max boost safety guard
  float GetMaxBoostLimitDb() const { return mMaxBoostLimitDb; }

  const std::vector<JZRoomModePeak>& GetDetectedModes() const { return mRoomModes; }

  // Real-time Master Monitor Processing
  void Process(float& left, float& right);

private:
  float mSampleRate;
  bool  mEnabled;
  float mCorrectionAmount;
  float mMaxBoostLimitDb;

  std::vector<JZRoomModePeak> mRoomModes;
  std::vector<float> mFilterStatesL;
  std::vector<float> mFilterStatesR;

  void InitRoomModes();
};
