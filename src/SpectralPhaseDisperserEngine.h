//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// SpectralPhaseDisperserEngine.h - Continuous Frequency-Dependent All-Pass
// Phase Dispersion & Multi-Mic Coherence Maximizer
//
// Frequency-dependent continuous phase rotation (0 to 360 degrees) and
// multi-microphone drum/room acoustic coherence optimization:
// 4-band cascaded all-pass phase rotation matrix, group-delay dispersion,
// comb-filter cancellation mitigation, and stereo phase correlation telemetry.
//*****************************************************************************

#ifndef JAZZ_SPECTRAL_PHASE_DISPERSER_ENGINE_H
#define JAZZ_SPECTRAL_PHASE_DISPERSER_ENGINE_H

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <array>
#include <algorithm>

namespace Jazz {

class JZSpectralPhaseDisperserEngine {
public:
    static constexpr size_t NUM_BANDS = 4;

    struct Parameters {
        std::array<float, NUM_BANDS> bandPhaseDegrees = {0.0f, 90.0f, 180.0f, 270.0f}; // 0 to 360 deg
        std::array<float, NUM_BANDS - 1> crossoverFrequencies = {120.0f, 650.0f, 3200.0f}; // Crossovers Hz
        float dispersionAmount = 1.0f;       // Overall dispersion scaling (0.0 to 2.0)
        float highFreqSmearMs = 0.5f;        // High-frequency phase delay smear (0.0 to 5.0 ms)
        bool crossChannelCoupling = false;   // Apply inverse dispersion to R for stereo widening
        bool bypass = false;
    };

    JZSpectralPhaseDisperserEngine(float sampleRate = 48000.0f);
    ~JZSpectralPhaseDisperserEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setBandPhaseDegrees(size_t bandIndex, float degrees);
    float getBandPhaseDegrees(size_t bandIndex) const;

    void setCrossovers(float xov1, float xov2, float xov3);
    void setDispersionAmount(float amt);
    float getDispersionAmount() const { return m_params.dispersionAmount; }

    void setCrossChannelCoupling(bool enabled);
    bool isCrossChannelCoupled() const { return m_params.crossChannelCoupling; }

    void setBypass(bool bp);
    bool isBypassed() const { return m_params.bypass; }

    /**
     * @brief Process stereo audio buffer through spectral phase disperser.
     */
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

    // Telemetry
    float getPhaseCorrelation() const { return m_phaseCorrelation; }
    float getEffectiveDispersionDegrees(size_t bandIndex) const;

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    float m_phaseCorrelation = 0.95f;

    // 2nd-order all-pass filter stages for each band (Left and Right channels)
    struct AllPassStage {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
        float a1 = 0.0f, a2 = 0.0f;
        float x1L = 0.0f, x2L = 0.0f, y1L = 0.0f, y2L = 0.0f;
        float x1R = 0.0f, x2R = 0.0f, y1R = 0.0f, y2R = 0.0f;
    };
    std::array<AllPassStage, NUM_BANDS> m_stages;

    void updateFilterStages();
};

} // namespace Jazz

namespace jazz {
    using JZSpectralPhaseDisperserEngine = Jazz::JZSpectralPhaseDisperserEngine;
}

#endif // JAZZ_SPECTRAL_PHASE_DISPERSER_ENGINE_H
