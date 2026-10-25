//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZDynamicResonanceSuppressorEngine.cpp - Dynamic Spectral Resonance Suppressor
//*****************************************************************************

#include "DynamicResonanceSuppressorEngine.h"
#include <algorithm>
#include <cmath>

namespace Jazz {

JZDynamicResonanceSuppressorEngine::JZDynamicResonanceSuppressorEngine(float sampleRate)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
    , m_depth(0.60f)
    , m_sharpness(3.5f)
    , m_selectivity(0.70f)
    , m_attackMs(2.0f)
    , m_releaseMs(40.0f)
    , m_deltaMode(false)
    , m_maxSuppressionDb(0.0f)
{
    initBands();
    reset();
}

void JZDynamicResonanceSuppressorEngine::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        m_sampleRate = sampleRate;
        reset();
    }
}

void JZDynamicResonanceSuppressorEngine::initBands() {
    // Problematic harsh whistling / ringing resonance frequencies
    // Typically in 800Hz to 8.5kHz range (vocal sibilance, snare ring, cymbal spill)
    static const float defaultFreqs[NUM_BANDS] = {
        650.0f, 1200.0f, 2100.0f, 3200.0f, 4500.0f, 5800.0f, 7500.0f, 9200.0f
    };

    for (size_t b = 0; b < NUM_BANDS; ++b) {
        m_bandCenterFreqs[b] = defaultFreqs[b];
        m_bandEnvelopesL[b] = 0.0f;
        m_bandEnvelopesR[b] = 0.0f;
        m_bandGainsDb[b] = 0.0f;
        m_filterState1L[b] = m_filterState2L[b] = 0.0f;
        m_filterState1R[b] = m_filterState2R[b] = 0.0f;
    }
}

void JZDynamicResonanceSuppressorEngine::setDepth(float depth) {
    m_depth = std::clamp(depth, 0.0f, 1.0f);
}

void JZDynamicResonanceSuppressorEngine::setSharpness(float sharpness) {
    m_sharpness = std::clamp(sharpness, 0.5f, 10.0f);
}

void JZDynamicResonanceSuppressorEngine::setSelectivity(float selectivity) {
    m_selectivity = std::clamp(selectivity, 0.0f, 1.0f);
}

void JZDynamicResonanceSuppressorEngine::setAttackReleaseMs(float attackMs, float releaseMs) {
    m_attackMs = std::clamp(attackMs, 0.1f, 100.0f);
    m_releaseMs = std::clamp(releaseMs, 1.0f, 500.0f);
}

void JZDynamicResonanceSuppressorEngine::reset() {
    for (size_t b = 0; b < NUM_BANDS; ++b) {
        m_bandEnvelopesL[b] = 0.0f;
        m_bandEnvelopesR[b] = 0.0f;
        m_bandGainsDb[b] = 0.0f;
        m_filterState1L[b] = m_filterState2L[b] = 0.0f;
        m_filterState1R[b] = m_filterState2R[b] = 0.0f;
    }
    m_detectedPeaks.clear();
    m_maxSuppressionDb = 0.0f;
}

void JZDynamicResonanceSuppressorEngine::process(const float* inL, const float* inR,
                                                float* outL, float* outR, size_t numSamples) {
    if (!inL || !inR || !outL || !outR || numSamples == 0) return;

    float dt = 1.0f / m_sampleRate;
    float alphaAttack = dt / ((m_attackMs * 0.001f) + dt);
    float alphaRelease = dt / ((m_releaseMs * 0.001f) + dt);

    m_detectedPeaks.clear();
    float globalMaxSupp = 0.0f;

    for (size_t s = 0; s < numSamples; ++s) {
        float xL = inL[s];
        float xR = inR[s];

        float deltaL = 0.0f;
        float deltaR = 0.0f;

        // Process through 8 resonant tracking band-pass filters
        for (size_t b = 0; b < NUM_BANDS; ++b) {
            float f0 = m_bandCenterFreqs[b];
            float q = m_sharpness;

            // Simple 2nd-order resonator
            float w0 = 2.0f * 3.14159265f * f0 / m_sampleRate;
            float cosW = std::cos(w0);
            float sinW = std::sin(w0);
            float alpha = sinW / (2.0f * q);

            float b0 = alpha;
            float b1 = 0.0f;
            float b2 = -alpha;
            float a0 = 1.0f + alpha;
            float a1 = -2.0f * cosW;
            float a2 = 1.0f - alpha;

            float invA0 = 1.0f / a0;
            b0 *= invA0; b2 *= invA0; a1 *= invA0; a2 *= invA0;

            // Bandpass filter
            float bandL = b0 * xL + b2 * m_filterState2L[b] - a1 * m_filterState1L[b] - a2 * m_filterState2L[b];
            m_filterState2L[b] = m_filterState1L[b];
            m_filterState1L[b] = bandL;

            float bandR = b0 * xR + b2 * m_filterState2R[b] - a1 * m_filterState1R[b] - a2 * m_filterState2R[b];
            m_filterState2R[b] = m_filterState1R[b];
            m_filterState1R[b] = bandR;

            // Track band energy envelope
            float absL = std::abs(bandL);
            float absR = std::abs(bandR);

            m_bandEnvelopesL[b] += (absL > m_bandEnvelopesL[b] ? alphaAttack : alphaRelease) * (absL - m_bandEnvelopesL[b]);
            m_bandEnvelopesR[b] += (absR > m_bandEnvelopesR[b] ? alphaAttack : alphaRelease) * (absR - m_bandEnvelopesR[b]);

            float bandEnergy = (m_bandEnvelopesL[b] + m_bandEnvelopesR[b]) * 0.5f;

            // Determine if this band forms an aggressive resonant peak above average signal
            float rawInEnergy = (std::abs(xL) + std::abs(xR)) * 0.5f + 1e-6f;
            float ratio = bandEnergy / rawInEnergy;

            // Threshold-free suppression calculation
            float excess = std::max(0.0f, ratio - (1.0f - m_selectivity * 0.5f));
            float suppressionGain = std::clamp(excess * m_depth * 18.0f, 0.0f, 18.0f); // Max 18 dB damping

            m_bandGainsDb[b] = -suppressionGain;
            if (suppressionGain > globalMaxSupp) {
                globalMaxSupp = suppressionGain;
            }

            // Subtract harsh resonance in proportion to suppression gain
            float attenuationCoeff = 1.0f - std::pow(10.0f, -suppressionGain / 20.0f);
            deltaL += bandL * attenuationCoeff;
            deltaR += bandR * attenuationCoeff;
        }

        if (m_deltaMode) {
            outL[s] = deltaL;
            outR[s] = deltaR;
        } else {
            outL[s] = xL - deltaL;
            outR[s] = xR - deltaR;
        }
    }

    m_maxSuppressionDb = globalMaxSupp;

    // Log detected peaks
    for (size_t b = 0; b < NUM_BANDS; ++b) {
        if (m_bandGainsDb[b] < -0.5f) {
            ResonancePeakInfo info;
            info.frequencyHz = m_bandCenterFreqs[b];
            info.deviationDb = -m_bandGainsDb[b];
            info.suppressionDb = -m_bandGainsDb[b];
            info.qFactor = m_sharpness;
            m_detectedPeaks.push_back(info);
        }
    }
}

} // namespace Jazz
