//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// SpeechDenoiseSpectralSubtractorEngine.h - Real-Time Speech Spectral De-Noiser
//
// Adaptive spectral subtraction and Wiener filtering (iZotope Voice De-noise style):
// Eliminates background room noise, HVAC hiss, and microphone preamp hum from
// dialogue and lead vocals without introducing musical noise or dulling consonants.
//*****************************************************************************

#ifndef JAZZ_SPEECH_DENOISE_SPECTRAL_SUBTRACTOR_ENGINE_H
#define JAZZ_SPEECH_DENOISE_SPECTRAL_SUBTRACTOR_ENGINE_H

#include <vector>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

class JZSpeechDenoiseSpectralSubtractorEngine {
public:
    static const size_t NUM_SPECTRAL_BANDS = 16;

    struct Parameters {
        float reductionDb = -15.0f;          // Target noise attenuation (-3.0 to -30.0 dB)
        float oversubtractionFactor = 1.35f; // Noise mask factor (1.0 to 2.5)
        float smoothingMs = 25.0f;           // Ballistics envelope smoothing (5.0 to 100.0 ms)
        bool preserveConsonants = true;      // Prevent HF consonant dulling
        bool isLearning = false;             // Profile capture mode
        float dryWet = 1.0f;                 // Wet / dry blend
    };

    JZSpeechDenoiseSpectralSubtractorEngine(float sampleRate = 48000.0f);
    ~JZSpeechDenoiseSpectralSubtractorEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setReductionDb(float db);
    float getReductionDb() const { return m_params.reductionDb; }

    void setLearnMode(bool learning) { m_params.isLearning = learning; }
    bool isLearnMode() const { return m_params.isLearning; }

    /**
     * @brief Learn background noise profile from an ambient silence buffer.
     */
    void learnNoiseProfile(const float* silenceBuffer, size_t numSamples);

    /**
     * @brief Process audio buffer through spectral subtractor de-noiser.
     */
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

    float getAverageNoiseFloorDb() const { return m_avgNoiseFloorDb; }
    float getCurrentReductionDb() const { return m_currentReductionDb; }

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    struct SpectralBand {
        float centerFreqHz;
        float b0, b1, b2, a1, a2;
        float x1L, x2L, y1L, y2L;
        float x1R, x2R, y1R, y2R;
        float noiseFloorPower = 1e-4f;
        float currentGainL = 1.0f;
        float currentGainR = 1.0f;
    };

    SpectralBand m_bands[NUM_SPECTRAL_BANDS];

    float m_smoothingCoeff = 0.0f;
    float m_avgNoiseFloorDb = -60.0f;
    float m_currentReductionDb = 0.0f;

    void updateFilterbank();
};

} // namespace Jazz

namespace jazz {
    using JZSpeechDenoiseSpectralSubtractorEngine = Jazz::JZSpeechDenoiseSpectralSubtractorEngine;
}

#endif // JAZZ_SPEECH_DENOISE_SPECTRAL_SUBTRACTOR_ENGINE_H
