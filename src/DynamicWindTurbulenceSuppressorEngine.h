//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// DynamicWindTurbulenceSuppressorEngine.h - Dynamic Atmospheric Wind Buffeting
// & Diaphragm Overload Attenuator
//
// Real-time atmospheric wind noise, micro-gust turbulence, and low-frequency
// capsule buffeting reduction (Cedar Wind Filter / iZotope RX De-wind style):
// Sub-band energy correlation, non-stationary gust acceleration detection,
// adaptive dynamic high-pass filter expansion (20Hz to 350Hz), and Wind Delta Solo.
//*****************************************************************************

#ifndef JAZZ_DYNAMIC_WIND_TURBULENCE_SUPPRESSOR_ENGINE_H
#define JAZZ_DYNAMIC_WIND_TURBULENCE_SUPPRESSOR_ENGINE_H

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

class JZDynamicWindTurbulenceSuppressorEngine {
public:
    struct Parameters {
        float reductionDb = -18.0f;          // Gust attenuation depth (-3.0 to -36.0 dB)
        float sensitivity = 1.0f;            // Wind turbulence detection sensitivity (0.2 to 2.5)
        float minCutoffHz = 25.0f;           // Resting low-cut filter frequency (20.0 to 60.0 Hz)
        float maxCutoffHz = 320.0f;          // Maximum expansion cutoff during heavy gusts (150.0 to 500.0 Hz)
        float attackMs = 2.0f;               // Rapid gust detection attack time (0.5 to 10.0 ms)
        float releaseMs = 120.0f;            // Smooth gust recovery release time (30.0 to 500.0 ms)
        bool deltaSolo = false;              // Audition extracted wind noise only
        bool bypass = false;                 // Bypass processing
    };

    JZDynamicWindTurbulenceSuppressorEngine(float sampleRate = 48000.0f);
    ~JZDynamicWindTurbulenceSuppressorEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setReductionDb(float db);
    float getReductionDb() const { return m_params.reductionDb; }

    void setSensitivity(float sens);
    float getSensitivity() const { return m_params.sensitivity; }

    void setCutoffRange(float minHz, float maxHz);
    float getMinCutoffHz() const { return m_params.minCutoffHz; }
    float getMaxCutoffHz() const { return m_params.maxCutoffHz; }

    void setDeltaSolo(bool deltaSolo);
    bool isDeltaSolo() const { return m_params.deltaSolo; }

    void setBypass(bool bp);
    bool isBypassed() const { return m_params.bypass; }

    /**
     * @brief Process stereo audio buffer through wind turbulence suppressor.
     */
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

    // Real-time telemetry
    float getCurrentCutoffHz() const { return m_currentCutoffHz; }
    float getWindTurbulenceLevelDb() const { return m_windTurbulenceDb; }
    float getGustSuppressionReductionDb() const { return m_gustReductionDb; }
    bool isGustDetected() const { return m_gustDetected; }

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    // Telemetry state
    float m_currentCutoffHz = 25.0f;
    float m_windTurbulenceDb = -96.0f;
    float m_gustReductionDb = 0.0f;
    bool m_gustDetected = false;

    // Energy tracking envelopes
    float m_subBassEnergyL = 0.0f;
    float m_subBassEnergyR = 0.0f;
    float m_midBandEnergyL = 0.0f;
    float m_midBandEnergyR = 0.0f;
    float m_gustEnvelope = 0.0f;

    // Detection sub-band filter states
    float m_detLpX1L = 0.0f, m_detLpX2L = 0.0f, m_detLpY1L = 0.0f, m_detLpY2L = 0.0f;
    float m_detLpX1R = 0.0f, m_detLpX2R = 0.0f, m_detLpY1R = 0.0f, m_detLpY2R = 0.0f;
    float m_detHpX1L = 0.0f, m_detHpX2L = 0.0f, m_detHpY1L = 0.0f, m_detHpY2L = 0.0f;
    float m_detHpX1R = 0.0f, m_detHpX2R = 0.0f, m_detHpY1R = 0.0f, m_detHpY2R = 0.0f;

    // Dynamic processing 2nd-order Butterworth High-pass filters (Stereo)
    float m_hpB0 = 1.0f, m_hpB1 = -2.0f, m_hpB2 = 1.0f;
    float m_hpA1 = 0.0f, m_hpA2 = 0.0f;
    float m_hpX1L = 0.0f, m_hpX2L = 0.0f, m_hpY1L = 0.0f, m_hpY2L = 0.0f;
    float m_hpX1R = 0.0f, m_hpX2R = 0.0f, m_hpY1R = 0.0f, m_hpY2R = 0.0f;

    void updateDetectionFilters();
    void updateDynamicHighpassCoeffs(float cutoffHz);
};

} // namespace Jazz

namespace jazz {
    using JZDynamicWindTurbulenceSuppressorEngine = Jazz::JZDynamicWindTurbulenceSuppressorEngine;
}

#endif // JAZZ_DYNAMIC_WIND_TURBULENCE_SUPPRESSOR_ENGINE_H
