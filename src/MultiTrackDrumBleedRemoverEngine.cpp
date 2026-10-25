//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// MultiTrackDrumBleedRemoverEngine.cpp - Multi-Mic Drum Spill De-Bleeder
//*****************************************************************************

#include "MultiTrackDrumBleedRemoverEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

JZMultiTrackDrumBleedRemoverEngine::JZMultiTrackDrumBleedRemoverEngine(float sampleRate) {
    initialize(sampleRate);
}

void JZMultiTrackDrumBleedRemoverEngine::initialize(float sampleRate) {
    if (sampleRate < 8000.0f) sampleRate = 48000.0f;
    m_sampleRate = sampleRate;

    reset();
    updateFilterCoefficients();
}

void JZMultiTrackDrumBleedRemoverEngine::reset() {
    m_shelfX1L = m_shelfX2L = m_shelfY1L = m_shelfY2L = 0.0f;
    m_shelfX1R = m_shelfX2R = m_shelfY1R = m_shelfY2R = 0.0f;

    m_bpX1 = m_bpX2 = m_bpY1 = m_bpY2 = 0.0f;

    m_gateEnvelope = 0.0f;
    m_holdCounterSec = 0.0f;
    m_detectedHits = 0;
    m_currentReductionDb = 0.0f;
}

void JZMultiTrackDrumBleedRemoverEngine::setParameters(const Parameters& params) {
    m_params = params;
    m_params.bleedReductionDb = std::max(-36.0f, std::min(-3.0f, m_params.bleedReductionDb));
    m_params.sensitivity = std::max(0.1f, std::min(3.0f, m_params.sensitivity));
    m_params.transientThresholdDb = std::max(-60.0f, std::min(0.0f, m_params.transientThresholdDb));
    m_params.holdMs = std::max(5.0f, std::min(300.0f, m_params.holdMs));
    m_params.decayMs = std::max(10.0f, std::min(600.0f, m_params.decayMs));
    m_params.crossoverFreqHz = std::max(800.0f, std::min(8000.0f, m_params.crossoverFreqHz));

    updateFilterCoefficients();
}

void JZMultiTrackDrumBleedRemoverEngine::setBleedReductionDb(float db) {
    m_params.bleedReductionDb = std::max(-36.0f, std::min(-3.0f, db));
    updateFilterCoefficients();
}

void JZMultiTrackDrumBleedRemoverEngine::setSensitivity(float sens) {
    m_params.sensitivity = std::max(0.1f, std::min(3.0f, sens));
}

void JZMultiTrackDrumBleedRemoverEngine::setTransientThresholdDb(float db) {
    m_params.transientThresholdDb = std::max(-60.0f, std::min(0.0f, db));
}

void JZMultiTrackDrumBleedRemoverEngine::setHoldMs(float ms) {
    m_params.holdMs = std::max(5.0f, std::min(300.0f, ms));
}

void JZMultiTrackDrumBleedRemoverEngine::updateFilterCoefficients() {
    // 1. Drum Body Transient Bandpass at 220 Hz, Q = 1.6
    float bpF0 = 220.0f;
    float bpW0 = 2.0f * static_cast<float>(M_PI) * (bpF0 / m_sampleRate);
    float bpAlpha = std::sin(bpW0) / (2.0f * 1.6f);
    float bpA0 = 1.0f + bpAlpha;

    m_bpB0 = bpAlpha / bpA0;
    m_bpB1 = 0.0f;
    m_bpB2 = -bpAlpha / bpA0;
    m_bpA1 = (-2.0f * std::cos(bpW0)) / bpA0;
    m_bpA2 = (1.0f - bpAlpha) / bpA0;

    // 2. High-shelf spill de-bleeding filter at crossover frequency
    float f0 = m_params.crossoverFreqHz;
    if (f0 >= m_sampleRate * 0.45f) f0 = m_sampleRate * 0.45f;
    float A = std::pow(10.0f, m_params.bleedReductionDb / 40.0f);
    float w0 = 2.0f * static_cast<float>(M_PI) * (f0 / m_sampleRate);
    float cos_w = std::cos(w0);
    float sin_w = std::sin(w0);
    float alpha = sin_w / (2.0f * 0.7071f);

    float b0 = A * ((A + 1.0f) + (A - 1.0f) * cos_w + 2.0f * std::sqrt(A) * alpha);
    float b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cos_w);
    float b2 = A * ((A + 1.0f) + (A - 1.0f) * cos_w - 2.0f * std::sqrt(A) * alpha);
    float a0 = (A + 1.0f) - (A - 1.0f) * cos_w + 2.0f * std::sqrt(A) * alpha;
    float a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cos_w);
    float a2 = (A + 1.0f) - (A - 1.0f) * cos_w - 2.0f * std::sqrt(A) * alpha;

    m_shelfB0 = b0 / a0;
    m_shelfB1 = b1 / a0;
    m_shelfB2 = b2 / a0;
    m_shelfA1 = a1 / a0;
    m_shelfA2 = a2 / a0;

    m_decayCoeff = std::exp(-1.0f / ((m_params.decayMs / 1000.0f) * m_sampleRate));
}

