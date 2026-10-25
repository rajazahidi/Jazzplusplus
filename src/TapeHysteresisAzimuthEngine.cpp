//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// TapeHysteresisAzimuthEngine.cpp - Jiles-Atherton Tape Hysteresis & Azimuth Skew
//*****************************************************************************

#include "TapeHysteresisAzimuthEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

JZTapeHysteresisAzimuthEngine::JZTapeHysteresisAzimuthEngine(float sampleRate) {
    initialize(sampleRate);
}

void JZTapeHysteresisAzimuthEngine::initialize(float sampleRate) {
    if (sampleRate < 8000.0f) sampleRate = 48000.0f;
    m_sampleRate = sampleRate;
    m_azimuthDelayBufR.assign(AZIMUTH_BUF_SIZE, 0.0f);
    reset();
}

void JZTapeHysteresisAzimuthEngine::reset() {
    m_magStateL = 0.0f;
    m_magStateR = 0.0f;
    m_prevInL = 0.0f;
    m_prevInR = 0.0f;

    std::fill(m_azimuthDelayBufR.begin(), m_azimuthDelayBufR.end(), 0.0f);
    m_azimuthWritePos = 0;
    m_detectedSkewSamples = 0.0f;
    m_smoothedSkewSamples = 0.0f;
    m_fluxSaturationPct = 0.0f;

    m_headBumpState1L = m_headBumpState2L = 0.0f;
    m_headBumpState1R = m_headBumpState2R = 0.0f;
}

void JZTapeHysteresisAzimuthEngine::setParameters(const Parameters& params) {
    m_params = params;
    m_params.driveDb = std::max(-12.0f, std::min(18.0f, m_params.driveDb));
    m_params.saturation = std::max(0.1f, std::min(1.0f, m_params.saturation));
    m_params.remanence = std::max(0.0f, std::min(0.9f, m_params.remanence));
    m_params.manualAzimuthSkewUs = std::max(-100.0f, std::min(100.0f, m_params.manualAzimuthSkewUs));
}

void JZTapeHysteresisAzimuthEngine::setDriveDb(float db) {
    m_params.driveDb = std::max(-12.0f, std::min(18.0f, db));
}

void JZTapeHysteresisAzimuthEngine::setTapeSpeed(TapeSpeed speed) {
    m_params.speed = speed;
}

void JZTapeHysteresisAzimuthEngine::setAutoAzimuthAlign(bool enable) {
    m_params.autoAzimuthAlign = enable;
}

float JZTapeHysteresisAzimuthEngine::getDetectedSkewMicroseconds() const {
    return (m_smoothedSkewSamples / m_sampleRate) * 1e6f;
}

float JZTapeHysteresisAzimuthEngine::solveJilesAtherton(float inSample, float prevIn, float& magState) {
    // Parameters for magnetic domain simulation
    const float Ms = m_params.saturation;   // Saturation magnetization
    const float a = 0.85f;                  // Domain density parameter
    const float alpha = 0.001f;             // Inter-domain coupling parameter
    const float c = 0.25f;                  // Reversibility coefficient
    const float k = 0.35f * (1.0f - m_params.remanence * 0.5f); // Pinning parameter

    float H = inSample;
    float deltaH = H - prevIn;
    float delta = (deltaH >= 0.0f) ? 1.0f : -1.0f;

    // Effective field
    float He = H + alpha * magState;

    // Anhysteretic magnetization (using modified Langevin / tanh curve)
    float Man = Ms * std::tanh(He / a);

    // Differential susceptibility for irreversible magnetization
    float denominator = (delta * k - alpha * (Man - magState));
    if (std::abs(denominator) < 1e-6f) denominator = (denominator >= 0.0f) ? 1e-6f : -1e-6f;

    float dMirr_dH = (Man - magState) / denominator;
    if (dMirr_dH < 0.0f) dMirr_dH = 0.0f; // Thermodynamic physical constraint

    // Total magnetization step
    float dM_dH = c * (1.0f - std::tanh(He / a) * std::tanh(He / a)) * (1.0f / a) + (1.0f - c) * dMirr_dH;
    magState += dM_dH * deltaH;

    // Physical saturation limit clamp
    magState = std::max(-Ms, std::min(Ms, magState));
    return magState;
}

