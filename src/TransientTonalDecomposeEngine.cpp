//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// TransientTonalDecomposeEngine.cpp - Real-Time Transient-Tonal-Ambience Decomposer
//*****************************************************************************

#include "TransientTonalDecomposeEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

JZTransientTonalDecomposeEngine::JZTransientTonalDecomposeEngine(float sampleRate)
    : m_sampleRate(sampleRate) {
    if (m_sampleRate < 8000.0f) m_sampleRate = 48000.0f;
    reset();
    updateCoefficients();
}

void JZTransientTonalDecomposeEngine::setSampleRate(float sampleRate) {
    if (sampleRate < 8000.0f) sampleRate = 48000.0f;
    m_sampleRate = sampleRate;
    reset();
    updateCoefficients();
}

void JZTransientTonalDecomposeEngine::reset() {
    m_fastEnvL = 0.0f;
    m_fastEnvR = 0.0f;
    m_slowEnvL = 0.0f;
    m_slowEnvR = 0.0f;
    m_ambienceEnvL = 0.0f;
    m_ambienceEnvR = 0.0f;

    m_transientEnergy = 0.0f;
    m_tonalEnergy = 0.0f;
    m_ambienceEnergy = 0.0f;
}

void JZTransientTonalDecomposeEngine::setParameters(const Parameters& params) {
    m_params = params;
    m_params.transientSensitivity = std::max(0.0f, std::min(1.0f, m_params.transientSensitivity));
    m_params.smoothingMs = std::max(5.0f, std::min(200.0f, m_params.smoothingMs));
    m_params.gainTransient = std::max(0.0f, std::min(4.0f, m_params.gainTransient));
    m_params.gainTonal = std::max(0.0f, std::min(4.0f, m_params.gainTonal));
    m_params.gainAmbience = std::max(0.0f, std::min(4.0f, m_params.gainAmbience));
    updateCoefficients();
}

void JZTransientTonalDecomposeEngine::setTransientSensitivity(float sens) {
    m_params.transientSensitivity = std::max(0.0f, std::min(1.0f, sens));
}

void JZTransientTonalDecomposeEngine::setTonalSmoothingMs(float ms) {
    m_params.smoothingMs = std::max(5.0f, std::min(200.0f, ms));
    updateCoefficients();
}

void JZTransientTonalDecomposeEngine::setComponentGains(float transientGain, float tonalGain, float ambienceGain) {
    m_params.gainTransient = std::max(0.0f, std::min(4.0f, transientGain));
    m_params.gainTonal = std::max(0.0f, std::min(4.0f, tonalGain));
    m_params.gainAmbience = std::max(0.0f, std::min(4.0f, ambienceGain));
}

void JZTransientTonalDecomposeEngine::updateCoefficients() {
    // Fast attack (1.0 ms) and fast release (12.0 ms)
    m_fastAttackCoeff = std::exp(-1.0f / (0.001f * m_sampleRate));
    m_fastReleaseCoeff = std::exp(-1.0f / (0.012f * m_sampleRate));

    // Slow attack (smoothingMs) and slow release (smoothingMs * 3.5)
    float slowAtkSec = m_params.smoothingMs / 1000.0f;
    float slowRelSec = slowAtkSec * 3.5f;
    m_slowAttackCoeff = std::exp(-1.0f / (slowAtkSec * m_sampleRate));
    m_slowReleaseCoeff = std::exp(-1.0f / (slowRelSec * m_sampleRate));
}

