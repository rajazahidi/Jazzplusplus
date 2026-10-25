//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZSpatialAmbisonicsUpMixerEngine.cpp - Stereo-to-7.1.4 / 3D Ambisonics Up-Mixer
//*****************************************************************************

#include "SpatialAmbisonicsUpMixerEngine.h"
#include <algorithm>
#include <cmath>

namespace Jazz {

JZSpatialAmbisonicsUpMixerEngine::JZSpatialAmbisonicsUpMixerEngine(float sampleRate)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
    , m_centerExtraction(0.65f)
    , m_surroundSpill(0.50f)
    , m_heightCeilingEnergy(0.40f)
    , m_lfeCrossoverHz(80.0f)
    , m_lfeFilterState(0.0f)
    , m_sideDelayIdx(0)
    , m_heightDelayIdx(0)
{
    // Surround delay: 15ms (~720 samples at 48kHz)
    size_t sDelay = static_cast<size_t>(0.015 * m_sampleRate) + 64;
    m_delaySideL.assign(sDelay, 0.0f);
    m_delaySideR.assign(sDelay, 0.0f);

    // Height ceiling delay: 25ms (~1200 samples at 48kHz)
    size_t hDelay = static_cast<size_t>(0.025 * m_sampleRate) + 64;
    m_delayHeightL.assign(hDelay, 0.0f);
    m_delayHeightR.assign(hDelay, 0.0f);
}

void JZSpatialAmbisonicsUpMixerEngine::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        m_sampleRate = sampleRate;
        reset();
    }
}

void JZSpatialAmbisonicsUpMixerEngine::setCenterExtraction(float amount) {
    m_centerExtraction = std::clamp(amount, 0.0f, 1.0f);
}

void JZSpatialAmbisonicsUpMixerEngine::setSurroundSpill(float amount) {
    m_surroundSpill = std::clamp(amount, 0.0f, 1.0f);
}

void JZSpatialAmbisonicsUpMixerEngine::setHeightCeilingEnergy(float amount) {
    m_heightCeilingEnergy = std::clamp(amount, 0.0f, 1.0f);
}

void JZSpatialAmbisonicsUpMixerEngine::setLfeCrossoverHz(float freqHz) {
    m_lfeCrossoverHz = std::clamp(freqHz, 40.0f, 160.0f);
}

void JZSpatialAmbisonicsUpMixerEngine::reset() {
    m_lfeFilterState = 0.0f;
    std::fill(m_delaySideL.begin(), m_delaySideL.end(), 0.0f);
    std::fill(m_delaySideR.begin(), m_delaySideR.end(), 0.0f);
    std::fill(m_delayHeightL.begin(), m_delayHeightL.end(), 0.0f);
    std::fill(m_delayHeightR.begin(), m_delayHeightR.end(), 0.0f);
    m_sideDelayIdx = 0;
    m_heightDelayIdx = 0;
}

