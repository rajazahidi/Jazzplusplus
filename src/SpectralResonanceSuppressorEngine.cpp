//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// SpectralResonanceSuppressorEngine.cpp - Dynamic Spectral Resonance Suppressor
//*****************************************************************************

#include "SpectralResonanceSuppressorEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

static constexpr float PI_F = 3.14159265358979323846f;

JZSpectralResonanceSuppressorEngine::JZSpectralResonanceSuppressorEngine(float sampleRate)
    : m_sampleRate(sampleRate)
{
    initialize(sampleRate);
}

void JZSpectralResonanceSuppressorEngine::initialize(float sampleRate)
{
    m_sampleRate = (sampleRate > 8000.0f) ? sampleRate : 48000.0f;

    // Distribute Bark-scale tracking frequencies across the focus zone
    for (size_t k = 0; k < NUM_BARK_BANDS; ++k) {
        m_bandFrequencies[k] = 800.0f * std::pow(1.22f, static_cast<float>(k));
        m_bandEnergies[k] = 0.0f;
    }

    // Default floating notch center frequencies
    for (size_t n = 0; n < NUM_NOTCHES; ++n) {
        m_notches[n].centerFreqHz = 1500.0f + static_cast<float>(n) * 1200.0f;
        m_notches[n].reset();
        updateNotchCoefficients(m_notches[n], m_notches[n].centerFreqHz, 0.0f, m_params.sharpnessQ);
    }

    reset();
}

void JZSpectralResonanceSuppressorEngine::reset()
{
    for (size_t n = 0; n < NUM_NOTCHES; ++n) {
        m_notches[n].reset();
    }
    for (size_t k = 0; k < NUM_BARK_BANDS; ++k) {
        m_bandEnergies[k] = 0.0f;
    }
    m_maxSuppressionDb = 0.0f;
    m_averageResonanceIndex = 0.0f;
}

void JZSpectralResonanceSuppressorEngine::setParameters(const Parameters& params)
{
    m_params = params;
    m_params.thresholdDb = std::max(-36.0f, std::min(m_params.thresholdDb, -6.0f));
    m_params.depth = std::max(0.1f, std::min(m_params.depth, 1.0f));
    m_params.sharpnessQ = std::max(5.0f, std::min(m_params.sharpnessQ, 40.0f));
    m_params.minFreqHz = std::max(500.0f, std::min(m_params.minFreqHz, 4000.0f));
    m_params.maxFreqHz = std::max(5000.0f, std::min(m_params.maxFreqHz, 18000.0f));
    m_params.dryWet = std::max(0.0f, std::min(m_params.dryWet, 1.0f));
}

void JZSpectralResonanceSuppressorEngine::setThresholdDb(float db)
{
    m_params.thresholdDb = std::max(-36.0f, std::min(db, -6.0f));
}

void JZSpectralResonanceSuppressorEngine::setDepth(float depth)
{
    m_params.depth = std::max(0.1f, std::min(depth, 1.0f));
}

void JZSpectralResonanceSuppressorEngine::setSharpnessQ(float q)
{
    m_params.sharpnessQ = std::max(5.0f, std::min(q, 40.0f));
}

void JZSpectralResonanceSuppressorEngine::setFrequencyFocus(float minHz, float maxHz)
{
    m_params.minFreqHz = std::max(500.0f, std::min(minHz, 4000.0f));
    m_params.maxFreqHz = std::max(5000.0f, std::min(maxHz, 18000.0f));
}

void JZSpectralResonanceSuppressorEngine::setAuditionDeltaSolo(bool solo)
{
    m_params.auditionDeltaSolo = solo;
}

void JZSpectralResonanceSuppressorEngine::setDryWet(float wet)
{
    m_params.dryWet = std::max(0.0f, std::min(wet, 1.0f));
}

void JZSpectralResonanceSuppressorEngine::setBypass(bool bypass)
{
    m_params.bypassed = bypass;
}

size_t JZSpectralResonanceSuppressorEngine::getActiveNotchesCount() const
{
    size_t count = 0;
    for (size_t n = 0; n < NUM_NOTCHES; ++n) {
        if (m_notches[n].currentDepthDb < -0.8f) {
            count++;
        }
    }
    return count;
}

