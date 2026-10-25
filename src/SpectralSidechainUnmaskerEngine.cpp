//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// SpectralSidechainUnmaskerEngine.cpp - Multi-Bus Dynamic Spectral Unmasker
//*****************************************************************************

#include "SpectralSidechainUnmaskerEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

JZSpectralSidechainUnmaskerEngine::JZSpectralSidechainUnmaskerEngine(float sampleRate) {
    initialize(sampleRate);
}

void JZSpectralSidechainUnmaskerEngine::initialize(float sampleRate) {
    if (sampleRate < 8000.0f) sampleRate = 48000.0f;
    m_sampleRate = sampleRate;

    static const float barkFreqs[NUM_BARK_BANDS] = {
        100.0f, 200.0f, 300.0f, 400.0f, 510.0f, 630.0f, 770.0f, 920.0f,
        1080.0f, 1270.0f, 1480.0f, 1720.0f, 2000.0f, 2320.0f, 2700.0f, 3150.0f,
        3700.0f, 4400.0f, 5300.0f, 6400.0f, 7700.0f, 9500.0f, 12000.0f, 15500.0f
    };

    for (size_t b = 0; b < NUM_BARK_BANDS; ++b) {
        m_bands[b].centerFreqHz = barkFreqs[b];
    }

    reset();
    updateFilterbank();
}

void JZSpectralSidechainUnmaskerEngine::reset() {
    for (size_t b = 0; b < NUM_BARK_BANDS; ++b) {
        BarkBand& band = m_bands[b];
        band.x1L = band.x2L = band.y1L = band.y2L = 0.0f;
        band.x1R = band.x2R = band.y1R = band.y2R = 0.0f;
        band.scX1L = band.scX2L = band.scY1L = band.scY2L = 0.0f;
        band.scX1R = band.scX2R = band.scY1R = band.scY2R = 0.0f;

        band.mainEnergyL = band.mainEnergyR = 0.0f;
        band.scEnergyL = band.scEnergyR = 0.0f;
        band.currentGainL = band.currentGainR = 1.0f;
    }

    m_maxReductionDb = 0.0f;
    m_activeMaskedBands = 0;
}

void JZSpectralSidechainUnmaskerEngine::setParameters(const Parameters& params) {
    m_params = params;
    m_params.unmaskingDepthDb = std::max(-24.0f, std::min(-3.0f, m_params.unmaskingDepthDb));
    m_params.sensitivity = std::max(0.1f, std::min(3.0f, m_params.sensitivity));
    m_params.attackMs = std::max(1.0f, std::min(100.0f, m_params.attackMs));
    m_params.releaseMs = std::max(10.0f, std::min(500.0f, m_params.releaseMs));
    m_params.dryWet = std::max(0.0f, std::min(1.0f, m_params.dryWet));
    updateFilterbank();
}

void JZSpectralSidechainUnmaskerEngine::setUnmaskingDepthDb(float db) {
    m_params.unmaskingDepthDb = std::max(-24.0f, std::min(-3.0f, db));
}

void JZSpectralSidechainUnmaskerEngine::setSensitivity(float sens) {
    m_params.sensitivity = std::max(0.1f, std::min(3.0f, sens));
}

void JZSpectralSidechainUnmaskerEngine::updateFilterbank() {
    m_attackCoeff = std::exp(-1.0f / ((m_params.attackMs / 1000.0f) * m_sampleRate));
    m_releaseCoeff = std::exp(-1.0f / ((m_params.releaseMs / 1000.0f) * m_sampleRate));

    for (size_t b = 0; b < NUM_BARK_BANDS; ++b) {
        float f0 = m_bands[b].centerFreqHz;
        if (f0 >= m_sampleRate * 0.45f) f0 = m_sampleRate * 0.45f;
        float Q = 3.2f;

        float w0 = 2.0f * static_cast<float>(M_PI) * (f0 / m_sampleRate);
        float sin_w = std::sin(w0);
        float cos_w = std::cos(w0);
        float alpha = sin_w / (2.0f * Q);

        float a0 = 1.0f + alpha;
        m_bands[b].b0 = alpha / a0;
        m_bands[b].b1 = 0.0f;
        m_bands[b].b2 = -alpha / a0;
        m_bands[b].a1 = (-2.0f * cos_w) / a0;
        m_bands[b].a2 = (1.0f - alpha) / a0;
    }
}

