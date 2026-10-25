//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// SpectralResonanceSuppressorEngine.h - Dynamic Spectral Resonance Suppressor
//
// Real-time psychoacoustic dynamic resonance tracker & harshness suppressor
// (Oeksound Soothe 2 / Gullfoss style). Automatically identifies and surgically
// tames piercing harshness, cymbal shrillness, and acoustic room modes using an
// array of floating dynamic notch filters without dulling the audio spectrum.
//*****************************************************************************

#ifndef JAZZ_SPECTRAL_RESONANCE_SUPPRESSOR_ENGINE_H
#define JAZZ_SPECTRAL_RESONANCE_SUPPRESSOR_ENGINE_H

#include <vector>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

class JZSpectralResonanceSuppressorEngine {
public:
    static constexpr size_t NUM_NOTCHES = 8;
    static constexpr size_t NUM_BARK_BANDS = 16;

    struct NotchFilter {
        float centerFreqHz = 3500.0f;
        float currentDepthDb = 0.0f;
        float targetDepthDb = 0.0f;
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
        float x1L = 0.0f, x2L = 0.0f, y1L = 0.0f, y2L = 0.0f;
        float x1R = 0.0f, x2R = 0.0f, y1R = 0.0f, y2R = 0.0f;

        void reset() {
            x1L = x2L = y1L = y2L = 0.0f;
            x1R = x2R = y1R = y2R = 0.0f;
            currentDepthDb = 0.0f;
            targetDepthDb = 0.0f;
        }
    };

    struct Parameters {
        float thresholdDb = -18.0f;          // Resonance detection sensitivity threshold (-36 to -6 dB)
        float depth = 0.75f;                 // Suppression intensity factor (0.1 to 1.0)
        float sharpnessQ = 16.0f;            // Surgical notch filter Q factor (5.0 to 40.0)
        float minFreqHz = 1200.0f;           // Lower frequency focus bound (500 to 4000 Hz)
        float maxFreqHz = 11000.0f;          // Upper frequency focus bound (5000 to 18000 Hz)
        bool auditionDeltaSolo = false;      // Listen exclusively to suppressed harsh resonances
        float dryWet = 1.0f;
        bool bypassed = false;
    };

    JZSpectralResonanceSuppressorEngine(float sampleRate = 48000.0f);
    ~JZSpectralResonanceSuppressorEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setThresholdDb(float db);
    float getThresholdDb() const { return m_params.thresholdDb; }

    void setDepth(float depth);
    float getDepth() const { return m_params.depth; }

    void setSharpnessQ(float q);
    float getSharpnessQ() const { return m_params.sharpnessQ; }

    void setFrequencyFocus(float minHz, float maxHz);
    float getMinFreqHz() const { return m_params.minFreqHz; }
    float getMaxFreqHz() const { return m_params.maxFreqHz; }

    void setAuditionDeltaSolo(bool solo);
    bool isAuditionDeltaSolo() const { return m_params.auditionDeltaSolo; }

    void setDryWet(float wet);
    float getDryWet() const { return m_params.dryWet; }

    void setBypass(bool bypass);
    bool isBypassed() const { return m_params.bypassed; }

    // Telemetry & Getters
    size_t getActiveNotchesCount() const;
    float getMaxSuppressionDb() const { return m_maxSuppressionDb; }
    float getAverageResonanceIndex() const { return m_averageResonanceIndex; }
    bool isSuppressing() const { return m_maxSuppressionDb < -1.0f; }

    // Audio Processing
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

private:
    void updateNotchCoefficients(NotchFilter& notch, float freqHz, float depthDb, float q);

    float m_sampleRate = 48000.0f;
    Parameters m_params;

    NotchFilter m_notches[NUM_NOTCHES];

    // Band energy tracking across critical bands
    float m_bandFrequencies[NUM_BARK_BANDS];
    float m_bandEnergies[NUM_BARK_BANDS];

    float m_maxSuppressionDb = 0.0f;
    float m_averageResonanceIndex = 0.0f;
};

} // namespace Jazz

namespace jazz {
    using JZSpectralResonanceSuppressorEngine = Jazz::JZSpectralResonanceSuppressorEngine;
}

#endif // JAZZ_SPECTRAL_RESONANCE_SUPPRESSOR_ENGINE_H
