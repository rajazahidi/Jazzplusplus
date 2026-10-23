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
string JZDetectedChord::GetChordName(bool second) const
{
  if (!hasNotes)
  {
    return "N.C.";
  }

  int r = second ? rootPitchClass2 : rootPitchClass;
  int b = second ? bassPitchClass2 : bassPitchClass;
  int inv = second ? inversion2 : inversion;
  JZChordQuality q = second ? quality2 : quality;

  string name = sPitchNames[r % 12];
  switch (q)
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

  // Slash chord notation for inversions (e.g. C/E, G/B, C/G)
  if (inv > 0 && (b % 12) != (r % 12))
  {
    name += "/";
    name += sPitchNames[b % 12];
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
// Helper to analyze notes in a clock window (with Inversion Disambiguation)
//-----------------------------------------------------------------------------
bool JZAutoAccompaniment::AnalyzeWindow(
  JZTrack* pTrack,
  int startClock,
  int endClock,
  int& rootPc,
  int& bassPc,
  int& inversion,
  JZChordQuality& quality,
  int& minP)
{
  if (!pTrack)
  {
    return false;
  }

  bool pcPresent[12] = {false};
  int noteCount = 0;
  int minPitch = 128;

  for (int i = 0; i < pTrack->mEventCount; ++i)
  {
    JZEvent* pEvent = pTrack->mppEvents[i];
    if (!pEvent || pEvent->IsKilled())
    {
      continue;
    }

    JZKeyOnEvent* pKeyOn = pEvent->IsKeyOn();
    if (!pKeyOn)
    {
      continue;
    }

    int clk = pKeyOn->GetClock();
    if (clk >= startClock && clk < endClock)
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
    return false;
  }

  minP = minPitch;
  bassPc = minPitch % 12;

  // Find candidate root among present pitch classes
  // A true root has standard intervals: 3rd (3 or 4) and 5th (6 or 7)
  int bestRoot = bassPc;
  JZChordQuality bestQuality = CHORD_MAJOR;
  int bestScore = -1;

  for (int candidate = 0; candidate < 12; ++candidate)
  {
    if (!pcPresent[candidate])
    {
      continue;
    }

    bool hasMin3 = pcPresent[(candidate + 3) % 12];
    bool hasMaj3 = pcPresent[(candidate + 4) % 12];
    bool hasDim5 = pcPresent[(candidate + 6) % 12];
    bool hasP5   = pcPresent[(candidate + 7) % 12];
    bool hasMin7 = pcPresent[(candidate + 10) % 12];
    bool hasMaj7 = pcPresent[(candidate + 11) % 12];
    bool hasSus4 = pcPresent[(candidate + 5) % 12];

    int score = 0;
    JZChordQuality q = CHORD_MAJOR;

    if (hasMin3 && !hasMaj3)
    {
      score += 3;
      if (hasDim5) { q = CHORD_DIMINISHED; score += 2; }
      else if (hasMin7) { q = CHORD_MINOR7; score += 3; }
      else { q = CHORD_MINOR; score += 2; }
      if (hasP5) score += 2;
    }
    else if (hasMaj3)
    {
      score += 3;
      if (hasMin7) { q = CHORD_DOMINANT7; score += 3; }
      else if (hasMaj7) { q = CHORD_MAJOR7; score += 3; }
      else { q = CHORD_MAJOR; score += 2; }
      if (hasP5) score += 2;
    }
    else if (hasSus4 && !hasMin3)
    {
      q = CHORD_SUS4;
      score += 2;
      if (hasP5) score += 2;
    }
    else if (hasP5)
    {
      // Power chord (Root + 5th)
      score += 2;
      q = CHORD_MAJOR;
    }

    // Bias slightly towards the actual bass note if tied
    if (candidate == bassPc)
    {
      score += 1;
    }

    if (score > bestScore)
    {
      bestScore = score;
      bestRoot = candidate;
      bestQuality = q;
    }
  }

  rootPc = bestRoot;
  quality = bestQuality;

  // Determine Inversion
  if (bassPc == rootPc)
  {
    inversion = 0; // Root position
  }
  else
  {
    int intervalFromRoot = (bassPc - rootPc + 12) % 12;
    if (intervalFromRoot == 3 || intervalFromRoot == 4)
    {
      inversion = 1; // 1st Inversion (3rd in bass, e.g. C/E)
    }
    else if (intervalFromRoot == 6 || intervalFromRoot == 7)
    {
      inversion = 2; // 2nd Inversion (5th in bass, e.g. C/G)
    }
    else if (intervalFromRoot == 10 || intervalFromRoot == 11)
    {
      inversion = 3; // 3rd Inversion (7th in bass, e.g. G7/F)
    }
    else
    {
      inversion = 1; // Generic slash chord
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
// Measure Analysis (With Half-Bar Split Detection)
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

  int barLen = barEndClock - barStartClock;
  int midClock = barStartClock + (barLen / 2);

  int r1 = 0, b1 = 0, inv1 = 0, minP1 = 60;
  JZChordQuality q1 = CHORD_MAJOR;
  bool hasHalf1 = AnalyzeWindow(pSourceTrack, barStartClock, midClock, r1, b1, inv1, q1, minP1);

  int r2 = 0, b2 = 0, inv2 = 0, minP2 = 60;
  JZChordQuality q2 = CHORD_MAJOR;
  bool hasHalf2 = AnalyzeWindow(pSourceTrack, midClock, barEndClock, r2, b2, inv2, q2, minP2);

  if (!hasHalf1 && !hasHalf2)
  {
    // Check if entire bar has sparse notes spanning across boundary
    int rAll = 0, bAll = 0, invAll = 0, minPAll = 60;
    JZChordQuality qAll = CHORD_MAJOR;
    if (AnalyzeWindow(pSourceTrack, barStartClock, barEndClock, rAll, bAll, invAll, qAll, minPAll))
    {
      result.hasNotes = true;
      result.rootPitchClass = rAll;
      result.bassPitchClass = bAll;
      result.inversion = invAll;
      result.quality = qAll;
      result.minPitch = minPAll;
      return result;
    }
    return fallbackChord;
  }

  if (hasHalf1)
  {
    result.hasNotes = true;
    result.rootPitchClass = r1;
    result.bassPitchClass = b1;
    result.inversion = inv1;
    result.quality = q1;
    result.minPitch = minP1;

    // Check if second half has a different chord (split-measure ii-V etc.)
    if (hasHalf2 && (r2 != r1 || q2 != q1 || b2 != b1))
    {
      result.hasSecondChord = true;
      result.rootPitchClass2 = r2;
      result.bassPitchClass2 = b2;
      result.inversion2 = inv2;
      result.quality2 = q2;
    }
  }
  else if (hasHalf2)
  {
    // Only second half had notes
    result.hasNotes = true;
    result.rootPitchClass = r2;
    result.bassPitchClass = b2;
    result.inversion = inv2;
    result.quality = q2;
    result.minPitch = minP2;
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
// Bass Generation (Supports Slash Inversions & Split Half-Bar Chords)
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
  const JZAccompanimentConfig& config)
{
  if (!pTrack || !chord.hasNotes)
  {
    return 0;
  }

  int ch = pTrack->GetChannel() > 0 ? (pTrack->GetChannel() - 1) : 1;
  int notesAdded = 0;
  int beatTicks = ticksPerQuarter;

  // Helper for humanized velocity drift
  auto hVel = [&](int base, int step) -> unsigned char {
    if (!config.enableHumanizeVelocity) return (unsigned char)base;
    int drift = ((barIndex * 5 + step * 3) % 9) - 4;
    return (unsigned char)max(20, min(127, base + drift));
  };

  // Chord 1 Root & Bass
  int rootBass1 = 36 + (chord.rootPitchClass % 12);
  int bassNote1 = 36 + (chord.bassPitchClass % 12);
  int thirdBass1 = rootBass1 + ((chord.quality == CHORD_MINOR || chord.quality == CHORD_MINOR7 || chord.quality == CHORD_DIMINISHED) ? 3 : 4);
  int fifthBass1 = rootBass1 + ((chord.quality == CHORD_DIMINISHED) ? 6 : 7);

  // If slash chord inversion (e.g. C/E), use bassNote on beat 1
  int downbeatPitch1 = (chord.inversion > 0) ? bassNote1 : rootBass1;

  // Chord 2 (for split-measure) or Next Bar Root
  int nextRoot = chord.hasSecondChord ? (36 + chord.rootPitchClass2 % 12) : (36 + nextChord.rootPitchClass % 12);
  int approachBass = (nextRoot > rootBass1) ? (nextRoot - 1) : (nextRoot + 1);

  if (chord.hasSecondChord)
  {
    // Split Bar: 2 beats for Chord 1, 2 beats for Chord 2
    int rootBass2 = 36 + (chord.rootPitchClass2 % 12);
    int bassNote2 = 36 + (chord.bassPitchClass2 % 12);
    int downbeatPitch2 = (chord.inversion2 > 0) ? bassNote2 : rootBass2;
    int nextBarRoot = 36 + (nextChord.rootPitchClass % 12);
    int approach2 = (nextBarRoot > rootBass2) ? (nextBarRoot - 1) : (nextBarRoot + 1);

    int len = (int)(beatTicks * 0.9);
    pTrack->Put(new JZKeyOnEvent(barStartClock, ch, (unsigned char)downbeatPitch1, hVel(95, 0), (unsigned short)len));
    pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks, ch, (unsigned char)fifthBass1, hVel(86, 1), (unsigned short)len));

    pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2, ch, (unsigned char)downbeatPitch2, hVel(94, 2), (unsigned short)len));
    pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3, ch, (unsigned char)approach2, hVel(85, 3), (unsigned short)len));
    return 4;
  }

  switch (config.style)
  {
    case STYLE_JAZZ_SWING:
    {
      if (countsPerBar == 4)
      {
        int len = (int)(beatTicks * 0.9);
        pTrack->Put(new JZKeyOnEvent(barStartClock, ch, (unsigned char)downbeatPitch1, hVel(96, 0), (unsigned short)len));
        int b2Pitch = (barIndex % 2 == 0) ? thirdBass1 : fifthBass1;
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks, ch, (unsigned char)b2Pitch, hVel(84, 1), (unsigned short)len));
        int b3Pitch = (barIndex % 2 == 0) ? fifthBass1 : (rootBass1 + 12);
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2, ch, (unsigned char)b3Pitch, hVel(90, 2), (unsigned short)len));
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3, ch, (unsigned char)approachBass, hVel(86, 3), (unsigned short)(beatTicks * 0.85)));
        notesAdded += 4;
      }
      else
      {
        int len = (int)(beatTicks * 0.9);
        pTrack->Put(new JZKeyOnEvent(barStartClock, ch, (unsigned char)downbeatPitch1, hVel(95, 0), (unsigned short)len));
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks, ch, (unsigned char)thirdBass1, hVel(84, 1), (unsigned short)len));
        pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2, ch, (unsigned char)fifthBass1, hVel(88, 2), (unsigned short)len));
        notesAdded += 3;
      }
      break;
    }

    case STYLE_BOSSA_NOVA:
    {
      int len1 = (int)(beatTicks * 1.35);
      int len2 = (int)(beatTicks * 0.45);
      pTrack->Put(new JZKeyOnEvent(barStartClock, ch, (unsigned char)downbeatPitch1, hVel(92, 0), (unsigned short)len1));
      pTrack->Put(new JZKeyOnEvent(barStartClock + (int)(beatTicks * 1.5), ch, (unsigned char)fifthBass1, hVel(82, 1), (unsigned short)len2));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2, ch, (unsigned char)downbeatPitch1, hVel(88, 2), (unsigned short)len1));
      pTrack->Put(new JZKeyOnEvent(barStartClock + (int)(beatTicks * 3.5), ch, (unsigned char)fifthBass1, hVel(80, 3), (unsigned short)len2));
      notesAdded += 4;
      break;
    }

    case STYLE_BLUES_SHUFFLE:
    {
      int sixthBass = rootBass1 + 9;
      int len = (int)(beatTicks * 0.85);
      pTrack->Put(new JZKeyOnEvent(barStartClock, ch, (unsigned char)downbeatPitch1, hVel(95, 0), (unsigned short)len));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks, ch, (unsigned char)thirdBass1, hVel(85, 1), (unsigned short)len));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2, ch, (unsigned char)fifthBass1, hVel(90, 2), (unsigned short)len));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3, ch, (unsigned char)sixthBass, hVel(85, 3), (unsigned short)len));
      notesAdded += 4;
      break;
    }

    case STYLE_POP_ROCK:
    {
      int eighthTicks = beatTicks / 2;
      int len = (int)(eighthTicks * 0.85);
      for (int b = 0; b < countsPerBar; ++b)
      {
        int pDown = (b == 0) ? downbeatPitch1 : ((b == 2 && barIndex % 2 == 1) ? fifthBass1 : rootBass1);
        int pUp = (b == 3 && barIndex % 4 == 3) ? approachBass : pDown;
        pTrack->Put(new JZKeyOnEvent(barStartClock + b * beatTicks, ch, (unsigned char)pDown, hVel(95, b), (unsigned short)len));
        pTrack->Put(new JZKeyOnEvent(barStartClock + b * beatTicks + eighthTicks, ch, (unsigned char)pUp, hVel(80, b + 4), (unsigned short)len));
        notesAdded += 2;
      }
      break;
    }

    case STYLE_FUNK_GROOVE:
    {
      int sixteenth = beatTicks / 4;
      pTrack->Put(new JZKeyOnEvent(barStartClock, ch, (unsigned char)downbeatPitch1, hVel(105, 0), (unsigned short)(sixteenth * 0.9)));
      pTrack->Put(new JZKeyOnEvent(barStartClock + sixteenth * 3, ch, (unsigned char)(rootBass1 + 12), hVel(92, 1), (unsigned short)(sixteenth * 0.9)));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks + sixteenth * 2, ch, (unsigned char)thirdBass1, hVel(88, 2), (unsigned short)(sixteenth * 0.9)));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2 + sixteenth, ch, (unsigned char)fifthBass1, hVel(96, 3), (unsigned short)(beatTicks * 0.7)));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3, ch, (unsigned char)rootBass1, hVel(98, 4), (unsigned short)(sixteenth * 0.9)));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3 + sixteenth * 3, ch, (unsigned char)approachBass, hVel(85, 5), (unsigned short)(sixteenth * 0.9)));
      notesAdded += 6;
      break;
    }

    default:
      break;
  }

  return notesAdded;
}

