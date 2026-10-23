//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Smart Auto-Accompaniment & Music Intelligence Engine (Pro Edition)
//
// Full-Band Accompaniment, Strumming, Plucking, Riffs, and Expressive Solo
// with Smooth Voice Leading, Song Form Dynamics, Key Center Detection,
// Smart Auto-Mixer Presets, Advanced Bass Articulations, and Live Jamming.
//*****************************************************************************

#pragma once

#include "Globals.h"
#include <string>
#include <vector>

class JZSong;
class JZTrack;

// Generation Modes
enum JZGenerationMode
{
  MODE_FULL_BAND = 0,     // Full Accompaniment: Bass, Drums, Strings/Pad
  MODE_STRUM_PLUCK,       // Guitar / Keys Rhythmic Strumming & Plucking
  MODE_RIFF_MAKER,        // Harmonic Riffs & Melodic Hooks
  MODE_AUTO_SOLO,         // Intelligent Lead Improviser / Solo Generator
  MODE_ALL_SIMULTANEOUS   // Generate all selected layers together
};

// Song Form Dynamic Sections
enum JZSongSection
{
  SECTION_AUTO = 0,       // Procedurally determined by bar position in the arrangement
  SECTION_INTRO,          // Soft ambient entry, pedal tones, light cymbal/hi-hat
  SECTION_VERSE,          // 2-feel bass with ghost notes, rimshot / closed hi-hat, lyrical phrasing
  SECTION_CHORUS,         // 4-feel walking bass, ride cymbal, energetic full dynamics, virtuoso solo
  SECTION_BRIDGE,         // Breakdown / syncopated hits, alternate harmonic texture
  SECTION_OUTRO,          // Decrescendo fade, pedal point, definitive final cadence
  SECTION_COUNT
};

// Advanced Bass Articulation Feels
enum JZBassFeel
{
  BASS_FEEL_AUTO = 0,     // 2-feel in Intro/Verse, 4-feel in Chorus/Bridge
  BASS_FEEL_2,            // Half-notes with subtle ghost notes (2-feel)
  BASS_FEEL_4             // Quarter-notes continuous walking bass (4-feel)
};

// Global Key Center / Tonal Anchor (Krumhansl-Schmuckler Key Finding)
struct JZKeyCenter
{
  int tonicPc;            // 0..11 (0=C, 1=C#, 2=D, 3=D#, 4=E, 5=F, 6=F#, 7=G, 8=G#, 9=A, 10=Bb, 11=B)
  bool isMinor;           // false = Major, true = Minor
  float confidence;       // 0.0 to 1.0 correlation score
  std::string name;       // e.g. "C Major", "A Minor"

  JZKeyCenter()
    : tonicPc(0),
      isMinor(false),
      confidence(0.0f),
      name("C Major")
  {
  }
};

// 1. Full Band Accompaniment Styles
enum JZAccompanimentStyle
{
  STYLE_JAZZ_SWING = 0,
  STYLE_BOSSA_NOVA,
  STYLE_BLUES_SHUFFLE,
  STYLE_POP_ROCK,
  STYLE_FUNK_GROOVE,
  STYLE_COUNT
};

// 2. Strumming & Plucking Styles
enum JZStrumPluckStyle
{
  STRUM_ACOUSTIC_FOLK = 0,    // Down-down-up-up-down folk pattern
  STRUM_POP_ROCK_8TH,         // Driving 8th down/up strokes
  STRUM_FUNK_16TH_CHOP,       // 16th chops with muted chucks
  STRUM_REGGAE_SKANK,         // Offbeat 8th skanks
  PLUCK_TRAVIS_PICKING,       // Alternating bass + finger syncopation
  PLUCK_BALLAD_ARPEGGIO,      // Flowing 8th/16th broken chord
  PLUCK_PIMA_CLASSICAL,       // Pulgar-Indice-Medio-Anular classical
  PLUCK_NEO_SOUL,             // Spread voicing with open release
  STRUM_PLUCK_COUNT
};

