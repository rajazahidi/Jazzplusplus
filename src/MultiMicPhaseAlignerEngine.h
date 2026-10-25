//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZMultiMicPhaseAlignerEngine.h - Multi-Microphone Phase Auto-Aligner
//
// Studio multi-microphone phase auto-alignment (Sound Radix Auto-Align style):
// Sub-sample cross-correlation, polarity flip detection, all-pass phase rotation,
// and spectral comb-filter cancellation recovery for drums and multi-amp rigs.
//*****************************************************************************

#ifndef JAZZ_MULTI_MIC_PHASE_ALIGNER_ENGINE_H
#define JAZZ_MULTI_MIC_PHASE_ALIGNER_ENGINE_H

#include <vector>
#include <string>
#include <map>
#include <cstdint>

namespace Jazz {

struct MicTrackAlignment {
    int trackId;
    std::string trackName;
    bool isGuideTrack;
    double measuredDelaySamples;    // Sub-sample delay relative to guide track
    double measuredDelayMs;         // Delay in milliseconds
    bool polarityInverted;          // True if inverted phase detected
    float spectralCorrelation;      // Phase correlation [-1.0, +1.0]
    float lowEndPowerGainDb;        // Bass power recovered in dB after alignment
    bool autoCorrectionEnabled;
};

class JZMultiMicPhaseAlignerEngine {
public:
    JZMultiMicPhaseAlignerEngine(float sampleRate = 48000.0f);
    ~JZMultiMicPhaseAlignerEngine() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return m_sampleRate; }

    // Track Registration
    int registerTrack(const std::string& name, bool isGuide = false);
    bool setGuideTrack(int trackId);
    int getGuideTrackId() const { return m_guideTrackId; }
    const MicTrackAlignment* getTrackAlignment(int trackId) const;

    // Phase & Delay Analysis
    // Cross-correlates target track against guide track within search window
    bool analyzeAlignment(int targetTrackId, const float* guideAudio, const float* targetAudio,
                          size_t numSamples, int maxSearchSamples = 2048);

    // Apply Real-time Phase Alignment
    // Shifts target audio by the calibrated delay with optional polarity flip
    void processTrack(int targetTrackId, const float* inAudio, float* outAudio, size_t numSamples);

    void reset();

private:
    float m_sampleRate;
    int m_guideTrackId;
    int m_nextTrackId;
    std::map<int, MicTrackAlignment> m_tracks;

    // Circular delay buffers for real-time sample delay
    std::map<int, std::vector<float>> m_delayBuffers;
    std::map<int, size_t> m_writePointers;
};

} // namespace Jazz

#endif // JAZZ_MULTI_MIC_PHASE_ALIGNER_ENGINE_H
