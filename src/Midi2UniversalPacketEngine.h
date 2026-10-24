#ifndef JAZZ_MIDI2_UNIVERSAL_PACKET_ENGINE_H
#define JAZZ_MIDI2_UNIVERSAL_PACKET_ENGINE_H

#include <vector>
#include <string>
#include <cstdint>

namespace Jazz {

enum class UmpMessageType : uint8_t {
    UTILITY                  = 0x0,
    SYSTEM_REALTIME_COMMON   = 0x1,
    MIDI1_CHANNEL_VOICE      = 0x2,
    DATA_64BIT_SYSEX7        = 0x3,
    MIDI2_CHANNEL_VOICE      = 0x4,
    DATA_128BIT              = 0x5,
    FLEX_DATA                = 0xD,
    UMP_STREAM               = 0xF
};

enum class Midi2Status : uint8_t {
    NOTE_OFF                 = 0x8,
    NOTE_ON                  = 0x9,
    POLY_PRESSURE            = 0xA,
    CONTROL_CHANGE           = 0xB,
    PROGRAM_CHANGE           = 0xC,
    CHANNEL_PRESSURE         = 0xD,
    PITCH_BEND               = 0xE,
    PER_NOTE_PITCH_BEND      = 0x6,
    PER_NOTE_MANAGEMENT      = 0xF
};

// 32-bit word Universal MIDI Packet (UMP) supporting 32, 64, 96, 128-bit packets
struct UniversalMidiPacket {
    uint32_t words[4];
    uint8_t wordCount;

    UniversalMidiPacket() : wordCount(1) {
        words[0] = words[1] = words[2] = words[3] = 0;
    }

    UmpMessageType getMessageType() const {
        return static_cast<UmpMessageType>((words[0] >> 28) & 0x0F);
    }

    uint8_t getGroup() const {
        return static_cast<uint8_t>((words[0] >> 24) & 0x0F);
    }

    Midi2Status getStatus() const {
        return static_cast<Midi2Status>((words[0] >> 20) & 0x0F);
    }

    uint8_t getChannel() const {
        return static_cast<uint8_t>((words[0] >> 16) & 0x0F);
    }
};

class JZMidi2UniversalPacketEngine {
public:
    JZMidi2UniversalPacketEngine();
    ~JZMidi2UniversalPacketEngine() = default;

    // MIDI 2.0 Packet Generation (64-bit Channel Voice Messages)
    UniversalMidiPacket makeNoteOn(uint8_t group, uint8_t channel, uint8_t noteNumber,
                                   uint16_t velocity16, uint8_t attributeType = 0, uint16_t attributeData = 0);

    UniversalMidiPacket makeNoteOff(uint8_t group, uint8_t channel, uint8_t noteNumber,
                                    uint16_t velocity16);

    UniversalMidiPacket makeControlChange32(uint8_t group, uint8_t channel, uint8_t controllerIndex,
                                           uint32_t value32);

    UniversalMidiPacket makePerNotePitchBend(uint8_t group, uint8_t channel, uint8_t noteNumber,
                                            int32_t pitchBend32);

    UniversalMidiPacket makeChannelPitchBend32(uint8_t group, uint8_t channel,
                                              uint32_t pitchBend32);

    // Parsing & Conversion
    bool parseUmp(const UniversalMidiPacket& packet,
                  uint8_t& outChannel, uint8_t& outNoteOrIndex, uint32_t& out32BitValue);

    // Legacy MIDI 1.0 <-> MIDI 2.0 Translation
    static uint16_t midi1ToMidi2Velocity(uint8_t vel7);
    static uint8_t midi2ToMidi1Velocity(uint16_t vel16);
    static uint32_t midi1ToMidi2Controller(uint8_t cc7);
    static uint8_t midi2ToMidi1Controller(uint32_t cc32);

    // MIDI-CI (Capability Inquiry) & Property Exchange
    bool initiateCapabilityInquiry(uint8_t sourceMuid, uint8_t destinationMuid);
    bool isCiNegotiated() const { return m_ciNegotiated; }
    void setCiNegotiated(bool state) { m_ciNegotiated = state; }

    std::string buildPropertyExchangeGet(const std::string& resource);
    bool parsePropertyExchangeReply(const std::string& jsonPayload, std::string& outResource, std::string& outValue);

private:
    bool m_ciNegotiated;
    uint32_t m_localMuid;
    uint32_t m_remoteMuid;
};

} // namespace Jazz

#endif // JAZZ_MIDI2_UNIVERSAL_PACKET_ENGINE_H
