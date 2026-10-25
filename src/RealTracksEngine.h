//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// RealTracksEngine.h - Band-in-a-Box Style Acoustic RealTracks Engine
//
// Multi-style real acoustic audio phrase accompaniment:
// - WSOLA (Waveform Similarity Overlap-Add) elastic time-stretching (50-240 BPM)
// - Harmonic chord-following & real-time pitch transposition
// - Multi-track acoustic phrase banks (Upright Bass, Acoustic Guitar, Grand Piano, Horns)
// - Downbeat & measure boundary phase synchronization
// - High-fidelity cubic Hermite resampling
//*****************************************************************************

#ifndef JAZZ_REAL_TRACKS_ENGINE_H
#define JAZZ_REAL_TRACKS_ENGINE_H

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <algorithm>
#include <memory>

namespace Jazz {

enum class JZRealTracksStyle {
    JAZZ_SWING_TRIO = 0,       // Upright Bass, Archtop Jazz Guitar, Grand Piano comping
    ACOUSTIC_FOLK_BALLAD = 1,  // Steel-string strum & fingerpicking, Upright Bass, Piano
    BOSSA_NOVA_CLUB = 2,       // Nylon Bossa Guitar, Mellow Upright, Soft Piano chords
    POP_ROCK_STUDIO = 3,       // Driving Bass, Acoustic 12-string strum, Rock Grand Piano
    BLUES_SHUFFLE = 4,         // Walking Shuffle Bass, Blues Rhythm Guitar, Honky-tonk Piano
    FUNK_GROOVE = 5,           // Syncopated Funk Bass, 16th Chop Guitar, Clav/Keys
    COUNT
};

enum class JZRealTrackType {
    BASS = 0,
    GUITAR = 1,
    PIANO = 2,
    HORNS_STRINGS = 3,
    COUNT
};

enum class JZRealChordQuality {
    MAJOR = 0,
    MINOR = 1,
    DOMINANT7 = 2,
    MAJOR7 = 3,
    MINOR7 = 4,
    DIMINISHED = 5,
    SUS4 = 6
};

class JZRealTracksEngine {
public:
    struct Parameters {
        JZRealTracksStyle style = JZRealTracksStyle::JAZZ_SWING_TRIO;
        double projectBpm = 120.0;           // Song tempo (50.0 to 240.0)
        double referenceLoopBpm = 120.0;     // Base recorded loop tempo
        int currentChordRoot = 0;            // 0=C, 1=C#, 2=D ... 11=B
        JZRealChordQuality currentChordQuality = JZRealChordQuality::MAJOR;
        bool trackEnabled[static_cast<size_t>(JZRealTrackType::COUNT)] = { true, true, true, false };
        float trackVolume[static_cast<size_t>(JZRealTrackType::COUNT)] = { 0.85f, 0.80f, 0.85f, 0.70f };
        float masterLevel = 1.0f;            // RealTracks bus master level (0.0 to 2.0)
        bool bypassed = false;
    };

    explicit JZRealTracksEngine(float sampleRate = 48000.0f);
    ~JZRealTracksEngine() = default;

    void initialize(float sampleRate);
    void reset();

    // Configuration
    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setStyle(JZRealTracksStyle style);
    JZRealTracksStyle getStyle() const { return m_params.style; }

    void setTempo(double bpm);
    double getTempo() const { return m_params.projectBpm; }

    void setChord(int rootPitchClass, JZRealChordQuality quality);
    int getChordRoot() const { return m_params.currentChordRoot; }
    JZRealChordQuality getChordQuality() const { return m_params.currentChordQuality; }

    void setTrackEnabled(JZRealTrackType track, bool enabled);
    bool isTrackEnabled(JZRealTrackType track) const;

    void setTrackVolume(JZRealTrackType track, float volume);
    float getTrackVolume(JZRealTrackType track) const;

    void setMasterLevel(float level);
    float getMasterLevel() const { return m_params.masterLevel; }

    void setBypass(bool bypass);
    bool isBypassed() const { return m_params.bypassed; }

    // Downbeat / Bar sync
    void syncToBarDownbeat();
    void setPlaybackPositionBeats(double beats);

    // Audio Processing
    void renderAudio(float* outL, float* outR, size_t numFrames);

    // Separate track stem rendering
    void renderTrackAudio(JZRealTrackType track, float* outL, float* outR, size_t numFrames);

    // Telemetry & Diagnostic Getters
    double getStretchRatio() const;
    float getPitchShiftRatio() const;
    std::string getStyleName() const;
    std::string getChordString() const;

private:
    struct AudioPhrase {
        std::vector<float> bufferL;
        std::vector<float> bufferR;
        size_t totalFrames = 0;
        float basePan = 0.0f;
    };

    struct TrackChannel {
        AudioPhrase phrase;
        double playPos = 0.0;
        double grainPhase = 0.0;
        float panL = 0.707f;
        float panR = 0.707f;
    };

    float m_sampleRate = 48000.0f;
    Parameters m_params;

    TrackChannel m_tracks[static_cast<size_t>(JZRealTrackType::COUNT)];

    // WSOLA parameters
    static constexpr size_t WSOLA_GRAIN_SIZE = 1024;
    static constexpr size_t WSOLA_SYN_HOP = 256;
    std::vector<float> m_hannWindow;

    uint32_t m_rngState = 987654321;

    void buildPhraseLibrary();
    void generateTrackPhrase(JZRealTrackType track, AudioPhrase& phrase);
    float xorshiftRandom();
    float resampleHermite(const std::vector<float>& buf, double pos) const;
    int calculateTranspositionSemitones(int root, int baseRoot = 0) const;
};

} // namespace Jazz

namespace jazz {
    using JZRealTracksEngine = Jazz::JZRealTracksEngine;
    using JZRealTracksStyle = Jazz::JZRealTracksStyle;
    using JZRealTrackType = Jazz::JZRealTrackType;
    using JZRealChordQuality = Jazz::JZRealChordQuality;
}

using JZRealTracksEngine = Jazz::JZRealTracksEngine;
using JZRealTracksStyle = Jazz::JZRealTracksStyle;
using JZRealTrackType = Jazz::JZRealTrackType;
using JZRealChordQuality = Jazz::JZRealChordQuality;

#endif // JAZZ_REAL_TRACKS_ENGINE_H
