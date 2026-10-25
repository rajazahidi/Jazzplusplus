//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// StereoWidthSpectralShufflerEngine.cpp - Alan Blumlein Frequency-Dependent
// Stereo Shuffler & Polar Image Sculptor
//*****************************************************************************

#include "StereoWidthSpectralShufflerEngine.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Jazz {

JZStereoWidthSpectralShufflerEngine::JZStereoWidthSpectralShufflerEngine(float sampleRate)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
{
    initialize(m_sampleRate);
}

void JZStereoWidthSpectralShufflerEngine::initialize(float sampleRate)
{
    m_sampleRate = sampleRate > 1000.0f ? sampleRate : 48000.0f;
    reset();
}

void JZStereoWidthSpectralShufflerEngine::reset()
{
    m_phaseCorrelation = 0.85f;
    m_phaseWarning = false;
    m_sideToMidRatioDb = -6.0f;

    m_shuffX1 = m_shuffX2 = m_shuffY1 = m_shuffY2 = 0.0f;
    m_monoX1 = m_monoX2 = m_monoY1 = m_monoY2 = 0.0f;
    m_airX1 = m_airX2 = m_airY1 = m_airY2 = 0.0f;

    updateFilterCoeffs();
}

void JZStereoWidthSpectralShufflerEngine::setParameters(const Parameters& params)
{
    m_params = params;
    m_params.shufflerGainDb = std::max(0.0f, std::min(m_params.shufflerGainDb, 12.0f));
    m_params.shufflerCrossoverHz = std::max(150.0f, std::min(m_params.shufflerCrossoverHz, 2000.0f));
    m_params.subMonoCutoffHz = std::max(30.0f, std::min(m_params.subMonoCutoffHz, 400.0f));
    m_params.airWidth = std::max(0.2f, std::min(m_params.airWidth, 3.0f));
    updateFilterCoeffs();
}

void JZStereoWidthSpectralShufflerEngine::setShufflerGainDb(float db)
{
    m_params.shufflerGainDb = std::max(0.0f, std::min(db, 12.0f));
    updateFilterCoeffs();
}

void JZStereoWidthSpectralShufflerEngine::setShufflerCrossoverHz(float hz)
{
    m_params.shufflerCrossoverHz = std::max(150.0f, std::min(hz, 2000.0f));
    updateFilterCoeffs();
}

void JZStereoWidthSpectralShufflerEngine::setSubMonoCutoffHz(float hz)
{
    m_params.subMonoCutoffHz = std::max(30.0f, std::min(hz, 400.0f));
    updateFilterCoeffs();
}

void JZStereoWidthSpectralShufflerEngine::setAirWidth(float width)
{
    m_params.airWidth = std::max(0.2f, std::min(width, 3.0f));
    updateFilterCoeffs();
}

void JZStereoWidthSpectralShufflerEngine::setMidGainDb(float db)
{
    m_params.midGainDb = std::max(-12.0f, std::min(db, 12.0f));
}

void JZStereoWidthSpectralShufflerEngine::setSideGainDb(float db)
{
    m_params.sideGainDb = std::max(-12.0f, std::min(db, 12.0f));
}

void JZStereoWidthSpectralShufflerEngine::setPhaseGuardEnabled(bool enabled)
{
    m_params.phaseGuardEnabled = enabled;
}

void JZStereoWidthSpectralShufflerEngine::setBypass(bool bp)
{
    m_params.bypass = bp;
}

void JZStereoWidthSpectralShufflerEngine::updateFilterCoeffs()
{
    // 1. Shuffler Low-Shelf Filter on Side channel (Robert Bristow-Johnson Audio EQ Cookbook)
    float A = std::pow(10.0f, m_params.shufflerGainDb / 40.0f);
    float w0 = 2.0f * static_cast<float>(M_PI) * m_params.shufflerCrossoverHz / m_sampleRate;
    w0 = std::max(0.01f, std::min(w0, static_cast<float>(M_PI) - 0.05f));
    float cosw0 = std::cos(w0);
    float sinw0 = std::sin(w0);
    float alpha = sinw0 / (2.0f * 0.7071f);
    float twoSqrtAAlpha = 2.0f * std::sqrt(A) * alpha;

    float a0 = (A + 1.0f) + (A - 1.0f) * cosw0 + twoSqrtAAlpha;
    m_shuffB0 = (A * ((A + 1.0f) - (A - 1.0f) * cosw0 + twoSqrtAAlpha)) / a0;
    m_shuffB1 = (2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosw0)) / a0;
    m_shuffB2 = (A * ((A + 1.0f) - (A - 1.0f) * cosw0 - twoSqrtAAlpha)) / a0;
    m_shuffA1 = (-2.0f * ((A - 1.0f) + (A + 1.0f) * cosw0)) / a0;
    m_shuffA2 = ((A + 1.0f) + (A - 1.0f) * cosw0 - twoSqrtAAlpha) / a0;

    // 2. High-pass Butterworth filter on Side channel (Elliptical Mono-maker)
    float wMono = 2.0f * static_cast<float>(M_PI) * m_params.subMonoCutoffHz / m_sampleRate;
    wMono = std::max(0.005f, std::min(wMono, 0.5f));
    float cosMono = std::cos(wMono);
    float alphaMono = std::sin(wMono) / (2.0f * 0.7071f);
    float a0Mono = 1.0f + alphaMono;
    m_monoB0 = ((1.0f + cosMono) * 0.5f) / a0Mono;
    m_monoB1 = (-(1.0f + cosMono)) / a0Mono;
    m_monoB2 = ((1.0f + cosMono) * 0.5f) / a0Mono;
    m_monoA1 = (-2.0f * cosMono) / a0Mono;
    m_monoA2 = (1.0f - alphaMono) / a0Mono;

    // 3. High-shelf filter on Side channel for Air expansion (> 6 kHz)
    float airGainDb = 20.0f * std::log10(std::max(0.2f, m_params.airWidth));
    float AAir = std::pow(10.0f, airGainDb / 40.0f);
    float wAir = 2.0f * static_cast<float>(M_PI) * 6500.0f / m_sampleRate;
    wAir = std::max(0.1f, std::min(wAir, static_cast<float>(M_PI) - 0.1f));
    float cosAir = std::cos(wAir);
    float sinAir = std::sin(wAir);
    float alphaAir = sinAir / (2.0f * 0.7071f);
    float twoSqrtAAirAlpha = 2.0f * std::sqrt(AAir) * alphaAir;

    float a0Air = (AAir + 1.0f) - (AAir - 1.0f) * cosAir + twoSqrtAAirAlpha;
    m_airB0 = (AAir * ((AAir + 1.0f) + (AAir - 1.0f) * cosAir + twoSqrtAAirAlpha)) / a0Air;
    m_airB1 = (-2.0f * AAir * ((AAir - 1.0f) + (AAir + 1.0f) * cosAir)) / a0Air;
    m_airB2 = (AAir * ((AAir + 1.0f) + (AAir - 1.0f) * cosAir - twoSqrtAAirAlpha)) / a0Air;
    m_airA1 = (2.0f * ((AAir - 1.0f) - (AAir + 1.0f) * cosAir)) / a0Air;
    m_airA2 = ((AAir + 1.0f) - (AAir - 1.0f) * cosAir - twoSqrtAAirAlpha) / a0Air;
}

