//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// AnalogBbdChorusDimensionEngine.h - Solid-State BBD Chorus & Dimension Matrix
//
// Discrete Bucket Brigade Device (MN3005 / MN3007) physical simulation:
// Charge-transfer inefficiency (CTI), clock-dependent reconstruction filter droop,
// and 4-mode spatial stereo dimension matrix (Roland Dimension D style).
//*****************************************************************************

#ifndef JAZZ_ANALOG_BBD_CHORUS_DIMENSION_ENGINE_H
#define JAZZ_ANALOG_BBD_CHORUS_DIMENSION_ENGINE_H

#include <vector>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

enum class BbdDimensionMode {
    MODE_1 = 1, // Subtle spatial spread (0.25 Hz, low depth)
    MODE_2 = 2, // Medium lush dimension (0.45 Hz, moderate depth)
    MODE_3 = 3, // Rich analog chorus (0.65 Hz, high depth)
    MODE_4 = 4  // Extreme stereo ensemble with cross-phase inversion
};

class JZAnalogBbdChorusDimensionEngine {
public:
    struct Parameters {
        BbdDimensionMode mode = BbdDimensionMode::MODE_2;
        int numStages = 1024;                // 512, 1024, or 2048 BBD stages
        float ctiLossFactor = 0.0012f;       // Charge transfer inefficiency (0.0001 to 0.005)
        float clockBleedDb = -54.0f;         // Ultrasonic clock feedthrough (-70 to -40 dB)
        float dryWet = 0.5f;                 // Wet / dry blend
    };

    JZAnalogBbdChorusDimensionEngine(float sampleRate = 48000.0f);
    ~JZAnalogBbdChorusDimensionEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setDimensionMode(BbdDimensionMode mode);
    BbdDimensionMode getDimensionMode() const { return m_params.mode; }

    void setNumStages(int stages);
    int getNumStages() const { return m_params.numStages; }

    void setCtiLossFactor(float cti);
    float getCtiLossFactor() const { return m_params.ctiLossFactor; }

    /**
     * @brief Process stereo audio buffer through analog BBD chorus engine.
     */
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

    float getCurrentClockRateHz() const { return m_currentClockHz; }
    float getDelayTimeMsL() const { return m_delayMsL; }
    float getDelayTimeMsR() const { return m_delayMsR; }

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    // BBD delay lines (circular buffers)
    static const size_t BBD_BUF_SIZE = 4096;
    std::vector<float> m_bbdBufferL;
    std::vector<float> m_bbdBufferR;
    size_t m_writePos = 0;

    // Dual anti-phase LFO states
    float m_lfoPhase = 0.0f;
    float m_currentClockHz = 50000.0f;
    float m_delayMsL = 4.5f;
    float m_delayMsR = 4.5f;

    // Anti-aliasing / reconstruction low-pass filters (2-pole Sallen-Key)
    float m_lpB0 = 0.1f, m_lpB1 = 0.2f, m_lpB2 = 0.1f, m_lpA1 = 0.0f, m_lpA2 = 0.0f;
    float m_lpX1L = 0.0f, m_lpX2L = 0.0f, m_lpY1L = 0.0f, m_lpY2L = 0.0f;
    float m_lpX1R = 0.0f, m_lpX2R = 0.0f, m_lpY1R = 0.0f, m_lpY2R = 0.0f;

    void updateReconstructionFilter(float clockHz);
};

} // namespace Jazz

namespace jazz {
    using JZAnalogBbdChorusDimensionEngine = Jazz::JZAnalogBbdChorusDimensionEngine;
    using BbdDimensionMode = Jazz::BbdDimensionMode;
}

#endif // JAZZ_ANALOG_BBD_CHORUS_DIMENSION_ENGINE_H
