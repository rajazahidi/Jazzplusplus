//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// VocalVibratoHumanizerEngine.h - Vocal Vibrato & Pitch Drift Humanizer
//
// Real-time vocal vibrato synthesis, attack onset delay ramping, 1/f fractal
// micro-intonation drift, and amplitude tremolo coupling (Melodyne / Auto-Tune style).
// Reintroduces organic human expression into rigidly tuned vocal tracks.
//*****************************************************************************

#ifndef JAZZ_VOCAL_VIBRATO_HUMANIZER_ENGINE_H
#define JAZZ_VOCAL_VIBRATO_HUMANIZER_ENGINE_H

#include <vector>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

class JZVocalVibratoHumanizerEngine {
public:
    struct Parameters {
        float vibratoRateHz = 5.5f;          // Vibrato frequency (3.5 to 8.5 Hz)
        float vibratoDepthCents = 40.0f;     // Pitch excursion (+/- 0 to 120 cents)
        float onsetDelayMs = 350.0f;         // Vibrato fade-in delay after note start (0 to 1200 ms)
        float pitchDriftCents = 8.0f;        // Slow fractal pitch drift (0 to 25 cents)
        float tremoloAmount = 0.12f;         // Amplitude modulation depth (0.0 to 0.35)
        float dryWet = 1.0f;
        bool bypassed = false;
    };

    JZVocalVibratoHumanizerEngine(float sampleRate = 48000.0f);
    ~JZVocalVibratoHumanizerEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setVibratoRateHz(float hz);
    float getVibratoRateHz() const { return m_params.vibratoRateHz; }

    void setVibratoDepthCents(float cents);
    float getVibratoDepthCents() const { return m_params.vibratoDepthCents; }

    void setOnsetDelayMs(float ms);
    float getOnsetDelayMs() const { return m_params.onsetDelayMs; }

    void setPitchDriftCents(float cents);
    float getPitchDriftCents() const { return m_params.pitchDriftCents; }

    void setTremoloAmount(float tremolo);
    float getTremoloAmount() const { return m_params.tremoloAmount; }

    void setDryWet(float wet);
    float getDryWet() const { return m_params.dryWet; }

    void setBypass(bool bypass);
    bool isBypassed() const { return m_params.bypassed; }

    void triggerNoteOn();

    // Telemetry & Getters
    float getCurrentPitchOffsetCents() const { return m_currentPitchOffsetCents; }
    float getVibratoPhase() const { return m_vibratoPhase; }
    bool isVibratoActive() const { return std::abs(m_currentPitchOffsetCents) > 1.0f; }

    // Audio Processing
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    // Fractional delay buffer for pitch modulation
    static const size_t DELAY_BUF_SIZE = 4096;
    std::vector<float> m_delayBufL;
    std::vector<float> m_delayBufR;
    size_t m_writePos = 0;
    float m_readPos = 0.0f;

    float m_vibratoPhase = 0.0f;
    float m_driftPhase = 0.0f;
    float m_noteTimeSec = 0.0f;
    float m_onsetEnvelope = 0.0f;
    float m_currentPitchOffsetCents = 0.0f;

    // Simple pink noise / 1/f state for fractal drift
    float m_pinkB0 = 0.0f, m_pinkB1 = 0.0f, m_pinkB2 = 0.0f;
};

} // namespace Jazz

namespace jazz {
    using JZVocalVibratoHumanizerEngine = Jazz::JZVocalVibratoHumanizerEngine;
}

#endif // JAZZ_VOCAL_VIBRATO_HUMANIZER_ENGINE_H
