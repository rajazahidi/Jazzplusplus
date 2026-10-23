//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Smart Auto-Accompaniment & Music Intelligence Engine Implementation
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
const char* JZAutoAccompaniment::GetStrumPluckStyleName(JZStrumPluckStyle style)
{
  switch (style)
  {
    case STRUM_ACOUSTIC_FOLK:
      return "Acoustic Folk Strum (D - D U - U D -)";
    case STRUM_POP_ROCK_8TH:
      return "Pop / Rock Driving 8th Strum (D U D U)";
    case STRUM_FUNK_16TH_CHOP:
      return "Funk 16th Rhythm Chops (with Chucks)";
    case STRUM_REGGAE_SKANK:
      return "Reggae / Ska Skank (Offbeat Chops)";
    case PLUCK_TRAVIS_PICKING:
      return "Travis Picking (Alternating Bass Fingerstyle)";
    case PLUCK_BALLAD_ARPEGGIO:
      return "Ballad Broken Chord Arpeggio (Flowing 8ths)";
    case PLUCK_PIMA_CLASSICAL:
      return "P-I-M-A Classical Arpeggio (Pulgar-Indice-Medio-Anular)";
    case PLUCK_NEO_SOUL:
      return "Neo-Soul / Ambient Pluck (Spread Voicings)";
    default:
      return "Acoustic Folk Strum";
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
const char* JZAutoAccompaniment::GetRiffStyleName(JZRiffStyle style)
{
  switch (style)
  {
    case RIFF_BLUES_ROCK:
      return "Blues Rock Driving Riff (Pentatonic Boogie)";
    case RIFF_FUNK_BRASS:
      return "Funk Brass Section Hits & Stabs";
    case RIFF_HARD_ROCK_POWER:
      return "Hard Rock Palm-Muted Power Riff";
    case RIFF_SYNTHWAVE_BASS:
      return "Synthwave 16th Rolling Bassline";
    case RIFF_JAZZ_BEBOP_MOTIF:
      return "Jazz Bebop Chromatic Enclosure Motif";
    default:
      return "Blues Rock Driving Riff";
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
const char* JZAutoAccompaniment::GetSoloStyleName(JZSoloStyle style)
{
  switch (style)
  {
    case SOLO_MELODIC_LYRICAL:
      return "Melodic & Lyrical (Vocal phrasing, sweet target tones)";
    case SOLO_BLUESY_SOULFUL:
      return "Bluesy & Soulful (Blue notes, call-and-response)";
    case SOLO_BEBOP_JAZZ:
      return "Bebop Virtuoso (Swung 8ths, chromatic passing tones)";
    case SOLO_ROCK_PENTATONIC:
      return "Rock Lead (High-energy pentatonic hooks & licks)";
    default:
      return "Melodic & Lyrical";
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
    return fallbackChord;
  }

  result.hasNotes = true;
  result.minPitch = minPitch;
  result.rootPitchClass = minPitch % 12;

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
// Bass Generation
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

  int rootBass = 36 + (chord.rootPitchClass % 12);
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
  int nextRootBass = 36 + (nextChord.rootPitchClass % 12);

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
        int len = (int)(beatTicks * 0.9);
        pTrack->Put(new JZKeyOnEvent(barStartClock, ch, (unsigned char)rootBass, 96, (unsigned short)len));
        notesAdded++;

        int b2Pitch = (barIndex % 2 == 0) ? thirdBass : fifthBass;
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks, ch, (unsigned char)b2Pitch, 84, (unsigned short)len));
        notesAdded++;

        int b3Pitch = (barIndex % 2 == 0) ? fifthBass : (rootBass + 12);
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2, ch, (unsigned char)b3Pitch, 90, (unsigned short)len));
        notesAdded++;

        int b4Len = (int)(beatTicks * 0.85);
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3, ch, (unsigned char)approachBass, 86, (unsigned short)b4Len));
        notesAdded++;
      }
      else
      {
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
      int len1 = (int)(beatTicks * 1.35);
      pTrack->Put(new JZKeyOnEvent(barStartClock, ch, (unsigned char)rootBass, 92, (unsigned short)len1));
      notesAdded++;

      int len2 = (int)(beatTicks * 0.45);
      pTrack->Put(new JZKeyOnEvent(barStartClock + (int)(beatTicks * 1.5), ch, (unsigned char)fifthBass, 82, (unsigned short)len2));
      notesAdded++;

      if (countsPerBar == 4)
      {
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2, ch, (unsigned char)rootBass, 88, (unsigned short)len1));
        notesAdded++;
        pTrack->Put(new JZKeyOnEvent(barStartClock + (int)(beatTicks * 3.5), ch, (unsigned char)fifthBass, 80, (unsigned short)len2));
        notesAdded++;
      }
      break;
    }

    case STYLE_BLUES_SHUFFLE:
    {
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
      break;
    }

    case STYLE_POP_ROCK:
    {
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
      int sixteenth = beatTicks / 4;
      int lenShort = (int)(sixteenth * 0.9);
      int lenMed = (int)(beatTicks * 0.7);

      pTrack->Put(new JZKeyOnEvent(barStartClock, ch, (unsigned char)rootBass, 105, (unsigned short)lenShort));
      pTrack->Put(new JZKeyOnEvent(barStartClock + sixteenth * 3, ch, (unsigned char)(rootBass + 12), 92, (unsigned short)lenShort));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks + sixteenth * 2, ch, (unsigned char)thirdBass, 88, (unsigned short)lenShort));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2 + sixteenth, ch, (unsigned char)fifthBass, 96, (unsigned short)lenMed));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3, ch, (unsigned char)rootBass, 98, (unsigned short)lenShort));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3 + sixteenth * 3, ch, (unsigned char)approachBass, 85, (unsigned short)lenShort));
      notesAdded += 6;
      break;
    }

    default:
      break;
  }

  return notesAdded;
}

