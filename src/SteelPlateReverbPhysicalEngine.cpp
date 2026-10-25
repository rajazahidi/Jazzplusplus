//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// SteelPlateReverbPhysicalEngine.cpp - Physical Steel Plate Reverb (EMT 140)
//*****************************************************************************

#include "SteelPlateReverbPhysicalEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

JZSteelPlateReverbPhysicalEngine::JZSteelPlateReverbPhysicalEngine(float sampleRate) {
    initialize(sampleRate);
}

void JZSteelPlateReverbPhysicalEngine::initialize(float sampleRate) {
    if (sampleRate < 8000.0f) sampleRate = 48000.0f;
    m_sampleRate = sampleRate;

    reset();
    updatePlateModalMesh();
}

void JZSteelPlateReverbPhysicalEngine::reset() {
    for (size_t k = 0; k < NUM_PLATE_MODES; ++k) {
        m_modes[k].reset();
    }
    m_hpX1 = m_hpX2 = m_hpY1 = m_hpY2 = 0.0f;
}

void JZSteelPlateReverbPhysicalEngine::setParameters(const Parameters& params) {
    m_params = params;
    m_params.decayTimeSec = std::max(0.4f, std::min(6.0f, m_params.decayTimeSec));
    m_params.plateTension = std::max(0.5f, std::min(1.8f, m_params.plateTension));
    m_params.driverDrive = std::max(1.0f, std::min(4.0f, m_params.driverDrive));
    m_params.highFrequencyDamping = std::max(0.05f, std::min(0.95f, m_params.highFrequencyDamping));
    m_params.dryWet = std::max(0.0f, std::min(1.0f, m_params.dryWet));

    updatePlateModalMesh();
}

void JZSteelPlateReverbPhysicalEngine::setDecayTimeSec(float sec) {
    m_params.decayTimeSec = std::max(0.4f, std::min(6.0f, sec));
    updatePlateModalMesh();
}

void JZSteelPlateReverbPhysicalEngine::setDriverDrive(float drive) {
    m_params.driverDrive = std::max(1.0f, std::min(4.0f, drive));
}

void JZSteelPlateReverbPhysicalEngine::setPlateTension(float tension) {
    m_params.plateTension = std::max(0.5f, std::min(1.8f, tension));
    updatePlateModalMesh();
}

