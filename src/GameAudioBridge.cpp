#include "GameAudioBridge.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace Jazz {

JZGameAudioBridge::JZGameAudioBridge(GameEngineTarget target)
    : m_target(target)
{
}

void JZGameAudioBridge::registerRtpc(const std::string& paramName, float defVal, float minVal, float maxVal)
{
    float val = std::clamp(defVal, minVal, maxVal);
    m_rtpcValues[paramName] = val;
    m_rtpcRanges[paramName] = std::make_pair(minVal, maxVal);
}

bool JZGameAudioBridge::setRtpcValue(const std::string& paramName, float value)
{
    auto it = m_rtpcValues.find(paramName);
    if (it == m_rtpcValues.end()) return false;

    auto rangeIt = m_rtpcRanges.find(paramName);
    if (rangeIt != m_rtpcRanges.end()) {
        it->second = std::clamp(value, rangeIt->second.first, rangeIt->second.second);
    } else {
        it->second = value;
    }
    return true;
}

float JZGameAudioBridge::getRtpcValue(const std::string& paramName) const
{
    auto it = m_rtpcValues.find(paramName);
    if (it == m_rtpcValues.end()) return 0.0f;
    return it->second;
}

void JZGameAudioBridge::setGameState(const std::string& stateGroup, const std::string& stateValue)
{
    m_gameStates[stateGroup] = stateValue;
}

std::string JZGameAudioBridge::getGameState(const std::string& stateGroup) const
{
    auto it = m_gameStates.find(stateGroup);
    if (it == m_gameStates.end()) return "None";
    return it->second;
}

bool JZGameAudioBridge::registerSoundEvent(
    const std::string& eventName, const std::string& assetRef, float volumeDb, bool looping)
{
    GameSoundEvent ev;
    ev.eventName = eventName;
    ev.audioAssetRef = assetRef;
    ev.volumeDb = volumeDb;
    ev.pitchShiftSemi = 0.0f;
    ev.isLooping = looping;
    ev.minAttenuationDistance = 2.0f;
    ev.maxAttenuationDistance = 50.0f;

    m_events[eventName] = ev;
    return true;
}

const GameSoundEvent* JZGameAudioBridge::getSoundEvent(const std::string& eventName) const
{
    auto it = m_events.find(eventName);
    if (it == m_events.end()) return nullptr;
    return &(it->second);
}

void JZGameAudioBridge::calculateAcousticObstruction(
    float obstructionRatio, float occlusionRatio,
    float& outDirectFilterLpfHz, float& outDirectLevelMultiplier, float& outReverbSendMultiplier) const
{
    float obs = std::clamp(obstructionRatio, 0.0f, 1.0f);
    float occ = std::clamp(occlusionRatio, 0.0f, 1.0f);

    // Obstruction affects direct path only (muffles direct sound via low-pass)
    // 20,000 Hz when open -> down to 800 Hz when fully obstructed
    outDirectFilterLpfHz = 20000.0f * std::pow(0.04f, obs);

    // Direct path level drops with obstruction and occlusion
    outDirectLevelMultiplier = (1.0f - obs * 0.7f) * (1.0f - occ * 0.9f);

    // Occlusion affects both direct and environmental reflections/reverb
    outReverbSendMultiplier = 1.0f - occ * 0.75f;
}

std::string JZGameAudioBridge::exportSoundBankDefinition(const std::string& bankName) const
{
    std::ostringstream ss;
    ss << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    ss << "<SoundBank Name=\"" << bankName << "\" TargetPlatform=\"Multiplatform\">\n";
    ss << "  <Events Count=\"" << m_events.size() << "\">\n";

    for (const auto& kv : m_events) {
        ss << "    <Event Name=\"" << kv.second.eventName << "\" Asset=\""
           << kv.second.audioAssetRef << "\" VolumeDb=\"" << kv.second.volumeDb
           << "\" Looping=\"" << (kv.second.isLooping ? "true" : "false") << "\"/>\n";
    }

    ss << "  </Events>\n";
    ss << "  <RTPCs Count=\"" << m_rtpcValues.size() << "\">\n";
    for (const auto& kv : m_rtpcValues) {
        ss << "    <RTPC Name=\"" << kv.first << "\" Value=\"" << kv.second << "\"/>\n";
    }
    ss << "  </RTPCs>\n";
    ss << "</SoundBank>\n";

    return ss.str();
}

} // namespace Jazz
