//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZAudioWatermarkProvenanceEngine.h - Audio Watermarking & Provenance Hub
//
// Broadcast & copyright audio watermarking and cryptographic provenance:
// Inaudible Direct Sequence Spread Spectrum (DSSS) psychoacoustic watermark,
// SHA-256 audio payload verification, and standardized ISRC/ISWC/BWF metadata.
//*****************************************************************************

#ifndef JAZZ_AUDIO_WATERMARK_PROVENANCE_ENGINE_H
#define JAZZ_AUDIO_WATERMARK_PROVENANCE_ENGINE_H

#include <vector>
#include <string>
#include <cstdint>

namespace Jazz {

struct AudioProvenanceMetadata {
    std::string trackTitle;
    std::string artistName;
    std::string isrcCode;          // e.g. "US-JAZ-26-00042" (12 characters)
    std::string iswcCode;          // e.g. "T-034.524.680-1" (11 characters)
    std::string copyrightNotice;   // "(C) 2026 Jazz++ Sound Lab"
    std::string sha256AudioHash;   // 64-character hex hash of raw audio payload
    uint32_t watermarkPayloadId;   // Embedded 32-bit DSSS watermark identifier
};

class JZAudioWatermarkProvenanceEngine {
public:
    JZAudioWatermarkProvenanceEngine(uint32_t pnSeedKey = 0x5A827999);
    ~JZAudioWatermarkProvenanceEngine() = default;

    void setPnSeedKey(uint32_t seed);
    uint32_t getPnSeedKey() const { return m_pnSeedKey; }

    // DSSS Psychoacoustic Watermark Embedder
    // Embeds 32-bit payload below masking threshold into audio buffer
    bool embedWatermark(float* audioL, float* audioR, size_t numSamples,
                        uint32_t payloadId, float embeddingStrength = 0.002f);

    // DSSS Watermark Detector
    // Extracts embedded 32-bit payload via cross-correlation with PN sequence
    bool detectWatermark(const float* audioL, const float* audioR, size_t numSamples,
                         uint32_t& outPayloadId, float& outConfidence) const;

    // Cryptographic SHA-256 Provenance & BWF Metadata Packaging
    std::string computeAudioSha256(const float* audioL, const float* audioR, size_t numSamples) const;

    AudioProvenanceMetadata generateProvenancePackage(const std::string& title,
                                                     const std::string& artist,
                                                     const std::string& isrc,
                                                     const std::string& iswc,
                                                     const float* audioL,
                                                     const float* audioR,
                                                     size_t numSamples,
                                                     uint32_t payloadId);

    // Verify audio integrity against expected SHA-256 hash
    bool verifyAudioIntegrity(const float* audioL, const float* audioR, size_t numSamples,
                              const std::string& expectedHash) const;

    // Export standardized BWF <bext> description text
    std::string exportBwfBextMetadata(const AudioProvenanceMetadata& meta) const;

private:
    uint32_t m_pnSeedKey;

    // Pseudo-Noise (PN) chip sequence generator
    std::vector<int8_t> generatePnSequence(size_t length) const;
};

} // namespace Jazz

#endif // JAZZ_AUDIO_WATERMARK_PROVENANCE_ENGINE_H
