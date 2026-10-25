//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// BinauralDopplerMotionSpatializerEngine.cpp - Binaural Relativistic Acoustic
// Doppler & Moving Sound Source Spatializer
//*****************************************************************************

#include "BinauralDopplerMotionSpatializerEngine.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Jazz {

JZBinauralDopplerMotionSpatializerEngine::JZBinauralDopplerMotionSpatializerEngine(float sampleRate)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
{
    initialize(m_sampleRate);
}

void JZBinauralDopplerMotionSpatializerEngine::initialize(float sampleRate)
{
    m_sampleRate = sampleRate > 1000.0f ? sampleRate : 48000.0f;
    m_ringCapacity = static_cast<size_t>(m_sampleRate * 2.0f);
    m_delayRingL.assign(m_ringCapacity, 0.0f);
    m_delayRingR.assign(m_ringCapacity, 0.0f);
    reset();
}

void JZBinauralDopplerMotionSpatializerEngine::reset()
{
    std::fill(m_delayRingL.begin(), m_delayRingL.end(), 0.0f);
    std::fill(m_delayRingR.begin(), m_delayRingR.end(), 0.0f);
    m_ringWritePos = 0;
    m_readPosL = 0.0;
    m_readPosR = 0.0;
    m_dopplerRatioL = 1.0f;
    m_dopplerRatioR = 1.0f;
    m_distanceMeters = 5.0f;
    m_radialVelocityMps = 0.0f;
    updateKinematics();
}

void JZBinauralDopplerMotionSpatializerEngine::setParameters(const Parameters& params)
{
    m_params = params;
    m_params.speedOfSound = std::max(200.0f, std::min(m_params.speedOfSound, 500.0f));
    m_params.headRadiusMeters = std::max(0.05f, std::min(m_params.headRadiusMeters, 0.15f));
    updateKinematics();
}

void JZBinauralDopplerMotionSpatializerEngine::setPosition(float x, float y, float z)
{
    m_params.posX = x;
    m_params.posY = y;
    m_params.posZ = z;
    updateKinematics();
}

void JZBinauralDopplerMotionSpatializerEngine::setVelocity(float vx, float vy, float vz)
{
    m_params.velX = vx;
    m_params.velY = vy;
    m_params.velZ = vz;
    updateKinematics();
}

void JZBinauralDopplerMotionSpatializerEngine::setSpeedOfSound(float c)
{
    m_params.speedOfSound = std::max(200.0f, std::min(c, 500.0f));
    updateKinematics();
}

void JZBinauralDopplerMotionSpatializerEngine::setDopplerEnabled(bool enabled)
{
    m_params.dopplerEnabled = enabled;
}

void JZBinauralDopplerMotionSpatializerEngine::setBypass(bool bp)
{
    m_params.bypass = bp;
}

void JZBinauralDopplerMotionSpatializerEngine::updateKinematics()
{
    float c = m_params.speedOfSound;
    float a = m_params.headRadiusMeters;

    // Ear positions relative to head origin (0, 0, 0):
    // Left ear: (-a, 0, 0), Right ear: (+a, 0, 0)
    float dxL = m_params.posX - (-a);
    float dyL = m_params.posY;
    float dzL = m_params.posZ;
    float distL = std::sqrt(dxL * dxL + dyL * dyL + dzL * dzL) + 1e-4f;

    float dxR = m_params.posX - (+a);
    float dyR = m_params.posY;
    float dzR = m_params.posZ;
    float distR = std::sqrt(dxR * dxR + dyR * dyR + dzR * dzR) + 1e-4f;

    m_distanceMeters = (distL + distR) * 0.5f;

    // Unit vectors pointing from source towards ears
    float uxL = -dxL / distL;
    float uyL = -dyL / distL;
    float uzL = -dzL / distL;

    float uxR = -dxR / distR;
    float uyR = -dyR / distR;
    float uzR = -dzR / distR;

    // Relative radial velocity towards ears: v_r = \vec{v} \cdot \hat{u}
    float vrL = m_params.velX * uxL + m_params.velY * uyL + m_params.velZ * uzL;
    float vrR = m_params.velX * uxR + m_params.velY * uyR + m_params.velZ * uzR;

    m_radialVelocityMps = (vrL + vrR) * 0.5f;

    // Acoustic Doppler shift formula: f' / f = c / (c - vr)
    // Positive vr means source is moving towards ear (pitch shifts up)
    if (m_params.dopplerEnabled) {
        float denomL = std::max(20.0f, c - vrL);
        float denomR = std::max(20.0f, c - vrR);
        m_dopplerRatioL = std::max(0.25f, std::min(c / denomL, 4.0f));
        m_dopplerRatioR = std::max(0.25f, std::min(c / denomR, 4.0f));
    } else {
        m_dopplerRatioL = 1.0f;
        m_dopplerRatioR = 1.0f;
    }
}

