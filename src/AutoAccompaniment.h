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
#include <map>
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
typedef JZGenerationMode JZAccompanimentMode;

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
  STYLE_JAZZ_WALTZ_3_4,       // 3/4 Time Signature Jazz Waltz Trio
  STYLE_BALLAD_6_8,           // 6/8 Time Signature Slow Soul / Blues Ballad
  STYLE_NUSANTARA_ZAPIN,      // Nusantara Zapin (Gambus & Marwas syncopation)
  STYLE_NUSANTARA_JOGET,      // Nusantara Joget (Lively 2/4 folk dance)
  STYLE_NUSANTARA_INANG,      // Nusantara Inang / Asli (Graceful 8-beat lenggang)
  STYLE_LOFI_CHILLHOP,        // Lo-Fi Chillhop (Laid-back boom-bap & warm Rhodes)
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
  STRUM_WALTZ_3_4,            // 3/4 Time Waltz acoustic strum
  PLUCK_BALLAD_6_8,           // 6/8 Time Slow 6-step arpeggio flow
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

// 5. Smart Chord Progression Presets (1-Click Song Starter)
enum JZChordProgressionPreset
{
  PROGRESSION_JAZZ_2_5_1_6 = 0,    // ii-V-I-vi (Dm7 -> G7 -> Cmaj7 -> A7)
  PROGRESSION_POP_4_CHORDS,        // I-V-vi-IV (C -> G -> Am -> F)
  PROGRESSION_NEO_SOUL_2_5_1,      // ii9 -> V13 -> Imaj9 -> VI7b13
  PROGRESSION_12_BAR_BLUES,        // 12-Bar Blues standard in Key (I7-IV7-V7)
  PROGRESSION_NUSANTARA_FOLK,      // Tradisi Melayu / Zapin-Joget (Am -> Dm -> E7 -> Am)
  PROGRESSION_ANDALUSIAN,          // Andalusian Flamenco Cadence (Am -> G -> F -> E)
  PROGRESSION_PACHELBEL,           // Pachelbel Canon (C -> G -> Am -> Em -> F -> C -> F -> G)
  PROGRESSION_LOFI_VAMP,           // Lo-Fi Chillhop Vamp (Fmaj7 -> Em7 -> Dm7 -> Cmaj7)
  PROGRESSION_COUNT
};

// 6. Velocity Dynamics & Human Touch Curves
enum JZVelocityCurve
{
  VELOCITY_CURVE_LINEAR = 0,       // Standard balanced response
  VELOCITY_CURVE_SOFT_LYRICAL,     // Logarithmic soft touch (ballads, smooth jazz)
  VELOCITY_CURVE_PUNCHY_HARD,      // Exponential high-energy attack (funk, rock)
  VELOCITY_CURVE_VINTAGE_ANALOG,   // Analog tape warmth and micro-tuning drift
  VELOCITY_CURVE_COUNT
};

// 7. Microtonal Tuning & Traditional Scales
enum JZMicrotonalTuning
{
  MICROTONAL_NONE = 0,
  MICROTONAL_BAYATI_RAST,          // Neutral 3rd (-50 cents) / Arabic-Malay Rast quarter-tone
  MICROTONAL_SLENDRO_PELOG,        // Gamelan equidistant scale approximation
  MICROTONAL_COUNT
};

// 8. Reharmonization & Chord Substitutions
enum JZReharmonizeMode
{
  REHARM_NONE = 0,
  REHARM_TRITONE_SUB,          // Tritone substitution: V7 -> bII7 (e.g. G7 -> Db7)
  REHARM_SECONDARY_DOMINANTS,  // Precede target chords with secondary dominant (V7/x)
  REHARM_PASSING_DIMINISHED,   // Chromatic passing diminished 7th chords
  REHARM_MODAL_INTERCHANGE,    // Minor plagal cadence (iv minor / bVImaj7)
  REHARM_COLTRANE_CHANGES,     // Coltrane 3-tonic matrix substitution
  REHARM_COUNT
};

// 9. Intelligent Melodic & Vocal / Horn Harmonizer
enum JZHarmonizerStyle
{
  HARMONY_PARALLEL_3RD = 0,    // Sweet diatonic 3rd above / 6th below
  HARMONY_PARALLEL_6TH,        // Warm diatonic 6th below
  HARMONY_3_PIECE_HORNS,       // Trumpet lead, Tenor Sax 3rd/7th, Trombone root/5th
  HARMONY_4_PART_VOCAL_SATB,   // SATB 4-part choir close-voicing
  HARMONY_STYLE_COUNT
};

struct JZHarmonizerConfig
{
  int sourceTrackIndex;        // Track with lead melody / vocal (default 0)
  int targetTrackIndex;        // Destination track for harmony voices (default 4)
  JZHarmonizerStyle style;     // HARMONY_PARALLEL_3RD, HARMONY_3_PIECE_HORNS, etc.
  int midiChannel;             // Default 5
  int patchNumber;             // Default 66 (Tenor Sax) or 53 (Choir Aahs)
  int stereoPan;               // Default 75
  int volume;                  // Default 95
  int octaveShift;             // Default 0

  JZHarmonizerConfig()
    : sourceTrackIndex(0),
      targetTrackIndex(4),
      style(HARMONY_PARALLEL_3RD),
      midiChannel(5),
      patchNumber(66),
      stereoPan(75),
      volume(95),
      octaveShift(0)
  {
  }
};

// Chord Qualities detected from piano track
enum JZChordQuality
{
  CHORD_MAJOR = 0,
  CHORD_MINOR,
  CHORD_DOMINANT7,
  CHORD_DOM7 = CHORD_DOMINANT7,
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

