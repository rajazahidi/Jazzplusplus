//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZInfiniteBarberpoleFlangerEngine.cpp - Infinite Barberpole & Through-Zero Flanger
//*****************************************************************************

#include "InfiniteBarberpoleFlangerEngine.h"
#include <algorithm>
#include <cmath>

namespace Jazz {

JZInfiniteBarberpoleFlangerEngine::JZInfiniteBarberpoleFlangerEngine(float sampleRate)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
    , m_rateHz(0.15f) // Slow psychedelic swirl
    , m_direction(BarberpoleDirection::INFINITE_UPWARD)
    , m_throughZero(true)
    , m_feedback(0.65f)
    , m_mix(0.50f)
    , m_writeIdx(0)
{
    // Max delay buffer: 25ms (~1200 samples at 48kHz)
    size_t bufSize = static_cast<size_t>(0.025 * m_sampleRate) + 128;
    m_delayBufferL.assign(bufSize, 0.0f);
    m_delayBufferR.assign(bufSize, 0.0f);
    m_dryDelayL.assign(bufSize, 0.0f);
    m_dryDelayR.assign(bufSize, 0.0f);

    reset();
}

void JZInfiniteBarberpoleFlangerEngine::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        m_sampleRate = sampleRate;
        reset();
    }
}

void JZInfiniteBarberpoleFlangerEngine::setRateHz(float rateHz) {
    m_rateHz = std::clamp(rateHz, 0.01f, 10.0f);
}

void JZInfiniteBarberpoleFlangerEngine::setDirection(BarberpoleDirection dir) {
    m_direction = dir;
}

void JZInfiniteBarberpoleFlangerEngine::setFeedback(float feedback) {
    m_feedback = std::clamp(feedback, -0.95f, 0.95f);
}

void JZInfiniteBarberpoleFlangerEngine::setMix(float wetDry) {
    m_mix = std::clamp(wetDry, 0.0f, 1.0f);
}

void JZInfiniteBarberpoleFlangerEngine::reset() {
    std::fill(m_delayBufferL.begin(), m_delayBufferL.end(), 0.0f);
    std::fill(m_delayBufferR.begin(), m_delayBufferR.end(), 0.0f);
    std::fill(m_dryDelayL.begin(), m_dryDelayL.end(), 0.0f);
    std::fill(m_dryDelayR.begin(), m_dryDelayR.end(), 0.0f);
    m_writeIdx = 0;

    for (size_t p = 0; p < NUM_PHASES; ++p) {
        m_phaseAcc[p] = static_cast<double>(p) / static_cast<double>(NUM_PHASES);
        m_feedbackStateL[p] = 0.0f;
        m_feedbackStateR[p] = 0.0f;
    }
}

void JZInfiniteBarberpoleFlangerEngine::process(const float* inL, const float* inR,
                                               float* outL, float* outR, size_t numSamples) {
    if (!inL || !inR || !outL || !outR || numSamples == 0) return;

    size_t bufSize = m_delayBufferL.size();
    double phaseInc = (m_rateHz / m_sampleRate);
    if (m_direction == BarberpoleDirection::INFINITE_DOWNWARD) {
        phaseInc = -phaseInc;
    }

    // Delay range: 0.2ms to 8.0ms
    float minDelaySamples = 0.0002f * m_sampleRate;
    float maxDelaySamples = 0.0080f * m_sampleRate;
    float centerDelaySamples = (minDelaySamples + maxDelaySamples) * 0.5f;

    float dryGain = 1.0f - m_mix;
    float wetGain = m_mix;

    for (size_t i = 0; i < numSamples; ++i) {
        float xL = inL[i];
        float xR = inR[i];

        // Store dry signal into delay line
        m_delayBufferL[m_writeIdx] = xL;
        m_delayBufferR[m_writeIdx] = xR;
        m_dryDelayL[m_writeIdx] = xL;
        m_dryDelayR[m_writeIdx] = xR;

        // Through-zero reference signal
        size_t rCenter = (m_writeIdx + bufSize - static_cast<size_t>(std::round(centerDelaySamples))) % bufSize;
        float tzRefL = m_throughZero ? m_dryDelayL[rCenter] : xL;
        float tzRefR = m_throughZero ? m_dryDelayR[rCenter] : xR;

        float flangedSumL = 0.0f;
        float flangedSumR = 0.0f;
        float totalWeight = 0.0f;

        // Sum across 4 staggered barberpole phase taps
        for (size_t p = 0; p < NUM_PHASES; ++p) {
            m_phaseAcc[p] += phaseInc;
            if (m_phaseAcc[p] >= 1.0) m_phaseAcc[p] -= 1.0;
            if (m_phaseAcc[p] < 0.0)  m_phaseAcc[p] += 1.0;

            // Raised-cosine Hann window: zero at endpoints, peak in middle
            float window = 0.5f * (1.0f - std::cos(2.0f * 3.14159265f * static_cast<float>(m_phaseAcc[p])));
            float currentDelay = minDelaySamples + static_cast<float>(m_phaseAcc[p]) * (maxDelaySamples - minDelaySamples);

            // Linear interpolation delay read
            size_t intDelay = static_cast<size_t>(std::floor(currentDelay));
            float frac = currentDelay - static_cast<float>(intDelay);

            size_t rIdx0 = (m_writeIdx + bufSize - intDelay) % bufSize;
            size_t rIdx1 = (m_writeIdx + bufSize - intDelay - 1) % bufSize;

            float tapL = (m_delayBufferL[rIdx0] * (1.0f - frac) + m_delayBufferL[rIdx1] * frac) + m_feedbackStateL[p] * m_feedback;
            float tapR = (m_delayBufferR[rIdx0] * (1.0f - frac) + m_delayBufferR[rIdx1] * frac) + m_feedbackStateR[p] * m_feedback;

            m_feedbackStateL[p] = std::tanh(tapL);
            m_feedbackStateR[p] = std::tanh(tapR);

            flangedSumL += tapL * window;
            flangedSumR += tapR * window;
            totalWeight += window;
        }

        if (totalWeight > 1e-6f) {
            flangedSumL /= totalWeight;
            flangedSumR /= totalWeight;
        }

        // Through-Zero Jet Notch subtraction
        float wetL = m_throughZero ? (flangedSumL - tzRefL) : flangedSumL;
        float wetR = m_throughZero ? (flangedSumR - tzRefR) : flangedSumR;

        outL[i] = std::clamp(tzRefL * dryGain + wetL * wetGain, -1.0f, 1.0f);
        outR[i] = std::clamp(tzRefR * dryGain + wetR * wetGain, -1.0f, 1.0f);

        m_writeIdx = (m_writeIdx + 1) % bufSize;
    }
}

} // namespace Jazz