void JZMultiTrackDrumBleedRemoverEngine::process(
    const float* targetInL, const float* targetInR,
    const float* spillRefL, const float* spillRefR,
    float* outL, float* outR,
    size_t numSamples) {

    if (!targetInL || !outL || numSamples == 0) return;
    const bool stereo = (targetInR != nullptr && outR != nullptr);
    const bool hasSpillRef = (spillRefL != nullptr);

    float threshLinear = std::pow(10.0f, m_params.transientThresholdDb / 20.0f);
    float dt = 1.0f / m_sampleRate;
    float totalAttenuationAccum = 0.0f;

    for (size_t i = 0; i < numSamples; ++i) {
        float inL = targetInL[i];
        float inR = stereo ? targetInR[i] : inL;
        float refL = hasSpillRef ? spillRefL[i] : 0.0f;

        // 1. Detect target drum transient via bandpass
        float bpOut = m_bpB0 * inL + m_bpB1 * m_bpX1 + m_bpB2 * m_bpX2
                      - m_bpA1 * m_bpY1 - m_bpA2 * m_bpY2;
        m_bpX2 = m_bpX1; m_bpX1 = inL;
        m_bpY2 = m_bpY1; m_bpY1 = bpOut;

        // Peak detector
        if (std::abs(bpOut) > threshLinear) {
            if (m_gateEnvelope < 0.2f) {
                m_detectedHits++;
            }
            m_gateEnvelope = 1.0f;
            m_holdCounterSec = m_params.holdMs / 1000.0f;
        } else {
            if (m_holdCounterSec > 0.0f) {
                m_holdCounterSec -= dt;
            } else {
                m_gateEnvelope = m_decayCoeff * m_gateEnvelope;
            }
        }

        // 2. High-shelf bleed filtering on drum track
        float shelfL = m_shelfB0 * inL + m_shelfB1 * m_shelfX1L + m_shelfB2 * m_shelfX2L
                       - m_shelfA1 * m_shelfY1L - m_shelfA2 * m_shelfY2L;
        m_shelfX2L = m_shelfX1L; m_shelfX1L = inL;
        m_shelfY2L = m_shelfY1L; m_shelfY1L = shelfL;

        float shelfR = inR;
        if (stereo) {
            shelfR = m_shelfB0 * inR + m_shelfB1 * m_shelfX1R + m_shelfB2 * m_shelfX2R
                     - m_shelfA1 * m_shelfY1R - m_shelfA2 * m_shelfY2R;
            m_shelfX2R = m_shelfX1R; m_shelfX1R = inR;
            m_shelfY2R = m_shelfY1R; m_shelfY1R = shelfR;
        }

        // 3. Bleed suppression weighting based on spill reference and gate envelope
        float deBleedMix = (1.0f - m_gateEnvelope);
        if (hasSpillRef) {
            float spillFactor = std::abs(refL) * m_params.sensitivity;
            deBleedMix = std::min(1.0f, deBleedMix + spillFactor * 0.3f);
        }

        float cleanL = inL * (1.0f - deBleedMix) + shelfL * deBleedMix;
        float cleanR = inR * (1.0f - deBleedMix) + shelfR * deBleedMix;

        totalAttenuationAccum += deBleedMix;

        if (m_params.soloBleed) {
            outL[i] = inL - cleanL;
            if (stereo) outR[i] = inR - cleanR;
        } else {
            outL[i] = cleanL;
            if (stereo) outR[i] = cleanR;
        }
    }

    float avgDeBleed = totalAttenuationAccum / static_cast<float>(numSamples);
    m_currentReductionDb = m_params.bleedReductionDb * avgDeBleed;
}

} // namespace Jazz
