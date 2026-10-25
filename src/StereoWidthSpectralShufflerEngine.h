//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// StereoWidthSpectralShufflerEngine.h - Alan Blumlein Frequency-Dependent
// Stereo Shuffler & Polar Image Sculptor
//
// Studio mastering frequency-dependent stereo image manipulation (Blumlein Shuffling):
// Mid/Side matrixing with low-frequency Side bass boost below 700Hz for acoustic ITD
// spatial depth, sub-bass elliptical mono-maker (<120Hz), high-frequency Air expansion (>6kHz),
// and continuous broadcast-compliant phase correlation meter with anti-cancellation guard.
//*****************************************************************************

#ifndef JAZZ_STEREO_WIDTH_SPECTRAL_SHUFFLER_ENGINE_H
#define JAZZ_STEREO_WIDTH_SPECTRAL_SHUFFLER_ENGINE_H

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

class JZStereoWidthSpectralShufflerEngine {
public:
    struct Parameters {
        float shufflerGainDb = 3.0f;         // Low-frequency Blumlein shuffling boost (0.0 to +8.0 dB)
        float shufflerCrossoverHz = 700.0f;  // Blumlein shuffling crossover frequency (300.0 to 1200.0 Hz)
        float subMonoCutoffHz = 110.0f;      // Elliptical mono threshold (40.0 to 250.0 Hz)
        float airWidth = 1.25f;              // High-frequency 'Air' width factor (>6kHz) (0.5 to 2.5)
        float midGainDb = 0.0f;              // Mid channel level trim (-6.0 to +6.0 dB)
        float sideGainDb = 0.0f;             // Overall Side channel trim (-6.0 to +6.0 dB)
        bool phaseGuardEnabled = true;       // Automatic mono-cancellation phase protection
        bool bypass = false;
    };

    JZStereoWidthSpectralShufflerEngine(float sampleRate = 48000.0f);
    ~JZStereoWidthSpectralShufflerEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setShufflerGainDb(float db);
    float getShufflerGainDb() const { return m_params.shufflerGainDb; }

    void setShufflerCrossoverHz(float hz);
    float getShufflerCrossoverHz() const { return m_params.shufflerCrossoverHz; }

    void setSubMonoCutoffHz(float hz);
    float getSubMonoCutoffHz() const { return m_params.subMonoCutoffHz; }

    void setAirWidth(float width);
    float getAirWidth() const { return m_params.airWidth; }

    void setMidGainDb(float db);
    float getMidGainDb() const { return m_params.midGainDb; }

    void setSideGainDb(float db);
    float getSideGainDb() const { return m_params.sideGainDb; }

    void setPhaseGuardEnabled(bool enabled);
    bool isPhaseGuardEnabled() const { return m_params.phaseGuardEnabled; }

    void setBypass(bool bp);
    bool isBypassed() const { return m_params.bypass; }

    /**
     * @brief Process stereo audio buffer through Blumlein spectral shuffler.
     */
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

    // Telemetry
    float getPhaseCorrelation() const { return m_phaseCorrelation; }
    bool isPhaseWarning() const { return m_phaseWarning; }
    float getSideToMidRatioDb() const { return m_sideToMidRatioDb; }

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    float m_phaseCorrelation = 0.85f;
    bool m_phaseWarning = false;
    float m_sideToMidRatioDb = -6.0f;

    // Filters for Side channel processing:
    // 1. Shuffler low-shelf filter (boosting LF Side)
    float m_shuffB0 = 1.0f, m_shuffB1 = 0.0f, m_shuffB2 = 0.0f;
    float m_shuffA1 = 0.0f, m_shuffA2 = 0.0f;
    float m_shuffX1 = 0.0f, m_shuffX2 = 0.0f, m_shuffY1 = 0.0f, m_shuffY2 = 0.0f;

    // 2. Sub-bass highpass filter on Side channel (Elliptical Mono-maker)
    float m_monoB0 = 1.0f, m_monoB1 = -2.0f, m_monoB2 = 1.0f;
    float m_monoA1 = 0.0f, m_monoA2 = 0.0f;
    float m_monoX1 = 0.0f, m_monoX2 = 0.0f, m_monoY1 = 0.0f, m_monoY2 = 0.0f;

    // 3. High-shelf filter on Side channel (Air Width expansion)
    float m_airB0 = 1.0f, m_airB1 = 0.0f, m_airB2 = 0.0f;
    float m_airA1 = 0.0f, m_airA2 = 0.0f;
    float m_airX1 = 0.0f, m_airX2 = 0.0f, m_airY1 = 0.0f, m_airY2 = 0.0f;

    void updateFilterCoeffs();
};

} // namespace Jazz

namespace jazz {
    using JZStereoWidthSpectralShufflerEngine = Jazz::JZStereoWidthSpectralShufflerEngine;
}

#endif // JAZZ_STEREO_WIDTH_SPECTRAL_SHUFFLER_ENGINE_H
