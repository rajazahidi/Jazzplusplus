//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// ReactiveLoadSpeakerImpedanceEngine.cpp - Reactive Load & Power Soak Simulator
//*****************************************************************************

#include "ReactiveLoadSpeakerImpedanceEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

JZReactiveLoadSpeakerImpedanceEngine::JZReactiveLoadSpeakerImpedanceEngine(float sampleRate) {
    initialize(sampleRate);
}

void JZReactiveLoadSpeakerImpedanceEngine::initialize(float sampleRate) {
    if (sampleRate < 8000.0f) sampleRate = 48000.0f;
    m_sampleRate = sampleRate;

    reset();
    updateImpedanceFilters();
}

void JZReactiveLoadSpeakerImpedanceEngine::reset() {
    m_resX1L = m_resX2L = m_resY1L = m_resY2L = 0.0f;
    m_resX1R = m_resX2R = m_resY1R = m_resY2R = 0.0f;

    m_indX1L = m_indX2L = m_indY1L = m_indY2L = 0.0f;
    m_indX1R = m_indX2R = m_indY1R = m_indY2R = 0.0f;

    m_sagEnvelope = 0.0f;
    m_powerSagReductDb = 0.0f;
}

void JZReactiveLoadSpeakerImpedanceEngine::setParameters(const Parameters& params) {
    m_params = params;
    m_params.resonanceFreqHz = std::max(45.0f, std::min(130.0f, m_params.resonanceFreqHz));
    m_params.inductiveRise = std::max(0.1f, std::min(3.0f, m_params.inductiveRise));
    m_params.powerSagAmount = std::max(0.0f, std::min(1.0f, m_params.powerSagAmount));
    m_params.powerSoakDamping = std::max(0.2f, std::min(2.0f, m_params.powerSoakDamping));
    m_params.dryWet = std::max(0.0f, std::min(1.0f, m_params.dryWet));

    updateImpedanceFilters();
}

void JZReactiveLoadSpeakerImpedanceEngine::setCabinetType(SpeakerCabinetType type) {
    m_params.cabType = type;
    switch (type) {
        case SpeakerCabinetType::VINTAGE_4X12_V30:
            m_params.resonanceFreqHz = 75.0f; m_params.inductiveRise = 1.1f; break;
        case SpeakerCabinetType::OPEN_BACK_2X12:
            m_params.resonanceFreqHz = 85.0f; m_params.inductiveRise = 0.85f; break;
        case SpeakerCabinetType::TWEED_DELUXE_1X12:
            m_params.resonanceFreqHz = 95.0f; m_params.inductiveRise = 0.70f; break;
        case SpeakerCabinetType::CLASSIC_4X10_ALNICO:
            m_params.resonanceFreqHz = 68.0f; m_params.inductiveRise = 1.25f; break;
    }
    updateImpedanceFilters();
}

void JZReactiveLoadSpeakerImpedanceEngine::setResonanceFreqHz(float hz) {
    m_params.resonanceFreqHz = std::max(45.0f, std::min(130.0f, hz));
    updateImpedanceFilters();
}

void JZReactiveLoadSpeakerImpedanceEngine::setPowerSagAmount(float sag) {
    m_params.powerSagAmount = std::max(0.0f, std::min(1.0f, sag));
}

std::string JZReactiveLoadSpeakerImpedanceEngine::getCabinetTypeName() const {
    switch (m_params.cabType) {
        case SpeakerCabinetType::VINTAGE_4X12_V30:   return "Vintage 4x12 Celestion V30";
        case SpeakerCabinetType::OPEN_BACK_2X12:     return "Open-Back 2x12 British Warm";
        case SpeakerCabinetType::TWEED_DELUXE_1X12:  return "Tweed Deluxe 1x12 American Sag";
        case SpeakerCabinetType::CLASSIC_4X10_ALNICO:return "Classic 4x10 Alnico Blue";
    }
    return "Custom Cabinet";
}

