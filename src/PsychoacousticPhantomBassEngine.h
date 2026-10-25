//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// PsychoacousticPhantomBassEngine.h - Phantom Missing Fundamental Bass Generator
//
// Generates psychoacoustic phantom bass harmonics (Waves MaxxBass / Aphex style):
// Synthesizes 2nd, 3rd, 4th, and 5th harmonic series (Chebyshev polynomials)
// enabling small smartphone/earbud speakers to perceive 30-80 Hz sub-bass.
//*****************************************************************************

#ifndef JAZZ_PSYCHOACOUSTIC_PHANTOM_BASS_ENGINE_H
#define JAZZ_PSYCHOACOUSTIC_PHANTOM_BASS_ENGINE_H

#include <vector>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

class JZPsychoacousticPhantomBassEngine {
public:
    struct Parameters {
        float crossoverFreqHz = 80.0f;     // Crossover cutoff (40.0 to 140.0 Hz)
        float harmonicIntensity = 1.0f;    // Intensity of generated overtone series (0.0 to 2.0)
        float harmonic2ndWeight = 0.8f;    // 2nd harmonic (2*f0) weight
        float harmonic3rdWeight = 0.5f;    // 3rd harmonic (3*f0) weight
        float harmonic4thWeight = 0.25f;   // 4th harmonic (4*f0) weight
        float originalSubLevel = 0.3f;     // Retained physical sub-fundamental (0.0 to 1.0)
        float phantomHarmonicMix = 0.85f;  // Psychoacoustic phantom bass mix (0.0 to 1.5)
        bool cutOriginalSub = false;       // Protect micro-speakers by muting pure sub
    };

    JZPsychoacousticPhantomBassEngine(float sampleRate = 48000.0f);
    ~JZPsychoacousticPhantomBassEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setCrossoverFreq(float hz);
    float getCrossoverFreq() const { return m_params.crossoverFreqHz; }

    void setHarmonicIntensity(float intensity);
    float getHarmonicIntensity() const { return m_params.harmonicIntensity; }

    void setPhantomHarmonicMix(float mix);
    float getPhantomHarmonicMix() const { return m_params.phantomHarmonicMix; }

    /**
     * @brief Process stereo audio buffer through psychoacoustic bass generator.
     */
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

    float getDetectedSubRms() const { return m_subRms; }
    float getGeneratedHarmonicsRms() const { return m_harmonicsRms; }

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    // Crossover filter state (2nd order Linkwitz-Riley low-pass and high-pass)
    struct Biquad {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
        float a1 = 0.0f, a2 = 0.0f;
        float x1 = 0.0f, x2 = 0.0f, y1 = 0.0f, y2 = 0.0f;

        void reset() { x1 = x2 = y1 = y2 = 0.0f; }

        inline float processSample(float in) {
            float out = b0 * in + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = in;
            y2 = y1; y1 = out;
            return out;
        }
    };

    Biquad m_lpFilterL, m_lpFilterR;
    Biquad m_hpFilterL, m_hpFilterR;

    // Dynamic envelope follower for sub-bass dynamics retention
    float m_subEnvL = 0.0f;
    float m_subEnvR = 0.0f;
    float m_envAttackCoeff = 0.0f;
    float m_envReleaseCoeff = 0.0f;

    float m_subRms = 0.0f;
    float m_harmonicsRms = 0.0f;

    void updateCrossover();
};

} // namespace Jazz

namespace jazz {
    using JZPsychoacousticPhantomBassEngine = Jazz::JZPsychoacousticPhantomBassEngine;
}

#endif // JAZZ_PSYCHOACOUSTIC_PHANTOM_BASS_ENGINE_H
