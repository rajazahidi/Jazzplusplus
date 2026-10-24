#include "ThroughZeroFlanger.h"
#include <algorithm>
#include <cmath>

namespace Jazz {

static const float PI = 3.14159265358979323846f;

JZThroughZeroFlanger::JZThroughZeroFlanger(float sampleRate)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f)
    , m_rateHz(0.25f)
    , m_depthMs(1.8f)
    , m_centerDelayMs(2.0f)
    , m_feedback(0.65f)
    , m_polarity(FlangerPolarity::INVERTED)
    , m_mix(1.0f)
    , m_tapeDrive(1.5f)
    , m_stereoPhaseDeg(90.0f)
    , m_manualMode(false)
    , m_manualPos(0.0f)
    , m_writeIdx(0)
    , m_lfoPhaseL(0.0f)
    , m_lfoPhaseR(0.25f)
    , m_currentOffsetMs(0.0f)
    , m_feedbackStateL(0.0f)
    , m_feedbackStateR(0.0f)
{
    reset();
}

void JZThroughZeroFlanger::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        m_sampleRate = sampleRate;
    }
}

void JZThroughZeroFlanger::setRateHz(float rateHz) {
    m_rateHz = std::max(0.01f, std::min(10.0f, rateHz));
}

void JZThroughZeroFlanger::setDepthMs(float depthMs) {
    m_depthMs = std::max(0.05f, std::min(m_centerDelayMs * 0.98f, depthMs));
}

void JZThroughZeroFlanger::setCenterDelayMs(float centerMs) {
    m_centerDelayMs = std::max(0.5f, std::min(10.0f, centerMs));
    if (m_depthMs > m_centerDelayMs * 0.98f) {
        m_depthMs = m_centerDelayMs * 0.98f;
    }
}

void JZThroughZeroFlanger::setFeedback(float feedback) {
    m_feedback = std::max(-0.95f, std::min(0.95f, feedback));
}

void JZThroughZeroFlanger::setMix(float mix) {
    m_mix = std::max(0.0f, std::min(1.0f, mix));
}

void JZThroughZeroFlanger::setTapeDrive(float drive) {
    m_tapeDrive = std::max(1.0f, std::min(4.0f, drive));
}

void JZThroughZeroFlanger::setStereoPhaseOffsetDeg(float deg) {
    m_stereoPhaseDeg = std::max(0.0f, std::min(180.0f, deg));
    m_lfoPhaseR = m_lfoPhaseL + (m_stereoPhaseDeg / 360.0f);
    while (m_lfoPhaseR >= 1.0f) m_lfoPhaseR -= 1.0f;
}

void JZThroughZeroFlanger::setManualPosition(float pos) {
    m_manualPos = std::max(-1.0f, std::min(1.0f, pos));
}

void JZThroughZeroFlanger::reset() {
    for (int i = 0; i < DELAY_SIZE; ++i) {
        m_dryDelayBufL[i] = 0.0f;
        m_dryDelayBufR[i] = 0.0f;
        m_wetDelayBufL[i] = 0.0f;
        m_wetDelayBufR[i] = 0.0f;
    }
    m_writeIdx = 0;
    m_lfoPhaseL = 0.0f;
    m_lfoPhaseR = m_stereoPhaseDeg / 360.0f;
    m_feedbackStateL = 0.0f;
    m_feedbackStateR = 0.0f;
    m_currentOffsetMs = 0.0f;
}

float JZThroughZeroFlanger::fractionalDelay(const float* buffer, int bufSize, int writeIdx, float delaySamples) const {
    float readPos = static_cast<float>(writeIdx) - delaySamples;
    while (readPos < 0.0f) readPos += static_cast<float>(bufSize);

    int idx0 = static_cast<int>(readPos) % bufSize;
    int idx1 = (idx0 + 1) % bufSize;
    float frac = readPos - std::floor(readPos);

    return buffer[idx0] * (1.0f - frac) + buffer[idx1] * frac;
}