  // Micro-Timing Pocket & Humanization (Laid-Back Snare & Push Bass)
  bool enableMicroPocket;         // Micro-delay on backbeat snare & push bass
  int snarePocketTicks;           // Default: +8 ticks behind beat (~12ms)
  int bassDriveTicks;             // Default: -3 ticks ahead of beat (~5ms)

  // Smart Scale Lock & Input Quantize
  bool enableScaleLock;           // Constrain notes to detected global key scale
  bool enableInputQuantize;       // Snap live notes to nearest grid division

  // Global Key Center
  JZKeyCenter keyCenter;

  // Velocity Dynamics & Touch Response
  JZVelocityCurve velocityCurve;

  // Tempo Automation & Outro Ritardando
  bool enableOutroRitardando;      // Decrescendo / ritardando on final measures
  int outroSlowdownPercent;        // Default: 25% slowdown on cadence

  // Microtonal Pitch Offset
  bool enableMicrotonalTuning;
  JZMicrotonalTuning microtonalTuning;

  // Reharmonization Engine
  JZReharmonizeMode reharmonizeMode;

  // Audio Click Track & Metronome Count-in
  bool enableAudioClickTrack;
  bool enableCountIn;
  int countInBars;

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
      enableMicroPocket(true),
      snarePocketTicks(8),
      bassDriveTicks(-3),
      enableScaleLock(false),
      enableInputQuantize(true),
      velocityCurve(VELOCITY_CURVE_LINEAR),
      enableOutroRitardando(true),
      outroSlowdownPercent(25),
      enableMicrotonalTuning(false),
      microtonalTuning(MICROTONAL_NONE),
      reharmonizeMode(REHARM_NONE),
      enableAudioClickTrack(false),
      enableCountIn(false),
      countInBars(1),
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

    // Smart Scale Lock (Snap-to-Key) and Quantize Helpers
    static int SnapToKeyScale(int pitch, const JZKeyCenter& keyCenter);
    static int QuantizeClock(int clock, int ticksPerQuarter, int division = 16, int swingPercent = 50);

    // Dynamic Velocity Response Curve Transformer
    static unsigned char ApplyVelocityCurve(int baseVelocity, JZVelocityCurve curve, int step = 0);

    // 1-Click Chord Progression Generator (Instant Song Starter)
    static const char* GetProgressionName(JZChordProgressionPreset preset);
    static const char* GetVelocityCurveName(JZVelocityCurve curve);
    static const char* GetMicrotonalTuningName(JZMicrotonalTuning tuning);

    static int GenerateChordProgression(
      JZSong* pSong,
      int targetTrackIndex,
      JZChordProgressionPreset preset,
      int tonicPitchClass = 0,
      int startBar = 0,
      int repeatCount = 2,
      bool isMinor = false);

    // Musician's Lead Sheet & Real Book Generator (HTML / Text)
    static std::string GenerateLeadSheetHtml(
      JZSong* pSong,
      const std::string& songTitle = "Untitled Jazz Project",
      const std::string& composer = "Jazz++ Arranger");

    static bool ExportLeadSheetHtml(
      JZSong* pSong,
      const std::string& filePath,
      const std::string& songTitle = "Untitled Jazz Project",
      const std::string& composer = "Jazz++ Arranger");

    // Reharmonization & Chord Substitutions
    static const char* GetReharmonizeModeName(JZReharmonizeMode mode);
    static const char* GetHarmonizerStyleName(JZHarmonizerStyle style);

    static int ApplyReharmonization(
      JZSong* pSong,
      JZReharmonizeMode mode,
      int fromBar = 0,
      int toBar = -1);

    // Intelligent Melodic & Vocal / Horn Harmonizer
    static int GenerateMelodicHarmony(
      JZSong* pSong,
      const JZHarmonizerConfig& config,
      int fromBar = 0,
      int toBar = -1);

    // Interactive Chord Pad Matrix Scale Degree Helper
    static JZDetectedChord GetChordPadChord(int tonicPc, bool isMinor, int padIndex);

    // Arrangement Preset Serialization
    static bool SaveArrangementPreset(const JZAccompanimentConfig& config, const std::string& filePath);
    static bool LoadArrangementPreset(JZAccompanimentConfig& outConfig, const std::string& filePath);

    // Global Chord Track access for Timeline Ruler and Viewers
    static std::string GetMeasureChordName(JZSong* pSong, int barIndex);

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

    // Generator helpers for each instrument (now with Song Form, Time Signature & Voice Leading awareness)
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

// Global Chord Track & Timeline Marker Registry
class JZChordTrack
{
  public:

    static JZChordTrack& Instance();

    void Clear();
    void SetChord(int barIndex, const JZDetectedChord& chord);
    bool HasChord(int barIndex) const;
    JZDetectedChord GetChord(int barIndex) const;
    std::string GetChordName(int barIndex) const;
    void PopulateFromSong(JZSong* pSong, int sourceTrackIndex = 0);

  private:

    JZChordTrack() {}
    std::map<int, JZDetectedChord> mChords;
};

// Direct MIDI-to-Audio WAV Mixdown Renderer
class JZMidiAudioRenderer
{
  public:

    // Renders all active tracks in pSong to a 16-bit 44.1kHz stereo PCM WAV file.
    static bool ExportWav(
      JZSong* pSong,
      const std::string& wavFilePath,
      int sampleRate = 44100,
      int fromClock = 0,
      int toClock = -1,
      bool includeClickTrack = false,
      bool includeCountIn = false,
      int countInBars = 1);

    // Multi-Track Stem Audio Export (renders each active track to separate stem WAV files)
    static bool ExportStems(
      JZSong* pSong,
      const std::string& outputDirectory,
      std::vector<std::string>& outGeneratedFiles,
      int sampleRate = 44100,
      int fromClock = 0,
      int toClock = -1,
      bool includeClickTrack = false);
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
