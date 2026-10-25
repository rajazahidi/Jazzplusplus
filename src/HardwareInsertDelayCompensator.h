//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZHardwareInsertDelayCompensator.h - Studio Hybrid Outboard Delay Alignment
//
// Hardware outboard gear delay compensation (Neve, SSL, Pultec analog inserts):
// Single-sample Dirac ping transmission, sub-sample fractional cross-correlation,
// polarity flip detection, and master multi-track delay alignment buffers.
//*****************************************************************************

#ifndef JAZZ_HARDWARE_INSERT_DELAY_COMPENSATOR_H
#define JAZZ_HARDWARE_INSERT_DELAY_COMPENSATOR_H

#include <vector>
#include <string>
#include <map>
#include <cstdint>

namespace Jazz {

struct HardwarePingResult {
    bool detected;
    double latencySamples;       // Total round-trip latency including fractional part
    double latencyMilliseconds;  // Round-trip latency in ms
    bool phaseInverted;          // True if outboard unit inverts audio phase
    float correlationPeak;       // Peak correlation value [-1.0, 1.0]
};

struct HardwareInsertChannel {
    int insertId;
    std::string hardwareName;
    double roundTripLatencySamples;
    bool phaseInverted;
    bool enabled;
    std::vector<float> delayBufferL;
    std::vector<float> delayBufferR;
    size_t writeIdx;
};

class JZHardwareInsertDelayCompensator {
public:
    JZHardwareInsertDelayCompensator(float sampleRate = 48000.0f);
    ~JZHardwareInsertDelayCompensator() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return m_sampleRate; }

    // Ping Generation: Generates calibrated reference buffer containing a Dirac pulse
    void generatePingSignal(std::vector<float>& outBuffer, size_t bufferLength = 2048, size_t pulseIndex = 64);

    // Ping Analysis: Cross-correlates recorded return signal with reference pulse
    bool analyzeReturnSignal(const float* returnSignal, size_t signalLength,
                             size_t pulseReferenceIndex, HardwarePingResult& outResult);

    // Hardware Insert Registration & Master Delay Calculation
    bool registerInsert(int insertId, const std::string& name, double latencySamples, bool phaseInverted = false);
    bool setInsertEnabled(int insertId, bool enabled);
    const HardwareInsertChannel* getInsert(int insertId) const;

    double calculateMaxDelayCompensationSamples() const;

    // Real-Time Processing: Delays track audio to align with maximum latency outboard insert
    void processAlignment(int insertId, const float* inL, const float* inR,
                          float* outL, float* outR, size_t numSamples);

    void reset();

private:
    float m_sampleRate;
    std::map<int, HardwareInsertChannel> m_inserts;
};

} // namespace Jazz

#endif // JAZZ_HARDWARE_INSERT_DELAY_COMPENSATOR_H
