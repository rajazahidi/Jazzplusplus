//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Smart Auto-Accompaniment / Session Band Engine
//
// Automatically generates Bass, Drum, and String/Pad accompaniment tracks
// based on chord and rhythmic analysis of an input piano/melody track.
//*****************************************************************************

#pragma once

#include "Globals.h"
#include <string>
#include <vector>

class JZSong;
class JZTrack;

// Musical Accompaniment Styles
enum JZAccompanimentStyle
{
  STYLE_JAZZ_SWING = 0,
  STYLE_BOSSA_NOVA,
  STYLE_BLUES_SHUFFLE,
  STYLE_POP_ROCK,
  STYLE_FUNK_GROOVE,
  STYLE_COUNT
};

// Chord Qualities detected from piano track
enum JZChordQuality
{
  CHORD_MAJOR = 0,
  CHORD_MINOR,
  CHORD_DOMINANT7,
  CHORD_MAJOR7,
  CHORD_MINOR7,
  CHORD_DIMINISHED,
  CHORD_SUS4
};

// Chord information for a single measure
struct JZDetectedChord
{
  int rootPitchClass; // 0..11 (0=C, 1=C#, 2=D, 3=D#/Eb, 4=E, 5=F, 6=F#, 7=G, 8=G#, 9=A, 10=A#/Bb, 11=B)
  JZChordQuality quality;
  bool hasNotes;
  int minPitch;       // Lowest pitch detected in measure

  JZDetectedChord()
    : rootPitchClass(0),
      quality(CHORD_MAJOR),
      hasNotes(false),
      minPitch(60)
  {
  }

  std::string GetChordName() const;
};

// Generation Configuration Options
struct JZAccompanimentConfig
{
  JZAccompanimentStyle style;
  int sourceTrackIndex; // 0-based (0 = Track 1, 1 = Track 2, etc.)

  bool generateBass;
  int bassTrackIndex;   // Default: 1 (Track 2)

  bool generateDrums;
  int drumsTrackIndex;  // Default: 9 (Track 10)

  bool generatePad;
  int padTrackIndex;    // Default: 2 (Track 3)

  int fromBar;          // 0-based bar index (0 = Bar 1)
  int toBar;            // 0-based bar index (-1 = up to end of song)
  bool overwriteExisting;

  JZAccompanimentConfig()
    : style(STYLE_JAZZ_SWING),
      sourceTrackIndex(0),
      generateBass(true),
      bassTrackIndex(1),
      generateDrums(true),
      drumsTrackIndex(9),
      generatePad(true),
      padTrackIndex(2),
      fromBar(0),
      toBar(-1),
      overwriteExisting(true)
  {
  }
};

// Summary report returned after generation
struct JZAccompanimentResult
{
  bool success;
  int barsProcessed;
  int bassNotesGenerated;
  int drumNotesGenerated;
  int padNotesGenerated;
  std::string message;

  JZAccompanimentResult()
    : success(false),
      barsProcessed(0),
      bassNotesGenerated(0),
      drumNotesGenerated(0),
      padNotesGenerated(0)
  {
  }
};

// Main Accompaniment Engine
class JZAutoAccompaniment
{
  public:

    static const char* GetStyleName(JZAccompanimentStyle style);

    // Run the generator on the given song according to config
    static JZAccompanimentResult Generate(JZSong* pSong, const JZAccompanimentConfig& config);

    // Analyze a measure of a track to extract root and chord quality
    static JZDetectedChord AnalyzeMeasure(
      JZTrack* pSourceTrack,
      int barStartClock,
      int barEndClock,
      const JZDetectedChord& fallbackChord);

  private:

    // Generator helpers for each instrument
    static int GenerateBassMeasure(
      JZTrack* pTrack,
      int barIndex,
      int barStartClock,
      int ticksPerBar,
      int ticksPerQuarter,
      int countsPerBar,
      const JZDetectedChord& chord,
      const JZDetectedChord& nextChord,
      JZAccompanimentStyle style);

    static int GenerateDrumsMeasure(
      JZTrack* pTrack,
      int barIndex,
      int barStartClock,
      int ticksPerBar,
      int ticksPerQuarter,
      int countsPerBar,
      JZAccompanimentStyle style);

    static int GeneratePadMeasure(
      JZTrack* pTrack,
      int barIndex,
      int barStartClock,
      int ticksPerBar,
      int ticksPerQuarter,
      int countsPerBar,
      const JZDetectedChord& chord,
      JZAccompanimentStyle style);

    // Clear key-on notes in clock range [fromClock, toClock)
    static void ClearRange(JZTrack* pTrack, int fromClock, int toClock);
};
