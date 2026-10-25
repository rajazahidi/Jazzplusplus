//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// PolyphaseAntiAliasingOversamplerEngine.h - Ultra-Low-Latency Polyphase
// Anti-Aliasing Oversampling Wrapper Engine
//
// Studio-grade multi-rate polyphase FIR oversampling engine (2x, 4x, 8x, 16x):
// Symmetric half-band coefficients, >100dB stopband anti-aliasing rejection,
// dual phase modes (Linear Phase for mastering vs Minimum Phase for zero-latency tracking),
// and universal wrapped processing functor for non-linear distortion modules.
//*****************************************************************************

#ifndef JAZZ_POLYPHASE_ANTI_ALIASING_OVERSAMPLER_ENGINE_H
#define JAZZ_POLYPHASE_ANTI_ALIASING_OVERSAMPLER_ENGINE_H

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <functional>
#include <algorithm>

namespace Jazz {

enum class OversamplingFactor {
    X2 = 2,
    X4 = 4,
    X8 = 8,
    X16 = 16
};

enum class OversamplingPhaseMode {
    LINEAR_PHASE = 0,   // Zero phase distortion, symmetric impulse (Mastering)
    MINIMUM_PHASE       // Zero pre-ringing, ultra-low group delay (Live Tracking)
};

class JZPolyphaseAntiAliasingOversamplerEngine {
public:
    JZPolyphaseAntiAliasingOversamplerEngine(float baseSampleRate = 48000.0f,
                                            OversamplingFactor factor = OversamplingFactor::X4,
                                            OversamplingPhaseMode mode = OversamplingPhaseMode::LINEAR_PHASE);
    ~JZPolyphaseAntiAliasingOversamplerEngine() = default;

    void initialize(float baseSampleRate);
    void reset();

    void setOversamplingFactor(OversamplingFactor factor);
    OversamplingFactor getOversamplingFactor() const { return m_factor; }
    int getFactorInt() const { return static_cast<int>(m_factor); }

    void setPhaseMode(OversamplingPhaseMode mode);
    OversamplingPhaseMode getPhaseMode() const { return m_phaseMode; }
    std::string getPhaseModeName() const;

    float getOversampledRate() const { return m_baseSampleRate * getFactorInt(); }
    size_t getLatencySamples() const { return m_latencySamples; }
    float getStopbandRejectionDb() const { return m_stopbandRejectionDb; }

    /**
     * @brief Upsample input stereo buffer to factor * numSamples.
     */
    void upsample(const float* inL, const float* inR, size_t numInputSamples,
                  std::vector<float>& outUpsampledL, std::vector<float>& outUpsampledR);

    /**
     * @brief Downsample oversampled stereo buffer back to base sample rate.
     */
    void downsample(const float* inUpsampledL, const float* inUpsampledR, size_t numUpsampledSamples,
                    float* outL, float* outR);

    /**
     * @brief Process an audio block through an oversampled non-linear processing functor.
     */
    void processWrapped(const float* inL, const float* inR,
                        float* outL, float* outR,
                        size_t numSamples,
                        const std::function<void(float* upL, float* upR, size_t upSamples)>& processFn);

private:
    float m_baseSampleRate = 48000.0f;
    OversamplingFactor m_factor = OversamplingFactor::X4;
    OversamplingPhaseMode m_phaseMode = OversamplingPhaseMode::LINEAR_PHASE;

    size_t m_latencySamples = 16;
    float m_stopbandRejectionDb = 104.0f;

    // Internal scratch buffers for upsampled processing
    std::vector<float> m_scratchUpL;
    std::vector<float> m_scratchUpR;

    // Filter memory buffers for polyphase FIR stages (Left and Right)
    std::vector<float> m_upHistL;
    std::vector<float> m_upHistR;
    std::vector<float> m_downHistL;
    std::vector<float> m_downHistR;

    // Polyphase FIR filter coefficients
    std::vector<float> m_halfbandCoeffs;

    void rebuildFilters();
};

} // namespace Jazz

namespace jazz {
    using JZPolyphaseAntiAliasingOversamplerEngine = Jazz::JZPolyphaseAntiAliasingOversamplerEngine;
}

#endif // JAZZ_POLYPHASE_ANTI_ALIASING_OVERSAMPLER_ENGINE_H
