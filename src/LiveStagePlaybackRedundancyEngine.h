//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// LiveStagePlaybackRedundancyEngine.h - Live Stage Dual-Rig Playback Redundancy
//
// Automated concert stage playback failover (Radial SW8 / PlayAUDIO12 style):
// Heartbeat pilot tone detection, clickless audio crossfade (<2ms), and
// MIDI All-Notes-Off panic flush to eliminate stuck synth notes during live failure.
//*****************************************************************************

#ifndef JAZZ_LIVE_STAGE_PLAYBACK_REDUNDANCY_ENGINE_H
#define JAZZ_LIVE_STAGE_PLAYBACK_REDUNDANCY_ENGINE_H

#include <vector>
#include <string>
#include <cstddef>
#include <cstdint>

namespace Jazz {

enum class StageRigId {
    PRIMARY_RIG_A = 0,
    BACKUP_RIG_B  = 1
};

class JZLiveStagePlaybackRedundancyEngine {
public:
    struct Configuration {
        float pilotToneFreqHz = 1000.0f;    // Heartbeat pilot tone frequency
        float dropoutThresholdMs = 2.5f;     // Failure detection latency
        float crossfadeTimeMs = 1.5f;        // Clickless audio crossfade
        bool autoFailoverEnabled = true;
        bool sendMidiPanicOnSwitch = true;   // Flush CC 120 & CC 123
    };

    struct MidiMessage {
        uint8_t status;
        uint8_t data1;
        uint8_t data2;
    };

    JZLiveStagePlaybackRedundancyEngine(float sampleRate = 48000.0f);
    ~JZLiveStagePlaybackRedundancyEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setConfiguration(const Configuration& config);
    const Configuration& getConfiguration() const { return m_config; }

    /**
     * @brief Ingest audio from Primary Rig A or Backup Rig B.
     */
    void ingestRigAudio(StageRigId rig, const float* inL, const float* inR, size_t numSamples);

    /**
     * @brief Ingest dedicated heartbeat pilot tone from Primary Rig A.
     */
    void ingestHeartbeatTone(const float* pilotSamples, size_t numSamples);

    /**
     * @brief Generate master house PA output with failover protection.
     */
    void processMasterOutput(float* outL, float* outR, size_t numSamples);

    /**
     * @brief Poll flushed MIDI panic events (CC 123 All Notes Off, CC 120 All Sound Off).
     */
    std::vector<MidiMessage> pollMidiPanicEvents();

    StageRigId getActiveRig() const { return m_activeRig; }
    void forceRigSwitch(StageRigId rig);

    bool isHeartbeatAlive() const { return m_heartbeatAlive; }
    float getPilotToneRms() const { return m_pilotToneRms; }
    uint32_t getFailoverCount() const { return m_failoverCount; }

private:
    float m_sampleRate = 48000.0f;
    Configuration m_config;

    StageRigId m_activeRig = StageRigId::PRIMARY_RIG_A;
    StageRigId m_previousRig = StageRigId::PRIMARY_RIG_A;

    // Ring buffers for Rig A and Rig B
    std::vector<float> m_bufAL, m_bufAR;
    std::vector<float> m_bufBL, m_bufBR;
    size_t m_writePosA = 0, m_readPosA = 0, m_countA = 0;
    size_t m_writePosB = 0, m_readPosB = 0, m_countB = 0;
    static const size_t BUFFER_CAPACITY = 2048;

    // Heartbeat pilot tone detector (Goertzel or envelope follower on resonant bandpass)
    float m_pilotBiquadX1 = 0.0f, m_pilotBiquadX2 = 0.0f;
    float m_pilotBiquadY1 = 0.0f, m_pilotBiquadY2 = 0.0f;
    float m_pilotB0 = 1.0f, m_pilotB1 = 0.0f, m_pilotB2 = -1.0f;
    float m_pilotA1 = 0.0f, m_pilotA2 = 0.0f;

    float m_pilotToneRms = 0.0f;
    size_t m_silenceSampleCounter = 0;
    size_t m_dropoutThresholdSamples = 120;
    bool m_heartbeatAlive = true;
    uint32_t m_failoverCount = 0;

    // Crossfade state
    bool m_inCrossfade = false;
    size_t m_crossfadeIndex = 0;
    size_t m_crossfadeTotalSamples = 72;

    // MIDI panic queue
    std::vector<MidiMessage> m_midiPanicQueue;

    void updateFilter();
    void triggerFailover(StageRigId targetRig);
    void generateMidiPanicFlush();
};

} // namespace Jazz

namespace jazz {
    using JZLiveStagePlaybackRedundancyEngine = Jazz::JZLiveStagePlaybackRedundancyEngine;
}

#endif // JAZZ_LIVE_STAGE_PLAYBACK_REDUNDANCY_ENGINE_H
