//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZBiomechanicalVocalTractEngine.cpp - Bio-Mechanical Vocal Tract Aero-Acoustics
//*****************************************************************************

#include "BiomechanicalVocalTractEngine.h"
#include <algorithm>
#include <cmath>

namespace Jazz {

JZBiomechanicalVocalTractEngine::JZBiomechanicalVocalTractEngine(float sampleRate)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
    , m_pitchHz(220.0f) // A3 default singing pitch
    , m_subglottalPressure(10.0f) // 10 cm H2O standard vocal effort
    , m_breathiness(0.08f)
    , m_nasality(0.0f)
    , m_currentVowel(VocalTractVowel::VOWEL_A)
    , m_glottalPhase(0.0f)
    , m_openQuotient(0.65f)
    , m_lipRadiationState(0.0f)
    , m_noiseSeed(987654321)
{
    setVowel(VocalTractVowel::VOWEL_A);
    for (size_t s = 0; s < TRACT_SECTIONS; ++s) {
        m_currentAreas[s] = m_targetAreas[s];
    }
    reset();
}

void JZBiomechanicalVocalTractEngine::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        m_sampleRate = sampleRate;
        reset();
    }
}

void JZBiomechanicalVocalTractEngine::setPitchHz(float f0Hz) {
    m_pitchHz = std::clamp(f0Hz, 50.0f, 1200.0f);
}

void JZBiomechanicalVocalTractEngine::setSubglottalPressure(float pressureCmH2O) {
    m_subglottalPressure = std::clamp(pressureCmH2O, 2.0f, 30.0f);
}

void JZBiomechanicalVocalTractEngine::setBreathiness(float breathiness) {
    m_breathiness = std::clamp(breathiness, 0.0f, 1.0f);
}

void JZBiomechanicalVocalTractEngine::setNasality(float velumOpening) {
    m_nasality = std::clamp(velumOpening, 0.0f, 1.0f);
}

float JZBiomechanicalVocalTractEngine::getNoise() {
    m_noiseSeed = (1103515245U * m_noiseSeed + 12345U) & 0x7fffffffU;
    return (static_cast<float>(m_noiseSeed) / static_cast<float>(0x7fffffffU)) * 2.0f - 1.0f;
}

void JZBiomechanicalVocalTractEngine::updateVowelAreas(VocalTractVowel v, float target[TRACT_SECTIONS]) const {
    // 8 vocal tract area sections (cm^2):
    // [0]=Lower Pharynx, [1]=Upper Pharynx, [2]=Velum, [3]=Tongue Body,
    // [4]=Tongue Blade, [5]=Anterior Mouth, [6]=Teeth, [7]=Lips
    switch (v) {
        case VocalTractVowel::VOWEL_A: // /a/ (Father)
            target[0] = 0.8f; target[1] = 1.0f; target[2] = 2.2f; target[3] = 3.5f;
            target[4] = 4.2f; target[5] = 4.8f; target[6] = 5.0f; target[7] = 5.2f;
            break;
        case VocalTractVowel::VOWEL_E: // /e/ (Bed)
            target[0] = 2.0f; target[1] = 2.5f; target[2] = 2.8f; target[3] = 2.2f;
            target[4] = 2.0f; target[5] = 2.8f; target[6] = 3.2f; target[7] = 3.5f;
            break;
        case VocalTractVowel::VOWEL_I: // /i/ (Meet)
            target[0] = 3.5f; target[1] = 3.2f; target[2] = 2.5f; target[3] = 1.2f;
            target[4] = 0.8f; target[5] = 1.2f; target[6] = 2.2f; target[7] = 2.8f;
            break;
        case VocalTractVowel::VOWEL_O: // /o/ (Boat)
            target[0] = 2.0f; target[1] = 2.2f; target[2] = 3.0f; target[3] = 2.8f;
            target[4] = 2.0f; target[5] = 1.5f; target[6] = 1.0f; target[7] = 0.9f;
            break;
        case VocalTractVowel::VOWEL_U: // /u/ (Boot)
            target[0] = 2.8f; target[1] = 2.5f; target[2] = 1.5f; target[3] = 0.9f;
            target[4] = 1.2f; target[5] = 1.0f; target[6] = 0.6f; target[7] = 0.4f;
            break;
    }
}

