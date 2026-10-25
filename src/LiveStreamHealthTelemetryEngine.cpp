#include "LiveStreamHealthTelemetryEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

JZLiveStreamHealthTelemetryEngine::JZLiveStreamHealthTelemetryEngine() {
    initialize(48000.0f, 4096);
}

void JZLiveStreamHealthTelemetryEngine::initialize(float sampleRate, size_t bufferCapacity) {
    if (sampleRate < 8000.0f) sampleRate = 48000.0f;
    if (bufferCapacity < 256) bufferCapacity = 256;

    m_sampleRate = sampleRate;
    m_bufferCapacity = bufferCapacity;

    m_streamA.reset(m_bufferCapacity);
    m_streamB.reset(m_bufferCapacity);

    m_activeStream = StreamId::PrimaryA;
    m_previousStream = StreamId::PrimaryA;
    m_failoverEvents = 0;
    m_inCrossfade = false;
    m_crossfadeSampleIndex = 0;

    m_crossfadeSamplesTotal = static_cast<size_t>((m_config.crossfadeTimeMs / 1000.0f) * m_sampleRate);
    if (m_crossfadeSamplesTotal < 4) m_crossfadeSamplesTotal = 4;
}

void JZLiveStreamHealthTelemetryEngine::reset() {
    m_streamA.reset(m_bufferCapacity);
    m_streamB.reset(m_bufferCapacity);
    m_activeStream = StreamId::PrimaryA;
    m_previousStream = StreamId::PrimaryA;
    m_failoverEvents = 0;
    m_inCrossfade = false;
    m_crossfadeSampleIndex = 0;
}

void JZLiveStreamHealthTelemetryEngine::setConfiguration(const Configuration& config) {
    m_config = config;
    m_crossfadeSamplesTotal = static_cast<size_t>((m_config.crossfadeTimeMs / 1000.0f) * m_sampleRate);
    if (m_crossfadeSamplesTotal < 4) m_crossfadeSamplesTotal = 4;
}

void JZLiveStreamHealthTelemetryEngine::updateTelemetry(StreamBuffer& stream,
                                                       const StreamFrameHeader& header,
                                                       size_t numSamples) {
    stream.telemetry.totalFramesReceived++;

    // 1. Sequence Drop Check
    if (stream.telemetry.totalFramesReceived > 1) {
        if (header.sequenceNumber > stream.lastSeq + 1) {
            uint32_t lost = header.sequenceNumber - (stream.lastSeq + 1);
            stream.telemetry.droppedFrames += lost;
            stream.consecutiveErrors += lost;
            stream.consecutiveHealthyFrames = 0;
        } else if (header.sequenceNumber <= stream.lastSeq && header.sequenceNumber != 0) {
            // Out of order frame
            stream.telemetry.droppedFrames++;
        }
    }
    stream.lastSeq = header.sequenceNumber;

    // 2. Parity & CRC Check
    if (header.parityError) {
        stream.telemetry.parityErrors++;
        stream.consecutiveErrors++;
        stream.consecutiveHealthyFrames = 0;
    }
    if (header.crcError) {
        stream.telemetry.crcErrors++;
        stream.consecutiveErrors++;
        stream.consecutiveHealthyFrames = 0;
    }

    // 3. Jitter calculation (based on timestamp delta)
    if (stream.lastTimestampNs > 0 && header.timestampNs > stream.lastTimestampNs) {
        uint64_t actualDeltaNs = header.timestampNs - stream.lastTimestampNs;
        uint64_t expectedDeltaNs = static_cast<uint64_t>((static_cast<double>(numSamples) / m_sampleRate) * 1e9);

        double diffNs = std::abs(static_cast<double>(actualDeltaNs) - static_cast<double>(expectedDeltaNs));
        float jitterUs = static_cast<float>(diffNs / 1000.0);
        float jitterPpm = static_cast<float>((diffNs / static_cast<double>(expectedDeltaNs)) * 1e6);

        // Exponential smoothing filter for jitter
        stream.telemetry.clockJitterUs = stream.telemetry.clockJitterUs * 0.85f + jitterUs * 0.15f;
        stream.telemetry.clockJitterPpm = stream.telemetry.clockJitterPpm * 0.85f + jitterPpm * 0.15f;
    }
    stream.lastTimestampNs = header.timestampNs;

    // 4. Update health state
    if (!header.validityBit) {
        stream.consecutiveErrors++;
        stream.consecutiveHealthyFrames = 0;
    } else if (!header.parityError && !header.crcError) {
        stream.consecutiveHealthyFrames++;
        if (stream.consecutiveErrors > 0) stream.consecutiveErrors--;
    }

    stream.telemetry.isLocked = (stream.count > 0) && (stream.telemetry.clockJitterPpm <= m_config.maxAllowedJitterPpm * 1.5f);

    if (stream.consecutiveErrors > 6) {
        stream.telemetry.status = LinkStatus::Glitching;
    } else if (stream.telemetry.clockJitterPpm > m_config.maxAllowedJitterPpm ||
               stream.consecutiveErrors > 0 || !header.validityBit) {
        stream.telemetry.status = LinkStatus::Degraded;
    } else {
        stream.telemetry.status = LinkStatus::OK;
    }
}

