//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// DynamicSubBassSpectralCarverEngine.cpp - Dynamic Kick-to-Bass Fundamental
// Frequency Surgical Sidechain Carver
//*****************************************************************************

#include "DynamicSubBassSpectralCarverEngine.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Jazz {

JZDynamicSubBassSpectralCarverEngine::JZDynamicSubBassSpectralCarverEngine(float sampleRate)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
{
    initialize(m_sampleRate);
}

void JZDynamicSubBassSpectralCarverEngine::initialize(float sampleRate)
{
    m_sampleRate = sampleRate > 1000.0f ? sampleRate : 48000.0f;
    reset();
}

void JZDynamicSubBassSpectralCarverEngine::reset()
{
    m_trackedKickFundamentalHz = 55.0f;
    m_currentCarveDepthDb = 0.0f;
    m_kickTriggerActive = false;
    m_kickEnvelope = 0.0f;
    m_prevSideSample = 0.0f;
    m_zeroCrossingPeriod = 0.0f;

    m_carveX1L = m_carveX2L = m_carveY1L = m_carveY2L = 0.0f;
    m_carveX1R = m_carveX2R = m_carveY1R = m_carveY2R = 0.0f;
    updateCarveFilter(55.0f, 0.0f);
}

void JZDynamicSubBassSpectralCarverEngine::setParameters(const Parameters& params)
{
    m_params = params;
    m_params.carveDepthDb = std::max(-36.0f, std::min(m_params.carveDepthDb, 0.0f));
    m_params.carveQ = std::max(1.0f, std::min(m_params.carveQ, 15.0f));
    m_params.sensitivity = std::max(0.1f, std::min(m_params.sensitivity, 5.0f));
    m_params.attackMs = std::max(0.2f, std::min(m_params.attackMs, 20.0f));
    m_params.releaseMs = std::max(10.0f, std::min(m_params.releaseMs, 500.0f));
}

void JZDynamicSubBassSpectralCarverEngine::setCarveDepthDb(float db)
{
    m_params.carveDepthDb = std::max(-36.0f, std::min(db, 0.0f));
}

void JZDynamicSubBassSpectralCarverEngine::setCarveQ(float q)
{
    m_params.carveQ = std::max(1.0f, std::min(q, 15.0f));
}

void JZDynamicSubBassSpectralCarverEngine::setSensitivity(float sens)
{
    m_params.sensitivity = std::max(0.1f, std::min(sens, 5.0f));
}

void JZDynamicSubBassSpectralCarverEngine::setReleaseMs(float ms)
{
    m_params.releaseMs = std::max(10.0f, std::min(ms, 500.0f));
}

void JZDynamicSubBassSpectralCarverEngine::setCarveDeltaSolo(bool solo)
{
    m_params.carveDeltaSolo = solo;
}

void JZDynamicSubBassSpectralCarverEngine::setBypass(bool bp)
{
    m_params.bypass = bp;
}

void JZDynamicSubBassSpectralCarverEngine::updateCarveFilter(float fundamentalHz, float depthDb)
{
    float fc = std::max(m_params.minCarveFreqHz, std::min(fundamentalHz, m_params.maxCarveFreqHz));
    float A = std::pow(10.0f, depthDb / 40.0f); // Peaking dip gain
    float w0 = 2.0f * static_cast<float>(M_PI) * fc / m_sampleRate;
    float alpha = std::sin(w0) / (2.0f * m_params.carveQ);
    float cosw0 = std::cos(w0);

    float a0 = 1.0f + alpha / A;
    m_carveB0 = (1.0f + alpha * A) / a0;
    m_carveB1 = (-2.0f * cosw0) / a0;
    m_carveB2 = (1.0f - alpha * A) / a0;
    m_carveA1 = (-2.0f * cosw0) / a0;
    m_carveA2 = (1.0f - alpha / A) / a0;
}

