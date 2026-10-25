//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZSympatheticGuitarResonanceEngine.cpp - Acoustic Guitar Sympathetic Resonance
//*****************************************************************************

#include "SympatheticGuitarResonanceEngine.h"
#include <algorithm>
#include <cmath>

namespace Jazz {

JZSympatheticGuitarResonanceEngine::JZSympatheticGuitarResonanceEngine(float sampleRate)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
    , m_coupling(0.45f)
    , m_bodyResonance(0.50f)
    , m_capoFret(0)
    , m_helmholtzState1(0.0f), m_helmholtzState2(0.0f)
{
    // Standard Guitar Tuning: E4, B3, G3, D3, A2, E2
    m_nominalFreqs[0] = 329.63f; // 1st string (High E)
    m_nominalFreqs[1] = 246.94f; // 2nd string (B)
    m_nominalFreqs[2] = 196.00f; // 3rd string (G)
    m_nominalFreqs[3] = 146.83f; // 4th string (D)
    m_nominalFreqs[4] = 110.00f; // 5th string (A)
    m_nominalFreqs[5] = 82.41f;  // 6th string (Low E)

    updateFrequencies();
    reset();
}

void JZSympatheticGuitarResonanceEngine::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        m_sampleRate = sampleRate;
        reset();
    }
}

void JZSympatheticGuitarResonanceEngine::setSympatheticCoupling(float amount) {
    m_coupling = std::clamp(amount, 0.0f, 1.0f);
}

void JZSympatheticGuitarResonanceEngine::setBodyResonance(float amount) {
    m_bodyResonance = std::clamp(amount, 0.0f, 1.0f);
}

void JZSympatheticGuitarResonanceEngine::setCapoFret(int fret) {
    m_capoFret = std::clamp(fret, 0, 7);
    updateFrequencies();
}

void JZSympatheticGuitarResonanceEngine::updateFrequencies() {
    float semitoneMultiplier = std::pow(2.0f, static_cast<float>(m_capoFret) / 12.0f);
    for (size_t s = 0; s < NUM_STRINGS; ++s) {
        m_stringFrequencies[s] = m_nominalFreqs[s] * semitoneMultiplier;
    }
}

void JZSympatheticGuitarResonanceEngine::pluckString(int stringNumber, float velocity) {
    int idx = stringNumber - 1;
    if (idx >= 0 && idx < static_cast<int>(NUM_STRINGS)) {
        m_stringEnvelopes[idx] = std::clamp(velocity, 0.1f, 1.0f);
    }
}

void JZSympatheticGuitarResonanceEngine::reset() {
    for (size_t s = 0; s < NUM_STRINGS; ++s) {
        m_stringEnvelopes[s] = 0.0f;
        m_filterState1[s] = 0.0f;
        m_filterState2[s] = 0.0f;
    }
    m_helmholtzState1 = m_helmholtzState2 = 0.0f;
}

std::vector<GuitarStringState> JZSympatheticGuitarResonanceEngine::getStringStates() const {
    std::vector<GuitarStringState> states;
    states.reserve(NUM_STRINGS);

    for (size_t s = 0; s < NUM_STRINGS; ++s) {
        GuitarStringState st;
        st.stringNumber = static_cast<int>(s + 1);
        st.baseFrequencyHz = m_stringFrequencies[s];
        st.currentResonanceEnergy = m_stringEnvelopes[s];
        st.isPlucked = (m_stringEnvelopes[s] > 0.05f);
        states.push_back(st);
    }
    return states;
}

void JZSympatheticGuitarResonanceEngine::process(const float* inL, const float* inR,
                                                float* outL, float* outR, size_t numSamples) {
    if (!inL || !inR || !outL || !outR || numSamples == 0) return;

    // Helmholtz air cavity resonator parameters (~102 Hz)
    float wH = 2.0f * 3.14159265f * 102.0f / m_sampleRate;
    float cosWH = std::cos(wH);
    float sinWH = std::sin(wH);
    float alphaH = sinWH / (2.0f * 6.0f); // Q = 6.0

    float b0_H = alphaH;
    float b2_H = -alphaH;
    float a0_H = 1.0f + alphaH;
    float a1_H = -2.0f * cosWH;
    float a2_H = 1.0f - alphaH;

    float invA0_H = 1.0f / a0_H;
    b0_H *= invA0_H; b2_H *= invA0_H; a1_H *= invA0_H; a2_H *= invA0_H;

    float decayCoeff = std::exp(-1.0f / (m_sampleRate * 1.5f)); // 1.5 second sustain

    for (size_t i = 0; i < numSamples; ++i) {
        float mono = (inL[i] + inR[i]) * 0.5f;

        // Helmholtz cavity resonance
        float helmholtz = b0_H * mono + b2_H * m_helmholtzState2 - a1_H * m_helmholtzState1 - a2_H * m_helmholtzState2;
        m_helmholtzState2 = m_helmholtzState1;
        m_helmholtzState1 = helmholtz;

        float sympatheticHalo = 0.0f;

        // 6-String Bridge Coupling Resonator Bank
        for (size_t s = 0; s < NUM_STRINGS; ++s) {
            float f0 = m_stringFrequencies[s];
            float w0 = 2.0f * 3.14159265f * f0 / m_sampleRate;
            float cosW = std::cos(w0);
            float sinW = std::sin(w0);
            float alpha = sinW / (2.0f * 25.0f); // High Q = 25.0 for ringing string

            float b0 = alpha;
            float b2 = -alpha;
            float a0 = 1.0f + alpha;
            float a1 = -2.0f * cosW;
            float a2 = 1.0f - alpha;

            float invA0 = 1.0f / a0;
            b0 *= invA0; b2 *= invA0; a1 *= invA0; a2 *= invA0;

            // Excitation signal: mono input + plucked envelope
            float excite = mono * m_coupling + (m_stringEnvelopes[s] * 0.15f * mono);
            float stringRes = b0 * excite + b2 * m_filterState2[s] - a1 * m_filterState1[s] - a2 * m_filterState2[s];
            m_filterState2[s] = m_filterState1[s];
            m_filterState1[s] = stringRes;

            sympatheticHalo += stringRes;
            m_stringEnvelopes[s] *= decayCoeff;
        }

        float totalAcoustic = mono + (sympatheticHalo * m_coupling * 0.7f) + (helmholtz * m_bodyResonance * 0.35f);

        outL[i] = std::clamp(inL[i] + (totalAcoustic - mono), -1.0f, 1.0f);
        outR[i] = std::clamp(inR[i] + (totalAcoustic - mono), -1.0f, 1.0f);
    }
}

} // namespace Jazz
