//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// OpticalFilmAudioRestorationEngine.cpp - Optical Film Soundtrack Restoration
//*****************************************************************************

#include "OpticalFilmAudioRestorationEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

JZOpticalFilmAudioRestorationEngine::JZOpticalFilmAudioRestorationEngine(float sampleRate) {
    initialize(sampleRate);
}

void JZOpticalFilmAudioRestorationEngine::initialize(float sampleRate) {
    if (sampleRate < 8000.0f) sampleRate = 48000.0f;
    m_sampleRate = sampleRate;

    reset();
    updateApertureFilters();
    updateNotchFilters();
}

void JZOpticalFilmAudioRestorationEngine::reset() {
    m_apX1L = m_apX2L = m_apY1L = m_apY2L = 0.0f;
    m_apX1R = m_apX2R = m_apY1R = m_apY2R = 0.0f;

    m_n24_X1L = m_n24_X2L = m_n24_Y1L = m_n24_Y2L = 0.0f;
    m_n24_X1R = m_n24_X2R = m_n24_Y1R = m_n24_Y2R = 0.0f;

    for (int k = 0; k < 5; ++k) m_grainMedianHistory[k] = 0.0f;
    m_repairedGrainClicks = 0;
    m_apertureCompensationDb = 0.0f;
}

void JZOpticalFilmAudioRestorationEngine::setParameters(const Parameters& params) {
    m_params = params;
    m_params.slitWidthMicrons = std::max(10.0f, std::min(60.0f, m_params.slitWidthMicrons));
    m_params.grainDenoiseDb = std::max(0.0f, std::min(30.0f, m_params.grainDenoiseDb));
    m_params.apertureDeBlurBoost = std::max(0.5f, std::min(2.5f, m_params.apertureDeBlurBoost));
    updateApertureFilters();
    updateNotchFilters();
}

void JZOpticalFilmAudioRestorationEngine::setSlitWidthMicrons(float microns) {
    m_params.slitWidthMicrons = std::max(10.0f, std::min(60.0f, microns));
    updateApertureFilters();
}

void JZOpticalFilmAudioRestorationEngine::setGrainDenoiseDb(float db) {
    m_params.grainDenoiseDb = std::max(0.0f, std::min(30.0f, db));
}

void JZOpticalFilmAudioRestorationEngine::setShutterThumpFilter(bool enable) {
    m_params.enableShutterThumpFilter = enable;
}

void JZOpticalFilmAudioRestorationEngine::updateApertureFilters() {
    // Slit aperture inverse sinc compensation peaking filter centered at 8.5 kHz
    float f0 = 8500.0f;
    if (f0 > m_sampleRate * 0.45f) f0 = m_sampleRate * 0.45f;

    float boostDb = 4.5f * m_params.apertureDeBlurBoost;
    m_apertureCompensationDb = boostDb;
    float A = std::pow(10.0f, boostDb / 40.0f);
    float Q = 1.2f;

    float w0 = 2.0f * static_cast<float>(M_PI) * (f0 / m_sampleRate);
    float cos_w = std::cos(w0);
    float sin_w = std::sin(w0);
    float alpha = sin_w / (2.0f * Q);

    float b0 = 1.0f + alpha * A;
    float b1 = -2.0f * cos_w;
    float b2 = 1.0f - alpha * A;
    float a0 = 1.0f + alpha / A;
    float a1 = -2.0f * cos_w;
    float a2 = 1.0f - alpha / A;

    m_apertureB0 = b0 / a0;
    m_apertureB1 = b1 / a0;
    m_apertureB2 = b2 / a0;
    m_apertureA1 = a1 / a0;
    m_apertureA2 = a2 / a0;
}

