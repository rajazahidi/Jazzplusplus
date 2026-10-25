//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// TransientTonalDecomposeEngine.h - Real-Time Transient-Tonal-Ambience Decomposer
//
// Real-time 3-way spectral decomposition: Fast attack transients, sustained
// harmonic tonal body, and diffuse ambient noise bed (Cedar/SpectraLayers style).
// Preserves perfect reconstruction phase coherence while providing surgical gains.
//*****************************************************************************

#ifndef JAZZ_TRANSIENT_TONAL_DECOMPOSE_ENGINE_H
#define JAZZ_TRANSIENT_TONAL_DECOMPOSE_ENGINE_H

#include <vector>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

class JZTransientTonalDecomposeEngine {
public:
    struct Parameters {
        float transientSensitivity = 0.6f;  // 0.0 to 1.0
        float smoothingMs = 25.0f;          // Tonal integration smoothing time
        float gainTransient = 1.0f;         // 0.0 to 2.0 (linear gain)
        float gainTonal = 1.0f;             // 0.0 to 2.0 (linear gain)
        float gainAmbience = 1.0f;          // 0.0 to 2.0 (linear gain)
        bool soloTransient = false;
        bool soloTonal = false;
        bool soloAmbience = false;
    };

    JZTransientTonalDecomposeEngine(float sampleRate = 48000.0f);
    ~JZTransientTonalDecomposeEngine() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return m_sampleRate; }

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setTransientSensitivity(float sens);
    float getTransientSensitivity() const { return m_params.transientSensitivity; }

    void setTonalSmoothingMs(float ms);
    float getTonalSmoothingMs() const { return m_params.smoothingMs; }

    void setComponentGains(float transientGain, float tonalGain, float ambienceGain);

    /**
     * @brief Process stereo audio and output recombined audio with adjusted component gains.
     */
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

    /**
     * @brief Process and extract individual decomposed streams (Transient, Tonal, Ambience).
     */
    void processDecomposed(const float* inL, const float* inR,
                           float* transL, float* transR,
                           float* tonalL, float* tonalR,
                           float* ambL, float* ambR,
                           size_t numSamples);

    float getTransientEnergy() const { return m_transientEnergy; }
    float getTonalEnergy() const { return m_tonalEnergy; }
    float getAmbienceEnergy() const { return m_ambienceEnergy; }

    void reset();

private:
    float m_sampleRate;
    Parameters m_params;

    // Dual-envelope attack/decay tracking
    float m_fastEnvL = 0.0f;
    float m_fastEnvR = 0.0f;
    float m_slowEnvL = 0.0f;
    float m_slowEnvR = 0.0f;
    float m_ambienceEnvL = 0.0f;
    float m_ambienceEnvR = 0.0f;

    float m_fastAttackCoeff = 0.0f;
    float m_fastReleaseCoeff = 0.0f;
    float m_slowAttackCoeff = 0.0f;
    float m_slowReleaseCoeff = 0.0f;

    // Telemetry energy meters
    float m_transientEnergy = 0.0f;
    float m_tonalEnergy = 0.0f;
    float m_ambienceEnergy = 0.0f;

    void updateCoefficients();
};

} // namespace Jazz

namespace jazz {
    using JZTransientTonalDecomposeEngine = Jazz::JZTransientTonalDecomposeEngine;
}

#endif // JAZZ_TRANSIENT_TONAL_DECOMPOSE_ENGINE_H
