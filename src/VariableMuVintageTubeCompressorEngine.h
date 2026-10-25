//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// VariableMuVintageTubeCompressorEngine.h - Variable-Mu Tube Compressor
//
// Dual-triode 6386 push-pull remote-cutoff tube gain reduction simulation
// (Fairchild 670 / Manley Variable Mu style) featuring smooth continuous
// variable ratio (1.5:1 to 20:1), 6 historic time constants, program-dependent
// auto-release, triode harmonic saturation, and Mid/Side processing.
//*****************************************************************************

#ifndef JAZZ_VARIABLE_MU_VINTAGE_TUBE_COMPRESSOR_ENGINE_H
#define JAZZ_VARIABLE_MU_VINTAGE_TUBE_COMPRESSOR_ENGINE_H

#include <vector>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

enum class VariableMuTimeConstant {
    FAST_0_3S = 1,       // Mode 1: Attack 0.2ms, Release 0.3s
    MEDIUM_0_8S = 2,     // Mode 2: Attack 0.2ms, Release 0.8s
    SLOW_2_0S = 3,       // Mode 3: Attack 0.4ms, Release 2.0s
    WARM_5_0S = 4,       // Mode 4: Attack 0.8ms, Release 5.0s
    AUTO_PROGRAM_1 = 5,  // Mode 5: Attack 0.4ms, Auto 2s short / 10s sustained
    AUTO_PROGRAM_2 = 6   // Mode 6: Attack 0.2ms, Auto 0.8s short / 25s sustained
};

class JZVariableMuVintageTubeCompressorEngine {
public:
    struct Parameters {
        float thresholdDb = -18.0f;          // Input threshold (-36 to 0 dB)
        VariableMuTimeConstant timeConstant = VariableMuTimeConstant::AUTO_PROGRAM_1;
        float inputDrive = 1.0f;             // 6386 tube saturation drive (0.5 to 3.0)
        float outputGainDb = 4.0f;           // Makeup gain (-12 to +18 dB)
        float sidechainHpHz = 90.0f;         // High-pass filter on sidechain (0 to 250 Hz)
        bool stereoLink = true;              // Linked L/R envelope detection
        bool midSideMode = false;            // Mid/Side matrix compression
        float dryWet = 1.0f;                 // Parallel blend (0.0 to 1.0)
        bool bypassed = false;
    };

    JZVariableMuVintageTubeCompressorEngine(float sampleRate = 48000.0f);
    ~JZVariableMuVintageTubeCompressorEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setThresholdDb(float db);
    float getThresholdDb() const { return m_params.thresholdDb; }

    void setTimeConstant(VariableMuTimeConstant tc);
    VariableMuTimeConstant getTimeConstant() const { return m_params.timeConstant; }

    void setInputDrive(float drive);
    float getInputDrive() const { return m_params.inputDrive; }

    void setOutputGainDb(float db);
    float getOutputGainDb() const { return m_params.outputGainDb; }

    void setSidechainHpHz(float hz);
    float getSidechainHpHz() const { return m_params.sidechainHpHz; }

    void setStereoLink(bool linked);
    bool isStereoLinked() const { return m_params.stereoLink; }

    void setMidSideMode(bool ms);
    bool isMidSideMode() const { return m_params.midSideMode; }

    void setDryWet(float wet);
    float getDryWet() const { return m_params.dryWet; }

    void setBypass(bool bypass);
    bool isBypassed() const { return m_params.bypassed; }

    // Telemetry & Getters
    float getCurrentGainReductionDb() const { return m_currentGrDb; }
    float getTubeBiasVolts() const { return m_currentBiasVolts; }
    float getThdPercent() const;
    bool isCompressing() const { return m_currentGrDb < -0.5f; }

    // Processing
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

private:
    void updateBallistics();
    void updateSidechainFilters();

    float m_sampleRate = 48000.0f;
    Parameters m_params;

    float m_attackCoeffFast = 0.0f;
    float m_releaseCoeffFast = 0.0f;
    float m_releaseCoeffSlow = 0.0f;

    float m_envL = 0.0f;
    float m_envR = 0.0f;
    float m_envSlowL = 0.0f;
    float m_envSlowR = 0.0f;

    float m_currentGrDb = 0.0f;
    float m_currentBiasVolts = -6.3f;

    // Sidechain 1st-order HP filter
    float m_scHpA = 0.0f;
    float m_scStateL = 0.0f;
    float m_scStateR = 0.0f;
};

} // namespace Jazz

namespace jazz {
    using JZVariableMuVintageTubeCompressorEngine = Jazz::JZVariableMuVintageTubeCompressorEngine;
    using VariableMuTimeConstant = Jazz::VariableMuTimeConstant;
}

#endif // JAZZ_VARIABLE_MU_VINTAGE_TUBE_COMPRESSOR_ENGINE_H
