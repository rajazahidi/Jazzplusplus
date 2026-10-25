#include "Midi2UniversalPacketEngine.h"
#include <cstdint>
#include <algorithm>

namespace Jazz {

JZMidi2UniversalPacketEngine::JZMidi2UniversalPacketEngine()
    : m_ciNegotiated(false)
    , m_localMuid(0x12345678)
    , m_remoteMuid(0)
{
}

UniversalMidiPacket JZMidi2UniversalPacketEngine::makeNoteOn(
    uint8_t group, uint8_t channel, uint8_t noteNumber,
    uint16_t velocity16, uint8_t attributeType, uint16_t attributeData)
{
    UniversalMidiPacket p;
    p.wordCount = 2; // 64-bit packet for MIDI 2.0 Channel Voice

    // Word 0: [MessageType: 4 bits = 0x4] [Group: 4 bits] [Status: 4 bits = 0x9] [Channel: 4 bits]
    //         [Note Number: 8 bits] [Attribute Type: 8 bits]
    uint32_t w0 = (static_cast<uint32_t>(UmpMessageType::MIDI2_CHANNEL_VOICE) << 28)
                | (static_cast<uint32_t>(group & 0x0F) << 24)
                | (static_cast<uint32_t>(Midi2Status::NOTE_ON) << 20)
                | (static_cast<uint32_t>(channel & 0x0F) << 16)
                | (static_cast<uint32_t>(noteNumber & 0x7F) << 8)
                | (static_cast<uint32_t>(attributeType));

    // Word 1: [Velocity: 16 bits] [Attribute Data: 16 bits]
    uint32_t w1 = (static_cast<uint32_t>(velocity16) << 16)
                | (static_cast<uint32_t>(attributeData));

    p.words[0] = w0;
    p.words[1] = w1;
    return p;
}

UniversalMidiPacket JZMidi2UniversalPacketEngine::makeNoteOff(
    uint8_t group, uint8_t channel, uint8_t noteNumber, uint16_t velocity16)
{
    UniversalMidiPacket p;
    p.wordCount = 2;

    uint32_t w0 = (static_cast<uint32_t>(UmpMessageType::MIDI2_CHANNEL_VOICE) << 28)
                | (static_cast<uint32_t>(group & 0x0F) << 24)
                | (static_cast<uint32_t>(Midi2Status::NOTE_OFF) << 20)
                | (static_cast<uint32_t>(channel & 0x0F) << 16)
                | (static_cast<uint32_t>(noteNumber & 0x7F) << 8);

    uint32_t w1 = (static_cast<uint32_t>(velocity16) << 16);

    p.words[0] = w0;
    p.words[1] = w1;
    return p;
}

UniversalMidiPacket JZMidi2UniversalPacketEngine::makeControlChange32(
    uint8_t group, uint8_t channel, uint8_t controllerIndex, uint32_t value32)
{
    UniversalMidiPacket p;
    p.wordCount = 2;

    uint32_t w0 = (static_cast<uint32_t>(UmpMessageType::MIDI2_CHANNEL_VOICE) << 28)
                | (static_cast<uint32_t>(group & 0x0F) << 24)
                | (static_cast<uint32_t>(Midi2Status::CONTROL_CHANGE) << 20)
                | (static_cast<uint32_t>(channel & 0x0F) << 16)
                | (static_cast<uint32_t>(controllerIndex & 0x7F) << 8);

    uint32_t w1 = value32;

    p.words[0] = w0;
    p.words[1] = w1;
    return p;
}

UniversalMidiPacket JZMidi2UniversalPacketEngine::makePerNotePitchBend(
    uint8_t group, uint8_t channel, uint8_t noteNumber, int32_t pitchBend32)
{
    UniversalMidiPacket p;
    p.wordCount = 2;

    uint32_t w0 = (static_cast<uint32_t>(UmpMessageType::MIDI2_CHANNEL_VOICE) << 28)
                | (static_cast<uint32_t>(group & 0x0F) << 24)
                | (static_cast<uint32_t>(Midi2Status::PER_NOTE_PITCH_BEND) << 20)
                | (static_cast<uint32_t>(channel & 0x0F) << 16)
                | (static_cast<uint32_t>(noteNumber & 0x7F) << 8);

    uint32_t w1 = static_cast<uint32_t>(pitchBend32);

    p.words[0] = w0;
    p.words[1] = w1;
    return p;
}

UniversalMidiPacket JZMidi2UniversalPacketEngine::makeChannelPitchBend32(
    uint8_t group, uint8_t channel, uint32_t pitchBend32)
{
    UniversalMidiPacket p;
    p.wordCount = 2;

    uint32_t w0 = (static_cast<uint32_t>(UmpMessageType::MIDI2_CHANNEL_VOICE) << 28)
                | (static_cast<uint32_t>(group & 0x0F) << 24)
                | (static_cast<uint32_t>(Midi2Status::PITCH_BEND) << 20)
                | (static_cast<uint32_t>(channel & 0x0F) << 16);

    uint32_t w1 = pitchBend32;

    p.words[0] = w0;
    p.words[1] = w1;
    return p;
}

bool JZMidi2UniversalPacketEngine::parseUmp(
    const UniversalMidiPacket& packet,
    uint8_t& outChannel, uint8_t& outNoteOrIndex, uint32_t& out32BitValue)
{
    if (packet.getMessageType() != UmpMessageType::MIDI2_CHANNEL_VOICE) {
        return false;
    }

    outChannel = packet.getChannel();
    outNoteOrIndex = static_cast<uint8_t>((packet.words[0] >> 8) & 0x7F);

    Midi2Status st = packet.getStatus();
    if (st == Midi2Status::NOTE_ON || st == Midi2Status::NOTE_OFF) {
        // High 16-bits in word 1 is 16-bit velocity
        out32BitValue = (packet.words[1] >> 16) & 0xFFFF;
    } else {
        // Full 32-bit controller or pitch-bend value
        out32BitValue = packet.words[1];
    }

    return true;
}

uint16_t JZMidi2UniversalPacketEngine::midi1ToMidi2Velocity(uint8_t vel7)
{
    if (vel7 == 0) return 0;
    // Scale 7-bit [1..127] to 16-bit [2..65535] with bit repetition
    uint16_t v = static_cast<uint16_t>(vel7 & 0x7F);
    return static_cast<uint16_t>((v << 9) | (v << 2) | (v >> 5));
}

uint8_t JZMidi2UniversalPacketEngine::midi2ToMidi1Velocity(uint16_t vel16)
{
    if (vel16 == 0) return 0;
    uint8_t v = static_cast<uint8_t>(vel16 >> 9);
    return std::clamp<uint8_t>(v, 1, 127);
}

uint32_t JZMidi2UniversalPacketEngine::midi1ToMidi2Controller(uint8_t cc7)
{
    uint32_t v = static_cast<uint32_t>(cc7 & 0x7F);
    // Expand 7 bits across 32 bits
    return (v << 25) | (v << 18) | (v << 11) | (v << 4) | (v >> 3);
}

uint8_t JZMidi2UniversalPacketEngine::midi2ToMidi1Controller(uint32_t cc32)
{
    return static_cast<uint8_t>((cc32 >> 25) & 0x7F);
}

bool JZMidi2UniversalPacketEngine::initiateCapabilityInquiry(uint8_t sourceMuid, uint8_t destinationMuid)
{
    m_localMuid = sourceMuid;
    m_remoteMuid = destinationMuid;
    m_ciNegotiated = true;
    return true;
}

std::string JZMidi2UniversalPacketEngine::buildPropertyExchangeGet(const std::string& resource)
{
    return "{\"header\":{\"action\":\"get\",\"resource\":\"" + resource + "\"},\"body\":{}}";
}

bool JZMidi2UniversalPacketEngine::parsePropertyExchangeReply(
    const std::string& jsonPayload, std::string& outResource, std::string& outValue)
{
    if (jsonPayload.empty()) return false;

    // Lightweight JSON token search
    size_t resPos = jsonPayload.find("\"resource\":");
    if (resPos != std::string::npos) {
        size_t q1 = jsonPayload.find('"', resPos + 11);
        size_t q2 = (q1 != std::string::npos) ? jsonPayload.find('"', q1 + 1) : std::string::npos;
        if (q1 != std::string::npos && q2 != std::string::npos) {
            outResource = jsonPayload.substr(q1 + 1, q2 - q1 - 1);
        }
    }

    size_t valPos = jsonPayload.find("\"value\":");
    if (valPos != std::string::npos) {
        size_t q1 = jsonPayload.find('"', valPos + 8);
        size_t q2 = (q1 != std::string::npos) ? jsonPayload.find('"', q1 + 1) : std::string::npos;
        if (q1 != std::string::npos && q2 != std::string::npos) {
            outValue = jsonPayload.substr(q1 + 1, q2 - q1 - 1);
        }
    }

    return !outResource.empty();
}

} // namespace Jazz