float JZThroughZeroFlanger::tapeSaturate(float in) const {
    if (m_tapeDrive <= 1.01f) return in;
    float d = m_tapeDrive;
    return std::tanh(in * d) / std::tanh(d);
}

void JZThroughZeroFlanger::process(float& left, float& right) {
    float inL = left;
    float inR = right;

    // Calculate LFO Modulation
    float modL = 0.0f;
    float modR = 0.0f;

    if (m_manualMode) {
        modL = m_manualPos;
        modR = m_manualPos;
    } else {
        modL = std::sin(m_lfoPhaseL * 2.0f * PI);
        modR = std::sin(m_lfoPhaseR * 2.0f * PI);

        float phaseInc = m_rateHz / m_sampleRate;
        m_lfoPhaseL += phaseInc;
        if (m_lfoPhaseL >= 1.0f) m_lfoPhaseL -= 1.0f;

        m_lfoPhaseR = m_lfoPhaseL + (m_stereoPhaseDeg / 360.0f);
        if (m_lfoPhaseR >= 1.0f) m_lfoPhaseR -= 1.0f;
    }

    // Relative delay offset around Through-Zero point
    m_currentOffsetMs = modL * m_depthMs;

    // Center delay in samples (the fixed reference)
    float dryDelaySamples = (m_centerDelayMs * 0.001f) * m_sampleRate;

    // Modulated wet delay in samples (sweeps through dry delay)
    float wetDelaySamplesL = ((m_centerDelayMs + modL * m_depthMs) * 0.001f) * m_sampleRate;
    float wetDelaySamplesR = ((m_centerDelayMs + modR * m_depthMs) * 0.001f) * m_sampleRate;

    // Write input into dry reference delay line
    m_dryDelayBufL[m_writeIdx] = inL;
    m_dryDelayBufR[m_writeIdx] = inR;

    // Write input + feedback into wet delay line with tape saturation
    float wetInL = tapeSaturate(inL + m_feedback * m_feedbackStateL);
    float wetInR = tapeSaturate(inR + m_feedback * m_feedbackStateR);

    m_wetDelayBufL[m_writeIdx] = wetInL;
    m_wetDelayBufR[m_writeIdx] = wetInR;

    // Read reference dry signals
    float dryOutL = fractionalDelay(m_dryDelayBufL, DELAY_SIZE, m_writeIdx, dryDelaySamples);
    float dryOutR = fractionalDelay(m_dryDelayBufR, DELAY_SIZE, m_writeIdx, dryDelaySamples);

    // Read modulated wet signals
    float wetOutL = fractionalDelay(m_wetDelayBufL, DELAY_SIZE, m_writeIdx, wetDelaySamplesL);
    float wetOutR = fractionalDelay(m_wetDelayBufR, DELAY_SIZE, m_writeIdx, wetDelaySamplesR);

    // Update feedback states
    m_feedbackStateL = wetOutL;
    m_feedbackStateR = wetOutR;

    m_writeIdx = (m_writeIdx + 1) % DELAY_SIZE;

    // Combine Dry and Wet with through-zero polarity
    float flangedL = 0.0f;
    float flangedR = 0.0f;

    if (m_polarity == FlangerPolarity::INVERTED) {
        // Complete cancellation null when wetOut == dryOut (Through-Zero Point!)
        flangedL = 0.5f * (dryOutL - wetOutL);
        flangedR = 0.5f * (dryOutR - wetOutR);
    } else {
        flangedL = 0.5f * (dryOutL + wetOutL);
        flangedR = 0.5f * (dryOutR + wetOutR);
    }

    // Mix dry (original zero latency or center delayed) and flanged
    left = (1.0f - m_mix) * inL + m_mix * flangedL * 1.414f;
    right = (1.0f - m_mix) * inR + m_mix * flangedR * 1.414f;
}

void JZThroughZeroFlanger::processBlock(float* left, float* right, int numSamples) {
    if (!left || !right || numSamples <= 0) return;
    for (int i = 0; i < numSamples; ++i) {
        process(left[i], right[i]);
    }
}

} // namespace Jazz
