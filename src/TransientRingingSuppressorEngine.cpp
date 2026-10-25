//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// TransientRingingSuppressorEngine.cpp - Transient-Gated Modal Ringing
// & Resonant Overhang Damping Engine
//*****************************************************************************

#include "TransientRingingSuppressorEngine.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Jazz {

JZTransientRingingSuppressorEngine::JZTransientRingingSuppressorEngine(float sampleRate)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
{
    initialize(m_sampleRate);
}

void JZTransientRingingSuppressorEngine::initialize(float sampleRate)
{
    m_sampleRate = sampleRate > 1000.0f ? sampleRate : 48000.0f;
    updateNotchFrequencies();
    reset();
}

void JZTransientRingingSuppressorEngine::reset()
{
    m_transientActive = false;
    m_transientHoldCounter = 0.0f;
    m_currentDampingReductionDb = 0.0f;
    m_dominantRingingFreqHz = 450.0f;

    m_fastEnvL = m_fastEnvR = 0.0f;
    m_slowEnvL = m_slowEnvR = 0.0f;

    for (size_t k = 0; k < NUM_RING_TRACKERS; ++k) {
        auto& n = m_notches[k];
        n.x1L = n.x2L = n.y1L = n.y2L = 0.0f;
        n.x1R = n.x2R = n.y1R = n.y2R = 0.0f;
        n.energySustain = 0.0f;
        n.currentGain = 1.0f;
        updateNotchCoeffs(k, 1.0f);
    }
}

void JZTransientRingingSuppressorEngine::updateNotchFrequencies()
{
    // Distribute 8 notch frequencies exponentially across minFrequencyHz..maxFrequencyHz
    float minF = std::max(40.0f, m_params.minFrequencyHz);
    float maxF = std::min(m_sampleRate * 0.45f, m_params.maxFrequencyHz);

    for (size_t k = 0; k < NUM_RING_TRACKERS; ++k) {
        float ratio = static_cast<float>(k) / static_cast<float>(NUM_RING_TRACKERS - 1);
        m_notches[k].centerFreqHz = minF * std::pow(maxF / minF, ratio);
        updateNotchCoeffs(k, 1.0f);
    }
}

void JZTransientRingingSuppressorEngine::setParameters(const Parameters& params)
{
    m_params = params;
    m_params.dampingAmountDb = std::max(-36.0f, std::min(m_params.dampingAmountDb, 0.0f));
    m_params.sensitivity = std::max(0.1f, std::min(m_params.sensitivity, 5.0f));
    m_params.transientHoldMs = std::max(2.0f, std::min(m_params.transientHoldMs, 50.0f));
    updateNotchFrequencies();
}

void JZTransientRingingSuppressorEngine::setDampingAmountDb(float db)
{
    m_params.dampingAmountDb = std::max(-36.0f, std::min(db, 0.0f));
}

void JZTransientRingingSuppressorEngine::setSensitivity(float sens)
{
    m_params.sensitivity = std::max(0.1f, std::min(sens, 5.0f));
}

void JZTransientRingingSuppressorEngine::setTransientHoldMs(float ms)
{
    m_params.transientHoldMs = std::max(2.0f, std::min(ms, 50.0f));
}

void JZTransientRingingSuppressorEngine::setRingingSolo(bool solo)
{
    m_params.ringingSolo = solo;
}

void JZTransientRingingSuppressorEngine::setFrequencyRange(float minHz, float maxHz)
{
    m_params.minFrequencyHz = std::max(40.0f, minHz);
    m_params.maxFrequencyHz = std::max(m_params.minFrequencyHz + 100.0f, maxHz);
    updateNotchFrequencies();
}

void JZTransientRingingSuppressorEngine::setBypass(bool bp)
{
    m_params.bypass = bp;
}

void JZTransientRingingSuppressorEngine::updateNotchCoeffs(size_t index, float gainLinear)
{
    auto& n = m_notches[index];
    float w0 = 2.0f * static_cast<float>(M_PI) * n.centerFreqHz / m_sampleRate;
    float q = 8.0f; // High Q surgical notch
    float cosw0 = std::cos(w0);
    float sinw0 = std::sin(w0);
    float alpha = sinw0 / (2.0f * q);

    // Peaking EQ dip formula
    float A = std::max(0.01f, gainLinear);
    float a0 = 1.0f + alpha / A;
    n.b0 = (1.0f + alpha * A) / a0;
    n.b1 = (-2.0f * cosw0) / a0;
    n.b2 = (1.0f - alpha * A) / a0;
    n.a1 = (-2.0f * cosw0) / a0;
    n.a2 = (1.0f - alpha / A) / a0;
    n.currentGain = gainLinear;
}

