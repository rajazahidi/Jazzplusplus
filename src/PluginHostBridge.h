#ifndef JAZZ_PLUGIN_HOST_BRIDGE_H
#define JAZZ_PLUGIN_HOST_BRIDGE_H

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <cstdint>

namespace Jazz {

enum class PluginFormat {
    INTERNAL,
    CLAP,   // CLever Audio Plugin (modern open-source DAW standard)
    VST3    // Steinberg VST3 standard
};

enum class PluginSandboxMode {
    IN_PROCESS,             // Direct memory pointer execution (minimum latency)
    SANDBOXED_SUBPROCESS    // Isolated IPC execution (crash-resilient protection)
};

enum class PluginCategory {
    EFFECT,
    SYNTHESIZER,
    ANALYZER,
    SPATIAL_PANNER
};

struct PluginParameterDesc {
    uint32_t id;
    std::string name;
    float defaultValue;
    float minValue;
    float maxValue;
    float currentValue;
    bool isAutomated;
};

class JZPluginHostBridge {
public:
    JZPluginHostBridge(PluginFormat format = PluginFormat::CLAP,
                       PluginSandboxMode sandboxMode = PluginSandboxMode::SANDBOXED_SUBPROCESS);
    ~JZPluginHostBridge();

    // Plugin Lifecycle Management
    bool loadPlugin(const std::string& pluginPath, const std::string& pluginId);
    void unloadPlugin();
    bool isLoaded() const { return m_isLoaded; }

    void initialize(float sampleRate, int maxBlockSize);
    bool isInitialized() const { return m_isInitialized; }

    // Configuration
    PluginFormat getFormat() const { return m_format; }
    PluginSandboxMode getSandboxMode() const { return m_sandboxMode; }
    void setSandboxMode(PluginSandboxMode mode) { m_sandboxMode = mode; }

    const std::string& getPluginName() const { return m_pluginName; }
    const std::string& getVendor() const { return m_vendor; }
    PluginCategory getCategory() const { return m_category; }

    // Parameter System
    void registerParameter(uint32_t id, const std::string& name, float defVal, float minVal, float maxVal);
    bool setParameter(uint32_t id, float value);
    float getParameter(uint32_t id) const;
    const std::unordered_map<uint32_t, PluginParameterDesc>& getParameters() const { return m_parameters; }

    // Real-Time Audio Processing
    void processAudio(float** inputs, float** outputs, int numChannels, int numSamples);

    // Sandboxing Watchdog & Crash Recovery
    void triggerHeartbeat();
    bool checkWatchdogHealthy(uint32_t maxInactiveMs = 500);
    bool simulateSubprocessCrash(); // For test & verification of sandbox failover
    bool recoverFromCrash();
    bool hasCrashed() const { return m_hasCrashed; }

    // State Serialization
    std::vector<uint8_t> saveState() const;
    bool restoreState(const std::vector<uint8_t>& stateData);

private:
    PluginFormat m_format;
    PluginSandboxMode m_sandboxMode;
    PluginCategory m_category;
    std::string m_pluginPath;
    std::string m_pluginId;
    std::string m_pluginName;
    std::string m_vendor;

    bool m_isLoaded;
    bool m_isInitialized;
    bool m_hasCrashed;
    float m_sampleRate;
    int m_maxBlockSize;
    uint32_t m_lastHeartbeatTimestamp;

    std::unordered_map<uint32_t, PluginParameterDesc> m_parameters;
    std::vector<uint8_t> m_cachedState;
};

} // namespace Jazz

#endif // JAZZ_PLUGIN_HOST_BRIDGE_H
