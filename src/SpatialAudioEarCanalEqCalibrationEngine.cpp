//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// SpatialAudioEarCanalEqCalibrationEngine.cpp - Headphone Harman Curve Calibrator
//*****************************************************************************

#include "SpatialAudioEarCanalEqCalibrationEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

JZSpatialAudioEarCanalEqCalibrationEngine::JZSpatialAudioEarCanalEqCalibrationEngine(float sampleRate) {
    initialize(sampleRate);
}

void JZSpatialAudioEarCanalEqCalibrationEngine::initialize(float sampleRate) {
    if (sampleRate < 8000.0f) sampleRate = 48000.0f;
    m_sampleRate = sampleRate;

    reset();
    updateCalibrationCoefficients();
}

void JZSpatialAudioEarCanalEqCalibrationEngine::reset() {
    for (size_t b = 0; b < NUM_CALIBRATION_BANDS; ++b) {
        m_filters[b].reset();
    }
    m_maxCorrectionDb = 0.0f;
}

void JZSpatialAudioEarCanalEqCalibrationEngine::setParameters(const Parameters& params) {
    m_params = params;
    m_params.targetBassTiltDb = std::max(-6.0f, std::min(12.0f, m_params.targetBassTiltDb));
    m_params.targetTrebleTiltDb = std::max(-8.0f, std::min(8.0f, m_params.targetTrebleTiltDb));
    m_params.leftEarTrimDb = std::max(-6.0f, std::min(6.0f, m_params.leftEarTrimDb));
    m_params.rightEarTrimDb = std::max(-6.0f, std::min(6.0f, m_params.rightEarTrimDb));
    m_params.dryWet = std::max(0.0f, std::min(1.0f, m_params.dryWet));

    updateCalibrationCoefficients();
}

void JZSpatialAudioEarCanalEqCalibrationEngine::setTargetCurve(HeadphoneTargetCurve target) {
    m_params.target = target;
    updateCalibrationCoefficients();
}

void JZSpatialAudioEarCanalEqCalibrationEngine::setHeadphoneModel(HeadphoneModelPreset model) {
    m_params.model = model;
    updateCalibrationCoefficients();
}

void JZSpatialAudioEarCanalEqCalibrationEngine::setBassTiltDb(float db) {
    m_params.targetBassTiltDb = std::max(-6.0f, std::min(12.0f, db));
    updateCalibrationCoefficients();
}

std::string JZSpatialAudioEarCanalEqCalibrationEngine::getTargetCurveName() const {
    switch (m_params.target) {
        case HeadphoneTargetCurve::HARMAN_OVER_EAR_2018: return "Harman Target 2018 (Over-Ear)";
        case HeadphoneTargetCurve::HARMAN_IN_EAR_2019:   return "Harman Target 2019 (In-Ear)";
        case HeadphoneTargetCurve::DIFFUSE_FIELD_FLAT:   return "Diffuse-Field Flat";
        case HeadphoneTargetCurve::FREE_FIELD_ANECHOIC:  return "Free-Field Anechoic";
    }
    return "Custom Target";
}

std::string JZSpatialAudioEarCanalEqCalibrationEngine::getHeadphoneModelName() const {
    switch (m_params.model) {
        case HeadphoneModelPreset::SENNHEISER_HD650:   return "Sennheiser HD650 / HD600";
        case HeadphoneModelPreset::BEYERDYNAMIC_DT990: return "Beyerdynamic DT990 Pro";
        case HeadphoneModelPreset::AUDIO_TECHNICA_M50X:return "Audio-Technica ATH-M50x";
        case HeadphoneModelPreset::APPLE_AIRPODS_MAX:  return "Apple AirPods Max";
        case HeadphoneModelPreset::GENERIC_FLAT:       return "Generic Flat Reference";
    }
    return "Generic";
}

