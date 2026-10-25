//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// PtpClockPrecisionSyncEngine.h - IEEE 1588v2 PTP & SMPTE ST 2110-30 Synchronizer
//
// Television & broadcast network audio clock synchronization:
// IEEE 1588v2 Precision Time Protocol (PTP) software Phase-Locked Loop (PLL),
// nanosecond phase offset tracking, SMPTE ST 2110-30 / AES67 media clock synthesis.
//*****************************************************************************

#ifndef JAZZ_PTP_CLOCK_PRECISION_SYNC_ENGINE_H
#define JAZZ_PTP_CLOCK_PRECISION_SYNC_ENGINE_H

#include <vector>
#include <string>
#include <cstddef>
#include <cstdint>
#include <cmath>

namespace Jazz {

enum class PtpSyncState {
    UNSYNCED = 0,
    LISTENING = 1,
    LOCKING = 2,
    LOCKED_LATCHED = 3
};

class JZPtpClockPrecisionSyncEngine {
public:
    struct PtpPacketHeader {
        uint16_t sequenceId = 0;
        uint64_t originTimestampNs = 0;
        uint8_t grandmasterPriority1 = 128;
        std::string grandmasterIdentity = "00:1E:C9:FF:FE:84:3B:12";
    };

    struct Telemetry {
        PtpSyncState state = PtpSyncState::UNSYNCED;
        double phaseOffsetNs = 0.0;          // Nanoseconds offset from Grandmaster
        double frequencyDriftPpm = 0.0;       // Clock frequency drift in PPM
        uint64_t currentTaiTimestampNs = 0;   // Synchronized TAI broadcast timestamp
        uint32_t rtpMediaClockSamples = 0;    // SMPTE ST 2110-30 RTP sample count
        uint32_t syncMessageCount = 0;
        bool pllLocked = false;
    };

    JZPtpClockPrecisionSyncEngine(float sampleRate = 48000.0f);
    ~JZPtpClockPrecisionSyncEngine() = default;

    void initialize(float sampleRate);
    void reset();

    /**
     * @brief Ingest incoming PTP Sync / Follow-Up timestamp packet from Grandmaster.
     */
    void ingestPtpSync(const PtpPacketHeader& header, uint64_t localRxNs);

    /**
     * @brief Ingest PTP Delay-Response message to calculate round-trip path delay.
     */
    void ingestDelayResponse(uint64_t localTxNs, uint64_t masterRxNs);

    /**
     * @brief Advance software audio media clock PLL by sample buffer block.
     */
    void advanceMediaClock(size_t numSamples);

    Telemetry getTelemetry() const { return m_telemetry; }
    PtpSyncState getSyncState() const { return m_telemetry.state; }
    bool isLocked() const { return m_telemetry.pllLocked; }
    double getPhaseOffsetNs() const { return m_telemetry.phaseOffsetNs; }
    double getFrequencyDriftPpm() const { return m_telemetry.frequencyDriftPpm; }

private:
    float m_sampleRate = 48000.0f;
    Telemetry m_telemetry;

    // Software Dual-Loop PLL parameters
    double m_pllPhaseIntegrator = 0.0;
    double m_pllFreqIntegrator = 0.0;
    double m_kp = 0.12;  // Proportional gain
    double m_ki = 0.005; // Integral gain

    uint64_t m_meanPathDelayNs = 2500; // Estimated 2.5us network switch transit
    uint32_t m_consecutiveLockedBlocks = 0;
};

} // namespace Jazz

namespace jazz {
    using JZPtpClockPrecisionSyncEngine = Jazz::JZPtpClockPrecisionSyncEngine;
}

#endif // JAZZ_PTP_CLOCK_PRECISION_SYNC_ENGINE_H
