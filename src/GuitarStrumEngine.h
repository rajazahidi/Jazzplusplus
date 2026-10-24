//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Realistic Guitar Strumming & Fingerpicking Engine
//
// Transforms static MIDI block chords into expressive acoustic and electric
// guitar performances featuring directional strumming (Downstroke/Upstroke),
// realistic inter-string delay timing, fingerpicking patterns (P-I-M-A),
// dynamic velocity weighting, and palm muting articulation.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

enum class JZStrumDirection
{
  Downstroke = 0,   // Low strings to high strings (Bass -> Treble)
  Upstroke,         // High strings to low strings (Treble -> Bass)
  Alternate,        // Alternating down and up strokes
  FingerpickingPIMA // P-I-M-A classical/acoustic arpeggio
};

struct JZStrummedNote
{
  int midiPitch;
  int velocity;
  int startTick;
  int durationTicks;
  int stringIndex;  // 1 to 6 (standard guitar strings)

  JZStrummedNote(int pitch = 60, int vel = 100, int tick = 0, int dur = 96, int str = 1)
    : midiPitch(pitch),
      velocity(vel),
      startTick(tick),
      durationTicks(dur),
      stringIndex(str)
  {
  }
};

class JZGuitarStrumEngine
{
public:
  JZGuitarStrumEngine(int ticksPerQuarter = 96);
  ~JZGuitarStrumEngine() = default;

  void SetTicksPerQuarter(int tpq) { mTicksPerQuarter = tpq > 24 ? tpq : 96; }
  int GetTicksPerQuarter() const { return mTicksPerQuarter; }

  // Strum Controls
  void SetStrumSpeedTicks(int speedTicks); // Micro-delay between successive strings (e.g. 2 to 16 ticks)
  int GetStrumSpeedTicks() const { return mStrumSpeedTicks; }

  void SetStrumDirection(JZStrumDirection dir) { mDirection = dir; }
  JZStrumDirection GetStrumDirection() const { return mDirection; }

  void SetPalmMute(bool enable, float damping = 0.5f);
  bool IsPalmMute() const { return mPalmMuteEnabled; }

  void SetVelocityTilt(float tilt); // -1.0 (accent low strings) to +1.0 (accent high strings)
  float GetVelocityTilt() const { return mVelocityTilt; }

  // Generate expressive strummed notes from input block chord
  void GenerateStrum(
    const std::vector<int>& rawChordPitches,
    int chordStartTick,
    int chordDurationTicks,
    int baseVelocity,
    std::vector<JZStrummedNote>& outStrummedNotes);

private:
  int mTicksPerQuarter;
  int mStrumSpeedTicks;
  JZStrumDirection mDirection;
  bool mPalmMuteEnabled;
  float mPalmMuteDamping;
  float mVelocityTilt;
  bool mAlternateToggle;
};
