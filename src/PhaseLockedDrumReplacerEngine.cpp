//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// PhaseLockedDrumReplacerEngine.cpp - Phase-Locked Acoustic Drum Replacer
//*****************************************************************************

#include "PhaseLockedDrumReplacerEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

static constexpr float PI_F = 3.14159265358979323846f;

JZPhaseLockedDrumReplacerEngine::JZPhaseLockedDrumReplacerEngine(float sampleRate)
    : m_sampleRate(sampleRate)
{
    initialize(sampleRate);
}

void JZPhaseLockedDrumReplacerEngine::initialize(float sampleRate)
{
    m_sampleRate = (sampleRate > 8000.0f) ? sampleRate : 48000.0f;
    m_inputHistory.assign(CORR_BUF_SIZE, 0.0f);
    m_historyPos = 0;

    generateInternalSamplePool();
    reset();
    updateDetectionFilters();
}

void JZPhaseLockedDrumReplacerEngine::reset()
{
    m_bpX1L = m_bpX2L = m_bpY1L = m_bpY2L = 0.0f;
    m_envelope = 0.0f;
    m_holdoffCounter = 0;
    m_sampleActive = false;
    m_samplePlayPos = 0;
    m_currentHitGain = 0.0f;
    m_currentHitHardness = 0.0f;
    m_phaseOffsetSamples = 0.0f;
    m_hitsCount = 0;
    m_lastVelocity = 0.0f;
    std::fill(m_inputHistory.begin(), m_inputHistory.end(), 0.0f);
    m_historyPos = 0;
}

void JZPhaseLockedDrumReplacerEngine::generateInternalSamplePool()
{
    // Generate 0.35s acoustic snare waveforms (16,800 samples at 48kHz)
    size_t numSamples = static_cast<size_t>(m_sampleRate * 0.35f);
    m_sampleSoft.assign(numSamples, 0.0f);
    m_sampleHard.assign(numSamples, 0.0f);

    float f0 = (m_params.instrument == DrumReplacerInstrument::KICK) ? 60.0f : 185.0f;

    for (size_t i = 0; i < numSamples; ++i) {
        float t = static_cast<float>(i) / m_sampleRate;
        float decayBody = std::exp(-t * 22.0f);
        float decaySnare = std::exp(-t * 16.0f);

        // Pitch drop envelope on strike
        float pitchEnv = std::exp(-t * 60.0f);
        float freq = f0 * (1.0f + 1.2f * pitchEnv);
        float phase = 2.0f * PI_F * freq * t;

        float sineBody = std::sin(phase) * decayBody;

        // Pseudo-random noise for snare wires
        float noise = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f) * decaySnare;

        // Soft layer: dominant body resonance, softer top crack
        m_sampleSoft[i] = sineBody * 0.85f + noise * 0.25f;

        // Hard layer: sharp initial strike transient, bright snare wire crack
        float strikeClick = (i < static_cast<size_t>(m_sampleRate * 0.005f))
                            ? std::sin(2.0f * PI_F * 4500.0f * t) * std::exp(-t * 400.0f) : 0.0f;
        m_sampleHard[i] = sineBody * 0.65f + noise * 0.65f + strikeClick * 0.75f;
    }
}

void JZPhaseLockedDrumReplacerEngine::updateDetectionFilters()
{
    // Bandpass filter to isolate fundamental body strike
    float centerHz = 190.0f;
    float q = 1.3f;
    if (m_params.instrument == DrumReplacerInstrument::KICK) {
        centerHz = 65.0f;
        q = 1.6f;
    } else if (m_params.instrument == DrumReplacerInstrument::TOM) {
        centerHz = 120.0f;
        q = 1.4f;
    }

    float w0 = 2.0f * PI_F * centerHz / m_sampleRate;
    float sinW0 = std::sin(w0);
    float cosW0 = std::cos(w0);
    float alpha = sinW0 / (2.0f * q);

    float b0 = alpha;
    float b1 = 0.0f;
    float b2 = -alpha;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cosW0;
    float a2 = 1.0f - alpha;

    m_bpB0 = b0 / a0;
    m_bpB1 = b1 / a0;
    m_bpB2 = b2 / a0;
    m_bpA1 = a1 / a0;
    m_bpA2 = a2 / a0;

    m_holdoffSamples = static_cast<size_t>((m_params.retriggerHoldoffMs * 0.001f) * m_sampleRate);
}

void JZPhaseLockedDrumReplacerEngine::setParameters(const Parameters& params)
{
    m_params = params;
    m_params.triggerThresholdDb = std::max(-40.0f, std::min(m_params.triggerThresholdDb, -6.0f));
    m_params.sensitivity = std::max(0.1f, std::min(m_params.sensitivity, 1.0f));
    m_params.retriggerHoldoffMs = std::max(10.0f, std::min(m_params.retriggerHoldoffMs, 150.0f));
    m_params.dryWet = std::max(0.0f, std::min(m_params.dryWet, 1.0f));

    updateDetectionFilters();
}

void JZPhaseLockedDrumReplacerEngine::setInstrument(DrumReplacerInstrument inst)
{
    m_params.instrument = inst;
    generateInternalSamplePool();
    updateDetectionFilters();
}

void JZPhaseLockedDrumReplacerEngine::setTriggerThresholdDb(float db)
{
    m_params.triggerThresholdDb = std::max(-40.0f, std::min(db, -6.0f));
}

void JZPhaseLockedDrumReplacerEngine::setSensitivity(float sens)
{
    m_params.sensitivity = std::max(0.1f, std::min(sens, 1.0f));
}

