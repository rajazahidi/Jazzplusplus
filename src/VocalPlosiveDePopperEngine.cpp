//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// VocalPlosiveDePopperEngine.cpp - Vocal Plosive De-Popper
//*****************************************************************************

#include "VocalPlosiveDePopperEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

static constexpr float PI_F = 3.14159265358979323846f;

JZVocalPlosiveDePopperEngine::JZVocalPlosiveDePopperEngine(float sampleRate)
    : m_sampleRate(sampleRate)
{
    initialize(sampleRate);
}

void JZVocalPlosiveDePopperEngine::initialize(float sampleRate)
{
    m_sampleRate = (sampleRate > 8000.0f) ? sampleRate : 48000.0f;
    reset();
    updateFilters();

    // Fast 1.5ms attack, 45ms release
    m_attackCoeff = 1.0f - std::exp(-1000.0f / (1.5f * m_sampleRate));
    m_releaseCoeff = 1.0f - std::exp(-1000.0f / (45.0f * m_sampleRate));
}

void JZVocalPlosiveDePopperEngine::reset()
{
    m_lpX1L = m_lpX2L = m_lpY1L = m_lpY2L = 0.0f;
    m_lpX1R = m_lpX2R = m_lpY1R = m_lpY2R = 0.0f;
    m_hpX1L = m_hpX2L = m_hpY1L = m_hpY2L = 0.0f;
    m_hpX1R = m_hpX2R = m_hpY1R = m_hpY2R = 0.0f;
    m_envelope = 0.0f;
    m_currentReductionDb = 0.0f;
    m_plosiveCount = 0;
    m_wasPlosiveActive = false;
}

void JZVocalPlosiveDePopperEngine::setParameters(const Parameters& params)
{
    m_params = params;
    m_params.thresholdDb = std::max(-36.0f, std::min(m_params.thresholdDb, -6.0f));
    m_params.sensitivity = std::max(0.1f, std::min(m_params.sensitivity, 1.0f));
    m_params.maxReductionDb = std::max(-24.0f, std::min(m_params.maxReductionDb, -6.0f));
    m_params.plosiveCutoffHz = std::max(50.0f, std::min(m_params.plosiveCutoffHz, 120.0f));
    m_params.dryWet = std::max(0.0f, std::min(m_params.dryWet, 1.0f));

    updateFilters();
}

void JZVocalPlosiveDePopperEngine::setThresholdDb(float db)
{
    m_params.thresholdDb = std::max(-36.0f, std::min(db, -6.0f));
}

void JZVocalPlosiveDePopperEngine::setSensitivity(float sens)
{
    m_params.sensitivity = std::max(0.1f, std::min(sens, 1.0f));
}

void JZVocalPlosiveDePopperEngine::setMaxReductionDb(float db)
{
    m_params.maxReductionDb = std::max(-24.0f, std::min(db, -6.0f));
}

void JZVocalPlosiveDePopperEngine::setPlosiveCutoffHz(float hz)
{
    m_params.plosiveCutoffHz = std::max(50.0f, std::min(hz, 120.0f));
    updateFilters();
}

void JZVocalPlosiveDePopperEngine::setAuditionPlosiveDelta(bool audition)
{
    m_params.auditionPlosiveDelta = audition;
}

void JZVocalPlosiveDePopperEngine::setDryWet(float wet)
{
    m_params.dryWet = std::max(0.0f, std::min(wet, 1.0f));
}

void JZVocalPlosiveDePopperEngine::setBypass(bool bypass)
{
    m_params.bypassed = bypass;
}

void JZVocalPlosiveDePopperEngine::updateFilters()
{
    float fc = m_params.plosiveCutoffHz;
    float w0 = 2.0f * PI_F * fc / m_sampleRate;
    float cosW0 = std::cos(w0);
    float sinW0 = std::sin(w0);
    float q = 0.70710678f;
    float alpha = sinW0 / (2.0f * q);

    // 2nd-order Low-Pass Filter
    float b0 = (1.0f - cosW0) * 0.5f;
    float b1 = 1.0f - cosW0;
    float b2 = (1.0f - cosW0) * 0.5f;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cosW0;
    float a2 = 1.0f - alpha;

    m_lpB0 = b0 / a0;
    m_lpB1 = b1 / a0;
    m_lpB2 = b2 / a0;
    m_lpA1 = a1 / a0;
    m_lpA2 = a2 / a0;
}