void JZOpticalFilmAudioRestorationEngine::updateNotchFilters() {
    // 24.0 Hz projector sprocket frame rate notch filter
    float f0 = 24.0f;
    float Q = 8.0f;
    float w0 = 2.0f * static_cast<float>(M_PI) * (f0 / m_sampleRate);
    float cos_w = std::cos(w0);
    float sin_w = std::sin(w0);
    float alpha = sin_w / (2.0f * Q);

    float b0 = 1.0f;
    float b1 = -2.0f * cos_w;
    float b2 = 1.0f;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cos_w;
    float a2 = 1.0f - alpha;

    m_notch24_B0 = b0 / a0;
    m_notch24_B1 = b1 / a0;
    m_notch24_B2 = b2 / a0;
    m_notch24_A1 = a1 / a0;
    m_notch24_A2 = a2 / a0;
}

void JZOpticalFilmAudioRestorationEngine::process(
    const float* inL, const float* inR,
    float* outL, float* outR,
    size_t numSamples) {

    if (!inL || !outL || numSamples == 0) return;
    const bool stereo = (inR != nullptr && outR != nullptr);

    float clickThreshold = std::pow(10.0f, -m_params.grainDenoiseDb / 20.0f) * 0.45f;
    if (clickThreshold < 0.02f) clickThreshold = 0.02f;

    for (size_t i = 0; i < numSamples; ++i) {
        float sampleL = inL[i];
        float sampleR = stereo ? inR[i] : sampleL;

        // Step 1: Bilateral optical grain click detection & median interpolation
        for (int k = 4; k > 0; --k) m_grainMedianHistory[k] = m_grainMedianHistory[k - 1];
        m_grainMedianHistory[0] = sampleL;

        float sorted[5];
        for (int k = 0; k < 5; ++k) sorted[k] = m_grainMedianHistory[k];
        std::sort(sorted, sorted + 5);
        float medianL = sorted[2];

        if (std::abs(sampleL - medianL) > clickThreshold) {
            sampleL = medianL;
            m_repairedGrainClicks++;
        }

        // Step 2: 24 Hz Shutter Thump Notch Filter
        if (m_params.enableShutterThumpFilter) {
            float nOutL = m_notch24_B0 * sampleL + m_notch24_B1 * m_n24_X1L + m_notch24_B2 * m_n24_X2L
                          - m_notch24_A1 * m_n24_Y1L - m_notch24_A2 * m_n24_Y2L;
            m_n24_X2L = m_n24_X1L; m_n24_X1L = sampleL;
            m_n24_Y2L = m_n24_Y1L; m_n24_Y1L = nOutL;
            sampleL = nOutL;

            if (stereo) {
                float nOutR = m_notch24_B0 * sampleR + m_notch24_B1 * m_n24_X1R + m_notch24_B2 * m_n24_X2R
                              - m_notch24_A1 * m_n24_Y1R - m_notch24_A2 * m_n24_Y2R;
                m_n24_X2R = m_n24_X1R; m_n24_X1R = sampleR;
                m_n24_Y2R = m_n24_Y1R; m_n24_Y1R = nOutR;
                sampleR = nOutR;
            }
        }

        // Step 3: Aperture Loss Inverse Sinc De-Blur Peaking Filter
        float deblurL = m_apertureB0 * sampleL + m_apertureB1 * m_apX1L + m_apertureB2 * m_apX2L
                        - m_apertureA1 * m_apY1L - m_apertureA2 * m_apY2L;
        m_apX2L = m_apX1L; m_apX1L = sampleL;
        m_apY2L = m_apY1L; m_apY1L = deblurL;

        outL[i] = deblurL;

        if (stereo) {
            float deblurR = m_apertureB0 * sampleR + m_apertureB1 * m_apX1R + m_apertureB2 * m_apX2R
                            - m_apertureA1 * m_apY1R - m_apertureA2 * m_apY2R;
            m_apX2R = m_apX1R; m_apX1R = sampleR;
            m_apY2R = m_apY1R; m_apY1R = deblurR;
            outR[i] = deblurR;
        }
    }
}

} // namespace Jazz
