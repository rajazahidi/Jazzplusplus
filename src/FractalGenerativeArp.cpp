//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Multi-Layer Fractal Generative Arpeggiator Implementation
//*****************************************************************************

#include "FractalGenerativeArp.h"
#include <cstdlib>
#include <cmath>
#include <algorithm>

JZFractalGenerativeArp::JZFractalGenerativeArp(int ticksPerQuarter)
  : mTicksPerQuarter(ticksPerQuarter > 24 ? ticksPerQuarter : 96),
    mMutationProbability(20.0f),
    mRecursionDepth(2),
    mOctaveSpread(2),
    mMasterStep(0)
{
  mLayers.resize(4);
  mLayers[0] = JZFractalLayer(16, 0, 110, 0.9f);  // Layer 0: 16-step root bass
  mLayers[1] = JZFractalLayer(12, 1, 95, 0.75f);  // Layer 1: 12-step Fibonacci mid
  mLayers[2] = JZFractalLayer(7, 2, 85, 0.6f);    // Layer 2: 7-step Mandelbrot syncopation
  mLayers[3] = JZFractalLayer(5, 2, 75, 0.5f);    // Layer 3: 5-step High chaos sparkle

  mHeldPitches = {60, 64, 67, 71}; // Default Cmaj7
  GenerateFractalSequences();
}

void JZFractalGenerativeArp::SetMutationProbability(float percent)
{
  mMutationProbability = std::clamp(percent, 0.0f, 100.0f);
}

void JZFractalGenerativeArp::SetRecursionDepth(int depth)
{
  mRecursionDepth = std::clamp(depth, 1, 4);
  GenerateFractalSequences();
}

void JZFractalGenerativeArp::SetOctaveSpread(int octaves)
{
  mOctaveSpread = std::clamp(octaves, 1, 4);
}

void JZFractalGenerativeArp::SetHeldChordNotes(const std::vector<int>& chordPitches)
{
  if (!chordPitches.empty())
  {
    mHeldPitches = chordPitches;
    std::sort(mHeldPitches.begin(), mHeldPitches.end());
    GenerateFractalSequences();
  }
}

void JZFractalGenerativeArp::GenerateFractalSequences()
{
  if (mHeldPitches.empty()) return;
  size_t numNotes = mHeldPitches.size();

  // Layer 1: Fibonacci modulo sequence
  int fib0 = 1, fib1 = 1;
  for (int i = 0; i < mLayers[1].stepCount; ++i)
  {
    int nextFib = fib0 + fib1;
    fib0 = fib1;
    fib1 = nextFib;
    mLayers[1].fractalOffsets[i] = nextFib % static_cast<int>(numNotes);
  }

  // Layer 2: Mandelbrot iterative quadratic map: Z = (Z^2 + 1) mod numNotes
  int z = 1;
  for (int i = 0; i < mLayers[2].stepCount; ++i)
  {
    z = (z * z + 1) % static_cast<int>(numNotes);
    mLayers[2].fractalOffsets[i] = z;
  }

  // Layer 3: Chaos cellular automata pattern
  for (int i = 0; i < mLayers[3].stepCount; ++i)
  {
    mLayers[3].fractalOffsets[i] = (i * 3 + mRecursionDepth) % static_cast<int>(numNotes);
  }
}

void JZFractalGenerativeArp::Reset()
{
  mMasterStep = 0;
  for (auto& l : mLayers)
  {
    l.currentStep = 0;
  }
}

void JZFractalGenerativeArp::AdvanceStep(std::vector<JZArpGeneratedNote>& outTriggeredNotes)
{
  outTriggeredNotes.clear();
  if (mHeldPitches.empty()) return;

  int stepTicks = mTicksPerQuarter / 4; // 16th note master clock

  for (size_t l = 0; l < mLayers.size(); ++l)
  {
    auto& layer = mLayers[l];
    if (layer.stepCount <= 0) continue;

    int stepIdx = layer.currentStep % layer.stepCount;
    int noteIdx = 0;

    if (l == 0)
    {
      noteIdx = stepIdx % static_cast<int>(mHeldPitches.size());
    }
    else
    {
      noteIdx = layer.fractalOffsets[stepIdx % layer.fractalOffsets.size()];
    }

    // Probabilistic Mutation
    if (mMutationProbability > 0.01f && (std::rand() % 100) < static_cast<int>(mMutationProbability))
    {
      noteIdx = (noteIdx + 1 + (std::rand() % 2)) % static_cast<int>(mHeldPitches.size());
    }

    int pitch = mHeldPitches[noteIdx % mHeldPitches.size()];
    pitch += layer.octaveOffset * 12;

    int dur = static_cast<int>(stepTicks * layer.gateFactor);
    if (dur < 12) dur = 12;

    int vel = std::clamp(layer.baseVelocity + ((stepIdx % 4 == 0) ? 10 : -5), 1, 127);

    outTriggeredNotes.emplace_back(static_cast<int>(l), pitch, vel, dur, 0);

    // Independent layer loop cycle advance
    layer.currentStep = (layer.currentStep + 1) % layer.stepCount;
  }

  mMasterStep++;
}
