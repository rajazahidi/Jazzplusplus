//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// SpeechDenoiseSpectralSubtractorEngine.cpp - Real-Time Speech Spectral De-Noiser
//*****************************************************************************

#include "SpeechDenoiseSpectralSubtractorEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

JZSpeechDenoiseSpectralSubtractorEngine::JZSpeechDenoiseSpectralSubtractorEngine(float sampleRate) {
    initialize(sampleRate);
}

void JZSpeechDenoiseSpectralSubtractorEngine::initialize(float sampleRate) {
    if (sampleRate < 8000.0f) sampleRate = 48000.0f;
    m_sampleRate = sampleRate;

    // 16 Center frequencies from 120 Hz to 13.5 kHz
    static const float centerFreqs[NUM_SPECTRAL_BANDS] = {
        120.0f, 220.0f, 350.0f, 500.0f, 750.0f, 1100.0f, 1600.0f, 2200.0f,
        3000.0f, 4000.0f, 5200.0f, 6800.0f, 8500.0f, 10500.0f, 12500.0f, 14500.0f
    };

    for (size_t b = 0; b < NUM_SPECTRAL_BANDS; ++b) {
        m_bands[b].centerFreqHz = centerFreqs[b];
        m_bands[b].noiseFloorPower = 1e-4f;
    }

    reset();
    updateFilterbank();
}

void JZSpeechDenoiseSpectralSubtractorEngine::reset() {
    for (size_t b = 0; b < NUM_SPECTRAL_BANDS; ++b) {
        SpectralBand& band = m_bands[b];
        band.x1L = band.x2L = band.y1L = band.y2L = 0.0f;
        band.x1R = band.x2R = band.y1R = band.y2R = 0.0f;
        band.currentGainL = 1.0f;
        band.currentGainR = 1.0f;
    }
    m_avgNoiseFloorDb = -60.0f;
    m_currentReductionDb = 0.0f;
}

void JZSpeechDenoiseSpectralSubtractorEngine::setParameters(const Parameters& params) {
    m_params = params;
    m_params.reductionDb = std::max(-36.0f, std::min(-2.0f, m_params.reductionDb));
    m_params.oversubtractionFactor = std::max(1.0f, std::min(3.0f, m_params.oversubtractionFactor));
    m_params.smoothingMs = std::max(2.0f, std::min(200.0f, m_params.smoothingMs));
    m_params.dryWet = std::max(0.0f, std::min(1.0f, m_params.dryWet));
    updateFilterbank();
}

void JZSpeechDenoiseSpectralSubtractorEngine::setReductionDb(float db) {
    m_params.reductionDb = std::max(-36.0f, std::min(-2.0f, db));
}

