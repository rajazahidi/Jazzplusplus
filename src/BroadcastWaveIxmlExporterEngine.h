//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// BroadcastWaveIxmlExporterEngine.h - Lossless Broadcast Wave Format (BWF /
// EBU Tech 3285) & iXML Multi-Stem Metadata Exporter
//
// Production-grade RIFF chunk builder supporting Broadcast Audio Extension (bext)
// with SMPTE sample offsets, EBU R128 loudness metrics, and embedded iXML
// metadata schema (Scene, Take, Tape, Timecode Rate, Track Names, Polyphonic stems)
// for film post-production and broadcast facility delivery.
//*****************************************************************************

#ifndef JAZZ_BROADCAST_WAVE_IXML_EXPORTER_ENGINE_H
#define JAZZ_BROADCAST_WAVE_IXML_EXPORTER_ENGINE_H

#include <vector>
#include <string>
#include <cstdint>
#include <cstddef>

namespace Jazz {

struct BwfMetadata {
    std::string description = "Jazz++ Master Audio Stem";
    std::string originator = "Jazz++ DAW Studio";
    std::string originatorReference = "JZ-BWF-2026-REF01";
    std::string originationDate = "2026-10-25";
    std::string originationTime = "12:00:00";
    uint64_t timeReferenceSamples = 0; // SMPTE timecode sample offset from midnight
    int16_t loudnessValueCentiLufs = -2300;     // -23.00 LUFS
    uint16_t loudnessRangeCentiLu = 600;       // 6.00 LU
    int16_t maxTruePeakCentiDbFs = -100;       // -1.00 dBFS
};

struct IxmlMetadata {
    std::string project = "Sovereign Master Soundtrack";
    std::string scene = "101";
    std::string take = "01";
    std::string tape = "A01";
    std::string timecodeRate = "24/1"; // "23.976", "24/1", "25/1", "29.97", "30/1"
    std::vector<std::string> trackNames = { "Left", "Right" };
    bool interleavedPolyphonic = true;
};

class JZBroadcastWaveIxmlExporterEngine {
public:
    JZBroadcastWaveIxmlExporterEngine(float sampleRate = 48000.0f, uint16_t bitDepth = 24, uint16_t numChannels = 2);
    ~JZBroadcastWaveIxmlExporterEngine() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return m_sampleRate; }

    void setBitDepth(uint16_t bitDepth);
    uint16_t getBitDepth() const { return m_bitDepth; }

    void setNumChannels(uint16_t channels);
    uint16_t getNumChannels() const { return m_numChannels; }

    void setBwfMetadata(const BwfMetadata& bwf) { m_bwf = bwf; }
    const BwfMetadata& getBwfMetadata() const { return m_bwf; }

    void setIxmlMetadata(const IxmlMetadata& ixml) { m_ixml = ixml; }
    const IxmlMetadata& getIxmlMetadata() const { return m_ixml; }

    // Generates the raw RIFF WAVE byte stream with 'bext', 'iXML', 'fmt ', and 'data' chunk headers
    std::vector<uint8_t> generateBwfHeader(size_t audioDataPayloadBytes) const;

    // Generates raw 'bext' chunk bytes
    std::vector<uint8_t> buildBextChunk() const;

    // Generates raw 'iXML' XML string payload
    std::string buildIxmlPayloadString() const;

    // Verification & Inspection
    bool hasValidBextMarker(const std::vector<uint8_t>& fileHeader) const;
    bool hasValidIxmlMarker(const std::vector<uint8_t>& fileHeader) const;

private:
    float m_sampleRate;
    uint16_t m_bitDepth;
    uint16_t m_numChannels;
    BwfMetadata m_bwf;
    IxmlMetadata m_ixml;
};

} // namespace Jazz

#endif // JAZZ_BROADCAST_WAVE_IXML_EXPORTER_ENGINE_H
