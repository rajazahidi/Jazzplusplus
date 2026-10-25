//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// AtmosphericAirAbsorptionSpatialEngine.h - ISO 9613-1 Atmospheric Air
// Molecular Absorption & Acoustic Distance Attenuator
//
// Physical simulation of sound wave propagation and atmospheric absorption in air:
// ISO 9613-1 international standard molecular relaxation of Oxygen and Nitrogen,
// temperature (-10C to +40C), relative humidity (10% to 100%), distance (0.5m to 300m),
// geometric inverse-square law, propagation delay line (343 m/s), and close-mic proximity boost.
//*****************************************************************************

#ifndef JAZZ_ATMOSPHERIC_AIR_ABSORPTION_SPATIAL_ENGINE_H
#define JAZZ_ATMOSPHERIC_AIR_ABSORPTION_SPATIAL_ENGINE_H

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

class JZAtmosphericAirAbsorptionSpatialEngine {
public:
    struct Parameters {
        float distanceMeters = 10.0f;        // Sound source distance from listener (0.5 to 300.0 m)
        float temperatureCelsius = 20.0f;    // Ambient temperature (-10.0 to +40.0 C)
        float relativeHumidityPercent = 50.0f;// Relative humidity (10.0% to 100.0%)
        float referenceDistanceMeters = 1.0f;// Standard 1-meter reference for inverse-square law
        bool airAbsorptionEnabled = true;    // ISO 9613-1 frequency-dependent air damping
        bool distanceAttenuationEnabled = true; // 1/r geometric sound dissipation
        bool proximityBassBoostEnabled = true;// Proximity effect when d < referenceDistance
        bool bypass = false;
    };

    JZAtmosphericAirAbsorptionSpatialEngine(float sampleRate = 48000.0f);
    ~JZAtmosphericAirAbsorptionSpatialEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setDistanceMeters(float meters);
    float getDistanceMeters() const { return m_params.distanceMeters; }

    void setTemperatureCelsius(float tempC);
    float getTemperatureCelsius() const { return m_params.temperatureCelsius; }

    void setRelativeHumidityPercent(float rhPercent);
    float getRelativeHumidityPercent() const { return m_params.relativeHumidityPercent; }

    void setAirAbsorptionEnabled(bool enabled);
    bool isAirAbsorptionEnabled() const { return m_params.airAbsorptionEnabled; }

    void setDistanceAttenuationEnabled(bool enabled);
    bool isDistanceAttenuationEnabled() const { return m_params.distanceAttenuationEnabled; }

    void setBypass(bool bp);
    bool isBypassed() const { return m_params.bypass; }

    /**
     * @brief Process stereo audio buffer through atmospheric air absorption model.
     */
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

    // Physics Telemetry
    float getSpeedOfSoundMps() const { return m_speedOfSound; }
    float getPropagationDelayMs() const { return m_propagationDelayMs; }
    float getAbsorptionLossAt10kHzDb() const { return m_absorptionAt10kHzDb; }
    float getGeometricLossDb() const { return m_geometricLossDb; }

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    float m_speedOfSound = 343.2f;          // m/s
    float m_propagationDelayMs = 29.1f;     // ms
    float m_absorptionAt10kHzDb = 0.0f;
    float m_geometricLossDb = 0.0f;

    // Atmospheric lowpass absorption filter states (2nd-order shelving/lowpass)
    float m_fltB0 = 1.0f, m_fltB1 = 0.0f, m_fltB2 = 0.0f;
    float m_fltA1 = 0.0f, m_fltA2 = 0.0f;
    float m_fltX1L = 0.0f, m_fltX2L = 0.0f, m_fltY1L = 0.0f, m_fltY2L = 0.0f;
    float m_fltX1R = 0.0f, m_fltX2R = 0.0f, m_fltY1R = 0.0f, m_fltY2R = 0.0f;

    // Proximity bass boost filter states
    float m_proxB0 = 1.0f, m_proxB1 = 0.0f, m_proxA1 = 0.0f;
    float m_proxX1L = 0.0f, m_proxY1L = 0.0f;
    float m_proxX1R = 0.0f, m_proxY1R = 0.0f;

    // Acoustic propagation delay ring buffer
    std::vector<float> m_delayBufferL;
    std::vector<float> m_delayBufferR;
    size_t m_delayWritePos = 0;
    size_t m_delayBufferCapacity = 65536; // up to ~1.3 seconds (~450 meters)

    void updateAtmosphericAcoustics();
};

} // namespace Jazz

namespace jazz {
    using JZAtmosphericAirAbsorptionSpatialEngine = Jazz::JZAtmosphericAirAbsorptionSpatialEngine;
}

#endif // JAZZ_ATMOSPHERIC_AIR_ABSORPTION_SPATIAL_ENGINE_H
