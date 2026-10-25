//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// PtpClockPrecisionSyncEngine.cpp - IEEE 1588v2 PTP & SMPTE ST 2110-30 Synchronizer
//*****************************************************************************

#include "PtpClockPrecisionSyncEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

JZPtpClockPrecisionSyncEngine::JZPtpClockPrecisionSyncEngine(float sampleRate) {
    initialize(sampleRate);
}

void JZPtpClockPrecisionSyncEngine::initialize(float sampleRate) {
    if (sampleRate < 8000.0f) sampleRate = 48000.0f;
    m_sampleRate = sampleRate;
    reset();
}

void JZPtpClockPrecisionSyncEngine::reset() {
    m_telemetry = Telemetry();
    m_pllPhaseIntegrator = 0.0;
    m_pllFreqIntegrator = 0.0;
    m_meanPathDelayNs = 2500;
    m_consecutiveLockedBlocks = 0;
}

void JZPtpClockPrecisionSyncEngine::ingestPtpSync(const PtpPacketHeader& header, uint64_t localRxNs) {
    m_telemetry.syncMessageCount++;

    // IEEE 1588v2 timestamp calculations: t1 = originTimestampNs, t2 = localRxNs
    double t1 = static_cast<double>(header.originTimestampNs);
    double t2 = static_cast<double>(localRxNs);

    // Raw phase error offset relative to Grandmaster
    double rawOffsetNs = (t2 - t1) - static_cast<double>(m_meanPathDelayNs);

    // Dual-Loop Proportional-Integral Phase-Locked Loop (PLL)
    double error = rawOffsetNs;
    m_pllFreqIntegrator += m_ki * error * 0.001;
    // Clamp integrator to +/- 100 ppm
    m_pllFreqIntegrator = std::max(-100.0, std::min(100.0, m_pllFreqIntegrator));

    double correctionPpm = (m_kp * error * 0.001) + m_pllFreqIntegrator;
    correctionPpm = std::max(-100.0, std::min(100.0, correctionPpm));

    m_telemetry.phaseOffsetNs = m_telemetry.phaseOffsetNs * 0.7 + error * 0.3;
    m_telemetry.frequencyDriftPpm = m_telemetry.frequencyDriftPpm * 0.8 + correctionPpm * 0.2;
    m_telemetry.currentTaiTimestampNs = header.originTimestampNs;

    // State machine updates
    if (m_telemetry.syncMessageCount == 1) {
        m_telemetry.state = PtpSyncState::LISTENING;
    } else if (std::abs(m_telemetry.phaseOffsetNs) > 200.0) {
        m_telemetry.state = PtpSyncState::LOCKING;
        m_consecutiveLockedBlocks = 0;
        m_telemetry.pllLocked = false;
    } else {
        m_consecutiveLockedBlocks++;
        if (m_consecutiveLockedBlocks >= 3) {
            m_telemetry.state = PtpSyncState::LOCKED_LATCHED;
            m_telemetry.pllLocked = true;
        } else {
            m_telemetry.state = PtpSyncState::LOCKING;
        }
    }
}

void JZPtpClockPrecisionSyncEngine::ingestDelayResponse(uint64_t localTxNs, uint64_t masterRxNs) {
    if (masterRxNs > localTxNs) {
        uint64_t roundTripDelayNs = masterRxNs - localTxNs;
        uint64_t oneWayDelayNs = roundTripDelayNs / 2;
        if (oneWayDelayNs < 10000000ULL) { // Filter unrealistic spikes (> 10ms)
            m_meanPathDelayNs = static_cast<uint64_t>(static_cast<double>(m_meanPathDelayNs) * 0.9 + static_cast<double>(oneWayDelayNs) * 0.1);
        }
    }
}

void JZPtpClockPrecisionSyncEngine::advanceMediaClock(size_t numSamples) {
    if (numSamples == 0) return;

    double adjustedSampleRate = static_cast<double>(m_sampleRate) * (1.0 + m_telemetry.frequencyDriftPpm * 1e-6);
    double elapsedNs = (static_cast<double>(numSamples) / adjustedSampleRate) * 1e9;

    m_telemetry.currentTaiTimestampNs += static_cast<uint64_t>(elapsedNs);
    m_telemetry.rtpMediaClockSamples += static_cast<uint32_t>(numSamples);
}

} // namespace Jazz