// 3. Riff Generator Styles
enum JZRiffStyle
{
  RIFF_BLUES_ROCK = 0,        // Pentatonic walking / driving blues hook
  RIFF_FUNK_BRASS,            // Sharp syncopated brass section hits
  RIFF_HARD_ROCK_POWER,       // Heavy palm-muted pedal-tone power riff
  RIFF_SYNTHWAVE_BASS,        // 16th rolling synth bass hook
  RIFF_JAZZ_BEBOP_MOTIF,      // Chromatic bebop horn lick
  RIFF_COUNT
};

// 4. Auto-Solo Styles & Density
enum JZSoloStyle
{
  SOLO_MELODIC_LYRICAL = 0,   // Singing melodic phrasing, sustained targets
  SOLO_BLUESY_SOULFUL,        // Blues scale, blue notes, call-and-response
  SOLO_BEBOP_JAZZ,            // Rapid 8th notes with chromatic enclosures
  SOLO_ROCK_PENTATONIC,       // Bending patterns, repeated high-energy licks
  SOLO_COUNT
};

enum JZSoloDensity
{
  SOLO_DENSITY_SPARSE = 0,    // Lots of rests, soulful breathing
  SOLO_DENSITY_BALANCED,      // Standard phrasing with 2-bar call/response
  SOLO_DENSITY_VIRTUOSO       // Dense 8th/16th lines
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

// Chord information for a measure (supports split-bar 2 chords & inversions)
struct JZDetectedChord
{
  // First Half (or full measure)
  int rootPitchClass; // 0..11 (0=C, 1=C#, 2=D, 3=D#/Eb, 4=E, 5=F, 6=F#, 7=G, 8=G#, 9=A, 10=A#/Bb, 11=B)
  int bassPitchClass; // Lowest bass note played (for slash chords e.g. C/E)
  int inversion;      // 0 = Root position, 1 = 1st inversion, 2 = 2nd inversion, 3 = 3rd inversion
  JZChordQuality quality;
  bool hasNotes;
  int minPitch;

  // Second Half of Measure (for split-measure progressions e.g. ii - V)
  bool hasSecondChord;
  int rootPitchClass2;
  int bassPitchClass2;
  int inversion2;
  JZChordQuality quality2;

  JZDetectedChord()
    : rootPitchClass(0),
      bassPitchClass(0),
      inversion(0),
      quality(CHORD_MAJOR),
      hasNotes(false),
      minPitch(60),
      hasSecondChord(false),
      rootPitchClass2(0),
      bassPitchClass2(0),
      inversion2(0),
      quality2(CHORD_MAJOR)
  {
  }

  std::string GetChordName(bool second = false) const;
};

// Configuration for Strum & Pluck
struct JZStrumPluckConfig
{
  JZStrumPluckStyle style;
  int targetTrackIndex; // Default: 3 (Track 4 in UI)
  int midiChannel;      // Default: 4
  int patchNumber;      // GM 26 (Steel Acoustic Guitar)
  int strumSpeedMs;     // 10 - 35ms (micro-delay between strings)
  bool humanize;

  JZStrumPluckConfig()
    : style(STRUM_ACOUSTIC_FOLK),
      targetTrackIndex(3),
      midiChannel(4),
      patchNumber(26),
      strumSpeedMs(18),
      humanize(true)
  {
  }
};

// Configuration for Riff Maker
struct JZRiffConfig
{
  JZRiffStyle style;
  int targetTrackIndex; // Default: 4 (Track 5 in UI)
  int midiChannel;      // Default: 5
  int patchNumber;      // GM 30 (Overdriven Guitar) or 62 (Brass Section)
  int octave;           // 2..5 (default 3)

  JZRiffConfig()
    : style(RIFF_BLUES_ROCK),
      targetTrackIndex(4),
      midiChannel(5),
      patchNumber(30),
      octave(3)
  {
  }
};

// Configuration for Auto-Solo
struct JZSoloConfig
{
  JZSoloStyle style;
  JZSoloDensity density;
  int targetTrackIndex; // Default: 5 (Track 6 in UI)
  int midiChannel;      // Default: 6
  int patchNumber;      // GM 66 (Alto Sax) or 57 (Trumpet)
  int octave;           // 4..6 (default 5)

