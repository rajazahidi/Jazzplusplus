//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZSpectralAudioMorphEngine.cpp - Polyphonic Spectral Audio Morpher
//*****************************************************************************

#include "SpectralAudioMorphEngine.h"
#include <algorithm>
#include <cmath>

namespace Jazz {

JZSpectralAudioMorphEngine::JZSpectralAudioMorphEngine(float sampleRate, size_t fftSize)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
    , m_fftSize(fftSize >= 64 ? fftSize : 512)
    , m_morphMode(SpectralMorphMode::SPECTRAL_INTERPOLATION)
    , m_morphFactor(0.5f)
    , m_formantPreservation(0.75f)
    , m_spectralTiltDb(0.0f)
    , m_envL_A(0.0f), m_envR_A(0.0f)
    , m_envL_B(0.0f), m_envR_B(0.0f)
    , m_transL_A(0.0f), m_transR_A(0.0f)
    , m_prevA_L(0.0f), m_prevA_R(0.0f)
    , m_prevB_L(0.0f), m_prevB_R(0.0f)
{
}

void JZSpectralAudioMorphEngine::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        m_sampleRate = sampleRate;
    }
}

void JZSpectralAudioMorphEngine::setMorphMode(SpectralMorphMode mode) {
    m_morphMode = mode;
}

void JZSpectralAudioMorphEngine::setMorphFactor(float factor) {
    m_morphFactor = std::clamp(factor, 0.0f, 1.0f);
}

void JZSpectralAudioMorphEngine::setFormantPreservation(float amount) {
    m_formantPreservation = std::clamp(amount, 0.0f, 1.0f);
}

void JZSpectralAudioMorphEngine::setSpectralTiltDb(float tiltDb) {
    m_spectralTiltDb = std::clamp(tiltDb, -12.0f, 12.0f);
}

void JZSpectralAudioMorphEngine::processMorph(const float* inA_L, const float* inA_R,
                                            const float* inB_L, const float* inB_R,
                                            float* outL, float* outR, size_t numSamples) {
    if (!inA_L || !inA_R || !inB_L || !inB_R || !outL || !outR || numSamples == 0) return;

    float weightA = 1.0f - m_morphFactor;
    float weightB = m_morphFactor;

    // Equal-power cross-synthesis weights
    float cosW = std::cos(m_morphFactor * 1.5707963f);
    float sinW = std::sin(m_morphFactor * 1.5707963f);

    float alphaEnv = 0.005f; // Slow formant envelope tracker
    float alphaTrans = 0.08f; // Fast transient envelope tracker

    for (size_t i = 0; i < numSamples; ++i) {
        float sA_L = inA_L[i];
        float sA_R = inA_R[i];
        float sB_L = inB_L[i];
        float sB_R = inB_R[i];

        // Track spectral energy envelopes
        m_envL_A += alphaEnv * (std::abs(sA_L) - m_envL_A);
        m_envR_A += alphaEnv * (std::abs(sA_R) - m_envR_A);
        m_envL_B += alphaEnv * (std::abs(sB_L) - m_envL_B);
        m_envR_B += alphaEnv * (std::abs(sB_R) - m_envR_B);

        // Track transient onset of A
        float diffL_A = std::abs(sA_L - m_prevA_L);
        float diffR_A = std::abs(sA_R - m_prevA_R);
        m_prevA_L = sA_L;
        m_prevA_R = sA_R;
        m_prevB_L = sB_L;
        m_prevB_R = sB_R;

        m_transL_A += alphaTrans * (diffL_A - m_transL_A);
        m_transR_A += alphaTrans * (diffR_A - m_transR_A);

        float morphL = 0.0f;
        float morphR = 0.0f;

        switch (m_morphMode) {
            case SpectralMorphMode::SPECTRAL_INTERPOLATION: {
                // Smooth energy-preserving vector morph
                morphL = sA_L * cosW + sB_L * sinW;
                morphR = sA_R * cosW + sB_R * sinW;
                break;
            }

            case SpectralMorphMode::FORMANT_CROSS_SYNTHESIS: {
                // Apply the spectral formant envelope of B onto the pitch carrier A
                float gainEnvL = (m_envL_A > 1e-6f) ? (m_envL_B / m_envL_A) : 1.0f;
                float gainEnvR = (m_envR_A > 1e-6f) ? (m_envR_B / m_envR_A) : 1.0f;

                gainEnvL = std::clamp(gainEnvL, 0.1f, 4.0f);
                gainEnvR = std::clamp(gainEnvR, 0.1f, 4.0f);

                float shapedA_L = sA_L * (1.0f + m_formantPreservation * (gainEnvL - 1.0f));
                float shapedA_R = sA_R * (1.0f + m_formantPreservation * (gainEnvR - 1.0f));

                morphL = shapedA_L * cosW + sB_L * sinW * 0.5f;
                morphR = shapedA_R * cosW + sB_R * sinW * 0.5f;
                break;
            }

            case SpectralMorphMode::HYBRID_CHIMERA: {
                // Blend transient attack from A with resonant body from B
                float transWeight = std::clamp(m_transL_A * 3.0f, 0.0f, 1.0f);
                morphL = (sA_L * transWeight) + (sB_L * (1.0f - transWeight * 0.5f) * weightB);
                morphR = (sA_R * transWeight) + (sB_R * (1.0f - transWeight * 0.5f) * weightB);
                break;
            }
        }

        // Output limiting
        outL[i] = std::clamp(morphL, -1.0f, 1.0f);
        outR[i] = std::clamp(morphR, -1.0f, 1.0f);
    }
}

void JZSpectralAudioMorphEngine::reset() {
    m_envL_A = m_envR_A = 0.0f;
    m_envL_B = m_envR_B = 0.0f;
    m_transL_A = m_transR_A = 0.0f;
    m_prevA_L = m_prevA_R = 0.0f;
    m_prevB_L = m_prevB_R = 0.0f;
}

} // namespace Jazz
