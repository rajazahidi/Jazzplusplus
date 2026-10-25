//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// AnalogBbdChorusDimensionEngine.cpp - Solid-State BBD Chorus & Dimension Matrix
//*****************************************************************************

#include "AnalogBbdChorusDimensionEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

JZAnalogBbdChorusDimensionEngine::JZAnalogBbdChorusDimensionEngine(float sampleRate) {
    initialize(sampleRate);
}

void JZAnalogBbdChorusDimensionEngine::initialize(float sampleRate) {
    if (sampleRate < 8000.0f) sampleRate = 48000.0f;
    m_sampleRate = sampleRate;

    m_bbdBufferL.assign(BBD_BUF_SIZE, 0.0f);
    m_bbdBufferR.assign(BBD_BUF_SIZE, 0.0f);

    reset();
    updateReconstructionFilter(50000.0f);
}

void JZAnalogBbdChorusDimensionEngine::reset() {
    std::fill(m_bbdBufferL.begin(), m_bbdBufferL.end(), 0.0f);
    std::fill(m_bbdBufferR.begin(), m_bbdBufferR.end(), 0.0f);
    m_writePos = 0;
    m_lfoPhase = 0.0f;

    m_lpX1L = m_lpX2L = m_lpY1L = m_lpY2L = 0.0f;
    m_lpX1R = m_lpX2R = m_lpY1R = m_lpY2R = 0.0f;
}

void JZAnalogBbdChorusDimensionEngine::setParameters(const Parameters& params) {
    m_params = params;
    m_params.numStages = (m_params.numStages <= 768) ? 512 : ((m_params.numStages <= 1536) ? 1024 : 2048);
    m_params.ctiLossFactor = std::max(0.0001f, std::min(0.01f, m_params.ctiLossFactor));
    m_params.clockBleedDb = std::max(-80.0f, std::min(-35.0f, m_params.clockBleedDb));
    m_params.dryWet = std::max(0.0f, std::min(1.0f, m_params.dryWet));
}

void JZAnalogBbdChorusDimensionEngine::setDimensionMode(BbdDimensionMode mode) {
    m_params.mode = mode;
}

void JZAnalogBbdChorusDimensionEngine::setNumStages(int stages) {
    m_params.numStages = (stages <= 768) ? 512 : ((stages <= 1536) ? 1024 : 2048);
}

void JZAnalogBbdChorusDimensionEngine::setCtiLossFactor(float cti) {
    m_params.ctiLossFactor = std::max(0.0001f, std::min(0.01f, cti));
}

void JZAnalogBbdChorusDimensionEngine::updateReconstructionFilter(float clockHz) {
    // Sallen-Key low-pass filter cutoff tracked to BBD clock
    float cutoffHz = clockHz * 0.35f;
    if (cutoffHz >= m_sampleRate * 0.45f) cutoffHz = m_sampleRate * 0.45f;
    if (cutoffHz < 4000.0f) cutoffHz = 4000.0f;

    float w0 = 2.0f * static_cast<float>(M_PI) * (cutoffHz / m_sampleRate);
    float sin_w = std::sin(w0);
    float cos_w = std::cos(w0);
    float alpha = sin_w / (2.0f * 0.7071f);

    float a0 = 1.0f + alpha;
    m_lpB0 = ((1.0f - cos_w) / 2.0f) / a0;
    m_lpB1 = (1.0f - cos_w) / a0;
    m_lpB2 = ((1.0f - cos_w) / 2.0f) / a0;
    m_lpA1 = (-2.0f * cos_w) / a0;
    m_lpA2 = (1.0f - alpha) / a0;
}

