//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// BroadcastWaveIxmlExporterEngine.cpp - Lossless Broadcast Wave Format (BWF /
// EBU Tech 3285) & iXML Multi-Stem Metadata Exporter
//*****************************************************************************

#include "BroadcastWaveIxmlExporterEngine.h"
#include <cstdint>
#include <cstring>
#include <sstream>
#include <algorithm>

namespace Jazz {

JZBroadcastWaveIxmlExporterEngine::JZBroadcastWaveIxmlExporterEngine(float sampleRate, uint16_t bitDepth, uint16_t numChannels)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f),
      m_bitDepth(bitDepth == 16 || bitDepth == 24 || bitDepth == 32 ? bitDepth : 24),
      m_numChannels(numChannels > 0 ? numChannels : 2)
{
}

void JZBroadcastWaveIxmlExporterEngine::setSampleRate(float sampleRate)
{
    m_sampleRate = sampleRate > 1000.0f ? sampleRate : 48000.0f;
}

void JZBroadcastWaveIxmlExporterEngine::setBitDepth(uint16_t bitDepth)
{
    m_bitDepth = (bitDepth == 16 || bitDepth == 24 || bitDepth == 32) ? bitDepth : 24;
}

void JZBroadcastWaveIxmlExporterEngine::setNumChannels(uint16_t channels)
{
    m_numChannels = (channels > 0) ? channels : 2;
}

static void appendUint32LE(std::vector<uint8_t>& buf, uint32_t val)
{
    buf.push_back(static_cast<uint8_t>(val & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >> 16) & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >> 24) & 0xFF));
}

