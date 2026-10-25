//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// RotaFonicRotarySpeakerSimulator.cpp - Leslie 122/147 Rotary Speaker Simulator
//*****************************************************************************

#include "RotaFonicRotarySpeakerSimulator.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

JZRotaFonicRotarySpeakerSimulator::JZRotaFonicRotarySpeakerSimulator(float sampleRate) {
    initialize(sampleRate);
}

void JZRotaFonicRotarySpeakerSimulator::initialize(float sampleRate) {
    if (sampleRate < 8000.0f) sampleRate = 48000.0f;
    m_sampleRate = sampleRate;

    m_hornDelayBuf.assign(HORN_BUF_SIZE, 0.0f);
    reset();
    updateCrossover();
    updateTargetSpeeds();
}

void JZRotaFonicRotarySpeakerSimulator::reset() {
    std::fill(m_hornDelayBuf.begin(), m_hornDelayBuf.end(), 0.0f);
    m_hornWritePos = 0;

    m_xLpX1 = m_xLpX2 = m_xLpY1 = m_xLpY2 = 0.0f;
    m_xHpX1 = m_xHpX2 = m_xHpY1 = m_xHpY2 = 0.0f;

    m_hornAngleRad = 0.0f;
    m_drumAngleRad = 0.0f;
    m_currentHornFreqHz = 0.8f;
    m_currentDrumFreqHz = 0.7f;
}

void JZRotaFonicRotarySpeakerSimulator::setParameters(const Parameters& params) {
    m_params = params;
    m_params.drive = std::max(1.0f, std::min(4.5f, m_params.drive));
    m_params.hornDrumBalance = std::max(0.1f, std::min(0.9f, m_params.hornDrumBalance));
    m_params.dryWet = std::max(0.0f, std::min(1.0f, m_params.dryWet));
    updateTargetSpeeds();
}

void JZRotaFonicRotarySpeakerSimulator::setSpeedMode(RotarySpeedMode mode) {
    m_params.mode = mode;
    updateTargetSpeeds();
}

void JZRotaFonicRotarySpeakerSimulator::setDrive(float drive) {
    m_params.drive = std::max(1.0f, std::min(4.5f, drive));
}

void JZRotaFonicRotarySpeakerSimulator::setHornDrumBalance(float balance) {
    m_params.hornDrumBalance = std::max(0.1f, std::min(0.9f, balance));
}

void JZRotaFonicRotarySpeakerSimulator::updateTargetSpeeds() {
    switch (m_params.mode) {
        case RotarySpeedMode::BRAKE_STOP:
            m_targetHornFreqHz = 0.0f;
            m_targetDrumFreqHz = 0.0f;
            break;
        case RotarySpeedMode::CHORALE:
            m_targetHornFreqHz = 0.8f;  // ~48 RPM
            m_targetDrumFreqHz = 0.67f; // ~40 RPM
            break;
        case RotarySpeedMode::TREMOLO:
            m_targetHornFreqHz = 6.8f;  // ~408 RPM
            m_targetDrumFreqHz = 5.7f;  // ~342 RPM
            break;
    }
}

void JZRotaFonicRotarySpeakerSimulator::updateCrossover() {
    // 800 Hz Leslie Crossover
    float f0 = 800.0f;
    float w0 = 2.0f * static_cast<float>(M_PI) * (f0 / m_sampleRate);
    float sin_w = std::sin(w0);
    float cos_w = std::cos(w0);
    float alpha = sin_w / (2.0f * 0.7071f);
    float a0 = 1.0f + alpha;

    m_xoverB0 = ((1.0f - cos_w) / 2.0f) / a0;
    m_xoverB1 = (1.0f - cos_w) / a0;
    m_xoverB2 = ((1.0f - cos_w) / 2.0f) / a0;
    m_xoverA1 = (-2.0f * cos_w) / a0;
    m_xoverA2 = (1.0f - alpha) / a0;

    m_xoverHpB0 = ((1.0f + cos_w) / 2.0f) / a0;
    m_xoverHpB1 = (-(1.0f + cos_w)) / a0;
    m_xoverHpB2 = ((1.0f + cos_w) / 2.0f) / a0;
}

