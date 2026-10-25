//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// VintageInductorWahPedalEngine.cpp - Historic Fasel / Halo Toroidal Inductor
// Wah-Wah & Resonant Drive Engine
//*****************************************************************************

#include "VintageInductorWahPedalEngine.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Jazz {

JZVintageInductorWahPedalEngine::JZVintageInductorWahPedalEngine(float sampleRate)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
{
    initialize(m_sampleRate);
}

void JZVintageInductorWahPedalEngine::initialize(float sampleRate)
{
    m_sampleRate = sampleRate > 1000.0f ? sampleRate : 48000.0f;
    reset();
}

void JZVintageInductorWahPedalEngine::reset()
{
    m_ic1eqL = m_ic2eqL = 0.0f;
    m_ic1eqR = m_ic2eqR = 0.0f;
    m_smoothedPos = m_params.pedalPosition;
    m_centerFreqHz = 850.0f;
    m_thdPercent = 0.5f;
}

void JZVintageInductorWahPedalEngine::setParameters(const Parameters& params)
{
    m_params = params;
    m_params.pedalPosition = std::max(0.0f, std::min(m_params.pedalPosition, 1.0f));
    m_params.driveDb = std::max(-18.0f, std::min(m_params.driveDb, 24.0f));
    m_params.inductorSaturation = std::max(0.0f, std::min(m_params.inductorSaturation, 1.0f));
    m_params.resonanceQ = std::max(1.5f, std::min(m_params.resonanceQ, 12.0f));
}

void JZVintageInductorWahPedalEngine::setPedalPosition(float position0to1)
{
    m_params.pedalPosition = std::max(0.0f, std::min(position0to1, 1.0f));
}

void JZVintageInductorWahPedalEngine::setDriveDb(float db)
{
    m_params.driveDb = std::max(-18.0f, std::min(db, 24.0f));
}

void JZVintageInductorWahPedalEngine::setInductorSaturation(float sat)
{
    m_params.inductorSaturation = std::max(0.0f, std::min(sat, 1.0f));
}

void JZVintageInductorWahPedalEngine::setResonanceQ(float q)
{
    m_params.resonanceQ = std::max(1.5f, std::min(q, 12.0f));
}

void JZVintageInductorWahPedalEngine::setInductorType(WahInductorType type)
{
    m_params.inductorType = type;
}

std::string JZVintageInductorWahPedalEngine::getInductorTypeName() const
{
    switch (m_params.inductorType) {
        case WahInductorType::FASEL_RED: return "Italian Red Fasel (500mH / Chime & Bite)";
        case WahInductorType::FASEL_YELLOW: return "Italian Yellow Fasel (Vintage Round Vowel)";
        case WahInductorType::HALO_TOROIDAL: return "Clyde McCoy Halo Toroidal (Thick Throaty Mid)";
        default: return "Custom Inductor";
    }
}

void JZVintageInductorWahPedalEngine::setBypass(bool bp)
{
    m_params.bypass = bp;
}

float JZVintageInductorWahPedalEngine::calculateIcarTaper(float pos) const
{
    // The vintage ICAR 100k pot has an authentic reverse S-curve taper
    // Most change occurs in the middle 40-70% of treadle rotation
    float p = std::max(0.0f, std::min(pos, 1.0f));
    float p2 = p * p;
    float invP = 1.0f - p;
    float invP2 = invP * invP;
    float taper = p2 / (p2 + invP2 + 1e-5f);
    return taper;
}

void JZVintageInductorWahPedalEngine::process(const float* inL, const float* inR,
                                            float* outL, float* outR,
                                            size_t numSamples)
{
    if (!inL || !inR || !outL || !outR || numSamples == 0) return;

    if (m_params.bypass) {
        if (outL != inL) std::copy(inL, inL + numSamples, outL);
        if (outR != inR) std::copy(inR, inR + numSamples, outR);
        return;
    }

    float driveLinear = std::pow(10.0f, m_params.driveDb / 20.0f);

    // Inductor model characteristics
    float baseLowFreq = 350.0f;
    float baseHighFreq = 2200.0f;
    float baseQ = m_params.resonanceQ;

    if (m_params.inductorType == WahInductorType::FASEL_RED) {
        baseLowFreq = 380.0f;
        baseHighFreq = 2400.0f;
        baseQ *= 1.15f;
    } else if (m_params.inductorType == WahInductorType::HALO_TOROIDAL) {
        baseLowFreq = 330.0f;
        baseHighFreq = 2050.0f;
        baseQ *= 1.25f;
    }

    float posSmoothing = 0.05f;

    for (size_t i = 0; i < numSamples; ++i) {
        // Smooth pedal movement
        m_smoothedPos += posSmoothing * (m_params.pedalPosition - m_smoothedPos);

        // Apply ICAR taper
        float potCurve = calculateIcarTaper(m_smoothedPos);

        // Frequency sweep exponential response
        m_centerFreqHz = baseLowFreq * std::pow(baseHighFreq / baseLowFreq, potCurve);

        // State-Variable Filter (Chamberlin / ZDF SVF topology)
        float g = std::tan(static_cast<float>(M_PI) * m_centerFreqHz / m_sampleRate);
        float k = 1.0f / baseQ;
        float a1 = 1.0f / (1.0f + g * (g + k));
        float a2 = g * a1;
        float a3 = g * a2;

        // Channel Left
        float xL = inL[i] * driveLinear;
        // Non-linear inductor core flux B-H saturation tanh()
        if (m_params.inductorSaturation > 0.01f) {
            float satFactor = 1.0f + m_params.inductorSaturation * 2.0f;
            xL = std::tanh(xL * satFactor) / satFactor;
        }

        float v3L = xL - m_ic2eqL;
        float v1L = a1 * m_ic1eqL + a2 * v3L;
        float v2L = m_ic2eqL + a2 * m_ic1eqL + a3 * v3L;
        m_ic1eqL = 2.0f * v1L - m_ic1eqL;
        m_ic2eqL = 2.0f * v2L - m_ic2eqL;

        // Wah output is primarily the resonant bandpass peak mixed with a small dry pass
        float bandPassL = v1L;
        float lowPassL = v2L;
        float wahOutL = bandPassL * 1.8f + lowPassL * 0.35f;

        // Channel Right
        float xR = inR[i] * driveLinear;
        if (m_params.inductorSaturation > 0.01f) {
            float satFactor = 1.0f + m_params.inductorSaturation * 2.0f;
            xR = std::tanh(xR * satFactor) / satFactor;
        }

        float v3R = xR - m_ic2eqR;
        float v1R = a1 * m_ic1eqR + a2 * v3R;
        float v2R = m_ic2eqR + a2 * m_ic1eqR + a3 * v3R;
        m_ic1eqR = 2.0f * v1R - m_ic1eqR;
        m_ic2eqR = 2.0f * v2R - m_ic2eqR;

        float bandPassR = v1R;
        float lowPassR = v2R;
        float wahOutR = bandPassR * 1.8f + lowPassR * 0.35f;

        // BJT Output buffer soft clip
        outL[i] = std::tanh(wahOutL * 0.9f);
        outR[i] = std::tanh(wahOutR * 0.9f);
    }

    m_thdPercent = 0.5f + m_params.inductorSaturation * 2.5f + (m_params.driveDb > 0.0f ? m_params.driveDb * 0.2f : 0.0f);
}

} // namespace Jazz
