//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// SteelPlateReverbPhysicalEngine.h - Physical Steel Plate Reverb (EMT 140)
//
// Physical modal modeling of historical mechanical steel plate reverberation:
// Tensioned cold-rolled steel sheet, electromagnetic driver coil saturation,
// asymmetrical dual-piezo pickups, and mechanical porous damper pad decay.
//*****************************************************************************

#ifndef JAZZ_STEEL_PLATE_REVERB_PHYSICAL_ENGINE_H
#define JAZZ_STEEL_PLATE_REVERB_PHYSICAL_ENGINE_H

#include <vector>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

class JZSteelPlateReverbPhysicalEngine {
public:
    static const size_t NUM_PLATE_MODES = 16;

    struct Parameters {
        float decayTimeSec = 2.4f;           // Mechanical damper pad decay (0.5 to 5.5 sec)
        float plateTension = 1.0f;           // Cold-rolled sheet tension factor (0.6 to 1.5)
        float driverDrive = 1.2f;            // Electromagnetic coil saturation (1.0 to 3.5)
        float highFrequencyDamping = 0.45f;  // Air and boundary viscous loss (0.1 to 0.9)
        float bassCutoffHz = 80.0f;          // Pre-drive mechanical high-pass filter
        float dryWet = 0.35f;                // Reverb mix blend (0.0 to 1.0)
    };

    struct PlateMode {
        float freqHz;
        float pickupWeightL;
        float pickupWeightR;
        float b0, b1, b2, a1, a2;
        float x1, x2, y1, y2;

        void reset() {
            x1 = x2 = y1 = y2 = 0.0f;
        }
    };

    JZSteelPlateReverbPhysicalEngine(float sampleRate = 48000.0f);
    ~JZSteelPlateReverbPhysicalEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setDecayTimeSec(float sec);
    float getDecayTimeSec() const { return m_params.decayTimeSec; }

    void setDriverDrive(float drive);
    float getDriverDrive() const { return m_params.driverDrive; }

    void setPlateTension(float tension);
    float getPlateTension() const { return m_params.plateTension; }

    /**
     * @brief Process stereo audio buffer through physical plate reverb engine.
     */
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

    float getDamperDistanceMm() const { return m_damperDistanceMm; }
    size_t getActivePlateModes() const { return NUM_PLATE_MODES; }

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    PlateMode m_modes[NUM_PLATE_MODES];
    float m_damperDistanceMm = 12.0f;

    // Pre-emphasis bass cut filter
    float m_hpB0 = 1.0f, m_hpB1 = 0.0f, m_hpB2 = 0.0f, m_hpA1 = 0.0f, m_hpA2 = 0.0f;
    float m_hpX1 = 0.0f, m_hpX2 = 0.0f, m_hpY1 = 0.0f, m_hpY2 = 0.0f;

    void updatePlateModalMesh();
};

} // namespace Jazz

namespace jazz {
    using JZSteelPlateReverbPhysicalEngine = Jazz::JZSteelPlateReverbPhysicalEngine;
}

#endif // JAZZ_STEEL_PLATE_REVERB_PHYSICAL_ENGINE_H
