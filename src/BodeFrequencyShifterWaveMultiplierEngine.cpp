//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// BodeFrequencyShifterWaveMultiplierEngine.cpp - Bode Shifter & Serge Multiplier
//*****************************************************************************

#include "BodeFrequencyShifterWaveMultiplierEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

JZBodeFrequencyShifterWaveMultiplierEngine::JZBodeFrequencyShifterWaveMultiplierEngine(float sampleRate) {
    initialize(sampleRate);
}

void JZBodeFrequencyShifterWaveMultiplierEngine::initialize(float sampleRate) {
    if (sampleRate < 8000.0f) sampleRate = 48000.0f;
    m_sampleRate = sampleRate;
    reset();
}

void JZBodeFrequencyShifterWaveMultiplierEngine::reset() {
    m_apI1_L = m_apI2_L = m_apI3_L = m_apI4_L = 0.0f;
    m_apQ1_L = m_apQ2_L = m_apQ3_L = m_apQ4_L = 0.0f;
    m_apI1_R = m_apI2_R = m_apI3_R = m_apI4_R = 0.0f;
    m_apQ1_R = m_apQ2_R = m_apQ3_R = m_apQ4_R = 0.0f;

    m_lfoPhase = 0.0f;
    m_feedbackStateL = 0.0f;
    m_feedbackStateR = 0.0f;
}

void JZBodeFrequencyShifterWaveMultiplierEngine::setParameters(const Parameters& params) {
    m_params = params;
    m_params.shiftHz = std::max(-5000.0f, std::min(5000.0f, m_params.shiftHz));
    m_params.waveFoldAmount = std::max(1.0f, std::min(8.0f, m_params.waveFoldAmount));
    m_params.feedback = std::max(-0.85f, std::min(0.85f, m_params.feedback));
    m_params.dryWet = std::max(0.0f, std::min(1.0f, m_params.dryWet));
}

void JZBodeFrequencyShifterWaveMultiplierEngine::setFrequencyShiftHz(float hz) {
    m_params.shiftHz = std::max(-5000.0f, std::min(5000.0f, hz));
}

void JZBodeFrequencyShifterWaveMultiplierEngine::setWaveFoldAmount(float amount) {
    m_params.waveFoldAmount = std::max(1.0f, std::min(8.0f, amount));
}

void JZBodeFrequencyShifterWaveMultiplierEngine::setSidebandMode(ShiftSidebandMode mode) {
    m_params.mode = mode;
}

void JZBodeFrequencyShifterWaveMultiplierEngine::hilbertSplit(
    float in, float& outI, float& outQ,
    float& i1, float& i2, float& i3, float& i4,
    float& q1, float& q2, float& q3, float& q4) {

    // 4-stage Weaver Allpass coefficients for 90-degree phase quadrature
    static const float cI[4] = { 0.161758f, 0.733029f, 0.945350f, 0.990598f };
    static const float cQ[4] = { 0.479401f, 0.876218f, 0.976599f, 0.997500f };

    // In-Phase Path (I)
    float inI = in;
    float y = cI[0] * (inI - i1) + i1; i1 = inI; inI = y;
    y = cI[1] * (inI - i2) + i2; i2 = inI; inI = y;
    y = cI[2] * (inI - i3) + i3; i3 = inI; inI = y;
    y = cI[3] * (inI - i4) + i4; i4 = inI;
    outI = y;

    // Quadrature Path (Q, 90-degree phase offset)
    float inQ = in;
    y = cQ[0] * (inQ - q1) + q1; q1 = inQ; inQ = y;
    y = cQ[1] * (inQ - q2) + q2; q2 = inQ; inQ = y;
    y = cQ[2] * (inQ - q3) + q3; q3 = inQ; inQ = y;
    y = cQ[3] * (inQ - q4) + q4; q4 = inQ;
    outQ = y;
}

float JZBodeFrequencyShifterWaveMultiplierEngine::applySergeWaveFold(float x, float foldAmount) {
    if (foldAmount <= 1.01f) return x;
    // Serge non-linear diode wave folding transfer curve
    float scaled = x * foldAmount;
    return std::sin(scaled * 1.5707963f) / (1.0f + 0.2f * (foldAmount - 1.0f));
}

void JZBodeFrequencyShifterWaveMultiplierEngine::process(
    const float* inL, const float* inR,
    float* outL, float* outR,
    size_t numSamples) {

    if (!inL || !outL || numSamples == 0) return;
    const bool stereo = (inR != nullptr && outR != nullptr);

    float phaseInc = 2.0f * static_cast<float>(M_PI) * (m_params.shiftHz / m_sampleRate);

    for (size_t i = 0; i < numSamples; ++i) {
        float sampleL = inL[i] + m_feedbackStateL * m_params.feedback;
        float sampleR = (stereo ? inR[i] : sampleL) + m_feedbackStateR * m_params.feedback;

        // Step 1: Hilbert 90-degree quadrature phase split
        float outI_L = 0.0f, outQ_L = 0.0f;
        hilbertSplit(sampleL, outI_L, outQ_L,
                     m_apI1_L, m_apI2_L, m_apI3_L, m_apI4_L,
                     m_apQ1_L, m_apQ2_L, m_apQ3_L, m_apQ4_L);

        float outI_R = outI_L, outQ_R = outQ_L;
        if (stereo) {
            hilbertSplit(sampleR, outI_R, outQ_R,
                         m_apI1_R, m_apI2_R, m_apI3_R, m_apI4_R,
                         m_apQ1_R, m_apQ2_R, m_apQ3_R, m_apQ4_R);
        }

        // Step 2: Complex quadrature carrier modulation
        float cosCar = std::cos(m_lfoPhase);
        float sinCar = std::sin(m_lfoPhase);

        float upperL = outI_L * cosCar - outQ_L * sinCar;
        float lowerL = outI_L * cosCar + outQ_L * sinCar;

        float upperR = outI_R * cosCar - outQ_R * sinCar;
        float lowerR = outI_R * cosCar + outQ_R * sinCar;

        m_lfoPhase += phaseInc;
        if (m_lfoPhase > 2.0f * static_cast<float>(M_PI)) m_lfoPhase -= 2.0f * static_cast<float>(M_PI);
        else if (m_lfoPhase < -2.0f * static_cast<float>(M_PI)) m_lfoPhase += 2.0f * static_cast<float>(M_PI);

        // Step 3: Select Sideband Output
        float shiftedL = upperL;
        float shiftedR = upperR;

        if (m_params.mode == ShiftSidebandMode::LOWER_SIDEBAND) {
            shiftedL = lowerL;
            shiftedR = lowerR;
        } else if (m_params.mode == ShiftSidebandMode::STEREO_SPLIT) {
            shiftedL = upperL; // Left shifts up
            shiftedR = lowerR; // Right shifts down
        }

        // Step 4: Serge Non-Linear Wave Folding Multiplication
        float foldedL = applySergeWaveFold(shiftedL, m_params.waveFoldAmount);
        float foldedR = applySergeWaveFold(shiftedR, m_params.waveFoldAmount);

        m_feedbackStateL = foldedL;
        m_feedbackStateR = foldedR;

        // Step 5: Output mix
        outL[i] = inL[i] * (1.0f - m_params.dryWet) + foldedL * m_params.dryWet;
        if (stereo) {
            outR[i] = inR[i] * (1.0f - m_params.dryWet) + foldedR * m_params.dryWet;
        }
    }
}

} // namespace Jazz
