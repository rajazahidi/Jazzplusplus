//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// RotaFonicRotarySpeakerSimulator.h - Leslie 122/147 Rotary Speaker Simulator
//
// Electro-mechanical dual-rotor Leslie speaker cabinet physical simulation:
// Rotating treble horn Doppler shift, wooden bass drum amplitude modulation,
// mechanical motor inertia acceleration/braking, and 12AX7 tube pre-amp drive.
//*****************************************************************************

#ifndef JAZZ_ROTA_FONIC_ROTARY_SPEAKER_SIMULATOR_H
#define JAZZ_ROTA_FONIC_ROTARY_SPEAKER_SIMULATOR_H

#include <vector>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

enum class RotarySpeedMode {
    BRAKE_STOP = 0, // Motor stopped
    CHORALE    = 1, // Slow lush speed (~40-48 RPM)
    TREMOLO    = 2  // Fast vibrato speed (~340-408 RPM)
};

class JZRotaFonicRotarySpeakerSimulator {
public:
    struct Parameters {
        RotarySpeedMode mode = RotarySpeedMode::CHORALE;
        float drive = 1.35f;                 // 12AX7 tube saturation (1.0 to 4.0)
        float hornDrumBalance = 0.55f;       // Balance between horn and drum (0.2 to 0.8)
        float micAngleDeg = 90.0f;           // Stereo mic cabinet placement angle (60 to 180 deg)
        float dryWet = 1.0f;                 // Wet / dry blend
    };

    JZRotaFonicRotarySpeakerSimulator(float sampleRate = 48000.0f);
    ~JZRotaFonicRotarySpeakerSimulator() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setSpeedMode(RotarySpeedMode mode);
    RotarySpeedMode getSpeedMode() const { return m_params.mode; }

    void setDrive(float drive);
    float getDrive() const { return m_params.drive; }

    void setHornDrumBalance(float balance);
    float getHornDrumBalance() const { return m_params.hornDrumBalance; }

    /**
     * @brief Process stereo audio buffer through rotary speaker cabinet.
     */
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

    float getHornRpm() const { return (m_currentHornFreqHz * 60.0f); }
    float getDrumRpm() const { return (m_currentDrumFreqHz * 60.0f); }

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    // 800 Hz Leslie Crossover filter
    float m_xoverB0, m_xoverB1, m_xoverB2, m_xoverA1, m_xoverA2;
    float m_xoverHpB0, m_xoverHpB1, m_xoverHpB2;
    float m_xLpX1, m_xLpX2, m_xLpY1, m_xLpY2;
    float m_xHpX1, m_xHpX2, m_xHpY1, m_xHpY2;

    // Doppler delay line for rotating horn
    static const size_t HORN_BUF_SIZE = 2048;
    std::vector<float> m_hornDelayBuf;
    size_t m_hornWritePos = 0;

    // Mechanical motor speed tracking
    float m_targetHornFreqHz = 0.8f;
    float m_targetDrumFreqHz = 0.7f;
    float m_currentHornFreqHz = 0.8f;
    float m_currentDrumFreqHz = 0.7f;

    float m_hornAngleRad = 0.0f;
    float m_drumAngleRad = 0.0f;

    void updateCrossover();
    void updateTargetSpeeds();
};

} // namespace Jazz

namespace jazz {
    using JZRotaFonicRotarySpeakerSimulator = Jazz::JZRotaFonicRotarySpeakerSimulator;
    using RotarySpeedMode = Jazz::RotarySpeedMode;
}

#endif // JAZZ_ROTA_FONIC_ROTARY_SPEAKER_SIMULATOR_H
