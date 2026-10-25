//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// MultibandTransientSculptorEngine.h - 4-Band Transient & Sustain Sculptor
//
// Frequency-split independent transient punch and sustain shaping:
// Allows surgical punch control on kick without affecting bass sustain,
// or boosting snare snap without accentuating cymbal wash (Softube / Eventide style).
//*****************************************************************************

#ifndef JAZZ_MULTIBAND_TRANSIENT_SCULPTOR_ENGINE_H
#define JAZZ_MULTIBAND_TRANSIENT_SCULPTOR_ENGINE_H

#include <vector>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

class JZMultibandTransientSculptorEngine {
public:
    static const size_t NUM_BANDS = 4; // Low, Low-Mid, High-Mid, High

    struct BandSettings {
        float attackDb = 0.0f;     // Transient gain (-12.0 to +12.0 dB)
        float sustainDb = 0.0f;    // Sustain body gain (-12.0 to +12.0 dB)
        float punch = 1.0f;        // Transient curve velocity (0.5 to 2.0)
    };

    struct Parameters {
        BandSettings bands[NUM_BANDS];
        float crossoverFreqs[NUM_BANDS - 1] = { 200.0f, 1200.0f, 5500.0f };
        float dryWet = 1.0f;
    };

    JZMultibandTransientSculptorEngine(float sampleRate = 48000.0f);
    ~JZMultibandTransientSculptorEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setBandAttackDb(size_t bandIdx, float db);
    float getBandAttackDb(size_t bandIdx) const;

    void setBandSustainDb(size_t bandIdx, float db);
    float getBandSustainDb(size_t bandIdx) const;

    /**
     * @brief Process stereo audio buffer through 4-band transient sculptor.
     */
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

    float getBandTransientActivity(size_t bandIdx) const;

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    // 4-Band Crossover filter state
    struct FilterBand {
        float lpB0, lpB1, lpB2, lpA1, lpA2;
        float hpB0, hpB1, hpB2, hpA1, hpA2;
        float lpX1L, lpX2L, lpY1L, lpY2L;
        float lpX1R, lpX2R, lpY1R, lpY2R;
        float hpX1L, hpX2L, hpY1L, hpY2L;
        float hpX1R, hpX2R, hpY1R, hpY2R;

        // Fast & slow envelope followers
        float fastEnvL = 0.0f, slowEnvL = 0.0f;
        float fastEnvR = 0.0f, slowEnvR = 0.0f;
        float transientActivity = 0.0f;
    };

    FilterBand m_filters[3]; // 3 crossovers split audio into 4 bands

    void updateCrossoverFilters();
};

} // namespace Jazz

namespace jazz {
    using JZMultibandTransientSculptorEngine = Jazz::JZMultibandTransientSculptorEngine;
}

#endif // JAZZ_MULTIBAND_TRANSIENT_SCULPTOR_ENGINE_H