void JZTransientRingingSuppressorEngine::process(const float* inL, const float* inR,
                                               float* outL, float* outR,
                                               size_t numSamples)
{
    if (!inL || !inR || !outL || !outR || numSamples == 0) return;

    if (m_params.bypass) {
        if (outL != inL) std::copy(inL, inL + numSamples, outL);
        if (outR != inR) std::copy(inR, inR + numSamples, outR);
        return;
    }

    float fastAlpha = std::exp(-1.0f / (m_sampleRate * 0.001f));  // 1ms fast detector
    float slowAlpha = std::exp(-1.0f / (m_sampleRate * 0.025f));  // 25ms slow envelope
    float targetDepthLinear = std::pow(10.0f, m_params.dampingAmountDb / 20.0f);
    float holdDec = 1.0f / (m_sampleRate * (m_params.transientHoldMs * 0.001f));

    float maxDampingLinear = 1.0f;

    for (size_t i = 0; i < numSamples; ++i) {
        float xL = inL[i];
        float xR = inR[i];
        float inMag = (std::abs(xL) + std::abs(xR)) * 0.5f;

        // 1. Dual-envelope transient detector
        m_fastEnvL = fastAlpha * m_fastEnvL + (1.0f - fastAlpha) * inMag;
        m_slowEnvL = slowAlpha * m_slowEnvL + (1.0f - slowAlpha) * inMag;

        // Transient spike trigger
        if (m_fastEnvL > m_slowEnvL * 2.2f && inMag > 0.05f) {
            m_transientActive = true;
            m_transientHoldCounter = 1.0f; // Reset transparency hold
        } else if (m_transientHoldCounter > 0.0f) {
            m_transientHoldCounter -= holdDec;
            if (m_transientHoldCounter <= 0.0f) {
                m_transientActive = false;
            }
        }

        // 2. If transient hold has elapsed, detect ringing overhang and apply notches
        float accL = xL;
        float accR = xR;

        if (!m_transientActive && inMag > 0.01f) {
            // Ringing overhang phase: check which band is sustaining disproportionately
            for (size_t k = 0; k < NUM_RING_TRACKERS; ++k) {
                auto& n = m_notches[k];

                // Calculate filtered bandpass energy
                float yL = n.b0 * accL + n.b1 * n.x1L + n.b2 * n.x2L - n.a1 * n.y1L - n.a2 * n.y2L;
                n.x2L = n.x1L; n.x1L = accL; n.y2L = n.y1L; n.y1L = yL;

                float yR = n.b0 * accR + n.b1 * n.x1R + n.b2 * n.x2R - n.a1 * n.y1R - n.a2 * n.y2R;
                n.x2R = n.x1R; n.x1R = accR; n.y2R = n.y1R; n.y1R = yR;

                accL = yL;
                accR = yR;

                // Update dynamic notch damping
                if ((i & 31) == 0) {
                    float bandGain = targetDepthLinear;
                    if (std::abs(bandGain - n.currentGain) > 0.05f) {
                        updateNotchCoeffs(k, bandGain);
                    }
                    if (bandGain < maxDampingLinear) {
                        maxDampingLinear = bandGain;
                        m_dominantRingingFreqHz = n.centerFreqHz;
                    }
                }
            }
        } else {
            // During transient attack, filters are bypassed (transparent)
            for (size_t k = 0; k < NUM_RING_TRACKERS; ++k) {
                auto& n = m_notches[k];
                n.x1L = accL; n.x2L = accL; n.y1L = accL; n.y2L = accL;
                n.x1R = accR; n.x2R = accR; n.y1R = accR; n.y2R = accR;
            }
            maxDampingLinear = 1.0f;
        }

        if (m_params.ringingSolo) {
            outL[i] = xL - accL;
            outR[i] = xR - accR;
        } else {
            outL[i] = accL;
            outR[i] = accR;
        }
    }

    m_currentDampingReductionDb = 20.0f * std::log10(std::max(0.001f, maxDampingLinear));
}

} // namespace Jazz
