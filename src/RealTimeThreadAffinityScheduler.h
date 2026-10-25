//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// RealTimeThreadAffinityScheduler.h - Real-Time Audio Thread Affinity Scheduler
//
// Locks real-time audio DSP worker threads to high-performance CPU cores (P-Cores)
// avoiding hybrid architecture E-Core migration, configures OS real-time scheduling
// priority (Windows MMCSS / Linux SCHED_FIFO), provides lock-free telemetry queues,
// and monitors microsecond execution deadlines to prevent buffer underruns.
//*****************************************************************************

#ifndef JAZZ_REAL_TIME_THREAD_AFFINITY_SCHEDULER_H
#define JAZZ_REAL_TIME_THREAD_AFFINITY_SCHEDULER_H

#include <vector>
#include <cstdint>
#include <cstddef>
#include <atomic>
#include <string>
#include <algorithm>

namespace Jazz {

enum class AudioThreadPriorityPolicy {
    NORMAL = 0,
    HIGH_PRIORITY = 1,
    REALTIME_CRITICAL = 2,
    PRO_AUDIO_MMCSS = 3
};

class JZRealTimeThreadAffinityScheduler {
public:
    struct TelemetryPacket {
        uint32_t eventId = 0;
        float value = 0.0f;
    };

    struct Parameters {
        AudioThreadPriorityPolicy priorityPolicy = AudioThreadPriorityPolicy::PRO_AUDIO_MMCSS;
        bool lockToPerformanceCores = true;  // Exclude low-power E-Cores
        uint64_t targetCoreMask = 0x0F;      // Default to first 4 performance cores
        float targetBufferDeadlineUs = 2666.0f; // Deadline in microseconds (e.g. 128 smp @ 48kHz = 2.66ms)
    };

    JZRealTimeThreadAffinityScheduler();
    ~JZRealTimeThreadAffinityScheduler() = default;

    void initialize();
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setPriorityPolicy(AudioThreadPriorityPolicy policy);
    AudioThreadPriorityPolicy getPriorityPolicy() const { return m_params.priorityPolicy; }

    void setLockToPerformanceCores(bool lock);
    bool isLockedToPerformanceCores() const { return m_params.lockToPerformanceCores; }

    void setTargetCoreMask(uint64_t mask);
    uint64_t getTargetCoreMask() const { return m_params.targetCoreMask; }

    void setTargetBufferDeadlineUs(float us);
    float getTargetBufferDeadlineUs() const { return m_params.targetBufferDeadlineUs; }

    // Real-Time Execution Tracking
    void recordCycleExecution(float elapsedMicroseconds);

    // Lock-Free SPSC Telemetry Queue
    bool pushTelemetry(uint32_t eventId, float value);
    bool popTelemetry(uint32_t& outEventId, float& outValue);

    // Telemetry & Getters
    size_t getDetectedCoreCount() const { return m_detectedCores; }
    size_t getPerformanceCoreCount() const { return m_performanceCores; }
    float getBufferCycleExecutionTimeUs() const { return m_lastCycleUs; }
    float getCpuUsagePercent() const { return m_cpuUsagePercent; }
    size_t getUnderrunCount() const { return m_underrunCount; }
    bool isRealTimeActive() const { return m_realTimeActive; }
    std::string getPolicyName() const;

private:
    void detectCpuTopology();

    Parameters m_params;
    size_t m_detectedCores = 8;
    size_t m_performanceCores = 8;
    bool m_realTimeActive = true;

    float m_lastCycleUs = 0.0f;
    float m_cpuUsagePercent = 0.0f;
    size_t m_underrunCount = 0;

    // Lock-Free Ring Buffer (Single Producer, Single Consumer)
    static constexpr size_t RING_BUFFER_SIZE = 256;
    TelemetryPacket m_ringBuffer[RING_BUFFER_SIZE];
    std::atomic<size_t> m_writeHead{0};
    std::atomic<size_t> m_readTail{0};
};

} // namespace Jazz

namespace jazz {
    using JZRealTimeThreadAffinityScheduler = Jazz::JZRealTimeThreadAffinityScheduler;
    using AudioThreadPriorityPolicy = Jazz::AudioThreadPriorityPolicy;
}

#endif // JAZZ_REAL_TIME_THREAD_AFFINITY_SCHEDULER_H
