//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Smart Auto-Accompaniment / Session Band Engine Implementation
//*****************************************************************************

#include "AutoAccompaniment.h"
#include "Song.h"
#include "Track.h"
#include "Events.h"

#include <algorithm>
#include <sstream>

using namespace std;

// Pitch class names
static const char* sPitchNames[12] = {
  "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
string JZDetectedChord::GetChordName() const
{
  if (!hasNotes)
  {
    return "N.C.";
  }

  string name = sPitchNames[rootPitchClass % 12];
  switch (quality)
  {
    case CHORD_MAJOR:
      break;
    case CHORD_MINOR:
      name += "m";
      break;
    case CHORD_DOMINANT7:
      name += "7";
      break;
    case CHORD_MAJOR7:
      name += "maj7";
      break;
    case CHORD_MINOR7:
      name += "m7";
      break;
    case CHORD_DIMINISHED:
      name += "dim";
      break;
    case CHORD_SUS4:
      name += "sus4";
      break;
  }
  return name;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
const char* JZAutoAccompaniment::GetStyleName(JZAccompanimentStyle style)
{
  switch (style)
  {
    case STYLE_JAZZ_SWING:
      return "Jazz Swing (Walking Bass, Spang-a-lang Ride, Chords)";
    case STYLE_BOSSA_NOVA:
      return "Bossa Nova (Syncopated Bass, Clave, Warm Pad)";
    case STYLE_BLUES_SHUFFLE:
      return "Blues Shuffle (Shuffle Boogie Bass & Drums)";
    case STYLE_POP_ROCK:
      return "Pop / Rock (Straight 8th Bass, 2&4 Snare Backbeat)";
    case STYLE_FUNK_GROOVE:
      return "Funk Groove (16th Syncopation, Octave Bass & Drums)";
    default:
      return "Jazz Swing";
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZDetectedChord JZAutoAccompaniment::AnalyzeMeasure(
  JZTrack* pSourceTrack,
  int barStartClock,
  int barEndClock,
  const JZDetectedChord& fallbackChord)
{
  JZDetectedChord result;
  if (!pSourceTrack)
  {
    return fallbackChord;
  }

  bool pcPresent[12] = {false};
  int noteCount = 0;
  int minPitch = 128;

  for (int i = 0; i < pSourceTrack->mEventCount; ++i)
  {
    JZEvent* pEvent = pSourceTrack->mppEvents[i];
    if (!pEvent || pEvent->IsKilled())
    {
      continue;
    }

    JZKeyOnEvent* pKeyOn = pEvent->IsKeyOn();
    if (!pKeyOn)
    {
      continue;
    }

    int clock = pKeyOn->GetClock();
    if (clock >= barStartClock && clock < barEndClock)
    {
      int pitch = pKeyOn->GetPitch();
      if (pitch >= 0 && pitch <= 127)
      {
        pcPresent[pitch % 12] = true;
        if (pitch < minPitch)
        {
          minPitch = pitch;
        }
        noteCount++;
      }
    }
  }

  if (noteCount == 0)
  {
    // No notes in this measure: inherit fallback chord if valid
    return fallbackChord;
  }

  result.hasNotes = true;
  result.minPitch = minPitch;
  result.rootPitchClass = minPitch % 12;

  // Chord intervals relative to root
  int r = result.rootPitchClass;
  bool hasMin3 = pcPresent[(r + 3) % 12];
  bool hasMaj3 = pcPresent[(r + 4) % 12];
  bool hasDim5 = pcPresent[(r + 6) % 12];
  bool hasP5   = pcPresent[(r + 7) % 12];
  bool hasMin7 = pcPresent[(r + 10) % 12];
  bool hasMaj7 = pcPresent[(r + 11) % 12];
  bool hasSus4 = pcPresent[(r + 5) % 12];

  if (hasMin3 && !hasMaj3)
  {
    if (hasDim5)
    {
      result.quality = CHORD_DIMINISHED;
    }
    else if (hasMin7)
    {
      result.quality = CHORD_MINOR7;
    }
    else
    {
      result.quality = CHORD_MINOR;
    }
  }
  else if (hasMaj3 || (!hasMin3 && !hasSus4))
  {
    if (hasMin7)
    {
      result.quality = CHORD_DOMINANT7;
    }
    else if (hasMaj7)
    {
      result.quality = CHORD_MAJOR7;
    }
    else
    {
      result.quality = CHORD_MAJOR;
    }
  }
  else if (hasSus4 && !hasMaj3)
  {
    result.quality = CHORD_SUS4;
  }
  else
  {
    result.quality = CHORD_MAJOR;
  }

  return result;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZAutoAccompaniment::ClearRange(JZTrack* pTrack, int fromClock, int toClock)
{
  if (!pTrack)
  {
    return;
  }

  for (int i = 0; i < pTrack->mEventCount; ++i)
  {
    JZEvent* pEvent = pTrack->mppEvents[i];
    if (pEvent && !pEvent->IsKilled())
    {
      int clk = pEvent->GetClock();
      if (clk >= fromClock && clk < toClock)
      {
        if (pEvent->IsKeyOn())
        {
          pTrack->Kill(pEvent);
        }
      }
    }
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZAutoAccompaniment::GenerateBassMeasure(
  JZTrack* pTrack,
  int barIndex,
  int barStartClock,
  int ticksPerBar,
  int ticksPerQuarter,
  int countsPerBar,
  const JZDetectedChord& chord,
  const JZDetectedChord& nextChord,
  JZAccompanimentStyle style)
{
  if (!pTrack || !chord.hasNotes)
  {
    return 0;
  }

  int ch = pTrack->GetChannel() > 0 ? (pTrack->GetChannel() - 1) : 1;
  int notesAdded = 0;

  // Root note in octave 2 (C2 = 36 to B2 = 47)
  int rootBass = 36 + (chord.rootPitchClass % 12);

  // Chord intervals
  int thirdBass = rootBass + 4;
  if (chord.quality == CHORD_MINOR || chord.quality == CHORD_MINOR7 || chord.quality == CHORD_DIMINISHED)
  {
    thirdBass = rootBass + 3;
  }
  else if (chord.quality == CHORD_SUS4)
  {
    thirdBass = rootBass + 5;
  }

  int fifthBass = (chord.quality == CHORD_DIMINISHED) ? (rootBass + 6) : (rootBass + 7);
  int seventhBass = (chord.quality == CHORD_MAJOR7) ? (rootBass + 11) : (rootBass + 10);
  int nextRootBass = 36 + (nextChord.rootPitchClass % 12);

  // Approach note to next bar root: chromatic step
  int approachBass = (nextRootBass > rootBass) ? (nextRootBass - 1) : (nextRootBass + 1);
  if (approachBass == nextRootBass)
  {
    approachBass = fifthBass;
  }

  int beatTicks = ticksPerQuarter;

  switch (style)
  {
    case STYLE_JAZZ_SWING:
    {
      if (countsPerBar == 4)
      {
        // 4/4 Walking Bass
        // Beat 1: Root note
        int len = (int)(beatTicks * 0.9);
        pTrack->Put(new JZKeyOnEvent(barStartClock, ch, (unsigned char)rootBass, 96, (unsigned short)len));
        notesAdded++;

        // Beat 2: 3rd or 5th
        int b2Pitch = (barIndex % 2 == 0) ? thirdBass : fifthBass;
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks, ch, (unsigned char)b2Pitch, 84, (unsigned short)len));
        notesAdded++;

        // Beat 3: 5th or Octave
        int b3Pitch = (barIndex % 2 == 0) ? fifthBass : (rootBass + 12);
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2, ch, (unsigned char)b3Pitch, 90, (unsigned short)len));
        notesAdded++;

        // Beat 4: Chromatic passing note towards next root
        int b4Len = (int)(beatTicks * 0.85);
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3, ch, (unsigned char)approachBass, 86, (unsigned short)b4Len));
        notesAdded++;
      }
      else
      {
        // 3/4 Jazz Waltz
        int len = (int)(beatTicks * 0.9);
        pTrack->Put(new JZKeyOnEvent(barStartClock, ch, (unsigned char)rootBass, 95, (unsigned short)len));
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks, ch, (unsigned char)thirdBass, 84, (unsigned short)len));
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2, ch, (unsigned char)fifthBass, 88, (unsigned short)len));
        notesAdded += 3;
      }
      break;
    }

    case STYLE_BOSSA_NOVA:
    {
      // Syncopated Brazilian Bossa pattern:
      // Beat 1: Root (dotted quarter, 1.5 beat)
      int len1 = (int)(beatTicks * 1.35);
      pTrack->Put(new JZKeyOnEvent(barStartClock, ch, (unsigned char)rootBass, 92, (unsigned short)len1));
      notesAdded++;

      // Beat 2.5: 5th (0.5 beat)
      int len2 = (int)(beatTicks * 0.45);
      pTrack->Put(new JZKeyOnEvent(barStartClock + (int)(beatTicks * 1.5), ch, (unsigned char)fifthBass, 82, (unsigned short)len2));
      notesAdded++;

      if (countsPerBar == 4)
      {
        // Beat 3: Root (dotted quarter)
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2, ch, (unsigned char)rootBass, 88, (unsigned short)len1));
        notesAdded++;

        // Beat 4.5: 5th (0.5 beat)
        pTrack->Put(new JZKeyOnEvent(barStartClock + (int)(beatTicks * 3.5), ch, (unsigned char)fifthBass, 80, (unsigned short)len2));
        notesAdded++;
      }
      break;
    }

    case STYLE_BLUES_SHUFFLE:
    {
      // 12-bar walking shuffle: root, 3rd, 5th, 6th
      int sixthBass = rootBass + 9;
      int len = (int)(beatTicks * 0.85);

      if (countsPerBar == 4)
      {
        pTrack->Put(new JZKeyOnEvent(barStartClock, ch, (unsigned char)rootBass, 95, (unsigned short)len));
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks, ch, (unsigned char)thirdBass, 85, (unsigned short)len));
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2, ch, (unsigned char)fifthBass, 90, (unsigned short)len));
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3, ch, (unsigned char)sixthBass, 85, (unsigned short)len));
        notesAdded += 4;
      }
      else
      {
        pTrack->Put(new JZKeyOnEvent(barStartClock, ch, (unsigned char)rootBass, 95, (unsigned short)len));
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks, ch, (unsigned char)thirdBass, 85, (unsigned short)len));
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2, ch, (unsigned char)fifthBass, 90, (unsigned short)len));
        notesAdded += 3;
      }
      break;
    }

    case STYLE_POP_ROCK:
    {
      // Driving 8th notes
      int eighthTicks = beatTicks / 2;
      int len = (int)(eighthTicks * 0.85);

      for (int b = 0; b < countsPerBar; ++b)
      {
        int pDown = (b == 2 && barIndex % 2 == 1) ? fifthBass : rootBass;
        int pUp = (b == 3 && barIndex % 4 == 3) ? approachBass : pDown;

        pTrack->Put(new JZKeyOnEvent(barStartClock + b * beatTicks, ch, (unsigned char)pDown, 95, (unsigned short)len));
        pTrack->Put(new JZKeyOnEvent(barStartClock + b * beatTicks + eighthTicks, ch, (unsigned char)pUp, 80, (unsigned short)len));
        notesAdded += 2;
      }
      break;
    }

    case STYLE_FUNK_GROOVE:
    {
      // Syncopated 16th funk bass with octave pops
      int sixteenth = beatTicks / 4;
      int lenShort = (int)(sixteenth * 0.9);
      int lenMed = (int)(beatTicks * 0.7);

      // Beat 1: Heavy root
      pTrack->Put(new JZKeyOnEvent(barStartClock, ch, (unsigned char)rootBass, 105, (unsigned short)lenShort));
      notesAdded++;

      // Beat 1.75: Octave pop
      pTrack->Put(new JZKeyOnEvent(barStartClock + sixteenth * 3, ch, (unsigned char)(rootBass + 12), 92, (unsigned short)lenShort));
      notesAdded++;

      // Beat 2.5: 3rd
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks + sixteenth * 2, ch, (unsigned char)thirdBass, 88, (unsigned short)lenShort));
      notesAdded++;

      // Beat 3.25: 5th
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2 + sixteenth, ch, (unsigned char)fifthBass, 96, (unsigned short)lenMed));
      notesAdded++;

      // Beat 4: Low Root
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3, ch, (unsigned char)rootBass, 98, (unsigned short)lenShort));
      notesAdded++;

      // Beat 4.75: Chromatic approach
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3 + sixteenth * 3, ch, (unsigned char)approachBass, 85, (unsigned short)lenShort));
      notesAdded++;
      break;
    }

    default:
      break;
  }

  return notesAdded;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZAutoAccompaniment::GenerateDrumsMeasure(
  JZTrack* pTrack,
  int barIndex,
  int barStartClock,
  int ticksPerBar,
  int ticksPerQuarter,
  int countsPerBar,
  JZAccompanimentStyle style)
{
  if (!pTrack)
  {
    return 0;
  }

  // MIDI Channel 10 is index 9 (0-based)
  int ch = 9;
  int notesAdded = 0;
  int beatTicks = ticksPerQuarter;
  int eighthTicks = beatTicks / 2;
  int sixteenthTicks = beatTicks / 4;
  int swingSkip = (beatTicks * 2) / 3; // Triplet swing 8th offset
  int hitLen = 40;

  // GM Drum constants
  const unsigned char KICK = 36;
  const unsigned char SNARE = 38;
  const unsigned char RIMSHOT = 37;
  const unsigned char CLOSED_HH = 42;
  const unsigned char PEDAL_HH = 44;
  const unsigned char OPEN_HH = 46;
  const unsigned char CRASH = 49;
  const unsigned char RIDE = 51;

  switch (style)
  {
    case STYLE_JAZZ_SWING:
    {
      // Crash on measure 0 downbeat
      if (barIndex == 0)
      {
        pTrack->Put(new JZKeyOnEvent(barStartClock, ch, CRASH, 95, (unsigned short)hitLen));
        notesAdded++;
      }

      if (countsPerBar == 4)
      {
        // Classic Jazz Swing Ride "Spang-a-lang"
        // Beat 1: Ride
        pTrack->Put(new JZKeyOnEvent(barStartClock, ch, RIDE, 88, (unsigned short)hitLen));
        notesAdded++;

        // Beat 2: Ride + Swing skip + Pedal Hi-Hat chick
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks, ch, RIDE, 94, (unsigned short)hitLen));
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks + swingSkip, ch, RIDE, 76, (unsigned short)hitLen));
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks, ch, PEDAL_HH, 90, (unsigned short)hitLen));
        notesAdded += 3;

        // Beat 3: Ride
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2, ch, RIDE, 88, (unsigned short)hitLen));
        notesAdded++;

        // Beat 4: Ride + Swing skip + Pedal Hi-Hat chick
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3, ch, RIDE, 94, (unsigned short)hitLen));
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3 + swingSkip, ch, RIDE, 76, (unsigned short)hitLen));
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3, ch, PEDAL_HH, 90, (unsigned short)hitLen));
        notesAdded += 3;

        // Subtle feathered bass drum on 1 and 3
        pTrack->Put(new JZKeyOnEvent(barStartClock, ch, KICK, 52, (unsigned short)hitLen));
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2, ch, KICK, 48, (unsigned short)hitLen));
        notesAdded += 2;
      }
      else
      {
        // 3/4 Jazz Waltz drums
        pTrack->Put(new JZKeyOnEvent(barStartClock, ch, RIDE, 90, (unsigned short)hitLen));
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks, ch, RIDE, 92, (unsigned short)hitLen));
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks + swingSkip, ch, RIDE, 75, (unsigned short)hitLen));
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2, ch, RIDE, 88, (unsigned short)hitLen));
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks, ch, PEDAL_HH, 88, (unsigned short)hitLen));
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2, ch, PEDAL_HH, 85, (unsigned short)hitLen));
        notesAdded += 6;
      }
      break;
    }

    case STYLE_BOSSA_NOVA:
    {
      // Continuous 8th note Hi-Hat
      for (int i = 0; i < countsPerBar * 2; ++i)
      {
        int vel = (i % 2 == 0) ? 70 : 58;
        pTrack->Put(new JZKeyOnEvent(barStartClock + i * eighthTicks, ch, CLOSED_HH, (unsigned char)vel, (unsigned short)hitLen));
        notesAdded++;
      }

      // Syncopated Bass Drum (dotted quarter pulse)
      pTrack->Put(new JZKeyOnEvent(barStartClock, ch, KICK, 82, (unsigned short)hitLen));
      pTrack->Put(new JZKeyOnEvent(barStartClock + (int)(beatTicks * 1.5), ch, KICK, 75, (unsigned short)hitLen));
      notesAdded += 2;

      if (countsPerBar == 4)
      {
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2, ch, KICK, 80, (unsigned short)hitLen));
        pTrack->Put(new JZKeyOnEvent(barStartClock + (int)(beatTicks * 3.5), ch, KICK, 75, (unsigned short)hitLen));
        notesAdded += 2;

        // Bossa Clave on Rimshot: Beat 1, 2.5, 3.5, 4
        pTrack->Put(new JZKeyOnEvent(barStartClock, ch, RIMSHOT, 85, (unsigned short)hitLen));
        pTrack->Put(new JZKeyOnEvent(barStartClock + (int)(beatTicks * 1.5), ch, RIMSHOT, 88, (unsigned short)hitLen));
        pTrack->Put(new JZKeyOnEvent(barStartClock + (int)(beatTicks * 2.5), ch, RIMSHOT, 86, (unsigned short)hitLen));
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3, ch, RIMSHOT, 84, (unsigned short)hitLen));
        notesAdded += 4;
      }
      break;
    }

    case STYLE_BLUES_SHUFFLE:
    {
      // Swung shuffle triplet ride on all beats
      for (int b = 0; b < countsPerBar; ++b)
      {
        pTrack->Put(new JZKeyOnEvent(barStartClock + b * beatTicks, ch, RIDE, 90, (unsigned short)hitLen));
        pTrack->Put(new JZKeyOnEvent(barStartClock + b * beatTicks + swingSkip, ch, RIDE, 76, (unsigned short)hitLen));
        notesAdded += 2;
      }

      // Snare on 2 and 4
      if (countsPerBar == 4)
      {
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks, ch, SNARE, 100, (unsigned short)hitLen));
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3, ch, SNARE, 100, (unsigned short)hitLen));
        notesAdded += 2;
      }

      // Four-on-the-floor kick
      for (int b = 0; b < countsPerBar; ++b)
      {
        pTrack->Put(new JZKeyOnEvent(barStartClock + b * beatTicks, ch, KICK, 85, (unsigned short)hitLen));
        notesAdded++;
      }
      break;
    }

    case STYLE_POP_ROCK:
    {
      if (barIndex == 0)
      {
        pTrack->Put(new JZKeyOnEvent(barStartClock, ch, CRASH, 100, (unsigned short)hitLen));
        notesAdded++;
      }

      // Straight 8th Hi-Hats
      for (int i = 0; i < countsPerBar * 2; ++i)
      {
        int vel = (i % 2 == 0) ? 85 : 68;
        pTrack->Put(new JZKeyOnEvent(barStartClock + i * eighthTicks, ch, CLOSED_HH, (unsigned char)vel, (unsigned short)hitLen));
        notesAdded++;
      }

      // Snare on 2 and 4
      if (countsPerBar == 4)
      {
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks, ch, SNARE, 102, (unsigned short)hitLen));
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3, ch, SNARE, 105, (unsigned short)hitLen));
        notesAdded += 2;

        // Kick on 1 and 3 (and 2.5 on alternate bars)
        pTrack->Put(new JZKeyOnEvent(barStartClock, ch, KICK, 100, (unsigned short)hitLen));
        notesAdded++;
        if (barIndex % 2 == 1)
        {
          pTrack->Put(new JZKeyOnEvent(barStartClock + (int)(beatTicks * 1.5), ch, KICK, 85, (unsigned short)hitLen));
          notesAdded++;
        }
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2, ch, KICK, 95, (unsigned short)hitLen));
        notesAdded++;
      }
      break;
    }

    case STYLE_FUNK_GROOVE:
    {
      // 16th note tight Hi-Hat with occasional open hat
      for (int i = 0; i < countsPerBar * 4; ++i)
      {
        bool isOpen = (i == 7 || i == 15);
        unsigned char hat = isOpen ? OPEN_HH : CLOSED_HH;
        int vel = (i % 4 == 0) ? 90 : ((i % 2 == 0) ? 75 : 60);
        if (isOpen) vel = 95;

        pTrack->Put(new JZKeyOnEvent(barStartClock + i * sixteenthTicks, ch, hat, (unsigned char)vel, (unsigned short)hitLen));
        notesAdded++;
      }

      // Snare on 2 and 4 + ghost notes
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks, ch, SNARE, 108, (unsigned short)hitLen));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3, ch, SNARE, 108, (unsigned short)hitLen));
      notesAdded += 2;

      // Snare ghost notes
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks + sixteenthTicks * 2, ch, SNARE, 50, (unsigned short)hitLen));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2 + sixteenthTicks * 3, ch, SNARE, 52, (unsigned short)hitLen));
      notesAdded += 2;

      // Syncopated Kick
      pTrack->Put(new JZKeyOnEvent(barStartClock, ch, KICK, 102, (unsigned short)hitLen));
      pTrack->Put(new JZKeyOnEvent(barStartClock + sixteenthTicks * 3, ch, KICK, 88, (unsigned short)hitLen));
      pTrack->Put(new JZKeyOnEvent(barStartClock + (int)(beatTicks * 1.5), ch, KICK, 94, (unsigned short)hitLen));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2 + sixteenthTicks, ch, KICK, 96, (unsigned short)hitLen));
      notesAdded += 4;
      break;
    }

    default:
      break;
  }

  return notesAdded;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZAutoAccompaniment::GeneratePadMeasure(
  JZTrack* pTrack,
  int barIndex,
  int barStartClock,
  int ticksPerBar,
  int ticksPerQuarter,
  int countsPerBar,
  const JZDetectedChord& chord,
  JZAccompanimentStyle style)
{
  if (!pTrack || !chord.hasNotes)
  {
    return 0;
  }

  int ch = pTrack->GetChannel() > 0 ? (pTrack->GetChannel() - 1) : 2;
  int notesAdded = 0;

  // Root in octave 4 (C4 = 60 to B4 = 71)
  int rootPad = 60 + (chord.rootPitchClass % 12);
  if (rootPad > 67)
  {
    rootPad -= 12; // Keep root in comfortable C4-G4 range
  }

  int thirdPad = rootPad + 4;
  if (chord.quality == CHORD_MINOR || chord.quality == CHORD_MINOR7 || chord.quality == CHORD_DIMINISHED)
  {
    thirdPad = rootPad + 3;
  }
  else if (chord.quality == CHORD_SUS4)
  {
    thirdPad = rootPad + 5;
  }

  int fifthPad = (chord.quality == CHORD_DIMINISHED) ? (rootPad + 6) : (rootPad + 7);
  int seventhPad = (chord.quality == CHORD_MAJOR7) ? (rootPad + 11) : (rootPad + 10);

  // Legato whole note duration
  int noteLen = ticksPerBar - (ticksPerQuarter / 8);
  unsigned char vel = 70;

  // Voicing: Root, 5th, 3rd (octave up), and 7th if 7th chord
  pTrack->Put(new JZKeyOnEvent(barStartClock, ch, (unsigned char)rootPad, vel, (unsigned short)noteLen));
  pTrack->Put(new JZKeyOnEvent(barStartClock, ch, (unsigned char)fifthPad, (unsigned char)(vel - 5), (unsigned short)noteLen));
  pTrack->Put(new JZKeyOnEvent(barStartClock, ch, (unsigned char)(thirdPad + 12), (unsigned char)(vel - 3), (unsigned short)noteLen));
  notesAdded += 3;

  if (chord.quality == CHORD_DOMINANT7 || chord.quality == CHORD_MAJOR7 || chord.quality == CHORD_MINOR7)
  {
    pTrack->Put(new JZKeyOnEvent(barStartClock, ch, (unsigned char)seventhPad, (unsigned char)(vel - 8), (unsigned short)noteLen));
    notesAdded++;
  }

  return notesAdded;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZAccompanimentResult JZAutoAccompaniment::Generate(JZSong* pSong, const JZAccompanimentConfig& config)
{
  JZAccompanimentResult res;
  if (!pSong)
  {
    res.message = "Invalid song project.";
    return res;
  }

  JZTrack* pSourceTrack = pSong->GetTrack(config.sourceTrackIndex);
  if (!pSourceTrack)
  {
    res.message = "Source track not found.";
    return res;
  }

  // Target tracks
  JZTrack* pBassTrack = config.generateBass ? pSong->GetTrack(config.bassTrackIndex) : 0;
  JZTrack* pDrumsTrack = config.generateDrums ? pSong->GetTrack(config.drumsTrackIndex) : 0;
  JZTrack* pPadTrack = config.generatePad ? pSong->GetTrack(config.padTrackIndex) : 0;

  // Determine bar range
  JZBarInfo barInfo(*pSong);
  barInfo.SetBar(0);

  int lastSongClock = pSong->GetLastClock();
  if (lastSongClock <= 0)
  {
    res.message = "Source track has no notes. Please record or input notes on the piano track first.";
    return res;
  }

  // Find total bars
  JZBarInfo tempBarInfo(*pSong);
  tempBarInfo.SetClock(lastSongClock);
  int songTotalBars = tempBarInfo.GetBarIndex() + 1;

  int startBar = max(0, config.fromBar);
  int endBar = (config.toBar >= 0) ? min(config.toBar, songTotalBars) : songTotalBars;

  if (endBar <= startBar)
  {
    endBar = startBar + 1;
  }

  // Setup track configurations
  if (pBassTrack)
  {
    if (pBassTrack->IsEmpty() || string(pBassTrack->GetName()).empty())
    {
      pBassTrack->SetName("Auto Bass");
    }
    pBassTrack->SetChannel(2);
    int patch = (config.style == STYLE_FUNK_GROOVE || config.style == STYLE_POP_ROCK) ? 34 : 33;
    pBassTrack->SetPatch(patch);
  }

  if (pDrumsTrack)
  {
    if (pDrumsTrack->IsEmpty() || string(pDrumsTrack->GetName()).empty())
    {
      pDrumsTrack->SetName("Auto Drums");
    }
    pDrumsTrack->SetChannel(10);
  }

  if (pPadTrack)
  {
    if (pPadTrack->IsEmpty() || string(pPadTrack->GetName()).empty())
    {
      pPadTrack->SetName("Auto Pad");
    }
    pPadTrack->SetChannel(3);
    int patch = (config.style == STYLE_BOSSA_NOVA) ? 90 : 49;
    pPadTrack->SetPatch(patch);
  }

  // Pre-analyze chords for all bars
  vector<JZDetectedChord> chords;
  chords.reserve(endBar - startBar + 1);

  JZDetectedChord currentChord;
  currentChord.rootPitchClass = 0; // C
  currentChord.quality = CHORD_MAJOR;
  currentChord.hasNotes = true;

  JZBarInfo chordBarIter(*pSong);
  chordBarIter.SetBar(startBar);

  for (int b = startBar; b < endBar; ++b)
  {
    int bStart = chordBarIter.GetClock();
    int bTicks = chordBarIter.GetTicksPerBar();
    int bEnd = bStart + bTicks;

    currentChord = AnalyzeMeasure(pSourceTrack, bStart, bEnd, currentChord);
    chords.push_back(currentChord);

    chordBarIter.Next();
  }

  // Check if any notes were found across the whole selection
  bool anyNotesFound = false;
  for (size_t i = 0; i < chords.size(); ++i)
  {
    if (chords[i].hasNotes)
    {
      anyNotesFound = true;
      break;
    }
  }

  if (!anyNotesFound)
  {
    res.message = "No notes detected in the selected measure range.";
    return res;
  }

  // Calculate clock boundaries for clear range
  JZBarInfo startInfo(*pSong);
  startInfo.SetBar(startBar);
  int fromClock = startInfo.GetClock();

  JZBarInfo endInfo(*pSong);
  endInfo.SetBar(endBar);
  int toClock = endInfo.GetClock();

  if (config.overwriteExisting)
  {
    if (pBassTrack) ClearRange(pBassTrack, fromClock, toClock);
    if (pDrumsTrack) ClearRange(pDrumsTrack, fromClock, toClock);
    if (pPadTrack) ClearRange(pPadTrack, fromClock, toClock);
  }

  // Generate measures
  JZBarInfo genIter(*pSong);
  genIter.SetBar(startBar);

  int totalBass = 0;
  int totalDrums = 0;
  int totalPad = 0;

  for (int b = startBar; b < endBar; ++b)
  {
    int barIdx = b - startBar;
    int bStart = genIter.GetClock();
    int bTicks = genIter.GetTicksPerBar();
    int bCounts = genIter.GetCountsPerBar();
    int tpq = pSong->GetTicksPerQuarter();

    const JZDetectedChord& ch = chords[barIdx];
    const JZDetectedChord& nextCh = (barIdx + 1 < (int)chords.size()) ? chords[barIdx + 1] : ch;

    if (pBassTrack)
    {
      totalBass += GenerateBassMeasure(pBassTrack, b, bStart, bTicks, tpq, bCounts, ch, nextCh, config.style);
    }

    if (pDrumsTrack)
    {
      totalDrums += GenerateDrumsMeasure(pDrumsTrack, b, bStart, bTicks, tpq, bCounts, config.style);
    }

    if (pPadTrack)
    {
      totalPad += GeneratePadMeasure(pPadTrack, b, bStart, bTicks, tpq, bCounts, ch, config.style);
    }

    genIter.Next();
  }

  if (pBassTrack) pBassTrack->Cleanup();
  if (pDrumsTrack) pDrumsTrack->Cleanup();
  if (pPadTrack) pPadTrack->Cleanup();

  res.success = true;
  res.barsProcessed = endBar - startBar;
  res.bassNotesGenerated = totalBass;
  res.drumNotesGenerated = totalDrums;
  res.padNotesGenerated = totalPad;

  ostringstream oss;
  oss << "Generated accompaniment for " << res.barsProcessed << " measure(s):\n"
      << " - Bass: " << totalBass << " notes (Track " << (config.bassTrackIndex + 1) << ")\n"
      << " - Drums: " << totalDrums << " notes (Track " << (config.drumsTrackIndex + 1) << ")\n"
      << " - Strings/Pad: " << totalPad << " notes (Track " << (config.padTrackIndex + 1) << ")";
  res.message = oss.str();

  return res;
}
