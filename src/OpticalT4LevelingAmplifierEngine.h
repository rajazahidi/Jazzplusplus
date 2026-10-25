//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// OpticalT4LevelingAmplifierEngine.h - T4 Optical Attenuator Leveling Amplifier
//
// Historic electro-luminescent T4 optical cell leveling amplifier (Teletronix
// LA-2A / Tube-Tech CL 1B style). Features dual-stage photocell memory decay,
// program-dependent release, 2-knob operation (Peak Reduction & Gain),
// Compress/Limit mode selection, and 12AX7/12BH7 tube cathode saturation.
//*****************************************************************************

#ifndef JAZZ_OPTICAL_T4_LEVELING_AMPLIFIER_ENGINE_H
#define JAZZ_OPTICAL_T4_LEVELING_AMPLIFIER_ENGINE_H

#include <vector>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

enum class OpticalLevelerMode {
    COMPRESS = 0,  // Soft-knee ~3:1 ratio for smooth vocal leveling
    LIMIT = 1      // Steep ~10:1 ratio for peak control
};

class JZOpticalT4LevelingAmplifierEngine {
public:
    struct Parameters {
        float peakReduction = 45.0f;         // Compression amount (0 to 100)
        float outputGain = 35.0f;            // Makeup gain (0 to 100)
        OpticalLevelerMode mode = OpticalLevelerMode::COMPRESS;
        float emphasis = 0.0f;               // High-frequency sidechain trim (flat to R37 broadcast trim)
        float tubeDrive = 1.0f;              // Cathode follower harmonic saturation (0.5 to 2.5)
        float dryWet = 1.0f;
        bool bypassed = false;
    };

    JZOpticalT4LevelingAmplifierEngine(float sampleRate = 48000.0f);
    ~JZOpticalT4LevelingAmplifierEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setPeakReduction(float pr);
    float getPeakReduction() const { return m_params.peakReduction; }

    void setOutputGain(float gain);
    float getOutputGain() const { return m_params.outputGain; }

    void setMode(OpticalLevelerMode mode);
    OpticalLevelerMode getMode() const { return m_params.mode; }

    void setEmphasis(float emphasis);
    float getEmphasis() const { return m_params.emphasis; }

    void setTubeDrive(float drive);
    float getTubeDrive() const { return m_params.tubeDrive; }

    void setDryWet(float wet);
    float getDryWet() const { return m_params.dryWet; }

    void setBypass(bool bypass);
    bool isBypassed() const { return m_params.bypassed; }

    // Telemetry & Getters
    float getCurrentGainReductionDb() const { return m_currentGrDb; }
    float getLuminescenceState() const { return m_photocellLuminescence; }
    float getThdPercent() const;
    bool isCompressing() const { return m_currentGrDb < -0.5f; }

    // Audio Processing
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    // T4 Dual-Stage Photocell Memory States
    float m_fastStage = 0.0f;
    float m_slowStage = 0.0f;
    float m_memoryHistory = 0.0f;
    float m_photocellLuminescence = 0.0f;
    float m_currentGrDb = 0.0f;

    // Emphasis high-pass sidechain filter
    float m_emphHpA = 0.0f;
    float m_emphState = 0.0f;
};

} // namespace Jazz

namespace jazz {
    using JZOpticalT4LevelingAmplifierEngine = Jazz::JZOpticalT4LevelingAmplifierEngine;
    using OpticalLevelerMode = Jazz::OpticalLevelerMode;
}

#endif // JAZZ_OPTICAL_T4_LEVELING_AMPLIFIER_ENGINE_H
