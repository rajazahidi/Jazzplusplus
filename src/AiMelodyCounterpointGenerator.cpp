//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// AI MIDI Melody & Counterpoint Generator Implementation
//*****************************************************************************

#include "AiMelodyCounterpointGenerator.h"
#include <cstdlib>
#include <cmath>
#include <algorithm>

JZAiMelodyCounterpointGenerator::JZAiMelodyCounterpointGenerator(int ticksPerQuarter)
  : mTicksPerQuarter(ticksPerQuarter > 24 ? ticksPerQuarter : 96),
    mStepToLeapRatio(0.75f), // 75% stepwise motion
    mDensity(1.0f),          // 1 note per quarter note
    mSyncopation(0.25f),
    mMinPitch(60),           // C4
    mMaxPitch(84)            // C6
{
}

void JZAiMelodyCounterpointGenerator::SetStepToLeapRatio(float ratio)
{
  mStepToLeapRatio = std::clamp(ratio, 0.1f, 1.0f);
}

void JZAiMelodyCounterpointGenerator::SetDensity(float notesPerQuarter)
{
  mDensity = std::clamp(notesPerQuarter, 0.25f, 4.0f);
}

void JZAiMelodyCounterpointGenerator::SetSyncopation(float syncFactor)
{
  mSyncopation = std::clamp(syncFactor, 0.0f, 1.0f);
}

void JZAiMelodyCounterpointGenerator::SetOctaveRange(int minPitch, int maxPitch)
{
  mMinPitch = std::clamp(minPitch, 36, 96);
  mMaxPitch = std::clamp(maxPitch, mMinPitch + 12, 108);
}

int JZAiMelodyCounterpointGenerator::ChooseNextPitch(
  int currentPitch,
  const JZChordContext& chord,
  bool isContraryMotion,
  int leadStepPitch)
{
  int root = chord.rootMidi % 12;
  int third = (root + (chord.isMinor ? 3 : 4)) % 12;
  int fifth = (root + 7) % 12;

  if (isContraryMotion)
  {
    // Contrary Motion Counterpoint: Move opposite to lead melody
    int leadDelta = leadStepPitch - currentPitch;
    int cpPitch = currentPitch;

    if (leadDelta > 0)
    {
      cpPitch -= (1 + (std::rand() % 3)); // Lead went up -> Counterpoint goes down
    }
    else
    {
      cpPitch += (1 + (std::rand() % 3)); // Lead went down -> Counterpoint goes up
    }

    // Check consonant interval (3rd, 6th, octave - avoid parallel 5th / octave)
    int interval = std::abs(leadStepPitch - cpPitch) % 12;
    if (interval == 7 || interval == 0) // Avoid parallel 5th / unison
    {
      cpPitch += (std::rand() % 2 == 0) ? 2 : -2; // Resolve to consonant 3rd/6th
    }

    return std::clamp(cpPitch, mMinPitch - 12, mMaxPitch - 12);
  }

  // Lead Melody Decision
  float roll = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
  int nextPitch = currentPitch;

  if (roll <= mStepToLeapRatio)
  {
    // Stepwise motion (diatonic step +/- 1 or 2 semitones)
    int step = (std::rand() % 2 == 0) ? 2 : -2;
    nextPitch = currentPitch + step;
  }
  else
  {
    // Leap to nearest Chord Tone (3rd, 5th, or root)
    int cand1 = currentPitch + 4;
    int cand2 = currentPitch - 3;
    nextPitch = (std::rand() % 2 == 0) ? cand1 : cand2;
  }

  // Boundary clamp
  if (nextPitch < mMinPitch) nextPitch += 12;
  if (nextPitch > mMaxPitch) nextPitch -= 12;

  return nextPitch;
}

bool JZAiMelodyCounterpointGenerator::Generate(
  const std::vector<JZChordContext>& chordProgression,
  std::vector<JZMelodyNote>& outLeadMelody,
  std::vector<JZMelodyNote>& outCounterpoint)
{
  outLeadMelody.clear();
  outCounterpoint.clear();

  if (chordProgression.empty())
  {
    return false;
  }

  int currentLeadPitch = mMinPitch + 7;       // Start on 5th of scale
  int currentCpPitch = mMinPitch - 5;         // Start lower in counterpoint

  int stepTicks = static_cast<int>(mTicksPerQuarter / mDensity);
  if (stepTicks < 24) stepTicks = 24;

  for (const auto& chord : chordProgression)
  {
    int chordEndTick = chord.startTick + chord.durationTicks;

    for (int t = chord.startTick; t < chordEndTick; t += stepTicks)
    {
      // Optional rhythmic syncopation shift
      int noteTick = t;
      if (mSyncopation > 0.01f && (std::rand() % 100) < static_cast<int>(mSyncopation * 60.0f))
      {
        noteTick += stepTicks / 2; // Shift to off-beat
      }

      int leadPitch = ChooseNextPitch(currentLeadPitch, chord, false, 0);
      currentLeadPitch = leadPitch;

      int cpPitch = ChooseNextPitch(currentCpPitch, chord, true, leadPitch);
      currentCpPitch = cpPitch;

      int dur = static_cast<int>(stepTicks * 0.9f);
      int velLead = 95 + (std::rand() % 16);
      int velCp = 80 + (std::rand() % 14);

      outLeadMelody.emplace_back(leadPitch, velLead, noteTick, dur, false);
      outCounterpoint.emplace_back(cpPitch, velCp, noteTick, dur, true);
    }
  }

  return !outLeadMelody.empty();
}
