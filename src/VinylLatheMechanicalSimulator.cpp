//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZVinylLatheMechanicalSimulator.cpp - Vinyl Cutting Lathe & Stylus Simulator
//*****************************************************************************

#include "VinylLatheMechanicalSimulator.h"
#include <algorithm>
#include <cmath>

namespace Jazz {

JZVinylLatheMechanicalSimulator::JZVinylLatheMechanicalSimulator(float sampleRate, VinylRotationSpeed speed)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
    , m_speed(speed)
    , m_stylusProfile(StylusPlaybackProfile::ELLIPTICAL)
    , m_currentRadiusCm(14.0f) // Start at outer diameter (lead-in)
    , m_accelLimitEnabled(true)
    , m_ellipticalBassMonoHz(120.0f)
    , m_modelTracingDistortion(true)
    , m_prevL(0.0f), m_prevR(0.0f)
    , m_prevVelL(0.0f), m_prevVelR(0.0f)
    , m_vertHpStateL(0.0f), m_vertHpStateR(0.0f)
{
    reset();
}

void JZVinylLatheMechanicalSimulator::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        m_sampleRate = sampleRate;
        reset();
    }
}

void JZVinylLatheMechanicalSimulator::setRotationSpeed(VinylRotationSpeed speed) {
    m_speed = speed;
}

void JZVinylLatheMechanicalSimulator::setStylusProfile(StylusPlaybackProfile profile) {
    m_stylusProfile = profile;
}

void JZVinylLatheMechanicalSimulator::setCurrentRadiusCm(float radiusCm) {
    m_currentRadiusCm = std::clamp(radiusCm, 5.5f, 15.0f);
}

void JZVinylLatheMechanicalSimulator::setEllipticalBassMonoFrequencyHz(float freqHz) {
    m_ellipticalBassMonoHz = std::clamp(freqHz, 40.0f, 300.0f);
}

void JZVinylLatheMechanicalSimulator::reset() {
    m_prevL = m_prevR = 0.0f;
    m_prevVelL = m_prevVelR = 0.0f;
    m_vertHpStateL = m_vertHpStateR = 0.0f;

    m_telemetry.peakLateralVelocityCmS = 0.0f;
    m_telemetry.peakVerticalExcursionUm = 0.0f;
    m_telemetry.peakAccelerationG = 0.0f;
    m_telemetry.innerGrooveRadiusCm = m_currentRadiusCm;
    m_telemetry.accelerationLimited = false;
    m_telemetry.verticalLiftWarning = false;
}

void JZVinylLatheMechanicalSimulator::process(const float* inL, const float* inR,
                                             float* outL, float* outR, size_t numSamples) {
    if (!inL || !inR || !outL || !outR || numSamples == 0) return;

    // Linear groove velocity: v = omega * r
    // 33.33 RPM: omega = 3.49 rad/s. At r = 14cm -> v = 48.8 cm/s. At r = 6cm -> v = 20.9 cm/s
    double rpm = (m_speed == VinylRotationSpeed::RPM_45) ? 45.0 : 33.33333333;
    double omega = (rpm * 2.0 * 3.14159265) / 60.0;
    double linearGrooveVelocityCmS = omega * m_currentRadiusCm;

    // Elliptical high-pass crossover filter coefficients for vertical (side) channel
    float dt = 1.0f / m_sampleRate;
    float rc = 1.0f / (2.0f * 3.14159265f * m_ellipticalBassMonoHz);
    float alphaHp = rc / (rc + dt);

    float maxLatVel = 0.0f;
    float maxVertExc = 0.0f;
    float maxAccel = 0.0f;
    bool accelEngaged = false;
    bool vertWarning = false;

    // Standard scaling: 1.0 peak audio = ~7.0 cm/s reference peak lateral velocity
    const float VELOCITY_SCALE = 7.0f;

    for (size_t i = 0; i < numSamples; ++i) {
        float sL = inL[i];
        float sR = inR[i];

        // Mid/Side matrix
        float mid = (sL + sR) * 0.5f;   // Lateral motion
        float side = (sL - sR) * 0.5f;  // Vertical depth modulation

        // High-pass filter side channel (elliptical bass mono guard)
        // HP = input - LP
        m_vertHpStateL += (dt / (rc + dt)) * (side - m_vertHpStateL);
        float filteredSide = side - m_vertHpStateL;

        // Calculate vertical groove excursion depth (um)
        float vertExcursionUm = std::abs(filteredSide) * 25.0f; // in micrometers
        if (vertExcursionUm > maxVertExc) maxVertExc = vertExcursionUm;
        if (vertExcursionUm > 28.0f) {
            vertWarning = true; // Danger of stylus lifting off groove bottom!
            filteredSide = std::clamp(filteredSide, -1.1f, 1.1f);
        }

        // Reconstruct Mid/Side to L/R
        float procL = mid + filteredSide;
        float procR = mid - filteredSide;

        // 1st derivative: Velocity = d(x)/dt
        float velL = (procL - m_prevL) * m_sampleRate;
        float velR = (procR - m_prevR) * m_sampleRate;
        m_prevL = procL;
        m_prevR = procR;

        float latVel = (std::abs(velL) + std::abs(velR)) * 0.5f * (VELOCITY_SCALE / m_sampleRate);
        if (latVel > maxLatVel) maxLatVel = latVel;

        // 2nd derivative: Acceleration = d(vel)/dt
        float accL = (velL - m_prevVelL) * m_sampleRate;
        float accR = (velR - m_prevVelR) * m_sampleRate;
        m_prevVelL = velL;
        m_prevVelR = velR;

        // Convert to g-force units (1g = 981 cm/s^2)
        float accelG = std::max(std::abs(accL), std::abs(accR)) / (981.0f * m_sampleRate);
        if (accelG > maxAccel) maxAccel = accelG;

        // High-Frequency Stylus Acceleration Limiter
        if (m_accelLimitEnabled && accelG > 12.0f) {
            accelEngaged = true;
            float limitGain = 12.0f / accelG;
            procL = (procL * limitGain);
            procR = (procR * limitGain);
        }

        // Pinch Effect / Inner Groove Tracing Distortion
        if (m_modelTracingDistortion && linearGrooveVelocityCmS < 35.0) {
            // Wavelength compression near inner radius induces 2nd harmonic distortion
            float distCoeff = static_cast<float>((35.0 - linearGrooveVelocityCmS) / 35.0) * 0.04f;
            if (m_stylusProfile == StylusPlaybackProfile::CONICAL) distCoeff *= 2.0f;
            if (m_stylusProfile == StylusPlaybackProfile::MICRO_RIDGE) distCoeff *= 0.3f;

            procL += (procL * procL) * distCoeff;
            procR += (procR * procR) * distCoeff;
        }

        outL[i] = std::clamp(procL, -1.0f, 1.0f);
        outR[i] = std::clamp(procR, -1.0f, 1.0f);
    }

    m_telemetry.peakLateralVelocityCmS = maxLatVel;
    m_telemetry.peakVerticalExcursionUm = maxVertExc;
    m_telemetry.peakAccelerationG = maxAccel;
    m_telemetry.innerGrooveRadiusCm = m_currentRadiusCm;
    m_telemetry.accelerationLimited = accelEngaged;
    m_telemetry.verticalLiftWarning = vertWarning;
}

} // namespace Jazz
