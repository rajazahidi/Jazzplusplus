//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Room Acoustic Mode Calibrator & Monitor Correction Implementation
//*****************************************************************************

#include "RoomAcousticCalibrator.h"
#include <cmath>
#include <algorithm>

JZRoomAcousticCalibrator::JZRoomAcousticCalibrator(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f),
    mEnabled(true),
    mCorrectionAmount(100.0f),
    mMaxBoostLimitDb(9.0f)
{
  InitRoomModes();
  mFilterStatesL.assign(16, 0.0f);
  mFilterStatesR.assign(16, 0.0f);
}

void JZRoomAcousticCalibrator::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
    InitRoomModes();
  }
}

void JZRoomAcousticCalibrator::InitRoomModes()
{
  mRoomModes.clear();
  mRoomModes.reserve(16);

  // Common room resonant problem frequencies (modes and boundary cancellations)
  static const float defaultFreqs[16] = {
    42.0f, 65.0f, 85.0f, 115.0f, 145.0f, 180.0f, 220.0f, 280.0f,
    350.0f, 500.0f, 800.0f, 1200.0f, 2500.0f, 4500.0f, 8000.0f, 12000.0f
  };
  // Typical uncalibrated room anomalies (+7dB 65Hz boom, -5dB 115Hz dip)
  static const float defaultDeviations[16] = {
    3.0f, 7.5f, 4.0f, -5.5f, -2.0f, 4.5f, 2.0f, -3.0f,
    1.5f, -1.0f, 0.5f, 1.2f, -1.5f, 2.0f, -0.5f, 1.0f
  };

  for (int i = 0; i < 16; ++i)
  {
    float dev = defaultDeviations[i];
    mRoomModes.emplace_back(defaultFreqs[i], dev);
    mRoomModes.back().correctionGainDb = -dev;
  }
}

void JZRoomAcousticCalibrator::SetCorrectionAmount(float percent)
{
  mCorrectionAmount = std::clamp(percent, 0.0f, 100.0f);
}

void JZRoomAcousticCalibrator::SetMaxBoostLimitDb(float maxDb)
{
  mMaxBoostLimitDb = std::clamp(maxDb, 2.0f, 18.0f);
}

void JZRoomAcousticCalibrator::GenerateChirpSweep(
  std::vector<float>& outChirpSweep,
  float durationSec)
{
  size_t totalSamples = static_cast<size_t>(durationSec * mSampleRate);
  outChirpSweep.resize(totalSamples, 0.0f);

  float f0 = 20.0f;
  float f1 = 20000.0f;
  float T = durationSec;

  for (size_t i = 0; i < totalSamples; ++i)
  {
    float t = static_cast<float>(i) / mSampleRate;
    // Logarithmic chirp equation
    float phase = 2.0f * 3.14159265f * f0 * (std::pow(f1 / f0, t / T) - 1.0f) / std::log(f1 / f0);
    // Envelope fade in and fade out
    float env = 1.0f;
    if (t < 0.05f) env = t / 0.05f;
    else if (t > T - 0.05f) env = (T - t) / 0.05f;

    outChirpSweep[i] = std::sin(phase) * env * 0.7f;
  }
}

bool JZRoomAcousticCalibrator::AnalyzeRoomResponse(const std::vector<float>& recordedSweepAudio)
{
  if (recordedSweepAudio.size() < static_cast<size_t>(mSampleRate * 1.0f))
  {
    return false;
  }

  // Energy distribution estimation across room mode frequency zones
  for (size_t m = 0; m < mRoomModes.size(); ++m)
  {
    float f = mRoomModes[m].frequencyHz;
    float alpha = std::exp(-2.0f * 3.14159265f * f / mSampleRate);
    float state = 0.0f;
    float energySum = 0.0f;

    for (size_t s = 0; s < recordedSweepAudio.size(); s += 4)
    {
      float val = recordedSweepAudio[s];
      state = alpha * state + (1.0f - alpha) * val;
      energySum += state * state;
    }

    float normRms = std::sqrt(energySum / (recordedSweepAudio.size() * 0.25f));
    float measuredDb = 20.0f * std::log10(std::max(0.0001f, normRms));
    float expectedFlatDb = -18.0f; // Target calibration baseline

    float deviation = std::clamp(measuredDb - expectedFlatDb, -15.0f, 15.0f);
    mRoomModes[m].deviationDb = deviation;

    // Inverse compensation calculation with safety guard limit
    float invDb = -deviation;
    if (invDb > mMaxBoostLimitDb) invDb = mMaxBoostLimitDb; // Don't overdrive speakers
    mRoomModes[m].correctionGainDb = invDb;
  }

  return true;
}

void JZRoomAcousticCalibrator::Process(float& left, float& right)
{
  if (!mEnabled || mCorrectionAmount <= 0.001f)
  {
    return;
  }

  float factor = mCorrectionAmount / 100.0f;

  // Apply critical bass room mode correction (e.g. 65Hz boom suppression)
  float mode1CorrDb = mRoomModes[1].correctionGainDb * factor; // 65Hz mode
  float mode3CorrDb = mRoomModes[3].correctionGainDb * factor; // 115Hz dip

  float g1 = std::pow(10.0f, mode1CorrDb / 30.0f);
  float g3 = std::pow(10.0f, mode3CorrDb / 30.0f);

  float netBassCorrectionGain = 0.5f * (g1 + g3);
  left = std::clamp(left * netBassCorrectionGain, -1.0f, 1.0f);
  right = std::clamp(right * netBassCorrectionGain, -1.0f, 1.0f);
}
