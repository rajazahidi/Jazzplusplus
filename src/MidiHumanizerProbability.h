//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// MIDI Humanizer, Note Trigger Probability & Generative Chance Engine
//
// Natural performance humanization engine applying Gaussian micro-timing deviations,
// dynamic velocity jitter, generative note trigger probabilities (0% to 100%),
// and automated ghost-note injection.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <random>
#include <cmath>
#include <algorithm>
#include "ClipMatrixEngine.h" // For JZMatrixMidiNote

// Configuration for MIDI Humanizer
struct JZMidiHumanizerConfig
{
  float timingJitterMs;        // Gaussian timing deviation (0.0 ms to 20.0 ms, default 6.0 ms)
  int velocityJitter;         // Velocity random spread (0 to 25, default 8)
  float triggerProbability;   // Note playback chance (0.0 to 1.0, default 1.0 = 100%)
  float ghostNoteChance;      // Injects low-velocity ghost notes (0.0 to 0.35, default 0.0)
  bool humanizeDuration;      // Slight note duration variation

  JZMidiHumanizerConfig()
    : timingJitterMs(6.0f),
      velocityJitter(8),
      triggerProbability(1.0f),
      ghostNoteChance(0.0f),
      humanizeDuration(true)
  {
  }
};

// MIDI Humanizer & Probability Class
class JZMidiHumanizerProbability
{
public:
  JZMidiHumanizerProbability(uint32_t seed = 42);
  ~JZMidiHumanizerProbability() = default;

  void SetConfig(const JZMidiHumanizerConfig& config) { mConfig = config; }
  const JZMidiHumanizerConfig& GetConfig() const { return mConfig; }
  JZMidiHumanizerConfig& GetConfig() { return mConfig; }

  void SetSeed(uint32_t seed);

  // Process and humanize a list of MIDI notes in-place
  void ProcessNotes(
    std::vector<JZMatrixMidiNote>& notes,
    float bpm,
    int ticksPerBar = 384);

private:
  JZMidiHumanizerConfig mConfig;
  std::mt19937 mRng;

  float GetGaussianRandom();
  float GetUniformRandom();
};