bool JZSpatialAmbisonicsUpMixerEngine::processUpMix714(const float* inL, const float* inR,
                                                     size_t numSamples, Surround714Channels& out714) {
    if (!inL || !inR || numSamples == 0) return false;
    out714.resize(numSamples);

    // LFE low-pass filter coefficient
    float dt = 1.0f / m_sampleRate;
    float rc = 1.0f / (2.0f * 3.14159265f * m_lfeCrossoverHz);
    float alphaLfe = dt / (rc + dt);

    size_t sBufSize = m_delaySideL.size();
    size_t hBufSize = m_delayHeightL.size();

    size_t sideDelaySamples = static_cast<size_t>(0.012 * m_sampleRate);   // 12ms side surround
    size_t backDelaySamples = static_cast<size_t>(0.020 * m_sampleRate);   // 20ms back surround
    size_t heightDelaySamples = static_cast<size_t>(0.024 * m_sampleRate); // 24ms top ceiling

    for (size_t i = 0; i < numSamples; ++i) {
        float xL = inL[i];
        float xR = inR[i];

        // Mid/Side Direct-Diffuse separation
        float mid = (xL + xR) * 0.5f;   // Correlated direct sound (Phantom Center)
        float side = (xL - xR) * 0.5f;  // Decorrelated ambient sound (Diffuse field)

        // Center extraction with energy preservation
        float centerExtract = mid * m_centerExtraction;
        float frontL = xL - (centerExtract * 0.5f);
        float frontR = xR - (centerExtract * 0.5f);

        // LFE Subwoofer extraction (low-pass filtered mono)
        m_lfeFilterState += alphaLfe * (mid - m_lfeFilterState);
        float lfeAudio = m_lfeFilterState * 1.2f;

        // Store into decorrelation ring buffers
        m_delaySideL[m_sideDelayIdx] = side;
        m_delaySideR[m_sideDelayIdx] = -side;
        m_delayHeightL[m_heightDelayIdx] = side * 0.7f + mid * 0.3f;
        m_delayHeightR[m_heightDelayIdx] = -side * 0.7f + mid * 0.3f;

        // Read delayed surround channels
        size_t rSide = (m_sideDelayIdx + sBufSize - sideDelaySamples) % sBufSize;
        size_t rBack = (m_sideDelayIdx + sBufSize - backDelaySamples) % sBufSize;
        size_t rHeight = (m_heightDelayIdx + hBufSize - heightDelaySamples) % hBufSize;

        float sideL = (side + m_delaySideL[rSide] * 0.5f) * m_surroundSpill;
        float sideR = (-side + m_delaySideR[rSide] * 0.5f) * m_surroundSpill;

        float backL = m_delaySideL[rBack] * m_surroundSpill * 0.8f;
        float backR = m_delaySideR[rBack] * m_surroundSpill * 0.8f;

        // Top 4 height ceiling channels
        float topFL = m_delayHeightL[rHeight] * m_heightCeilingEnergy;
        float topFR = m_delayHeightR[rHeight] * m_heightCeilingEnergy;
        float topRL = m_delayHeightL[rBack] * m_heightCeilingEnergy * 0.9f;
        float topRR = m_delayHeightR[rBack] * m_heightCeilingEnergy * 0.9f;

        // Populate 7.1.4 outputs
        out714.L[i] = frontL;
        out714.R[i] = frontR;
        out714.C[i] = centerExtract;
        out714.LFE[i] = lfeAudio;
        out714.Ls[i] = sideL;
        out714.Rs[i] = sideR;
        out714.Lb[i] = backL;
        out714.Rb[i] = backR;
        out714.Tfl[i] = topFL;
        out714.Tfr[i] = topFR;
        out714.Trl[i] = topRL;
        out714.Trr[i] = topRR;

        m_sideDelayIdx = (m_sideDelayIdx + 1) % sBufSize;
        m_heightDelayIdx = (m_heightDelayIdx + 1) % hBufSize;
    }

    return true;
}

bool JZSpatialAmbisonicsUpMixerEngine::processUpMixBFormat(const float* inL, const float* inR,
                                                         size_t numSamples, AmbisonicsBFormat& outBFormat) {
    if (!inL || !inR || numSamples == 0) return false;
    outBFormat.resize(numSamples);

    for (size_t i = 0; i < numSamples; ++i) {
        float xL = inL[i];
        float xR = inR[i];

        float mid = (xL + xR) * 0.5f;
        float side = (xL - xR) * 0.5f;

        // B-format 1st-order spherical harmonics
        outBFormat.W[i] = (xL + xR) * 0.70710678f; // Omnidirectional W = (L+R)/sqrt(2)
        outBFormat.X[i] = mid * 1.0f;               // Front-Back dipole X
        outBFormat.Y[i] = side * 1.0f;              // Left-Right dipole Y
        outBFormat.Z[i] = (side * 0.5f) * m_heightCeilingEnergy; // Vertical height Z
    }
    return true;
}

} // namespace Jazz
