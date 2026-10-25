//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// MasterBusGlueVcaCompressorEngine.cpp - VCA Master Bus Glue Compressor
//*****************************************************************************

#include "MasterBusGlueVcaCompressorEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

JZMasterBusGlueVcaCompressorEngine::JZMasterBusGlueVcaCompressorEngine(float sampleRate) {
    initialize(sampleRate);
}

void JZMasterBusGlueVcaCompressorEngine::initialize(float sampleRate) {
    if (sampleRate < 8000.0f) sampleRate = 48000.0f;
    m_sampleRate = sampleRate;

    reset();
    updateBallistics();
    updateThrustFilter();
}

void JZMasterBusGlueVcaCompressorEngine::reset() {
    m_scX1 = m_scX2 = m_scY1 = m_scY2 = 0.0f;
    m_envelope = 0.0f;
    m_autoReleaseSlow = 0.0f;
    m_currentGrDb = 0.0f;
}

void JZMasterBusGlueVcaCompressorEngine::setParameters(const Parameters& params) {
    m_params = params;
    m_params.thresholdDb = std::max(-40.0f, std::min(0.0f, m_params.thresholdDb));
    m_params.ratio = std::max(1.2f, std::min(20.0f, m_params.ratio));
    m_params.attackMs = std::max(0.05f, std::min(50.0f, m_params.attackMs));
    m_params.releaseSec = std::max(0.05f, std::min(2.5f, m_params.releaseSec));
    m_params.makeupGainDb = std::max(0.0f, std::min(24.0f, m_params.makeupGainDb));
    m_params.thrustHpFreqHz = std::max(40.0f, std::min(300.0f, m_params.thrustHpFreqHz));
    m_params.dryWet = std::max(0.0f, std::min(1.0f, m_params.dryWet));

    updateBallistics();
    updateThrustFilter();
}

void JZMasterBusGlueVcaCompressorEngine::setThresholdDb(float db) {
    m_params.thresholdDb = std::max(-40.0f, std::min(0.0f, db));
}

void JZMasterBusGlueVcaCompressorEngine::setRatio(float ratio) {
    m_params.ratio = std::max(1.2f, std::min(20.0f, ratio));
}

void JZMasterBusGlueVcaCompressorEngine::setMakeupGainDb(float db) {
    m_params.makeupGainDb = std::max(0.0f, std::min(24.0f, db));
}

void JZMasterBusGlueVcaCompressorEngine::setThrustFilterEnabled(bool enabled) {
    m_params.thrustFilterEnabled = enabled;
}

void JZMasterBusGlueVcaCompressorEngine::updateBallistics() {
    m_attackCoeff = std::exp(-1.0f / ((m_params.attackMs / 1000.0f) * m_sampleRate));
    float relTime = m_params.autoRelease ? 0.35f : m_params.releaseSec;
    m_releaseCoeff = std::exp(-1.0f / (relTime * m_sampleRate));
}

void JZMasterBusGlueVcaCompressorEngine::updateThrustFilter() {
    // 2nd-order Butterworth High-Pass at thrustHpFreqHz
    float f0 = m_params.thrustHpFreqHz;
    float w0 = 2.0f * static_cast<float>(M_PI) * (f0 / m_sampleRate);
    float cos_w = std::cos(w0);
    float alpha = std::sin(w0) / (2.0f * 0.7071f);
    float a0 = 1.0f + alpha;

    m_scB0 = ((1.0f + cos_w) / 2.0f) / a0;
    m_scB1 = (-(1.0f + cos_w)) / a0;
    m_scB2 = ((1.0f + cos_w) / 2.0f) / a0;
    m_scA1 = (-2.0f * cos_w) / a0;
    m_scA2 = (1.0f - alpha) / a0;
}

void JZMasterBusGlueVcaCompressorEngine::process(
    const float* inL, const float* inR,
    float* outL, float* outR,
    size_t numSamples) {

    if (!inL || !outL || numSamples == 0) return;
    const bool stereo = (inR != nullptr && outR != nullptr);

    float T = m_params.thresholdDb;
    float R = m_params.ratio;
    float K = 5.0f; // 5 dB Soft knee
    float makeupLinear = std::pow(10.0f, m_params.makeupGainDb / 20.0f);
    float wet = m_params.dryWet;
    float dry = 1.0f - wet;

    float minGrObserved = 0.0f;

    for (size_t i = 0; i < numSamples; ++i) {
        float sL = inL[i];
        float sR = stereo ? inR[i] : sL;

        // Detector input (max of stereo channels)
        float detIn = std::max(std::abs(sL), std::abs(sR));

        // Apply Thrust High-Pass Filter to sidechain detector
        if (m_params.thrustFilterEnabled) {
            float scFiltered = m_scB0 * detIn + m_scB1 * m_scX1 + m_scB2 * m_scX2
                               - m_scA1 * m_scY1 - m_scA2 * m_scY2;
            m_scX2 = m_scX1; m_scX1 = detIn;
            m_scY2 = m_scY1; m_scY1 = scFiltered;
            detIn = std::abs(scFiltered);
        }

        // Convert detector to dB
        float xDb = 20.0f * std::log10(std::max(1e-5f, detIn));

        // Soft-Knee VCA Gain Computer
        float grDb = 0.0f;
        if (xDb <= (T - K * 0.5f)) {
            grDb = 0.0f;
        } else if (xDb >= (T + K * 0.5f)) {
            grDb = (T + (xDb - T) / R) - xDb;
        } else {
            float excess = xDb - T + K * 0.5f;
            grDb = ((1.0f / R - 1.0f) * excess * excess) / (2.0f * K);
        }

        // Ballistics smoothing
        if (grDb < m_envelope) {
            m_envelope = m_attackCoeff * m_envelope + (1.0f - m_attackCoeff) * grDb;
        } else {
            float relCoeff = m_releaseCoeff;
            if (m_params.autoRelease) {
                // Program-dependent auto-release: slow component tracks RMS
                m_autoReleaseSlow = m_autoReleaseSlow * 0.999f + m_envelope * 0.001f;
                relCoeff = (m_envelope < m_autoReleaseSlow) ? 0.9995f : m_releaseCoeff;
            }
            m_envelope = relCoeff * m_envelope + (1.0f - relCoeff) * grDb;
        }

        if (m_envelope < minGrObserved) minGrObserved = m_envelope;

        // Convert smoothed gain reduction to linear multiplier
        float vcaGain = std::pow(10.0f, m_envelope / 20.0f);

        float compL = sL * vcaGain * makeupLinear;
        float compR = sR * vcaGain * makeupLinear;

        outL[i] = sL * dry + compL * wet;
        if (stereo) {
            outR[i] = sR * dry + compR * wet;
        }
    }

    m_currentGrDb = minGrObserved;
}

} // namespace Jazz
