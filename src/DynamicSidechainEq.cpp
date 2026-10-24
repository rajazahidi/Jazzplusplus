#include "DynamicSidechainEq.h"
#include <algorithm>
#include <cmath>

namespace Jazz {

static const float PI = 3.14159265358979323846f;

JZDynamicSidechainEq::JZDynamicSidechainEq(float sampleRate)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f)
    , m_mode(SidechainCarveMode::SPECTRAL_MULTI_BAND)
    , m_amountPercent(50.0f)
    , m_lowCutHz(30.0f)
    , m_highCutHz(16000.0f)
    , m_attackMs(5.0f)
    , m_releaseMs(60.0f)
    , m_targetFreqHz(60.0f)
    , m_targetQ(1.5f)
    , m_attackCoeff(0.95f)
    , m_releaseCoeff(0.999f)
    , m_currentMaxReductionDb(0.0f)
{
    m_bandCenterFreqs[0] = 60.0f;   // Sub punch
    m_bandCenterFreqs[1] = 180.0f;  // Bass warmth
    m_bandCenterFreqs[2] = 600.0f;  // Low-mid boxiness
    m_bandCenterFreqs[3] = 2500.0f; // High-mid vocal bite
    m_bandCenterFreqs[4] = 8000.0f; // Air & sibilance

    for (int i = 0; i < NUM_BANDS; ++i) {
        m_sidechainEnvelopes[i] = 0.0f;
        m_bandReductionsDb[i] = 0.0f;
    }

    reset();
    setAttackMs(5.0f);
    setReleaseMs(60.0f);
    updateFilterCoeffs();
}

void JZDynamicSidechainEq::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        m_sampleRate = sampleRate;
        setAttackMs(m_attackMs);
        setReleaseMs(m_releaseMs);
        updateFilterCoeffs();
    }
}

void JZDynamicSidechainEq::setAmountPercent(float amount) {
    m_amountPercent = std::max(0.0f, std::min(100.0f, amount));
}

void JZDynamicSidechainEq::setSidechainBandLimits(float lowCutHz, float highCutHz) {
    m_lowCutHz = std::max(10.0f, std::min(1000.0f, lowCutHz));
    m_highCutHz = std::max(1000.0f, std::min(22000.0f, highCutHz));
    updateFilterCoeffs();
}

void JZDynamicSidechainEq::setAttackMs(float attackMs) {
    m_attackMs = std::max(0.2f, std::min(100.0f, attackMs));
    m_attackCoeff = std::exp(-1.0f / (m_attackMs * 0.001f * m_sampleRate));
}

void JZDynamicSidechainEq::setReleaseMs(float releaseMs) {
    m_releaseMs = std::max(5.0f, std::min(1000.0f, releaseMs));
    m_releaseCoeff = std::exp(-1.0f / (m_releaseMs * 0.001f * m_sampleRate));
}

void JZDynamicSidechainEq::setTargetBand(float centerFreqHz, float Q) {
    m_targetFreqHz = std::max(20.0f, std::min(20000.0f, centerFreqHz));
    m_targetQ = std::max(0.5f, std::min(10.0f, Q));
    updateFilterCoeffs();
}

float JZDynamicSidechainEq::getBandReductionDb(int bandIdx) const {
    if (bandIdx >= 0 && bandIdx < NUM_BANDS) {
        return m_bandReductionsDb[bandIdx];
    }
    return 0.0f;
}

void JZDynamicSidechainEq::reset() {
    for (int b = 0; b < NUM_BANDS; ++b) {
        m_sidechainEnvelopes[b] = 0.0f;
        m_bandReductionsDb[b] = 0.0f;
        m_scBpStateL[b][0] = m_scBpStateL[b][1] = 0.0f;
        m_scBpStateR[b][0] = m_scBpStateR[b][1] = 0.0f;
        m_mainEqStateL[b][0] = m_mainEqStateL[b][1] = 0.0f;
        m_mainEqStateR[b][0] = m_mainEqStateR[b][1] = 0.0f;
    }
    m_scHpState[0] = m_scHpState[1] = 0.0f;
    m_scLpState[0] = m_scLpState[1] = 0.0f;
    m_currentMaxReductionDb = 0.0f;
}

void JZDynamicSidechainEq::updateFilterCoeffs() {
    // Design sidechain bandpass analysis filters for each of the 5 bands (Q = 1.8)
    for (int b = 0; b < NUM_BANDS; ++b) {
        float f0 = m_bandCenterFreqs[b];
        if (m_mode == SidechainCarveMode::PARAMETRIC_TARGET && b == 0) {
            f0 = m_targetFreqHz;
        }

        float w0 = 2.0f * PI * f0 / m_sampleRate;
        float alpha = std::sin(w0) / (2.0f * 1.8f);

        float a0 = 1.0f + alpha;
        m_scBpB0[b] = alpha / a0;
        m_scBpB1[b] = 0.0f;
        m_scBpB2[b] = -alpha / a0;
        m_scBpA1[b] = (-2.0f * std::cos(w0)) / a0;
        m_scBpA2[b] = (1.0f - alpha) / a0;
    }

    // Sidechain Low-Cut (High-pass 2nd-order Butterworth at m_lowCutHz)
    float wHp = 2.0f * PI * m_lowCutHz / m_sampleRate;
    float alphaHp = std::sin(wHp) / (2.0f * 0.707f);
    float a0Hp = 1.0f + alphaHp;
    m_scHpB0 = (1.0f + std::cos(wHp)) * 0.5f / a0Hp;
    m_scHpB1 = -(1.0f + std::cos(wHp)) / a0Hp;
    m_scHpB2 = (1.0f + std::cos(wHp)) * 0.5f / a0Hp;
    m_scHpA1 = (-2.0f * std::cos(wHp)) / a0Hp;
    m_scHpA2 = (1.0f - alphaHp) / a0Hp;

    // Sidechain High-Cut (Low-pass 2nd-order Butterworth at m_highCutHz)
    float wLp = 2.0f * PI * m_highCutHz / m_sampleRate;
    float alphaLp = std::sin(wLp) / (2.0f * 0.707f);
    float a0Lp = 1.0f + alphaLp;
    m_scLpB0 = (1.0f - std::cos(wLp)) * 0.5f / a0Lp;
    m_scLpB1 = (1.0f - std::cos(wLp)) / a0Lp;
    m_scLpB2 = (1.0f - std::cos(wLp)) * 0.5f / a0Lp;
    m_scLpA1 = (-2.0f * std::cos(wLp)) / a0Lp;
    m_scLpA2 = (1.0f - alphaLp) / a0Lp;
}