void JZReactiveLoadSpeakerImpedanceEngine::updateImpedanceFilters() {
    // 1. Mechanical voice-coil bass resonance peak (8 -> 32 ohms = +8.5 dB boost)
    float resF0 = m_params.resonanceFreqHz;
    float resGainDb = 8.5f;
    float resA = std::pow(10.0f, resGainDb / 40.0f);
    float resW0 = 2.0f * static_cast<float>(M_PI) * (resF0 / m_sampleRate);
    float resAlpha = std::sin(resW0) / (2.0f * 2.2f);
    float resCos = std::cos(resW0);

    float b0_r = 1.0f + resAlpha * resA;
    float b1_r = -2.0f * resCos;
    float b2_r = 1.0f - resAlpha * resA;
    float a0_r = 1.0f + resAlpha / resA;
    float a1_r = -2.0f * resCos;
    float a2_r = 1.0f - resAlpha / resA;

    m_resB0 = b0_r / a0_r; m_resB1 = b1_r / a0_r; m_resB2 = b2_r / a0_r;
    m_resA1 = a1_r / a0_r; m_resA2 = a2_r / a0_r;

    // 2. High-frequency voice-coil inductive rise (High Shelf at 2800 Hz)
    float indF0 = 2800.0f;
    float indGainDb = 5.5f * m_params.inductiveRise;
    float indA = std::pow(10.0f, indGainDb / 40.0f);
    float indW0 = 2.0f * static_cast<float>(M_PI) * (indF0 / m_sampleRate);
    float indCos = std::cos(indW0);
    float indSin = std::sin(indW0);
    float indAlpha = indSin / (2.0f * 0.7071f);

    float b0_i = indA * ((indA + 1.0f) + (indA - 1.0f) * indCos + 2.0f * std::sqrt(indA) * indAlpha);
    float b1_i = -2.0f * indA * ((indA - 1.0f) + (indA + 1.0f) * indCos);
    float b2_i = indA * ((indA + 1.0f) + (indA - 1.0f) * indCos - 2.0f * std::sqrt(indA) * indAlpha);
    float a0_i = (indA + 1.0f) - (indA - 1.0f) * indCos + 2.0f * std::sqrt(indA) * indAlpha;
    float a1_i = 2.0f * ((indA - 1.0f) - (indA + 1.0f) * indCos);
    float a2_i = (indA + 1.0f) - (indA - 1.0f) * indCos - 2.0f * std::sqrt(indA) * indAlpha;

    m_indB0 = b0_i / a0_i; m_indB1 = b1_i / a0_i; m_indB2 = b2_i / a0_i;
    m_indA1 = a1_i / a0_i; m_indA2 = a2_i / a0_i;
}

void JZReactiveLoadSpeakerImpedanceEngine::process(
    const float* inL, const float* inR,
    float* outL, float* outR,
    size_t numSamples) {

    if (!inL || !outL || numSamples == 0) return;
    const bool stereo = (inR != nullptr && outR != nullptr);

    float soakDamping = m_params.powerSoakDamping;
    float sagAmount = m_params.powerSagAmount;
    float wet = m_params.dryWet;
    float dry = 1.0f - wet;

    float minSagObserved = 1.0f;

    for (size_t i = 0; i < numSamples; ++i) {
        float sL = inL[i];
        float sR = stereo ? inR[i] : sL;

        // 1. Power Supply Sag dynamic compression
        float ampLvl = std::max(std::abs(sL), std::abs(sR));
        m_sagEnvelope = m_sagEnvelope * 0.995f + ampLvl * 0.005f;

        float sagGain = 1.0f;
        if (m_sagEnvelope > 0.2f && sagAmount > 0.01f) {
            float excess = (m_sagEnvelope - 0.2f) * sagAmount * 1.5f;
            sagGain = 1.0f / (1.0f + excess);
            if (sagGain < minSagObserved) minSagObserved = sagGain;
        }

        sL *= sagGain;
        if (stereo) sR *= sagGain;

        // 2. Mechanical voice-coil bass resonance filter
        float resL = m_resB0 * sL + m_resB1 * m_resX1L + m_resB2 * m_resX2L
                     - m_resA1 * m_resY1L - m_resA2 * m_resY2L;
        m_resX2L = m_resX1L; m_resX1L = sL;
        m_resY2L = m_resY1L; m_resY1L = resL;

        float resR = sR;
        if (stereo) {
            resR = m_resB0 * sR + m_resB1 * m_resX1R + m_resB2 * m_resX2R
                   - m_resA1 * m_resY1R - m_resA2 * m_resY2R;
            m_resX2R = m_resX1R; m_resX1R = sR;
            m_resY2R = m_resY1R; m_resY1R = resR;
        }

        // 3. Inductive voice-coil high-frequency rise filter
        float indL = m_indB0 * resL + m_indB1 * m_indX1L + m_indB2 * m_indX2L
                     - m_indA1 * m_indY1L - m_indA2 * m_indY2L;
        m_indX2L = m_indX1L; m_indX1L = resL;
        m_indY2L = m_indY1L; m_indY1L = indL;

        float indR = resR;
        if (stereo) {
            indR = m_indB0 * resR + m_indB1 * m_indX1R + m_indB2 * m_indX2R
                   - m_indA1 * m_indY1R - m_indA2 * m_indY2R;
            m_indX2R = m_indX1R; m_indX1R = resR;
            m_indY2R = m_indY1R; m_indY1R = indR;
        }

        // 4. Power soak back-EMF soft magnetic saturation
        float loadL = std::tanh(indL * soakDamping);
        float loadR = std::tanh(indR * soakDamping);

        outL[i] = inL[i] * dry + loadL * wet;
        if (stereo) {
            outR[i] = inR[i] * dry + loadR * wet;
        }
    }

    m_powerSagReductDb = 20.0f * std::log10(std::max(1e-4f, minSagObserved));
}

} // namespace Jazz