void JZVocalPlosiveDePopperEngine::process(
    const float* inL, const float* inR,
    float* outL, float* outR,
    size_t numSamples)
{
    if (!inL || !inR || !outL || !outR || numSamples == 0) return;

    if (m_params.bypassed) {
        std::copy(inL, inL + numSamples, outL);
        std::copy(inR, inR + numSamples, outR);
        return;
    }

    float thresh = m_params.thresholdDb;
    float sens = m_params.sensitivity;
    float maxReduct = m_params.maxReductionDb;

    for (size_t i = 0; i < numSamples; ++i) {
        float xL = inL[i];
        float xR = inR[i];

        // 1. Isolate 20-85 Hz sub-bass wind & plosive energy
        float lpL = m_lpB0 * xL + m_lpB1 * m_lpX1L + m_lpB2 * m_lpX2L - m_lpA1 * m_lpY1L - m_lpA2 * m_lpY2L;
        m_lpX2L = m_lpX1L; m_lpX1L = xL;
        m_lpY2L = m_lpY1L; m_lpY1L = lpL;

        float lpR = m_lpB0 * xR + m_lpB1 * m_lpX1R + m_lpB2 * m_lpX2R - m_lpA1 * m_lpY1R - m_lpA2 * m_lpY2R;
        m_lpX2R = m_lpX1R; m_lpX1R = xR;
        m_lpY2R = m_lpY1R; m_lpY1R = lpR;

        // 2. Asymmetric pressure burst detection (microphone pop diaphragm displacement)
        float sidechainSignal = std::max(std::abs(lpL), std::abs(lpR));
        if (sidechainSignal > m_envelope) {
            m_envelope += m_attackCoeff * (sidechainSignal - m_envelope);
        } else {
            m_envelope += m_releaseCoeff * (sidechainSignal - m_envelope);
        }

        float envDb = 20.0f * std::log10(m_envelope + 1e-6f);
        float targetGr = 0.0f;

        if (envDb > thresh) {
            float over = (envDb - thresh) * sens;
            targetGr = -std::min(std::abs(maxReduct), over * 1.8f);
        }

        // Smooth gain reduction
        if (targetGr < m_currentReductionDb) {
            m_currentReductionDb += m_attackCoeff * (targetGr - m_currentReductionDb);
        } else {
            m_currentReductionDb += m_releaseCoeff * (targetGr - m_currentReductionDb);
        }

        // Track plosive occurrences
        bool isPop = (m_currentReductionDb < -2.0f);
        if (isPop && !m_wasPlosiveActive) {
            m_plosiveCount++;
        }
        m_wasPlosiveActive = isPop;

        // Linear reduction multiplier (1.0 = no reduction, ~0.125 = -18 dB)
        float grLin = std::pow(10.0f, m_currentReductionDb / 20.0f);
        float popSuppressionFactor = 1.0f - grLin; // Amount of plosive to remove

        // 3. Dynamic plosive extraction and subtraction
        float plosiveDeltaL = lpL * popSuppressionFactor;
        float plosiveDeltaR = lpR * popSuppressionFactor;

        // Solo plosive delta audition
        if (m_params.auditionPlosiveDelta) {
            outL[i] = plosiveDeltaL;
            outR[i] = plosiveDeltaR;
            continue;
        }

        float dePoppedL = xL - plosiveDeltaL;
        float dePoppedR = xR - plosiveDeltaR;

        // Dry/Wet blend
        outL[i] = xL * (1.0f - m_params.dryWet) + dePoppedL * m_params.dryWet;
        outR[i] = xR * (1.0f - m_params.dryWet) + dePoppedR * m_params.dryWet;
    }
}

} // namespace Jazz
