//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// EllipticalMonoBassCenteringEngine.h - Elliptical Mono Bass Centering Matrix
//
// Mastering Elliptical Equalizer (EEQ) and Low-End Mono Centering matrix
// (Neumann W495 / Brainworx bx_digital style). Eliminates stereo phase cancelations
// and mud by forcing sub-bass below a variable cutoff (40-350 Hz) strictly to Mono,
// while preserving punch and offering high-frequency stereo width expansion.
//*****************************************************************************

#ifndef JAZZ_ELLIPTICAL_MONO_BASS_CENTERING_ENGINE_H
#define JAZZ_ELLIPTICAL_MONO_BASS_CENTERING_ENGINE_H

#include <vector>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

enum class EllipticalFilterSlope {
    SLOPE_12DB_OCT = 1, // 2nd-order Butterworth
    SLOPE_24DB_OCT = 2  // 4th-order Linkwitz-Riley
};

class JZEllipticalMonoBassCenteringEngine {
public:
    struct Parameters {
        float monoCutoffHz = 120.0f;           // Crossover cutoff for mono centering (40 to 350 Hz)
        EllipticalFilterSlope filterSlope = EllipticalFilterSlope::SLOPE_12DB_OCT;
        float stereoWidth = 1.0f;              // Side width scaling above cutoff (0.0 to 2.0)
        bool redirectSubEnergyToMid = true;    // Conserve low-end energy by summing Side bass into Mid
        bool auditionSideSub = false;          // Solo listen exclusively to removed out-of-phase bass
        float dryWet = 1.0f;
        bool bypassed = false;
    };

    JZEllipticalMonoBassCenteringEngine(float sampleRate = 48000.0f);
    ~JZEllipticalMonoBassCenteringEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setMonoCutoffHz(float hz);
    float getMonoCutoffHz() const { return m_params.monoCutoffHz; }

    void setFilterSlope(EllipticalFilterSlope slope);
    EllipticalFilterSlope getFilterSlope() const { return m_params.filterSlope; }

    void setStereoWidth(float width);
    float getStereoWidth() const { return m_params.stereoWidth; }

    void setRedirectSubEnergyToMid(bool redirect);
    bool isRedirectSubEnergyToMid() const { return m_params.redirectSubEnergyToMid; }

    void setAuditionSideSub(bool audition);
    bool isAuditionSideSub() const { return m_params.auditionSideSub; }

    void setDryWet(float wet);
    float getDryWet() const { return m_params.dryWet; }

    void setBypass(bool bypass);
    bool isBypassed() const { return m_params.bypassed; }

    // Telemetry & Getters
    float getFilteredSideEnergyDb() const { return m_filteredSideEnergyDb; }
    float getPhaseCorrelation() const { return m_phaseCorrelation; }

    // Audio Processing
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

private:
    void updateFilters();

    float m_sampleRate = 48000.0f;
    Parameters m_params;

    // Side channel High-Pass & Low-Pass Biquads
    // Biquad 1
    float m_hpB0_1 = 1.0f, m_hpB1_1 = 0.0f, m_hpB2_1 = 0.0f, m_hpA1_1 = 0.0f, m_hpA2_1 = 0.0f;
    float m_hpX1_1 = 0.0f, m_hpX2_1 = 0.0f, m_hpY1_1 = 0.0f, m_hpY2_1 = 0.0f;

    // Biquad 2 (for 24 dB/oct slope)
    float m_hpB0_2 = 1.0f, m_hpB1_2 = 0.0f, m_hpB2_2 = 0.0f, m_hpA1_2 = 0.0f, m_hpA2_2 = 0.0f;
    float m_hpX1_2 = 0.0f, m_hpX2_2 = 0.0f, m_hpY1_2 = 0.0f, m_hpY2_2 = 0.0f;

    float m_filteredSideEnergyDb = -60.0f;
    float m_phaseCorrelation = 1.0f;
};

} // namespace Jazz

namespace jazz {
    using JZEllipticalMonoBassCenteringEngine = Jazz::JZEllipticalMonoBassCenteringEngine;
    using EllipticalFilterSlope = Jazz::EllipticalFilterSlope;
}

#endif // JAZZ_ELLIPTICAL_MONO_BASS_CENTERING_ENGINE_H
