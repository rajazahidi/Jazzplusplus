#ifndef JAZZ_BROADCAST_FAILOVER_ENGINE_H
#define JAZZ_BROADCAST_FAILOVER_ENGINE_H

#include <vector>
#include <string>
#include <cmath>

namespace Jazz {

enum class FailoverStream {
    PRIMARY,
    SECONDARY
};

enum class FailoverTriggerReason {
    NONE,
    SILENCE_DROPOUT,
    BUFFER_FREEZE_STALL,
    DISCONTINUITY_SPIKE,
    MANUAL_OPERATOR_SWITCH
};

class JZBroadcastFailoverEngine {
public:
    JZBroadcastFailoverEngine(float sampleRate = 44100.0f);
    ~JZBroadcastFailoverEngine() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return m_sampleRate; }

    // Thresholds & Timings
    void setSilenceThresholdDb(float threshDb); // default -60.0 dB
    float getSilenceThresholdDb() const { return m_silenceThreshDb; }

    void setDropoutTimeoutMs(float timeoutMs); // default 4.0 ms (fast failover)
    float getDropoutTimeoutMs() const { return m_timeoutMs; }

    void setCrossfadeTimeMs(float crossfadeMs); // default 2.0 ms (zero-click)
    float getCrossfadeTimeMs() const { return m_crossfadeMs; }

    void setAutoFailoverEnabled(bool enable) { m_autoFailoverEnabled = enable; }
    bool isAutoFailoverEnabled() const { return m_autoFailoverEnabled; }

    // Manual Force Switch
    void forceSwitchTo(FailoverStream stream);

    // Real-Time Audio Ingestion & Failover Switching
    // Inputs: Primary stream and Secondary backup stream
    // Outputs: Safeguarded reliable broadcast audio
    void process(float primL, float primR, float secL, float secR, float& outL, float& outR);
    void processBlock(const float* primL, const float* primR,
                      const float* secL, const float* secR,
                      float* outL, float* outR, int numSamples);

    // Telemetry & Health Monitoring
    FailoverStream getActiveStream() const { return m_activeStream; }
    FailoverTriggerReason getLastTriggerReason() const { return m_lastReason; }
    int getFailoverCount() const { return m_failoverCount; }
    bool isPrimaryHealthy() const { return m_primaryHealthy; }
    bool isSecondaryHealthy() const { return m_secondaryHealthy; }
    float getPrimaryRmsDb() const;
    float getSecondaryRmsDb() const;

    void reset();

private:
    float dbToLinear(float db) const;
    float linearToDb(float lin) const;

    float m_sampleRate;
    float m_silenceThreshDb;
    float m_timeoutMs;
    float m_crossfadeMs;
    bool m_autoFailoverEnabled;

    FailoverStream m_activeStream;
    FailoverTriggerReason m_lastReason;
    int m_failoverCount;

    // Stream health analyzers
    bool m_primaryHealthy;
    bool m_secondaryHealthy;

    int m_primarySilentSamples;
    int m_secondarySilentSamples;
    int m_timeoutSamples;

    float m_prevPrimL, m_prevPrimR;
    int m_primStallCount;

    // Crossfade interpolation state (0.0 = 100% Primary, 1.0 = 100% Secondary)
    float m_crossfadePos;
    float m_crossfadeInc;

    float m_primaryRms;
    float m_secondaryRms;
};

} // namespace Jazz

#endif // JAZZ_BROADCAST_FAILOVER_ENGINE_H
