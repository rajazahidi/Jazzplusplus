//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// SpatialAudioEarCanalEqCalibrationEngine.h - Headphone Harman Curve Calibrator
//
// Headphone acoustic calibration & Harman target curve matching (SoundID Reference style):
// Flattens transducer non-linearities and optimizes headphones for Dolby Atmos
// 3D spatial binaural monitoring (HD650, DT990, M50x, AirPods Max).
//*****************************************************************************

#ifndef JAZZ_SPATIAL_AUDIO_EAR_CANAL_EQ_CALIBRATION_ENGINE_H
#define JAZZ_SPATIAL_AUDIO_EAR_CANAL_EQ_CALIBRATION_ENGINE_H

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

enum class HeadphoneTargetCurve {
    HARMAN_OVER_EAR_2018 = 0, // Industry standard listener preferred over-ear curve
    HARMAN_IN_EAR_2019   = 1, // In-ear monitor target with ear canal resonance compensation
    DIFFUSE_FIELD_FLAT   = 2, // Classical diffuse-field acoustic room reference
    FREE_FIELD_ANECHOIC  = 3  // Anechoic direct sound flat reference
};

enum class HeadphoneModelPreset {
    SENNHEISER_HD650 = 0, // Compensates low-bass roll-off, smooths 3.5 kHz
    BEYERDYNAMIC_DT990 = 1, // Tames 8.5 kHz treble spike, linearizes mids
    AUDIO_TECHNICA_M50X = 2, // Flattens boomy 120Hz bass, clears scooped mids
    APPLE_AIRPODS_MAX = 3, // Linearizes computational DSP response
    GENERIC_FLAT = 4
};

class JZSpatialAudioEarCanalEqCalibrationEngine {
public:
    static const size_t NUM_CALIBRATION_BANDS = 5;

    struct Parameters {
        HeadphoneTargetCurve target = HeadphoneTargetCurve::HARMAN_OVER_EAR_2018;
        HeadphoneModelPreset model = HeadphoneModelPreset::SENNHEISER_HD650;
        float targetBassTiltDb = 4.5f;       // Harman preferred bass lift (0.0 to 9.0 dB)
        float targetTrebleTiltDb = 0.0f;     // Treble brightness tilt (-6.0 to +6.0 dB)
        float leftEarTrimDb = 0.0f;          // Transducer balance trim L (-3.0 to +3.0 dB)
        float rightEarTrimDb = 0.0f;         // Transducer balance trim R (-3.0 to +3.0 dB)
        float dryWet = 1.0f;                 // Calibration blend (0.0 = uncalibrated, 1.0 = full)
        bool enabled = true;
    };

    struct CalibrationBiquad {
        float b0, b1, b2, a1, a2;
        float x1L, x2L, y1L, y2L;
        float x1R, x2R, y1R, y2R;

        void reset() {
            x1L = x2L = y1L = y2L = 0.0f;
            x1R = x2R = y1R = y2R = 0.0f;
        }
    };

    JZSpatialAudioEarCanalEqCalibrationEngine(float sampleRate = 48000.0f);
    ~JZSpatialAudioEarCanalEqCalibrationEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setTargetCurve(HeadphoneTargetCurve target);
    HeadphoneTargetCurve getTargetCurve() const { return m_params.target; }

    void setHeadphoneModel(HeadphoneModelPreset model);
    HeadphoneModelPreset getHeadphoneModel() const { return m_params.model; }

    void setBassTiltDb(float db);
    float getBassTiltDb() const { return m_params.targetBassTiltDb; }

    /**
     * @brief Process stereo audio buffer through headphone calibration filters.
     */
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

    std::string getTargetCurveName() const;
    std::string getHeadphoneModelName() const;
    float getMaxCorrectionDb() const { return m_maxCorrectionDb; }

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    CalibrationBiquad m_filters[NUM_CALIBRATION_BANDS];
    float m_maxCorrectionDb = 0.0f;

    void updateCalibrationCoefficients();
};

} // namespace Jazz

namespace jazz {
    using JZSpatialAudioEarCanalEqCalibrationEngine = Jazz::JZSpatialAudioEarCanalEqCalibrationEngine;
    using HeadphoneTargetCurve = Jazz::HeadphoneTargetCurve;
    using HeadphoneModelPreset = Jazz::HeadphoneModelPreset;
}

#endif // JAZZ_SPATIAL_AUDIO_EAR_CANAL_EQ_CALIBRATION_ENGINE_H
