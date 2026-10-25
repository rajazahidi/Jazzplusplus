//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// TapeHeadBumpVarispeedTransportEngine.cpp - Tape Head-Bump & Varispeed Engine
//*****************************************************************************

#include "TapeHeadBumpVarispeedTransportEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

JZTapeHeadBumpVarispeedTransportEngine::JZTapeHeadBumpVarispeedTransportEngine(float sampleRate) {
    initialize(sampleRate);
}

void JZTapeHeadBumpVarispeedTransportEngine::initialize(float sampleRate) {
    if (sampleRate < 8000.0f) sampleRate = 48000.0f;
    m_sampleRate = sampleRate;

    m_tapeBufL.assign(VARISPEED_BUF_SIZE, 0.0f);
    m_tapeBufR.assign(VARISPEED_BUF_SIZE, 0.0f);

    reset();
    updateHeadBumpFilter();
}

void JZTapeHeadBumpVarispeedTransportEngine::reset() {
    std::fill(m_tapeBufL.begin(), m_tapeBufL.end(), 0.0f);
    std::fill(m_tapeBufR.begin(), m_tapeBufR.end(), 0.0f);
    m_writePos = 0;
    m_readPos = 0.0f;

    m_bumpX1L = m_bumpX2L = m_bumpY1L = m_bumpY2L = 0.0f;
    m_bumpX1R = m_bumpX2R = m_bumpY1R = m_bumpY2R = 0.0f;

    m_currentMotorSpeed = m_params.varispeedFactor;
}

void JZTapeHeadBumpVarispeedTransportEngine::setParameters(const Parameters& params) {
    m_params = params;
    m_params.headBumpAmount = std::max(0.0f, std::min(2.5f, m_params.headBumpAmount));
    m_params.varispeedFactor = std::max(0.0f, std::min(2.5f, m_params.varispeedFactor));
    m_params.tapeStopDecaySec = std::max(0.1f, std::min(5.0f, m_params.tapeStopDecaySec));
    m_params.dryWet = std::max(0.0f, std::min(1.0f, m_params.dryWet));

    updateHeadBumpFilter();
}

void JZTapeHeadBumpVarispeedTransportEngine::setTapeSpeedIps(float ips) {
    m_params.tapeSpeedIps = ips;
    updateHeadBumpFilter();
}

void JZTapeHeadBumpVarispeedTransportEngine::setHeadBumpAmount(float amount) {
    m_params.headBumpAmount = std::max(0.0f, std::min(2.5f, amount));
    updateHeadBumpFilter();
}

void JZTapeHeadBumpVarispeedTransportEngine::setVarispeedFactor(float factor) {
    m_params.varispeedFactor = std::max(0.0f, std::min(2.5f, factor));
}

void JZTapeHeadBumpVarispeedTransportEngine::triggerTapeStop(bool stop) {
    m_params.tapeStopActive = stop;
}

void JZTapeHeadBumpVarispeedTransportEngine::updateHeadBumpFilter() {
    float gainDb = 3.5f * m_params.headBumpAmount;
    if (m_params.tapeSpeedIps <= 10.0f) {
        m_headBumpCenterHz = 35.0f; // 7.5 IPS
        gainDb = 2.5f * m_params.headBumpAmount;
    } else if (m_params.tapeSpeedIps <= 20.0f) {
        m_headBumpCenterHz = 55.0f; // 15 IPS (classic warmth)
        gainDb = 3.8f * m_params.headBumpAmount;
    } else {
        m_headBumpCenterHz = 110.0f; // 30 IPS (punchy upper bass)
        gainDb = 2.2f * m_params.headBumpAmount;
    }

    float f0 = m_headBumpCenterHz;
    float Q = 2.4f;
    float A = std::pow(10.0f, gainDb / 40.0f);
    float w0 = 2.0f * static_cast<float>(M_PI) * (f0 / m_sampleRate);
    float alpha = std::sin(w0) / (2.0f * Q);
    float cos_w = std::cos(w0);

    float b0 = 1.0f + alpha * A;
    float b1 = -2.0f * cos_w;
    float b2 = 1.0f - alpha * A;
    float a0 = 1.0f + alpha / A;
    float a1 = -2.0f * cos_w;
    float a2 = 1.0f - alpha / A;

    m_bumpB0 = b0 / a0;
    m_bumpB1 = b1 / a0;
    m_bumpB2 = b2 / a0;
    m_bumpA1 = a1 / a0;
    m_bumpA2 = a2 / a0;
}

