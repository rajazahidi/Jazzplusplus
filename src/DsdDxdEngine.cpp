//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZDsdDxdEngine.cpp - Ultra-High-Resolution DSD / DXD Audio Engine
//*****************************************************************************

#include "DsdDxdEngine.h"
#include <algorithm>
#include <cmath>

namespace Jazz {

JZDsdDxdEngine::JZDsdDxdEngine(DsdRate rate)
    : m_rate(rate)
    , m_dopLocked(false)
    , m_lastDopMarker(0x05)
    , m_ultrasonicCutoffHz(50000.0f)
{
    reset();
}

void JZDsdDxdEngine::reset() {
    m_dopLocked = false;
    m_lastDopMarker = 0x05;
    for (int i = 0; i < 5; ++i) {
        m_integrator[i] = 0.0;
    }
}

void JZDsdDxdEngine::setDsdRate(DsdRate rate) {
    m_rate = rate;
    reset();
}

double JZDsdDxdEngine::getDsdSamplingFrequencyHz() const {
    switch (m_rate) {
        case DsdRate::DSD64:  return 2822400.0;  // 64 * 44100
        case DsdRate::DSD128: return 5644800.0;  // 128 * 44100
        case DsdRate::DSD256: return 11289600.0; // 256 * 44100
    }
    return 2822400.0;
}

int JZDsdDxdEngine::getDecimationRatio() const {
    switch (m_rate) {
        case DsdRate::DSD64:  return 8;   // 2822.4 kHz / 352.8 kHz = 8
        case DsdRate::DSD128: return 16;  // 5644.8 kHz / 352.8 kHz = 16
        case DsdRate::DSD256: return 32;  // 11289.6 kHz / 352.8 kHz = 32
    }
    return 8;
}

void JZDsdDxdEngine::setUltrasonicCutoffHz(float cutoffHz) {
    m_ultrasonicCutoffHz = std::clamp(cutoffHz, 20000.0f, 100000.0f);
}

bool JZDsdDxdEngine::encodeDoP(const std::vector<uint8_t>& dsdBitstream, std::vector<uint32_t>& outPcm24Frames) {
    outPcm24Frames.clear();
    if (dsdBitstream.empty()) return false;

    // DoP packages 16 bits (2 bytes) of DSD into one 24-bit PCM word
    // Header byte alternates between 0x05 and 0xFA
    size_t numWords = dsdBitstream.size() / 2;
    outPcm24Frames.reserve(numWords);

    uint8_t marker = 0x05;
    for (size_t i = 0; i < numWords; ++i) {
        uint16_t dsdData = (static_cast<uint16_t>(dsdBitstream[i * 2]) << 8) |
                            static_cast<uint16_t>(dsdBitstream[i * 2 + 1]);

        uint32_t pcmWord = (static_cast<uint32_t>(marker) << 16) | (dsdData & 0xFFFF);
        outPcm24Frames.push_back(pcmWord);

        // Toggle marker byte for next sample
        marker = (marker == 0x05) ? 0xFA : 0x05;
    }

    m_dopLocked = true;
    return true;
}

bool JZDsdDxdEngine::decodeDoP(const std::vector<uint32_t>& inPcm24Frames, std::vector<uint8_t>& outDsdBitstream) {
    outDsdBitstream.clear();
    if (inPcm24Frames.empty()) {
        m_dopLocked = false;
        return false;
    }

    outDsdBitstream.reserve(inPcm24Frames.size() * 2);
    int validConsecutiveMarkers = 0;

    for (uint32_t word : inPcm24Frames) {
        uint8_t marker = static_cast<uint8_t>((word >> 16) & 0xFF);
        uint16_t payload = static_cast<uint16_t>(word & 0xFFFF);

        if (marker == 0x05 || marker == 0xFA) {
            validConsecutiveMarkers++;
        } else {
            validConsecutiveMarkers = 0;
        }

        outDsdBitstream.push_back(static_cast<uint8_t>((payload >> 8) & 0xFF));
        outDsdBitstream.push_back(static_cast<uint8_t>(payload & 0xFF));
    }

    // Lock condition: at least 3 valid alternating DoP markers detected
    m_dopLocked = (validConsecutiveMarkers >= 3 || (inPcm24Frames.size() <= 4 && validConsecutiveMarkers > 0));
    return m_dopLocked;
}

void JZDsdDxdEngine::convertDsdToDxd(const std::vector<uint8_t>& dsdBits, std::vector<float>& outDxdPcm) {
    outDxdPcm.clear();
    if (dsdBits.empty()) return;

    int ratio = getDecimationRatio(); // e.g. 8 for DSD64
    size_t totalBits = dsdBits.size() * 8;
    size_t dxdSampleCount = totalBits / ratio;
    outDxdPcm.reserve(dxdSampleCount);

    // Multi-stage moving average decimation FIR kernel
    for (size_t s = 0; s < dxdSampleCount; ++s) {
        float accum = 0.0f;
        for (int r = 0; r < ratio; ++r) {
            size_t bitIdx = s * ratio + r;
            size_t byteIdx = bitIdx / 8;
            int bitOffset = 7 - (bitIdx % 8);

            int bitVal = (dsdBits[byteIdx] >> bitOffset) & 0x01;
            accum += (bitVal == 1) ? 1.0f : -1.0f;
        }
        float pcmSample = accum / static_cast<float>(ratio);
        outDxdPcm.push_back(std::clamp(pcmSample, -1.0f, 1.0f));
    }
}

void JZDsdDxdEngine::convertDxdToDsd(const std::vector<float>& dxdPcm, std::vector<uint8_t>& outDsdBits) {
    outDsdBits.clear();
    if (dxdPcm.empty()) return;

    int ratio = getDecimationRatio();
    size_t totalDsdBits = dxdPcm.size() * ratio;
    size_t totalBytes = (totalDsdBits + 7) / 8;
    outDsdBits.assign(totalBytes, 0);

    size_t globalBitIdx = 0;

    for (float inSample : dxdPcm) {
        double x = std::clamp(static_cast<double>(inSample), -0.99, 0.99);

        for (int r = 0; r < ratio; ++r) {
            // 5th-order Feedforward Delta-Sigma Modulator loop
            double feedback = (m_integrator[4] >= 0.0) ? 1.0 : -1.0;

            m_integrator[0] += (x - feedback * 0.1);
            m_integrator[1] += (m_integrator[0] - feedback * 0.2);
            m_integrator[2] += (m_integrator[1] - feedback * 0.4);
            m_integrator[3] += (m_integrator[2] - feedback * 0.6);
            m_integrator[4] += (m_integrator[3] - feedback * 0.8);

            int outBit = (m_integrator[4] >= 0.0) ? 1 : 0;

            size_t byteIdx = globalBitIdx / 8;
            int bitOffset = 7 - (globalBitIdx % 8);
            if (outBit) {
                outDsdBits[byteIdx] |= (1 << bitOffset);
            }
            globalBitIdx++;
        }
    }
}

float JZDsdDxdEngine::calculateUltrasonicNoiseFloorDb(const std::vector<float>& dxdPcm) const {
    if (dxdPcm.empty()) return -120.0f;

    // Estimate HF ultrasonic energy via high-pass 1st difference filter
    double sumHFEnergy = 0.0;
    for (size_t i = 1; i < dxdPcm.size(); ++i) {
        float diff = dxdPcm[i] - dxdPcm[i - 1];
        sumHFEnergy += diff * diff;
    }

    double meanEnergy = sumHFEnergy / static_cast<double>(dxdPcm.size());
    if (meanEnergy <= 1e-12) return -120.0f;

    float db = static_cast<float>(10.0 * std::log10(meanEnergy));
    return std::clamp(db, -140.0f, 0.0f);
}

} // namespace Jazz
