//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// AnalogSubharmonicBassExciterEngine.cpp - Subharmonic Bass & Optical Exciter
//*****************************************************************************

#include "AnalogSubharmonicBassExciterEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

JZAnalogSubharmonicBassExciterEngine::JZAnalogSubharmonicBassExciterEngine(float sampleRate) {
    initialize(sampleRate);
}

void JZAnalogSubharmonicBassExciterEngine::initialize(float sampleRate) {
    if (sampleRate < 8000.0f) sampleRate = 48000.0f;
    m_sampleRate = sampleRate;

    reset();
    updateCrossoverFilters();
}

void JZAnalogSubharmonicBassExciterEngine::reset() {
    m_subLpX1 = m_subLpX2 = m_subLpY1 = m_subLpY2 = 0.0f;

    m_airHpX1L = m_airHpX2L = m_airHpY1L = m_airHpY2L = 0.0f;
    m_airHpX1R = m_airHpX2R = m_airHpY1R = m_airHpY2R = 0.0f;

    m_flipFlop = false;
    m_prevSubSign = 0.0f;
    m_subEnv = 0.0f;
    m_subSinePhase = 0.0f;

    m_subEnergy = 0.0f;
    m_airEnergy = 0.0f;
}

void JZAnalogSubharmonicBassExciterEngine::setParameters(const Parameters& params) {
    m_params = params;
    m_params.subharmonicGainDb = std::max(-36.0f, std::min(18.0f, m_params.subharmonicGainDb));
    m_params.opticalAirGainDb = std::max(-36.0f, std::min(18.0f, m_params.opticalAirGainDb));
    m_params.subCrossoverHz = std::max(40.0f, std::min(180.0f, m_params.subCrossoverHz));
    m_params.airCrossoverHz = std::max(2000.0f, std::min(12000.0f, m_params.airCrossoverHz));
    m_params.drive = std::max(1.0f, std::min(4.0f, m_params.drive));
    m_params.dryWet = std::max(0.0f, std::min(1.0f, m_params.dryWet));

    updateCrossoverFilters();
}

void JZAnalogSubharmonicBassExciterEngine::setSubharmonicGainDb(float db) {
    m_params.subharmonicGainDb = std::max(-36.0f, std::min(18.0f, db));
}

void JZAnalogSubharmonicBassExciterEngine::setOpticalAirGainDb(float db) {
    m_params.opticalAirGainDb = std::max(-36.0f, std::min(18.0f, db));
}

void JZAnalogSubharmonicBassExciterEngine::setSubCrossoverHz(float hz) {
    m_params.subCrossoverHz = std::max(40.0f, std::min(180.0f, hz));
    updateCrossoverFilters();
}

void JZAnalogSubharmonicBassExciterEngine::setAirCrossoverHz(float hz) {
    m_params.airCrossoverHz = std::max(2000.0f, std::min(12000.0f, hz));
    updateCrossoverFilters();
}

void JZAnalogSubharmonicBassExciterEngine::updateCrossoverFilters() {
    // 1. Sub-bass Butterworth 2-pole Low-Pass
    float subW0 = 2.0f * static_cast<float>(M_PI) * (m_params.subCrossoverHz / m_sampleRate);
    float subSin = std::sin(subW0);
    float subCos = std::cos(subW0);
    float subAlpha = subSin / (2.0f * 0.7071f);
    float subA0 = 1.0f + subAlpha;

    m_subLpB0 = ((1.0f - subCos) / 2.0f) / subA0;
    m_subLpB1 = (1.0f - subCos) / subA0;
    m_subLpB2 = ((1.0f - subCos) / 2.0f) / subA0;
    m_subLpA1 = (-2.0f * subCos) / subA0;
    m_subLpA2 = (1.0f - subAlpha) / subA0;

    // 2. Air exciter Butterworth 2-pole High-Pass
    float airW0 = 2.0f * static_cast<float>(M_PI) * (m_params.airCrossoverHz / m_sampleRate);
    float airSin = std::sin(airW0);
    float airCos = std::cos(airW0);
    float airAlpha = airSin / (2.0f * 0.7071f);
    float airA0 = 1.0f + airAlpha;

    m_airHpB0 = ((1.0f + airCos) / 2.0f) / airA0;
    m_airHpB1 = (-(1.0f + airCos)) / airA0;
    m_airHpB2 = ((1.0f + airCos) / 2.0f) / airA0;
    m_airHpA1 = (-2.0f * airCos) / airA0;
    m_airHpA2 = (1.0f - airAlpha) / airA0;
}