void JZBinauralDopplerMotionSpatializerEngine::process(const float* inL, const float* inR,
                                                      float* outL, float* outR,
                                                      size_t numSamples)
{
    if (!inL || !inR || !outL || !outR || numSamples == 0) return;

    if (m_params.bypass) {
        if (outL != inL) std::copy(inL, inL + numSamples, outL);
        if (outR != inR) std::copy(inR, inR + numSamples, outR);
        return;
    }

    float c = m_params.speedOfSound;
    float a = m_params.headRadiusMeters;

    // Calculate interaural level and distance gain
    float distL = std::sqrt(std::pow(m_params.posX + a, 2.0f) + std::pow(m_params.posY, 2.0f) + std::pow(m_params.posZ, 2.0f)) + 1e-4f;
    float distR = std::sqrt(std::pow(m_params.posX - a, 2.0f) + std::pow(m_params.posY, 2.0f) + std::pow(m_params.posZ, 2.0f)) + 1e-4f;

    float gainL = m_params.distanceAttenuation ? std::min(2.0f, 1.0f / std::max(0.5f, distL)) : 1.0f;
    float gainR = m_params.distanceAttenuation ? std::min(2.0f, 1.0f / std::max(0.5f, distR)) : 1.0f;

    // Woodworth head-shadow attenuation
    float azimuth = std::atan2(m_params.posX, std::max(0.1f, m_params.posY));
    gainL *= (1.0f - 0.25f * std::sin(azimuth));
    gainR *= (1.0f + 0.25f * std::sin(azimuth));

    double stepL = m_dopplerRatioL;
    double stepR = m_dopplerRatioR;

    for (size_t i = 0; i < numSamples; ++i) {
        // Write incoming mono/stereo stream into circular delay buffer
        m_delayRingL[m_ringWritePos] = inL[i];
        m_delayRingR[m_ringWritePos] = inR[i];
        m_ringWritePos = (m_ringWritePos + 1) % m_ringCapacity;

        // Advance fractional read position with Doppler velocity step
        m_readPosL += stepL;
        m_readPosR += stepR;

        // Wrap read positions
        while (m_readPosL >= static_cast<double>(m_ringCapacity)) m_readPosL -= m_ringCapacity;
        while (m_readPosR >= static_cast<double>(m_ringCapacity)) m_readPosR -= m_ringCapacity;

        // Hermite 4-point cubic interpolation for ultra-clean pitch shifting
        auto sampleRing = [this](const std::vector<float>& ring, double pos) -> float {
            size_t idx1 = static_cast<size_t>(pos) % m_ringCapacity;
            size_t idx0 = (idx1 > 0) ? (idx1 - 1) : (m_ringCapacity - 1);
            size_t idx2 = (idx1 + 1) % m_ringCapacity;
            size_t idx3 = (idx1 + 2) % m_ringCapacity;

            float frac = static_cast<float>(pos - static_cast<double>(idx1));
            float y0 = ring[idx0], y1 = ring[idx1], y2 = ring[idx2], y3 = ring[idx3];

            float c0 = y1;
            float c1 = 0.5f * (y2 - y0);
            float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
            float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

            return ((c3 * frac + c2) * frac + c1) * frac + c0;
        };

        float sL = sampleRing(m_delayRingL, m_readPosL);
        float sR = sampleRing(m_delayRingR, m_readPosR);

        outL[i] = sL * gainL;
        outR[i] = sR * gainR;
    }
}

} // namespace Jazz
