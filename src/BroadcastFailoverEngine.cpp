#include "BroadcastFailoverEngine.h"
#include <algorithm>
#include <cmath>

namespace Jazz {

static const float PI = 3.14159265358979323846f;
static const float EPSILON = 1e-9f;

JZBroadcastFailoverEngine::JZBroadcastFailoverEngine(float sampleRate)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f)
    , m_silenceThreshDb(-60.0f)
    , m_timeoutMs(4.0f)
    , m_crossfadeMs(2.0f)
    , m_autoFailoverEnabled(true)
    , m_activeStream(FailoverStream::PRIMARY)
    , m_lastReason(FailoverTriggerReason::NONE)
    , m_failoverCount(0)
    , m_primaryHealthy(true)
    , m_secondaryHealthy(true)
    , m_primarySilentSamples(0)
    , m_secondarySilentSamples(0)
    , m_prevPrimL(0.0f), m_prevPrimR(0.0f)
    , m_primStallCount(0)
    , m_crossfadePos(0.0f)
    , m_primaryRms(0.0f)
    , m_secondaryRms(0.0f)
{
    m_timeoutSamples = static_cast<int>((m_timeoutMs * 0.001f) * m_sampleRate);
    m_crossfadeInc = 1.0f / std::max(1.0f, (m_crossfadeMs * 0.001f) * m_sampleRate);
}

void JZBroadcastFailoverEngine::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        m_sampleRate = sampleRate;
        m_timeoutSamples = static_cast<int>((m_timeoutMs * 0.001f) * m_sampleRate);
        m_crossfadeInc = 1.0f / std::max(1.0f, (m_crossfadeMs * 0.001f) * m_sampleRate);
    }
}

float JZBroadcastFailoverEngine::dbToLinear(float db) const {
    return std::pow(10.0f, db / 20.0f);
}

float JZBroadcastFailoverEngine::linearToDb(float lin) const {
    if (lin <= EPSILON) return -96.0f;
    return 20.0f * std::log10(lin);
}

void JZBroadcastFailoverEngine::setSilenceThresholdDb(float threshDb) {
    m_silenceThreshDb = std::max(-90.0f, std::min(-20.0f, threshDb));
}

void JZBroadcastFailoverEngine::setDropoutTimeoutMs(float timeoutMs) {
    m_timeoutMs = std::max(0.5f, std::min(50.0f, timeoutMs));
    m_timeoutSamples = static_cast<int>((m_timeoutMs * 0.001f) * m_sampleRate);
}

void JZBroadcastFailoverEngine::setCrossfadeTimeMs(float crossfadeMs) {
    m_crossfadeMs = std::max(0.5f, std::min(20.0f, crossfadeMs));
    m_crossfadeInc = 1.0f / std::max(1.0f, (m_crossfadeMs * 0.001f) * m_sampleRate);
}

void JZBroadcastFailoverEngine::forceSwitchTo(FailoverStream stream) {
    if (m_activeStream != stream) {
        m_activeStream = stream;
        m_lastReason = FailoverTriggerReason::MANUAL_OPERATOR_SWITCH;
        m_failoverCount++;
    }
}

float JZBroadcastFailoverEngine::getPrimaryRmsDb() const {
    return linearToDb(m_primaryRms);
}

float JZBroadcastFailoverEngine::getSecondaryRmsDb() const {
    return linearToDb(m_secondaryRms);
}

void JZBroadcastFailoverEngine::reset() {
    m_activeStream = FailoverStream::PRIMARY;
    m_lastReason = FailoverTriggerReason::NONE;
    m_failoverCount = 0;
    m_primaryHealthy = true;
    m_secondaryHealthy = true;
    m_primarySilentSamples = 0;
    m_secondarySilentSamples = 0;
    m_prevPrimL = m_prevPrimR = 0.0f;
    m_primStallCount = 0;
    m_crossfadePos = 0.0f;
    m_primaryRms = 0.0f;
    m_secondaryRms = 0.0f;
}

