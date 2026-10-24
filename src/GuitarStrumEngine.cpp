//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Realistic Guitar Strumming & Fingerpicking Implementation
//*****************************************************************************

#include "GuitarStrumEngine.h"
#include <algorithm>
#include <cstdlib>

JZGuitarStrumEngine::JZGuitarStrumEngine(int ticksPerQuarter)
  : mTicksPerQuarter(ticksPerQuarter > 24 ? ticksPerQuarter : 96),
    mStrumSpeedTicks(4), // ~15ms strum delay at 120 BPM
    mDirection(JZStrumDirection::Downstroke),
    mPalmMuteEnabled(false),
    mPalmMuteDamping(0.35f),
    mVelocityTilt(-0.2f), // Authentic guitar downstrokes naturally hit bass harder
    mAlternateToggle(false)
{
}

void JZGuitarStrumEngine::SetStrumSpeedTicks(int speedTicks)
{
  mStrumSpeedTicks = std::clamp(speedTicks, 1, 32);
}

void JZGuitarStrumEngine::SetPalmMute(bool enable, float damping)
{
  mPalmMuteEnabled = enable;
  mPalmMuteDamping = std::clamp(damping, 0.1f, 0.9f);
}

void JZGuitarStrumEngine::SetVelocityTilt(float tilt)
{
  mVelocityTilt = std::clamp(tilt, -1.0f, 1.0f);
}

void JZGuitarStrumEngine::GenerateStrum(
  const std::vector<int>& rawChordPitches,
  int chordStartTick,
  int chordDurationTicks,
  int baseVelocity,
  std::vector<JZStrummedNote>& outStrummedNotes)
{
  outStrummedNotes.clear();
  if (rawChordPitches.empty() || chordDurationTicks <= 0)
  {
    return;
  }

  std::vector<int> sortedPitches = rawChordPitches;
  std::sort(sortedPitches.begin(), sortedPitches.end());

  size_t noteCount = sortedPitches.size();
  std::vector<size_t> playOrder(noteCount);

  JZStrumDirection currentDir = mDirection;
  if (currentDir == JZStrumDirection::Alternate)
  {
    currentDir = mAlternateToggle ? JZStrumDirection::Upstroke : JZStrumDirection::Downstroke;
    mAlternateToggle = !mAlternateToggle;
  }

  if (currentDir == JZStrumDirection::FingerpickingPIMA)
  {
    // Classical acoustic P-I-M-A arpeggiation across 4 steps
    int stepTicks = mTicksPerQuarter / 2; // 8th note spacing
    for (size_t i = 0; i < noteCount; ++i)
    {
      int noteTick = chordStartTick + static_cast<int>(i) * stepTicks;
      int noteDur = (mPalmMuteEnabled) ? static_cast<int>(stepTicks * mPalmMuteDamping) : (chordDurationTicks - static_cast<int>(i) * stepTicks);
      if (noteDur < 12) noteDur = 12;

      int vel = std::clamp(baseVelocity + ((i == 0) ? 12 : -4), 1, 127);
      outStrummedNotes.emplace_back(sortedPitches[i], vel, noteTick, noteDur, static_cast<int>(i + 1));
    }
    return;
  }

  if (currentDir == JZStrumDirection::Downstroke)
  {
    for (size_t i = 0; i < noteCount; ++i)
    {
      playOrder[i] = i; // Low to high
    }
  }
  else // Upstroke
  {
    for (size_t i = 0; i < noteCount; ++i)
    {
      playOrder[i] = noteCount - 1 - i; // High to low
    }
  }

  // Generate strummed notes with physical inter-string delay
  for (size_t step = 0; step < noteCount; ++step)
  {
    size_t pitchIdx = playOrder[step];
    int pitch = sortedPitches[pitchIdx];

    int stringOffset = static_cast<int>(step) * mStrumSpeedTicks;
    int noteStart = chordStartTick + stringOffset;
    int noteDur = chordDurationTicks - stringOffset;

    if (mPalmMuteEnabled)
    {
      noteDur = static_cast<int>(chordDurationTicks * mPalmMuteDamping);
    }
    if (noteDur < 16)
    {
      noteDur = 16;
    }

    // Velocity dynamics
    float normString = (noteCount > 1) ? (static_cast<float>(pitchIdx) / static_cast<float>(noteCount - 1)) : 0.5f;
    float tiltFactor = 1.0f + mVelocityTilt * (normString - 0.5f);
    int vel = static_cast<int>(baseVelocity * tiltFactor);
    vel = std::clamp(vel, 1, 127);

    outStrummedNotes.emplace_back(pitch, vel, noteStart, noteDur, static_cast<int>(pitchIdx + 1));
  }
}