void JZDynamicSidechainEq::process(float& mainL, float& mainR, float sidechainL, float sidechainR) {
    // 1. Band-limit the sidechain signal
    float scMono = 0.5f * (sidechainL + sidechainR);

    // Apply High-Pass LowCut
    float hpOut = m_scHpB0 * scMono + m_scHpState[0];
    m_scHpState[0] = m_scHpB1 * scMono - m_scHpA1 * hpOut + m_scHpState[1];
    m_scHpState[1] = m_scHpB2 * scMono - m_scHpA2 * hpOut;

    // Apply Low-Pass HighCut
    float lpOut = m_scLpB0 * hpOut + m_scLpState[0];
    m_scLpState[0] = m_scLpB1 * hpOut - m_scLpA1 * lpOut + m_scLpState[1];
    m_scLpState[1] = m_scLpB2 * hpOut - m_scLpA2 * lpOut;

    float cleanSc = lpOut;

    // 2. Analyze spectral bands on sidechain and apply dynamic notches on main audio
    m_currentMaxReductionDb = 0.0f;
    int numActiveBands = (m_mode == SidechainCarveMode::PARAMETRIC_TARGET) ? 1 : NUM_BANDS;

    for (int b = 0; b < numActiveBands; ++b) {
        // Bandpass filter on sidechain
        float bpOut = m_scBpB0[b] * cleanSc + m_scBpStateL[b][0];
        m_scBpStateL[b][0] = m_scBpB1[b] * cleanSc - m_scBpA1[b] * bpOut + m_scBpStateL[b][1];
        m_scBpStateL[b][1] = m_scBpB2[b] * cleanSc - m_scBpA2[b] * bpOut;

        float bandMag = std::abs(bpOut);

        // Ballistics Envelope Follower
        if (bandMag > m_sidechainEnvelopes[b]) {
            m_sidechainEnvelopes[b] = m_attackCoeff * m_sidechainEnvelopes[b] + (1.0f - m_attackCoeff) * bandMag;
        } else {
            m_sidechainEnvelopes[b] = m_releaseCoeff * m_sidechainEnvelopes[b] + (1.0f - m_releaseCoeff) * bandMag;
        }

        // Calculate dynamic cut depth in dB (proportional to envelope and amount)
        float cutDb = - (m_sidechainEnvelopes[b] * (m_amountPercent * 0.35f));
        cutDb = std::max(-18.0f, std::min(0.0f, cutDb));
        m_bandReductionsDb[b] = cutDb;

        if (std::abs(cutDb) > std::abs(m_currentMaxReductionDb)) {
            m_currentMaxReductionDb = cutDb;
        }

        // Dynamic Peaking EQ Notch Filter on Main audio
        if (std::abs(cutDb) > 0.05f) {
            float f0 = (m_mode == SidechainCarveMode::PARAMETRIC_TARGET) ? m_targetFreqHz : m_bandCenterFreqs[b];
            float Q = (m_mode == SidechainCarveMode::PARAMETRIC_TARGET) ? m_targetQ : 1.8f;

            float w0 = 2.0f * PI * f0 / m_sampleRate;
            float alpha = std::sin(w0) / (2.0f * Q);
            float A = std::pow(10.0f, cutDb / 40.0f);
            float cosw0 = std::cos(w0);

            float a0 = 1.0f + alpha / A;
            float b0 = (1.0f + alpha * A) / a0;
            float b1 = (-2.0f * cosw0) / a0;
            float b2 = (1.0f - alpha * A) / a0;
            float a1 = (-2.0f * cosw0) / a0;
            float a2 = (1.0f - alpha / A) / a0;

            // Filter Left
            float outL = b0 * mainL + m_mainEqStateL[b][0];
            m_mainEqStateL[b][0] = b1 * mainL - a1 * outL + m_mainEqStateL[b][1];
            m_mainEqStateL[b][1] = b2 * mainL - a2 * outL;
            mainL = outL;

            // Filter Right
            float outR = b0 * mainR + m_mainEqStateR[b][0];
            m_mainEqStateR[b][0] = b1 * mainR - a1 * outR + m_mainEqStateR[b][1];
            m_mainEqStateR[b][1] = b2 * mainR - a2 * outR;
            mainR = outR;
        }
    }
}

void JZDynamicSidechainEq::processBlock(float* mainL, float* mainR, const float* scL, const float* scR, int numSamples) {
    if (!mainL || !mainR || !scL || !scR || numSamples <= 0) return;
    for (int i = 0; i < numSamples; ++i) {
        process(mainL[i], mainR[i], scL[i], scR[i]);
    }
}

} // namespace Jazz