void JZAnalogSubharmonicBassExciterEngine::process(
    const float* inL, const float* inR,
    float* outL, float* outR,
    size_t numSamples) {

    if (!inL || !outL || numSamples == 0) return;
    const bool stereo = (inR != nullptr && outR != nullptr);

    float subGain = std::pow(10.0f, m_params.subharmonicGainDb / 20.0f);
    float airGain = std::pow(10.0f, m_params.opticalAirGainDb / 20.0f);
    float wet = m_params.dryWet;
    float dry = 1.0f - wet;
    float drive = m_params.drive;

    float subEnergyAccum = 0.0f;
    float airEnergyAccum = 0.0f;

    for (size_t i = 0; i < numSamples; ++i) {
        float sL = inL[i];
        float sR = stereo ? inR[i] : sL;
        float mono = 0.5f * (sL + sR);

        // 1. Subharmonic Fundamental Synthesis
        float subFiltered = m_subLpB0 * mono + m_subLpB1 * m_subLpX1 + m_subLpB2 * m_subLpX2
                            - m_subLpA1 * m_subLpY1 - m_subLpA2 * m_subLpY2;
        m_subLpX2 = m_subLpX1; m_subLpX1 = mono;
        m_subLpY2 = m_subLpY1; m_subLpY1 = subFiltered;

        // Zero-crossing octave divider (-1 octave flip-flop)
        if (m_prevSubSign < 0.0f && subFiltered >= 0.0f) {
            m_flipFlop = !m_flipFlop;
        }
        m_prevSubSign = subFiltered;

        // Envelope follower on bass signal
        m_subEnv = m_subEnv * 0.995f + std::abs(subFiltered) * 0.005f;

        // Synthesize smooth subharmonic waveform (flip-flop shaped sine)
        float subTone = (m_flipFlop ? 1.0f : -1.0f) * m_subEnv * subGain;
        subEnergyAccum += std::abs(subTone);

        // 2. Optical High-Frequency Harmonic Air Exciter
        float airInL = m_airHpB0 * sL + m_airHpB1 * m_airHpX1L + m_airHpB2 * m_airHpX2L
                       - m_airHpA1 * m_airHpY1L - m_airHpA2 * m_airHpY2L;
        m_airHpX2L = m_airHpX1L; m_airHpX1L = sL;
        m_airHpY2L = m_airHpY1L; m_airHpY1L = airInL;

        float airInR = airInL;
        if (stereo) {
            airInR = m_airHpB0 * sR + m_airHpB1 * m_airHpX1R + m_airHpB2 * m_airHpX2R
                     - m_airHpA1 * m_airHpY1R - m_airHpA2 * m_airHpY2R;
            m_airHpX2R = m_airHpX1R; m_airHpX1R = sR;
            m_airHpY2R = m_airHpY1R; m_airHpY1R = airInR;
        }

        // Asymmetric optical non-linear harmonic generation: y = x + 0.35*x^2 - 0.15*x^3
        float drivenL = airInL * drive;
        float drivenR = airInR * drive;
        float excAirL = (drivenL + 0.35f * drivenL * std::abs(drivenL) - 0.15f * drivenL * drivenL * drivenL) * airGain;
        float excAirR = (drivenR + 0.35f * drivenR * std::abs(drivenR) - 0.15f * drivenR * drivenR * drivenR) * airGain;

        airEnergyAccum += (std::abs(excAirL) + std::abs(excAirR)) * 0.5f;

        // 3. Recombine: original signal + subharmonic bass + optical air
        outL[i] = sL * dry + (sL + subTone + excAirL) * wet;
        if (stereo) {
            outR[i] = sR * dry + (sR + subTone + excAirR) * wet;
        }
    }

    m_subEnergy = subEnergyAccum / static_cast<float>(numSamples);
    m_airEnergy = airEnergyAccum / static_cast<float>(numSamples);
}

} // namespace Jazz