static void appendUint16LE(std::vector<uint8_t>& buf, uint16_t val)
{
    buf.push_back(static_cast<uint8_t>(val & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
}

static void appendInt16LE(std::vector<uint8_t>& buf, int16_t val)
{
    appendUint16LE(buf, static_cast<uint16_t>(val));
}

static void appendPaddedString(std::vector<uint8_t>& buf, const std::string& str, size_t exactLength)
{
    size_t copyLen = std::min(str.size(), exactLength);
    for (size_t i = 0; i < copyLen; ++i) {
        buf.push_back(static_cast<uint8_t>(str[i]));
    }
    for (size_t i = copyLen; i < exactLength; ++i) {
        buf.push_back(0);
    }
}

std::vector<uint8_t> JZBroadcastWaveIxmlExporterEngine::buildBextChunk()
{
    std::vector<uint8_t> chunk;
    // FourCC 'bext'
    chunk.push_back('b'); chunk.push_back('e'); chunk.push_back('x'); chunk.push_back('t');

    // Standard BWF v2 bext chunk data payload length: 602 bytes
    const uint32_t payloadSize = 602;
    appendUint32LE(chunk, payloadSize);

    // 1. Description: 256 bytes
    appendPaddedString(chunk, m_bwf.description, 256);
    // 2. Originator: 32 bytes
    appendPaddedString(chunk, m_bwf.originator, 32);
    // 3. OriginatorReference: 32 bytes
    appendPaddedString(chunk, m_bwf.originatorReference, 32);
    // 4. OriginationDate: 10 bytes (ASCII "YYYY:MM:DD" or "YYYY-MM-DD")
    appendPaddedString(chunk, m_bwf.originationDate, 10);
    // 5. OriginationTime: 8 bytes (ASCII "HH:MM:SS")
    appendPaddedString(chunk, m_bwf.originationTime, 8);
    // 6. TimeReference: uint32 low, uint32 high (SMPTE samples from midnight)
    uint32_t timeRefLow = static_cast<uint32_t>(m_bwf.timeReferenceSamples & 0xFFFFFFFF);
    uint32_t timeRefHigh = static_cast<uint32_t>((m_bwf.timeReferenceSamples >> 32) & 0xFFFFFFFF);
    appendUint32LE(chunk, timeRefLow);
    appendUint32LE(chunk, timeRefHigh);
    // 7. Version: uint16 (0x0002 for BWF with loudness)
    appendUint16LE(chunk, 0x0002);
    // 8. UMID: 64 bytes binary
    for (size_t i = 0; i < 64; ++i) chunk.push_back(0);
    // 9. LoudnessValue: int16 (in centi-LUFS)
    appendInt16LE(chunk, m_bwf.loudnessValueCentiLufs);
    // 10. LoudnessRange: uint16 (in centi-LU)
    appendUint16LE(chunk, m_bwf.loudnessRangeCentiLu);
    // 11. MaxTruePeakLevel: int16 (in centi-dBFS)
    appendInt16LE(chunk, m_bwf.maxTruePeakCentiDbFs);
    // 12. MaxMomentaryLoudness: int16
    appendInt16LE(chunk, -2000);
    // 13. MaxShortTermLoudness: int16
    appendInt16LE(chunk, -2150);
    // 14. Reserved: 180 bytes padding
    for (size_t i = 0; i < 180; ++i) chunk.push_back(0);

    return chunk;
}

std::string JZBroadcastWaveIxmlExporterEngine::buildIxmlPayloadString() const
{
    std::ostringstream ss;
    ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    ss << "<BWFXML>\n";
    ss << "  <IXML_VERSION>1.52</IXML_VERSION>\n";
    ss << "  <PROJECT>" << m_ixml.project << "</PROJECT>\n";
    ss << "  <SCENE>" << m_ixml.scene << "</SCENE>\n";
    ss << "  <TAKE>" << m_ixml.take << "</TAKE>\n";
    ss << "  <TAPE>" << m_ixml.tape << "</TAPE>\n";
    ss << "  <SPEED>\n";
    ss << "    <TIMECODE_RATE>" << m_ixml.timecodeRate << "</TIMECODE_RATE>\n";
    ss << "  </SPEED>\n";
    ss << "  <TRACK_LIST>\n";
    ss << "    <TRACK_COUNT>" << m_ixml.trackNames.size() << "</TRACK_COUNT>\n";
    for (size_t i = 0; i < m_ixml.trackNames.size(); ++i) {
        ss << "    <TRACK>\n";
        ss << "      <CHANNEL_INDEX>" << (i + 1) << "</CHANNEL_INDEX>\n";
        ss << "      <NAME>" << m_ixml.trackNames[i] << "</NAME>\n";
        ss << "    </TRACK>\n";
    }
    ss << "  </TRACK_LIST>\n";
    ss << "</BWFXML>\n";
    return ss.str();
}

std::vector<uint8_t> JZBroadcastWaveIxmlExporterEngine::generateBwfHeader(size_t audioDataPayloadBytes) const
{
    std::vector<uint8_t> header;

    // RIFF chunk ID
    header.push_back('R'); header.push_back('I'); header.push_back('F'); header.push_back('F');
    // Total file size placeholder (36 + bextSize + ixmlSize + dataSize)
    size_t fileSizePos = header.size();
    appendUint32LE(header, 0); // Temporary

    // WAVE format ID
    header.push_back('W'); header.push_back('A'); header.push_back('V'); header.push_back('E');

    // 1. 'bext' Broadcast Extension chunk
    std::vector<uint8_t> bextChunk = const_cast<JZBroadcastWaveIxmlExporterEngine*>(this)->buildBextChunk();
    header.insert(header.end(), bextChunk.begin(), bextChunk.end());

    // 2. 'iXML' chunk
    std::string ixmlStr = buildIxmlPayloadString();
    header.push_back('i'); header.push_back('X'); header.push_back('M'); header.push_back('L');
    uint32_t ixmlLen = static_cast<uint32_t>(ixmlStr.size());
    appendUint32LE(header, ixmlLen);
    header.insert(header.end(), ixmlStr.begin(), ixmlStr.end());
    // Pad byte if odd length
    if (ixmlLen % 2 != 0) {
        header.push_back(0);
    }

    // 3. 'fmt ' chunk
    header.push_back('f'); header.push_back('m'); header.push_back('t'); header.push_back(' ');
    appendUint32LE(header, 16); // PCM chunk size
    appendUint16LE(header, 1);  // Audio format 1 = PCM
    appendUint16LE(header, m_numChannels);
    appendUint32LE(header, static_cast<uint32_t>(m_sampleRate));
    uint32_t byteRate = static_cast<uint32_t>(m_sampleRate) * m_numChannels * (m_bitDepth / 8);
    appendUint32LE(header, byteRate);
    uint16_t blockAlign = m_numChannels * (m_bitDepth / 8);
    appendUint16LE(header, blockAlign);
    appendUint16LE(header, m_bitDepth);

    // 4. 'data' chunk header
    header.push_back('d'); header.push_back('a'); header.push_back('t'); header.push_back('a');
    appendUint32LE(header, static_cast<uint32_t>(audioDataPayloadBytes));

    // Update RIFF total size: header size minus 8 + audioDataPayloadBytes
    uint32_t totalRiffSize = static_cast<uint32_t>(header.size() - 8 + audioDataPayloadBytes);
    header[fileSizePos] = static_cast<uint8_t>(totalRiffSize & 0xFF);
    header[fileSizePos + 1] = static_cast<uint8_t>((totalRiffSize >> 8) & 0xFF);
    header[fileSizePos + 2] = static_cast<uint8_t>((totalRiffSize >> 16) & 0xFF);
    header[fileSizePos + 3] = static_cast<uint8_t>((totalRiffSize >> 24) & 0xFF);

    return header;
}

bool JZBroadcastWaveIxmlExporterEngine::hasValidBextMarker(const std::vector<uint8_t>& fileHeader) const
{
    if (fileHeader.size() < 12) return false;
    // Search for 'bext'
    for (size_t i = 12; i + 4 <= fileHeader.size(); ++i) {
        if (fileHeader[i] == 'b' && fileHeader[i+1] == 'e' && fileHeader[i+2] == 'x' && fileHeader[i+3] == 't') {
            return true;
        }
    }
    return false;
}

bool JZBroadcastWaveIxmlExporterEngine::hasValidIxmlMarker(const std::vector<uint8_t>& fileHeader) const
{
    if (fileHeader.size() < 12) return false;
    // Search for 'iXML'
    for (size_t i = 12; i + 4 <= fileHeader.size(); ++i) {
        if (fileHeader[i] == 'i' && fileHeader[i+1] == 'X' && fileHeader[i+2] == 'M' && fileHeader[i+3] == 'L') {
            return true;
        }
    }
    return false;
}

} // namespace Jazz
