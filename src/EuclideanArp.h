//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Euclidean Rhythm Generator & Advanced Algorithmic Arpeggiator
//
// Bjorklund / Bresenham Euclidean Algorithm E(k, n), Syncopated Polyrhythms,
// Strum Rolling, Chord Repeat, Walking Octaves & Algorithmic Pattern Sequencer.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <algorithm>

class JZTrack;

// Algorithmic Arpeggiator Patterns
enum JZAlgorithmicArpStyle
{
  ARP_UP_DOWN = 0,         // Classic Up / Down arpeggio
  ARP_STRUM_ROLL,          // Tight realistic strum rolling (15-25ms spread)
  ARP_CHORD_REPEAT,        // Punchy syncopated chord repeat stabs
  ARP_RANDOM_POLY,         // Randomized organic polyphonic voicings
  ARP_WALKING_OCTAVES,     // Jazz walking bass octave alternation
  ARP_STYLE_COUNT
};

// Euclidean Rhythm Generator
class JZEuclideanRhythm
{
  public:

    // Generates binary Euclidean rhythm vector of size 'steps' with 'pulses' distributed
    // e.g. E(5, 8) -> [1, 0, 1, 1, 0, 1, 1, 0] (Cuban Cinquillo)
    static std::vector<bool> Generate(int pulses, int steps, int rotation = 0);

    // Common named world music Euclidean presets
    static std::vector<bool> PresetTresillo() { return Generate(3, 8); }
    static std::vector<bool> PresetCinquillo() { return Generate(5, 8); }
    static std::vector<bool> PresetSamba() { return Generate(7, 16); }
    static std::vector<bool> PresetAfrobeat() { return Generate(5, 16); }
    static std::vector<bool> PresetBlues12_8() { return Generate(4, 12); }
    static std::vector<bool> PresetAksak() { return Generate(9, 16); }

    // Renders Euclidean pattern into target track with specified pitch, velocity, and clock grid
    static int RenderToTrack(
      JZTrack* pTrack,
      int pulses,
      int steps,
      int startClock,
      int ticksPerStep = 24,
      int pitch = 36,
      int velocity = 100,
      int channel = 9,
      int rotation = 0);
};

// Algorithmic Arpeggiator Engine
class JZAlgorithmicArpeggiator
{
  public:

    static int RenderArp(
      JZTrack* pTrack,
      const std::vector<int>& chordPitches,
      JZAlgorithmicArpStyle style,
      int startClock,
      int totalClock,
      int ticksPerStep = 24,
      int velocity = 90,
      int channel = 0,
      int octaveRange = 1);

    static const char* GetStyleName(JZAlgorithmicArpStyle style);
};
