//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// VocalSibilanceSplitDeEsserEngine.cpp - Intelligent Split-Band Vocal De-Esser
//*****************************************************************************

#include "VocalSibilanceSplitDeEsserEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

JZVocalSibilanceSplitDeEsserEngine::JZVocalSibilanceSplitDeEsserEngine(float sampleRate) {
    initialize(sampleRate);
}

void JZVocalSibilanceSplitDeEsserEngine::initialize(float sampleRate) {
    if (sampleRate < 8000.0f) sampleRate = 48000.0f;
    m_sampleRate = sampleRate;

    m_lookaheadBufL.assign(LOOKAHEAD_SIZE, 0.0f);
    m_lookaheadBufR.assign(LOOKAHEAD_SIZE, 0.0f);

    reset();
    updateFilterCoefficients();
}

void JZVocalSibilanceSplitDeEsserEngine::reset() {
    std::fill(m_lookaheadBufL.begin(), m_lookaheadBufL.end(), 0.0f);
    std::fill(m_lookaheadBufR.begin(), m_lookaheadBufR.end(), 0.0f);
    m_writePos = 0;

    m_scX1L = m_scX2L = m_scY1L = m_scY2L = 0.0f;
    m_scX1R = m_scX2R = m_scY1R = m_scY2R = 0.0f;

    m_bellX1L = m_bellX2L = m_bellY1L = m_bellY2L = 0.0f;
    m_bellX1R = m_bellX2R = m_bellY1R = m_bellY2R = 0.0f;

    m_gainSmoothed = 1.0f;
    m_currentReductionDb = 0.0f;
}

void JZVocalSibilanceSplitDeEsserEngine::setParameters(const Parameters& params) {
    m_params = params;
    m_params.frequencyHz = std::max(2500.0f, std::min(14000.0f, m_params.frequencyHz));
    m_params.thresholdDb = std::max(-60.0f, std::min(0.0f, m_params.thresholdDb));
    m_params.rangeDb = std::max(-32.0f, std::min(-2.0f, m_params.rangeDb));
    m_params.bandwidthQ = std::max(0.4f, std::min(4.0f, m_params.bandwidthQ));
    m_params.attackMs = std::max(0.2f, std::min(20.0f, m_params.attackMs));
    m_params.releaseMs = std::max(5.0f, std::min(300.0f, m_params.releaseMs));
    m_params.dryWet = std::max(0.0f, std::min(1.0f, m_params.dryWet));

    updateFilterCoefficients();
}

void JZVocalSibilanceSplitDeEsserEngine::setFrequencyHz(float hz) {
    m_params.frequencyHz = std::max(2500.0f, std::min(14000.0f, hz));
    updateFilterCoefficients();
}

void JZVocalSibilanceSplitDeEsserEngine::setThresholdDb(float db) {
    m_params.thresholdDb = std::max(-60.0f, std::min(0.0f, db));
}

void JZVocalSibilanceSplitDeEsserEngine::setRangeDb(float db) {
    m_params.rangeDb = std::max(-32.0f, std::min(-2.0f, db));
}

void JZVocalSibilanceSplitDeEsserEngine::updateFilterCoefficients() {
    float f0 = m_params.frequencyHz;
    if (f0 >= m_sampleRate * 0.45f) f0 = m_sampleRate * 0.45f;
    float Q = m_params.bandwidthQ;

    float w0 = 2.0f * static_cast<float>(M_PI) * (f0 / m_sampleRate);
    float sin_w = std::sin(w0);
    float cos_w = std::cos(w0);
    float alpha = sin_w / (2.0f * Q);

    // Bandpass detection biquad
    float a0 = 1.0f + alpha;
    m_scB0 = alpha / a0;
    m_scB1 = 0.0f;
    m_scB2 = -alpha / a0;
    m_scA1 = (-2.0f * cos_w) / a0;
    m_scA2 = (1.0f - alpha) / a0;

    m_attackCoeff = std::exp(-1.0f / ((m_params.attackMs / 1000.0f) * m_sampleRate));
    m_releaseCoeff = std::exp(-1.0f / ((m_params.releaseMs / 1000.0f) * m_sampleRate));
}

