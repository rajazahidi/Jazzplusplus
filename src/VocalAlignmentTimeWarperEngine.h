//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// VocalAlignmentTimeWarperEngine.h - Multi-Track Vocal Alignment
//
// Automated vocal timing and phrasing alignment (VocALign / Revoice Pro style):
// Computes dynamic time warping (DTW) elastic audio deformation matching
// backing vocal doubles tightly to a lead vocal guide track.
//*****************************************************************************

#ifndef JAZZ_VOCAL_ALIGNMENT_TIME_WARPER_ENGINE_H
#define JAZZ_VOCAL_ALIGNMENT_TIME_WARPER_ENGINE_H

#include <vector>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

class JZVocalAlignmentTimeWarperEngine {
public:
    struct Parameters {
        float tightness = 0.85f;             // 0.0 (natural slack) to 1.0 (strict robotic lock)
        float maxWarpStretchFactor = 1.6f;   // Maximum time stretch limit (1.2 to 2.5)
        float maxWarpCompressFactor = 0.65f; // Maximum time compression limit (0.4 to 0.85)
        float transientSensitivity = 1.0f;   // Consonant onset weight (0.2 to 2.0)
        size_t windowHopSamples = 256;       // DTW energy analysis hop size
    };

    JZVocalAlignmentTimeWarperEngine(float sampleRate = 48000.0f);
    ~JZVocalAlignmentTimeWarperEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setTightness(float tightness);
    float getTightness() const { return m_params.tightness; }

    void setMaxWarpStretchFactor(float stretch);
    float getMaxWarpStretchFactor() const { return m_params.maxWarpStretchFactor; }

    /**
     * @brief Align a Dub buffer to a Guide buffer using Dynamic Time Warping.
     * @param guideAudio Pointer to lead vocal guide samples
     * @param guideSamples Number of guide samples
     * @param dubAudio Pointer to backing vocal dub samples
     * @param dubSamples Number of dub samples
     * @param outAlignedDub Vector receiving elastic time-warped dub audio
     * @return Overall alignment correlation score (0.0 to 1.0)
     */
    float alignDubToGuide(const float* guideAudio, size_t guideSamples,
                          const float* dubAudio, size_t dubSamples,
                          std::vector<float>& outAlignedDub);

    float getAverageWarpRatio() const { return m_avgWarpRatio; }
    size_t getProcessedGuideSamples() const { return m_lastGuideSamples; }
    size_t getProcessedDubSamples() const { return m_lastDubSamples; }

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    float m_avgWarpRatio = 1.0f;
    size_t m_lastGuideSamples = 0;
    size_t m_lastDubSamples = 0;

    void extractEnergyEnvelope(const float* audio, size_t numSamples,
                               size_t hopSize, std::vector<float>& outEnvelope);
};

} // namespace Jazz

namespace jazz {
    using JZVocalAlignmentTimeWarperEngine = Jazz::JZVocalAlignmentTimeWarperEngine;
}

#endif // JAZZ_VOCAL_ALIGNMENT_TIME_WARPER_ENGINE_H
