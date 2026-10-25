//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// HelicalSpringDispersionEngine.h - Physical Helical Spring Reverb Modeler
//
// Accutronics / Roland Space Echo mechanical spring reverb tank physical modeling:
// Frequency-dependent chirp dispersion cascade (highs delayed relative to lows),
// multi-spring helical wire transmission, transducer saturation, and tank kick impact.
//*****************************************************************************

#ifndef JAZZ_HELICAL_SPRING_DISPERSION_ENGINE_H
#define JAZZ_HELICAL_SPRING_DISPERSION_ENGINE_H

#include <vector>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

class JZHelicalSpringDispersionEngine {
public:
    struct Parameters {
        float decayTimeSec = 3.2f;         // Reverb decay time in seconds (1.0 to 8.0)
        float dispersion = 0.75f;          // Helical chirp dispersion amount (0.0 to 1.0)
        float dampingDampFreqHz = 3500.0f; // High frequency spring damping
        float drive = 1.3f;                // Input transducer coil saturation
        float dryWet = 0.4f;               // Dry / wet blend
        float stereoSpread = 0.6f;         // Stereo spring phase decoupling
    };

    JZHelicalSpringDispersionEngine(float sampleRate = 48000.0f);
    ~JZHelicalSpringDispersionEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setDecayTimeSec(float sec);
    float getDecayTimeSec() const { return m_params.decayTimeSec; }

    void setDispersion(float dispersion);
    float getDispersion() const { return m_params.dispersion; }

    void setDryWet(float mix);
    float getDryWet() const { return m_params.dryWet; }

    /**
     * @brief Trigger mechanical spring tank physical bump/kick impact (surf-rock thunder).
     */
    void triggerTankKickImpact(float intensity = 1.0f);

    /**
     * @brief Process stereo audio buffer through physical helical spring model.
     */
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

    float getSpringTensionEnergy() const { return m_springEnergy; }

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    // 3 parallel helical springs with non-commensurate prime lengths (in samples)
    struct SpringLine {
        std::vector<float> delay;
        size_t writePos = 0;
        size_t length = 1200;
        float feedback = 0.85f;
        float dampState = 0.0f;

        // Dispersive all-pass filter cascade for chirp effect (6 all-pass stages per spring)
        static const size_t NUM_DISP_STAGES = 6;
        float apX[NUM_DISP_STAGES] = {0};
        float apY[NUM_DISP_STAGES] = {0};
    };

    SpringLine m_springs[3];

    // Tank mechanical kick excitation state
    float m_tankKickEnergy = 0.0f;
    float m_tankKickPhase = 0.0f;

    float m_springEnergy = 0.0f;

    void updateSpringParameters();
};

} // namespace Jazz

namespace jazz {
    using JZHelicalSpringDispersionEngine = Jazz::JZHelicalSpringDispersionEngine;
}

#endif // JAZZ_HELICAL_SPRING_DISPERSION_ENGINE_H