void JZBiomechanicalVocalTractEngine::setVowel(VocalTractVowel vowel) {
    m_currentVowel = vowel;
    updateVowelAreas(vowel, m_targetAreas);
}

void JZBiomechanicalVocalTractEngine::setVowelBlend(VocalTractVowel v1, VocalTractVowel v2, float blendFactor) {
    float a1[TRACT_SECTIONS], a2[TRACT_SECTIONS];
    updateVowelAreas(v1, a1);
    updateVowelAreas(v2, a2);

    float b = std::clamp(blendFactor, 0.0f, 1.0f);
    for (size_t s = 0; s < TRACT_SECTIONS; ++s) {
        m_targetAreas[s] = a1[s] * (1.0f - b) + a2[s] * b;
    }
}

void JZBiomechanicalVocalTractEngine::reset() {
    m_glottalPhase = 0.0f;
    m_lipRadiationState = 0.0f;
    for (size_t s = 0; s < TRACT_SECTIONS; ++s) {
        m_forwardWave[s] = 0.0f;
        m_backwardWave[s] = 0.0f;
    }
}

void JZBiomechanicalVocalTractEngine::generate(float* outAudio, size_t numSamples) {
    if (!outAudio || numSamples == 0) return;

    float phaseIncrement = m_pitchHz / m_sampleRate;
    float pressureScale = std::sqrt(m_subglottalPressure / 10.0f);

    for (size_t i = 0; i < numSamples; ++i) {
        // Smoothly interpolate vocal tract areas towards targets
        for (size_t s = 0; s < TRACT_SECTIONS; ++s) {
            m_currentAreas[s] += 0.002f * (m_targetAreas[s] - m_currentAreas[s]);
        }

        // 2-mass glottal flow model: Liljencrants-Fant pulse approximation
        m_glottalPhase += phaseIncrement;
        if (m_glottalPhase >= 1.0f) m_glottalPhase -= 1.0f;

        float glottalPulse = 0.0f;
        if (m_glottalPhase < m_openQuotient) {
            float p = m_glottalPhase / m_openQuotient;
            // Half-sine glottal volume velocity pulse with sharp closing edge
            glottalPulse = 0.5f * (1.0f - std::cos(3.14159265f * p)) * std::sin(3.14159265f * p);
        }

        // Inject aspiration breathiness noise
        float aspiration = getNoise() * m_breathiness * 0.15f;
        float glottalSource = (glottalPulse + aspiration) * pressureScale;

        // Waveguide acoustic wave scattering through 8 tube sections
        float inputWave = glottalSource;

        for (size_t s = 0; s < TRACT_SECTIONS - 1; ++s) {
            float A1 = m_currentAreas[s];
            float A2 = m_currentAreas[s + 1];

            // Kelly-Lochbaum reflection coefficient k = (A2 - A1) / (A2 + A1)
            float k = (A2 - A1) / (A2 + A1 + 1e-6f);

            float fwd = m_forwardWave[s] + inputWave;
            float bwd = m_backwardWave[s + 1];

            // Junction scattering
            m_forwardWave[s + 1] = fwd * (1.0f - k) + bwd * k;
            m_backwardWave[s] = fwd * (-k) + bwd * (1.0f + k);

            inputWave = m_forwardWave[s + 1];
        }

        // Lip Radiation: 1st difference high-pass boundary condition
        float lipVolumeVelocity = inputWave;
        float lipAcousticOutput = lipVolumeVelocity - m_lipRadiationState;
        m_lipRadiationState = lipVolumeVelocity * 0.95f;

        outAudio[i] = std::clamp(lipAcousticOutput * 2.5f, -1.0f, 1.0f);
    }
}

} // namespace Jazz
