//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// BinauralDopplerMotionSpatializerEngine.h - Binaural Relativistic Acoustic
// Doppler & Moving Sound Source Spatializer
//
// 3D vector velocity relativistic acoustic Doppler pitch shift and spatializer:
// Independent Left/Right fractional-delay sinc interpolation lines,
// 3D trajectory kinematics (position x, y, z and velocity vx, vy, vz),
// interaural parallax Doppler effect during high-speed fly-bys,
// Woodworth ITD head delay, and distance air attenuation.
//*****************************************************************************

#ifndef JAZZ_BINAURAL_DOPPLER_MOTION_SPATIALIZER_ENGINE_H
#define JAZZ_BINAURAL_DOPPLER_MOTION_SPATIALIZER_ENGINE_H

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

class JZBonauralDopplerMotionSpatializerEngine; // forward

class JZBinauralDopplerMotionSpatializerEngine {
public:
    struct Parameters {
        float posX = 0.0f;                   // 3D Cartesian position: X (left/right, -100m to +100m)
        float posY = 5.0f;                   // Y (front/back, -100m to +100m)
        float posZ = 0.0f;                   // Z (elevation up/down, -50m to +50m)
        float velX = 15.0f;                  // Velocity vector: Vx (m/s)
        float velY = 0.0f;                   // Vy (m/s)
        float velZ = 0.0f;                   // Vz (m/s)
        float speedOfSound = 343.2f;         // Air speed of sound (m/s)
        float headRadiusMeters = 0.0875f;    // Human head radius (meters, ~17.5cm ear spacing)
        bool dopplerEnabled = true;          // Enable physical Doppler pitch shift
        bool distanceAttenuation = true;     // 1/r geometric attenuation
        bool bypass = false;
    };

    JZBinauralDopplerMotionSpatializerEngine(float sampleRate = 48000.0f);
    ~JZBinauralDopplerMotionSpatializerEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setPosition(float x, float y, float z);
    void setVelocity(float vx, float vy, float vz);

    void setSpeedOfSound(float c);
    float getSpeedOfSound() const { return m_params.speedOfSound; }

    void setDopplerEnabled(bool enabled);
    bool isDopplerEnabled() const { return m_params.dopplerEnabled; }

    void setBypass(bool bp);
    bool isBypassed() const { return m_params.bypass; }

    /**
     * @brief Process stereo audio buffer through binaural Doppler motion spatializer.
     */
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

    // Physics Telemetry
    float getLeftEarDopplerRatio() const { return m_dopplerRatioL; }
    float getRightEarDopplerRatio() const { return m_dopplerRatioR; }
    float getCurrentDistanceMeters() const { return m_distanceMeters; }
    float getRadialVelocityMps() const { return m_radialVelocityMps; }

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    float m_dopplerRatioL = 1.0f;
    float m_dopplerRatioR = 1.0f;
    float m_distanceMeters = 5.0f;
    float m_radialVelocityMps = 0.0f;

    // Fractional-delay interpolation ring buffers (Left and Right)
    std::vector<float> m_delayRingL;
    std::vector<float> m_delayRingR;
    size_t m_ringWritePos = 0;
    size_t m_ringCapacity = 96000; // 2 seconds capacity at 48kHz

    // Current continuous fractional read pointers
    double m_readPosL = 0.0;
    double m_readPosR = 0.0;

    void updateKinematics();
};

} // namespace Jazz

namespace jazz {
    using JZBinauralDopplerMotionSpatializerEngine = Jazz::JZBinauralDopplerMotionSpatializerEngine;
}

#endif // JAZZ_BINAURAL_DOPPLER_MOTION_SPATIALIZER_ENGINE_H
