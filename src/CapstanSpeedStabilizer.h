//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Capstan Speed Stabilizer & Wow/Flutter Corrector (Capstan-Style)
//
// Forensic and archival audio restoration engine: analyzes continuous motor
// speed fluctuations (wow, flutter, pitch drift) from legacy tape and vinyl recordings,
// and applies inverse dynamic varispeed time-warping to lock pitch rock-solid.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

struct JZCapstanReport
{
  float detectedWowHz;        // Dominant wow modulation frequency
  float maxSpeedDriftPercent; // e.g. +/- 3.5%
  float speedCorrectionDb;
  bool  success;

  JZCapstanReport()
    : detectedWowHz(0.0f),
      maxSpeedDriftPercent(0.0f),
      speedCorrectionDb(0.0f),
      success(false)
  {
  }
};

class JZCapstanSpeedStabilizer
{
public:
  JZCapstanSpeedStabilizer(float sampleRate = 44100.0f);
  ~JZCapstanSpeedStabilizer() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  // Stabilizer Controls
  void SetStrength(float strengthPercent);    // 0% (bypass) to 100% (full correction)
  float GetStrength() const { return mStrength; }

  void SetReferenceTuningHz(float refHz);     // e.g. 440.0Hz or 50.0Hz mains hum
  float GetReferenceTuningHz() const { return mReferenceTuningHz; }

  // Process & Stabilize Audio Stream
  bool StabilizeAudio(
    const std::vector<float>& inAudio,
    std::vector<float>& outStabilizedAudio,
    JZCapstanReport& outReport);

private:
  float mSampleRate;
  float mStrength;
  float mReferenceTuningHz;

  float DetectInstantaneousPitch(const std::vector<float>& audio, size_t pos);
};