void JZSpectralSidechainUnmaskerEngine::process(
    const float* mainL, const float* mainR,
    const float* sidechainL, const float* sidechainR,
    float* outL, float* outR,
    size_t numSamples) {

    if (!mainL || !outL || numSamples == 0) return;
    const bool stereo = (mainR != nullptr && outR != nullptr);
    const bool hasSidechain = (sidechainL != nullptr);

    float minGainLinear = std::pow(10.0f, m_params.unmaskingDepthDb / 20.0f);
    float maxReductObserved = 1.0f;
    size_t maskedCount = 0;

    for (size_t i = 0; i < numSamples; ++i) {
        float sL = mainL[i];
        float sR = stereo ? mainR[i] : sL;
        float scL = hasSidechain ? sidechainL[i] : 0.0f;
        float scR = (hasSidechain && sidechainR) ? sidechainR[i] : scL;

        float unmaskedL = 0.0f;
        float unmaskedR = 0.0f;

        for (size_t b = 0; b < NUM_BARK_BANDS; ++b) {
            BarkBand& band = m_bands[b];

            // 1. Bandpass filter main signal
            float bMainL = band.b0 * sL + band.b1 * band.x1L + band.b2 * band.x2L
                           - band.a1 * band.y1L - band.a2 * band.y2L;
            band.x2L = band.x1L; band.x1L = sL;
            band.y2L = band.y1L; band.y1L = bMainL;

            band.mainEnergyL = band.mainEnergyL * 0.95f + std::abs(bMainL) * 0.05f;

            // 2. Bandpass filter sidechain signal
            float bScL = 0.0f;
            if (hasSidechain) {
                bScL = band.b0 * scL + band.b1 * band.scX1L + band.b2 * band.scX2L
                       - band.a1 * band.scY1L - band.a2 * band.scY2L;
                band.scX2L = band.scX1L; band.scX1L = scL;
                band.scY2L = band.scY1L; band.scY1L = bScL;
                band.scEnergyL = band.scEnergyL * 0.95f + std::abs(bScL) * 0.05f;
            }

            // 3. Compute dynamic unmasking gain
            float targetGainL = 1.0f;
            if (band.scEnergyL > 1e-4f) {
                float maskRatio = (band.scEnergyL * m_params.sensitivity) / (band.mainEnergyL + band.scEnergyL + 1e-5f);
                maskRatio = std::max(0.0f, std::min(1.0f, maskRatio));
                targetGainL = 1.0f - (1.0f - minGainLinear) * maskRatio;
                if (targetGainL < 0.9f) maskedCount++;
            }

            // Ballistics smoothing
            if (targetGainL < band.currentGainL) {
                band.currentGainL = m_attackCoeff * band.currentGainL + (1.0f - m_attackCoeff) * targetGainL;
            } else {
                band.currentGainL = m_releaseCoeff * band.currentGainL + (1.0f - m_releaseCoeff) * targetGainL;
            }

            if (band.currentGainL < maxReductObserved) maxReductObserved = band.currentGainL;

            unmaskedL += bMainL * band.currentGainL;

            // Right channel
            if (stereo) {
                float bMainR = band.b0 * sR + band.b1 * band.x1R + band.b2 * band.x2R
                               - band.a1 * band.y1R - band.a2 * band.y2R;
                band.x2R = band.x1R; band.x1R = sR;
                band.y2R = band.y1R; band.y1R = bMainR;
                band.mainEnergyR = band.mainEnergyR * 0.95f + std::abs(bMainR) * 0.05f;

                float targetGainR = 1.0f;
                if (hasSidechain) {
                    float bScR = band.b0 * scR + band.b1 * band.scX1R + band.b2 * band.scX2R
                                 - band.a1 * band.scY1R - band.a2 * band.scY2R;
                    band.scX2R = band.scX1R; band.scX1R = scR;
                    band.scY2R = band.scY1R; band.scY1R = bScR;
                    band.scEnergyR = band.scEnergyR * 0.95f + std::abs(bScR) * 0.05f;

                    if (band.scEnergyR > 1e-4f) {
                        float maskRatioR = (band.scEnergyR * m_params.sensitivity) / (band.mainEnergyR + band.scEnergyR + 1e-5f);
                        targetGainR = 1.0f - (1.0f - minGainLinear) * std::max(0.0f, std::min(1.0f, maskRatioR));
                    }
                }

                if (targetGainR < band.currentGainR) {
                    band.currentGainR = m_attackCoeff * band.currentGainR + (1.0f - m_attackCoeff) * targetGainR;
                } else {
                    band.currentGainR = m_releaseCoeff * band.currentGainR + (1.0f - m_releaseCoeff) * targetGainR;
                }

                unmaskedR += bMainR * band.currentGainR;
            }
        }

        // Output mix / solo delta
        if (m_params.soloDelta) {
            outL[i] = sL - unmaskedL;
            if (stereo) outR[i] = sR - unmaskedR;
        } else {
            outL[i] = sL * (1.0f - m_params.dryWet) + unmaskedL * m_params.dryWet;
            if (stereo) outR[i] = sR * (1.0f - m_params.dryWet) + unmaskedR * m_params.dryWet;
        }
    }

    m_maxReductionDb = (maxReductObserved > 1e-5f) ? (20.0f * std::log10(maxReductObserved)) : 0.0f;
    m_activeMaskedBands = maskedCount / std::max(size_t(1), numSamples);
}

} // namespace Jazz