//-----------------------------------------------------------------------------
// Drums Generation
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

  int ch = 9; // MIDI Ch 10
  int notesAdded = 0;
  int beatTicks = ticksPerQuarter;
  int eighthTicks = beatTicks / 2;
  int sixteenthTicks = beatTicks / 4;
  int swingSkip = (beatTicks * 2) / 3;
  int hitLen = 40;

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
      if (barIndex == 0)
      {
        pTrack->Put(new JZKeyOnEvent(barStartClock, ch, CRASH, 95, (unsigned short)hitLen));
        notesAdded++;
      }

      if (countsPerBar == 4)
      {
        pTrack->Put(new JZKeyOnEvent(barStartClock, ch, RIDE, 88, (unsigned short)hitLen));
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks, ch, RIDE, 94, (unsigned short)hitLen));
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks + swingSkip, ch, RIDE, 76, (unsigned short)hitLen));
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks, ch, PEDAL_HH, 90, (unsigned short)hitLen));

        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2, ch, RIDE, 88, (unsigned short)hitLen));
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3, ch, RIDE, 94, (unsigned short)hitLen));
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3 + swingSkip, ch, RIDE, 76, (unsigned short)hitLen));
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3, ch, PEDAL_HH, 90, (unsigned short)hitLen));

        pTrack->Put(new JZKeyOnEvent(barStartClock, ch, KICK, 52, (unsigned short)hitLen));
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2, ch, KICK, 48, (unsigned short)hitLen));
        notesAdded += 10;
      }
      break;
    }

    case STYLE_BOSSA_NOVA:
    {
      for (int i = 0; i < countsPerBar * 2; ++i)
      {
        int vel = (i % 2 == 0) ? 70 : 58;
        pTrack->Put(new JZKeyOnEvent(barStartClock + i * eighthTicks, ch, CLOSED_HH, (unsigned char)vel, (unsigned short)hitLen));
        notesAdded++;
      }

      pTrack->Put(new JZKeyOnEvent(barStartClock, ch, KICK, 82, (unsigned short)hitLen));
      pTrack->Put(new JZKeyOnEvent(barStartClock + (int)(beatTicks * 1.5), ch, KICK, 75, (unsigned short)hitLen));
      notesAdded += 2;

      if (countsPerBar == 4)
      {
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2, ch, KICK, 80, (unsigned short)hitLen));
        pTrack->Put(new JZKeyOnEvent(barStartClock + (int)(beatTicks * 3.5), ch, KICK, 75, (unsigned short)hitLen));
        notesAdded += 2;

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
      for (int b = 0; b < countsPerBar; ++b)
      {
        pTrack->Put(new JZKeyOnEvent(barStartClock + b * beatTicks, ch, RIDE, 90, (unsigned short)hitLen));
        pTrack->Put(new JZKeyOnEvent(barStartClock + b * beatTicks + swingSkip, ch, RIDE, 76, (unsigned short)hitLen));
        pTrack->Put(new JZKeyOnEvent(barStartClock + b * beatTicks, ch, KICK, 85, (unsigned short)hitLen));
        notesAdded += 3;
      }
      if (countsPerBar == 4)
      {
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks, ch, SNARE, 100, (unsigned short)hitLen));
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3, ch, SNARE, 100, (unsigned short)hitLen));
        notesAdded += 2;
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
      for (int i = 0; i < countsPerBar * 2; ++i)
      {
        int vel = (i % 2 == 0) ? 85 : 68;
        pTrack->Put(new JZKeyOnEvent(barStartClock + i * eighthTicks, ch, CLOSED_HH, (unsigned char)vel, (unsigned short)hitLen));
        notesAdded++;
      }
      if (countsPerBar == 4)
      {
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks, ch, SNARE, 102, (unsigned short)hitLen));
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3, ch, SNARE, 105, (unsigned short)hitLen));
        pTrack->Put(new JZKeyOnEvent(barStartClock, ch, KICK, 100, (unsigned short)hitLen));
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2, ch, KICK, 95, (unsigned short)hitLen));
        notesAdded += 4;
      }
      break;
    }

    case STYLE_FUNK_GROOVE:
    {
      for (int i = 0; i < countsPerBar * 4; ++i)
      {
        bool isOpen = (i == 7 || i == 15);
        unsigned char hat = isOpen ? OPEN_HH : CLOSED_HH;
        int vel = (i % 4 == 0) ? 90 : ((i % 2 == 0) ? 75 : 60);
        if (isOpen) vel = 95;
        pTrack->Put(new JZKeyOnEvent(barStartClock + i * sixteenthTicks, ch, hat, (unsigned char)vel, (unsigned short)hitLen));
        notesAdded++;
      }
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks, ch, SNARE, 108, (unsigned short)hitLen));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3, ch, SNARE, 108, (unsigned short)hitLen));
      pTrack->Put(new JZKeyOnEvent(barStartClock, ch, KICK, 102, (unsigned short)hitLen));
      pTrack->Put(new JZKeyOnEvent(barStartClock + sixteenthTicks * 3, ch, KICK, 88, (unsigned short)hitLen));
      pTrack->Put(new JZKeyOnEvent(barStartClock + (int)(beatTicks * 1.5), ch, KICK, 94, (unsigned short)hitLen));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2 + sixteenthTicks, ch, KICK, 96, (unsigned short)hitLen));
      notesAdded += 6;
      break;
    }

    default:
      break;
  }

  return notesAdded;
}

