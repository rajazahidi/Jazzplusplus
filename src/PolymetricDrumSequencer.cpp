//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Polymetric & Polyrhythmic Drum Machine Sequencer Implementation
//*****************************************************************************

#include "PolymetricDrumSequencer.h"
#include <cstdlib>
#include <cmath>
#include <algorithm>

JZPolymetricDrumSequencer::JZPolymetricDrumSequencer()
  : mGlobalSwing(0.0f),
    mGlobalStepTick(0)
{
  SetTrackCount(4);
  LoadNeoSoulPolymeterPreset();
}

void JZPolymetricDrumSequencer::SetTrackCount(size_t count)
{
  mTracks.resize(count);
  static const int defaultNotes[8] = {36, 38, 42, 46, 39, 49, 70, 75};
  static const char* defaultNames[8] = {"Kick", "Snare", "Closed Hat", "Open Hat", "Clap", "Crash", "Maracas", "Claves"};

  for (size_t i = 0; i < mTracks.size(); ++i)
  {
    int note = defaultNotes[i % 8];
    const char* name = defaultNames[i % 8];
    mTracks[i] = JZDrumTrack(name, note, 16);
  }
}

JZDrumTrack& JZPolymetricDrumSequencer::GetTrack(size_t index)
{
  static JZDrumTrack dummy("Dummy", 36, 16);
  if (index < mTracks.size())
  {
    return mTracks[index];
  }
  return dummy;
}

const JZDrumTrack& JZPolymetricDrumSequencer::GetTrack(size_t index) const
{
  static JZDrumTrack dummy("Dummy", 36, 16);
  if (index < mTracks.size())
  {
    return mTracks[index];
  }
  return dummy;
}

void JZPolymetricDrumSequencer::SetGlobalSwing(float swingPercent)
{
  mGlobalSwing = std::clamp(swingPercent, 0.0f, 75.0f);
}

void JZPolymetricDrumSequencer::ResetPlayhead()
{
  mGlobalStepTick = 0;
  for (auto& track : mTracks)
  {
    track.currentStep = 0;
  }
}

void JZPolymetricDrumSequencer::AdvanceStep(std::vector<JZTriggeredDrumNote>& outTriggeredNotes)
{
  outTriggeredNotes.clear();

  for (auto& track : mTracks)
  {
    if (track.stepCount <= 0)
    {
      continue;
    }

    int stepIdx = track.currentStep % track.stepCount;
    if (stepIdx < static_cast<int>(track.steps.size()))
    {
      const auto& step = track.steps[stepIdx];
      if (step.active)
      {
        int roll = std::rand() % 100;
        if (roll < step.probability)
        {
          int swingOffset = 0;
          if (stepIdx % 2 != 0) // Off-beat step
          {
            swingOffset = static_cast<int>((mGlobalSwing / 100.0f) * 24.0f);
          }

          int ratchets = std::clamp(step.ratchetCount, 1, 4);
          for (int r = 0; r < ratchets; ++r)
          {
            JZTriggeredDrumNote note;
            note.midiNote = track.midiNote;
            note.velocity = std::clamp(step.velocity - r * 6, 1, 127);
            note.subTickOffset = swingOffset + r * (24 / ratchets);
            outTriggeredNotes.push_back(note);
          }
        }
      }
    }

    // Advance track-specific independent loop counter
    track.currentStep = (track.currentStep + 1) % track.stepCount;
  }

  mGlobalStepTick++;
}

void JZPolymetricDrumSequencer::LoadNeoSoulPolymeterPreset()
{
  if (mTracks.size() < 4)
  {
    SetTrackCount(4);
  }

  // Track 0: Kick (16-step straight grid)
  mTracks[0].name = "Kick 4/4";
  mTracks[0].midiNote = 36;
  mTracks[0].stepCount = 16;
  for (int i = 0; i < 16; ++i)
  {
    mTracks[0].steps[i].active = (i == 0 || i == 6 || i == 10);
    mTracks[0].steps[i].velocity = (i == 0) ? 120 : 105;
  }

  // Track 1: Snare (16-step grid, backbeat on 4 and 12)
  mTracks[1].name = "Snare Backbeat";
  mTracks[1].midiNote = 38;
  mTracks[1].stepCount = 16;
  for (int i = 0; i < 16; ++i)
  {
    mTracks[1].steps[i].active = (i == 4 || i == 12);
    mTracks[1].steps[i].velocity = 115;
  }

  // Track 2: Closed Hat (12-step triplet polymeter!)
  mTracks[2].name = "HiHat 12-Poly";
  mTracks[2].midiNote = 42;
  mTracks[2].stepCount = 12; // 12-step cycle creates 3:4 polymeter against 16-step kick
  for (int i = 0; i < 12; ++i)
  {
    mTracks[2].steps[i].active = true;
    mTracks[2].steps[i].velocity = (i % 3 == 0) ? 105 : 85;
    mTracks[2].steps[i].probability = 95;
    if (i == 11) mTracks[2].steps[i].ratchetCount = 2; // Micro roll
  }

  // Track 3: Maracas / Percussion (7-step odd polymeter!)
  mTracks[3].name = "Perc 7-Poly";
  mTracks[3].midiNote = 70;
  mTracks[3].stepCount = 7; // 7-step odd loop creates shifting syncopation
  for (int i = 0; i < 7; ++i)
  {
    mTracks[3].steps[i].active = (i == 0 || i == 2 || i == 5);
    mTracks[3].steps[i].velocity = 90;
    mTracks[3].steps[i].probability = 85;
  }

  SetGlobalSwing(18.0f); // Subtle neo-soul bounce
}
