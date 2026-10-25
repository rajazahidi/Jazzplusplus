//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// ReactiveLoadSpeakerImpedanceEngine.h - Reactive Load & Power Soak Simulator
//
// Guitar tube amp reactive load box simulation (Universal Audio OX style):
// Models complex voice-coil impedance curve Z(w), speaker cone back-EMF,
// power transformer saturation, and power supply sag dynamics.
//*****************************************************************************

#ifndef JAZZ_REACTIVE_LOAD_SPEAKER_IMPEDANCE_ENGINE_H
#define JAZZ_REACTIVE_LOAD_SPEAKER_IMPEDANCE_ENGINE_H

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

enum class SpeakerCabinetType {
    VINTAGE_4X12_V30   = 0, // Tight punchy 75Hz resonance, aggressive inductive bite
    OPEN_BACK_2X12     = 1, // Airy 85Hz resonance, warm smooth top
    TWEED_DELUXE_1X12  = 2, // Looser 95Hz resonance, high sag compression
    CLASSIC_4X10_ALNICO = 3  // Fast snappy 68Hz resonance, scooped upper mids
};

class JZReactiveLoadSpeakerImpedanceEngine {
public:
    struct Parameters {
        SpeakerCabinetType cabType = SpeakerCabinetType::VINTAGE_4X12_V30;
        float resonanceFreqHz = 75.0f;       // Voice-coil mechanical resonance (50.0 to 110.0 Hz)
        float inductiveRise = 1.0f;          // Voice-coil inductance L (0.2 to 2.5)
        float powerSagAmount = 0.45f;        // Power supply sag compression (0.0 to 1.0)
        float powerSoakDamping = 0.85f;      // Back-EMF magnetic damping (0.2 to 1.5)
        float dryWet = 1.0f;
    };

    JZReactiveLoadSpeakerImpedanceEngine(float sampleRate = 48000.0f);
    ~JZReactiveLoadSpeakerImpedanceEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setCabinetType(SpeakerCabinetType type);
    SpeakerCabinetType getCabinetType() const { return m_params.cabType; }

    void setResonanceFreqHz(float hz);
    float getResonanceFreqHz() const { return m_params.resonanceFreqHz; }

    void setPowerSagAmount(float sag);
    float getPowerSagAmount() const { return m_params.powerSagAmount; }

    /**
     * @brief Process stereo audio buffer through reactive load box simulation.
     */
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

    float getImpedancePeakOhms() const { return 32.0f; }
    float getPowerSagReductionDb() const { return m_powerSagReductDb; }
    std::string getCabinetTypeName() const;

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    // Bass mechanical resonance peaking biquad (Z_res at 75 Hz)
    float m_resB0 = 1.0f, m_resB1 = 0.0f, m_resB2 = 0.0f, m_resA1 = 0.0f, m_resA2 = 0.0f;
    float m_resX1L = 0.0f, m_resX2L = 0.0f, m_resY1L = 0.0f, m_resY2L = 0.0f;
    float m_resX1R = 0.0f, m_resX2R = 0.0f, m_resY1R = 0.0f, m_resY2R = 0.0f;

    // Inductive high-frequency rise shelving biquad (Z_L from 2.5 kHz to 12 kHz)
    float m_indB0 = 1.0f, m_indB1 = 0.0f, m_indB2 = 0.0f, m_indA1 = 0.0f, m_indA2 = 0.0f;
    float m_indX1L = 0.0f, m_indX2L = 0.0f, m_indY1L = 0.0f, m_indY2L = 0.0f;
    float m_indX1R = 0.0f, m_indX2R = 0.0f, m_indY1R = 0.0f, m_indY2R = 0.0f;

    // Power supply sag envelope follower
    float m_sagEnvelope = 0.0f;
    float m_powerSagReductDb = 0.0f;

    void updateImpedanceFilters();
};

} // namespace Jazz

namespace jazz {
    using JZReactiveLoadSpeakerImpedanceEngine = Jazz::JZReactiveLoadSpeakerImpedanceEngine;
    using SpeakerCabinetType = Jazz::SpeakerCabinetType;
}

#endif // JAZZ_REACTIVE_LOAD_SPEAKER_IMPEDANCE_ENGINE_H
