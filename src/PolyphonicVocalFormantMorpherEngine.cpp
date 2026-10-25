//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// PolyphonicVocalFormantMorpherEngine.cpp - Vocal Formant & Throat Resynthesizer
//*****************************************************************************

#include "PolyphonicVocalFormantMorpherEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

JZPolyphonicVocalFormantMorpherEngine::JZPolyphonicVocalFormantMorpherEngine(float sampleRate) {
    initialize(sampleRate);
}

void JZPolyphonicVocalFormantMorpherEngine::initialize(float sampleRate) {
    if (sampleRate < 8000.0f) sampleRate = 48000.0f;
    m_sampleRate = sampleRate;

    reset();
    updateFormantCoefficients();
}

void JZPolyphonicVocalFormantMorpherEngine::reset() {
    for (int k = 0; k < 4; ++k) {
        m_formants[k].reset();
    }
    m_noiseSeed = 987654321;
}

void JZPolyphonicVocalFormantMorpherEngine::setParameters(const Parameters& params) {
    m_params = params;
    m_params.throatLengthFactor = std::max(0.5f, std::min(1.8f, m_params.throatLengthFactor));
    m_params.formantShiftSemitones = std::max(-18.0f, std::min(18.0f, m_params.formantShiftSemitones));
    m_params.genderMorph = std::max(-1.0f, std::min(1.0f, m_params.genderMorph));
    m_params.breathiness = std::max(0.0f, std::min(1.0f, m_params.breathiness));
    m_params.dryWet = std::max(0.0f, std::min(1.0f, m_params.dryWet));
    updateFormantCoefficients();
}

void JZPolyphonicVocalFormantMorpherEngine::setThroatLengthFactor(float factor) {
    m_params.throatLengthFactor = std::max(0.5f, std::min(1.8f, factor));
    updateFormantCoefficients();
}

void JZPolyphonicVocalFormantMorpherEngine::setFormantShiftSemitones(float st) {
    m_params.formantShiftSemitones = std::max(-18.0f, std::min(18.0f, st));
    updateFormantCoefficients();
}

void JZPolyphonicVocalFormantMorpherEngine::setGenderMorph(float gender) {
    m_params.genderMorph = std::max(-1.0f, std::min(1.0f, gender));
    updateFormantCoefficients();
}

float JZPolyphonicVocalFormantMorpherEngine::getNoise() {
    m_noiseSeed = m_noiseSeed * 1664525u + 1013904223u;
    return (static_cast<float>(m_noiseSeed & 0x7FFFFFFF) / static_cast<float>(0x3FFFFFFF)) - 1.0f;
}

void JZPolyphonicVocalFormantMorpherEngine::updateFormantCoefficients() {
    // Acoustic tract length inversely scales resonant frequencies
    float lengthScale = 1.0f / m_params.throatLengthFactor;
    float semitoneScale = std::pow(2.0f, m_params.formantShiftSemitones / 12.0f);
    float genderScale = std::pow(2.0f, -m_params.genderMorph * 0.25f);

    float totalScale = lengthScale * semitoneScale * genderScale;

    // Nominal vowel formants
    m_f1 = 700.0f * totalScale;
    m_f2 = 1250.0f * totalScale;
    m_f3 = 2800.0f * totalScale;
    m_f4 = 3800.0f * totalScale;

    float freqs[4] = { m_f1, m_f2, m_f3, m_f4 };
    float Qs[4] = { 4.5f, 5.5f, 6.5f, 7.5f };

    for (int k = 0; k < 4; ++k) {
        float f0 = freqs[k];
        if (f0 >= m_sampleRate * 0.45f) f0 = m_sampleRate * 0.45f;
        if (f0 < 100.0f) f0 = 100.0f;

        float w0 = 2.0f * static_cast<float>(M_PI) * (f0 / m_sampleRate);
        float sin_w = std::sin(w0);
        float cos_w = std::cos(w0);
        float alpha = sin_w / (2.0f * Qs[k]);

        // Bandpass resonator with constant skirt gain
        float a0 = 1.0f + alpha;
        m_formants[k].b0 = alpha / a0;
        m_formants[k].b1 = 0.0f;
        m_formants[k].b2 = -alpha / a0;
        m_formants[k].a1 = (-2.0f * cos_w) / a0;
        m_formants[k].a2 = (1.0f - alpha) / a0;
    }
}

void JZPolyphonicVocalFormantMorpherEngine::process(
    const float* inL, const float* inR,
    float* outL, float* outR,
    size_t numSamples) {

    if (!inL || !outL || numSamples == 0) return;
    const bool stereo = (inR != nullptr && outR != nullptr);

    float weights[4] = { 1.0f, 0.65f, 0.42f, 0.28f };

    for (size_t i = 0; i < numSamples; ++i) {
        float sampleL = inL[i];
        float sampleR = stereo ? inR[i] : sampleL;

        // Add subtle breathiness noise
        float breathL = getNoise() * m_params.breathiness * std::abs(sampleL) * 0.2f;
        float breathR = getNoise() * m_params.breathiness * std::abs(sampleR) * 0.2f;

        float excitedL = sampleL + breathL;
        float excitedR = sampleR + breathR;

        float resynthL = 0.0f;
        float resynthR = 0.0f;

        for (int k = 0; k < 4; ++k) {
            resynthL += m_formants[k].processL(excitedL) * weights[k];
            if (stereo) {
                resynthR += m_formants[k].processR(excitedR) * weights[k];
            }
        }

        resynthL *= 1.8f;
        resynthR *= 1.8f;

        outL[i] = sampleL * (1.0f - m_params.dryWet) + resynthL * m_params.dryWet;
        if (stereo) {
            outR[i] = sampleR * (1.0f - m_params.dryWet) + resynthR * m_params.dryWet;
        }
    }
}

} // namespace Jazz
