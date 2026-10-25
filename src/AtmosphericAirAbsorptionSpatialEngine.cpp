//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// AtmosphericAirAbsorptionSpatialEngine.cpp - ISO 9613-1 Atmospheric Air
// Molecular Absorption & Acoustic Distance Attenuator
//*****************************************************************************

#include "AtmosphericAirAbsorptionSpatialEngine.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Jazz {

JZAtmosphericAirAbsorptionSpatialEngine::JZAtmosphericAirAbsorptionSpatialEngine(float sampleRate)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
{
    initialize(m_sampleRate);
}

void JZAtmosphericAirAbsorptionSpatialEngine::initialize(float sampleRate)
{
    m_sampleRate = sampleRate > 1000.0f ? sampleRate : 48000.0f;
    m_delayBufferCapacity = static_cast<size_t>(m_sampleRate * 1.5f); // 1.5 seconds capacity
    m_delayBufferL.assign(m_delayBufferCapacity, 0.0f);
    m_delayBufferR.assign(m_delayBufferCapacity, 0.0f);
    m_delayWritePos = 0;
    reset();
}

void JZAtmosphericAirAbsorptionSpatialEngine::reset()
{
    std::fill(m_delayBufferL.begin(), m_delayBufferL.end(), 0.0f);
    std::fill(m_delayBufferR.begin(), m_delayBufferR.end(), 0.0f);
    m_delayWritePos = 0;

    m_fltX1L = m_fltX2L = m_fltY1L = m_fltY2L = 0.0f;
    m_fltX1R = m_fltX2R = m_fltY1R = m_fltY2R = 0.0f;
    m_proxX1L = m_proxY1L = 0.0f;
    m_proxX1R = m_proxY1R = 0.0f;

    updateAtmosphericAcoustics();
}

void JZAtmosphericAirAbsorptionSpatialEngine::setParameters(const Parameters& params)
{
    m_params = params;
    m_params.distanceMeters = std::max(0.1f, std::min(m_params.distanceMeters, 500.0f));
    m_params.temperatureCelsius = std::max(-20.0f, std::min(m_params.temperatureCelsius, 50.0f));
    m_params.relativeHumidityPercent = std::max(5.0f, std::min(m_params.relativeHumidityPercent, 100.0f));
    m_params.referenceDistanceMeters = std::max(0.1f, std::min(m_params.referenceDistanceMeters, 10.0f));
    updateAtmosphericAcoustics();
}

void JZAtmosphericAirAbsorptionSpatialEngine::setDistanceMeters(float meters)
{
    m_params.distanceMeters = std::max(0.1f, std::min(meters, 500.0f));
    updateAtmosphericAcoustics();
}

void JZAtmosphericAirAbsorptionSpatialEngine::setTemperatureCelsius(float tempC)
{
    m_params.temperatureCelsius = std::max(-20.0f, std::min(tempC, 50.0f));
    updateAtmosphericAcoustics();
}

void JZAtmosphericAirAbsorptionSpatialEngine::setRelativeHumidityPercent(float rhPercent)
{
    m_params.relativeHumidityPercent = std::max(5.0f, std::min(rhPercent, 100.0f));
    updateAtmosphericAcoustics();
}

void JZAtmosphericAirAbsorptionSpatialEngine::setAirAbsorptionEnabled(bool enabled)
{
    m_params.airAbsorptionEnabled = enabled;
    updateAtmosphericAcoustics();
}

void JZAtmosphericAirAbsorptionSpatialEngine::setDistanceAttenuationEnabled(bool enabled)
{
    m_params.distanceAttenuationEnabled = enabled;
}

void JZAtmosphericAirAbsorptionSpatialEngine::setBypass(bool bp)
{
    m_params.bypass = bp;
}

