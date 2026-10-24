//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Audio Transient Groove Extractor & Swing Quantize Template
//
// Extracts the living micro-timing pocket and velocity dynamics from real
// audio recordings (e.g. vintage funk breaks, live drum takes), generating
// custom reusable Groove Templates that can be quantized onto any MIDI track.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

struct JZGrooveTemplate
{
  std::string name;
  float detectedBpm;
  float swingPercent;         // 50% = straight, 66% = triplet, etc.
  std::vector<int> timingOffsetsTicks; // 16-step micro-timing deviation in ticks
  std::vector<float> velocityScalers;  // 16-step velocity accents (0.5 to 1.5)

  JZGrooveTemplate()
    : name("Extracted Audio Groove"),
      detectedBpm(120.0f),
      swingPercent(50.0f)
  {
    timingOffsetsTicks.assign(16, 0);
    velocityScalers.assign(16, 1.0f);
  }
};

class JZAudioGrooveExtractor
{
public:
  JZAudioGrooveExtractor(float sampleRate = 44100.0f, int ticksPerQuarter = 96);
  ~JZAudioGrooveExtractor() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  void SetTicksPerQuarter(int tpq) { mTicksPerQuarter = tpq > 24 ? tpq : 96; }
  int GetTicksPerQuarter() const { return mTicksPerQuarter; }

  // Extract Groove from Audio Loop
  bool ExtractGrooveFromAudio(
    const std::vector<float>& audioLoop,
    float knownOrEstimatedBpm,
    JZGrooveTemplate& outGroove);

  // Apply Extracted Groove to MIDI note timings
  int ApplyGrooveToTick(int rawTick, const JZGrooveTemplate& groove, float grooveAmountPercent) const;
  int ApplyGrooveToVelocity(int rawVelocity, int tick, const JZGrooveTemplate& groove, float grooveAmountPercent) const;

private:
  float mSampleRate;
  int mTicksPerQuarter;

  void DetectTransients(const std::vector<float>& audio, std::vector<size_t>& outTransientSamples);
};