void JZRotaFonicRotarySpeakerSimulator::process(
    const float* inL, const float* inR,
    float* outL, float* outR,
    size_t numSamples) {

    if (!inL || !outL || numSamples == 0) return;
    const bool stereo = (inR != nullptr && outR != nullptr);

    float drive = m_params.drive;
    float hornBal = m_params.hornDrumBalance;
    float drumBal = 1.0f - hornBal;
    float wet = m_params.dryWet;
    float dry = 1.0f - wet;

    // Mic 2 angle offset
    float mic2AngleRad = m_params.micAngleDeg * (static_cast<float>(M_PI) / 180.0f);

    // Motor inertia coefficients (horn responds faster than heavy wooden drum)
    float hornInertia = 0.00035f;
    float drumInertia = 0.000085f;

    for (size_t i = 0; i < numSamples; ++i) {
        float sL = inL[i];
        float sR = stereo ? inR[i] : sL;
        float mono = 0.5f * (sL + sR);

        // 1. 12AX7 Tube Preamp Saturation
        float saturated = std::tanh(mono * drive);

        // 2. 800 Hz Crossover
        float drumIn = m_xoverB0 * saturated + m_xoverB1 * m_xLpX1 + m_xoverB2 * m_xLpX2
                       - m_xoverA1 * m_xLpY1 - m_xoverA2 * m_xLpY2;
        m_xLpX2 = m_xLpX1; m_xLpX1 = saturated;
        m_xLpY2 = m_xLpY1; m_xLpY1 = drumIn;

        float hornIn = m_xoverHpB0 * saturated + m_xoverHpB1 * m_xHpX1 + m_xoverHpB2 * m_xHpX2
                       - m_xoverA1 * m_xHpY1 - m_xoverA2 * m_xHpY2;
        m_xHpX2 = m_xHpX1; m_xHpX1 = saturated;
        m_xHpY2 = m_xHpY1; m_xHpY1 = hornIn;

        // 3. Horn Doppler Delay Line
        m_hornDelayBuf[m_hornWritePos] = hornIn;

        // Accelerate/Brake motors
        m_currentHornFreqHz += (m_targetHornFreqHz - m_currentHornFreqHz) * hornInertia;
        m_currentDrumFreqHz += (m_targetDrumFreqHz - m_currentDrumFreqHz) * drumInertia;

        m_hornAngleRad += (2.0f * static_cast<float>(M_PI) * m_currentHornFreqHz) / m_sampleRate;
        if (m_hornAngleRad >= 2.0f * static_cast<float>(M_PI)) m_hornAngleRad -= 2.0f * static_cast<float>(M_PI);

        m_drumAngleRad += (2.0f * static_cast<float>(M_PI) * m_currentDrumFreqHz) / m_sampleRate;
        if (m_drumAngleRad >= 2.0f * static_cast<float>(M_PI)) m_drumAngleRad -= 2.0f * static_cast<float>(M_PI);

        // Doppler pitch shift: delay time modulates with cos(angle)
        // Horn radius ~0.18m -> delta delay ~0.5ms (~24 samples at 48kHz)
        float dopplerSamples = 20.0f;
        float baseDelaySamples = 40.0f;

        // Mic 1 (Left)
        float delayL = baseDelaySamples + dopplerSamples * std::cos(m_hornAngleRad);
        float rPosL = static_cast<float>(m_hornWritePos) - delayL;
        while (rPosL < 0.0f) rPosL += static_cast<float>(HORN_BUF_SIZE);
        size_t idxL0 = static_cast<size_t>(rPosL) % HORN_BUF_SIZE;
        size_t idxL1 = (idxL0 + 1) % HORN_BUF_SIZE;
        float fracL = rPosL - std::floor(rPosL);
        float hornSampleL = m_hornDelayBuf[idxL0] * (1.0f - fracL) + m_hornDelayBuf[idxL1] * fracL;
        float hornAmpL = 0.5f * (1.0f + std::cos(m_hornAngleRad));

        // Mic 2 (Right)
        float hornAngleR = m_hornAngleRad + mic2AngleRad;
        float delayR = baseDelaySamples + dopplerSamples * std::cos(hornAngleR);
        float rPosR = static_cast<float>(m_hornWritePos) - delayR;
        while (rPosR < 0.0f) rPosR += static_cast<float>(HORN_BUF_SIZE);
        size_t idxR0 = static_cast<size_t>(rPosR) % HORN_BUF_SIZE;
        size_t idxR1 = (idxR0 + 1) % HORN_BUF_SIZE;
        float fracR = rPosR - std::floor(rPosR);
        float hornSampleR = m_hornDelayBuf[idxR0] * (1.0f - fracR) + m_hornDelayBuf[idxR1] * fracR;
        float hornAmpR = 0.5f * (1.0f + std::cos(hornAngleR));

        float totalHornL = hornSampleL * hornAmpL;
        float totalHornR = hornSampleR * hornAmpR;

        // 4. Drum Amplitude Modulation
        float drumAmpL = 1.0f + 0.35f * std::cos(m_drumAngleRad);
        float drumAmpR = 1.0f + 0.35f * std::cos(m_drumAngleRad + mic2AngleRad);
        float totalDrumL = drumIn * drumAmpL;
        float totalDrumR = drumIn * drumAmpR;

        // Recombine cabinet audio
        float cabinetL = totalHornL * hornBal + totalDrumL * drumBal;
        float cabinetR = totalHornR * hornBal + totalDrumR * drumBal;

        outL[i] = sL * dry + cabinetL * wet;
        if (stereo) {
            outR[i] = sR * dry + cabinetR * wet;
        }

        m_hornWritePos = (m_hornWritePos + 1) % HORN_BUF_SIZE;
    }
}

} // namespace Jazz