//-----------------------------------------------------------------------------
// Pad / Strings Generation
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

  int rootPad = 60 + (chord.rootPitchClass % 12);
  if (rootPad > 67)
  {
    rootPad -= 12;
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

  int noteLen = ticksPerBar - (ticksPerQuarter / 8);
  unsigned char vel = 70;

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
// Strumming & Plucking Generation
//-----------------------------------------------------------------------------
int JZAutoAccompaniment::GenerateStrumPluckMeasure(
  JZTrack* pTrack,
  int barIndex,
  int barStartClock,
  int ticksPerBar,
  int ticksPerQuarter,
  int countsPerBar,
  const JZDetectedChord& chord,
  const JZStrumPluckConfig& cfg)
{
  if (!pTrack || !chord.hasNotes)
  {
    return 0;
  }

  int ch = pTrack->GetChannel() > 0 ? (pTrack->GetChannel() - 1) : (cfg.midiChannel - 1);
  int notesAdded = 0;
  int beatTicks = ticksPerQuarter;
  int eighthTicks = beatTicks / 2;

  // Build authentic guitar chord voicing (5 to 6 strings from low to high)
  int r = chord.rootPitchClass % 12;
  int rootGtr = 40 + r; // E2 (40) to D#3 (51)
  if (rootGtr < 40) rootGtr += 12;
  if (rootGtr > 50) rootGtr -= 12;

  int thirdGtr = rootGtr + 4;
  if (chord.quality == CHORD_MINOR || chord.quality == CHORD_MINOR7 || chord.quality == CHORD_DIMINISHED)
  {
    thirdGtr = rootGtr + 3;
  }
  int fifthGtr = (chord.quality == CHORD_DIMINISHED) ? (rootGtr + 6) : (rootGtr + 7);
  int seventhGtr = (chord.quality == CHORD_MAJOR7) ? (rootGtr + 11) : (rootGtr + 10);

  // 5-string voicing: Root, 5th, Octave, 3rd, 5th (or 7th)
  vector<int> chordPitches;
  chordPitches.push_back(rootGtr);
  chordPitches.push_back(fifthGtr);
  chordPitches.push_back(rootGtr + 12);
  chordPitches.push_back(thirdGtr + 12);
  if (chord.quality == CHORD_DOMINANT7 || chord.quality == CHORD_MAJOR7 || chord.quality == CHORD_MINOR7)
  {
    chordPitches.push_back(seventhGtr + 12);
  }
  else
  {
    chordPitches.push_back(fifthGtr + 12);
  }

  int strumDelay = max(2, (ticksPerQuarter * cfg.strumSpeedMs) / 500);

  // Helper lambda to emit downstroke (↓)
  auto emitDownstroke = [&](int startClk, int vel, int dur) {
    for (size_t k = 0; k < chordPitches.size(); ++k)
    {
      int clk = startClk + (int)k * strumDelay;
      int noteVel = max(30, vel - (int)k * 2);
      pTrack->Put(new JZKeyOnEvent(clk, ch, (unsigned char)chordPitches[k], (unsigned char)noteVel, (unsigned short)dur));
      notesAdded++;
    }
  };

  // Helper lambda to emit upstroke (↑)
  auto emitUpstroke = [&](int startClk, int vel, int dur) {
    size_t count = min((size_t)4, chordPitches.size()); // Upstrokes hit upper strings
    for (size_t k = 0; k < count; ++k)
    {
      size_t idx = chordPitches.size() - 1 - k;
      int clk = startClk + (int)k * strumDelay;
      int noteVel = max(30, vel - (int)k * 2);
      pTrack->Put(new JZKeyOnEvent(clk, ch, (unsigned char)chordPitches[idx], (unsigned char)noteVel, (unsigned short)dur));
      notesAdded++;
    }
  };

  // Helper lambda for muted percussive chuck (x)
  auto emitChuck = [&](int startClk) {
    for (size_t k = 0; k < chordPitches.size(); ++k)
    {
      pTrack->Put(new JZKeyOnEvent(startClk, ch, (unsigned char)chordPitches[k], 75, 35));
      notesAdded++;
    }
  };

  int ringLen = eighthTicks * 2;

  switch (cfg.style)
  {
    case STRUM_ACOUSTIC_FOLK:
    {
      // D - D U - U D - pattern
      emitDownstroke(barStartClock, 95, ringLen);
      emitDownstroke(barStartClock + beatTicks, 88, eighthTicks);
      emitUpstroke(barStartClock + beatTicks + eighthTicks, 78, eighthTicks);
      emitUpstroke(barStartClock + beatTicks * 2 + eighthTicks, 82, eighthTicks);
      emitDownstroke(barStartClock + beatTicks * 3, 90, eighthTicks);
      emitUpstroke(barStartClock + beatTicks * 3 + eighthTicks, 76, eighthTicks);
      break;
    }

    case STRUM_POP_ROCK_8TH:
    {
      // Driving 8th notes
      for (int b = 0; b < countsPerBar; ++b)
      {
        emitDownstroke(barStartClock + b * beatTicks, 96, eighthTicks);
        emitUpstroke(barStartClock + b * beatTicks + eighthTicks, 80, eighthTicks);
      }
      break;
    }

    case STRUM_FUNK_16TH_CHOP:
    {
      int sixteenth = beatTicks / 4;
      emitDownstroke(barStartClock, 102, sixteenth * 2);
      emitChuck(barStartClock + sixteenth);
      emitChuck(barStartClock + sixteenth * 2);
      emitUpstroke(barStartClock + sixteenth * 3, 85, sixteenth * 2);

      emitDownstroke(barStartClock + beatTicks, 105, sixteenth * 2);
      emitChuck(barStartClock + beatTicks + sixteenth * 2);

      emitDownstroke(barStartClock + beatTicks * 2 + sixteenth * 2, 98, sixteenth * 2);
      emitDownstroke(barStartClock + beatTicks * 3, 104, sixteenth * 2);
      break;
    }

    case STRUM_REGGAE_SKANK:
    {
      // Sharp offbeat chops
      for (int b = 0; b < countsPerBar; ++b)
      {
        emitDownstroke(barStartClock + b * beatTicks + eighthTicks, 98, eighthTicks / 2);
      }
      break;
    }

    case PLUCK_TRAVIS_PICKING:
    {
      // Thumb alternating bass + finger syncopation
      int pBass1 = chordPitches[0];
      int pBass2 = chordPitches[1];
      int pHigh1 = chordPitches[3];
      int pHigh2 = chordPitches[4];

      pTrack->Put(new JZKeyOnEvent(barStartClock, ch, (unsigned char)pBass1, 95, (unsigned short)ringLen));
      pTrack->Put(new JZKeyOnEvent(barStartClock + eighthTicks, ch, (unsigned char)pHigh1, 80, (unsigned short)eighthTicks));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks, ch, (unsigned char)pBass2, 88, (unsigned short)ringLen));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks + eighthTicks, ch, (unsigned char)pHigh2, 85, (unsigned short)eighthTicks));

      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2, ch, (unsigned char)pBass1, 92, (unsigned short)ringLen));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2 + eighthTicks, ch, (unsigned char)pHigh1, 80, (unsigned short)eighthTicks));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3, ch, (unsigned char)pBass2, 88, (unsigned short)ringLen));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3 + eighthTicks, ch, (unsigned char)pHigh2, 82, (unsigned short)eighthTicks));
      notesAdded += 8;
      break;
    }

    case PLUCK_BALLAD_ARPEGGIO:
    {
      // Flowing 8th notes broken chord
      int steps[8] = {0, 1, 2, 3, 4, 3, 2, 1};
      for (int i = 0; i < 8; ++i)
      {
        int p = chordPitches[steps[i] % chordPitches.size()];
        int vel = (i == 0 || i == 4) ? 92 : 78;
        pTrack->Put(new JZKeyOnEvent(barStartClock + i * eighthTicks, ch, (unsigned char)p, (unsigned char)vel, (unsigned short)ringLen));
        notesAdded++;
      }
      break;
    }

    case PLUCK_PIMA_CLASSICAL:
    {
      // P (Root), I (3rd), M (5th), A (High)
      int pP = chordPitches[0];
      int pI = chordPitches[2];
      int pM = chordPitches[3];
      int pA = chordPitches[4];

      for (int cycle = 0; cycle < 2; ++cycle)
      {
        int offset = cycle * beatTicks * 2;
        pTrack->Put(new JZKeyOnEvent(barStartClock + offset, ch, (unsigned char)pP, 94, (unsigned short)ringLen));
        pTrack->Put(new JZKeyOnEvent(barStartClock + offset + eighthTicks, ch, (unsigned char)pI, 80, (unsigned short)ringLen));
        pTrack->Put(new JZKeyOnEvent(barStartClock + offset + beatTicks, ch, (unsigned char)pM, 84, (unsigned short)ringLen));
        pTrack->Put(new JZKeyOnEvent(barStartClock + offset + beatTicks + eighthTicks, ch, (unsigned char)pA, 88, (unsigned short)ringLen));
        notesAdded += 4;
      }
      break;
    }

    case PLUCK_NEO_SOUL:
    {
      // Spread voicings with gentle delay and release
      emitDownstroke(barStartClock, 92, beatTicks * 2);
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2 + eighthTicks, ch, (unsigned char)chordPitches[4], 84, (unsigned short)eighthTicks));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3, ch, (unsigned char)chordPitches[3], 80, (unsigned short)eighthTicks));
      notesAdded += 2;
      break;
    }

    default:
      break;
  }

  return notesAdded;
}

