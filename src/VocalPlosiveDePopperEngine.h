//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// VocalPlosiveDePopperEngine.h - Vocal Plosive De-Popper & Wind Blast Suppressor
//
// Asymmetric air pressure displacement tracker and dynamic infrasonic attenuator
// (Cedar Studio / iZotope RX De-plosive style). Detects and surgically eliminates
// microphone 'P', 'B', and wind blast thumps (20-90 Hz) in real-time while
// strictly preserving fundamental vocal chest resonances and speech naturalness.
//*****************************************************************************

#ifndef JAZZ_VOCAL_PLOSIVE_DE_POPPER_ENGINE_H
#define JAZZ_VOCAL_PLOSIVE_DE_POPPER_ENGINE_H

#include <vector>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

class JZVocalPlosiveDePopperEngine {
public:
    struct Parameters {
        float thresholdDb = -18.0f;          // Plosive detection threshold (-36 to -6 dB)
        float sensitivity = 0.75f;           // Asymmetric pressure burst sensitivity (0.1 to 1.0)
        float maxReductionDb = -18.0f;       // Maximum plosive attenuation (-24 to -6 dB)
        float plosiveCutoffHz = 85.0f;       // Upper bound of plosive burst band (50 to 120 Hz)
        bool auditionPlosiveDelta = false;   // Listen exclusively to extracted plosive bursts
        float dryWet = 1.0f;
        bool bypassed = false;
    };

    JZVocalPlosiveDePopperEngine(float sampleRate = 48000.0f);
    ~JZVocalPlosiveDePopperEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setThresholdDb(float db);
    float getThresholdDb() const { return m_params.thresholdDb; }

    void setSensitivity(float sens);
    float getSensitivity() const { return m_params.sensitivity; }

    void setMaxReductionDb(float db);
    float getMaxReductionDb() const { return m_params.maxReductionDb; }

    void setPlosiveCutoffHz(float hz);
    float getPlosiveCutoffHz() const { return m_params.plosiveCutoffHz; }

    void setAuditionPlosiveDelta(bool audition);
    bool isAuditionPlosiveDelta() const { return m_params.auditionPlosiveDelta; }

    void setDryWet(float wet);
    float getDryWet() const { return m_params.dryWet; }

    void setBypass(bool bypass);
    bool isBypassed() const { return m_params.bypassed; }

    // Telemetry & Getters
    size_t getPlosivesDetectedCount() const { return m_plosiveCount; }
    float getCurrentReductionDb() const { return m_currentReductionDb; }
    bool isPlosiveActive() const { return m_currentReductionDb < -1.5f; }

    // Audio Processing
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

private:
    void updateFilters();

    float m_sampleRate = 48000.0f;
    Parameters m_params;

    // Infrasonic sidechain lowpass filter (detects 20-85 Hz wind/pop energy)
    float m_lpB0 = 0.0f, m_lpB1 = 0.0f, m_lpB2 = 0.0f, m_lpA1 = 0.0f, m_lpA2 = 0.0f;
    float m_lpX1L = 0.0f, m_lpX2L = 0.0f, m_lpY1L = 0.0f, m_lpY2L = 0.0f;
    float m_lpX1R = 0.0f, m_lpX2R = 0.0f, m_lpY1R = 0.0f, m_lpY2R = 0.0f;

    // Dynamic High-Pass Filter biquad for plosive suppression
    float m_hpB0 = 1.0f, m_hpB1 = 0.0f, m_hpB2 = 0.0f, m_hpA1 = 0.0f, m_hpA2 = 0.0f;
    float m_hpX1L = 0.0f, m_hpX2L = 0.0f, m_hpY1L = 0.0f, m_hpY2L = 0.0f;
    float m_hpX1R = 0.0f, m_hpX2R = 0.0f, m_hpY1R = 0.0f, m_hpY2R = 0.0f;

    float m_envelope = 0.0f;
    float m_attackCoeff = 0.0f;
    float m_releaseCoeff = 0.0f;

    float m_currentReductionDb = 0.0f;
    size_t m_plosiveCount = 0;
    bool m_wasPlosiveActive = false;
};

} // namespace Jazz

namespace jazz {
    using JZVocalPlosiveDePopperEngine = Jazz::JZVocalPlosiveDePopperEngine;
}

#endif // JAZZ_VOCAL_PLOSIVE_DE_POPPER_ENGINE_H