void JZAnalogBbdChorusDimensionEngine::process(
    const float* inL, const float* inR,
    float* outL, float* outR,
    size_t numSamples) {

    if (!inL || !outL || numSamples == 0) return;
    const bool stereo = (inR != nullptr && outR != nullptr);

    // Dimension Mode LFO Rate and Depth parameters
    float lfoRateHz = 0.45f;
    float depthMs = 2.4f;
    float baseDelayMs = 5.0f;

    switch (m_params.mode) {
        case BbdDimensionMode::MODE_1:
            lfoRateHz = 0.25f; depthMs = 1.2f; baseDelayMs = 4.0f; break;
        case BbdDimensionMode::MODE_2:
            lfoRateHz = 0.45f; depthMs = 2.4f; baseDelayMs = 5.0f; break;
        case BbdDimensionMode::MODE_3:
            lfoRateHz = 0.65f; depthMs = 3.8f; baseDelayMs = 6.2f; break;
        case BbdDimensionMode::MODE_4:
            lfoRateHz = 0.85f; depthMs = 5.0f; baseDelayMs = 7.5f; break;
    }

    float lfoPhaseInc = (2.0f * static_cast<float>(M_PI) * lfoRateHz) / m_sampleRate;
    float clockBleedAmp = std::pow(10.0f, m_params.clockBleedDb / 20.0f);
    float wet = m_params.dryWet;
    float dry = 1.0f - wet;

    // Cumulative CTI dispersion coefficient
    float totalCti = std::min(0.65f, static_cast<float>(m_params.numStages) * m_params.ctiLossFactor);

    for (size_t i = 0; i < numSamples; ++i) {
        float sL = inL[i];
        float sR = stereo ? inR[i] : sL;

        // Ingest into BBD circular buffer
        m_bbdBufferL[m_writePos] = sL;
        m_bbdBufferR[m_writePos] = sR;

        // Dual Anti-Phase LFO modulation
        float lfoSin = std::sin(m_lfoPhase);
        m_delayMsL = baseDelayMs + depthMs * lfoSin;
        m_delayMsR = baseDelayMs - depthMs * lfoSin;

        // Calculate instantaneous BBD clock frequency: f_clock = 2 * N / T_delay
        float avgDelaySec = (m_delayMsL + m_delayMsR) * 0.5f * 1e-3f;
        m_currentClockHz = (2.0f * static_cast<float>(m_params.numStages)) / avgDelaySec;

        // Sample delay lines with linear interpolation
        float delaySamplesL = (m_delayMsL / 1000.0f) * m_sampleRate;
        float delaySamplesR = (m_delayMsR / 1000.0f) * m_sampleRate;

        // Read Left
        float rPosL = static_cast<float>(m_writePos) - delaySamplesL;
        while (rPosL < 0.0f) rPosL += static_cast<float>(BBD_BUF_SIZE);
        size_t rIdxL0 = static_cast<size_t>(rPosL) % BBD_BUF_SIZE;
        size_t rIdxL1 = (rIdxL0 + 1) % BBD_BUF_SIZE;
        float fracL = rPosL - std::floor(rPosL);
        float bbdOutL = m_bbdBufferL[rIdxL0] * (1.0f - fracL) + m_bbdBufferL[rIdxL1] * fracL;

        // Read Right
        float rPosR = static_cast<float>(m_writePos) - delaySamplesR;
        while (rPosR < 0.0f) rPosR += static_cast<float>(BBD_BUF_SIZE);
        size_t rIdxR0 = static_cast<size_t>(rPosR) % BBD_BUF_SIZE;
        size_t rIdxR1 = (rIdxR0 + 1) % BBD_BUF_SIZE;
        float fracR = rPosR - std::floor(rPosR);
        float bbdOutR = m_bbdBufferR[rIdxR0] * (1.0f - fracR) + m_bbdBufferR[rIdxR1] * fracR;

        // Apply Charge Transfer Inefficiency (CTI) smearing
        bbdOutL = bbdOutL * (1.0f - totalCti * 0.5f);
        bbdOutR = bbdOutR * (1.0f - totalCti * 0.5f);

        // Ultrasonic clock bleed simulation
        float clockBleed = clockBleedAmp * std::sin(m_lfoPhase * 32.0f);
        bbdOutL += clockBleed;
        bbdOutR -= clockBleed;

        // Low-pass reconstruction filtering
        float filtL = m_lpB0 * bbdOutL + m_lpB1 * m_lpX1L + m_lpB2 * m_lpX2L
                      - m_lpA1 * m_lpY1L - m_lpA2 * m_lpY2L;
        m_lpX2L = m_lpX1L; m_lpX1L = bbdOutL;
        m_lpY2L = m_lpY1L; m_lpY1L = filtL;

        float filtR = m_lpB0 * bbdOutR + m_lpB1 * m_lpX1R + m_lpB2 * m_lpX2R
                      - m_lpA1 * m_lpY1R - m_lpA2 * m_lpY2R;
        m_lpX2R = m_lpX1R; m_lpX1R = bbdOutR;
        m_lpY2R = m_lpY1R; m_lpY1R = filtR;

        // Dimension D Spatial Matrix Cross-Coupling
        float dimL, dimR;
        if (m_params.mode == BbdDimensionMode::MODE_4) {
            // Mode 4 cross-phase inversion
            dimL = sL * dry + (filtL * 0.85f - filtR * 0.35f) * wet;
            dimR = sR * dry + (filtR * 0.85f - filtL * 0.35f) * wet;
        } else {
            dimL = sL * dry + filtL * wet;
            dimR = sR * dry + filtR * wet;
        }

        outL[i] = dimL;
        if (stereo) outR[i] = dimR;

        m_writePos = (m_writePos + 1) % BBD_BUF_SIZE;
        m_lfoPhase += lfoPhaseInc;
        if (m_lfoPhase >= 2.0f * static_cast<float>(M_PI)) {
            m_lfoPhase -= 2.0f * static_cast<float>(M_PI);
        }
    }
}

} // namespace Jazz