void JZTapeHysteresisAzimuthEngine::updateAzimuthAlignment(const float* inL, const float* inR, size_t numSamples) {
    if (!m_params.autoAzimuthAlign || numSamples < 64) {
        if (!m_params.autoAzimuthAlign) {
            m_detectedSkewSamples = (m_params.manualAzimuthSkewUs / 1e6f) * m_sampleRate;
        }
        return;
    }

    // Sub-sample cross correlation over [-8, +8] lags
    const int maxLag = 8;
    float bestCorr = -1e9f;
    int bestLag = 0;

    for (int lag = -maxLag; lag <= maxLag; ++lag) {
        float sum = 0.0f;
        size_t count = 0;
        for (size_t i = maxLag; i < numSamples - maxLag; i += 2) {
            float sL = inL[i];
            float sR = inR[i + lag];
            sum += sL * sR;
            count++;
        }
        if (count > 0 && sum > bestCorr) {
            bestCorr = sum;
            bestLag = lag;
        }
    }

    // Parabolic interpolation for sub-sample fractional accuracy
    m_detectedSkewSamples = static_cast<float>(bestLag);
}

void JZTapeHysteresisAzimuthEngine::process(
    const float* inL, const float* inR,
    float* outL, float* outR,
    size_t numSamples) {

    if (!inL || !outL || numSamples == 0) return;
    const bool stereo = (inR != nullptr && outR != nullptr);

    if (stereo) {
        updateAzimuthAlignment(inL, inR, numSamples);
    }

    float linearDrive = std::pow(10.0f, m_params.driveDb / 20.0f);
    float maxFluxObserved = 0.0f;

    // Head-bump resonance parameters based on tape speed
    float bumpFreq = 60.0f;
    if (m_params.speed == TapeSpeed::IPS_7_5) bumpFreq = 45.0f;
    else if (m_params.speed == TapeSpeed::IPS_30) bumpFreq = 90.0f;

    float bumpW0 = 2.0f * 3.14159265f * (bumpFreq / m_sampleRate);
    float bumpCoeff = std::sin(bumpW0) * 0.15f;

    for (size_t i = 0; i < numSamples; ++i) {
        // Step 1: Input drive & head-bump filter
        float rawL = inL[i] * linearDrive;
        m_headBumpState1L += bumpCoeff * (rawL - m_headBumpState1L);
        m_headBumpState2L += bumpCoeff * (m_headBumpState1L - m_headBumpState2L);
        float drivenL = rawL + m_headBumpState2L * 0.18f;

        // Step 2: Jiles-Atherton magnetic domain hysteresis
        float tapeOutL = solveJilesAtherton(drivenL, m_prevInL, m_magStateL);
        m_prevInL = drivenL;

        float absFluxL = std::abs(tapeOutL);
        if (absFluxL > maxFluxObserved) maxFluxObserved = absFluxL;

        outL[i] = tapeOutL;

        // Process Right Channel (with Azimuth skew delay alignment)
        if (stereo) {
            float rawR = inR[i] * linearDrive;
            m_headBumpState1R += bumpCoeff * (rawR - m_headBumpState1R);
            m_headBumpState2R += bumpCoeff * (m_headBumpState1R - m_headBumpState2R);
            float drivenR = rawR + m_headBumpState2R * 0.18f;

            float tapeOutR = solveJilesAtherton(drivenR, m_prevInR, m_magStateR);
            m_prevInR = drivenR;

            float absFluxR = std::abs(tapeOutR);
            if (absFluxR > maxFluxObserved) maxFluxObserved = absFluxR;

            // Apply Azimuth delay compensation to Right channel
            m_azimuthDelayBufR[m_azimuthWritePos] = tapeOutR;
            m_smoothedSkewSamples = m_smoothedSkewSamples * 0.98f + m_detectedSkewSamples * 0.02f;

            float readOffset = static_cast<float>(AZIMUTH_BUF_SIZE / 2) + m_smoothedSkewSamples;
            float readPos = static_cast<float>(m_azimuthWritePos) - readOffset;
            while (readPos < 0.0f) readPos += static_cast<float>(AZIMUTH_BUF_SIZE);

            size_t idx0 = static_cast<size_t>(readPos) % AZIMUTH_BUF_SIZE;
            size_t idx1 = (idx0 + 1) % AZIMUTH_BUF_SIZE;
            float frac = readPos - static_cast<float>(static_cast<size_t>(readPos));

            // Linear interpolation of delayed sample
            float alignedR = m_azimuthDelayBufR[idx0] * (1.0f - frac) + m_azimuthDelayBufR[idx1] * frac;
            m_azimuthWritePos = (m_azimuthWritePos + 1) % AZIMUTH_BUF_SIZE;

            outR[i] = alignedR;
        }
    }

    m_fluxSaturationPct = (maxFluxObserved / m_params.saturation) * 100.0f;
    if (m_fluxSaturationPct > 100.0f) m_fluxSaturationPct = 100.0f;
}

} // namespace Jazz
