//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Universal Parameter Modulation Matrix & Multi-Shape LFO Engine
//
// Modular modulation matrix featuring 8 independent multi-wave LFOs,
// Tempo-synced clock divisions, Envelope followers, and 1-to-many macro routing
// with bipolar depth (-100% to +100%) to modulate any synthesizer or FX parameter.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

enum class JZLfoShape
{
  Sine = 0,
  Triangle,
  SawUp,
  SawDown,
  Square,
  RandomSampleAndHold,
  EnvelopeFollower,
  ChaosWander
};

struct JZModulatorSlot
{
  int id;
  JZLfoShape shape;
  float rateHz;             // Free rate (0.05Hz to 40Hz)
  float syncDivision;       // BPM division factor (e.g. 0.25 = 1/16th, 1.0 = quarter note)
  bool  bpmSync;
  float phase;              // Current phase 0.0 to 1.0
  float currentOutput;      // -1.0 to +1.0
  float lastRandomVal;

  JZModulatorSlot()
    : id(0),
      shape(JZLfoShape::Sine),
      rateHz(1.0f),
      syncDivision(1.0f),
      bpmSync(false),
      phase(0.0f),
      currentOutput(0.0f),
      lastRandomVal(0.0f)
  {
  }
};

struct JZModulationRoute
{
  int sourceModulatorId;    // 0 to 7
  int targetParamId;        // Parameter identifier
  float bipolarDepth;       // -1.0 to +1.0

  JZModulationRoute(int src = 0, int tgt = 0, float depth = 1.0f)
    : sourceModulatorId(src),
      targetParamId(tgt),
      bipolarDepth(depth)
  {
  }
};

class JZModulationMatrix
{
public:
  JZModulationMatrix(float sampleRate = 44100.0f);
  ~JZModulationMatrix() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  void SetProjectBpm(float bpm);
  float GetProjectBpm() const { return mProjectBpm; }

  // Modulator Slot Configuration
  void ConfigureSlot(int slotIdx, JZLfoShape shape, float rateHz, bool bpmSync, float syncDivision);
  JZModulatorSlot& GetSlot(int slotIdx);

  // Routing Management
  void AddRoute(int sourceModId, int targetParamId, float depth);
  void ClearRoutes();
  const std::vector<JZModulationRoute>& GetRoutes() const { return mRoutes; }

  // Advance time by 1 sample or block of samples
  void AdvanceSample();
  void FeedAudioEnvelope(float audioSample);

  // Query modulated value
  float GetModulatedValue(int targetParamId, float baseNormalizedValue) const;

private:
  float mSampleRate;
  float mProjectBpm;
  std::vector<JZModulatorSlot> mSlots;
  std::vector<JZModulationRoute> mRoutes;
  float mEnvelopeFollowerState;
};
