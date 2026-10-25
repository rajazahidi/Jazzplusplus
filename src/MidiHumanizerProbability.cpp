//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// MIDI Humanizer, Note Trigger Probability & Generative Chance Implementation
//*****************************************************************************

#include "MidiHumanizerProbability.h"
#include <cstdint>

JZMidiHumanizerProbability::JZMidiHumanizerProbability(uint32_t seed)
  : mRng(seed)
{
}

void JZMidiHumanizerProbability::SetSeed(uint32_t seed)
{
  mRng.seed(seed);
}

float JZMidiHumanizerProbability::GetGaussianRandom()
{
  // Box-Muller transform for zero-mean unit-variance Gaussian
  float u1 = std::max(1.0e-7f, (float)mRng() / (float)mRng.max());
  float u2 = (float)mRng() / (float)mRng.max();
  return std::sqrt(-2.0f * std::log(u1)) * std::cos(6.2831853f * u2);
}

float JZMidiHumanizerProbability::GetUniformRandom()
{
  return (float)mRng() / (float)mRng.max();
}

void JZMidiHumanizerProbability::ProcessNotes(
  std::vector<JZMatrixMidiNote>& notes,
  float bpm,
  int ticksPerBar)
{
  if (notes.empty() || bpm <= 10.0f) return;

  float ticksPerMs = (bpm * 96.0f) / 60000.0f; // 96 PPQN
  std::vector<JZMatrixMidiNote> processed;
  processed.reserve(notes.size() + 4);

  for (const auto& note : notes)
  {
    // 1. Trigger Probability test
    if (mConfig.triggerProbability < 0.999f)
    {
      if (GetUniformRandom() > mConfig.triggerProbability)
      {
        continue; // Note skipped by generative chance!
      }
    }

    JZMatrixMidiNote outNote = note;

    // 2. Gaussian micro-timing jitter
    if (mConfig.timingJitterMs > 0.001f)
    {
      float jitterMs = GetGaussianRandom() * mConfig.timingJitterMs;
      int deltaTicks = (int)(jitterMs * ticksPerMs);
      outNote.startTick = std::max(0, outNote.startTick + deltaTicks);
    }

    // 3. Velocity randomization
    if (mConfig.velocityJitter > 0)
    {
      int vJitter = (int)(GetGaussianRandom() * (float)mConfig.velocityJitter);
      outNote.velocity = std::max(1, std::min(127, outNote.velocity + vJitter));
    }

    // 4. Duration humanization
    if (mConfig.humanizeDuration)
    {
      float durFactor = 1.0f + GetGaussianRandom() * 0.08f;
      outNote.durationTicks = std::max(12, (int)(outNote.durationTicks * durFactor));
    }

    processed.push_back(outNote);

    // 5. Ghost note injection
    if (mConfig.ghostNoteChance > 0.001f && GetUniformRandom() < mConfig.ghostNoteChance)
    {
      JZMatrixMidiNote ghost = outNote;
      ghost.startTick += 24; // 16th note offset
      ghost.velocity = std::max(15, (int)(outNote.velocity * 0.35f));
      ghost.durationTicks = 24;
      processed.push_back(ghost);
    }
  }

  // Sort notes chronologically by startTick
  std::sort(processed.begin(), processed.end(), [](const JZMatrixMidiNote& a, const JZMatrixMidiNote& b) {
    return a.startTick < b.startTick;
  });

  notes = processed;
}
