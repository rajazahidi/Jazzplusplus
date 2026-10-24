#ifndef JAZZ_NETWORK_SESSION_ENGINE_H
#define JAZZ_NETWORK_SESSION_ENGINE_H

#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>

namespace Jazz {

enum class NetworkTransportProtocol {
    AES67_RTP,       // Broadcast Audio-over-IP standard
    WEBRTC_DATA,     // Peer-to-peer browser/remote collaboration
    LOCAL_DISCOVERY  // Zero-conf LAN mDNS session
};

struct NetworkPeerSession {
    uint32_t peerId;
    std::string displayName;
    std::string ipAddress;
    uint16_t port;
    float roundTripLatencyMs;
    float packetLossRate;
    uint32_t lastHeartbeat;
    bool isConnected;
};

struct TimelineDeltaCrdt {
    uint64_t lamportClock;
    uint32_t originPeerId;
    std::string entityId;     // e.g. "track:1:note:60"
    std::string action;       // "insert", "delete", "modify"
    std::string payloadJson;  // Delta property payload
};

struct AoipAudioPacket {
    uint32_t sequenceNumber;
    uint32_t timestampSamples;
    uint8_t channelCount;
    uint16_t sampleCount;
    std::vector<float> audioData; // Interleaved PCM float
};

class JZNetworkSessionEngine {
public:
    JZNetworkSessionEngine(uint32_t localPeerId = 1, const std::string& userName = "LocalProducer");
    ~JZNetworkSessionEngine() = default;

    // Session & Peer Management
    uint32_t getLocalPeerId() const { return m_localPeerId; }
    const std::string& getUserName() const { return m_userName; }

    bool registerPeer(uint32_t peerId, const std::string& name, const std::string& ip, uint16_t port);
    bool unregisterPeer(uint32_t peerId);
    size_t getConnectedPeerCount() const;
    const NetworkPeerSession* getPeer(uint32_t peerId) const;

    // Audio-over-IP (AoIP) Frame Streaming
    // Packetizes local audio buffer into an AoIP RTP packet
    AoipAudioPacket packetizeAudio(const float* left, const float* right, int numSamples, uint32_t sampleRate = 48000);

    // Ingests incoming remote packet, performs jitter-buffer sequencing and packet-loss concealment (PLC)
    bool ingestRemotePacket(uint32_t peerId, const AoipAudioPacket& packet, float* outL, float* outR, int numSamples);

    // Timeline Delta-CRDT Distributed Synchronization
    TimelineDeltaCrdt createTimelineDelta(const std::string& entityId, const std::string& action, const std::string& payload);
    bool applyRemoteDelta(const TimelineDeltaCrdt& delta);
    uint64_t getLamportClock() const { return m_lamportClock; }

    // Jitter Buffer & Latency Tuning
    void setJitterBufferDepthMs(float depthMs);
    float getJitterBufferDepthMs() const { return m_jitterBufferDepthMs; }
    float getEstimatedLatencyMs() const { return m_estimatedLatencyMs; }

private:
    uint32_t m_localPeerId;
    std::string m_userName;
    uint64_t m_lamportClock;
    uint32_t m_nextSequence;
    float m_jitterBufferDepthMs;
    float m_estimatedLatencyMs;

    std::unordered_map<uint32_t, NetworkPeerSession> m_peers;
    std::unordered_map<std::string, TimelineDeltaCrdt> m_crdtState; // State by entityId
};

} // namespace Jazz

#endif // JAZZ_NETWORK_SESSION_ENGINE_H
