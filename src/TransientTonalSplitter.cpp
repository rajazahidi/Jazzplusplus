#include "TransientTonalSplitter.h"
#include <algorithm>
#include <cmath>

namespace Jazz {

JZTransientTonalSplitter::JZTransientTonalSplitter(float sampleRate)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f)
    , m_transientGainDb(0.0f)
    , m_tonalGainDb(0.0f)
    , m_sensitivity(0.5f)
    , m_transientDecayMs(20.0f)
    , m_outputMode(SplitterOutputMode::COMBINED_MIX)
    , m_fastEnvL(0.0f), m_fastEnvR(0.0f)
    , m_slowEnvL(0.0f), m_slowEnvR(0.0f)
    , m_transientRms(0.0f)
    , m_tonalRms(0.0f)
    , m_currentTransWeight(0.0f)
{
    reset();
    updateBallistics();
}

void JZTransientTonalSplitter::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        m_sampleRate = sampleRate;
        updateBallistics();
    }
}

float JZTransientTonalSplitter::dbToLinear(float db) const {
    return std::pow(10.0f, db / 20.0f);
}

void JZTransientTonalSplitter::setTransientGainDb(float gainDb) {
    m_transientGainDb = std::max(-24.0f, std::min(12.0f, gainDb));
}

void JZTransientTonalSplitter::setTonalGainDb(float gainDb) {
    m_tonalGainDb = std::max(-24.0f, std::min(12.0f, gainDb));
}

void JZTransientTonalSplitter::setSensitivity(float sensitivity) {
    m_sensitivity = std::max(0.0f, std::min(1.0f, sensitivity));
}

void JZTransientTonalSplitter::setTransientDecayMs(float decayMs) {
    m_transientDecayMs = std::max(5.0f, std::min(100.0f, decayMs));
    updateBallistics();
}

void JZTransientTonalSplitter::updateBallistics() {
    // Fast tracker: 0.5 ms attack, user-specified decay release
    float fastAttackMs = 0.5f;
    float fastReleaseMs = m_transientDecayMs;

    m_fastAttackCoeff = std::exp(-1.0f / (fastAttackMs * 0.001f * m_sampleRate));
    m_fastReleaseCoeff = std::exp(-1.0f / (fastReleaseMs * 0.001f * m_sampleRate));

    // Slow tracker: 15.0 ms attack, 80.0 ms release
    float slowAttackMs = 15.0f;
    float slowReleaseMs = 80.0f;

    m_slowAttackCoeff = std::exp(-1.0f / (slowAttackMs * 0.001f * m_sampleRate));
    m_slowReleaseCoeff = std::exp(-1.0f / (slowReleaseMs * 0.001f * m_sampleRate));
}

void JZTransientTonalSplitter::reset() {
    m_fastEnvL = m_fastEnvR = 0.0f;
    m_slowEnvL = m_slowEnvR = 0.0f;
    m_transientRms = 0.0f;
    m_tonalRms = 0.0f;
    m_currentTransWeight = 0.0f;
}

void JZTransientTonalSplitter::splitExplicit(float inL, float inR,
                                            float& outTransL, float& outTransR,
                                            float& outTonalL, float& outTonalR) {
    float absL = std::abs(inL);
    float absR = std::abs(inR);

    // Fast envelope tracker
    if (absL > m_fastEnvL) m_fastEnvL = m_fastAttackCoeff * m_fastEnvL + (1.0f - m_fastAttackCoeff) * absL;
    else                  m_fastEnvL = m_fastReleaseCoeff * m_fastEnvL + (1.0f - m_fastReleaseCoeff) * absL;

    if (absR > m_fastEnvR) m_fastEnvR = m_fastAttackCoeff * m_fastEnvR + (1.0f - m_fastAttackCoeff) * absR;
    else                  m_fastEnvR = m_fastReleaseCoeff * m_fastEnvR + (1.0f - m_fastReleaseCoeff) * absR;

    // Slow envelope tracker
    if (absL > m_slowEnvL) m_slowEnvL = m_slowAttackCoeff * m_slowEnvL + (1.0f - m_slowAttackCoeff) * absL;
    else                  m_slowEnvL = m_slowReleaseCoeff * m_slowEnvL + (1.0f - m_slowReleaseCoeff) * absL;

    if (absR > m_slowEnvR) m_slowEnvR = m_slowAttackCoeff * m_slowEnvR + (1.0f - m_slowAttackCoeff) * absR;
    else                  m_slowEnvR = m_slowReleaseCoeff * m_slowEnvR + (1.0f - m_slowReleaseCoeff) * absR;

    // Transient flux delta calculation
    float deltaL = std::max(0.0f, m_fastEnvL - m_slowEnvL);
    float deltaR = std::max(0.0f, m_fastEnvR - m_slowEnvR);

    float scale = 1.0f + m_sensitivity * 1.5f;

    float wL = (deltaL / (m_fastEnvL + 1e-4f)) * scale;
    float wR = (deltaR / (m_fastEnvR + 1e-4f)) * scale;

    wL = std::max(0.0f, std::min(1.0f, wL));
    wR = std::max(0.0f, std::min(1.0f, wR));

    m_currentTransWeight = 0.5f * (wL + wR);

    // Lossless complementary separation
    outTransL = inL * wL;
    outTransR = inR * wR;

    outTonalL = inL * (1.0f - wL);
    outTonalR = inR * (1.0f - wR);

    // RMS telemetry
    float transE = 0.5f * (outTransL * outTransL + outTransR * outTransR);
    float tonalE = 0.5f * (outTonalL * outTonalL + outTonalR * outTonalR);
    m_transientRms = 0.95f * m_transientRms + 0.05f * std::sqrt(transE);
    m_tonalRms = 0.95f * m_tonalRms + 0.05f * std::sqrt(tonalE);
}

void JZTransientTonalSplitter::process(float& left, float& right) {
    float transL = 0.0f, transR = 0.0f;
    float tonalL = 0.0f, tonalR = 0.0f;

    splitExplicit(left, right, transL, transR, tonalL, tonalR);

    float gTrans = dbToLinear(m_transientGainDb);
    float gTonal = dbToLinear(m_tonalGainDb);

    switch (m_outputMode) {
        case SplitterOutputMode::COMBINED_MIX:
            left  = transL * gTrans + tonalL * gTonal;
            right = transR * gTrans + tonalR * gTonal;
            break;

        case SplitterOutputMode::SOLO_TRANSIENT:
            left  = transL * gTrans;
            right = transR * gTrans;
            break;

        case SplitterOutputMode::SOLO_TONAL:
            left  = tonalL * gTonal;
            right = tonalR * gTonal;
            break;
    }
}

void JZTransientTonalSplitter::processBlock(float* left, float* right, int numSamples) {
    if (!left || !right || numSamples <= 0) return;
    for (int i = 0; i < numSamples; ++i) {
        process(left[i], right[i]);
    }
}

} // namespace Jazz