//-----------------------------------------------------------------------------
// Riff & Melodic Hook Generation
//-----------------------------------------------------------------------------
int JZAutoAccompaniment::GenerateRiffMeasure(
  JZTrack* pTrack,
  int barIndex,
  int barStartClock,
  int ticksPerBar,
  int ticksPerQuarter,
  int countsPerBar,
  const JZDetectedChord& chord,
  const JZRiffConfig& cfg)
{
  if (!pTrack || !chord.hasNotes)
  {
    return 0;
  }

  int ch = pTrack->GetChannel() > 0 ? (pTrack->GetChannel() - 1) : (cfg.midiChannel - 1);
  int notesAdded = 0;
  int beatTicks = ticksPerQuarter;
  int eighthTicks = beatTicks / 2;
  int sixteenthTicks = beatTicks / 4;

  int baseOct = max(2, min(5, cfg.octave));
  int root = (baseOct * 12) + (chord.rootPitchClass % 12);

  int min3 = root + 3;
  int maj3 = root + 4;
  int fourth = root + 5;
  int dim5 = root + 6;
  int fifth = root + 7;
  int min7 = root + 10;
  int octave = root + 12;

  switch (cfg.style)
  {
    case RIFF_BLUES_ROCK:
    {
      // Driving Pentatonic Boogie Hook
      pTrack->Put(new JZKeyOnEvent(barStartClock, ch, (unsigned char)root, 102, (unsigned short)(eighthTicks * 0.9)));
      pTrack->Put(new JZKeyOnEvent(barStartClock + eighthTicks, ch, (unsigned char)root, 90, (unsigned short)(eighthTicks * 0.9)));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks, ch, (unsigned char)min3, 98, (unsigned short)(eighthTicks * 0.8)));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks + eighthTicks, ch, (unsigned char)fourth, 92, (unsigned short)(eighthTicks * 0.8)));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2, ch, (unsigned char)dim5, 96, (unsigned short)(eighthTicks * 0.7)));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2 + eighthTicks, ch, (unsigned char)fifth, 100, (unsigned short)(eighthTicks * 0.9)));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3, ch, (unsigned char)min7, 95, (unsigned short)(eighthTicks * 0.85)));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3 + eighthTicks, ch, (unsigned char)octave, 104, (unsigned short)(eighthTicks * 0.9)));
      notesAdded += 8;
      break;
    }

    case RIFF_FUNK_BRASS:
    {
      // Sharp Brass Section Stabs (triad chords on offbeats)
      auto emitBrassStab = [&](int clk, int vel) {
        int len = sixteenthTicks * 2;
        pTrack->Put(new JZKeyOnEvent(clk, ch, (unsigned char)root, (unsigned char)vel, (unsigned short)len));
        pTrack->Put(new JZKeyOnEvent(clk, ch, (unsigned char)fifth, (unsigned char)(vel - 4), (unsigned short)len));
        pTrack->Put(new JZKeyOnEvent(clk, ch, (unsigned char)octave, (unsigned char)(vel - 2), (unsigned short)len));
        notesAdded += 3;
      };

      emitBrassStab(barStartClock + eighthTicks, 105);
      emitBrassStab(barStartClock + beatTicks + eighthTicks, 108);
      emitBrassStab(barStartClock + beatTicks * 3, 112);
      break;
    }

    case RIFF_HARD_ROCK_POWER:
    {
      // Palm-muted root pedal with power chord hits
      auto emitPowerChord = [&](int clk, int vel, int len) {
        pTrack->Put(new JZKeyOnEvent(clk, ch, (unsigned char)root, (unsigned char)vel, (unsigned short)len));
        pTrack->Put(new JZKeyOnEvent(clk, ch, (unsigned char)fifth, (unsigned char)(vel - 2), (unsigned short)len));
        pTrack->Put(new JZKeyOnEvent(clk, ch, (unsigned char)octave, (unsigned char)(vel - 3), (unsigned short)len));
        notesAdded += 3;
      };

      emitPowerChord(barStartClock, 110, beatTicks);
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks, ch, (unsigned char)root, 88, (unsigned short)eighthTicks));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks + eighthTicks, ch, (unsigned char)root, 88, (unsigned short)eighthTicks));
      notesAdded += 2;

      emitPowerChord(barStartClock + beatTicks * 2 + eighthTicks, 108, eighthTicks);
      emitPowerChord(barStartClock + beatTicks * 3, 112, beatTicks);
      break;
    }

    case RIFF_SYNTHWAVE_BASS:
    {
      // 16th rolling synth bass hook
      int pat[16] = {0, 12, 7, 12, 10, 12, 7, 12, 0, 12, 7, 12, 10, 12, 12, 7};
      for (int i = 0; i < 16; ++i)
      {
        int p = root + pat[i];
        int vel = (i % 4 == 0) ? 104 : 85;
        pTrack->Put(new JZKeyOnEvent(barStartClock + i * sixteenthTicks, ch, (unsigned char)p, (unsigned char)vel, (unsigned short)(sixteenthTicks * 0.9)));
        notesAdded++;
      }
      break;
    }

    case RIFF_JAZZ_BEBOP_MOTIF:
    {
      // Chromatic enclosure motif leading to chord root
      int encUpper = root + 14;
      int encLower = root + 11;
      int encPassing = root + 13;

      pTrack->Put(new JZKeyOnEvent(barStartClock + eighthTicks, ch, (unsigned char)encUpper, 95, (unsigned short)eighthTicks));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks, ch, (unsigned char)encPassing, 90, (unsigned short)eighthTicks));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks + eighthTicks, ch, (unsigned char)encLower, 92, (unsigned short)eighthTicks));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2, ch, (unsigned char)octave, 108, (unsigned short)(beatTicks * 1.5)));
      notesAdded += 4;
      break;
    }

    default:
      break;
  }

  return notesAdded;
}

