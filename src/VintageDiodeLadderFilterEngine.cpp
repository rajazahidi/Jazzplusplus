//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// VintageDiodeLadderFilterEngine.cpp - Historic 18dB/24dB Diode Ladder
// Non-Linear Resonant Filter Engine
//*****************************************************************************

#include "VintageDiodeLadderFilterEngine.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Jazz {

JZVintageDiodeLadderFilterEngine::JZVintageDiodeLadderFilterEngine(float sampleRate)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
{
    initialize(m_sampleRate);
}

void JZVintageDiodeLadderFilterEngine::initialize(float sampleRate)
{
    m_sampleRate = sampleRate > 1000.0f ? sampleRate : 48000.0f;
    reset();
}

void JZVintageDiodeLadderFilterEngine::reset()
{
    m_stateL.fill(0.0f);
    m_stateR.fill(0.0f);
    m_thdPercent = 1.2f;
}

void JZVintageDiodeLadderFilterEngine::setParameters(const Parameters& params)
{
    m_params = params;
    m_params.cutoffHz = std::max(20.0f, std::min(m_params.cutoffHz, 20000.0f));
    m_params.resonance = std::max(0.0f, std::min(m_params.resonance, 1.25f));
    m_params.driveDb = std::max(-12.0f, std::min(m_params.driveDb, 24.0f));
    m_params.accentPunch = std::max(0.0f, std::min(m_params.accentPunch, 1.0f));
    m_params.bassCompensation = std::max(0.0f, std::min(m_params.bassCompensation, 1.0f));
}

void JZVintageDiodeLadderFilterEngine::setCutoffHz(float hz)
{
    m_params.cutoffHz = std::max(20.0f, std::min(hz, 20000.0f));
}

void JZVintageDiodeLadderFilterEngine::setResonance(float res)
{
    m_params.resonance = std::max(0.0f, std::min(res, 1.25f));
}

void JZVintageDiodeLadderFilterEngine::setDriveDb(float db)
{
    m_params.driveDb = std::max(-12.0f, std::min(db, 24.0f));
}

void JZVintageDiodeLadderFilterEngine::setAccentPunch(float punch)
{
    m_params.accentPunch = std::max(0.0f, std::min(punch, 1.0f));
}

void JZVintageDiodeLadderFilterEngine::setBassCompensation(float comp)
{
    m_params.bassCompensation = std::max(0.0f, std::min(comp, 1.0f));
}

void JZVintageDiodeLadderFilterEngine::setSlope(DiodeLadderSlope slope)
{
    m_params.slope = slope;
}

void JZVintageDiodeLadderFilterEngine::setBypass(bool bp)
{
    m_params.bypass = bp;
}

float JZVintageDiodeLadderFilterEngine::diodeNonLinearity(float v)
{
    // Characteristic asymmetrical Shockley diode conduction:
    // Conduction is steeper on positive swing than on negative swing
    if (v > 0.0f) {
        return std::tanh(v * 1.35f) * 0.74f;
    } else {
        return std::tanh(v * 0.85f) * 1.17f;
    }
}

void JZVintageDiodeLadderFilterEngine::process(const float* inL, const float* inR,
                                             float* outL, float* outR,
                                             size_t numSamples)
{
    if (!inL || !inR || !outL || !outR || numSamples == 0) return;

    if (m_params.bypass) {
        if (outL != inL) std::copy(inL, inL + numSamples, outL);
        if (outR != inR) std::copy(inR, inR + numSamples, outR);
        return;
    }

    float totalDriveDb = m_params.driveDb + m_params.accentPunch * 6.0f;
    float driveLinear = std::pow(10.0f, totalDriveDb / 20.0f);

    // Diode ladder tuning (normalized cutoff frequency g)
    float fc = std::max(20.0f, std::min(m_params.cutoffHz, m_sampleRate * 0.45f));
    float wd = 2.0f * static_cast<float>(M_PI) * fc;
    float T = 1.0f / m_sampleRate;
    float wa = (2.0f / T) * std::tan(wd * T * 0.5f);
    float g = wa * T * 0.5f;
    g = std::max(0.001f, std::min(g, 0.95f));

    // Acid resonance feedback amount (k factor, 17.0 gives screaming self-oscillation)
    float k = m_params.resonance * 17.5f;

    // Bass loss compensation factor
    float bassComp = m_params.bassCompensation * (m_params.resonance * 0.45f);

    for (size_t i = 0; i < numSamples; ++i) {
        // --- Channel Left ---
        float xL = inL[i] * driveLinear;
        // Negative resonance feedback with diode non-linearity
        float fbL = k * diodeNonLinearity(m_stateL[3]);
        float uL = xL - fbL;

        // 4 cascaded 1-pole diode ladder stages with inter-stage dynamic impedance
        // Stage 1
        float v0L = diodeNonLinearity(uL);
        float v1L = m_stateL[0] + g * (v0L - diodeNonLinearity(m_stateL[0]));
        m_stateL[0] = v1L + g * (v0L - diodeNonLinearity(v1L));

        // Stage 2
        float v2L = m_stateL[1] + g * (v1L - diodeNonLinearity(m_stateL[1]));
        m_stateL[1] = v2L + g * (v1L - diodeNonLinearity(v2L));

        // Stage 3 (18dB / oct output point)
        float v3L = m_stateL[2] + g * (v2L - diodeNonLinearity(m_stateL[2]));
        m_stateL[2] = v3L + g * (v2L - diodeNonLinearity(v3L));

        // Stage 4 (24dB / oct output point)
        float v4L = m_stateL[3] + g * (v3L - diodeNonLinearity(m_stateL[3]));
        m_stateL[3] = v4L + g * (v3L - diodeNonLinearity(v4L));

        float filteredL = (m_params.slope == DiodeLadderSlope::SLOPE_18DB_OCT) ? v3L : v4L;
        // Apply bass compensation
        filteredL += inL[i] * bassComp;

        // --- Channel Right ---
        float xR = inR[i] * driveLinear;
        float fbR = k * diodeNonLinearity(m_stateR[3]);
        float uR = xR - fbR;

        float v0R = diodeNonLinearity(uR);
        float v1R = m_stateR[0] + g * (v0R - diodeNonLinearity(m_stateR[0]));
        m_stateR[0] = v1R + g * (v0R - diodeNonLinearity(v1R));

        float v2R = m_stateR[1] + g * (v1R - diodeNonLinearity(m_stateR[1]));
        m_stateR[1] = v2R + g * (v1R - diodeNonLinearity(v2R));

        float v3R = m_stateR[2] + g * (v2R - diodeNonLinearity(m_stateR[2]));
        m_stateR[2] = v3R + g * (v2R - diodeNonLinearity(v3R));

        float v4R = m_stateR[3] + g * (v3R - diodeNonLinearity(m_stateR[3]));
        m_stateR[3] = v4R + g * (v3R - diodeNonLinearity(v4R));

        float filteredR = (m_params.slope == DiodeLadderSlope::SLOPE_18DB_OCT) ? v3R : v4R;
        filteredR += inR[i] * bassComp;

        // Final output soft clipping
        outL[i] = std::tanh(filteredL);
        outR[i] = std::tanh(filteredR);
    }

    m_thdPercent = 1.0f + m_params.resonance * 4.5f + (totalDriveDb > 0.0f ? totalDriveDb * 0.3f : 0.0f);
}

} // namespace Jazz
