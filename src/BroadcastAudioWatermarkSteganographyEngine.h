//=============================================================================
// BroadcastAudioWatermarkSteganographyEngine.h
//
// Master Studio Pro Suite v30.0 - Apex Colossus Sovereign Diamond Jubilee Release
// Broadcast Audio Psychoacoustic Watermark & DSSS Steganography Engine.
//
// Features:
// - Direct Sequence Spread Spectrum (DSSS) psychoacoustic watermark embedding
// - Binary Phase Shift Keying (BPSK) with 63-bit Gold Code pseudo-noise sequence
// - Sub-audible perceptual masking threshold (-26 dB below audio energy)
// - 64-bit standardized frame: 16-bit Sync Preamble (0xA55A) + 32-bit Payload + 16-bit CRC
// - Robust watermark extraction surviving lossy MP3/AAC compression & gain changes
// - Bit Error Rate (BER) & cross-correlation confidence scoring
//=============================================================================

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace Jazz {

class JZBroadcastAudioWatermarkSteganographyEngine {
public:
    explicit JZBroadcastAudioWatermarkSteganographyEngine(float sampleRate = 48000.0f);
    ~JZBroadcastAudioWatermarkSteganographyEngine() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return mSampleRate; }

    // Watermark Configuration
    void setWatermarkStrengthDb(float db); // -36.0 dB to -18.0 dB (nominally -26 dB)
    float getWatermarkStrengthDb() const { return mStrengthDb; }

    void setCarrierFrequencyHz(float freqHz); // 1500 Hz to 6000 Hz (nominally 3200 Hz)
    float getCarrierFrequencyHz() const { return mCarrierHz; }

    // Watermark Embedding
    void embedWatermark(const float* inL, const float* inR,
                        float* outL, float* outR, size_t numSamples,
                        uint32_t payload32);

    // Watermark Detection & Decoding
    bool detectWatermark(const float* inL, const float* inR, size_t numSamples,
                         uint32_t& outPayload, float& confidence);

    // Query Results
    bool isPayloadValid() const { return mLastValid; }
    uint32_t getLastDecodedPayload() const { return mLastDecodedPayload; }
    float getLastConfidence() const { return mLastConfidence; }
    float getBitErrorRate() const { return mBitErrorRate; }

    void reset();

    // Utility: 16-bit CCITT CRC
    static uint16_t calculateCrc16(const uint8_t* data, size_t len);

private:
    float mSampleRate;
    float mStrengthDb;
    float mCarrierHz;

    // Detection telemetry
    bool mLastValid;
    uint32_t mLastDecodedPayload;
    float mLastConfidence;
    float mBitErrorRate;

    // Frame parameters
    static constexpr size_t CHIPS_PER_BIT = 63;
    static constexpr size_t SAMPLES_PER_CHIP = 4;
    static constexpr size_t TOTAL_FRAME_BITS = 64; // 16 sync + 32 payload + 16 crc
    static constexpr uint16_t SYNC_PREAMBLE = 0xA55A;

    // 63-bit PN Gold Code sequence (-1.0f or +1.0f)
    std::vector<float> mPnSequence;

    // Embedding state
    size_t mEmbedSampleCounter;
    float mEmbedCarrierPhase;

    // Detection state
    size_t mDetectSampleCounter;
    float mDetectCarrierPhase;
    std::vector<float> mBitCorrelators;
    std::vector<uint8_t> mReceivedBits;

    void initPnSequence();
    std::vector<uint8_t> serializeFrame(uint32_t payload, uint16_t crc);
    bool deserializeFrame(const std::vector<uint8_t>& bits, uint32_t& outPayload, float& confidence);
};

} // namespace Jazz
