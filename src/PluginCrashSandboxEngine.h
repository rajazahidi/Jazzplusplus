//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// PluginCrashSandboxEngine.h - Out-of-Process VST3/CLAP Plugin Crash Sandboxing
// & Host Isolation Architecture
//
// Protects the main DAW audio and MIDI engine from third-party plugin crashes.
// Implements lock-free IPC shared-memory ringbuffers, watchdog heartbeats,
// structured crash detection, failsafe auto-bypass, and seamless state respawn.
//*****************************************************************************

#ifndef JAZZ_PLUGIN_CRASH_SANDBOX_ENGINE_H
#define JAZZ_PLUGIN_CRASH_SANDBOX_ENGINE_H

#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include <cstdint>
#include <atomic>
#include <functional>

namespace Jazz {

enum class SandboxProcessRole {
    HOST = 0,
    SANDBOX_WORKER
};

enum class SandboxStatus {
    RUNNING_HEALTHY = 0,
    HEARTBEAT_TIMEOUT,
    CRASHED_ISOLATED,
    RESPAWNING,
    BYPASSED_SAFE
};

enum class PluginCrashType {
    NONE = 0,
    ACCESS_VIOLATION,      // Segfault / null pointer dereference
    DIVISION_BY_ZERO,      // Arithmetic fault
    STACK_OVERFLOW,        // Recursion limit exceeded
    HANG_UNRESPONSIVE      // Infinite loop / thread lock
};

class JZPluginCrashSandboxEngine {
public:
    struct Configuration {
        std::string pluginId = "Jazz_VST3_Worker";
        std::string pluginName = "Virtual Acoustic Instrument";
        uint32_t bufferCapacitySamples = 4096;
        uint32_t heartbeatIntervalMs = 50;
        uint32_t watchdogTimeoutMs = 250;
        bool autoRespawnOnCrash = true;
        uint32_t maxRespawnAttempts = 3;
    };

    struct SandboxDiagnostics {
        SandboxStatus status = SandboxStatus::RUNNING_HEALTHY;
        uint64_t totalFramesProcessed = 0;
        uint32_t crashCount = 0;
        uint32_t respawnCount = 0;
        double roundTripLatencyMs = 0.15;
        double lastHeartbeatAgeMs = 0.0;
        PluginCrashType lastCrashType = PluginCrashType::NONE;
        bool hostRemainedAlive = true;
    };

    JZPluginCrashSandboxEngine(float sampleRate = 48000.0f);
    ~JZPluginCrashSandboxEngine();

    void initialize(const Configuration& config);
    void reset();

    // Configuration
    void setConfiguration(const Configuration& config);
    const Configuration& getConfiguration() const { return m_config; }

    // Audio & State Processing
    void processAudio(const float* inL, const float* inR, float* outL, float* outR, size_t numFrames);
    void saveStateChunk(const std::vector<uint8_t>& stateBlob);
    const std::vector<uint8_t>& getSavedStateChunk() const { return m_lastKnownGoodState; }

    // Heartbeat & Watchdog Monitor
    void sendWorkerHeartbeat();
    void updateWatchdogTick(double elapsedMs);

    // Crash Simulation & Recovery
    void simulatePluginCrash(PluginCrashType crashType);
    bool triggerRecoveryRespawn();

    // Diagnostics & Inspection
    const SandboxDiagnostics& getDiagnostics() const { return m_diagnostics; }
    SandboxStatus getStatus() const { return m_diagnostics.status; }
    bool isHostAlive() const { return m_diagnostics.hostRemainedAlive; }
    uint32_t getCrashCount() const { return m_diagnostics.crashCount; }
    uint32_t getRespawnCount() const { return m_diagnostics.respawnCount; }

private:
    float m_sampleRate;
    Configuration m_config;
    SandboxDiagnostics m_diagnostics;

    std::vector<float> m_sharedRingBufferL;
    std::vector<float> m_sharedRingBufferR;
    std::vector<uint8_t> m_lastKnownGoodState;

    double m_watchdogTimerMs;
    bool m_workerCrashed;
};

} // namespace Jazz

#endif // JAZZ_PLUGIN_CRASH_SANDBOX_ENGINE_H
