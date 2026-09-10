//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Copyright (C) 2026 Jazz++ Developers
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//*****************************************************************************

#pragma once

#include <string>
#include <vector>

class JZTrack;
class JZEvent;

// ============================================================================
// 1. MIDI Arpeggiator
// ============================================================================
enum JZArpPattern
{
  eArpUp = 0,
  eArpDown,
  eArpUpDown,
  eArpDownUp,
  eArpRandom,
  eArpChordPulse
};

enum JZArpRate
{
  eArpRate_Quarter = 0,  // 1/4
  eArpRate_Eighth,       // 1/8
  eArpRate_Sixteenth,    // 1/16
  eArpRate_ThirtySecond, // 1/32
  eArpRate_EighthTriplet,// 1/8T
  eArpRate_SixteenthTriplet // 1/16T
};

struct JZArpSettings
{
  JZArpPattern pattern;
  JZArpRate rate;
  int octaves;         // 1 to 4
  int gatePercent;     // 10% to 200% (default 80%)
  int velocity;        // 1 to 127 (or -1 to keep original note velocity)
  std::string scaleName; // optional constraint, e.g. "Major", "Natural Minor", "" for unconstrained
  int scaleRoot;       // 0 = C, 1 = C#, ... 11 = B

  JZArpSettings()
    : pattern(eArpUp),
      rate(eArpRate_Sixteenth),
      octaves(1),
      gatePercent(80),
      velocity(-1),
      scaleName(""),
      scaleRoot(0)
  {
  }
};

class JZMidiArpeggiator
{
  public:
    static int GetRateTicks(JZArpRate rate, int ticksPerQuarter = 480);

    // Expands a base set of chord pitches across specified octaves according to pattern
    static std::vector<int> GeneratePitchSequence(
      const std::vector<int>& chordPitches,
      JZArpPattern pattern,
      int octaves,
      const std::string& scaleName = "",
      int scaleRoot = 0);

    // Apply arpeggiator to selected range on a track
    static bool Apply(
      JZTrack* pTrack,
      int startClock,
      int endClock,
      int ticksPerQuarter,
      const JZArpSettings& settings);
};

// ============================================================================
// 2. MIDI Humanizer & Swing
// ============================================================================
struct JZHumanizeSettings
{
  int timingJitterTicks;  // +/- ticks, e.g. 0 to 30
  int velocityJitter;     // +/- velocity, e.g. 0 to 25
  int swingPercent;       // 50% = straight, 66% = triplet swing, 75% = hard shuffle
  int swingGridTicks;     // e.g. 240 for 1/8 note swing at 480 tpq

  JZHumanizeSettings()
    : timingJitterTicks(10),
      velocityJitter(8),
      swingPercent(50),
      swingGridTicks(240)
  {
  }
};

class JZMidiHumanizer
{
  public:
    static bool Apply(
      JZTrack* pTrack,
      int startClock,
      int endClock,
      const JZHumanizeSettings& settings);
};

// ============================================================================
// 3. Intelligent MIDI Harmonizer
// ============================================================================
enum JZHarmonizeInterval
{
  eHarmThirdAbove = 0,
  eHarmThirdBelow,
  eHarmFifthAbove,
  eHarmFifthBelow,
  eHarmOctaveAbove,
  eHarmOctaveBelow,
  eHarmDiatonicTriad,    // Root + 3rd + 5th
  eHarmDiatonicSeventh   // Root + 3rd + 5th + 7th
};

struct JZHarmonizeSettings
{
  JZHarmonizeInterval interval;
  int scaleRoot;          // 0 = C, 1 = C#, ... 11 = B
  std::string scaleName;  // e.g. "Major", "Natural Minor", "Dorian"
  int velocityRatioPercent; // e.g. 80% of original velocity

  JZHarmonizeSettings()
    : interval(eHarmThirdAbove),
      scaleRoot(0),
      scaleName("Major"),
      velocityRatioPercent(85)
  {
  }
};

class JZMidiHarmonizer
{
  public:
    // Calculates added harmonic pitches for a given single pitch within a musical scale
    static std::vector<int> GetHarmonizedPitches(
      int basePitch,
      JZHarmonizeInterval interval,
      int scaleRoot,
      const std::string& scaleName);

    // Apply harmonizer to selected range on a track
    static bool Apply(
      JZTrack* pTrack,
      int startClock,
      int endClock,
      const JZHarmonizeSettings& settings);
};

// ============================================================================
// 4. Tempo-Synced MIDI Delay / Echo
// ============================================================================
struct JZMidiEchoSettings
{
  int delayTicks;         // e.g. 240 for 1/8th note, 480 for 1/4 note
  int repeatCount;        // 1 to 16 repeats
  double feedback;        // 0.1 to 0.95 (decay ratio per repeat)
  int pitchShiftPerRepeat;// -12 to +12 semitones per bounce

  JZMidiEchoSettings()
    : delayTicks(240),
      repeatCount(4),
      feedback(0.65),
      pitchShiftPerRepeat(0)
  {
  }
};

class JZMidiEcho
{
  public:
    static bool Apply(
      JZTrack* pTrack,
      int startClock,
      int endClock,
      const JZMidiEchoSettings& settings);
};
