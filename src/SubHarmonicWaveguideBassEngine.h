#pragma once

#include <vector>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

/**
 * @brief Sub-Harmonic Waveguide Bass Synthesizer & Phase-Locked Octave Generator.
 * Inspired by classic hardware LF synthesizers (dbx 120XP, Little Labs Voice of God, Waves Submarine).
 * Synthesizes phase-coherent sub-octaves (-1 oct and -2 oct) coupled to a resonant acoustic waveguide.
 */
class JZSubHarmonicWaveguideBassEngine {
public:
    struct Parameters {
        float subOctave1Level = 0.7f;     // -1 Octave level (0.0 to 1.0)
        float subOctave2Level = 0.3f;     // -2 Octaves level (0.0 to 1.0)
        float centerFrequencyHz = 40.0f;  // Resonant center frequency (20.0 to 90.0 Hz)
        float resonanceQ = 4.5f;          // Resonant waveguide Q-factor (1.0 to 12.0)
        float drive = 1.4f;               // Soft saturation drive (1.0 to 5.0)
        float lowCutHz = 22.0f;           // Infrasonic rumble protection highpass (15.0 to 35.0 Hz)
        float dryWet = 0.45f;             // Dry / Wet blend (0.0 to 1.0)
        bool invertPhase = false;         // Subharmonic polarity inversion
        bool soloSub = false;             // Mute dry signal completely
    };

    JZSubHarmonicWaveguideBassEngine();
    ~JZSubHarmonicWaveguideBassEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    /**
     * @brief Process stereo audio buffer through sub-harmonic waveguide bass engine.
     */
    void process(const float* inL, const float* inR, float* outL, float* outR, size_t numSamples);

    // Telemetry & metering
    float getSubEnergyPeak() const { return m_subEnergyPeak; }
    float getSubEnergyRms() const { return m_subEnergyRms; }
    bool isLockAcquired() const { return m_lockAcquired; }

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    // Zero-crossing / Flip-flop sub-octave divider states
    bool m_prevSignL = false;
    bool m_prevSignR = false;
    bool m_div1StateL = false;
    bool m_div1StateR = false;
    bool m_div2StateL = false;
    bool m_div2StateR = false;
    int m_zeroCrossingCountL = 0;
    int m_zeroCrossingCountR = 0;

    // Resonant Biquad Filter (Bandpass & Lowcut) states
    struct Biquad {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
        float a1 = 0.0f, a2 = 0.0f;
        float x1 = 0.0f, x2 = 0.0f;
        float y1 = 0.0f, y2 = 0.0f;

        void reset() {
            x1 = x2 = y1 = y2 = 0.0f;
        }

        inline float processSample(float in) {
            float out = b0 * in + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1;
            x1 = in;
            y2 = y1;
            y1 = out;
            return out;
        }
    };

    Biquad m_bpFilterL;
    Biquad m_bpFilterR;
    Biquad m_hpFilterL;
    Biquad m_hpFilterR;

    // Physical waveguide acoustic cavity delay line
    std::vector<float> m_waveguideL;
    std::vector<float> m_waveguideR;
    size_t m_wgIndexL = 0;
    size_t m_wgIndexR = 0;
    size_t m_wgLength = 1024;
    float m_wgFeedback = 0.65f;

    // Telemetry
    float m_subEnergyPeak = 0.0f;
    float m_subEnergyRms = 0.0f;
    bool m_lockAcquired = false;

    void updateFilters();
};

} // namespace Jazz

namespace jazz {
    using JZSubHarmonicWaveguideBassEngine = Jazz::JZSubHarmonicWaveguideBassEngine;
}