  JZSoloConfig()
    : style(SOLO_MELODIC_LYRICAL),
      density(SOLO_DENSITY_BALANCED),
      targetTrackIndex(5),
      midiChannel(6),
      patchNumber(66),
      octave(5)
  {
  }
};

// Master Generation Configuration
struct JZAccompanimentConfig
{
  JZGenerationMode mode;
  int sourceTrackIndex; // 0-based (0 = Track 1)

  // Full Band Accompaniment Settings
  JZAccompanimentStyle style;
  bool generateBass;
  int bassTrackIndex;   // Default: 1 (Track 2)
  bool generateDrums;
  int drumsTrackIndex;  // Default: 9 (Track 10)
  bool generatePad;
  int padTrackIndex;    // Default: 2 (Track 3)

  // Modular Feature Settings
  JZStrumPluckConfig strumPluck;
  JZRiffConfig riff;
  JZSoloConfig solo;

  // Song Form & Arrangement Dynamics
  JZSongSection songSection;      // SECTION_AUTO, SECTION_INTRO, SECTION_VERSE, SECTION_CHORUS, SECTION_BRIDGE, SECTION_OUTRO
  JZBassFeel bassFeel;            // BASS_FEEL_AUTO, BASS_FEEL_2, BASS_FEEL_4

  // Studio Audio & Harmonization
  bool applyMixerPresets;         // Balanced stereo panning & volume levels for professional mix
  bool enableSmoothVoiceLeading;  // Intelligent inversions to minimize pad register leaps
  bool enableGhostNotes;          // Subtle ghost notes & octave drops on bass

  // Musicality Enhancements
  int swingPercent;               // 50 (straight) to 70 (heavy swing)
  bool enableHumanizeVelocity;    // Subtle velocity drift
  bool enableDrumFills;           // Fills on measure 4, 8, 12, 16
  bool enableSoloBendsAndVibrato; // Pitch bend & CC 1 vibrato on solo

  // Global Key Center
  JZKeyCenter keyCenter;

  // Range and Options
  int fromBar;          // 0-based bar index (0 = Bar 1)
  int toBar;            // 0-based bar index (-1 = up to end of song)
  bool overwriteExisting;

  JZAccompanimentConfig()
    : mode(MODE_FULL_BAND),
      sourceTrackIndex(0),
      style(STYLE_JAZZ_SWING),
      generateBass(true),
      bassTrackIndex(1),
      generateDrums(true),
      drumsTrackIndex(9),
      generatePad(true),
      padTrackIndex(2),
      songSection(SECTION_AUTO),
      bassFeel(BASS_FEEL_AUTO),
      applyMixerPresets(true),
      enableSmoothVoiceLeading(true),
      enableGhostNotes(true),
      swingPercent(62),
      enableHumanizeVelocity(true),
      enableDrumFills(true),
      enableSoloBendsAndVibrato(true),
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
  int sectionsProcessed;
  int bassNotesGenerated;
  int drumNotesGenerated;
  int padNotesGenerated;
  int strumNotesGenerated;
  int riffNotesGenerated;
  int soloNotesGenerated;
  JZKeyCenter detectedKey;
  std::string message;

  JZAccompanimentResult()
    : success(false),
      barsProcessed(0),
      sectionsProcessed(0),
      bassNotesGenerated(0),
      drumNotesGenerated(0),
      padNotesGenerated(0),
      strumNotesGenerated(0),
      riffNotesGenerated(0),
      soloNotesGenerated(0)
  {
  }
};

// Main Accompaniment Engine
class JZAutoAccompaniment
{
  public:

    static const char* GetStyleName(JZAccompanimentStyle style);
    static const char* GetStrumPluckStyleName(JZStrumPluckStyle style);
    static const char* GetRiffStyleName(JZRiffStyle style);
    static const char* GetSoloStyleName(JZSoloStyle style);
    static const char* GetSectionName(JZSongSection section);
    static const char* GetBassFeelName(JZBassFeel feel);

    // Procedural song structure mapping
    static JZSongSection DetermineSection(int barIndex, int totalBars, JZSongSection overrideSection = SECTION_AUTO);

