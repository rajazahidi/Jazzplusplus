#pragma once

#include <vector>
#include <string>
#include <cstddef>
#include <cstdint>

namespace Jazz {

/**
 * @brief MADI (AES10) / Dante / AES3 Live Broadcast Stream Health & Jitter Telemetry Hub.
 * Provides hitless dual-redundant stream switching (SMPTE ST 2022-7 style) with clock jitter
 * and buffer slip telemetry.
 */
class JZLiveStreamHealthTelemetryEngine {
public:
    enum class StreamId {
        PrimaryA = 0,
        SecondaryB = 1
    };

    enum class LinkStatus {
        OK = 0,
        Degraded = 1,
        Glitching = 2,
        Disconnected = 3
    };

    enum class StreamProtocol {
        MADI_AES10 = 0,
        Dante_AES67 = 1,
        AES3_EBU = 2
    };

    struct StreamFrameHeader {
        uint32_t sequenceNumber = 0;
        uint64_t timestampNs = 0;
        bool validityBit = true;     // AES3/MADI Validity (true = valid audio)
        bool userBit = false;        // AES3 User bit
        bool channelStatusBit = false;// AES3 Channel Status
        bool parityError = false;    // AES3/MADI Parity error flag
        bool crcError = false;       // IP packet CRC/checksum error
    };

    struct StreamTelemetry {
        LinkStatus status = LinkStatus::Disconnected;
        float clockJitterPpm = 0.0f;     // Parts-per-million clock jitter
        float clockJitterUs = 0.0f;      // Microseconds peak-to-peak jitter
        float bufferFillPct = 0.0f;      // 0.0 to 100.0%
        uint32_t droppedFrames = 0;
        uint32_t slippedSamples = 0;
        uint32_t parityErrors = 0;
        uint32_t crcErrors = 0;
        uint64_t totalFramesReceived = 0;
        bool isLocked = false;
    };

    struct Configuration {
        StreamProtocol protocol = StreamProtocol::Dante_AES67;
        float maxAllowedJitterPpm = 50.0f; // Broadcast threshold (typically 50 ppm for AES11)
        float bufferSlipThresholdPct = 90.0f;
        uint32_t failoverHysteresisFrames = 5;
        bool autoFailoverEnabled = true;
        float crossfadeTimeMs = 5.0f;      // Hitless crossfade duration (ms)
    };

    JZLiveStreamHealthTelemetryEngine();
    ~JZLiveStreamHealthTelemetryEngine() = default;

    void initialize(float sampleRate, size_t bufferCapacity = 4096);
    void reset();

    void setConfiguration(const Configuration& config);
    const Configuration& getConfiguration() const { return m_config; }

    /**
     * @brief Ingest a block of audio samples from a specific stream path (A or B).
     */
    void ingestStreamBlock(StreamId id, const float* left, const float* right,
                           size_t numSamples, const StreamFrameHeader& header);

    /**
     * @brief Generate mastered live stream broadcast output with hitless failover protection.
     */
    void processOutput(float* outL, float* outR, size_t numSamples);

    /**
     * @brief Query current telemetry stats for a stream.
     */
    StreamTelemetry getTelemetry(StreamId id) const;

    /**
     * @brief Get currently active broadcast output stream.
     */
    StreamId getActiveStream() const { return m_activeStream; }

    /**
     * @brief Manually force active stream.
     */
    void forceActiveStream(StreamId id);

    /**
     * @brief Get total failover transition count.
     */
    uint32_t getFailoverEventCount() const { return m_failoverEvents; }

private:
    float m_sampleRate = 48000.0f;
    size_t m_bufferCapacity = 4096;
    Configuration m_config;

    StreamId m_activeStream = StreamId::PrimaryA;
    StreamId m_previousStream = StreamId::PrimaryA;
    uint32_t m_failoverEvents = 0;

    // Crossfade state for hitless switching
    bool m_inCrossfade = false;
    size_t m_crossfadeSamplesTotal = 240;
    size_t m_crossfadeSampleIndex = 0;

    struct StreamBuffer {
        std::vector<float> bufL;
        std::vector<float> bufR;
        size_t writePos = 0;
        size_t readPos = 0;
        size_t count = 0;
        StreamTelemetry telemetry;
        uint32_t lastSeq = 0;
        uint64_t lastTimestampNs = 0;
        uint32_t consecutiveErrors = 0;
        uint32_t consecutiveHealthyFrames = 0;

        void reset(size_t capacity) {
            bufL.assign(capacity, 0.0f);
            bufR.assign(capacity, 0.0f);
            writePos = 0;
            readPos = 0;
            count = 0;
            telemetry = StreamTelemetry();
            lastSeq = 0;
            lastTimestampNs = 0;
            consecutiveErrors = 0;
            consecutiveHealthyFrames = 0;
        }
    };

    StreamBuffer m_streamA;
    StreamBuffer m_streamB;

    void updateTelemetry(StreamBuffer& stream, const StreamFrameHeader& header, size_t numSamples);
    void evaluateFailover();
};

} // namespace Jazz

namespace jazz {
    using JZLiveStreamHealthTelemetryEngine = Jazz::JZLiveStreamHealthTelemetryEngine;
}
