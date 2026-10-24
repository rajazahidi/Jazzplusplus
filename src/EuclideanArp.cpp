//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Euclidean Rhythm Generator & Advanced Algorithmic Arpeggiator Implementation
//*****************************************************************************

#include "EuclideanArp.h"
#include "Track.h"
#include "Events.h"

#include <cmath>
#include <algorithm>

using namespace std;

//-----------------------------------------------------------------------------
// JZEuclideanRhythm Implementation
//-----------------------------------------------------------------------------
vector<bool> JZEuclideanRhythm::Generate(int pulses, int steps, int rotation)
{
  if (steps <= 0) steps = 8;
  pulses = max(0, min(steps, pulses));

  vector<bool> pattern;
  pattern.reserve(steps);

  if (pulses == 0)
  {
    pattern.assign(steps, false);
    return pattern;
  }
  if (pulses == steps)
  {
    pattern.assign(steps, true);
    return pattern;
  }

  // Bresenham accumulator distribution
  int bucket = 0;
  for (int i = 0; i < steps; ++i)
  {
    bucket += pulses;
    if (bucket >= steps)
    {
      bucket -= steps;
      pattern.push_back(true);
    }
    else
    {
      pattern.push_back(false);
    }
  }

  // Apply rotation
  if (rotation != 0 && steps > 1)
  {
    int rot = (rotation % steps + steps) % steps;
    vector<bool> rotated(steps);
    for (int i = 0; i < steps; ++i)
    {
      rotated[(i + rot) % steps] = pattern[i];
    }
    return rotated;
  }

  return pattern;
}

int JZEuclideanRhythm::RenderToTrack(
  JZTrack* pTrack,
  int pulses,
  int steps,
  int startClock,
  int ticksPerStep,
  int pitch,
  int velocity,
  int channel,
  int rotation)
{
  if (!pTrack || steps <= 0) return 0;

  vector<bool> pat = Generate(pulses, steps, rotation);
  int notesCreated = 0;
  int noteLen = max(6, ticksPerStep / 2);

  for (size_t i = 0; i < pat.size(); ++i)
  {
    if (pat[i])
    {
      int clk = startClock + (int)i * ticksPerStep;
      // Slight accent on downbeat
      int vel = (i == 0) ? min(127, (int)(velocity * 1.15f)) : velocity;
      pTrack->Put(new JZKeyOnEvent(clk, channel, (unsigned char)pitch, (unsigned char)vel, (unsigned short)noteLen));
      notesCreated++;
    }
  }

  return notesCreated;
}

//-----------------------------------------------------------------------------
// JZAlgorithmicArpeggiator Implementation
//-----------------------------------------------------------------------------
const char* JZAlgorithmicArpeggiator::GetStyleName(JZAlgorithmicArpStyle style)
{
  switch (style)
  {
    case ARP_UP_DOWN: return "Up / Down Classic Arpeggio";
    case ARP_STRUM_ROLL: return "Strum Roll (Acoustic Spread)";
    case ARP_CHORD_REPEAT: return "Chord Repeat Stabs";
    case ARP_RANDOM_POLY: return "Random Polyphonic Organic";
    case ARP_WALKING_OCTAVES: return "Walking Octaves (Jazz Bass)";
    default: return "Up / Down Arpeggio";
  }
}

int JZAlgorithmicArpeggiator::RenderArp(
  JZTrack* pTrack,
  const vector<int>& chordPitches,
  JZAlgorithmicArpStyle style,
  int startClock,
  int totalClock,
  int ticksPerStep,
  int velocity,
  int channel,
  int octaveRange)
{
  if (!pTrack || chordPitches.empty() || totalClock <= 0 || ticksPerStep <= 0)
  {
    return 0;
  }

  octaveRange = max(1, min(3, octaveRange));
  vector<int> sortedPitches = chordPitches;
  sort(sortedPitches.begin(), sortedPitches.end());

  // Expand across octave range
  vector<int> fullPitches;
  for (int oct = 0; oct < octaveRange; ++oct)
  {
    for (size_t p = 0; p < sortedPitches.size(); ++p)
    {
      int pitch = sortedPitches[p] + (oct * 12);
      if (pitch <= 120) fullPitches.push_back(pitch);
    }
  }
  if (fullPitches.empty()) fullPitches = chordPitches;

  int notesCreated = 0;
  int currentClock = startClock;
  int endClock = startClock + totalClock;
  int noteLen = max(6, (int)(ticksPerStep * 0.85f));
  int stepIndex = 0;

  while (currentClock < endClock)
  {
    switch (style)
    {
      case ARP_UP_DOWN:
      {
        int totalCycle = ((int)fullPitches.size() * 2) - 2;
        if (totalCycle <= 0) totalCycle = (int)fullPitches.size();
        int cyclePos = stepIndex % totalCycle;
        int pIdx = (cyclePos < (int)fullPitches.size())
                     ? cyclePos
                     : (totalCycle - cyclePos);
        int pitch = fullPitches[max(0, min((int)fullPitches.size() - 1, pIdx))];
        pTrack->Put(new JZKeyOnEvent(currentClock, channel, (unsigned char)pitch, (unsigned char)velocity, (unsigned short)noteLen));
        notesCreated++;
        break;
      }

      case ARP_STRUM_ROLL:
      {
        // Tight guitar strum rolling across chord pitches (12 ticks offset)
        for (size_t p = 0; p < sortedPitches.size(); ++p)
        {
          int rollClk = currentClock + (int)p * 8;
          if (rollClk < endClock)
          {
            int pitch = sortedPitches[p];
            int vel = max(1, min(127, (int)(velocity * (0.85f + (p * 0.05f)))));
            pTrack->Put(new JZKeyOnEvent(rollClk, channel, (unsigned char)pitch, (unsigned char)vel, (unsigned short)noteLen));
            notesCreated++;
          }
        }
        break;
      }

      case ARP_CHORD_REPEAT:
      {
        // Punchy syncopated chord stabs
        for (size_t p = 0; p < sortedPitches.size(); ++p)
        {
          pTrack->Put(new JZKeyOnEvent(currentClock, channel, (unsigned char)sortedPitches[p], (unsigned char)velocity, (unsigned short)noteLen));
          notesCreated++;
        }
        break;
      }

      case ARP_WALKING_OCTAVES:
      {
        // Alternating low root and upper octave root
        int baseRoot = sortedPitches.front();
        int pitch = (stepIndex % 2 == 0) ? baseRoot : (baseRoot + 12);
        pTrack->Put(new JZKeyOnEvent(currentClock, channel, (unsigned char)pitch, (unsigned char)velocity, (unsigned short)noteLen));
        notesCreated++;
        break;
      }

      case ARP_RANDOM_POLY:
      {
        int pIdx = (stepIndex * 3 + 1) % (int)fullPitches.size();
        int pitch = fullPitches[pIdx];
        pTrack->Put(new JZKeyOnEvent(currentClock, channel, (unsigned char)pitch, (unsigned char)velocity, (unsigned short)noteLen));
        notesCreated++;
        break;
      }

      default:
        break;
    }

    currentClock += ticksPerStep;
    stepIndex++;
  }

  return notesCreated;
}
