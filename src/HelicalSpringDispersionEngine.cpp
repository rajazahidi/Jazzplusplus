//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// HelicalSpringDispersionEngine.cpp - Physical Helical Spring Reverb Modeler
//*****************************************************************************

#include "HelicalSpringDispersionEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

JZHelicalSpringDispersionEngine::JZHelicalSpringDispersionEngine(float sampleRate) {
    initialize(sampleRate);
}

void JZHelicalSpringDispersionEngine::initialize(float sampleRate) {
    if (sampleRate < 8000.0f) sampleRate = 48000.0f;
    m_sampleRate = sampleRate;

    // Relative prime delay lengths for 3 physical spring lines (approx 33ms, 41ms, 47ms)
    float baseTimes[3] = { 0.0331f, 0.0413f, 0.0479f };
    for (int s = 0; s < 3; ++s) {
        size_t len = static_cast<size_t>(baseTimes[s] * m_sampleRate);
        if (len < 64) len = 64;
        m_springs[s].length = len;
        m_springs[s].delay.assign(len + 32, 0.0f);
    }

    reset();
    updateSpringParameters();
}

void JZHelicalSpringDispersionEngine::reset() {
    for (int s = 0; s < 3; ++s) {
        std::fill(m_springs[s].delay.begin(), m_springs[s].delay.end(), 0.0f);
        m_springs[s].writePos = 0;
        m_springs[s].dampState = 0.0f;
        for (size_t k = 0; k < SpringLine::NUM_DISP_STAGES; ++k) {
            m_springs[s].apX[k] = 0.0f;
            m_springs[s].apY[k] = 0.0f;
        }
    }
    m_tankKickEnergy = 0.0f;
    m_tankKickPhase = 0.0f;
    m_springEnergy = 0.0f;
}

void JZHelicalSpringDispersionEngine::setParameters(const Parameters& params) {
    m_params = params;
    m_params.decayTimeSec = std::max(0.5f, std::min(10.0f, m_params.decayTimeSec));
    m_params.dispersion = std::max(0.0f, std::min(1.0f, m_params.dispersion));
    m_params.dampingDampFreqHz = std::max(500.0f, std::min(12000.0f, m_params.dampingDampFreqHz));
    m_params.drive = std::max(0.5f, std::min(4.0f, m_params.drive));
    m_params.dryWet = std::max(0.0f, std::min(1.0f, m_params.dryWet));
    m_params.stereoSpread = std::max(0.0f, std::min(1.0f, m_params.stereoSpread));
    updateSpringParameters();
}

void JZHelicalSpringDispersionEngine::setDecayTimeSec(float sec) {
    m_params.decayTimeSec = std::max(0.5f, std::min(10.0f, sec));
    updateSpringParameters();
}

void JZHelicalSpringDispersionEngine::setDispersion(float dispersion) {
    m_params.dispersion = std::max(0.0f, std::min(1.0f, dispersion));
}

void JZHelicalSpringDispersionEngine::setDryWet(float mix) {
    m_params.dryWet = std::max(0.0f, std::min(1.0f, mix));
}

void JZHelicalSpringDispersionEngine::updateSpringParameters() {
    float rt60 = m_params.decayTimeSec;
    for (int s = 0; s < 3; ++s) {
        float delaySec = static_cast<float>(m_springs[s].length) / m_sampleRate;
        // -60 dB attenuation over rt60: feedback = 10^(-3 * delaySec / rt60)
        m_springs[s].feedback = std::pow(10.0f, (-3.0f * delaySec) / rt60);
        if (m_springs[s].feedback > 0.985f) m_springs[s].feedback = 0.985f;
    }
}

void JZHelicalSpringDispersionEngine::triggerTankKickImpact(float intensity) {
    m_tankKickEnergy = std::max(0.0f, std::min(2.0f, intensity));
    m_tankKickPhase = 0.0f;
}

void JZHelicalSpringDispersionEngine::process(
    const float* inL, const float* inR,
    float* outL, float* outR,
    size_t numSamples) {

    if (!inL || !outL || numSamples == 0) return;
    const bool stereo = (inR != nullptr && outR != nullptr);

    // 1st order Allpass dispersion coefficient: a = 0.65 * dispersion
    float apA = 0.65f * m_params.dispersion;

    // High frequency damping coefficient
    float wDamp = 2.0f * static_cast<float>(M_PI) * (m_params.dampingDampFreqHz / m_sampleRate);
    float dampCoeff = std::max(0.1f, std::min(0.9f, std::exp(-wDamp)));

    float totalSpringEnergySq = 0.0f;

    for (size_t i = 0; i < numSamples; ++i) {
        float monoIn = (inL[i] + (stereo ? inR[i] : inL[i])) * 0.5f;

        // Transducer coil non-linear magnetic saturation
        float drivenIn = std::tanh(monoIn * m_params.drive);

        // Add tank kick mechanical shock impact if active
        if (m_tankKickEnergy > 1e-4f) {
            float kickWave = std::sin(m_tankKickPhase) + 0.4f * std::sin(m_tankKickPhase * 2.85f);
            drivenIn += kickWave * m_tankKickEnergy * 0.8f;
            m_tankKickPhase += 2.0f * static_cast<float>(M_PI) * (75.0f / m_sampleRate); // 75 Hz physical resonant clatter
            m_tankKickEnergy *= 0.9992f; // Exponential mechanical decay
        }

        float springOut[3] = { 0.0f, 0.0f, 0.0f };

        // Process 3 physical helical spring transmission lines
        for (int s = 0; s < 3; ++s) {
            SpringLine& sp = m_springs[s];

            // Read from delay line
            float delayed = sp.delay[sp.writePos];

            // Pass through Dispersive All-Pass Ladder (creates chirp frequency dispersion)
            float apIn = delayed;
            for (size_t k = 0; k < SpringLine::NUM_DISP_STAGES; ++k) {
                float apOut = -apA * apIn + sp.apX[k] + apA * sp.apY[k];
                sp.apX[k] = apIn;
                sp.apY[k] = apOut;
                apIn = apOut;
            }
            float dispersed = apIn;

            // Apply high-frequency wire damping (lossy transmission)
            sp.dampState = (1.0f - dampCoeff) * dispersed + dampCoeff * sp.dampState;
            float damped = sp.dampState;

            // Feedback injection into spring coil
            sp.delay[sp.writePos] = drivenIn * 0.4f + damped * sp.feedback;
            sp.writePos = (sp.writePos + 1) % sp.length;

            springOut[s] = dispersed;
            totalSpringEnergySq += dispersed * dispersed;
        }

        // Stereo matrix decoding with physical decoupling
        float wetL = springOut[0] - 0.5f * springOut[2] * m_params.stereoSpread;
        float wetR = springOut[1] + 0.5f * springOut[2] * m_params.stereoSpread;

        float dryWeight = 1.0f - m_params.dryWet;
        float wetWeight = m_params.dryWet;

        outL[i] = inL[i] * dryWeight + wetL * wetWeight;
        if (stereo) {
            outR[i] = inR[i] * dryWeight + wetR * wetWeight;
        }
    }

    m_springEnergy = std::sqrt(totalSpringEnergySq / static_cast<float>(numSamples * 3));
}

} // namespace Jazz