void JZVocalSibilanceSplitDeEsserEngine::process(
    const float* inL, const float* inR,
    float* outL, float* outR,
    size_t numSamples) {

    if (!inL || !outL || numSamples == 0) return;
    const bool stereo = (inR != nullptr && outR != nullptr);

    float threshLinear = std::pow(10.0f, m_params.thresholdDb / 20.0f);
    float minGain = std::pow(10.0f, m_params.rangeDb / 20.0f);
    float wet = m_params.dryWet;
    float dry = 1.0f - wet;

    float maxReductObserved = 1.0f;

    for (size_t i = 0; i < numSamples; ++i) {
        float sL = inL[i];
        float sR = stereo ? inR[i] : sL;

        // Store into lookahead buffer
        m_lookaheadBufL[m_writePos] = sL;
        m_lookaheadBufR[m_writePos] = sR;

        // 1. Detect sibilant band energy
        float scL = m_scB0 * sL + m_scB1 * m_scX1L + m_scB2 * m_scX2L
                    - m_scA1 * m_scY1L - m_scA2 * m_scY2L;
        m_scX2L = m_scX1L; m_scX1L = sL;
        m_scY2L = m_scY1L; m_scY1L = scL;

        float scR = scL;
        if (stereo) {
            scR = m_scB0 * sR + m_scB1 * m_scX1R + m_scB2 * m_scX2R
                  - m_scA1 * m_scY1R - m_scA2 * m_scY2R;
            m_scX2R = m_scX1R; m_scX1R = sR;
            m_scY2R = m_scY1R; m_scY1R = scR;
        }

        // Sidechain listen audition mode
        if (m_params.auditionSidechain) {
            outL[i] = scL;
            if (stereo) outR[i] = scR;
            m_writePos = (m_writePos + 1) % LOOKAHEAD_SIZE;
            continue;
        }

        // Sibilance peak level
        float sibilanceLevel = std::max(std::abs(scL), std::abs(scR));

        // 2. Compute dynamic gain reduction
        float targetGain = 1.0f;
        if (sibilanceLevel > threshLinear) {
            float excessRatio = (sibilanceLevel - threshLinear) / (sibilanceLevel + 1e-5f);
            targetGain = 1.0f - (1.0f - minGain) * std::min(1.0f, excessRatio * 2.0f);
        }

        // Ballistics smoothing
        if (targetGain < m_gainSmoothed) {
            m_gainSmoothed = m_attackCoeff * m_gainSmoothed + (1.0f - m_attackCoeff) * targetGain;
        } else {
            m_gainSmoothed = m_releaseCoeff * m_gainSmoothed + (1.0f - m_releaseCoeff) * targetGain;
        }

        if (m_gainSmoothed < maxReductObserved) maxReductObserved = m_gainSmoothed;

        // 3. Retrieve sample from lookahead delay buffer (lookahead = 64 samples ~ 1.3ms)
        size_t rPos = (m_writePos + LOOKAHEAD_SIZE - 64) % LOOKAHEAD_SIZE;
        float delayedL = m_lookaheadBufL[rPos];
        float delayedR = m_lookaheadBufR[rPos];

        // 4. Apply split-band or wideband attenuation
        float deEssedL, deEssedR;
        if (m_params.splitBand) {
            // Split-band: attenuate only the sibilant component, leaving body intact
            deEssedL = delayedL - scL * (1.0f - m_gainSmoothed);
            deEssedR = delayedR - scR * (1.0f - m_gainSmoothed);
        } else {
            // Wideband ducking
            deEssedL = delayedL * m_gainSmoothed;
            deEssedR = delayedR * m_gainSmoothed;
        }

        outL[i] = delayedL * dry + deEssedL * wet;
        if (stereo) {
            outR[i] = delayedR * dry + deEssedR * wet;
        }

        m_writePos = (m_writePos + 1) % LOOKAHEAD_SIZE;
    }

    m_currentReductionDb = 20.0f * std::log10(std::max(1e-4f, maxReductObserved));
}

} // namespace Jazz