//-----------------------------------------------------------------------------
// Auto-Solo / Lead Improviser Generation
//-----------------------------------------------------------------------------
int JZAutoAccompaniment::GenerateSoloMeasure(
  JZTrack* pTrack,
  int barIndex,
  int barStartClock,
  int ticksPerBar,
  int ticksPerQuarter,
  int countsPerBar,
  const JZDetectedChord& chord,
  const JZSoloConfig& cfg)
{
  if (!pTrack || !chord.hasNotes)
  {
    return 0;
  }

  int ch = pTrack->GetChannel() > 0 ? (pTrack->GetChannel() - 1) : (cfg.midiChannel - 1);
  int notesAdded = 0;
  int beatTicks = ticksPerQuarter;
  int eighthTicks = beatTicks / 2;
  int swingSkip = (beatTicks * 2) / 3;

  int baseOct = max(4, min(6, cfg.octave));
  int root = (baseOct * 12) + (chord.rootPitchClass % 12);

  // Build chord-scale tones pool
  vector<int> scale;
  if (chord.quality == CHORD_MINOR || chord.quality == CHORD_MINOR7)
  {
    // Dorian / Minor Pentatonic
    int degs[] = {0, 2, 3, 5, 7, 9, 10, 12, 14, 15};
    for (int d : degs) scale.push_back(root + d);
  }
  else if (chord.quality == CHORD_DOMINANT7)
  {
    // Mixolydian with Blue Note
    int degs[] = {0, 2, 4, 5, 6, 7, 10, 12, 14, 16};
    for (int d : degs) scale.push_back(root + d);
  }
  else if (chord.quality == CHORD_DIMINISHED)
  {
    int degs[] = {0, 3, 6, 9, 12, 15};
    for (int d : degs) scale.push_back(root + d);
  }
  else
  {
    // Major Pentatonic / Ionian
    int degs[] = {0, 2, 4, 7, 9, 12, 14, 16};
    for (int d : degs) scale.push_back(root + d);
  }

  // Phrasing: 2-measure Call-and-Response structure
  bool isCallMeasure = (barIndex % 2 == 0);

  if (isCallMeasure)
  {
    // Call: Ascending melodic line, starts on offbeat
    int nNotes = (cfg.density == SOLO_DENSITY_SPARSE) ? 3 : (cfg.density == SOLO_DENSITY_VIRTUOSO ? 6 : 4);
    for (int i = 0; i < nNotes; ++i)
    {
      int clk = barStartClock + eighthTicks + (i * eighthTicks);
      int scaleIdx = min((size_t)(i + 1), scale.size() - 1);
      int p = scale[scaleIdx];
      int vel = 90 + (i * 4);
      int len = (int)(eighthTicks * 0.9);
      pTrack->Put(new JZKeyOnEvent(clk, ch, (unsigned char)p, (unsigned char)vel, (unsigned short)len));
      notesAdded++;
    }
  }
  else
  {
    // Response: Resolves into strong chord tone (Root or 5th or 3rd), leaves resting space
    int resolvePitch = scale[0] + 12; // Octave Root
    if (cfg.style == SOLO_BLUESY_SOULFUL)
    {
      // Blue note slide into 5th
      pTrack->Put(new JZKeyOnEvent(barStartClock, ch, (unsigned char)(root + 6), 92, (unsigned short)(eighthTicks / 2)));
      pTrack->Put(new JZKeyOnEvent(barStartClock + (eighthTicks / 2), ch, (unsigned char)(root + 7), 108, (unsigned short)(beatTicks * 1.5)));
      notesAdded += 2;
    }
    else
    {
      pTrack->Put(new JZKeyOnEvent(barStartClock, ch, (unsigned char)resolvePitch, 112, (unsigned short)(beatTicks * 2)));
      notesAdded++;
    }

    if (cfg.density == SOLO_DENSITY_VIRTUOSO)
    {
      // Additional turnaround lick on beat 3 & 4
      int p3 = scale[3];
      int p2 = scale[2];
      int pRoot = scale[0];
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2 + eighthTicks, ch, (unsigned char)p3, 95, (unsigned short)eighthTicks));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3, ch, (unsigned char)p2, 90, (unsigned short)eighthTicks));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3 + eighthTicks, ch, (unsigned char)pRoot, 98, (unsigned short)eighthTicks));
      notesAdded += 3;
    }
  }

  return notesAdded;
}

