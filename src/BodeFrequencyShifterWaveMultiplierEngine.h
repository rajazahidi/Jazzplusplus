//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// BodeFrequencyShifterWaveMultiplierEngine.h - Bode Shifter & Serge Multiplier
//
// Analog modular experimental sound design (Bode 1630 / Serge Modular style):
// True linear frequency shifting (+/- 5000 Hz) via Hilbert 90-deg quadrature
// phase network combined with Serge non-linear diode wave folding multiplication.
//*****************************************************************************

#ifndef JAZZ_BODE_FREQUENCY_SHIFTER_WAVE_MULTIPLIER_ENGINE_H
#define JAZZ_BODE_FREQUENCY_SHIFTER_WAVE_MULTIPLIER_ENGINE_H

#include <vector>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

enum class ShiftSidebandMode {
    UPPER_SIDEBAND = 0, // Inharmonic upward shift (+delta f)
    LOWER_SIDEBAND = 1, // Inharmonic downward shift (-delta f)
    STEREO_SPLIT   = 2  // Left upper (+df), Right lower (-df)
};

class JZBodeFrequencyShifterWaveMultiplierEngine {
public:
    struct Parameters {
        float shiftHz = 35.0f;               // Linear Hertz offset (-5000.0 to +5000.0 Hz)
        ShiftSidebandMode mode = ShiftSidebandMode::UPPER_SIDEBAND;
        float waveFoldAmount = 1.0f;         // Serge wave multiplier drive (1.0 = dry, 5.0 = folded)
        float feedback = 0.0f;               // Shifter loop feedback (-0.85 to +0.85)
        float dryWet = 1.0f;                 // Wet / dry blend
    };

    JZBodeFrequencyShifterWaveMultiplierEngine(float sampleRate = 48000.0f);
    ~JZBodeFrequencyShifterWaveMultiplierEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setFrequencyShiftHz(float hz);
    float getFrequencyShiftHz() const { return m_params.shiftHz; }

    void setWaveFoldAmount(float amount);
    float getWaveFoldAmount() const { return m_params.waveFoldAmount; }

    void setSidebandMode(ShiftSidebandMode mode);
    ShiftSidebandMode getSidebandMode() const { return m_params.mode; }

    /**
     * @brief Process audio through Bode frequency shifter and Serge wave multiplier.
     */
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

    float getLfoPhase() const { return m_lfoPhase; }

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    // Hilbert Transform 90-degree phase splitter (I and Q all-pass cascades)
    // 4-stage Weaver/all-pass network
    float m_apI1_L = 0, m_apI2_L = 0, m_apI3_L = 0, m_apI4_L = 0;
    float m_apQ1_L = 0, m_apQ2_L = 0, m_apQ3_L = 0, m_apQ4_L = 0;
    float m_apI1_R = 0, m_apI2_R = 0, m_apI3_R = 0, m_apI4_R = 0;
    float m_apQ1_R = 0, m_apQ2_R = 0, m_apQ3_R = 0, m_apQ4_R = 0;

    float m_lfoPhase = 0.0f;
    float m_feedbackStateL = 0.0f;
    float m_feedbackStateR = 0.0f;

    float applySergeWaveFold(float x, float foldAmount);
    void hilbertSplit(float in, float& outI, float& outQ,
                      float& i1, float& i2, float& i3, float& i4,
                      float& q1, float& q2, float& q3, float& q4);
};

} // namespace Jazz

namespace jazz {
    using JZBodeFrequencyShifterWaveMultiplierEngine = Jazz::JZBodeFrequencyShifterWaveMultiplierEngine;
}

#endif // JAZZ_BODE_FREQUENCY_SHIFTER_WAVE_MULTIPLIER_ENGINE_H