void JZSpeechDenoiseSpectralSubtractorEngine::updateFilterbank() {
    m_smoothingCoeff = std::exp(-1.0f / ((m_params.smoothingMs / 1000.0f) * m_sampleRate));

    for (size_t b = 0; b < NUM_SPECTRAL_BANDS; ++b) {
        float f0 = m_bands[b].centerFreqHz;
        if (f0 >= m_sampleRate * 0.45f) f0 = m_sampleRate * 0.45f;
        float Q = 2.8f;

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

void JZSpeechDenoiseSpectralSubtractorEngine::learnNoiseProfile(
    const float* silenceBuffer, size_t numSamples) {

    if (!silenceBuffer || numSamples == 0) return;

    float totalPower = 0.0f;
    for (size_t b = 0; b < NUM_SPECTRAL_BANDS; ++b) {
        SpectralBand& band = m_bands[b];
        float bandPower = 0.0f;

        for (size_t i = 0; i < numSamples; ++i) {
            float s = silenceBuffer[i];
            float out = band.b0 * s + band.b1 * band.x1L + band.b2 * band.x2L
                        - band.a1 * band.y1L - band.a2 * band.y2L;
            band.x2L = band.x1L; band.x1L = s;
            band.y2L = band.y1L; band.y1L = out;
            bandPower += out * out;
        }

        band.noiseFloorPower = std::max(1e-7f, bandPower / static_cast<float>(numSamples));
        totalPower += band.noiseFloorPower;
    }

    m_avgNoiseFloorDb = 10.0f * std::log10(std::max(1e-8f, totalPower / static_cast<float>(NUM_SPECTRAL_BANDS)));
}

void JZSpeechDenoiseSpectralSubtractorEngine::process(
    const float* inL, const float* inR,
    float* outL, float* outR,
    size_t numSamples) {

    if (!inL || !outL || numSamples == 0) return;
    const bool stereo = (inR != nullptr && outR != nullptr);

    float minGainLinear = std::pow(10.0f, m_params.reductionDb / 20.0f);
    float alpha = m_params.oversubtractionFactor;
    float wet = m_params.dryWet;
    float dry = 1.0f - wet;

    float maxReductObserved = 1.0f;

    for (size_t i = 0; i < numSamples; ++i) {
        float sL = inL[i];
        float sR = stereo ? inR[i] : sL;

        float cleanL = 0.0f;
        float cleanR = 0.0f;

        for (size_t b = 0; b < NUM_SPECTRAL_BANDS; ++b) {
            SpectralBand& band = m_bands[b];

            // 1. Filter into band
            float bL = band.b0 * sL + band.b1 * band.x1L + band.b2 * band.x2L
                       - band.a1 * band.y1L - band.a2 * band.y2L;
            band.x2L = band.x1L; band.x1L = sL;
            band.y2L = band.y1L; band.y1L = bL;

            float bR = bL;
            if (stereo) {
                bR = band.b0 * sR + band.b1 * band.x1R + band.b2 * band.x2R
                     - band.a1 * band.y1R - band.a2 * band.y2R;
                band.x2R = band.x1R; band.x1R = sR;
                band.y2R = band.y1R; band.y1R = bR;
            }

            // 2. If in learning mode, adaptively update noise floor
            if (m_params.isLearning) {
                float p = 0.5f * (bL * bL + bR * bR);
                band.noiseFloorPower = band.noiseFloorPower * 0.999f + p * 0.001f;
            }

            // 3. Compute Wiener gain suppression
            float pL = bL * bL;
            float noiseP = band.noiseFloorPower * alpha;
            float snrL = std::max(0.0f, (pL - noiseP) / (noiseP + 1e-8f));
            float targetGainL = snrL / (1.0f + snrL);

            // Consonant protection for high frequencies
            float bandFloor = minGainLinear;
            if (m_params.preserveConsonants && band.centerFreqHz > 4500.0f) {
                bandFloor = std::max(minGainLinear, 0.35f); // Cap cut at -9dB for sibilants
            }
            targetGainL = std::max(bandFloor, targetGainL);

            // Ballistics smoothing
            band.currentGainL = m_smoothingCoeff * band.currentGainL + (1.0f - m_smoothingCoeff) * targetGainL;
            if (band.currentGainL < maxReductObserved) maxReductObserved = band.currentGainL;

            cleanL += bL * band.currentGainL;

            if (stereo) {
                float pR = bR * bR;
                float snrR = std::max(0.0f, (pR - noiseP) / (noiseP + 1e-8f));
                float targetGainR = std::max(bandFloor, snrR / (1.0f + snrR));
                band.currentGainR = m_smoothingCoeff * band.currentGainR + (1.0f - m_smoothingCoeff) * targetGainR;
                cleanR += bR * band.currentGainR;
            }
        }

        outL[i] = sL * dry + cleanL * wet;
        if (stereo) {
            outR[i] = sR * dry + cleanR * wet;
        }
    }

    m_currentReductionDb = 20.0f * std::log10(std::max(1e-4f, maxReductObserved));
}

} // namespace Jazz
