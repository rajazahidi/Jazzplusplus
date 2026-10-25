//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZVinylLatheMechanicalSimulator.h - Vinyl Cutting Lathe & Stylus Simulator
//
// Mastering vinyl disc cutting lathe physics (Neumann VMS-80 / SX-74 style):
// High-frequency stylus acceleration limiting (prevents cutter coil burnout),
// lateral/vertical groove displacement & lift-off protection, inner-groove
// tracing distortion (Pinch Effect), and 33-1/3 vs 45 RPM mechanical modeling.
//*****************************************************************************

#ifndef JAZZ_VINYL_LATHE_MECHANICAL_SIMULATOR_H
#define JAZZ_VINYL_LATHE_MECHANICAL_SIMULATOR_H

#include <vector>
#include <string>
#include <cstdint>

namespace Jazz {

enum class VinylRotationSpeed {
    RPM_33_THIRD, // 33 1/3 RPM (Standard 12" LP)
    RPM_45        // 45 RPM (Audiophile 12" / 7" Single)
};

enum class StylusPlaybackProfile {
    ELLIPTICAL,   // Modern high-compliance 0.3 x 0.7 mil elliptical
    CONICAL,      // Vintage spherical / DJ 0.7 mil conical (higher inner distortion)
    MICRO_RIDGE   // Advanced audiophile line-contact (lowest pinch distortion)
};

struct VinylGrooveTelemetry {
    float peakLateralVelocityCmS;    // Max horizontal stylus velocity (safety < 25 cm/s)
    float peakVerticalExcursionUm;   // Vertical depth modulation (safety < 30 um)
    float peakAccelerationG;         // Stylus acceleration in g-force units
    float innerGrooveRadiusCm;       // Current disc radius (14.5cm outer to 5.8cm inner)
    bool accelerationLimited;        // True if HF acceleration limiter engaged
    bool verticalLiftWarning;        // True if needle jump / lift-off danger detected
};

class JZVinylLatheMechanicalSimulator {
public:
    JZVinylLatheMechanicalSimulator(float sampleRate = 48000.0f,
                                   VinylRotationSpeed speed = VinylRotationSpeed::RPM_33_THIRD);
    ~JZVinylLatheMechanicalSimulator() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return m_sampleRate; }

    void setRotationSpeed(VinylRotationSpeed speed);
    VinylRotationSpeed getRotationSpeed() const { return m_speed; }

    void setStylusProfile(StylusPlaybackProfile profile);
    StylusPlaybackProfile getStylusProfile() const { return m_stylusProfile; }

    // Physical Disc Location
    void setCurrentRadiusCm(float radiusCm); // 5.5 cm (inner lead-out) to 14.5 cm (outer lead-in)
    float getCurrentRadiusCm() const { return m_currentRadiusCm; }

    // Safety Guards & Modeling Controls
    void setAccelerationLimitEnabled(bool enable) { m_accelLimitEnabled = enable; }
    bool isAccelerationLimitEnabled() const { return m_accelLimitEnabled; }

    void setEllipticalBassMonoFrequencyHz(float freqHz);
    float getEllipticalBassMonoFrequencyHz() const { return m_ellipticalBassMonoHz; }

    void setTracingDistortionModeling(bool enable) { m_modelTracingDistortion = enable; }
    bool isTracingDistortionModelingEnabled() const { return m_modelTracingDistortion; }

    // Real-Time Audio Processing
    void process(const float* inL, const float* inR,
                 float* outL, float* outR, size_t numSamples);

    VinylGrooveTelemetry getTelemetry() const { return m_telemetry; }

    void reset();

private:
    float m_sampleRate;
    VinylRotationSpeed m_speed;
    StylusPlaybackProfile m_stylusProfile;
    float m_currentRadiusCm;
    bool m_accelLimitEnabled;
    float m_ellipticalBassMonoHz;
    bool m_modelTracingDistortion;

    VinylGrooveTelemetry m_telemetry;

    // Previous sample history for 1st & 2nd derivative (velocity & acceleration)
    float m_prevL, m_prevR;
    float m_prevVelL, m_prevVelR;

    // Elliptical equalizer high-pass filter state for L - R (vertical bass mono)
    float m_vertHpStateL, m_vertHpStateR;
};

} // namespace Jazz

#endif // JAZZ_VINYL_LATHE_MECHANICAL_SIMULATOR_H
