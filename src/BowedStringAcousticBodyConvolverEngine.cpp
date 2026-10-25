//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// BowedStringAcousticBodyConvolverEngine.cpp - Bowed String Acoustic Body Impulse
// & Chladni Wood Resonator
//*****************************************************************************

#include "BowedStringAcousticBodyConvolverEngine.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Jazz {

JZBowedStringAcousticBodyConvolverEngine::JZBowedStringAcousticBodyConvolverEngine(float sampleRate)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
{
    initialize(m_sampleRate);
}

void JZBowedStringAcousticBodyConvolverEngine::initialize(float sampleRate)
{
    m_sampleRate = sampleRate > 1000.0f ? sampleRate : 48000.0f;
    reset();
}

void JZBowedStringAcousticBodyConvolverEngine::reset()
{
    m_a0X1L = m_a0X2L = m_a0Y1L = m_a0Y2L = 0.0f;
    m_a0X1R = m_a0X2R = m_a0Y1R = m_a0Y2R = 0.0f;
    m_b1X1L = m_b1X2L = m_b1Y1L = m_b1Y2L = 0.0f;
    m_b1X1R = m_b1X2R = m_b1Y1R = m_b1Y2R = 0.0f;
    m_brgX1L = m_brgX2L = m_brgY1L = m_brgY2L = 0.0f;
    m_brgX1R = m_brgX2R = m_brgY1R = m_brgY2R = 0.0f;
    updateModalAcoustics();
}

void JZBowedStringAcousticBodyConvolverEngine::setParameters(const Parameters& params)
{
    m_params = params;
    m_params.bodyWoodAge = std::max(0.0f, std::min(m_params.bodyWoodAge, 1.0f));
    m_params.varnishDamping = std::max(0.0f, std::min(m_params.varnishDamping, 1.0f));
    m_params.helmholtzA0DepthDb = std::max(0.0f, std::min(m_params.helmholtzA0DepthDb, 12.0f));
    m_params.bridgeRockingPeakDb = std::max(-12.0f, std::min(m_params.bridgeRockingPeakDb, 12.0f));
    m_params.bodyMix = std::max(0.0f, std::min(m_params.bodyMix, 1.0f));
    updateModalAcoustics();
}

void JZBowedStringAcousticBodyConvolverEngine::setInstrumentType(BowedInstrumentType type)
{
    m_params.instrumentType = type;
    updateModalAcoustics();
}

std::string JZBowedStringAcousticBodyConvolverEngine::getInstrumentTypeName() const
{
    switch (m_params.instrumentType) {
        case BowedInstrumentType::VIOLIN: return "Concert Master Violin (Spruce & Maple Body)";
        case BowedInstrumentType::VIOLA: return "16.5-Inch Orchestral Viola (Warm Dark Vowel)";
        case BowedInstrumentType::CELLO: return "4/4 Soloist Violoncello (Deep Resonant Chamber)";
        case BowedInstrumentType::DOUBLE_BASS: return "3/4 Orchestral Contrabass (Monumental Sub-Acoustic)";
        default: return "Custom Bowed String";
    }
}

void JZBowedStringAcousticBodyConvolverEngine::setBodyWoodAge(float age0to1)
{
    m_params.bodyWoodAge = std::max(0.0f, std::min(age0to1, 1.0f));
    updateModalAcoustics();
}

void JZBowedStringAcousticBodyConvolverEngine::setVarnishDamping(float damp0to1)
{
    m_params.varnishDamping = std::max(0.0f, std::min(damp0to1, 1.0f));
    updateModalAcoustics();
}

void JZBowedStringAcousticBodyConvolverEngine::setHelmholtzA0DepthDb(float db)
{
    m_params.helmholtzA0DepthDb = std::max(0.0f, std::min(db, 12.0f));
    updateModalAcoustics();
}

void JZBowedStringAcousticBodyConvolverEngine::setBridgeRockingPeakDb(float db)
{
    m_params.bridgeRockingPeakDb = std::max(-12.0f, std::min(db, 12.0f));
    updateModalAcoustics();
}

void JZBowedStringAcousticBodyConvolverEngine::setBodyMix(float mix)
{
    m_params.bodyMix = std::max(0.0f, std::min(mix, 1.0f));
}

void JZBowedStringAcousticBodyConvolverEngine::setBypass(bool bp)
{
    m_params.bypass = bp;
}