void JZSpectralResonanceSuppressorEngine::updateNotchCoefficients(
    NotchFilter& notch, float freqHz, float depthDb, float q)
{
    notch.centerFreqHz = freqHz;
    float w0 = 2.0f * PI_F * freqHz / m_sampleRate;
    float cosW0 = std::cos(w0);
    float sinW0 = std::sin(w0);
    float alpha = sinW0 / (2.0f * q);
    float A = std::pow(10.0f, depthDb / 40.0f); // RBJ Peaking EQ formula

    float b0 = 1.0f + alpha / A;
    float b1 = -2.0f * cosW0;
    float b2 = 1.0f - alpha / A;
    float a0 = 1.0f + alpha * A;
    float a1 = -2.0f * cosW0;
    float a2 = 1.0f - alpha * A;

    notch.b0 = b0 / a0;
    notch.b1 = b1 / a0;
    notch.b2 = b2 / a0;
    notch.a1 = a1 / a0;
    notch.a2 = a2 / a0;
}

void JZSpectralResonanceSuppressorEngine::process(
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

    float smoothCoeff = 1.0f - std::exp(-1000.0f / (20.0f * m_sampleRate)); // 20ms envelope
    float maxGr = 0.0f;

    for (size_t i = 0; i < numSamples; ++i) {
        float xL = inL[i];
        float xR = inR[i];
        float mono = (xL + xR) * 0.5f;

        // 1. Estimate energy in critical bands
        for (size_t k = 0; k < NUM_BARK_BANDS; ++k) {
            // Simplified resonant heterodyne energy tracker
            float bandW = 2.0f * PI_F * m_bandFrequencies[k] / m_sampleRate;
            float det = mono * std::sin(static_cast<float>(i) * bandW);
            float absDet = std::abs(det);
            m_bandEnergies[k] += smoothCoeff * (absDet - m_bandEnergies[k]);
        }

        // 2. Identify resonant peaks sticking out above smoothed neighborhood
        if (i % 64 == 0) {
            float meanEnergy = 0.0f;
            for (size_t k = 0; k < NUM_BARK_BANDS; ++k) {
                meanEnergy += m_bandEnergies[k];
            }
            meanEnergy /= static_cast<float>(NUM_BARK_BANDS);

            size_t notchIdx = 0;
            maxGr = 0.0f;

            for (size_t k = 1; k < NUM_BARK_BANDS - 1 && notchIdx < NUM_NOTCHES; ++k) {
                float freq = m_bandFrequencies[k];
                if (freq < m_params.minFreqHz || freq > m_params.maxFreqHz) continue;

                float localEnv = (m_bandEnergies[k-1] + m_bandEnergies[k+1]) * 0.5f;
                float peakDelta = (m_bandEnergies[k] - localEnv) / (localEnv + 1e-5f);

                if (peakDelta > 0.45f) { // Resonant harsh peak detected
                    float targetDepth = -std::min(18.0f, peakDelta * m_params.depth * 8.0f);
                    m_notches[notchIdx].targetDepthDb = targetDepth;
                    m_notches[notchIdx].currentDepthDb += 0.25f * (targetDepth - m_notches[notchIdx].currentDepthDb);
                    updateNotchCoefficients(m_notches[notchIdx], freq, m_notches[notchIdx].currentDepthDb, m_params.sharpnessQ);
                    maxGr = std::min(maxGr, m_notches[notchIdx].currentDepthDb);
                    notchIdx++;
                } else if (notchIdx < NUM_NOTCHES) {
                    m_notches[notchIdx].currentDepthDb *= 0.85f;
                    updateNotchCoefficients(m_notches[notchIdx], m_notches[notchIdx].centerFreqHz, m_notches[notchIdx].currentDepthDb, m_params.sharpnessQ);
                }
            }
            m_maxSuppressionDb = maxGr;
        }

        // 3. Cascade notch filtering
        float curL = xL;
        float curR = xR;

        for (size_t n = 0; n < NUM_NOTCHES; ++n) {
            auto& f = m_notches[n];
            if (f.currentDepthDb < -0.1f) {
                float yL = f.b0 * curL + f.b1 * f.x1L + f.b2 * f.x2L - f.a1 * f.y1L - f.a2 * f.y2L;
                f.x2L = f.x1L; f.x1L = curL;
                f.y2L = f.y1L; f.y1L = yL;
                curL = yL;

                float yR = f.b0 * curR + f.b1 * f.x1R + f.b2 * f.x2R - f.a1 * f.y1R - f.a2 * f.y2R;
                f.x2R = f.x1R; f.x1R = curR;
                f.y2R = f.y1R; f.y1R = yR;
                curR = yR;
            }
        }

        // Delta Solo Mode (listen exclusively to removed harshness)
        if (m_params.auditionDeltaSolo) {
            outL[i] = xL - curL;
            outR[i] = xR - curR;
            continue;
        }

        // Dry/Wet blend
        outL[i] = xL * (1.0f - m_params.dryWet) + curL * m_params.dryWet;
        outR[i] = xR * (1.0f - m_params.dryWet) + curR * m_params.dryWet;
    }
}

} // namespace Jazz
