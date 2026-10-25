//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// SpectralSidechainUnmaskerEngine.h - Multi-Bus Dynamic Spectral Unmasker
//
// Real-time psychoacoustic 24-band Bark-scale dynamic spectral unmasking:
// Carves conflicting harmonic resonances in the main track using the sidechain
// signal (vocal/lead) without audible ducking or volume loss (smart:EQ/Soothe style).
//*****************************************************************************

#ifndef JAZZ_SPECTRAL_SIDECHAIN_UNMASKER_ENGINE_H
#define JAZZ_SPECTRAL_SIDECHAIN_UNMASKER_ENGINE_H

#include <vector>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

class JZSpectralSidechainUnmaskerEngine {
public:
    struct Parameters {
        float unmaskingDepthDb = -9.0f;     // Maximum spectral carving attenuation (-3.0 to -18.0 dB)
        float sensitivity = 1.0f;           // Masking detection sensitivity (0.2 to 2.0)
        float attackMs = 15.0f;             // Attack time per band (2.0 to 50.0 ms)
        float releaseMs = 60.0f;            // Release time per band (20.0 to 250.0 ms)
        float dryWet = 1.0f;                // Unmasked mix blend (0.0 to 1.0)
        bool soloDelta = false;             // Audition only carved spectral energy
    };

    JZSpectralSidechainUnmaskerEngine(float sampleRate = 48000.0f);
    ~JZSpectralSidechainUnmaskerEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setUnmaskingDepthDb(float db);
    float getUnmaskingDepthDb() const { return m_params.unmaskingDepthDb; }

    void setSensitivity(float sens);
    float getSensitivity() const { return m_params.sensitivity; }

    /**
     * @brief Process main audio buffer with sidechain masking input.
     */
    void process(const float* mainL, const float* mainR,
                 const float* sidechainL, const float* sidechainR,
                 float* outL, float* outR,
                 size_t numSamples);

    float getMaxReductionDb() const { return m_maxReductionDb; }
    size_t getActiveMaskedBands() const { return m_activeMaskedBands; }

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    // 24 Bark-scale critical bands
    static const size_t NUM_BARK_BANDS = 24;
    struct BarkBand {
        float centerFreqHz;
        float b0, b1, b2, a1, a2;
        float x1L, x2L, y1L, y2L;
        float x1R, x2R, y1R, y2R;
        float scX1L, scX2L, scY1L, scY2L;
        float scX1R, scX2R, scY1R, scY2R;

        float mainEnergyL = 0.0f;
        float mainEnergyR = 0.0f;
        float scEnergyL = 0.0f;
        float scEnergyR = 0.0f;
        float currentGainL = 1.0f;
        float currentGainR = 1.0f;
    };

    BarkBand m_bands[NUM_BARK_BANDS];

    float m_attackCoeff = 0.0f;
    float m_releaseCoeff = 0.0f;
    float m_maxReductionDb = 0.0f;
    size_t m_activeMaskedBands = 0;

    void updateFilterbank();
};

} // namespace Jazz

namespace jazz {
    using JZSpectralSidechainUnmaskerEngine = Jazz::JZSpectralSidechainUnmaskerEngine;
}

#endif // JAZZ_SPECTRAL_SIDECHAIN_UNMASKER_ENGINE_H
