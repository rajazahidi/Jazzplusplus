//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// TapePrintThroughDemagSimulator.cpp - Tape Print-Through & Demagnetization
//*****************************************************************************

#include "TapePrintThroughDemagSimulator.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

JZTapePrintThroughDemagSimulator::JZTapePrintThroughDemagSimulator(float sampleRate) {
    initialize(sampleRate);
}

void JZTapePrintThroughDemagSimulator::initialize(float sampleRate) {
    if (sampleRate < 8000.0f) sampleRate = 48000.0f;
    m_sampleRate = sampleRate;

    // Buffer capacity for up to 6.0 seconds of tape revolution history
    m_bufferCapacity = static_cast<size_t>(6.0f * m_sampleRate);
    m_tapeReelBufL.assign(m_bufferCapacity, 0.0f);
    m_tapeReelBufR.assign(m_bufferCapacity, 0.0f);

    reset();
    updateReelTiming();
    updateDemagFilter();
}

void JZTapePrintThroughDemagSimulator::reset() {
    std::fill(m_tapeReelBufL.begin(), m_tapeReelBufL.end(), 0.0f);
    std::fill(m_tapeReelBufR.begin(), m_tapeReelBufR.end(), 0.0f);
    m_writePos = 0;

    m_demagX1L = m_demagX2L = m_demagY1L = m_demagY2L = 0.0f;
    m_demagX1R = m_demagX2R = m_demagY1R = m_demagY2R = 0.0f;
}

void JZTapePrintThroughDemagSimulator::setParameters(const Parameters& params) {
    m_params = params;
    m_params.reelRadiusCm = std::max(5.0f, std::min(18.0f, m_params.reelRadiusCm));
    m_params.printThroughLevelDb = std::max(-80.0f, std::min(-35.0f, m_params.printThroughLevelDb));
    m_params.playbackPasses = std::max(0, std::min(100, m_params.playbackPasses));
    updateReelTiming();
    updateDemagFilter();
}

void JZTapePrintThroughDemagSimulator::setReelRadiusCm(float cm) {
    m_params.reelRadiusCm = std::max(5.0f, std::min(18.0f, cm));
    updateReelTiming();
}

void JZTapePrintThroughDemagSimulator::setPrintThroughLevelDb(float db) {
    m_params.printThroughLevelDb = std::max(-80.0f, std::min(-35.0f, db));
}

void JZTapePrintThroughDemagSimulator::setPlaybackPasses(int passes) {
    m_params.playbackPasses = std::max(0, std::min(100, passes));
    updateDemagFilter();
}

void JZTapePrintThroughDemagSimulator::updateReelTiming() {
    // Tape linear speed in cm/s: speedIps * 2.54
    float tapeSpeedCmSec = m_params.tapeSpeedIps * 2.54f;
    if (tapeSpeedCmSec < 5.0f) tapeSpeedCmSec = 38.1f;

    // Reel circumference revolution period: T = 2 * pi * R / v
    float circumferenceCm = 2.0f * static_cast<float>(M_PI) * m_params.reelRadiusCm;
    m_revolutionPeriodSec = circumferenceCm / tapeSpeedCmSec;
}

void JZTapePrintThroughDemagSimulator::updateDemagFilter() {
    // High frequency demagnetization attenuation: -0.12 dB * passes
    m_demagLossDb = -m_params.highLossPerPassDb * static_cast<float>(m_params.playbackPasses);
    if (m_demagLossDb < -12.0f) m_demagLossDb = -12.0f;

    // High-shelf filter at 12 kHz
    float f0 = 12000.0f;
    if (f0 >= m_sampleRate * 0.45f) f0 = m_sampleRate * 0.45f;
    float A = std::pow(10.0f, m_demagLossDb / 40.0f);
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

    m_demagB0 = b0 / a0;
    m_demagB1 = b1 / a0;
    m_demagB2 = b2 / a0;
    m_demagA1 = a1 / a0;
    m_demagA2 = a2 / a0;
}

void JZTapePrintThroughDemagSimulator::process(
    const float* inL, const float* inR,
    float* outL, float* outR,
    size_t numSamples) {

    if (!inL || !outL || numSamples == 0) return;
    const bool stereo = (inR != nullptr && outR != nullptr);

    size_t revDelaySamples = static_cast<size_t>(m_revolutionPeriodSec * m_sampleRate);
    if (revDelaySamples < 64) revDelaySamples = 64;
    if (revDelaySamples * 2 >= m_bufferCapacity) revDelaySamples = (m_bufferCapacity / 2) - 1;

    float baseLinear = std::pow(10.0f, m_params.printThroughLevelDb / 20.0f);
    float preGain = baseLinear * m_params.preEchoRatio;
    float postGain = baseLinear * (1.0f - m_params.preEchoRatio * 0.4f);

    for (size_t i = 0; i < numSamples; ++i) {
        float sampleL = inL[i];
        float sampleR = stereo ? inR[i] : sampleL;

        // Apply demagnetization high frequency loss
        float demagL = m_demagB0 * sampleL + m_demagB1 * m_demagX1L + m_demagB2 * m_demagX2L
                       - m_demagA1 * m_demagY1L - m_demagA2 * m_demagY2L;
        m_demagX2L = m_demagX1L; m_demagX1L = sampleL;
        m_demagY2L = m_demagY1L; m_demagY1L = demagL;

        float demagR = demagL;
        if (stereo) {
            demagR = m_demagB0 * sampleR + m_demagB1 * m_demagX1R + m_demagB2 * m_demagX2R
                     - m_demagA1 * m_demagY1R - m_demagA2 * m_demagY2R;
            m_demagX2R = m_demagX1R; m_demagX1R = sampleR;
            m_demagY2R = m_demagY1R; m_demagY1R = demagR;
        }

        // Write directly to tape reel buffer
        m_tapeReelBufL[m_writePos] = demagL;
        m_tapeReelBufR[m_writePos] = demagR;

        // Direct tape playback (at latency D = revDelaySamples)
        size_t directReadPos = (m_writePos + m_bufferCapacity - revDelaySamples) % m_bufferCapacity;
        float directL = m_tapeReelBufL[directReadPos];
        float directR = m_tapeReelBufR[directReadPos];

        // Pre-echo (signal from inner wrap: current incoming sample before direct)
        float preL = demagL * preGain;
        float preR = demagR * preGain;

        // Post-echo (signal from outer wrap: past sample read at 2*D)
        size_t postReadPos = (m_writePos + m_bufferCapacity - (revDelaySamples * 2)) % m_bufferCapacity;
        float postL = m_tapeReelBufL[postReadPos] * postGain;
        float postR = m_tapeReelBufR[postReadPos] * postGain;

        m_writePos = (m_writePos + 1) % m_bufferCapacity;

        outL[i] = directL + preL + postL;
        if (stereo) outR[i] = directR + preR + postR;
    }
}

} // namespace Jazz
