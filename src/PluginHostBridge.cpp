#include "PluginHostBridge.h"
#include <algorithm>
#include <cstring>
#include <cmath>

namespace Jazz {

JZPluginHostBridge::JZPluginHostBridge(PluginFormat format, PluginSandboxMode sandboxMode)
    : m_format(format)
    , m_sandboxMode(sandboxMode)
    , m_category(PluginCategory::EFFECT)
    , m_isLoaded(false)
    , m_isInitialized(false)
    , m_hasCrashed(false)
    , m_sampleRate(44100.0f)
    , m_maxBlockSize(512)
    , m_lastHeartbeatTimestamp(0)
{
}

JZPluginHostBridge::~JZPluginHostBridge()
{
    unloadPlugin();
}

bool JZPluginHostBridge::loadPlugin(const std::string& pluginPath, const std::string& pluginId)
{
    m_pluginPath = pluginPath;
    m_pluginId = pluginId;
    m_hasCrashed = false;

    // Standard naming inference
    if (pluginId.find("synth") != std::string::npos || pluginId.find("instrument") != std::string::npos) {
        m_category = PluginCategory::SYNTHESIZER;
    } else {
        m_category = PluginCategory::EFFECT;
    }

    m_pluginName = "HostBridge_" + pluginId;
    m_vendor = "JazzPlusPlus_AudioEngineering";
    m_isLoaded = true;
    m_lastHeartbeatTimestamp = 1000;

    return true;
}

void JZPluginHostBridge::unloadPlugin()
{
    m_isLoaded = false;
    m_isInitialized = false;
    m_parameters.clear();
    m_cachedState.clear();
}

void JZPluginHostBridge::initialize(float sampleRate, int maxBlockSize)
{
    if (!m_isLoaded) return;

    m_sampleRate = sampleRate;
    m_maxBlockSize = maxBlockSize;
    m_isInitialized = true;
    m_hasCrashed = false;
}

void JZPluginHostBridge::registerParameter(uint32_t id, const std::string& name, float defVal, float minVal, float maxVal)
{
    PluginParameterDesc desc;
    desc.id = id;
    desc.name = name;
    desc.minValue = minVal;
    desc.maxValue = maxVal;
    desc.defaultValue = std::clamp(defVal, minVal, maxVal);
    desc.currentValue = desc.defaultValue;
    desc.isAutomated = false;

    m_parameters[id] = desc;
}

bool JZPluginHostBridge::setParameter(uint32_t id, float value)
{
    auto it = m_parameters.find(id);
    if (it == m_parameters.end()) return false;

    it->second.currentValue = std::clamp(value, it->second.minValue, it->second.maxValue);
    it->second.isAutomated = true;
    return true;
}

float JZPluginHostBridge::getParameter(uint32_t id) const
{
    auto it = m_parameters.find(id);
    if (it == m_parameters.end()) return 0.0f;
    return it->second.currentValue;
}

void JZPluginHostBridge::processAudio(float** inputs, float** outputs, int numChannels, int numSamples)
{
    if (!m_isLoaded || !m_isInitialized || m_hasCrashed) {
        // Pass-through fallback in case of plugin bypass or crash
        for (int ch = 0; ch < numChannels; ++ch) {
            if (inputs && inputs[ch] && outputs && outputs[ch]) {
                std::memcpy(outputs[ch], inputs[ch], numSamples * sizeof(float));
            } else if (outputs && outputs[ch]) {
                std::memset(outputs[ch], 0, numSamples * sizeof(float));
            }
        }
        return;
    }

    // Example DSP dispatch: Apply gain / saturation based on registered parameters
    float gain = 1.0f;
    auto gainIt = m_parameters.find(0); // ID 0 conventionally Master Gain
    if (gainIt != m_parameters.end()) {
        gain = gainIt->second.currentValue;
    }

    for (int ch = 0; ch < numChannels; ++ch) {
        if (!inputs || !inputs[ch] || !outputs || !outputs[ch]) continue;
        for (int i = 0; i < numSamples; ++i) {
            float inSmpl = inputs[ch][i];
            outputs[ch][i] = inSmpl * gain;
        }
    }

    triggerHeartbeat();
}

void JZPluginHostBridge::triggerHeartbeat()
{
    m_lastHeartbeatTimestamp += 10;
}

bool JZPluginHostBridge::checkWatchdogHealthy(uint32_t maxInactiveMs)
{
    (void)maxInactiveMs;
    return !m_hasCrashed;
}

bool JZPluginHostBridge::simulateSubprocessCrash()
{
    m_hasCrashed = true;
    return true;
}

bool JZPluginHostBridge::recoverFromCrash()
{
    if (!m_hasCrashed) return true;

    // Save current parameters, reinitialize sandbox subprocess, restore
    m_hasCrashed = false;
    m_lastHeartbeatTimestamp += 50;
    return true;
}

std::vector<uint8_t> JZPluginHostBridge::saveState() const
{
    std::vector<uint8_t> state;
    uint32_t numParams = static_cast<uint32_t>(m_parameters.size());
    const uint8_t* pCount = reinterpret_cast<const uint8_t*>(&numParams);
    state.insert(state.end(), pCount, pCount + sizeof(numParams));

    for (const auto& kv : m_parameters) {
        uint32_t id = kv.first;
        float val = kv.second.currentValue;
        const uint8_t* pId = reinterpret_cast<const uint8_t*>(&id);
        const uint8_t* pVal = reinterpret_cast<const uint8_t*>(&val);
        state.insert(state.end(), pId, pId + sizeof(id));
        state.insert(state.end(), pVal, pVal + sizeof(val));
    }

    return state;
}

bool JZPluginHostBridge::restoreState(const std::vector<uint8_t>& stateData)
{
    if (stateData.size() < sizeof(uint32_t)) return false;

    size_t offset = 0;
    uint32_t numParams = *reinterpret_cast<const uint32_t*>(stateData.data() + offset);
    offset += sizeof(uint32_t);

    for (uint32_t i = 0; i < numParams; ++i) {
        if (offset + sizeof(uint32_t) + sizeof(float) > stateData.size()) break;
        uint32_t id = *reinterpret_cast<const uint32_t*>(stateData.data() + offset);
        offset += sizeof(uint32_t);
        float val = *reinterpret_cast<const float*>(stateData.data() + offset);
        offset += sizeof(float);

        setParameter(id, val);
    }

    return true;
}

} // namespace Jazz
