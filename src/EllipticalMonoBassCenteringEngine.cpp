//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// EllipticalMonoBassCenteringEngine.cpp - Elliptical Mono Bass Centering Matrix
//*****************************************************************************

#include "EllipticalMonoBassCenteringEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

static constexpr float PI_F = 3.14159265358979323846f;
static constexpr float SQRT2_INV = 0.7071067811865475f;

JZEllipticalMonoBassCenteringEngine::JZEllipticalMonoBassCenteringEngine(float sampleRate)
    : m_sampleRate(sampleRate)
{
    initialize(sampleRate);
}

void JZEllipticalMonoBassCenteringEngine::initialize(float sampleRate)
{
    m_sampleRate = (sampleRate > 8000.0f) ? sampleRate : 48000.0f;
    reset();
    updateFilters();
}

void JZEllipticalMonoBassCenteringEngine::reset()
{
    m_hpX1_1 = m_hpX2_1 = m_hpY1_1 = m_hpY2_1 = 0.0f;
    m_hpX1_2 = m_hpX2_2 = m_hpY1_2 = m_hpY2_2 = 0.0f;
    m_filteredSideEnergyDb = -60.0f;
    m_phaseCorrelation = 1.0f;
}

void JZEllipticalMonoBassCenteringEngine::setParameters(const Parameters& params)
{
    m_params = params;
    m_params.monoCutoffHz = std::max(40.0f, std::min(m_params.monoCutoffHz, 350.0f));
    m_params.stereoWidth = std::max(0.0f, std::min(m_params.stereoWidth, 2.0f));
    m_params.dryWet = std::max(0.0f, std::min(m_params.dryWet, 1.0f));

    updateFilters();
}

void JZEllipticalMonoBassCenteringEngine::setMonoCutoffHz(float hz)
{
    m_params.monoCutoffHz = std::max(40.0f, std::min(hz, 350.0f));
    updateFilters();
}

void JZEllipticalMonoBassCenteringEngine::setFilterSlope(EllipticalFilterSlope slope)
{
    m_params.filterSlope = slope;
    updateFilters();
}

void JZEllipticalMonoBassCenteringEngine::setStereoWidth(float width)
{
    m_params.stereoWidth = std::max(0.0f, std::min(width, 2.0f));
}

void JZEllipticalMonoBassCenteringEngine::setRedirectSubEnergyToMid(bool redirect)
{
    m_params.redirectSubEnergyToMid = redirect;
}

void JZEllipticalMonoBassCenteringEngine::setAuditionSideSub(bool audition)
{
    m_params.auditionSideSub = audition;
}

void JZEllipticalMonoBassCenteringEngine::setDryWet(float wet)
{
    m_params.dryWet = std::max(0.0f, std::min(wet, 1.0f));
}

void JZEllipticalMonoBassCenteringEngine::setBypass(bool bypass)
{
    m_params.bypassed = bypass;
}

void JZEllipticalMonoBassCenteringEngine::updateFilters()
{
    float fc = m_params.monoCutoffHz;
    float w0 = 2.0f * PI_F * fc / m_sampleRate;
    float cosW0 = std::cos(w0);
    float sinW0 = std::sin(w0);
    float q = 0.70710678f; // Butterworth Q
    float alpha = sinW0 / (2.0f * q);

    float b0 = (1.0f + cosW0) * 0.5f;
    float b1 = -(1.0f + cosW0);
    float b2 = (1.0f + cosW0) * 0.5f;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cosW0;
    float a2 = 1.0f - alpha;

    m_hpB0_1 = b0 / a0;
    m_hpB1_1 = b1 / a0;
    m_hpB2_1 = b2 / a0;
    m_hpA1_1 = a1 / a0;
    m_hpA2_1 = a2 / a0;

    // Stage 2 coefficients (cascaded for 24 dB/oct slope)
    m_hpB0_2 = m_hpB0_1;
    m_hpB1_2 = m_hpB1_1;
    m_hpB2_2 = m_hpB2_1;
    m_hpA1_2 = m_hpA1_1;
    m_hpA2_2 = m_hpA2_1;
}

void JZEllipticalMonoBassCenteringEngine::process(
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

    bool is24dB = (m_params.filterSlope == EllipticalFilterSlope::SLOPE_24DB_OCT);
    float sideSubEnergyAccum = 0.0f;
    float dotLR = 0.0f, sumL2 = 0.0f, sumR2 = 0.0f;

    for (size_t i = 0; i < numSamples; ++i) {
        float xL = inL[i];
        float xR = inR[i];

        // 1. Mid/Side Encoding
        float M = (xL + xR) * SQRT2_INV;
        float S = (xL - xR) * SQRT2_INV;

        // 2. High-Pass filter on Side channel
        float y1 = m_hpB0_1 * S + m_hpB1_1 * m_hpX1_1 + m_hpB2_1 * m_hpX2_1 - m_hpA1_1 * m_hpY1_1 - m_hpA2_1 * m_hpY2_1;
        m_hpX2_1 = m_hpX1_1; m_hpX1_1 = S;
        m_hpY2_1 = m_hpY1_1; m_hpY1_1 = y1;

        float sideHp = y1;
        if (is24dB) {
            float y2 = m_hpB0_2 * y1 + m_hpB1_2 * m_hpX1_2 + m_hpB2_2 * m_hpX2_2 - m_hpA1_2 * m_hpY1_2 - m_hpA2_2 * m_hpY2_2;
            m_hpX2_2 = m_hpX1_2; m_hpX1_2 = y1;
            m_hpY2_2 = m_hpY1_2; m_hpY2_2 = y2;
            sideHp = y2;
        }

        // Removed sub-bass side component
        float sideSub = S - sideHp;
        sideSubEnergyAccum += sideSub * sideSub;

        // Audition Side Sub solo mode
        if (m_params.auditionSideSub) {
            outL[i] = sideSub * SQRT2_INV;
            outR[i] = -sideSub * SQRT2_INV;
            continue;
        }

        // Redirect sub energy to Mid channel if enabled
        float procM = M;
        if (m_params.redirectSubEnergyToMid) {
            procM += sideSub * 0.7071f;
        }

        // High frequency Side channel width scaling
        float procS = sideHp * m_params.stereoWidth;

        // 3. Mid/Side Decoding
        float wetL = (procM + procS) * SQRT2_INV;
        float wetR = (procM - procS) * SQRT2_INV;

        // Dry/Wet blend
        float finalL = xL * (1.0f - m_params.dryWet) + wetL * m_params.dryWet;
        float finalR = xR * (1.0f - m_params.dryWet) + wetR * m_params.dryWet;

        outL[i] = finalL;
        outR[i] = finalR;

        // Correlation tracking
        dotLR += finalL * finalR;
        sumL2 += finalL * finalL;
        sumR2 += finalR * finalR;
    }

    // Telemetry updates
    float meanSubPower = sideSubEnergyAccum / static_cast<float>(numSamples);
    m_filteredSideEnergyDb = 10.0f * std::log10(meanSubPower + 1e-6f);

    float denom = std::sqrt(sumL2 * sumR2);
    if (denom > 1e-6f) {
        m_phaseCorrelation = std::max(-1.0f, std::min(dotLR / denom, 1.0f));
    }
}

} // namespace Jazz