void JZSteelPlateReverbPhysicalEngine::updatePlateModalMesh() {
    // Mechanical damper pad distance from steel surface (mm)
    m_damperDistanceMm = m_params.decayTimeSec * 5.8f;

    // Nominal mode indices (m, n) for 2m x 1m rectangular cold-rolled plate
    static const int modeIndices[NUM_PLATE_MODES][2] = {
        { 1, 1 }, { 2, 1 }, { 1, 2 }, { 3, 1 },
        { 2, 2 }, { 3, 2 }, { 4, 1 }, { 1, 3 },
        { 4, 2 }, { 2, 3 }, { 5, 1 }, { 3, 3 },
        { 5, 2 }, { 4, 3 }, { 6, 1 }, { 5, 3 }
    };

    // Normalized pickup coordinates on the plate
    // Pickup L at (x=0.32, y=0.45), Pickup R at (x=0.74, y=0.62)
    const float xL = 0.32f, yL = 0.45f;
    const float xR = 0.74f, yR = 0.62f;

    float baseDispersionHz = 165.0f * m_params.plateTension;

    for (size_t k = 0; k < NUM_PLATE_MODES; ++k) {
        float m = static_cast<float>(modeIndices[k][0]);
        float n = static_cast<float>(modeIndices[k][1]);

        // Plate bending modal frequency: f_mn ~ (m^2 / Lx^2 + n^2 / Ly^2)
        float modalTerm = (m * m / 4.0f) + (n * n / 1.0f);
        float f0 = baseDispersionHz * std::sqrt(modalTerm);
        if (f0 >= m_sampleRate * 0.45f) f0 = m_sampleRate * 0.45f;
        if (f0 < 60.0f) f0 = 60.0f;

        m_modes[k].freqHz = f0;

        // Spatial mode shapes at pickup positions
        m_modes[k].pickupWeightL = std::sin(m * static_cast<float>(M_PI) * xL) *
                                   std::sin(n * static_cast<float>(M_PI) * yL);
        m_modes[k].pickupWeightR = std::sin(m * static_cast<float>(M_PI) * xR) *
                                   std::sin(n * static_cast<float>(M_PI) * yR);

        // Damper pad decay Q factor: Q = pi * f0 * T60 / ln(1000)
        // High frequency modes decay faster due to internal steel viscosity
        float hfLossFactor = 1.0f + (f0 / 3500.0f) * m_params.highFrequencyDamping;
        float modeT60 = m_params.decayTimeSec / hfLossFactor;
        float Q = (static_cast<float>(M_PI) * f0 * modeT60) / 6.907755f;
        if (Q < 5.0f) Q = 5.0f;
        if (Q > 450.0f) Q = 450.0f;

        // Bandpass resonator design
        float w0 = 2.0f * static_cast<float>(M_PI) * (f0 / m_sampleRate);
        float sin_w = std::sin(w0);
        float cos_w = std::cos(w0);
        float alpha = sin_w / (2.0f * Q);

        float a0 = 1.0f + alpha;
        m_modes[k].b0 = alpha / a0;
        m_modes[k].b1 = 0.0f;
        m_modes[k].b2 = -alpha / a0;
        m_modes[k].a1 = (-2.0f * cos_w) / a0;
        m_modes[k].a2 = (1.0f - alpha) / a0;
    }

    // Highpass pre-filter at bassCutoffHz
    float hpW0 = 2.0f * static_cast<float>(M_PI) * (m_params.bassCutoffHz / m_sampleRate);
    float hpAlpha = std::sin(hpW0) / (2.0f * 0.7071f);
    float hpCos = std::cos(hpW0);
    float hpA0 = 1.0f + hpAlpha;

    m_hpB0 = ((1.0f + hpCos) / 2.0f) / hpA0;
    m_hpB1 = (-(1.0f + hpCos)) / hpA0;
    m_hpB2 = ((1.0f + hpCos) / 2.0f) / hpA0;
    m_hpA1 = (-2.0f * hpCos) / hpA0;
    m_hpA2 = (1.0f - hpAlpha) / hpA0;
}

void JZSteelPlateReverbPhysicalEngine::process(
    const float* inL, const float* inR,
    float* outL, float* outR,
    size_t numSamples) {

    if (!inL || !outL || numSamples == 0) return;
    const bool stereo = (inR != nullptr && outR != nullptr);

    float drive = m_params.driverDrive;
    float wet = m_params.dryWet;
    float dry = 1.0f - wet;

    for (size_t i = 0; i < numSamples; ++i) {
        float monoIn = inL[i];
        if (stereo) monoIn = 0.5f * (monoIn + inR[i]);

        // 1. High-pass pre-filtering
        float hpIn = m_hpB0 * monoIn + m_hpB1 * m_hpX1 + m_hpB2 * m_hpX2
                     - m_hpA1 * m_hpY1 - m_hpA2 * m_hpY2;
        m_hpX2 = m_hpX1; m_hpX1 = monoIn;
        m_hpY2 = m_hpY1; m_hpY1 = hpIn;

        // 2. Driver coil non-linear saturation
        float drivenSignal = std::tanh(hpIn * drive);

        // 3. Excite 2D steel plate modal mesh
        float plateOutL = 0.0f;
        float plateOutR = 0.0f;

        for (size_t k = 0; k < NUM_PLATE_MODES; ++k) {
            PlateMode& mode = m_modes[k];
            float modeOut = mode.b0 * drivenSignal + mode.b1 * mode.x1 + mode.b2 * mode.x2
                            - mode.a1 * mode.y1 - mode.a2 * mode.y2;
            mode.x2 = mode.x1; mode.x1 = drivenSignal;
            mode.y2 = mode.y1; mode.y1 = modeOut;

            plateOutL += modeOut * mode.pickupWeightL;
            plateOutR += modeOut * mode.pickupWeightR;
        }

        // Wet / Dry blend
        outL[i] = inL[i] * dry + plateOutL * wet * 1.5f;
        if (stereo) {
            outR[i] = inR[i] * dry + plateOutR * wet * 1.5f;
        }
    }
}

} // namespace Jazz
