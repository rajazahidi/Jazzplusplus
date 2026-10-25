//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// RealDrumsEngine.h - Virtual Session Band Acoustic RealDrums Engine
//
// Multi-velocity studio acoustic drum sample triggering & replacement:
// - Multi-velocity dynamic layering (Soft vs Hard hit crossfading)
// - Hi-Hat Choke Group (Closed & Pedal HH cleanly choke ringing Open HH)
// - Round-Robin sample micro-variation (eliminates machine-gun effect)
// - Sympathetic Snare Wire Buzz & Studio Room Ambiance
// - Studio acoustic kit profiles (Vintage Maple, Modern Birch, Jazz Club, Rock Stadium)
//*****************************************************************************

#ifndef JAZZ_REAL_DRUMS_ENGINE_H
#define JAZZ_REAL_DRUMS_ENGINE_H

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <algorithm>
#include <memory>
#include <map>

namespace Jazz {

enum class JZDrumKitType {
    VINTAGE_MAPLE = 0,   // Warm 1970s maple shells, dry punch, intimate studio
    MODERN_BIRCH  = 1,   // Bright scooped attack, aggressive transient snap
    JAZZ_CUSTOM   = 2,   // Dynamic open shells, brush/stick nuance, singing ride
    ROCK_STADIUM  = 3    // Big heavy shells, gated ambient room wash, thunderous kick
};

enum class JZDrumInstrument {
    KICK = 0,
    SNARE,
    SIDE_STICK,
    HIHAT_CLOSED,
    HIHAT_PEDAL,
    HIHAT_OPEN,
    TOM_LOW,
    TOM_MID,
    TOM_HIGH,
    CRASH_1,
    CRASH_2,
    RIDE,
    RIDE_BELL,
    TAMBOURINE,
    COWBELL,
    COUNT
};

class JZRealDrumsEngine {
public:
    struct Parameters {
        JZDrumKitType kitType = JZDrumKitType::VINTAGE_MAPLE;
        float velocitySensitivity = 0.85f;    // 0.1 to 1.0 dynamic scaling
        bool enableChokeGroup = true;         // Open Hi-Hat choked by Closed/Pedal HH
        bool enableRoundRobin = true;         // Anti-machine-gun human micro-variations
        float snareBuzzAmount = 0.25f;        // Sympathetic snare buzz on kick/toms (0.0 to 1.0)
        float roomAmbianceSend = 0.30f;       // Studio room reverb send (0.0 to 1.0)
        float masterLevel = 1.0f;             // Drum bus output gain (0.0 to 2.0)
        bool bypassed = false;
    };

    explicit JZRealDrumsEngine(float sampleRate = 48000.0f);
    ~JZRealDrumsEngine() = default;

    void initialize(float sampleRate);
    void reset();

    // Configuration
    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setKitType(JZDrumKitType kit);
    JZDrumKitType getKitType() const { return m_params.kitType; }

    void setVelocitySensitivity(float sens);
    float getVelocitySensitivity() const { return m_params.velocitySensitivity; }

    void setChokeGroupEnabled(bool enable);
    bool isChokeGroupEnabled() const { return m_params.enableChokeGroup; }

    void setRoundRobinEnabled(bool enable);
    bool isRoundRobinEnabled() const { return m_params.enableRoundRobin; }

    void setSnareBuzzAmount(float amount);
    float getSnareBuzzAmount() const { return m_params.snareBuzzAmount; }

    void setRoomAmbianceSend(float send);
    float getRoomAmbianceSend() const { return m_params.roomAmbianceSend; }

    void setMasterLevel(float level);
    float getMasterLevel() const { return m_params.masterLevel; }

    void setBypass(bool bypass);
    bool isBypassed() const { return m_params.bypassed; }

    // MIDI Drum Triggering
    void noteOn(int midiNote, float velocity = 1.0f);
    void noteOff(int midiNote);
    void chokeHiHat();

    // Audio Rendering
    void renderAudio(float* outL, float* outR, size_t numFrames);

    // Custom Sample Loading
    bool loadCustomWavSample(int midiNote, const std::string& wavPath);

    // Telemetry & Diagnostics
    size_t getActiveVoicesCount() const;
    size_t getTotalHitsCount() const { return m_totalHits; }
    float getLastVelocity() const { return m_lastVelocity; }
    int getLastNote() const { return m_lastNote; }
    std::string getKitName() const;

private:
    struct DrumSamplePair {
        std::vector<float> soft;
        std::vector<float> hard;
        float basePan = 0.0f; // -1.0 (Left) to +1.0 (Right)
        float defaultPitchHz = 100.0f;
    };

    struct DrumVoice {
        bool active = false;
        int midiNote = -1;
        JZDrumInstrument instrument = JZDrumInstrument::KICK;
        const DrumSamplePair* sample = nullptr;
        float playPos = 0.0f;
        float playInc = 1.0f;
        float gainSoft = 0.0f;
        float gainHard = 0.0f;
        float panL = 0.707f;
        float panR = 0.707f;
        bool choking = false;
        float chokeGain = 1.0f;
        float snareBuzzGain = 0.0f;
        float snareBuzzPos = 0.0f;
    };

    static constexpr size_t MAX_VOICES = 32;
    DrumVoice m_voices[MAX_VOICES];

    float m_sampleRate = 48000.0f;
    Parameters m_params;

    // Built-in studio acoustic samples for each instrument
    DrumSamplePair m_samples[static_cast<size_t>(JZDrumInstrument::COUNT)];
    std::vector<float> m_snareWireNoise; // Shared sympathetic snare wire noise

    // Simple early-reflection room reverb line
    std::vector<float> m_roomDelayBufferL;
    std::vector<float> m_roomDelayBufferR;
    size_t m_roomDelayPos = 0;

    size_t m_totalHits = 0;
    float m_lastVelocity = 0.0f;
    int m_lastNote = 0;
    uint32_t m_rngState = 123456789;

    void buildSampleLibrary();
    void generateInstrumentWaveforms(JZDrumInstrument inst, DrumSamplePair& pair);
    JZDrumInstrument midiNoteToInstrument(int note) const;
    float xorshiftRandom();
};

} // namespace Jazz

namespace jazz {
    using JZRealDrumsEngine = Jazz::JZRealDrumsEngine;
    using JZDrumKitType = Jazz::JZDrumKitType;
    using JZDrumInstrument = Jazz::JZDrumInstrument;
}

using JZRealDrumsEngine = Jazz::JZRealDrumsEngine;
using JZDrumKitType = Jazz::JZDrumKitType;
using JZDrumInstrument = Jazz::JZDrumInstrument;

#endif // JAZZ_REAL_DRUMS_ENGINE_H
