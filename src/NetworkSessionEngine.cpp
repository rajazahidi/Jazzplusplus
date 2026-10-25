#include "NetworkSessionEngine.h"
#include <cstdint>
#include <algorithm>
#include <cstring>

namespace Jazz {

JZNetworkSessionEngine::JZNetworkSessionEngine(uint32_t localPeerId, const std::string& userName)
    : m_localPeerId(localPeerId)
    , m_userName(userName)
    , m_lamportClock(0)
    , m_nextSequence(1)
    , m_jitterBufferDepthMs(10.0f)
    , m_estimatedLatencyMs(12.5f)
{
}

bool JZNetworkSessionEngine::registerPeer(uint32_t peerId, const std::string& name, const std::string& ip, uint16_t port)
{
    if (peerId == m_localPeerId) return false;

    NetworkPeerSession peer;
    peer.peerId = peerId;
    peer.displayName = name;
    peer.ipAddress = ip;
    peer.port = port;
    peer.roundTripLatencyMs = 15.0f;
    peer.packetLossRate = 0.0f;
    peer.lastHeartbeat = 1000;
    peer.isConnected = true;

    m_peers[peerId] = peer;
    return true;
}

bool JZNetworkSessionEngine::unregisterPeer(uint32_t peerId)
{
    return m_peers.erase(peerId) > 0;
}

size_t JZNetworkSessionEngine::getConnectedPeerCount() const
{
    size_t count = 0;
    for (const auto& kv : m_peers) {
        if (kv.second.isConnected) count++;
    }
    return count;
}

const NetworkPeerSession* JZNetworkSessionEngine::getPeer(uint32_t peerId) const
{
    auto it = m_peers.find(peerId);
    if (it == m_peers.end()) return nullptr;
    return &(it->second);
}

AoipAudioPacket JZNetworkSessionEngine::packetizeAudio(
    const float* left, const float* right, int numSamples, uint32_t sampleRate)
{
    (void)sampleRate;
    AoipAudioPacket pkt;
    pkt.sequenceNumber = m_nextSequence++;
    pkt.timestampSamples = static_cast<uint32_t>(m_lamportClock * 128);
    pkt.channelCount = 2;
    pkt.sampleCount = static_cast<uint16_t>(numSamples > 0 ? numSamples : 0);

    if (numSamples > 0 && left && right) {
        pkt.audioData.resize(numSamples * 2);
        for (int i = 0; i < numSamples; ++i) {
            pkt.audioData[i * 2 + 0] = left[i];
            pkt.audioData[i * 2 + 1] = right[i];
        }
    }

    return pkt;
}

bool JZNetworkSessionEngine::ingestRemotePacket(
    uint32_t peerId, const AoipAudioPacket& packet, float* outL, float* outR, int numSamples)
{
    auto it = m_peers.find(peerId);
    if (it == m_peers.end() || !it->second.isConnected) return false;

    if (!outL || !outR || numSamples <= 0) return false;

    int copySamples = std::min(numSamples, static_cast<int>(packet.sampleCount));
    if (packet.channelCount == 2 && packet.audioData.size() >= static_cast<size_t>(copySamples * 2)) {
        for (int i = 0; i < copySamples; ++i) {
            outL[i] = packet.audioData[i * 2 + 0];
            outR[i] = packet.audioData[i * 2 + 1];
        }
    } else {
        // Zero-fill or packet loss concealment fallback
        std::memset(outL, 0, numSamples * sizeof(float));
        std::memset(outR, 0, numSamples * sizeof(float));
    }

    // Zero remaining buffer if incoming packet was shorter
    for (int i = copySamples; i < numSamples; ++i) {
        outL[i] = 0.0f;
        outR[i] = 0.0f;
    }

    return true;
}

TimelineDeltaCrdt JZNetworkSessionEngine::createTimelineDelta(
    const std::string& entityId, const std::string& action, const std::string& payload)
{
    m_lamportClock++;

    TimelineDeltaCrdt delta;
    delta.lamportClock = m_lamportClock;
    delta.originPeerId = m_localPeerId;
    delta.entityId = entityId;
    delta.action = action;
    delta.payloadJson = payload;

    m_crdtState[entityId] = delta;
    return delta;
}

bool JZNetworkSessionEngine::applyRemoteDelta(const TimelineDeltaCrdt& delta)
{
    // Lamport clock synchronization rule: L = max(L_local, L_remote) + 1
    m_lamportClock = std::max(m_lamportClock, delta.lamportClock) + 1;

    auto it = m_crdtState.find(delta.entityId);
    if (it == m_crdtState.end() || delta.lamportClock > it->second.lamportClock ||
        (delta.lamportClock == it->second.lamportClock && delta.originPeerId > it->second.originPeerId)) {
        m_crdtState[delta.entityId] = delta;
        return true;
    }

    return false; // Outdated or superseded delta
}

void JZNetworkSessionEngine::setJitterBufferDepthMs(float depthMs)
{
    m_jitterBufferDepthMs = std::clamp(depthMs, 1.0f, 200.0f);
    m_estimatedLatencyMs = m_jitterBufferDepthMs + 2.5f;
}

} // namespace Jazz