void JZLiveStreamHealthTelemetryEngine::ingestStreamBlock(StreamId id,
                                                         const float* left,
                                                         const float* right,
                                                         size_t numSamples,
                                                         const StreamFrameHeader& header) {
    if (!left || numSamples == 0) return;

    StreamBuffer& stream = (id == StreamId::PrimaryA) ? m_streamA : m_streamB;

    // Push into ring buffer
    for (size_t i = 0; i < numSamples; ++i) {
        if (stream.count >= m_bufferCapacity) {
            // Buffer overrun - slip oldest sample
            stream.readPos = (stream.readPos + 1) % m_bufferCapacity;
            stream.count--;
            stream.telemetry.slippedSamples++;
        }

        stream.bufL[stream.writePos] = left[i];
        stream.bufR[stream.writePos] = right ? right[i] : left[i];
        stream.writePos = (stream.writePos + 1) % m_bufferCapacity;
        stream.count++;
    }

    stream.telemetry.bufferFillPct = (static_cast<float>(stream.count) / static_cast<float>(m_bufferCapacity)) * 100.0f;

    updateTelemetry(stream, header, numSamples);
    evaluateFailover();
}

void JZLiveStreamHealthTelemetryEngine::evaluateFailover() {
    if (!m_config.autoFailoverEnabled) return;

    if (m_activeStream == StreamId::PrimaryA) {
        // Evaluate switching to SecondaryB
        bool aIsBad = (m_streamA.telemetry.status == LinkStatus::Glitching ||
                       m_streamA.telemetry.status == LinkStatus::Disconnected ||
                       m_streamA.consecutiveErrors >= m_config.failoverHysteresisFrames);
        bool bIsGood = (m_streamB.telemetry.status == LinkStatus::OK ||
                        m_streamB.telemetry.status == LinkStatus::Degraded) &&
                       (m_streamB.count > 0);

        if (aIsBad && bIsGood) {
            forceActiveStream(StreamId::SecondaryB);
        }
    } else {
        // Currently on SecondaryB, check if PrimaryA is solidly healthy again
        bool aIsRestored = (m_streamA.telemetry.status == LinkStatus::OK &&
                            m_streamA.consecutiveHealthyFrames >= (m_config.failoverHysteresisFrames * 2) &&
                            m_streamA.count > 0);
        bool bIsBad = (m_streamB.telemetry.status == LinkStatus::Glitching ||
                       m_streamB.telemetry.status == LinkStatus::Disconnected);

        if (bIsBad || aIsRestored) {
            forceActiveStream(StreamId::PrimaryA);
        }
    }
}

void JZLiveStreamHealthTelemetryEngine::forceActiveStream(StreamId id) {
    if (m_activeStream != id) {
        m_previousStream = m_activeStream;
        m_activeStream = id;
        m_inCrossfade = true;
        m_crossfadeSampleIndex = 0;
        m_failoverEvents++;
    }
}

void JZLiveStreamHealthTelemetryEngine::processOutput(float* outL, float* outR, size_t numSamples) {
    if (!outL || numSamples == 0) return;

    StreamBuffer& active = (m_activeStream == StreamId::PrimaryA) ? m_streamA : m_streamB;
    StreamBuffer& prev = (m_previousStream == StreamId::PrimaryA) ? m_streamA : m_streamB;

    for (size_t i = 0; i < numSamples; ++i) {
        float sampleL = 0.0f;
        float sampleR = 0.0f;

        if (active.count > 0) {
            sampleL = active.bufL[active.readPos];
            sampleR = active.bufR[active.readPos];
            active.readPos = (active.readPos + 1) % m_bufferCapacity;
            active.count--;
        } else {
            active.telemetry.slippedSamples++;
        }

        // Apply hitless crossfade if switching streams
        if (m_inCrossfade) {
            float fadeProgress = static_cast<float>(m_crossfadeSampleIndex) / static_cast<float>(m_crossfadeSamplesTotal);
            float prevSampleL = 0.0f;
            float prevSampleR = 0.0f;

            if (prev.count > 0) {
                prevSampleL = prev.bufL[prev.readPos];
                prevSampleR = prev.bufR[prev.readPos];
                prev.readPos = (prev.readPos + 1) % m_bufferCapacity;
                prev.count--;
            }

            sampleL = prevSampleL * (1.0f - fadeProgress) + sampleL * fadeProgress;
            sampleR = prevSampleR * (1.0f - fadeProgress) + sampleR * fadeProgress;

            m_crossfadeSampleIndex++;
            if (m_crossfadeSampleIndex >= m_crossfadeSamplesTotal) {
                m_inCrossfade = false;
            }
        }

        outL[i] = sampleL;
        if (outR) outR[i] = sampleR;
    }

    active.telemetry.bufferFillPct = (static_cast<float>(active.count) / static_cast<float>(m_bufferCapacity)) * 100.0f;
}

JZLiveStreamHealthTelemetryEngine::StreamTelemetry
JZLiveStreamHealthTelemetryEngine::getTelemetry(StreamId id) const {
    return (id == StreamId::PrimaryA) ? m_streamA.telemetry : m_streamB.telemetry;
}

} // namespace Jazz