void JZDynamicSubBassSpectralCarverEngine::processWithSidechain(
    const float* inBassL, const float* inBassR,
    const float* sideKickL, const float* sideKickR,
    float* outBassL, float* outBassR,
    size_t numSamples)
{
    if (!inBassL || !inBassR || !outBassL || !outBassR || numSamples == 0) return;

    if (m_params.bypass) {
        if (outBassL != inBassL) std::copy(inBassL, inBassL + numSamples, outBassL);
        if (outBassR != inBassR) std::copy(inBassR, inBassR + numSamples, outBassR);
        return;
    }

    const float* sideL = sideKickL ? sideKickL : inBassL;
    const float* sideR = sideKickR ? sideKickR : inBassR;

    float attAlpha = std::exp(-1.0f / (m_sampleRate * (m_params.attackMs * 0.001f)));
    float relAlpha = std::exp(-1.0f / (m_sampleRate * (m_params.releaseMs * 0.001f)));

    for (size_t i = 0; i < numSamples; ++i) {
        float kickSample = (sideL[i] + sideR[i]) * 0.5f;
        float kickMag = std::abs(kickSample) * m_params.sensitivity;

        // 1. Envelope follower for kick drum transient punch
        if (kickMag > m_kickEnvelope) {
            m_kickEnvelope = attAlpha * m_kickEnvelope + (1.0f - attAlpha) * kickMag;
        } else {
            m_kickEnvelope = relAlpha * m_kickEnvelope + (1.0f - relAlpha) * kickMag;
        }

        m_kickTriggerActive = (m_kickEnvelope > 0.08f);

        // 2. Real-time zero-crossing pitch tracker for kick fundamental
        if ((m_prevSideSample <= 0.0f && kickSample > 0.0f) || (m_prevSideSample >= 0.0f && kickSample < 0.0f)) {
            if (m_zeroCrossingPeriod > 5.0f) {
                float detectedHz = m_sampleRate / (m_zeroCrossingPeriod * 2.0f);
                if (detectedHz >= m_params.minCarveFreqHz && detectedHz <= m_params.maxCarveFreqHz) {
                    m_trackedKickFundamentalHz = 0.9f * m_trackedKickFundamentalHz + 0.1f * detectedHz;
                }
            }
            m_zeroCrossingPeriod = 0.0f;
        } else {
            m_zeroCrossingPeriod += 1.0f;
        }
        m_prevSideSample = kickSample;

        // 3. Calculate dynamic surgical notch carve depth
        float activeDepthDb = 0.0f;
        if (m_kickTriggerActive) {
            float carveRatio = std::min(1.0f, m_kickEnvelope * 1.5f);
            activeDepthDb = carveRatio * m_params.carveDepthDb;
        }
        m_currentCarveDepthDb = activeDepthDb;

        // Update filter coefficients smoothly
        if ((i & 15) == 0) {
            updateCarveFilter(m_trackedKickFundamentalHz, m_currentCarveDepthDb);
        }

        // 4. Apply dynamic carve notch to bass track
        float bL = inBassL[i];
        float bR = inBassR[i];

        float carvedL = m_carveB0 * bL + m_carveB1 * m_carveX1L + m_carveB2 * m_carveX2L - m_carveA1 * m_carveY1L - m_carveA2 * m_carveY2L;
        m_carveX2L = m_carveX1L; m_carveX1L = bL; m_carveY2L = m_carveY1L; m_carveY1L = carvedL;

        float carvedR = m_carveB0 * bR + m_carveB1 * m_carveX1R + m_carveB2 * m_carveX2R - m_carveA1 * m_carveY1R - m_carveA2 * m_carveY2R;
        m_carveX2R = m_carveX1R; m_carveX1R = bR; m_carveY2R = m_carveY1R; m_carveY1R = carvedR;

        if (m_params.carveDeltaSolo) {
            outBassL[i] = bL - carvedL;
            outBassR[i] = bR - carvedR;
        } else {
            outBassL[i] = carvedL;
            outBassR[i] = carvedR;
        }
    }
}

void JZDynamicSubBassSpectralCarverEngine::process(const float* inL, const float* inR,
                                                 float* outL, float* outR,
                                                 size_t numSamples)
{
    processWithSidechain(inL, inR, inL, inR, outL, outR, numSamples);
}

} // namespace Jazz
