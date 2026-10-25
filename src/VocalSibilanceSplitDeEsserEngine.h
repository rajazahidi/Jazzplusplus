//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// VocalSibilanceSplitDeEsserEngine.h - Intelligent Split-Band Vocal De-Esser
//
// Precision psychoacoustic vocal sibilance suppression (FabFilter Pro-DS style):
// Lookahead sibilance detection, split-band high-frequency dynamic attenuation,
// and sidechain listen audition filter without dulling vocal warmth.
//*****************************************************************************

#ifndef JAZZ_VOCAL_SIBILANCE_SPLIT_DE_ESSER_ENGINE_H
#define JAZZ_VOCAL_SIBILANCE_SPLIT_DE_ESSER_ENGINE_H

#include <vector>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

class JZVocalSibilanceSplitDeEsserEngine {
public:
    struct Parameters {
        float frequencyHz = 6500.0f;         // Center sibilant frequency (3500.0 to 11000.0 Hz)
        float thresholdDb = -22.0f;          // Detection trigger threshold (-45.0 to 0.0 dB)
        float rangeDb = -12.0f;              // Maximum de-essing attenuation (-3.0 to -24.0 dB)
        float bandwidthQ = 1.4f;             // Sibilant detection bandwidth (0.5 to 3.5)
        float attackMs = 1.5f;               // Fast attack ballistics (0.5 to 10.0 ms)
        float releaseMs = 35.0f;             // Release ballistics (10.0 to 150.0 ms)
        bool splitBand = true;               // True = split-band only; False = wideband ducking
        bool auditionSidechain = false;      // Listen exclusively to detected sibilance
        float dryWet = 1.0f;
    };

    JZVocalSibilanceSplitDeEsserEngine(float sampleRate = 48000.0f);
    ~JZVocalSibilanceSplitDeEsserEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setFrequencyHz(float hz);
    float getFrequencyHz() const { return m_params.frequencyHz; }

    void setThresholdDb(float db);
    float getThresholdDb() const { return m_params.thresholdDb; }

    void setRangeDb(float db);
    float getRangeDb() const { return m_params.rangeDb; }

    void setAuditionSidechain(bool audition) { m_params.auditionSidechain = audition; }
    bool isAuditionSidechain() const { return m_params.auditionSidechain; }

    /**
     * @brief Process stereo audio buffer through vocal de-esser.
     */
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

    float getCurrentGainReductionDb() const { return m_currentReductionDb; }
    bool isSibilanceActive() const { return m_currentReductionDb < -1.0f; }

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    // Sibilance bandpass detection biquad
    float m_scB0 = 0.0f, m_scB1 = 0.0f, m_scB2 = 0.0f, m_scA1 = 0.0f, m_scA2 = 0.0f;
    float m_scX1L = 0.0f, m_scX2L = 0.0f, m_scY1L = 0.0f, m_scY2L = 0.0f;
    float m_scX1R = 0.0f, m_scX2R = 0.0f, m_scY1R = 0.0f, m_scY2R = 0.0f;

    // Split-band bell reduction biquad
    float m_bellB0 = 1.0f, m_bellB1 = 0.0f, m_bellB2 = 0.0f, m_bellA1 = 0.0f, m_bellA2 = 0.0f;
    float m_bellX1L = 0.0f, m_bellX2L = 0.0f, m_bellY1L = 0.0f, m_bellY2L = 0.0f;
    float m_bellX1R = 0.0f, m_bellX2R = 0.0f, m_bellY1R = 0.0f, m_bellY2R = 0.0f;

    // 2.5ms Lookahead circular buffer
    static const size_t LOOKAHEAD_SIZE = 128;
    std::vector<float> m_lookaheadBufL;
    std::vector<float> m_lookaheadBufR;
    size_t m_writePos = 0;

    float m_attackCoeff = 0.0f;
    float m_releaseCoeff = 0.0f;
    float m_gainSmoothed = 1.0f;
    float m_currentReductionDb = 0.0f;

    void updateFilterCoefficients();
};

} // namespace Jazz

namespace jazz {
    using JZVocalSibilanceSplitDeEsserEngine = Jazz::JZVocalSibilanceSplitDeEsserEngine;
}

#endif // JAZZ_VOCAL_SIBILANCE_SPLIT_DE_ESSER_ENGINE_H