void JZTransientTonalDecomposeEngine::processDecomposed(
    const float* inL, const float* inR,
    float* transL, float* transR,
    float* tonalL, float* tonalR,
    float* ambL, float* ambR,
    size_t numSamples) {

    if (!inL || numSamples == 0) return;
    const bool stereo = (inR != nullptr);

    float sumTransSq = 0.0f;
    float sumTonalSq = 0.0f;
    float sumAmbSq = 0.0f;

    for (size_t i = 0; i < numSamples; ++i) {
        float sampleL = inL[i];
        float sampleR = stereo ? inR[i] : sampleL;
        float absL = std::abs(sampleL);
        float absR = std::abs(sampleR);

        // Update Left Envelope
        if (absL > m_fastEnvL) {
            m_fastEnvL = m_fastAttackCoeff * m_fastEnvL + (1.0f - m_fastAttackCoeff) * absL;
        } else {
            m_fastEnvL = m_fastReleaseCoeff * m_fastEnvL + (1.0f - m_fastReleaseCoeff) * absL;
        }

        if (absL > m_slowEnvL) {
            m_slowEnvL = m_slowAttackCoeff * m_slowEnvL + (1.0f - m_slowAttackCoeff) * absL;
        } else {
            m_slowEnvL = m_slowReleaseCoeff * m_slowEnvL + (1.0f - m_slowReleaseCoeff) * absL;
        }

        // Ambient floor tracking
        m_ambienceEnvL = m_ambienceEnvL * 0.999f + absL * 0.001f;

        // Calculate Transient / Tonal / Ambience weights for Left
        float diffL = std::max(0.0f, m_fastEnvL - m_slowEnvL);
        float wTransL = std::tanh(diffL * (m_params.transientSensitivity * 6.0f));
        wTransL = std::max(0.0f, std::min(1.0f, wTransL));

        float remL = 1.0f - wTransL;
        float tonalRatioL = (m_slowEnvL > 1e-5f) ? (m_slowEnvL / (m_slowEnvL + m_ambienceEnvL + 1e-5f)) : 0.5f;
        float wTonalL = remL * tonalRatioL;
        float wAmbL = std::max(0.0f, 1.0f - wTransL - wTonalL);

        float tL = sampleL * wTransL;
        float tonL = sampleL * wTonalL;
        float aL = sampleL * wAmbL;

        if (transL) transL[i] = tL;
        if (tonalL) tonalL[i] = tonL;
        if (ambL) ambL[i] = aL;

        // Update Right channel
        float tR = tL, tonR = tonL, aR = aL;
        if (stereo) {
            if (absR > m_fastEnvR) {
                m_fastEnvR = m_fastAttackCoeff * m_fastEnvR + (1.0f - m_fastAttackCoeff) * absR;
            } else {
                m_fastEnvR = m_fastReleaseCoeff * m_fastEnvR + (1.0f - m_fastReleaseCoeff) * absR;
            }

            if (absR > m_slowEnvR) {
                m_slowEnvR = m_slowAttackCoeff * m_slowEnvR + (1.0f - m_slowAttackCoeff) * absR;
            } else {
                m_slowEnvR = m_slowReleaseCoeff * m_slowEnvR + (1.0f - m_slowReleaseCoeff) * absR;
            }

            m_ambienceEnvR = m_ambienceEnvR * 0.999f + absR * 0.001f;

            float diffR = std::max(0.0f, m_fastEnvR - m_slowEnvR);
            float wTransR = std::tanh(diffR * (m_params.transientSensitivity * 6.0f));
            wTransR = std::max(0.0f, std::min(1.0f, wTransR));

            float remR = 1.0f - wTransR;
            float tonalRatioR = (m_slowEnvR > 1e-5f) ? (m_slowEnvR / (m_slowEnvR + m_ambienceEnvR + 1e-5f)) : 0.5f;
            float wTonalR = remR * tonalRatioR;
            float wAmbR = std::max(0.0f, 1.0f - wTransR - wTonalR);

            tR = sampleR * wTransR;
            tonR = sampleR * wTonalR;
            aR = sampleR * wAmbR;

            if (transR) transR[i] = tR;
            if (tonalR) tonalR[i] = tonR;
            if (ambR) ambR[i] = aR;
        }

        sumTransSq += (tL * tL + tR * tR) * 0.5f;
        sumTonalSq += (tonL * tonL + tonR * tonR) * 0.5f;
        sumAmbSq += (aL * aL + aR * aR) * 0.5f;
    }

    m_transientEnergy = std::sqrt(sumTransSq / static_cast<float>(numSamples));
    m_tonalEnergy = std::sqrt(sumTonalSq / static_cast<float>(numSamples));
    m_ambienceEnergy = std::sqrt(sumAmbSq / static_cast<float>(numSamples));
}

void JZTransientTonalDecomposeEngine::process(
    const float* inL, const float* inR,
    float* outL, float* outR,
    size_t numSamples) {

    if (!inL || !outL || numSamples == 0) return;
    const bool stereo = (inR != nullptr && outR != nullptr);

    std::vector<float> tL(numSamples), tR(numSamples);
    std::vector<float> tonL(numSamples), tonR(numSamples);
    std::vector<float> aL(numSamples), aR(numSamples);

    processDecomposed(inL, inR,
                      tL.data(), stereo ? tR.data() : nullptr,
                      tonL.data(), stereo ? tonR.data() : nullptr,
                      aL.data(), stereo ? aR.data() : nullptr,
                      numSamples);

    for (size_t i = 0; i < numSamples; ++i) {
        if (m_params.soloTransient) {
            outL[i] = tL[i] * m_params.gainTransient;
            if (stereo) outR[i] = tR[i] * m_params.gainTransient;
        } else if (m_params.soloTonal) {
            outL[i] = tonL[i] * m_params.gainTonal;
            if (stereo) outR[i] = tonR[i] * m_params.gainTonal;
        } else if (m_params.soloAmbience) {
            outL[i] = aL[i] * m_params.gainAmbience;
            if (stereo) outR[i] = aR[i] * m_params.gainAmbience;
        } else {
            outL[i] = tL[i] * m_params.gainTransient +
                      tonL[i] * m_params.gainTonal +
                      aL[i] * m_params.gainAmbience;
            if (stereo) {
                outR[i] = tR[i] * m_params.gainTransient +
                          tonR[i] * m_params.gainTonal +
                          aR[i] * m_params.gainAmbience;
            }
        }
    }
}

} // namespace Jazz