    // Global Key Center Detection via Krumhansl-Schmuckler Key-Finding Algorithm
    static JZKeyCenter DetectSongKeyCenter(JZTrack* pSourceTrack);

    // Smooth Voice Leading: finds voicing that minimizes semitone distance from prevVoicing
    static std::vector<int> FindSmoothVoiceLeading(
      int rootPc,
      JZChordQuality quality,
      const std::vector<int>& prevVoicing);

    // Run the generator on the given song according to config
    static JZAccompanimentResult Generate(JZSong* pSong, const JZAccompanimentConfig& config);

    // Analyze a measure of a track to extract root, quality, inversions, and half-bar splits
    static JZDetectedChord AnalyzeMeasure(
      JZTrack* pSourceTrack,
      int barStartClock,
      int barEndClock,
      const JZDetectedChord& fallbackChord);

  public:

    // Generator helpers for each instrument (now with Song Form & Voice Leading awareness)
    static int GenerateBassMeasure(
      JZTrack* pTrack,
      int barIndex,
      int barStartClock,
      int ticksPerBar,
      int ticksPerQuarter,
      int countsPerBar,
      const JZDetectedChord& chord,
      const JZDetectedChord& nextChord,
      const JZAccompanimentConfig& config,
      JZSongSection currentSection);

    static int GenerateDrumsMeasure(
      JZTrack* pTrack,
      int barIndex,
      int barStartClock,
      int ticksPerBar,
      int ticksPerQuarter,
      int countsPerBar,
      const JZAccompanimentConfig& config,
      JZSongSection currentSection);

    static int GeneratePadMeasure(
      JZTrack* pTrack,
      int barIndex,
      int barStartClock,
      int ticksPerBar,
      int ticksPerQuarter,
      int countsPerBar,
      const JZDetectedChord& chord,
      const JZAccompanimentConfig& config,
      JZSongSection currentSection,
      std::vector<int>& prevVoicing);

    static int GenerateStrumPluckMeasure(
      JZTrack* pTrack,
      int barIndex,
      int barStartClock,
      int ticksPerBar,
      int ticksPerQuarter,
      int countsPerBar,
      const JZDetectedChord& chord,
      const JZAccompanimentConfig& config,
      JZSongSection currentSection);

    static int GenerateRiffMeasure(
      JZTrack* pTrack,
      int barIndex,
      int barStartClock,
      int ticksPerBar,
      int ticksPerQuarter,
      int countsPerBar,
      const JZDetectedChord& chord,
      const JZAccompanimentConfig& config,
      JZSongSection currentSection);

    static int GenerateSoloMeasure(
      JZTrack* pTrack,
      int barIndex,
      int barStartClock,
      int ticksPerBar,
      int ticksPerQuarter,
      int countsPerBar,
      const JZDetectedChord& chord,
      const JZAccompanimentConfig& config,
      JZSongSection currentSection);

    // Clear key-on notes in clock range [fromClock, toClock)
    static void ClearRange(JZTrack* pTrack, int fromClock, int toClock);

  private:

    // Helper for analyzing notes within an arbitrary clock window
    static bool AnalyzeWindow(
      JZTrack* pTrack,
      int startClock,
      int endClock,
      int& rootPc,
      int& bassPc,
      int& inversion,
      JZChordQuality& quality,
      int& minP);
};

// Real-Time Interactive Chord & Jamming Tracker
class JZLiveJamEngine
{
  public:

    JZLiveJamEngine();
    void Reset();
    void NoteOn(int pitch, int velocity);
    void NoteOff(int pitch);
    bool HasActiveChord() const;
    const JZDetectedChord& GetCurrentChord() const;
    std::string GetCurrentChordName() const;

  private:

    void Reevaluate();
    std::vector<int> mActivePitches;
    JZDetectedChord mCurrentChord;
};
    static void ClearRange(JZTrack* pTrack, int fromClock, int toClock);

  private:

    // Helper for analyzing notes within an arbitrary clock window
    static bool AnalyzeWindow(
      JZTrack* pTrack,
      int startClock,
      int endClock,
      int& rootPc,
      int& bassPc,
      int& inversion,
      JZChordQuality& quality,
      int& minP);
};
