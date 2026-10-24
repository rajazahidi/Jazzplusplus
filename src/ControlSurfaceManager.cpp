#include "ControlSurfaceManager.h"
#include <algorithm>
#include <cstring>

namespace Jazz {

JZControlSurfaceManager::JZControlSurfaceManager(ControlSurfaceProtocol protocol)
    : m_protocol(protocol)
    , m_bankOffset(0)
    , m_isPlaying(false)
    , m_isRecording(false)
    , m_transportTicks(0)
{
    m_channelStrips.resize(8);
    for (int i = 0; i < 8; ++i) {
        m_channelStrips[i].channelIndex = i;
        m_channelStrips[i].trackName = "Trk " + std::to_string(i + 1);
        m_channelStrips[i].motorizedFader10Bit = 800; // ~0 dB
        m_channelStrips[i].vpotPosition = 64;        // Center pan
        m_channelStrips[i].ledMode = VPotLedRingMode::SINGLE_DOT;
        m_channelStrips[i].isMuted = false;
        m_channelStrips[i].isSoloed = false;
        m_channelStrips[i].isArmed = false;
        m_channelStrips[i].isSelected = (i == 0);
    }
}

void JZControlSurfaceManager::bankLeft()
{
    m_bankOffset = std::max(0, m_bankOffset - 8);
}

void JZControlSurfaceManager::bankRight(int maxTracks)
{
    if (m_bankOffset + 8 < maxTracks) {
        m_bankOffset += 8;
    }
}

void JZControlSurfaceManager::channelLeft()
{
    m_bankOffset = std::max(0, m_bankOffset - 1);
}

void JZControlSurfaceManager::channelRight(int maxTracks)
{
    if (m_bankOffset + 1 < maxTracks) {
        m_bankOffset += 1;
    }
}

void JZControlSurfaceManager::setTrackInfo(int globalTrackIdx, const std::string& trackName, float volumeNorm, float panNorm)
{
    int localIdx = globalTrackIdx - m_bankOffset;
    if (localIdx >= 0 && localIdx < 8) {
        std::string trimmed = trackName.substr(0, 7);
        while (trimmed.size() < 7) trimmed.push_back(' ');
        m_channelStrips[localIdx].trackName = trimmed;

        uint16_t fader10 = static_cast<uint16_t>(std::clamp(volumeNorm, 0.0f, 1.0f) * 1023.0f);
        m_channelStrips[localIdx].motorizedFader10Bit = fader10;

        uint8_t pan7 = static_cast<uint8_t>(std::clamp(panNorm, 0.0f, 1.0f) * 127.0f);
        m_channelStrips[localIdx].vpotPosition = pan7;
    }
}

const ChannelStripHardwareState* JZControlSurfaceManager::getChannelState(int localFaderIdx) const
{
    if (localFaderIdx >= 0 && localFaderIdx < 8) {
        return &(m_channelStrips[localFaderIdx]);
    }
    return nullptr;
}

std::vector<uint8_t> JZControlSurfaceManager::generateFaderMotorMessage(int localFaderIdx, uint16_t faderVal10Bit) const
{
    std::vector<uint8_t> msg;
    if (localFaderIdx < 0 || localFaderIdx > 8) return msg;

    // MCU Motorized Fader uses Pitch Bend message on channel = localFaderIdx (0xE0 to 0xE8)
    // 14-bit pitch bend: lsb = val & 0x7F, msb = (val >> 7) & 0x7F
    // Map 10-bit [0..1023] to 14-bit [0..16383]
    uint16_t val14 = static_cast<uint16_t>((faderVal10Bit & 0x3FF) << 4);
    uint8_t status = static_cast<uint8_t>(0xE0 | (localFaderIdx & 0x0F));
    uint8_t lsb = static_cast<uint8_t>(val14 & 0x7F);
    uint8_t msb = static_cast<uint8_t>((val14 >> 7) & 0x7F);

    msg.push_back(status);
    msg.push_back(lsb);
    msg.push_back(msb);
    return msg;
}

bool JZControlSurfaceManager::parseFaderMoveMessage(
    uint8_t statusByte, uint8_t data1, uint8_t data2, int& outLocalFaderIdx, uint16_t& outFaderVal10Bit) const
{
    if ((statusByte & 0xF0) != 0xE0) return false;

    outLocalFaderIdx = statusByte & 0x0F;
    if (outLocalFaderIdx > 8) return false; // 0..7 tracks, 8 = master fader

    uint16_t val14 = static_cast<uint16_t>((data2 & 0x7F) << 7 | (data1 & 0x7F));
    outFaderVal10Bit = val14 >> 4; // Back to 10-bit
    return true;
}

std::vector<uint8_t> JZControlSurfaceManager::generateScribbleStripSysEx(int lineIndex) const
{
    std::vector<uint8_t> sysex;
    // MCU LCD Header: F0 00 00 66 14 12 <offset> ... F7
    sysex.push_back(0xF0);
    sysex.push_back(0x00);
    sysex.push_back(0x00);
    sysex.push_back(0x66);
    sysex.push_back(0x14);
    sysex.push_back(0x12);

    uint8_t offset = (lineIndex == 0) ? 0x00 : 0x38; // Line 0 (top), Line 1 (bottom)
    sysex.push_back(offset);

    // 8 channels * 7 characters = 56 characters
    for (int ch = 0; ch < 8; ++ch) {
        std::string name = m_channelStrips[ch].trackName;
        while (name.size() < 7) name.push_back(' ');
        for (int c = 0; c < 7; ++c) {
            sysex.push_back(static_cast<uint8_t>(name[c] & 0x7F));
        }
    }

    sysex.push_back(0xF7);
    return sysex;
}

int JZControlSurfaceManager::processJogWheelDelta(int8_t tickDelta)
{
    m_transportTicks += tickDelta * 24; // 24 ticks per jog notch (quarter note subdivision)
    if (m_transportTicks < 0) m_transportTicks = 0;
    return static_cast<int>(m_transportTicks);
}

} // namespace Jazz
