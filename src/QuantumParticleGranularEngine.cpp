//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// QuantumParticleGranularEngine.cpp - Gravitational 3D Particle Granulator
//*****************************************************************************

#include "QuantumParticleGranularEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

JZQuantumParticleGranularEngine::JZQuantumParticleGranularEngine(float sampleRate) {
    initialize(sampleRate);
}

void JZQuantumParticleGranularEngine::initialize(float sampleRate) {
    if (sampleRate < 8000.0f) sampleRate = 48000.0f;
    m_sampleRate = sampleRate;

    // Default rich harmonic source buffer
    m_sourceBuffer.assign(4096, 0.0f);
    for (size_t i = 0; i < m_sourceBuffer.size(); ++i) {
        float phase = 2.0f * static_cast<float>(M_PI) * (static_cast<float>(i) / static_cast<float>(m_sourceBuffer.size()));
        m_sourceBuffer[i] = 0.6f * std::sin(phase * 4.0f) + 0.3f * std::sin(phase * 7.0f) + 0.1f * std::sin(phase * 11.0f);
    }

    reset();
}

void JZQuantumParticleGranularEngine::reset() {
    m_particles.resize(m_config.maxParticles);
    m_randomSeed = 123456789;
    m_activeParticles = 0;

    for (auto& p : m_particles) {
        p.active = false;
        spawnParticle(p);
    }
}

void JZQuantumParticleGranularEngine::setConfiguration(const Configuration& config) {
    m_config = config;
    if (m_particles.size() != m_config.maxParticles) {
        m_particles.resize(m_config.maxParticles);
        reset();
    }
}

void JZQuantumParticleGranularEngine::loadSourceBuffer(const std::vector<float>& sourceSamples) {
    if (!sourceSamples.empty()) {
        m_sourceBuffer = sourceSamples;
    }
}

float JZQuantumParticleGranularEngine::getPseudoRandom() {
    m_randomSeed = m_randomSeed * 1664525u + 1013904223u;
    return static_cast<float>(m_randomSeed & 0x7FFFFFFF) / static_cast<float>(0x7FFFFFFF);
}

void JZQuantumParticleGranularEngine::spawnParticle(ParticleGrain& p) {
    float spread = m_config.spraySpreadMeters;
    p.x = (getPseudoRandom() * 2.0f - 1.0f) * spread;
    p.y = (getPseudoRandom() * 2.0f - 1.0f) * spread;
    p.z = (getPseudoRandom() * 2.0f - 1.0f) * spread * 0.5f;

    // Tangential orbital initial velocity
    p.vx = -p.y * 0.8f + (getPseudoRandom() - 0.5f) * 0.2f;
    p.vy = p.x * 0.8f + (getPseudoRandom() - 0.5f) * 0.2f;
    p.vz = (getPseudoRandom() - 0.5f) * 0.5f;

    p.samplePos = getPseudoRandom() * static_cast<float>(m_sourceBuffer.size() > 1 ? (m_sourceBuffer.size() - 1) : 0);
    p.playbackSpeed = 0.85f + getPseudoRandom() * 0.3f; // Slight pitch variation

    float grainDurationSec = (m_config.grainDurationMs / 1000.0f) * (0.8f + getPseudoRandom() * 0.4f);
    float totalSamples = grainDurationSec * m_sampleRate;
    p.grainLife = 0.0f;
    p.grainLifeStep = (totalSamples > 0.0f) ? (1.0f / totalSamples) : 0.01f;
    p.mass = 0.5f + getPseudoRandom() * 1.0f;
    p.active = true;
}

void JZQuantumParticleGranularEngine::updateParticlePhysics(ParticleGrain& p) {
    float dt = 0.001f;
    float r = std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z) + 0.2f;

    // Gravitational acceleration towards center
    float Fg = (m_config.gravityStrength * p.mass) / (r * r);
    float ax = -Fg * (p.x / r);
    float ay = -Fg * (p.y / r);
    float az = -Fg * (p.z / r);

    // Lorentz chaotic attractor perturbation
    float chaos = m_config.chaosFactor * 0.1f;
    float lx = 10.0f * (p.y - p.x) * chaos;
    float ly = (p.x * (28.0f - p.z) - p.y) * chaos;
    float lz = (p.x * p.y - 2.6667f * p.z) * chaos;

    p.vx += (ax + lx) * dt;
    p.vy += (ay + ly) * dt;
    p.vz += (az + lz) * dt;

    p.x += p.vx * dt;
    p.y += p.vy * dt;
    p.z += p.vz * dt;
}

void JZQuantumParticleGranularEngine::generateBlock(float* outL, float* outR, size_t numSamples) {
    if (!outL || numSamples == 0 || m_sourceBuffer.empty()) return;

    std::fill(outL, outL + numSamples, 0.0f);
    if (outR) std::fill(outR, outR + numSamples, 0.0f);

    size_t activeCount = 0;

    for (size_t s = 0; s < numSamples; ++s) {
        float sampleAccumL = 0.0f;
        float sampleAccumR = 0.0f;

        for (auto& p : m_particles) {
            if (!p.active) {
                spawnParticle(p);
            }

            // Radial distance and velocity for Doppler effect
            float r = std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z) + 0.1f;
            float vr = (p.x * p.vx + p.y * p.vy + p.z * p.vz) / r;
            float doppler = p.playbackSpeed * (1.0f - (vr / 343.0f) * m_config.dopplerIntensity);
            if (doppler < 0.2f) doppler = 0.2f;

            // Grain Hann window envelope
            float env = 0.5f * (1.0f - std::cos(2.0f * static_cast<float>(M_PI) * p.grainLife));

            // Sample from source buffer using linear interpolation
            size_t idx0 = static_cast<size_t>(p.samplePos) % m_sourceBuffer.size();
            size_t idx1 = (idx0 + 1) % m_sourceBuffer.size();
            float frac = p.samplePos - static_cast<float>(static_cast<size_t>(p.samplePos));
            float rawSample = m_sourceBuffer[idx0] * (1.0f - frac) + m_sourceBuffer[idx1] * frac;

            float grainSample = rawSample * env;

            // 3D Spatial Panning (Azimuth and Distance)
            float azimuth = std::atan2(p.x, p.y); // -PI to +PI
            float panL = 0.5f * (1.0f - std::sin(azimuth));
            float panR = 0.5f * (1.0f + std::sin(azimuth));
            float distAtten = 1.0f / (1.0f + r * 0.15f);

            sampleAccumL += grainSample * panL * distAtten;
            sampleAccumR += grainSample * panR * distAtten;

            // Advance grain
            p.samplePos += doppler;
            p.grainLife += p.grainLifeStep;

            if (p.grainLife >= 1.0f) {
                spawnParticle(p);
            }
        }

        outL[s] = sampleAccumL * 0.35f;
        if (outR) outR[s] = sampleAccumR * 0.35f;

        // Periodic physics update (every 16 samples)
        if ((s & 15) == 0) {
            activeCount = 0;
            for (auto& p : m_particles) {
                updateParticlePhysics(p);
                if (p.active) activeCount++;
            }
        }
    }

    m_activeParticles = activeCount;
}

} // namespace Jazz
