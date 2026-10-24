#ifndef JAZZ_GAME_AUDIO_BRIDGE_H
#define JAZZ_GAME_AUDIO_BRIDGE_H

#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>

namespace Jazz {

enum class GameEngineTarget {
    UNREAL_ENGINE_5,
    UNITY_AUDIO,
    WWISE_MIDDLEWARE,
    FMOD_STUDIO,
    GODOT_ENGINE
};

struct GameSoundEvent {
    std::string eventName;
    std::string audioAssetRef;
    float volumeDb;
    float pitchShiftSemi;
    bool isLooping;
    float minAttenuationDistance;
    float maxAttenuationDistance;
};

class JZGameAudioBridge {
public:
    JZGameAudioBridge(GameEngineTarget target = GameEngineTarget::WWISE_MIDDLEWARE);
    ~JZGameAudioBridge() = default;

    // Target Engine Configuration
    GameEngineTarget getTarget() const { return m_target; }
    void setTarget(GameEngineTarget target) { m_target = target; }

    // RTPC (Real-Time Parameter Control) Management
    void registerRtpc(const std::string& paramName, float defVal = 0.0f, float minVal = 0.0f, float maxVal = 100.0f);
    bool setRtpcValue(const std::string& paramName, float value);
    float getRtpcValue(const std::string& paramName) const;

    // State & Switch Management
    void setGameState(const std::string& stateGroup, const std::string& stateValue);
    std::string getGameState(const std::string& stateGroup) const;

    // Sound Events & SoundBanks
    bool registerSoundEvent(const std::string& eventName, const std::string& assetRef,
                            float volumeDb = 0.0f, bool looping = false);
    const GameSoundEvent* getSoundEvent(const std::string& eventName) const;
    size_t getSoundEventCount() const { return m_events.size(); }

    // Spatial Acoustics: Ray-Traced Obstruction & Occlusion
    // Calculates dynamic filter cutoff & wet attenuation based on physical geometry
    void calculateAcousticObstruction(
        float obstructionRatio, float occlusionRatio,
        float& outDirectFilterLpfHz, float& outDirectLevelMultiplier, float& outReverbSendMultiplier) const;

    // SoundBank Metadata Serialization
    std::string exportSoundBankDefinition(const std::string& bankName) const;

private:
    GameEngineTarget m_target;
    std::unordered_map<std::string, float> m_rtpcValues;
    std::unordered_map<std::string, std::pair<float, float>> m_rtpcRanges;
    std::unordered_map<std::string, std::string> m_gameStates;
    std::unordered_map<std::string, GameSoundEvent> m_events;
};

} // namespace Jazz

#endif // JAZZ_GAME_AUDIO_BRIDGE_H
