//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Phase-Locked Transient Drum Replacer & Velocity Layer Trigger
//
// Studio drum replacement engine (Slate Trigger style) featuring:
// 1. Dynamic leakage rejection filtering (rejecting hi-hat / cymbal bleed)
// 2. Sub-sample phase-aligned transient triggering (locking sample attack to acoustic hit)
// 3. Multi-velocity sample layering (Soft, Medium, Hard, Rimshot dynamics)
// 4. Retrigger guard & dry/wet blending
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

enum class JZDrumHitType
{
  Soft = 0,
  Medium,
  Hard,
  Rimshot
};

struct JZTriggeredSampleEvent
{
  size_t sampleIndex;
  float velocity;             // 0.0 to 1.0
  JZDrumHitType hitType;
  float peakDb;

  JZTriggeredSampleEvent(size_t idx = 0, float vel = 1.0f, JZDrumHitType type = JZDrumHitType::Hard, float db = 0.0f)
    : sampleIndex(idx),
      velocity(vel),
      hitType(type),
      peakDb(db)
  {
  }
};

class JZDrumReplacerEngine
{
public:
  JZDrumReplacerEngine(float sampleRate = 44100.0f);
  ~JZDrumReplacerEngine() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  // Replacer Controls
  void SetThresholdDb(float thresholdDb);     // -40 dB to 0 dB
  float GetThresholdDb() const { return mThresholdDb; }

  void SetSensitivity(float sens);            // 0.0 to 1.0
  float GetSensitivity() const { return mSensitivity; }

  void SetRetriggerIntervalMs(float ms);      // 20ms to 200ms guard
  float GetRetriggerIntervalMs() const { return mRetriggerIntervalMs; }

  void SetLeakageRejection(float rejection);  // 0.0 (off) to 1.0 (strict hi-hat filter)
  float GetLeakageRejection() const { return mLeakageRejection; }

  void SetDryWetMix(float mixPercent);        // 0% to 100%
  float GetDryWetMix() const { return mDryWetMix; }

  // Load Replacement Sample Data
  void LoadReplacementSample(JZDrumHitType type, const std::vector<float>& pcmAudio);

  // Process & Trigger Drum Replacement
  bool ProcessAudio(
    const std::vector<float>& acousticDrumTrack,
    std::vector<float>& outReplacedAudio,
    std::vector<JZTriggeredSampleEvent>& outTriggerEvents);

private:
  float mSampleRate;
  float mThresholdDb;
  float mSensitivity;
  float mRetriggerIntervalMs;
  float mLeakageRejection;
  float mDryWetMix;

  // Sample buffers for 4 velocity layers
  std::vector<float> mSampleSoft;
  std::vector<float> mSampleMed;
  std::vector<float> mSampleHard;
  std::vector<float> mSampleRim;

  bool DetectPhaseLockedHit(
    const std::vector<float>& audio,
    size_t idx,
    float thresholdLinear,
    float& outVelocity,
    JZDrumHitType& outHitType);
};
