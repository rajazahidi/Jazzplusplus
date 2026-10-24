//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Vintage Lo-Fi Cassette Degradation & Pitch Wobble Modeler
//
// Retro cassette tape emulation featuring:
// 1. Capstan motor wow & fast flutter pitch modulation
// 2. Misaligned tape head azimuth high-end roll-off
// 3. Analog cassette tape hiss generation (Type I Ferric / Type II Chrome)
// 4. Random magnetic tape oxide dropouts
//*****************************************************************************

#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

enum class JZCassetteType
{
  TypeIFerric = 0,    // Warm, gritty, moderate hiss
  TypeIIChrome        // Brighter, refined, lower noise floor
};

class JZVintageLofiProcessor
{
public:
  JZVintageLofiProcessor(float sampleRate = 44100.0f);
  ~JZVintageLofiProcessor() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  // Lo-Fi Controls
  void SetWowDepth(float depth);        // 0.0 to 1.0 (slow pitch warble)
  float GetWowDepth() const { return mWowDepth; }

  void SetFlutterDepth(float depth);    // 0.0 to 1.0 (fast flutter)
  float GetFlutterDepth() const { return mFlutterDepth; }

  void SetCassetteType(JZCassetteType type) { mCassetteType = type; }
  JZCassetteType GetCassetteType() const { return mCassetteType; }

  void SetHissLevelDb(float hissDb);    // -80 dB to -30 dB
  float GetHissLevelDb() const { return mHissLevelDb; }

  void SetAzimuthLoss(float loss);      // 0.0 (crisp) to 1.0 (muffled dark)
  float GetAzimuthLoss() const { return mAzimuthLoss; }

  void SetDropoutsEnabled(bool enable) { mDropoutsEnabled = enable; }
  bool IsDropoutsEnabled() const { return mDropoutsEnabled; }

  // Stereo Real-Time Processing
  void Process(float& left, float& right);

private:
  float mSampleRate;
  float mWowDepth;
  float mFlutterDepth;
  JZCassetteType mCassetteType;
  float mHissLevelDb;
  float mAzimuthLoss;
  bool  mDropoutsEnabled;

  // LFO Phases
  float mWowPhase;
  float mFlutterPhase;

  // Fractional Delay Ring Buffers for Pitch Warble
  std::vector<float> mDelayBufL;
  std::vector<float> mDelayBufR;
  size_t mWriteIdx;

  // Filter States
  float mAzimuthLpL;
  float mAzimuthLpR;
  float mDropoutGain;

  float GenerateHissSample();
};
