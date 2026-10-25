//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// RealTimeThreadAffinityScheduler.cpp - Real-Time Audio Thread Affinity Scheduler
//*****************************************************************************

#include "RealTimeThreadAffinityScheduler.h"
#include <thread>
#include <cmath>
#include <algorithm>

namespace Jazz {

JZRealTimeThreadAffinityScheduler::JZRealTimeThreadAffinityScheduler()
{
    initialize();
}

void JZRealTimeThreadAffinityScheduler::initialize()
{
    detectCpuTopology();
    reset();
}

void JZRealTimeThreadAffinityScheduler::reset()
{
    m_lastCycleUs = 0.0f;
    m_cpuUsagePercent = 0.0f;
    m_underrunCount = 0;
    m_writeHead.store(0, std::memory_order_relaxed);
    m_readTail.store(0, std::memory_order_relaxed);
    m_realTimeActive = true;
}

void JZRealTimeThreadAffinityScheduler::detectCpuTopology()
{
    unsigned int hardwareCores = std::thread::hardware_concurrency();
    m_detectedCores = (hardwareCores > 0) ? hardwareCores : 8;

    // By default on hybrid architectures, allocate primary performance cores (e.g. first 8 cores)
    m_performanceCores = std::min(m_detectedCores, static_cast<size_t>(8));
    if (m_performanceCores == 0) m_performanceCores = 1;

    // Generate affinity mask for performance cores
    uint64_t mask = 0;
    for (size_t c = 0; c < m_performanceCores; ++c) {
        mask |= (1ULL << c);
    }
    m_params.targetCoreMask = mask;
}

void JZRealTimeThreadAffinityScheduler::setParameters(const Parameters& params)
{
    m_params = params;
    m_params.targetBufferDeadlineUs = std::max(200.0f, std::min(m_params.targetBufferDeadlineUs, 100000.0f));
}

void JZRealTimeThreadAffinityScheduler::setPriorityPolicy(AudioThreadPriorityPolicy policy)
{
    m_params.priorityPolicy = policy;
}

void JZRealTimeThreadAffinityScheduler::setLockToPerformanceCores(bool lock)
{
    m_params.lockToPerformanceCores = lock;
}

void JZRealTimeThreadAffinityScheduler::setTargetCoreMask(uint64_t mask)
{
    m_params.targetCoreMask = mask;
}

void JZRealTimeThreadAffinityScheduler::setTargetBufferDeadlineUs(float us)
{
    m_params.targetBufferDeadlineUs = std::max(200.0f, std::min(us, 100000.0f));
}

void JZRealTimeThreadAffinityScheduler::recordCycleExecution(float elapsedMicroseconds)
{
    m_lastCycleUs = elapsedMicroseconds;
    if (m_params.targetBufferDeadlineUs > 0.0f) {
        m_cpuUsagePercent = (elapsedMicroseconds / m_params.targetBufferDeadlineUs) * 100.0f;
    }

    // Overrun watchdog
    if (elapsedMicroseconds > m_params.targetBufferDeadlineUs) {
        m_underrunCount++;
    }
}

bool JZRealTimeThreadAffinityScheduler::pushTelemetry(uint32_t eventId, float value)
{
    size_t head = m_writeHead.load(std::memory_order_relaxed);
    size_t nextHead = (head + 1) % RING_BUFFER_SIZE;

    // Full check
    if (nextHead == m_readTail.load(std::memory_order_acquire)) {
        return false;
    }

    m_ringBuffer[head].eventId = eventId;
    m_ringBuffer[head].value = value;
    m_writeHead.store(nextHead, std::memory_order_release);
    return true;
}

bool JZRealTimeThreadAffinityScheduler::popTelemetry(uint32_t& outEventId, float& outValue)
{
    size_t tail = m_readTail.load(std::memory_order_relaxed);

    // Empty check
    if (tail == m_writeHead.load(std::memory_order_acquire)) {
        return false;
    }

    outEventId = m_ringBuffer[tail].eventId;
    outValue = m_ringBuffer[tail].value;
    m_readTail.store((tail + 1) % RING_BUFFER_SIZE, std::memory_order_release);
    return true;
}

std::string JZRealTimeThreadAffinityScheduler::getPolicyName() const
{
    switch (m_params.priorityPolicy) {
        case AudioThreadPriorityPolicy::PRO_AUDIO_MMCSS:
            return "Windows Pro Audio (MMCSS)";
        case AudioThreadPriorityPolicy::REALTIME_CRITICAL:
            return "Realtime Critical (SCHED_FIFO)";
        case AudioThreadPriorityPolicy::HIGH_PRIORITY:
            return "High Priority";
        case AudioThreadPriorityPolicy::NORMAL:
            return "Normal Priority";
    }
    return "Unknown";
}

} // namespace Jazz