//-----------------------------------------------------------------------------
// Drums Generation (Smart Drum Fills on Measure 4/8/12/16 & Swing Control)
//-----------------------------------------------------------------------------
int JZAutoAccompaniment::GenerateDrumsMeasure(
  JZTrack* pTrack,
  int barIndex,
  int barStartClock,
  int ticksPerBar,
  int ticksPerQuarter,
  int countsPerBar,
  const JZAccompanimentConfig& config)
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

  // Swing ratio calculation
  int swingSkip = (beatTicks * config.swingPercent) / 100;
  int hitLen = 40;

  const unsigned char KICK = 36;
  const unsigned char SNARE = 38;
  const unsigned char RIMSHOT = 37;
  const unsigned char CLOSED_HH = 42;
  const unsigned char PEDAL_HH = 44;
  const unsigned char OPEN_HH = 46;
  const unsigned char CRASH = 49;
  const unsigned char RIDE = 51;
  const unsigned char HIGH_TOM = 50;
  const unsigned char MID_TOM = 47;
  const unsigned char LOW_TOM = 45;
  const unsigned char FLOOR_TOM = 41;

  bool isFillBar = config.enableDrumFills && (countsPerBar == 4) && (barIndex % 4 == 3);

  // Crash on start of song and on section starts (every 4 bars)
  if (barIndex % 4 == 0)
  {
    pTrack->Put(new JZKeyOnEvent(barStartClock, ch, CRASH, 100, (unsigned short)hitLen));
    pTrack->Put(new JZKeyOnEvent(barStartClock, ch, KICK, 105, (unsigned short)hitLen));
    notesAdded += 2;
  }

  if (isFillBar)
  {
    // Dynamic Procedural Drum Fill on beats 3 & 4!
    // Beats 1 & 2: Keep beat solid
    pTrack->Put(new JZKeyOnEvent(barStartClock, ch, KICK, 95, (unsigned short)hitLen));
    pTrack->Put(new JZKeyOnEvent(barStartClock, ch, CLOSED_HH, 85, (unsigned short)hitLen));
    pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks, ch, SNARE, 100, (unsigned short)hitLen));
    pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks, ch, CLOSED_HH, 85, (unsigned short)hitLen));
    notesAdded += 4;

    // Beat 3: Snare 16ths into High Tom
    pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2, ch, SNARE, 90, (unsigned short)hitLen));
    pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2 + sixteenthTicks, ch, SNARE, 96, (unsigned short)hitLen));
    pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2 + sixteenthTicks * 2, ch, HIGH_TOM, 100, (unsigned short)hitLen));
    pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2 + sixteenthTicks * 3, ch, HIGH_TOM, 105, (unsigned short)hitLen));

    // Beat 4: Cascading Toms down to floor tom + kick flam
    pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3, ch, MID_TOM, 102, (unsigned short)hitLen));
    pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3 + sixteenthTicks, ch, LOW_TOM, 106, (unsigned short)hitLen));
    pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3 + sixteenthTicks * 2, ch, FLOOR_TOM, 110, (unsigned short)hitLen));
    pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3 + sixteenthTicks * 3, ch, SNARE, 115, (unsigned short)hitLen));
    pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3 + sixteenthTicks * 3, ch, KICK, 110, (unsigned short)hitLen));
    notesAdded += 9;
    return notesAdded;
  }

  switch (config.style)
  {
    case STYLE_JAZZ_SWING:
    {
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
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2, ch, KICK, 80, (unsigned short)hitLen));
      pTrack->Put(new JZKeyOnEvent(barStartClock + (int)(beatTicks * 3.5), ch, KICK, 75, (unsigned short)hitLen));
      pTrack->Put(new JZKeyOnEvent(barStartClock, ch, RIMSHOT, 85, (unsigned short)hitLen));
      pTrack->Put(new JZKeyOnEvent(barStartClock + (int)(beatTicks * 1.5), ch, RIMSHOT, 88, (unsigned short)hitLen));
      pTrack->Put(new JZKeyOnEvent(barStartClock + (int)(beatTicks * 2.5), ch, RIMSHOT, 86, (unsigned short)hitLen));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3, ch, RIMSHOT, 84, (unsigned short)hitLen));
      notesAdded += 8;
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
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks, ch, SNARE, 100, (unsigned short)hitLen));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3, ch, SNARE, 100, (unsigned short)hitLen));
      notesAdded += 2;
      break;
    }

    case STYLE_POP_ROCK:
    {
      for (int i = 0; i < countsPerBar * 2; ++i)
      {
        int vel = (i % 2 == 0) ? 85 : 68;
        pTrack->Put(new JZKeyOnEvent(barStartClock + i * eighthTicks, ch, CLOSED_HH, (unsigned char)vel, (unsigned short)hitLen));
        notesAdded++;
      }
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks, ch, SNARE, 102, (unsigned short)hitLen));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3, ch, SNARE, 105, (unsigned short)hitLen));
      pTrack->Put(new JZKeyOnEvent(barStartClock, ch, KICK, 100, (unsigned short)hitLen));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2, ch, KICK, 95, (unsigned short)hitLen));
      notesAdded += 4;
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
// Pad / Strings Generation (Supports Split Half-Bar Chords)
//-----------------------------------------------------------------------------
int JZAutoAccompaniment::GeneratePadMeasure(
  JZTrack* pTrack,
  int barIndex,
  int barStartClock,
  int ticksPerBar,
  int ticksPerQuarter,
  int countsPerBar,
  const JZDetectedChord& chord,
  const JZAccompanimentConfig& config)
{
  if (!pTrack || !chord.hasNotes)
  {
    return 0;
  }

  int ch = pTrack->GetChannel() > 0 ? (pTrack->GetChannel() - 1) : 2;
  int notesAdded = 0;

  auto emitPadVoicing = [&](int clk, int r, JZChordQuality q, int dur, unsigned char vel) {
    int rootPad = 60 + (r % 12);
    if (rootPad > 67) rootPad -= 12;
    int thirdPad = rootPad + ((q == CHORD_MINOR || q == CHORD_MINOR7 || q == CHORD_DIMINISHED) ? 3 : (q == CHORD_SUS4 ? 5 : 4));
    int fifthPad = rootPad + ((q == CHORD_DIMINISHED) ? 6 : 7);
    int seventhPad = rootPad + ((q == CHORD_MAJOR7) ? 11 : 10);

    pTrack->Put(new JZKeyOnEvent(clk, ch, (unsigned char)rootPad, vel, (unsigned short)dur));
    pTrack->Put(new JZKeyOnEvent(clk, ch, (unsigned char)fifthPad, (unsigned char)(vel - 5), (unsigned short)dur));
    pTrack->Put(new JZKeyOnEvent(clk, ch, (unsigned char)(thirdPad + 12), (unsigned char)(vel - 3), (unsigned short)dur));
    notesAdded += 3;

    if (q == CHORD_DOMINANT7 || q == CHORD_MAJOR7 || q == CHORD_MINOR7)
    {
      pTrack->Put(new JZKeyOnEvent(clk, ch, (unsigned char)seventhPad, (unsigned char)(vel - 8), (unsigned short)dur));
      notesAdded++;
    }
  };

  if (chord.hasSecondChord)
  {
    int halfLen = (ticksPerBar / 2) - (ticksPerQuarter / 8);
    emitPadVoicing(barStartClock, chord.rootPitchClass, chord.quality, halfLen, 70);
    emitPadVoicing(barStartClock + (ticksPerBar / 2), chord.rootPitchClass2, chord.quality2, halfLen, 68);
  }
  else
  {
    int noteLen = ticksPerBar - (ticksPerQuarter / 8);
    emitPadVoicing(barStartClock, chord.rootPitchClass, chord.quality, noteLen, 70);
  }

  return notesAdded;
}