void JZPhaseLockedDrumReplacerEngine::setRetriggerHoldoffMs(float ms)
{
    m_params.retriggerHoldoffMs = std::max(10.0f, std::min(ms, 150.0f));
    m_holdoffSamples = static_cast<size_t>((m_params.retriggerHoldoffMs * 0.001f) * m_sampleRate);
}

void JZPhaseLockedDrumReplacerEngine::setPhaseLockEnabled(bool enable)
{
    m_params.enablePhaseLock = enable;
}

void JZPhaseLockedDrumReplacerEngine::setInvertPolarity(bool invert)
{
    m_params.invertPolarity = invert;
}

void JZPhaseLockedDrumReplacerEngine::setDryWet(float wet)
{
    m_params.dryWet = std::max(0.0f, std::min(wet, 1.0f));
}

void JZPhaseLockedDrumReplacerEngine::setBypass(bool bypass)
{
    m_params.bypassed = bypass;
}

void JZPhaseLockedDrumReplacerEngine::triggerHit(float velocity)
{
    m_sampleActive = true;
    m_samplePlayPos = 0;
    m_currentHitHardness = std::max(0.0f, std::min(velocity, 1.0f));
    m_currentHitGain = 0.3f + 0.7f * m_currentHitHardness;
    m_lastVelocity = m_currentHitHardness;
    m_hitsCount++;
    m_holdoffCounter = m_holdoffSamples;
}

void JZPhaseLockedDrumReplacerEngine::process(
    const float* inL, const float* inR,
    float* outL, float* outR,
    size_t numSamples)
{
    if (!inL || !inR || !outL || !outR || numSamples == 0) return;

    if (m_params.bypassed) {
        std::copy(inL, inL + numSamples, outL);
        std::copy(inR, inR + numSamples, outR);
        return;
    }

    float threshLin = std::pow(10.0f, m_params.triggerThresholdDb / 20.0f);
    float attackCoeff = 1.0f - std::exp(-1000.0f / (1.0f * m_sampleRate));   // 1ms fast attack
    float releaseCoeff = 1.0f - std::exp(-1000.0f / (30.0f * m_sampleRate)); // 30ms release

    for (size_t i = 0; i < numSamples; ++i) {
        float xL = inL[i];
        float xR = inR[i];
        float monoIn = (xL + xR) * 0.5f;

        // Store into circular input history buffer for phase cross-correlation
        m_inputHistory[m_historyPos] = monoIn;
        m_historyPos = (m_historyPos + 1) % CORR_BUF_SIZE;

        // 1. Bandpass filter incoming mic to isolate acoustic strike
        float bp = m_bpB0 * monoIn + m_bpB1 * m_bpX1L + m_bpB2 * m_bpX2L - m_bpA1 * m_bpY1L - m_bpA2 * m_bpY2L;
        m_bpX2L = m_bpX1L; m_bpX1L = monoIn;
        m_bpY2L = m_bpY1L; m_bpY1L = bp;

        float absBp = std::abs(bp) * m_params.sensitivity;
        if (absBp > m_envelope) {
            m_envelope += attackCoeff * (absBp - m_envelope);
        } else {
            m_envelope += releaseCoeff * (absBp - m_envelope);
        }

        // 2. Transient hit detection with retrigger holdoff
        if (m_holdoffCounter > 0) {
            m_holdoffCounter--;
        } else if (m_envelope > threshLin) {
            // Trigger new drum sample
            float normVel = std::min(1.0f, (m_envelope - threshLin) / (1.0f - threshLin) + 0.35f);

            // Sub-sample cross-correlation phase lock
            if (m_params.enablePhaseLock) {
                float bestCorr = -1e9f;
                int bestLag = 0;
                for (int lag = -8; lag <= 8; ++lag) {
                    float corr = 0.0f;
                    for (int k = 0; k < 24; ++k) {
                        int hIdx = static_cast<int>(m_historyPos) - 24 - lag + k;
                        while (hIdx < 0) hIdx += CORR_BUF_SIZE;
                        hIdx %= CORR_BUF_SIZE;
                        float smpVal = (k < static_cast<int>(m_sampleHard.size())) ? m_sampleHard[k] : 0.0f;
                        corr += m_inputHistory[hIdx] * smpVal;
                    }
                    if (corr > bestCorr) {
                        bestCorr = corr;
                        bestLag = lag;
                    }
                }
                m_phaseOffsetSamples = static_cast<float>(bestLag);
            } else {
                m_phaseOffsetSamples = 0.0f;
            }

            triggerHit(normVel);
        }

        // 3. Render replacement drum sample
        float sampleVal = 0.0f;
        if (m_sampleActive && m_samplePlayPos < m_sampleSoft.size()) {
            float sSoft = m_sampleSoft[m_samplePlayPos];
            float sHard = m_sampleHard[m_samplePlayPos];
            sampleVal = (sSoft * (1.0f - m_currentHitHardness) + sHard * m_currentHitHardness) * m_currentHitGain;

            if (m_params.invertPolarity) {
                sampleVal = -sampleVal;
            }

            m_samplePlayPos++;
            if (m_samplePlayPos >= m_sampleSoft.size()) {
                m_sampleActive = false;
            }
        }

        // 4. Parallel Dry/Wet blend
        outL[i] = xL * (1.0f - m_params.dryWet) + sampleVal * m_params.dryWet;
        outR[i] = xR * (1.0f - m_params.dryWet) + sampleVal * m_params.dryWet;
    }
}

} // namespace Jazz
