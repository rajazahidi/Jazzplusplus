//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// MultiTrackDrumBleedRemoverEngine.h - Multi-Mic Drum Spill De-Bleeder
//
// Cross-microphone intelligent drum bleed and cymbal spill removal:
// Carves out hi-hat and cymbal leakage from close-mic snare, kick, and tom
// tracks without cutting natural drum shell decay or transient snap.
//*****************************************************************************

#ifndef JAZZ_MULTI_TRACK_DRUM_BLEED_REMOVER_ENGINE_H
#define JAZZ_MULTI_TRACK_DRUM_BLEED_REMOVER_ENGINE_H

#include <vector>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

class JZMultiTrackDrumBleedRemoverEngine {
public:
    struct Parameters {
        float bleedReductionDb = -18.0f;     // Spill attenuation depth (-6.0 to -32.0 dB)
        float sensitivity = 1.0f;            // Leakage detection sensitivity (0.2 to 2.5)
        float transientThresholdDb = -24.0f; // Snare/drum hit trigger threshold (-45.0 to -6.0 dB)
        float holdMs = 45.0f;                // Natural drum body ring hold (10.0 to 200.0 ms)
        float decayMs = 120.0f;              // Smooth decay release (20.0 to 500.0 ms)
        float crossoverFreqHz = 2800.0f;     // Spill frequency threshold (1200.0 to 6000.0 Hz)
        bool soloBleed = false;              // Audition removed spill only
        bool enabled = true;
    };

    JZMultiTrackDrumBleedRemoverEngine(float sampleRate = 48000.0f);
    ~JZMultiTrackDrumBleedRemoverEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setBleedReductionDb(float db);
    float getBleedReductionDb() const { return m_params.bleedReductionDb; }

    void setSensitivity(float sens);
    float getSensitivity() const { return m_params.sensitivity; }

    void setTransientThresholdDb(float db);
    float getTransientThresholdDb() const { return m_params.transientThresholdDb; }

    void setHoldMs(float ms);
    float getHoldMs() const { return m_params.holdMs; }

    void setSoloBleed(bool solo) { m_params.soloBleed = solo; }
    bool isSoloBleed() const { return m_params.soloBleed; }

    /**
     * @brief Process target drum track with spill reference sidechain.
     * @param targetInL Primary drum track (e.g. Snare Mic)
     * @param targetInR Primary drum track right channel (or nullptr)
     * @param spillRefL Spill reference mic (e.g. Hi-Hat or Overhead Mic)
     * @param spillRefR Spill reference right channel (or nullptr)
     * @param outL De-bled drum output left
     * @param outR De-bled drum output right
     * @param numSamples Number of samples to process
     */
    void process(const float* targetInL, const float* targetInR,
                 const float* spillRefL, const float* spillRefR,
                 float* outL, float* outR,
                 size_t numSamples);

    size_t getDetectedHitCount() const { return m_detectedHits; }
    float getCurrentReductionDb() const { return m_currentReductionDb; }
    bool isGateOpen() const { return m_gateEnvelope > 0.5f; }

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    // Filter biquad for spill band (high shelf / crossover)
    float m_shelfB0 = 1.0f, m_shelfB1 = 0.0f, m_shelfB2 = 0.0f, m_shelfA1 = 0.0f, m_shelfA2 = 0.0f;
    float m_shelfX1L = 0.0f, m_shelfX2L = 0.0f, m_shelfY1L = 0.0f, m_shelfY2L = 0.0f;
    float m_shelfX1R = 0.0f, m_shelfX2R = 0.0f, m_shelfY1R = 0.0f, m_shelfY2R = 0.0f;

    // Target transient bandpass (snare fundamental 180-300 Hz)
    float m_bpB0 = 0.0f, m_bpB1 = 0.0f, m_bpB2 = 0.0f, m_bpA1 = 0.0f, m_bpA2 = 0.0f;
    float m_bpX1 = 0.0f, m_bpX2 = 0.0f, m_bpY1 = 0.0f, m_bpY2 = 0.0f;

    float m_gateEnvelope = 1.0f;
    float m_holdCounterSec = 0.0f;
    float m_decayCoeff = 0.0f;
    size_t m_detectedHits = 0;
    float m_currentReductionDb = 0.0f;

    void updateFilterCoefficients();
};

} // namespace Jazz

namespace jazz {
    using JZMultiTrackDrumBleedRemoverEngine = Jazz::JZMultiTrackDrumBleedRemoverEngine;
}

#endif // JAZZ_MULTI_TRACK_DRUM_BLEED_REMOVER_ENGINE_H
