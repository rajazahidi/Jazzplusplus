//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// MasterBusGlueVcaCompressorEngine.h - VCA Master Bus Glue Compressor
//
// Analog VCA master bus glue compressor (SSL G-Bus / API 2500 style):
// Soft-knee VCA gain reduction, program-dependent auto-release, and
// high-pass Thrust sidechain filter preventing kick drum pumping.
//*****************************************************************************

#ifndef JAZZ_MASTER_BUS_GLUE_VCA_COMPRESSOR_ENGINE_H
#define JAZZ_MASTER_BUS_GLUE_VCA_COMPRESSOR_ENGINE_H

#include <vector>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

class JZMasterBusGlueVcaCompressorEngine {
public:
    struct Parameters {
        float thresholdDb = -16.0f;          // Compression threshold (-32.0 to 0.0 dB)
        float ratio = 4.0f;                  // Ratio (1.5, 2.0, 4.0, 10.0)
        float attackMs = 10.0f;              // Attack time (0.1, 0.3, 1.0, 3.0, 10.0, 30.0 ms)
        float releaseSec = 0.4f;             // Release time (0.1, 0.3, 0.6, 1.2 s, or -1 for Auto)
        bool autoRelease = true;             // Program-dependent dual-time constant release
        float makeupGainDb = 3.5f;           // Post-compression makeup gain (0.0 to 18.0 dB)
        bool thrustFilterEnabled = true;     // High-pass Thrust sidechain filter
        float thrustHpFreqHz = 90.0f;        // Sidechain HP cutoff (60.0 to 240.0 Hz)
        float dryWet = 1.0f;                 // Parallel compression mix (0.0 to 1.0)
    };

    JZMasterBusGlueVcaCompressorEngine(float sampleRate = 48000.0f);
    ~JZMasterBusGlueVcaCompressorEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setThresholdDb(float db);
    float getThresholdDb() const { return m_params.thresholdDb; }

    void setRatio(float ratio);
    float getRatio() const { return m_params.ratio; }

    void setMakeupGainDb(float db);
    float getMakeupGainDb() const { return m_params.makeupGainDb; }

    void setThrustFilterEnabled(bool enabled);
    bool isThrustFilterEnabled() const { return m_params.thrustFilterEnabled; }

    /**
     * @brief Process stereo audio buffer through VCA Master Bus Glue Compressor.
     */
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

    float getCurrentGainReductionDb() const { return m_currentGrDb; }
    bool isCompressing() const { return m_currentGrDb < -0.5f; }

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    // Thrust high-pass sidechain biquad filter
    float m_scB0 = 1.0f, m_scB1 = 0.0f, m_scB2 = 0.0f, m_scA1 = 0.0f, m_scA2 = 0.0f;
    float m_scX1 = 0.0f, m_scX2 = 0.0f, m_scY1 = 0.0f, m_scY2 = 0.0f;

    float m_envelope = 0.0f;
    float m_autoReleaseSlow = 0.0f;
    float m_attackCoeff = 0.0f;
    float m_releaseCoeff = 0.0f;
    float m_currentGrDb = 0.0f;

    void updateBallistics();
    void updateThrustFilter();
};

} // namespace Jazz

namespace jazz {
    using JZMasterBusGlueVcaCompressorEngine = Jazz::JZMasterBusGlueVcaCompressorEngine;
}

#endif // JAZZ_MASTER_BUS_GLUE_VCA_COMPRESSOR_ENGINE_H