void JZAtmosphericAirAbsorptionSpatialEngine::updateAtmosphericAcoustics()
{
    // Speed of sound in dry air: c = 331.3 * sqrt(1 + T / 273.15)
    float tKelvin = m_params.temperatureCelsius + 273.15f;
    m_speedOfSound = 331.3f * std::sqrt(tKelvin / 273.15f);

    // Propagation delay: t = distance / speed
    m_propagationDelayMs = (m_params.distanceMeters / m_speedOfSound) * 1000.0f;

    // Geometric inverse-square attenuation: lossDb = 20 * log10(d_ref / d)
    if (m_params.distanceAttenuationEnabled) {
        float distRatio = m_params.referenceDistanceMeters / std::max(0.2f, m_params.distanceMeters);
        m_geometricLossDb = 20.0f * std::log10(distRatio);
        m_geometricLossDb = std::min(6.0f, m_geometricLossDb); // clamp max gain at very close proximity
    } else {
        m_geometricLossDb = 0.0f;
    }

    // ISO 9613-1 atmospheric absorption calculation
    // Approximated absorption coefficient alpha at 10kHz (dB per meter):
    // Standard atmosphere (20C, 50% RH) has alpha ~ 0.16 dB/m at 10kHz.
    float tRatio = tKelvin / 293.15f;
    float rh = m_params.relativeHumidityPercent;
    float alpha10k = 0.16f * (1.0f + (20.0f - m_params.temperatureCelsius) * 0.015f) * (60.0f / (rh + 10.0f));
    alpha10k = std::max(0.04f, std::min(alpha10k, 1.2f));

    m_absorptionAt10kHzDb = -(alpha10k * m_params.distanceMeters);

    // Dynamic low-pass damping corner frequency:
    // As distance increases, the effective corner drops from 20kHz down to ~800Hz
    float cutoffHz = 20000.0f / (1.0f + 0.06f * m_params.distanceMeters * (alpha10k / 0.16f));
    cutoffHz = std::max(500.0f, std::min(cutoffHz, 20000.0f));

    if (!m_params.airAbsorptionEnabled) {
        cutoffHz = 20000.0f;
        m_absorptionAt10kHzDb = 0.0f;
    }

    // 2nd-order Butterworth lowpass filter coefficients
    float omega = 2.0f * static_cast<float>(M_PI) * cutoffHz / m_sampleRate;
    omega = std::max(0.01f, std::min(omega, static_cast<float>(M_PI) - 0.02f));
    float cosw = std::cos(omega);
    float alpha = std::sin(omega) / (2.0f * 0.7071f);
    float a0 = 1.0f + alpha;

    m_fltB0 = ((1.0f - cosw) * 0.5f) / a0;
    m_fltB1 = (1.0f - cosw) / a0;
    m_fltB2 = ((1.0f - cosw) * 0.5f) / a0;
    m_fltA1 = (-2.0f * cosw) / a0;
    m_fltA2 = (1.0f - alpha) / a0;

    // Proximity effect low-frequency boost when closer than reference distance
    if (m_params.proximityBassBoostEnabled && m_params.distanceMeters < m_params.referenceDistanceMeters) {
        float proxFactor = (m_params.referenceDistanceMeters - m_params.distanceMeters) / m_params.referenceDistanceMeters;
        float proxGain = 1.0f + proxFactor * 1.5f; // up to +8 dB bass boost
        m_proxB0 = proxGain;
        m_proxB1 = 0.0f;
        m_proxA1 = 0.0f;
    } else {
        m_proxB0 = 1.0f;
        m_proxB1 = 0.0f;
        m_proxA1 = 0.0f;
    }
}

void JZAtmosphericAirAbsorptionSpatialEngine::process(const float* inL, const float* inR,
                                                      float* outL, float* outR,
                                                      size_t numSamples)
{
    if (!inL || !inR || !outL || !outR || numSamples == 0) return;

    if (m_params.bypass) {
        if (outL != inL) std::copy(inL, inL + numSamples, outL);
        if (outR != inR) std::copy(inR, inR + numSamples, outR);
        return;
    }

    float linearGain = std::pow(10.0f, m_geometricLossDb / 20.0f);
    size_t delaySamples = static_cast<size_t>((m_propagationDelayMs * 0.001f) * m_sampleRate);
    if (delaySamples >= m_delayBufferCapacity) {
        delaySamples = m_delayBufferCapacity - 1;
    }

    for (size_t i = 0; i < numSamples; ++i) {
        // Write to acoustic propagation delay buffer
        m_delayBufferL[m_delayWritePos] = inL[i];
        m_delayBufferR[m_delayWritePos] = inR[i];

        // Read delayed sample
        size_t readPos = (m_delayWritePos + m_delayBufferCapacity - delaySamples) % m_delayBufferCapacity;
        float delayedL = m_delayBufferL[readPos];
        float delayedR = m_delayBufferR[readPos];

        m_delayWritePos = (m_delayWritePos + 1) % m_delayBufferCapacity;

        // Apply atmospheric lowpass filter (molecular air absorption)
        float fltL = m_fltB0 * delayedL + m_fltB1 * m_fltX1L + m_fltB2 * m_fltX2L - m_fltA1 * m_fltY1L - m_fltA2 * m_fltY2L;
        m_fltX2L = m_fltX1L; m_fltX1L = delayedL; m_fltY2L = m_fltY1L; m_fltY1L = fltL;

        float fltR = m_fltB0 * delayedR + m_fltB1 * m_fltX1R + m_fltB2 * m_fltX2R - m_fltA1 * m_fltY1R - m_fltA2 * m_fltY2R;
        m_fltX2R = m_fltX1R; m_fltX1R = delayedR; m_fltY2R = m_fltY1R; m_fltY1R = fltR;

        // Apply geometric inverse-square gain and proximity boost
        float outSampleL = fltL * linearGain * m_proxB0;
        float outSampleR = fltR * linearGain * m_proxB0;

        outL[i] = outSampleL;
        outR[i] = outSampleR;
    }
}

} // namespace Jazz
