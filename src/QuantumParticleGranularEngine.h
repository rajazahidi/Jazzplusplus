//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// QuantumParticleGranularEngine.h - Gravitational 3D Particle Granulator
//
// Sound design particle synthesis (Curtis Roads / Xenakis / GRM Tools style):
// Grains orbit in 3D gravitational / Lorentz attractor fields with Doppler
// pitch shifts and spatial binaural orbit panning.
//*****************************************************************************

#ifndef JAZZ_QUANTUM_PARTICLE_GRANULAR_ENGINE_H
#define JAZZ_QUANTUM_PARTICLE_GRANULAR_ENGINE_H

#include <vector>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <algorithm>

namespace Jazz {

class JZQuantumParticleGranularEngine {
public:
    struct ParticleGrain {
        float x, y, z;        // 3D position in meters (-10.0 to +10.0)
        float vx, vy, vz;     // Velocity vectors
        float samplePos;      // Reading position in source buffer
        float playbackSpeed;  // Natural playback rate
        float grainLife;      // 0.0 to 1.0 (envelope progress)
        float grainLifeStep;  // Progress per sample
        float mass;           // Gravitational mass
        bool active;
    };

    struct Configuration {
        size_t maxParticles = 32;          // Max simultaneous orbiting grains
        float gravityStrength = 4.5f;      // Central attractor gravitational pull
        float chaosFactor = 0.35f;         // Lorentz attractor non-linearity (0.0 to 1.0)
        float grainDurationMs = 45.0f;     // Average grain length (15.0 to 150.0 ms)
        float dopplerIntensity = 0.5f;     // Pitch velocity shift scaling
        float spraySpreadMeters = 3.0f;    // Spatial orbital radius
    };

    JZQuantumParticleGranularEngine(float sampleRate = 48000.0f);
    ~JZQuantumParticleGranularEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setConfiguration(const Configuration& config);
    const Configuration& getConfiguration() const { return m_config; }

    /**
     * @brief Load mono audio buffer used as grain source material.
     */
    void loadSourceBuffer(const std::vector<float>& sourceSamples);

    /**
     * @brief Synthesize spatialized granular particle cloud block.
     */
    void generateBlock(float* outL, float* outR, size_t numSamples);

    size_t getActiveParticleCount() const { return m_activeParticles; }
    size_t getSourceBufferSize() const { return m_sourceBuffer.size(); }

private:
    float m_sampleRate = 48000.0f;
    Configuration m_config;

    std::vector<float> m_sourceBuffer;
    std::vector<ParticleGrain> m_particles;
    size_t m_activeParticles = 0;
    uint32_t m_randomSeed = 123456789;

    float getPseudoRandom();
    void spawnParticle(ParticleGrain& p);
    void updateParticlePhysics(ParticleGrain& p);
};

} // namespace Jazz

namespace jazz {
    using JZQuantumParticleGranularEngine = Jazz::JZQuantumParticleGranularEngine;
}

#endif // JAZZ_QUANTUM_PARTICLE_GRANULAR_ENGINE_H