void JZSpatialAudioEarCanalEqCalibrationEngine::updateCalibrationCoefficients() {
    // 5 Calibration band definitions: CenterFreq, Q, GainDb
    float freqs[NUM_CALIBRATION_BANDS] = { 75.0f, 220.0f, 1200.0f, 3200.0f, 8500.0f };
    float Qs[NUM_CALIBRATION_BANDS]    = { 0.7071f, 1.2f, 1.4f, 2.0f, 0.7071f };
    float gainsDb[NUM_CALIBRATION_BANDS] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    // 1. Model-specific calibration corrections
    switch (m_params.model) {
        case HeadphoneModelPreset::SENNHEISER_HD650:
            gainsDb[0] += 4.5f; // Lift rolled-off sub-bass
            gainsDb[1] -= 1.0f; // Clean up 200 Hz warmth
            gainsDb[3] += 1.5f; // Enhance 3.2 kHz ear canal gain
            gainsDb[4] += 1.0f; // Airy extension
            break;
        case HeadphoneModelPreset::BEYERDYNAMIC_DT990:
            gainsDb[0] += 2.0f; // Sub-bass fill
            gainsDb[2] += 2.0f; // Bring forward recessed midrange
            gainsDb[4] -= 5.5f; // Tame notorious 8.5 kHz treble spike
            break;
        case HeadphoneModelPreset::AUDIO_TECHNICA_M50X:
            gainsDb[0] -= 2.5f; // Tame boomy 100 Hz punch
            gainsDb[1] -= 2.0f; // Clean up muddy lower-mids
            gainsDb[2] += 2.5f; // Fill scooped 1.2 kHz vocal pocket
            break;
        case HeadphoneModelPreset::APPLE_AIRPODS_MAX:
            gainsDb[0] -= 1.5f; // Tighten over-boosted bass
            gainsDb[3] += 1.0f; // Clarify spatial elevation
            break;
        case HeadphoneModelPreset::GENERIC_FLAT:
            break;
    }

    // 2. Harman Target adjustments
    if (m_params.target == HeadphoneTargetCurve::HARMAN_OVER_EAR_2018 ||
        m_params.target == HeadphoneTargetCurve::HARMAN_IN_EAR_2019) {
        gainsDb[0] += m_params.targetBassTiltDb;
        gainsDb[4] += m_params.targetTrebleTiltDb;
        if (m_params.target == HeadphoneTargetCurve::HARMAN_IN_EAR_2019) {
            gainsDb[3] += 2.0f; // Extra pinna/canal gain for in-ears
        }
    }

    float maxG = 0.0f;
    for (size_t b = 0; b < NUM_CALIBRATION_BANDS; ++b) {
        float g = gainsDb[b];
        if (std::abs(g) > maxG) maxG = std::abs(g);

        float f0 = freqs[b];
        if (f0 >= m_sampleRate * 0.45f) f0 = m_sampleRate * 0.45f;
        float A = std::pow(10.0f, g / 40.0f);
        float w0 = 2.0f * static_cast<float>(M_PI) * (f0 / m_sampleRate);
        float alpha = std::sin(w0) / (2.0f * Qs[b]);
        float cos_w = std::cos(w0);

        // Peaking biquad
        float b0 = 1.0f + alpha * A;
        float b1 = -2.0f * cos_w;
        float b2 = 1.0f - alpha * A;
        float a0 = 1.0f + alpha / A;
        float a1 = -2.0f * cos_w;
        float a2 = 1.0f - alpha / A;

        m_filters[b].b0 = b0 / a0;
        m_filters[b].b1 = b1 / a0;
        m_filters[b].b2 = b2 / a0;
        m_filters[b].a1 = a1 / a0;
        m_filters[b].a2 = a2 / a0;
    }

    m_maxCorrectionDb = maxG;
}

void JZSpatialAudioEarCanalEqCalibrationEngine::process(
    const float* inL, const float* inR,
    float* outL, float* outR,
    size_t numSamples) {

    if (!inL || !outL || numSamples == 0) return;
    const bool stereo = (inR != nullptr && outR != nullptr);

    if (!m_params.enabled) {
        for (size_t i = 0; i < numSamples; ++i) {
            outL[i] = inL[i];
            if (stereo) outR[i] = inR[i];
        }
        return;
    }

    float wet = m_params.dryWet;
    float dry = 1.0f - wet;
    float trimGainL = std::pow(10.0f, m_params.leftEarTrimDb / 20.0f);
    float trimGainR = std::pow(10.0f, m_params.rightEarTrimDb / 20.0f);

    for (size_t i = 0; i < numSamples; ++i) {
        float sL = inL[i];
        float sR = stereo ? inR[i] : sL;

        // Cascade through 5 calibration biquad bands
        for (size_t b = 0; b < NUM_CALIBRATION_BANDS; ++b) {
            CalibrationBiquad& f = m_filters[b];

            float yL = f.b0 * sL + f.b1 * f.x1L + f.b2 * f.x2L
                       - f.a1 * f.y1L - f.a2 * f.y2L;
            f.x2L = f.x1L; f.x1L = sL;
            f.y2L = f.y1L; f.y1L = yL;
            sL = yL;

            if (stereo) {
                float yR = f.b0 * sR + f.b1 * f.x1R + f.b2 * f.x2R
                           - f.a1 * f.y1R - f.a2 * f.y2R;
                f.x2R = f.x1R; f.x1R = sR;
                f.y2R = f.y1R; f.y1R = yR;
                sR = yR;
            }
        }

        sL *= trimGainL;
        if (stereo) sR *= trimGainR;

        outL[i] = inL[i] * dry + sL * wet;
        if (stereo) {
            outR[i] = inR[i] * dry + sR * wet;
        }
    }
}

} // namespace Jazz
