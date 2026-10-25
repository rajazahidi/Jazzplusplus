//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZDsdDxdEngine.h - Ultra-High-Resolution DSD / DXD Audio Engine
//
// Audiophile SACD, Direct Stream Digital (DSD64, DSD128, DSD256),
// DSD-over-PCM (DoP v1.1) Framing/Unframing, and Digital eXtreme Definition
// (DXD 352.8 kHz / 32-bit float) Multi-Stage Decimation/Modulation Engine.
//*****************************************************************************

#ifndef JAZZ_DSD_DXD_ENGINE_H
#define JAZZ_DSD_DXD_ENGINE_H

#include <vector>
#include <string>
#include <cstdint>
#include <cmath>

namespace Jazz {

enum class DsdRate {
    DSD64,   // 2.8224 MHz (64 x 44.1 kHz, SACD Standard)
    DSD128,  // 5.6448 MHz (128 x 44.1 kHz, Double DSD)
    DSD256   // 11.2896 MHz (256 x 44.1 kHz, Quad DSD)
};

class JZDsdDxdEngine {
public:
    JZDsdDxdEngine(DsdRate rate = DsdRate::DSD64);
    ~JZDsdDxdEngine() = default;

    // Rate configuration
    void setDsdRate(DsdRate rate);
    DsdRate getDsdRate() const { return m_rate; }
    double getDsdSamplingFrequencyHz() const;
    int getDecimationRatio() const;

    // DSD-over-PCM (DoP v1.1) Framing & Extraction
    // Encodes 16-bit DSD chunks into 24-bit PCM container with alternating 0x05 / 0xFA marker
    bool encodeDoP(const std::vector<uint8_t>& dsdBitstream, std::vector<uint32_t>& outPcm24Frames);
    bool decodeDoP(const std::vector<uint32_t>& inPcm24Frames, std::vector<uint8_t>& outDsdBitstream);
    bool isDopLocked() const { return m_dopLocked; }

    // Multi-Stage Decimation: 1-Bit DSD Stream -> 352.8 kHz 32-bit Float DXD PCM
    void convertDsdToDxd(const std::vector<uint8_t>& dsdBits, std::vector<float>& outDxdPcm);

    // 5th-Order Sigma-Delta Modulation: 352.8 kHz DXD PCM -> 1-Bit DSD Stream
    void convertDxdToDsd(const std::vector<float>& dxdPcm, std::vector<uint8_t>& outDsdBits);

    // Ultrasonic noise shaping analysis (measures noise above 50 kHz in dB)
    float calculateUltrasonicNoiseFloorDb(const std::vector<float>& dxdPcm) const;

    // Filtering controls
    void setUltrasonicCutoffHz(float cutoffHz);
    float getUltrasonicCutoffHz() const { return m_ultrasonicCutoffHz; }

    void reset();

private:
    DsdRate m_rate;
    bool m_dopLocked;
    uint8_t m_lastDopMarker;
    float m_ultrasonicCutoffHz;

    // 5th-order Sigma-Delta Modulator filter states
    double m_integrator[5];
};

} // namespace Jazz

#endif // JAZZ_DSD_DXD_ENGINE_H
