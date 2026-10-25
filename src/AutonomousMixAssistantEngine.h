//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZAutonomousMixAssistantEngine.h - Autonomous AI Multi-Track Mix Assistant
//
// Intelligent multi-track mix balancing & spectral masking resolution:
// Simultaneous multi-bus spectral cross-entropy monitoring, collision detection
// (Kick vs Bass at 60Hz, Guitars vs Vocals at 2kHz), automated dynamic sidechain
// frequency carving, and fader balance leveling recommendations.
//*****************************************************************************

#ifndef JAZZ_AUTONOMOUS_MIX_ASSISTANT_ENGINE_H
#define JAZZ_AUTONOMOUS_MIX_ASSISTANT_ENGINE_H

#include <vector>
#include <string>
#include <map>
#include <cstdint>

namespace Jazz {

enum class MixTrackRole {
    KICK_DRUM,
    BASS_INSTRUMENT,
    LEAD_VOCAL,
    GUITARS_SYNTHS,
    DRUM_OVERHEADS,
    BACKING_TRACKS
};

struct SpectralMaskingCollision {
    int dominantTrackId;
    std::string dominantTrackName;
    int maskedTrackId;
    std::string maskedTrackName;
    float centerFrequencyHz;
    float maskingSeverity;         // 0.0 (no clash) to 1.0 (severe masking clash)
    float recommendedCarveDb;      // e.g. -3.5 dB dynamic EQ dip
    float recommendedQ;            // e.g. 2.5
};

struct TrackMixProfile {
    int trackId;
    std::string trackName;
    MixTrackRole role;
    float currentRmsDb;
    float currentPeakDb;
    float recommendedFaderTrimDb;
    float subBassEnergy;           // 40Hz - 100Hz
    float lowMidEnergy;            // 200Hz - 500Hz
    float vocalPresenceEnergy;     // 1.5kHz - 3.5kHz
    float highAirEnergy;           // 6kHz - 12kHz
};

class JZAutonomousMixAssistantEngine {
public:
    JZAutonomousMixAssistantEngine(float sampleRate = 48000.0f);
    ~JZAutonomousMixAssistantEngine() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return m_sampleRate; }

    // Multi-Track Bus Registration
    int registerTrack(const std::string& name, MixTrackRole role);
    bool unregisterTrack(int trackId);
    size_t getTrackCount() const { return m_tracks.size(); }

    // Ingest Multi-Track Audio Frame for Real-Time Analysis
    void ingestTrackAudio(int trackId, const float* audioData, size_t numSamples);

    // Analyze Global Mix & Detect Spectral Masking Collisions
    void analyzeMixCollisions();
    const std::vector<SpectralMaskingCollision>& getDetectedCollisions() const { return m_collisions; }

    const TrackMixProfile* getTrackProfile(int trackId) const;

    // Apply Recommended Dynamic Sidechain Carve to a Track's Audio
    void applyDynamicCarve(int trackId, float* audioData, size_t numSamples);

    void reset();

private:
    float m_sampleRate;
    int m_nextTrackId;
    std::map<int, TrackMixProfile> m_tracks;
    std::vector<SpectralMaskingCollision> m_collisions;
};

} // namespace Jazz

#endif // JAZZ_AUTONOMOUS_MIX_ASSISTANT_ENGINE_H
