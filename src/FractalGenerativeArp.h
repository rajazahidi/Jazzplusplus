//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Multi-Layer Fractal Generative Arpeggiator
//
// 4-layer generative arpeggiator engine driven by mathematical Fibonacci
// and recursive self-similar fractal patterns, polyrhythmic step gating,
// probabilistic pattern mutation, and multi-octave branching.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

struct JZArpGeneratedNote
{
  int layerIndex;
  int midiPitch;
  int velocity;
  int durationTicks;
  int tickOffset;

  JZArpGeneratedNote(int layer = 0, int pitch = 60, int vel = 100, int dur = 48, int offset = 0)
    : layerIndex(layer),
      midiPitch(pitch),
      velocity(vel),
      durationTicks(dur),
      tickOffset(offset)
  {
  }
};

struct JZFractalLayer
{
  int stepCount;              // Independent layer step length (e.g. 16, 12, 7, 5)
  int currentStep;
  int octaveOffset;           // e.g. -1, 0, +1, +2
  int baseVelocity;
  float gateFactor;           // 0.2 to 1.5
  std::vector<int> fractalOffsets;

  JZFractalLayer(int steps = 16, int oct = 0, int vel = 100, float gate = 0.8f)
    : stepCount(steps),
      currentStep(0),
      octaveOffset(oct),
      baseVelocity(vel),
      gateFactor(gate)
  {
    fractalOffsets.resize(steps, 0);
  }
};

class JZFractalGenerativeArp
{
public:
  JZFractalGenerativeArp(int ticksPerQuarter = 96);
  ~JZFractalGenerativeArp() = default;

  void SetTicksPerQuarter(int tpq) { mTicksPerQuarter = tpq > 24 ? tpq : 96; }
  int GetTicksPerQuarter() const { return mTicksPerQuarter; }

  // Fractal Controls
  void SetMutationProbability(float percent); // 0% to 100%
  float GetMutationProbability() const { return mMutationProbability; }

  void SetRecursionDepth(int depth);          // 1 to 4
  int GetRecursionDepth() const { return mRecursionDepth; }

  void SetOctaveSpread(int octaves);          // 1 to 4
  int GetOctaveSpread() const { return mOctaveSpread; }

  // Set Chord Input Notes
  void SetHeldChordNotes(const std::vector<int>& chordPitches);

  // Advance Sequence by 1 Master Clock Step
  void AdvanceStep(std::vector<JZArpGeneratedNote>& outTriggeredNotes);
  void Reset();

private:
  int mTicksPerQuarter;
  float mMutationProbability;
  int mRecursionDepth;
  int mOctaveSpread;

  std::vector<int> mHeldPitches;
  std::vector<JZFractalLayer> mLayers;
  int mMasterStep;

  void GenerateFractalSequences();
};