void JZStereoWidthSpectralShufflerEngine::process(const float* inL, const float* inR,
                                                  float* outL, float* outR,
                                                  size_t numSamples)
{
    if (!inL || !inR || !outL || !outR || numSamples == 0) return;

    if (m_params.bypass) {
        if (outL != inL) std::copy(inL, inL + numSamples, outL);
        if (outR != inR) std::copy(inR, inR + numSamples, outR);
        return;
    }

    float midGainLinear = std::pow(10.0f, m_params.midGainDb / 20.0f);
    float sideGainLinear = std::pow(10.0f, m_params.sideGainDb / 20.0f);

    float sumDot = 0.0f;
    float sumL2 = 0.0f;
    float sumR2 = 0.0f;
    float sumMid = 0.0f;
    float sumSide = 0.0f;

    for (size_t i = 0; i < numSamples; ++i) {
        float l = inL[i];
        float r = inR[i];

        // 1. Blumlein Mid/Side decomposition (equal-power normalization factor 0.7071)
        float mid = (l + r) * 0.70710678f * midGainLinear;
        float side = (l - r) * 0.70710678f * sideGainLinear;

        // 2. High-pass filter Side channel (Elliptical Sub-Bass Mono-maker)
        float sMono = m_monoB0 * side + m_monoB1 * m_monoX1 + m_monoB2 * m_monoX2 - m_monoA1 * m_monoY1 - m_monoA2 * m_monoY2;
        m_monoX2 = m_monoX1; m_monoX1 = side; m_monoY2 = m_monoY1; m_monoY1 = sMono;

        // 3. Low-shelf filter Side channel (Low-Frequency Blumlein Shuffler)
        float sShuff = m_shuffB0 * sMono + m_shuffB1 * m_shuffX1 + m_shuffB2 * m_shuffX2 - m_shuffA1 * m_shuffY1 - m_shuffA2 * m_shuffY2;
        m_shuffX2 = m_shuffX1; m_shuffX1 = sMono; m_shuffY2 = m_shuffY1; m_shuffY1 = sShuff;

        // 4. High-shelf filter Side channel (Air Width expansion)
        float sAir = m_airB0 * sShuff + m_airB1 * m_airX1 + m_airB2 * m_airX2 - m_airA1 * m_airY1 - m_airA2 * m_airY2;
        m_airX2 = m_airX1; m_airX1 = sShuff; m_airY2 = m_airY1; m_airY1 = sAir;

        // Phase protection guard: clamp side if it overpowers mid
        float processedSide = sAir;
        if (m_params.phaseGuardEnabled) {
            float maxAllowedSide = std::abs(mid) * 2.2f + 0.05f;
            if (std::abs(processedSide) > maxAllowedSide) {
                processedSide = std::copysign(maxAllowedSide, processedSide);
            }
        }

        // 5. Reconstruct Left and Right from Mid / Side
        float newL = (mid + processedSide) * 0.70710678f;
        float newR = (mid - processedSide) * 0.70710678f;

        outL[i] = newL;
        outR[i] = newR;

        // Correlation accumulator
        sumDot += newL * newR;
        sumL2 += newL * newL;
        sumR2 += newR * newR;
        sumMid += mid * mid;
        sumSide += processedSide * processedSide;
    }

    // Update real-time telemetry
    float denom = std::sqrt(sumL2 * sumR2) + 1e-9f;
    float currentCorr = sumDot / denom;
    m_phaseCorrelation = 0.9f * m_phaseCorrelation + 0.1f * currentCorr;
    m_phaseWarning = (m_phaseCorrelation < 0.1f);

    float midPower = sumMid + 1e-9f;
    float sidePower = sumSide + 1e-9f;
    m_sideToMidRatioDb = 10.0f * std::log10(sidePower / midPower);
}

} // namespace Jazz
