//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZAudioWatermarkProvenanceEngine.cpp - Audio Watermarking & Provenance Hub
//*****************************************************************************

#include "AudioWatermarkProvenanceEngine.h"
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cmath>

namespace Jazz {

JZAudioWatermarkProvenanceEngine::JZAudioWatermarkProvenanceEngine(uint32_t pnSeedKey)
    : m_pnSeedKey(pnSeedKey != 0 ? pnSeedKey : 0x5A827999)
{
}

void JZAudioWatermarkProvenanceEngine::setPnSeedKey(uint32_t seed) {
    if (seed != 0) {
        m_pnSeedKey = seed;
    }
}

std::vector<int8_t> JZAudioWatermarkProvenanceEngine::generatePnSequence(size_t length) const {
    std::vector<int8_t> pn(length);
    uint32_t state = m_pnSeedKey;

    for (size_t i = 0; i < length; ++i) {
        state = (state * 1664525U + 1013904223U);
        pn[i] = ((state >> 31) & 0x01) ? 1 : -1;
    }
    return pn;
}

bool JZAudioWatermarkProvenanceEngine::embedWatermark(float* audioL, float* audioR, size_t numSamples,
                                                     uint32_t payloadId, float embeddingStrength) {
    if (!audioL || !audioR || numSamples < 1024) return false;

    const size_t BITS = 32;
    size_t chipsPerBit = numSamples / BITS;
    if (chipsPerBit < 16) return false;

    std::vector<int8_t> pn = generatePnSequence(numSamples);
    float strength = std::clamp(embeddingStrength, 0.0005f, 0.02f);

    for (size_t b = 0; b < BITS; ++b) {
        int bit = (payloadId >> (31 - b)) & 0x01;
        float bitPolarity = (bit == 1) ? 1.0f : -1.0f;

        size_t startIdx = b * chipsPerBit;
        size_t endIdx = (b == BITS - 1) ? numSamples : (startIdx + chipsPerBit);

        for (size_t i = startIdx; i < endIdx; ++i) {
            float chipVal = static_cast<float>(pn[i]) * bitPolarity * strength;
            audioL[i] += chipVal;
            audioR[i] += chipVal;
        }
    }

    return true;
}

bool JZAudioWatermarkProvenanceEngine::detectWatermark(const float* audioL, const float* audioR,
                                                      size_t numSamples, uint32_t& outPayloadId,
                                                      float& outConfidence) const {
    outPayloadId = 0;
    outConfidence = 0.0f;
    if (!audioL || !audioR || numSamples < 1024) return false;

    const size_t BITS = 32;
    size_t chipsPerBit = numSamples / BITS;
    if (chipsPerBit < 16) return false;

    std::vector<int8_t> pn = generatePnSequence(numSamples);
    double totalCorrelation = 0.0;
    uint32_t extractedPayload = 0;

    for (size_t b = 0; b < BITS; ++b) {
        size_t startIdx = b * chipsPerBit;
        size_t endIdx = (b == BITS - 1) ? numSamples : (startIdx + chipsPerBit);

        double corrSum = 0.0;
        for (size_t i = startIdx; i < endIdx; ++i) {
            float mono = (audioL[i] + audioR[i]) * 0.5f;
            corrSum += mono * static_cast<float>(pn[i]);
        }

        if (corrSum > 0.0) {
            extractedPayload |= (1U << (31 - b));
        }
        totalCorrelation += std::abs(corrSum);
    }

    outPayloadId = extractedPayload;
    outConfidence = static_cast<float>(totalCorrelation / (numSamples * 0.001f));
    return (outConfidence > 0.1f);
}

// Compact, standard-compliant SHA-256 implementation
namespace {
    inline uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }
    inline uint32_t ch(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (~x & z); }
    inline uint32_t maj(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (x & z) ^ (y & z); }
    inline uint32_t sig0(uint32_t x) { return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22); }
    inline uint32_t sig1(uint32_t x) { return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25); }
    inline uint32_t gam0(uint32_t x) { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
    inline uint32_t gam1(uint32_t x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }

    static const uint32_t K256[64] = {
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
    };
}