//-----------------------------------------------------------------------------
// Strumming & Plucking Generation (Supports Split Chords & Human Micro-Delay)
//-----------------------------------------------------------------------------
int JZAutoAccompaniment::GenerateStrumPluckMeasure(
  JZTrack* pTrack,
  int barIndex,
  int barStartClock,
  int ticksPerBar,
  int ticksPerQuarter,
  int countsPerBar,
  const JZDetectedChord& chord,
  const JZAccompanimentConfig& config)
{
  if (!pTrack || !chord.hasNotes)
  {
    return 0;
  }

  const JZStrumPluckConfig& cfg = config.strumPluck;
  int ch = pTrack->GetChannel() > 0 ? (pTrack->GetChannel() - 1) : (cfg.midiChannel - 1);
  int notesAdded = 0;
  int beatTicks = ticksPerQuarter;
  int eighthTicks = beatTicks / 2;
  int strumDelay = max(2, (ticksPerQuarter * cfg.strumSpeedMs) / 500);

  auto buildGuitarVoicing = [](int r, JZChordQuality q) -> vector<int> {
    int rootGtr = 40 + (r % 12);
    if (rootGtr < 40) rootGtr += 12;
    if (rootGtr > 50) rootGtr -= 12;
    int thirdGtr = rootGtr + ((q == CHORD_MINOR || q == CHORD_MINOR7 || q == CHORD_DIMINISHED) ? 3 : 4);
    int fifthGtr = (q == CHORD_DIMINISHED) ? (rootGtr + 6) : (rootGtr + 7);
    int seventhGtr = (q == CHORD_MAJOR7) ? (rootGtr + 11) : (rootGtr + 10);

    vector<int> p;
    p.push_back(rootGtr);
    p.push_back(fifthGtr);
    p.push_back(rootGtr + 12);
    p.push_back(thirdGtr + 12);
    if (q == CHORD_DOMINANT7 || q == CHORD_MAJOR7 || q == CHORD_MINOR7) p.push_back(seventhGtr + 12);
    else p.push_back(fifthGtr + 12);
    return p;
  };

  vector<int> chordPitches1 = buildGuitarVoicing(chord.rootPitchClass, chord.quality);
  vector<int> chordPitches2 = chord.hasSecondChord ? buildGuitarVoicing(chord.rootPitchClass2, chord.quality2) : chordPitches1;

  auto emitDown = [&](int clk, const vector<int>& pitches, int vel, int dur) {
    for (size_t k = 0; k < pitches.size(); ++k)
    {
      pTrack->Put(new JZKeyOnEvent(clk + (int)k * strumDelay, ch, (unsigned char)pitches[k], (unsigned char)max(30, vel - (int)k * 2), (unsigned short)dur));
      notesAdded++;
    }
  };

  auto emitUp = [&](int clk, const vector<int>& pitches, int vel, int dur) {
    size_t count = min((size_t)4, pitches.size());
    for (size_t k = 0; k < count; ++k)
    {
      size_t idx = pitches.size() - 1 - k;
      pTrack->Put(new JZKeyOnEvent(clk + (int)k * strumDelay, ch, (unsigned char)pitches[idx], (unsigned char)max(30, vel - (int)k * 2), (unsigned short)dur));
      notesAdded++;
    }
  };

  int ringLen = eighthTicks * 2;

  switch (cfg.style)
  {
    case STRUM_ACOUSTIC_FOLK:
    {
      emitDown(barStartClock, chordPitches1, 95, ringLen);
      emitDown(barStartClock + beatTicks, chordPitches1, 88, eighthTicks);
      emitUp(barStartClock + beatTicks + eighthTicks, chordPitches1, 78, eighthTicks);
      emitUp(barStartClock + beatTicks * 2 + eighthTicks, chordPitches2, 82, eighthTicks);
      emitDown(barStartClock + beatTicks * 3, chordPitches2, 90, eighthTicks);
      emitUp(barStartClock + beatTicks * 3 + eighthTicks, chordPitches2, 76, eighthTicks);
      break;
    }

    case STRUM_POP_ROCK_8TH:
    {
      for (int b = 0; b < countsPerBar; ++b)
      {
        const vector<int>& p = (b >= 2 && chord.hasSecondChord) ? chordPitches2 : chordPitches1;
        emitDown(barStartClock + b * beatTicks, p, 96, eighthTicks);
        emitUp(barStartClock + b * beatTicks + eighthTicks, p, 80, eighthTicks);
      }
      break;
    }

    case STRUM_FUNK_16TH_CHOP:
    {
      int sixteenth = beatTicks / 4;
      emitDown(barStartClock, chordPitches1, 102, sixteenth * 2);
      emitUp(barStartClock + sixteenth * 3, chordPitches1, 85, sixteenth * 2);
      emitDown(barStartClock + beatTicks, chordPitches1, 105, sixteenth * 2);
      emitDown(barStartClock + beatTicks * 2 + sixteenth * 2, chordPitches2, 98, sixteenth * 2);
      emitDown(barStartClock + beatTicks * 3, chordPitches2, 104, sixteenth * 2);
      break;
    }

    case STRUM_REGGAE_SKANK:
    {
      for (int b = 0; b < countsPerBar; ++b)
      {
        const vector<int>& p = (b >= 2 && chord.hasSecondChord) ? chordPitches2 : chordPitches1;
        emitDown(barStartClock + b * beatTicks + eighthTicks, p, 98, eighthTicks / 2);
      }
      break;
    }

    case PLUCK_TRAVIS_PICKING:
    {
      int pB1 = chordPitches1[0], pB2 = chordPitches1[1], pH1 = chordPitches1[3], pH2 = chordPitches1[4];
      pTrack->Put(new JZKeyOnEvent(barStartClock, ch, (unsigned char)pB1, 95, (unsigned short)ringLen));
      pTrack->Put(new JZKeyOnEvent(barStartClock + eighthTicks, ch, (unsigned char)pH1, 80, (unsigned short)eighthTicks));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks, ch, (unsigned char)pB2, 88, (unsigned short)ringLen));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks + eighthTicks, ch, (unsigned char)pH2, 85, (unsigned short)eighthTicks));

      const vector<int>& p2 = chordPitches2;
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2, ch, (unsigned char)p2[0], 92, (unsigned short)ringLen));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2 + eighthTicks, ch, (unsigned char)p2[3], 80, (unsigned short)eighthTicks));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3, ch, (unsigned char)p2[1], 88, (unsigned short)ringLen));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3 + eighthTicks, ch, (unsigned char)p2[4], 82, (unsigned short)eighthTicks));
      notesAdded += 8;
      break;
    }

    case PLUCK_BALLAD_ARPEGGIO:
    {
      int steps[8] = {0, 1, 2, 3, 4, 3, 2, 1};
      for (int i = 0; i < 8; ++i)
      {
        const vector<int>& p = (i >= 4 && chord.hasSecondChord) ? chordPitches2 : chordPitches1;
        int note = p[steps[i] % p.size()];
        int vel = (i == 0 || i == 4) ? 92 : 78;
        pTrack->Put(new JZKeyOnEvent(barStartClock + i * eighthTicks, ch, (unsigned char)note, (unsigned char)vel, (unsigned short)ringLen));
        notesAdded++;
      }
      break;
    }

    case PLUCK_PIMA_CLASSICAL:
    {
      for (int cycle = 0; cycle < 2; ++cycle)
      {
        const vector<int>& p = (cycle == 1 && chord.hasSecondChord) ? chordPitches2 : chordPitches1;
        int offset = cycle * beatTicks * 2;
        pTrack->Put(new JZKeyOnEvent(barStartClock + offset, ch, (unsigned char)p[0], 94, (unsigned short)ringLen));
        pTrack->Put(new JZKeyOnEvent(barStartClock + offset + eighthTicks, ch, (unsigned char)p[2], 80, (unsigned short)ringLen));
        pTrack->Put(new JZKeyOnEvent(barStartClock + offset + beatTicks, ch, (unsigned char)p[3], 84, (unsigned short)ringLen));
        pTrack->Put(new JZKeyOnEvent(barStartClock + offset + beatTicks + eighthTicks, ch, (unsigned char)p[4], 88, (unsigned short)ringLen));
        notesAdded += 4;
      }
      break;
    }

    case PLUCK_NEO_SOUL:
    {
      emitDown(barStartClock, chordPitches1, 92, beatTicks * 2);
      const vector<int>& p2 = chordPitches2;
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 2 + eighthTicks, ch, (unsigned char)p2[4], 84, (unsigned short)eighthTicks));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks * 3, ch, (unsigned char)p2[3], 80, (unsigned short)eighthTicks));
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
  const JZAccompanimentConfig& config)
{
  if (!pTrack || !chord.hasNotes)
  {
    return 0;
  }

  const JZRiffConfig& cfg = config.riff;
  int ch = pTrack->GetChannel() > 0 ? (pTrack->GetChannel() - 1) : (cfg.midiChannel - 1);
  int notesAdded = 0;
  int beatTicks = ticksPerQuarter;
  int eighthTicks = beatTicks / 2;
  int sixteenthTicks = beatTicks / 4;

  int baseOct = max(2, min(5, cfg.octave));
  int root = (baseOct * 12) + (chord.rootPitchClass % 12);

  int min3 = root + 3;
  int fourth = root + 5;
  int dim5 = root + 6;
  int fifth = root + 7;
  int min7 = root + 10;
  int octave = root + 12;

  switch (cfg.style)
  {
    case RIFF_BLUES_ROCK:
    {
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
      auto emitBrassStab = [&](int clk, int r, int vel) {
        int len = sixteenthTicks * 2;
        pTrack->Put(new JZKeyOnEvent(clk, ch, (unsigned char)r, (unsigned char)vel, (unsigned short)len));
        pTrack->Put(new JZKeyOnEvent(clk, ch, (unsigned char)(r + 7), (unsigned char)(vel - 4), (unsigned short)len));
        pTrack->Put(new JZKeyOnEvent(clk, ch, (unsigned char)(r + 12), (unsigned char)(vel - 2), (unsigned short)len));
        notesAdded += 3;
      };

      emitBrassStab(barStartClock + eighthTicks, root, 105);
      emitBrassStab(barStartClock + beatTicks + eighthTicks, root, 108);
      int r2 = chord.hasSecondChord ? (baseOct * 12 + chord.rootPitchClass2 % 12) : root;
      emitBrassStab(barStartClock + beatTicks * 3, r2, 112);
      break;
    }

    case RIFF_HARD_ROCK_POWER:
    {
      auto emitPowerChord = [&](int clk, int r, int vel, int len) {
        pTrack->Put(new JZKeyOnEvent(clk, ch, (unsigned char)r, (unsigned char)vel, (unsigned short)len));
        pTrack->Put(new JZKeyOnEvent(clk, ch, (unsigned char)(r + 7), (unsigned char)(vel - 2), (unsigned short)len));
        pTrack->Put(new JZKeyOnEvent(clk, ch, (unsigned char)(r + 12), (unsigned char)(vel - 3), (unsigned short)len));
        notesAdded += 3;
      };

      emitPowerChord(barStartClock, root, 110, beatTicks);
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks, ch, (unsigned char)root, 88, (unsigned short)eighthTicks));
      pTrack->Put(new JZKeyOnEvent(barStartClock + beatTicks + eighthTicks, ch, (unsigned char)root, 88, (unsigned short)eighthTicks));
      notesAdded += 2;

      int r2 = chord.hasSecondChord ? (baseOct * 12 + chord.rootPitchClass2 % 12) : root;
      emitPowerChord(barStartClock + beatTicks * 2 + eighthTicks, r2, 108, eighthTicks);
      emitPowerChord(barStartClock + beatTicks * 3, r2, 112, beatTicks);
      break;
    }

    case RIFF_SYNTHWAVE_BASS:
    {
      int pat[16] = {0, 12, 7, 12, 10, 12, 7, 12, 0, 12, 7, 12, 10, 12, 12, 7};
      for (int i = 0; i < 16; ++i)
      {
        int currRoot = (i >= 8 && chord.hasSecondChord) ? (baseOct * 12 + chord.rootPitchClass2 % 12) : root;
        int p = currRoot + pat[i];
        int vel = (i % 4 == 0) ? 104 : 85;
        pTrack->Put(new JZKeyOnEvent(barStartClock + i * sixteenthTicks, ch, (unsigned char)p, (unsigned char)vel, (unsigned short)(sixteenthTicks * 0.9)));
        notesAdded++;
      }
      break;
    }

    case RIFF_JAZZ_BEBOP_MOTIF:
    {
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
// Auto-Solo Lead Generation (With Pitch Bends & Vibrato Modulation CC 1)
//-----------------------------------------------------------------------------
int JZAutoAccompaniment::GenerateSoloMeasure(
  JZTrack* pTrack,
  int barIndex,
  int barStartClock,
  int ticksPerBar,
  int ticksPerQuarter,
  int countsPerBar,
  const JZDetectedChord& chord,
  const JZAccompanimentConfig& config)
{
  if (!pTrack || !chord.hasNotes)
  {
    return 0;
  }

  const JZSoloConfig& cfg = config.solo;
  int ch = pTrack->GetChannel() > 0 ? (pTrack->GetChannel() - 1) : (cfg.midiChannel - 1);
  int notesAdded = 0;
  int beatTicks = ticksPerQuarter;
  int eighthTicks = beatTicks / 2;

  int baseOct = max(4, min(6, cfg.octave));
  int root = (baseOct * 12) + (chord.rootPitchClass % 12);

  // Build chord-scale tones pool
  vector<int> scale;
  if (chord.quality == CHORD_MINOR || chord.quality == CHORD_MINOR7)
  {
    int degs[] = {0, 2, 3, 5, 7, 9, 10, 12, 14, 15};
    for (int d : degs) scale.push_back(root + d);
  }
  else if (chord.quality == CHORD_DOMINANT7)
  {
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
    int degs[] = {0, 2, 4, 7, 9, 12, 14, 16};
    for (int d : degs) scale.push_back(root + d);
  }

  bool isCallMeasure = (barIndex % 2 == 0);

  if (isCallMeasure)
  {
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
    int resolvePitch = scale[0] + 12; // Octave Root
    int noteDur = beatTicks * 2;

    if (cfg.style == SOLO_BLUESY_SOULFUL && config.enableSoloBendsAndVibrato)
    {
      // Expressive Pitch Bend: Blues flat-5 bending into natural 5th
      int bendNote = root + 6;
      pTrack->Put(new JZKeyOnEvent(barStartClock, ch, (unsigned char)bendNote, 105, (unsigned short)noteDur));
      notesAdded++;

      // Half-step pitch bend upwards at midpoint (+4096)
      pTrack->Put(new JZPitchEvent(barStartClock + eighthTicks, ch, 4096));
      // Reset pitch bend at end of note
      pTrack->Put(new JZPitchEvent(barStartClock + noteDur, ch, 0));
    }
    else
    {
      pTrack->Put(new JZKeyOnEvent(barStartClock, ch, (unsigned char)resolvePitch, 112, (unsigned short)noteDur));
      notesAdded++;
    }

    // Expressive Vibrato (CC 1 Modulation Wheel) on sustained note
    if (config.enableSoloBendsAndVibrato)
    {
      pTrack->Put(new JZControlEvent(barStartClock + beatTicks, ch, 1, 40));
      pTrack->Put(new JZControlEvent(barStartClock + (int)(beatTicks * 1.5), ch, 1, 64));
      pTrack->Put(new JZControlEvent(barStartClock + noteDur, ch, 1, 0));
    }

    if (cfg.density == SOLO_DENSITY_VIRTUOSO)
    {
      int p3 = scale[3], p2 = scale[2], pRoot = scale[0];
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
      totalBass += GenerateBassMeasure(pBassTrack, b, bStart, bTicks, tpq, bCounts, ch, nextCh, config);
    }
    if (pDrumsTrack)
    {
      totalDrums += GenerateDrumsMeasure(pDrumsTrack, b, bStart, bTicks, tpq, bCounts, config);
    }
    if (pPadTrack)
    {
      totalPad += GeneratePadMeasure(pPadTrack, b, bStart, bTicks, tpq, bCounts, ch, config);
    }
    if (pStrumTrack)
    {
      totalStrum += GenerateStrumPluckMeasure(pStrumTrack, b, bStart, bTicks, tpq, bCounts, ch, config);
    }
    if (pRiffTrack)
    {
      totalRiff += GenerateRiffMeasure(pRiffTrack, b, bStart, bTicks, tpq, bCounts, ch, config);
    }
    if (pSoloTrack)
    {
      totalSolo += GenerateSoloMeasure(pSoloTrack, b, bStart, bTicks, tpq, bCounts, ch, config);
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