void JZTapeHeadBumpVarispeedTransportEngine::process(
    const float* inL, const float* inR,
    float* outL, float* outR,
    size_t numSamples) {

    if (!inL || !outL || numSamples == 0) return;
    const bool stereo = (inR != nullptr && outR != nullptr);

    float targetSpeed = m_params.tapeStopActive ? 0.0f : m_params.varispeedFactor;
    float speedSlew = 1.0f / (m_params.tapeStopDecaySec * m_sampleRate);
    float wet = m_params.dryWet;
    float dry = 1.0f - wet;

    for (size_t i = 0; i < numSamples; ++i) {
        float sL = inL[i];
        float sR = stereo ? inR[i] : sL;

        // Ingest into circular buffer
        m_tapeBufL[m_writePos] = sL;
        m_tapeBufR[m_writePos] = sR;

        // Update motor transport inertia
        if (m_currentMotorSpeed < targetSpeed) {
            m_currentMotorSpeed = std::min(targetSpeed, m_currentMotorSpeed + speedSlew * 2.0f);
        } else if (m_currentMotorSpeed > targetSpeed) {
            m_currentMotorSpeed = std::max(targetSpeed, m_currentMotorSpeed - speedSlew);
        }

        // Fractional reading from tape buffer
        size_t rIdx0 = static_cast<size_t>(m_readPos) % VARISPEED_BUF_SIZE;
        size_t rIdx1 = (rIdx0 + 1) % VARISPEED_BUF_SIZE;
        float frac = m_readPos - std::floor(m_readPos);

        float tapeOutL = m_tapeBufL[rIdx0] * (1.0f - frac) + m_tapeBufL[rIdx1] * frac;
        float tapeOutR = m_tapeBufR[rIdx0] * (1.0f - frac) + m_tapeBufR[rIdx1] * frac;

        // Advance read position according to instantaneous motor speed
        m_readPos += m_currentMotorSpeed;
        while (m_readPos >= static_cast<float>(VARISPEED_BUF_SIZE)) {
            m_readPos -= static_cast<float>(VARISPEED_BUF_SIZE);
        }

        // Apply physical head-bump bass resonance
        float bumpedL = m_bumpB0 * tapeOutL + m_bumpB1 * m_bumpX1L + m_bumpB2 * m_bumpX2L
                        - m_bumpA1 * m_bumpY1L - m_bumpA2 * m_bumpY2L;
        m_bumpX2L = m_bumpX1L; m_bumpX1L = tapeOutL;
        m_bumpY2L = m_bumpY1L; m_bumpY1L = bumpedL;

        float bumpedR = bumpedL;
        if (stereo) {
            bumpedR = m_bumpB0 * tapeOutR + m_bumpB1 * m_bumpX1R + m_bumpB2 * m_bumpX2R
                      - m_bumpA1 * m_bumpY1R - m_bumpA2 * m_bumpY2R;
            m_bumpX2R = m_bumpX1R; m_bumpX1R = tapeOutR;
            m_bumpY2R = m_bumpY1R; m_bumpY1R = bumpedR;
        }

        outL[i] = sL * dry + bumpedL * wet;
        if (stereo) {
            outR[i] = sR * dry + bumpedR * wet;
        }

        m_writePos = (m_writePos + 1) % VARISPEED_BUF_SIZE;
    }
}

} // namespace Jazz