void JZBowedStringAcousticBodyConvolverEngine::updateModalAcoustics()
{
    // Define physical modal frequencies per instrument family
    switch (m_params.instrumentType) {
        case BowedInstrumentType::VIOLIN:
            m_helmholtzA0Hz = 280.0f;
            m_plateB1Hz = 480.0f;
            m_bridgeHillHz = 3000.0f;
            break;
        case BowedInstrumentType::VIOLA:
            m_helmholtzA0Hz = 225.0f;
            m_plateB1Hz = 380.0f;
            m_bridgeHillHz = 2400.0f;
            break;
        case BowedInstrumentType::CELLO:
            m_helmholtzA0Hz = 98.0f;
            m_plateB1Hz = 190.0f;
            m_bridgeHillHz = 1200.0f;
            break;
        case BowedInstrumentType::DOUBLE_BASS:
            m_helmholtzA0Hz = 60.0f;
            m_plateB1Hz = 110.0f;
            m_bridgeHillHz = 750.0f;
            break;
    }

    // Aged wood raises resonance Q and clarity; heavy varnish damps high-frequency bridge hill
    float ageQFactor = 1.0f + m_params.bodyWoodAge * 0.8f;
    float varnishLoss = 1.0f - m_params.varnishDamping * 0.4f;

    // Helper lambda to compute peaking biquad filter
    auto makePeaking = [this](float freq, float gainDb, float q,
                              float& b0, float& b1, float& b2, float& a1, float& a2) {
        freq = std::max(20.0f, std::min(freq, m_sampleRate * 0.48f));
        float A = std::pow(10.0f, gainDb / 40.0f);
        float w0 = 2.0f * static_cast<float>(M_PI) * freq / m_sampleRate;
        float alpha = std::sin(w0) / (2.0f * q);
        float cosw0 = std::cos(w0);
        float a0 = 1.0f + alpha / A;
        b0 = (1.0f + alpha * A) / a0;
        b1 = (-2.0f * cosw0) / a0;
        b2 = (1.0f - alpha * A) / a0;
        a1 = (-2.0f * cosw0) / a0;
        a2 = (1.0f - alpha / A) / a0;
    };

    // 1. F-hole Helmholtz A0 resonance peak
    makePeaking(m_helmholtzA0Hz, m_params.helmholtzA0DepthDb, 4.0f * ageQFactor,
                m_a0B0, m_a0B1, m_a0B2, m_a0A1, m_a0A2);

    // 2. Chladni B1 wood plate resonance
    makePeaking(m_plateB1Hz, 3.5f * ageQFactor, 3.5f * ageQFactor,
                m_b1B0, m_b1B1, m_b1B2, m_b1A1, m_b1A2);

    // 3. Bridge hill transmission
    makePeaking(m_bridgeHillHz, m_params.bridgeRockingPeakDb * varnishLoss, 2.5f,
                m_brgB0, m_brgB1, m_brgB2, m_brgA1, m_brgA2);
}

void JZBowedStringAcousticBodyConvolverEngine::process(const float* inL, const float* inR,
                                                      float* outL, float* outR,
                                                      size_t numSamples)
{
    if (!inL || !inR || !outL || !outR || numSamples == 0) return;

    if (m_params.bypass) {
        if (outL != inL) std::copy(inL, inL + numSamples, outL);
        if (outR != inR) std::copy(inR, inR + numSamples, outR);
        return;
    }

    float wet = m_params.bodyMix;
    float dry = 1.0f - wet * 0.5f;

    for (size_t i = 0; i < numSamples; ++i) {
        float xL = inL[i];
        float xR = inR[i];

        // 1. Helmholtz A0 Air Cavity Filter
        float yA0L = m_a0B0 * xL + m_a0B1 * m_a0X1L + m_a0B2 * m_a0X2L - m_a0A1 * m_a0Y1L - m_a0A2 * m_a0Y2L;
        m_a0X2L = m_a0X1L; m_a0X1L = xL; m_a0Y2L = m_a0Y1L; m_a0Y1L = yA0L;

        float yA0R = m_a0B0 * xR + m_a0B1 * m_a0X1R + m_a0B2 * m_a0X2R - m_a0A1 * m_a0Y1R - m_a0A2 * m_a0Y2R;
        m_a0X2R = m_a0X1R; m_a0X1R = xR; m_a0Y2R = m_a0Y1R; m_a0Y1R = yA0R;

        // 2. Chladni B1 Wood Plate Resonator
        float yB1L = m_b1B0 * yA0L + m_b1B1 * m_b1X1L + m_b1B2 * m_b1X2L - m_b1A1 * m_b1Y1L - m_b1A2 * m_b1Y2L;
        m_b1X2L = m_b1X1L; m_b1X1L = yA0L; m_b1Y2L = m_b1Y1L; m_b1Y1L = yB1L;

        float yB1R = m_b1B0 * yA0R + m_b1B1 * m_b1X1R + m_b1B2 * m_b1X2R - m_b1A1 * m_b1Y1R - m_b1A2 * m_b1Y2R;
        m_b1X2R = m_b1X1R; m_b1X1R = yA0R; m_b1Y2R = m_b1Y1R; m_b1Y1R = yB1R;

        // 3. Bridge Hill Transmission
        float yBrgL = m_brgB0 * yB1L + m_brgB1 * m_brgX1L + m_brgB2 * m_brgX2L - m_brgA1 * m_brgY1L - m_brgA2 * m_brgY2L;
        m_brgX2L = m_brgX1L; m_brgX1L = yB1L; m_brgY2L = m_brgY1L; m_brgY1L = yBrgL;

        float yBrgR = m_brgB0 * yB1R + m_brgB1 * m_brgX1R + m_brgB2 * m_brgX2R - m_brgA1 * m_brgY1R - m_brgA2 * m_brgY2R;
        m_brgX2R = m_brgX1R; m_brgX1R = yB1R; m_brgY2R = m_brgY1R; m_brgY1R = yBrgR;

        // Output mix
        outL[i] = xL * dry + yBrgL * wet;
        outR[i] = xR * dry + yBrgR * wet;
    }
}

} // namespace Jazz