//-----------------------------------------------------------------------------
// Unified Master Generation
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

  int lastSongClock = pSong->GetLastClock();
  if (lastSongClock <= 0)
  {
    res.message = "Source track has no notes. Please record or input notes on the piano track first.";
    return res;
  }

  JZBarInfo tempBarInfo(*pSong);
  tempBarInfo.SetClock(lastSongClock);
  int songTotalBars = tempBarInfo.GetBarIndex() + 1;

  int startBar = max(0, config.fromBar);
  int endBar = (config.toBar >= 0) ? min(config.toBar, songTotalBars) : songTotalBars;
  if (endBar <= startBar) endBar = startBar + 1;

  // Determine what targets are active based on mode
  bool doBass = (config.mode == MODE_FULL_BAND || config.mode == MODE_ALL_SIMULTANEOUS) && config.generateBass;
  bool doDrums = (config.mode == MODE_FULL_BAND || config.mode == MODE_ALL_SIMULTANEOUS) && config.generateDrums;
  bool doPad = (config.mode == MODE_FULL_BAND || config.mode == MODE_ALL_SIMULTANEOUS) && config.generatePad;
  bool doStrum = (config.mode == MODE_STRUM_PLUCK || config.mode == MODE_ALL_SIMULTANEOUS);
  bool doRiff = (config.mode == MODE_RIFF_MAKER || config.mode == MODE_ALL_SIMULTANEOUS);
  bool doSolo = (config.mode == MODE_AUTO_SOLO || config.mode == MODE_ALL_SIMULTANEOUS);

  JZTrack* pBassTrack = doBass ? pSong->GetTrack(config.bassTrackIndex) : 0;
  JZTrack* pDrumsTrack = doDrums ? pSong->GetTrack(config.drumsTrackIndex) : 0;
  JZTrack* pPadTrack = doPad ? pSong->GetTrack(config.padTrackIndex) : 0;
  JZTrack* pStrumTrack = doStrum ? pSong->GetTrack(config.strumPluck.targetTrackIndex) : 0;
  JZTrack* pRiffTrack = doRiff ? pSong->GetTrack(config.riff.targetTrackIndex) : 0;
  JZTrack* pSoloTrack = doSolo ? pSong->GetTrack(config.solo.targetTrackIndex) : 0;

  // Initialize track properties
  if (pBassTrack)
  {
    if (pBassTrack->IsEmpty() || string(pBassTrack->GetName()).empty()) pBassTrack->SetName("Auto Bass");
    pBassTrack->SetChannel(2);
    pBassTrack->SetPatch((config.style == STYLE_FUNK_GROOVE || config.style == STYLE_POP_ROCK) ? 34 : 33);
  }
  if (pDrumsTrack)
  {
    if (pDrumsTrack->IsEmpty() || string(pDrumsTrack->GetName()).empty()) pDrumsTrack->SetName("Auto Drums");
    pDrumsTrack->SetChannel(10);
  }
  if (pPadTrack)
  {
    if (pPadTrack->IsEmpty() || string(pPadTrack->GetName()).empty()) pPadTrack->SetName("Auto Pad");
    pPadTrack->SetChannel(3);
    pPadTrack->SetPatch((config.style == STYLE_BOSSA_NOVA) ? 90 : 49);
  }
  if (pStrumTrack)
  {
    if (pStrumTrack->IsEmpty() || string(pStrumTrack->GetName()).empty()) pStrumTrack->SetName("Auto Strum/Pluck");
    pStrumTrack->SetChannel(config.strumPluck.midiChannel);
    pStrumTrack->SetPatch(config.strumPluck.patchNumber);
  }
  if (pRiffTrack)
  {
    if (pRiffTrack->IsEmpty() || string(pRiffTrack->GetName()).empty()) pRiffTrack->SetName("Auto Riff");
    pRiffTrack->SetChannel(config.riff.midiChannel);
    pRiffTrack->SetPatch(config.riff.patchNumber);
  }
  if (pSoloTrack)
  {
    if (pSoloTrack->IsEmpty() || string(pSoloTrack->GetName()).empty()) pSoloTrack->SetName("Auto Solo Lead");
    pSoloTrack->SetChannel(config.solo.midiChannel);
    pSoloTrack->SetPatch(config.solo.patchNumber);
  }

  // Pre-analyze chords for all bars in range
  vector<JZDetectedChord> chords;
  chords.reserve(endBar - startBar + 1);

  JZDetectedChord currentChord;
  currentChord.rootPitchClass = 0;
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

  bool anyNotesFound = false;
  for (size_t i = 0; i < chords.size(); ++i)
  {
    if (chords[i].hasNotes) { anyNotesFound = true; break; }
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
    if (pStrumTrack) ClearRange(pStrumTrack, fromClock, toClock);
    if (pRiffTrack) ClearRange(pRiffTrack, fromClock, toClock);
    if (pSoloTrack) ClearRange(pSoloTrack, fromClock, toClock);
  }

  // Generation loop
  JZBarInfo genIter(*pSong);
  genIter.SetBar(startBar);

  int totalBass = 0, totalDrums = 0, totalPad = 0;
  int totalStrum = 0, totalRiff = 0, totalSolo = 0;

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
    if (pStrumTrack)
    {
      totalStrum += GenerateStrumPluckMeasure(pStrumTrack, b, bStart, bTicks, tpq, bCounts, ch, config.strumPluck);
    }
    if (pRiffTrack)
    {
      totalRiff += GenerateRiffMeasure(pRiffTrack, b, bStart, bTicks, tpq, bCounts, ch, config.riff);
    }
    if (pSoloTrack)
    {
      totalSolo += GenerateSoloMeasure(pSoloTrack, b, bStart, bTicks, tpq, bCounts, ch, config.solo);
    }

    genIter.Next();
  }

  if (pBassTrack) pBassTrack->Cleanup();
  if (pDrumsTrack) pDrumsTrack->Cleanup();
  if (pPadTrack) pPadTrack->Cleanup();
  if (pStrumTrack) pStrumTrack->Cleanup();
  if (pRiffTrack) pRiffTrack->Cleanup();
  if (pSoloTrack) pSoloTrack->Cleanup();

  res.success = true;
  res.barsProcessed = endBar - startBar;
  res.bassNotesGenerated = totalBass;
  res.drumNotesGenerated = totalDrums;
  res.padNotesGenerated = totalPad;
  res.strumNotesGenerated = totalStrum;
  res.riffNotesGenerated = totalRiff;
  res.soloNotesGenerated = totalSolo;

  ostringstream oss;
  oss << "Generated music intelligence layers for " << res.barsProcessed << " measure(s):\n";
  if (totalBass > 0) oss << " • Bass: " << totalBass << " notes (Track " << (config.bassTrackIndex + 1) << ")\n";
  if (totalDrums > 0) oss << " • Drums: " << totalDrums << " notes (Track " << (config.drumsTrackIndex + 1) << ")\n";
  if (totalPad > 0) oss << " • Strings/Pad: " << totalPad << " notes (Track " << (config.padTrackIndex + 1) << ")\n";
  if (totalStrum > 0) oss << " • Strum/Pluck: " << totalStrum << " notes (Track " << (config.strumPluck.targetTrackIndex + 1) << ")\n";
  if (totalRiff > 0) oss << " • Riffs/Hooks: " << totalRiff << " notes (Track " << (config.riff.targetTrackIndex + 1) << ")\n";
  if (totalSolo > 0) oss << " • Solo/Lead: " << totalSolo << " notes (Track " << (config.solo.targetTrackIndex + 1) << ")\n";

  res.message = oss.str();
  return res;
}