void JZBroadcastFailoverEngine::process(float primL, float primR, float secL, float secR, float& outL, float& outR) {
    float silLin = dbToLinear(m_silenceThreshDb);

    // 1. Sanity check: NaNs or Infinities
    bool primCorrupted = std::isnan(primL) || std::isnan(primR) || std::isinf(primL) || std::isinf(primR);
    if (primCorrupted) {
        primL = primR = 0.0f;
    }

    // 2. Buffer Freeze / DC Stall detection
    if (std::abs(primL - m_prevPrimL) < 1e-7f && std::abs(primR - m_prevPrimR) < 1e-7f && std::abs(primL) > 1e-4f) {
        m_primStallCount++;
    } else {
        m_primStallCount = 0;
    }
    m_prevPrimL = primL;
    m_prevPrimR = primR;

    // 3. RMS tracking
    float primMag = 0.5f * (std::abs(primL) + std::abs(primR));
    float secMag  = 0.5f * (std::abs(secL)  + std::abs(secR));

    m_primaryRms   = 0.99f * m_primaryRms   + 0.01f * primMag;
    m_secondaryRms = 0.99f * m_secondaryRms + 0.01f * secMag;

    // 4. Silence detection
    if (primMag < silLin) {
        m_primarySilentSamples++;
    } else {
        m_primarySilentSamples = 0;
    }

    if (secMag < silLin) {
        m_secondarySilentSamples++;
    } else {
        m_secondarySilentSamples = 0;
    }

    m_secondaryHealthy = (m_secondarySilentSamples < m_timeoutSamples);

    // 5. Auto-Failover Logic
    if (m_autoFailoverEnabled && m_activeStream == FailoverStream::PRIMARY) {
        bool shouldFailover = false;
        FailoverTriggerReason reason = FailoverTriggerReason::NONE;

        if (primCorrupted) {
            shouldFailover = true;
            reason = FailoverTriggerReason::DISCONTINUITY_SPIKE;
        } else if (m_primStallCount >= 48) { // ~1 ms stall
            shouldFailover = true;
            reason = FailoverTriggerReason::BUFFER_FREEZE_STALL;
        } else if (m_primarySilentSamples >= m_timeoutSamples && m_secondaryHealthy) {
            shouldFailover = true;
            reason = FailoverTriggerReason::SILENCE_DROPOUT;
        }

        if (shouldFailover) {
            m_activeStream = FailoverStream::SECONDARY;
            m_lastReason = reason;
            m_failoverCount++;
            m_primaryHealthy = false;
        }
    } else if (m_activeStream == FailoverStream::SECONDARY && m_primarySilentSamples == 0 && !primCorrupted) {
        m_primaryHealthy = true;
    }

    // 6. Seamless Equal-Power Crossfade
    float targetPos = (m_activeStream == FailoverStream::SECONDARY) ? 1.0f : 0.0f;
    if (m_crossfadePos < targetPos) {
        m_crossfadePos = std::min(targetPos, m_crossfadePos + m_crossfadeInc);
    } else if (m_crossfadePos > targetPos) {
        m_crossfadePos = std::max(targetPos, m_crossfadePos - m_crossfadeInc);
    }

    float angle = m_crossfadePos * (PI * 0.5f);
    float wPrim = std::cos(angle);
    float wSec  = std::sin(angle);

    outL = primL * wPrim + secL * wSec;
    outR = primR * wPrim + secR * wSec;
}

void JZBroadcastFailoverEngine::processBlock(const float* primL, const float* primR,
                                            const float* secL, const float* secR,
                                            float* outL, float* outR, int numSamples) {
    if (!primL || !primR || !secL || !secR || !outL || !outR || numSamples <= 0) return;
    for (int i = 0; i < numSamples; ++i) {
        process(primL[i], primR[i], secL[i], secR[i], outL[i], outR[i]);
    }
}

} // namespace Jazz