std::string JZAudioWatermarkProvenanceEngine::computeAudioSha256(const float* audioL, const float* audioR, size_t numSamples) const {
    if (!audioL || !audioR || numSamples == 0) return "0000000000000000000000000000000000000000000000000000000000000000";

    // Flatten audio floats into byte stream
    std::vector<uint8_t> data;
    size_t sampleCount = std::min(numSamples, static_cast<size_t>(4096)); // Fast representative hash
    data.reserve(sampleCount * 8);

    for (size_t i = 0; i < sampleCount; ++i) {
        int16_t sL = static_cast<int16_t>(std::clamp(audioL[i], -1.0f, 1.0f) * 32767.0f);
        int16_t sR = static_cast<int16_t>(std::clamp(audioR[i], -1.0f, 1.0f) * 32767.0f);
        data.push_back(static_cast<uint8_t>(sL & 0xFF));
        data.push_back(static_cast<uint8_t>((sL >> 8) & 0xFF));
        data.push_back(static_cast<uint8_t>(sR & 0xFF));
        data.push_back(static_cast<uint8_t>((sR >> 8) & 0xFF));
    }

    // SHA-256 padding
    uint64_t bitLen = data.size() * 8;
    data.push_back(0x80);
    while ((data.size() % 64) != 56) {
        data.push_back(0x00);
    }
    for (int i = 7; i >= 0; --i) {
        data.push_back(static_cast<uint8_t>((bitLen >> (i * 8)) & 0xFF));
    }

    uint32_t H[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    for (size_t chunk = 0; chunk < data.size(); chunk += 64) {
        uint32_t W[64];
        for (int t = 0; t < 16; ++t) {
            W[t] = (static_cast<uint32_t>(data[chunk + t * 4]) << 24) |
                   (static_cast<uint32_t>(data[chunk + t * 4 + 1]) << 16) |
                   (static_cast<uint32_t>(data[chunk + t * 4 + 2]) << 8) |
                   (static_cast<uint32_t>(data[chunk + t * 4 + 3]));
        }
        for (int t = 16; t < 64; ++t) {
            W[t] = gam1(W[t - 2]) + W[t - 7] + gam0(W[t - 15]) + W[t - 16];
        }

        uint32_t a = H[0], b = H[1], c = H[2], d = H[3], e = H[4], f = H[5], g = H[6], h = H[7];
        for (int t = 0; t < 64; ++t) {
            uint32_t T1 = h + sig1(e) + ch(e, f, g) + K256[t] + W[t];
            uint32_t T2 = sig0(a) + maj(a, b, c);
            h = g; g = f; f = e; e = d + T1;
            d = c; c = b; b = a; a = T1 + T2;
        }
        H[0] += a; H[1] += b; H[2] += c; H[3] += d;
        H[4] += e; H[5] += f; H[6] += g; H[7] += h;
    }

    std::ostringstream ss;
    for (int i = 0; i < 8; ++i) {
        ss << std::hex << std::setfill('0') << std::setw(8) << H[i];
    }
    return ss.str();
}

AudioProvenanceMetadata JZAudioWatermarkProvenanceEngine::generateProvenancePackage(
    const std::string& title, const std::string& artist,
    const std::string& isrc, const std::string& iswc,
    const float* audioL, const float* audioR,
    size_t numSamples, uint32_t payloadId) {

    AudioProvenanceMetadata meta;
    meta.trackTitle = title;
    meta.artistName = artist;
    meta.isrcCode = isrc;
    meta.iswcCode = iswc;
    meta.copyrightNotice = "(C) 2026 Jazz++ Sound Lab. All Rights Reserved.";
    meta.watermarkPayloadId = payloadId;
    meta.sha256AudioHash = computeAudioSha256(audioL, audioR, numSamples);
    return meta;
}

bool JZAudioWatermarkProvenanceEngine::verifyAudioIntegrity(const float* audioL, const float* audioR,
                                                           size_t numSamples, const std::string& expectedHash) const {
    std::string currentHash = computeAudioSha256(audioL, audioR, numSamples);
    return (currentHash == expectedHash);
}

std::string JZAudioWatermarkProvenanceEngine::exportBwfBextMetadata(const AudioProvenanceMetadata& meta) const {
    std::ostringstream ss;
    ss << "BWF_BEXT_METADATA_CHUNK [EBU Tech 3285 / ITU-R BS.2076]\n"
       << "Description=" << meta.trackTitle << "\n"
       << "Originator=" << meta.artistName << "\n"
       << "ISRC=" << meta.isrcCode << "\n"
       << "ISWC=" << meta.iswcCode << "\n"
       << "Copyright=" << meta.copyrightNotice << "\n"
       << "AudioSHA256=" << meta.sha256AudioHash << "\n"
       << "WatermarkID=0x" << std::hex << std::uppercase << meta.watermarkPayloadId << "\n";
    return ss.str();
}

} // namespace Jazz
