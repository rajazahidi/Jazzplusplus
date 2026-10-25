//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// TransientRingingSuppressorEngine.h - Transient-Gated Modal Ringing
// & Resonant Overhang Damping Engine
//
// Dynamic drum shell and metallic percussion ringing suppressor (Oeksound Spiff /
// Eventide SplitEQ style):
// 100% transparent transient strike preservation (0-15ms attack window),
// spectral decay rate analysis, dynamic narrow-band resonant overhang tracking,
// adaptive 8-notch modal ring damping, and Ringing Delta Solo mode.
//*****************************************************************************

#ifndef JAZZ_TRANSIENT_RINGING_SUPPRESSOR_ENGINE_H
#define JAZZ_TRANSIENT_RINGING_SUPPRESSOR_ENGINE_H

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <array>
#include <algorithm>

namespace Jazz {

class JZTransientRingingSuppressorEngine {
public:
    static constexpr size_t NUM_RING_TRACKERS = 8;

    struct Parameters {
        float dampingAmountDb = -15.0f;       // Ringing attenuation depth (-3.0 to -30.0 dB)
        float sensitivity = 1.0f;             // Ringing detection sensitivity (0.2 to 3.0)
        float transientHoldMs = 12.0f;        // Attack transparency window (5.0 to 30.0 ms)
        float ringingDecayThresholdMs = 45.0f;// Sustained overhang trigger threshold (20.0 to 150.0 ms)
        float minFrequencyHz = 150.0f;        // Detection frequency range (80.0 to 12000.0 Hz)
        float maxFrequencyHz = 6000.0f;
        bool ringingSolo = false;             // Audition clamped ringing overtones only
        bool bypass = false;
    };

    JZTransientRingingSuppressorEngine(float sampleRate = 48000.0f);
    ~JZTransientRingingSuppressorEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setDampingAmountDb(float db);
    float getDampingAmountDb() const { return m_params.dampingAmountDb; }

    void setSensitivity(float sens);
    float getSensitivity() const { return m_params.sensitivity; }

    void setTransientHoldMs(float ms);
    float getTransientHoldMs() const { return m_params.transientHoldMs; }

    void setRingingSolo(bool solo);
    bool isRingingSolo() const { return m_params.ringingSolo; }

    void setFrequencyRange(float minHz, float maxHz);
    float getMinFrequencyHz() const { return m_params.minFrequencyHz; }
    float getMaxFrequencyHz() const { return m_params.maxFrequencyHz; }

    void setBypass(bool bp);
    bool isBypassed() const { return m_params.bypass; }

    /**
     * @brief Process stereo audio buffer through transient ringing suppressor.
     */
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

    // Telemetry
    bool isTransientActive() const { return m_transientActive; }
    float getDampingGainReductionDb() const { return m_currentDampingReductionDb; }
    float getDominantRingingFrequencyHz() const { return m_dominantRingingFreqHz; }

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    bool m_transientActive = false;
    float m_transientHoldCounter = 0.0f;
    float m_currentDampingReductionDb = 0.0f;
    float m_dominantRingingFreqHz = 450.0f;

    // Fast transient envelope & slow sustain envelope
    float m_fastEnvL = 0.0f, m_fastEnvR = 0.0f;
    float m_slowEnvL = 0.0f, m_slowEnvR = 0.0f;

    // 8 Adaptive modal notch filters across the target spectrum
    struct NotchFilter {
        float centerFreqHz = 500.0f;
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
        float x1L = 0.0f, x2L = 0.0f, y1L = 0.0f, y2L = 0.0f;
        float x1R = 0.0f, x2R = 0.0f, y1R = 0.0f, y2R = 0.0f;
        float energySustain = 0.0f;
        float currentGain = 1.0f;
    };
    std::array<NotchFilter, NUM_RING_TRACKERS> m_notches;

    void updateNotchFrequencies();
    void updateNotchCoeffs(size_t index, float gainLinear);
};

} // namespace Jazz

namespace jazz {
    using JZTransientRingingSuppressorEngine = Jazz::JZTransientRingingSuppressorEngine;
}

#endif // JAZZ_TRANSIENT_RINGING_SUPPRESSOR_ENGINE_H
