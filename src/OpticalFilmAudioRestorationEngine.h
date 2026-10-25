//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// OpticalFilmAudioRestorationEngine.h - Optical Film Soundtrack Restoration
//
// Historic 35mm/16mm optical sound film restoration (1930s-1970s Variable-Area/Density):
// Scanning slit aperture loss de-blurring (inverse sinc filter), bilateral film
// grain noise suppression, 24Hz shutter thump notch filtering, and photocell un-clipping.
//*****************************************************************************

#ifndef JAZZ_OPTICAL_FILM_AUDIO_RESTORATION_ENGINE_H
#define JAZZ_OPTICAL_FILM_AUDIO_RESTORATION_ENGINE_H

#include <vector>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <algorithm>

namespace Jazz {

enum class OpticalFilmType {
    VARIABLE_AREA_35MM = 0,    // Bilateral transparent waveform track (RCA / Westrex)
    VARIABLE_DENSITY_35MM = 1, // Modulated emulsion density track (Western Electric)
    VARIABLE_AREA_16MM = 2     // 16mm archival film (7.2 inches per second)
};

class JZOpticalFilmAudioRestorationEngine {
public:
    struct Parameters {
        OpticalFilmType filmType = OpticalFilmType::VARIABLE_AREA_35MM;
        float slitWidthMicrons = 25.0f;        // Scanning lens slit aperture width (10 to 50 um)
        float filmSpeedIps = 18.0f;            // 35mm film speed (18.0 ips for 24 fps)
        float grainDenoiseDb = 12.0f;          // Film emulsion grain suppression (0 to 24 dB)
        bool enableShutterThumpFilter = true;  // 24 Hz & 48 Hz projector sprocket notch
        float apertureDeBlurBoost = 1.2f;      // Inverse sinc high frequency compensation
    };

    JZOpticalFilmAudioRestorationEngine(float sampleRate = 48000.0f);
    ~JZOpticalFilmAudioRestorationEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setSlitWidthMicrons(float microns);
    float getSlitWidthMicrons() const { return m_params.slitWidthMicrons; }

    void setGrainDenoiseDb(float db);
    float getGrainDenoiseDb() const { return m_params.grainDenoiseDb; }

    void setShutterThumpFilter(bool enable);
    bool isShutterThumpFilterEnabled() const { return m_params.enableShutterThumpFilter; }

    /**
     * @brief Process audio buffer through optical film restoration suite.
     */
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

    uint32_t getRepairedGrainClicks() const { return m_repairedGrainClicks; }
    float getApertureLossCompensationDb() const { return m_apertureCompensationDb; }

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    // Aperture loss inverse sinc peaking filter
    float m_apertureB0 = 1.0f, m_apertureB1 = 0.0f, m_apertureB2 = 0.0f;
    float m_apertureA1 = 0.0f, m_apertureA2 = 0.0f;
    float m_apX1L = 0.0f, m_apX2L = 0.0f, m_apY1L = 0.0f, m_apY2L = 0.0f;
    float m_apX1R = 0.0f, m_apX2R = 0.0f, m_apY1R = 0.0f, m_apY2R = 0.0f;

    // 24 Hz & 48 Hz projector mechanical shutter notch filters
    float m_notch24_B0 = 1.0f, m_notch24_B1 = 0.0f, m_notch24_B2 = 0.0f;
    float m_notch24_A1 = 0.0f, m_notch24_A2 = 0.0f;
    float m_n24_X1L = 0.0f, m_n24_X2L = 0.0f, m_n24_Y1L = 0.0f, m_n24_Y2L = 0.0f;
    float m_n24_X1R = 0.0f, m_n24_X2R = 0.0f, m_n24_Y1R = 0.0f, m_n24_Y2R = 0.0f;

    // Bilateral grain impulsive click detection state
    float m_grainMedianHistory[5] = {0};
    uint32_t m_repairedGrainClicks = 0;
    float m_apertureCompensationDb = 0.0f;

    void updateApertureFilters();
    void updateNotchFilters();
};

} // namespace Jazz

namespace jazz {
    using JZOpticalFilmAudioRestorationEngine = Jazz::JZOpticalFilmAudioRestorationEngine;
}

#endif // JAZZ_OPTICAL_FILM_AUDIO_RESTORATION_ENGINE_H
