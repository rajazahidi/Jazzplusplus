//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// AdaptiveRoomDereverberationEngine.cpp - Adaptive Spectral Dereverberation
//*****************************************************************************

#include "AdaptiveRoomDereverberationEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

JZAdaptiveRoomDereverberationEngine::JZAdaptiveRoomDereverberationEngine(float sampleRate) {
    initialize(sampleRate);
}

void JZAdaptiveRoomDereverberationEngine::initialize(float sampleRate) {
    if (sampleRate < 8000.0f) sampleRate = 48000.0f;
    m_sampleRate = sampleRate;

    float centerFreqs[NUM_BANDS] = { 125.0f, 250.0f, 500.0f, 1000.0f, 2000.0f, 4000.0f, 6000.0f, 8000.0f };
    for (size_t b = 0; b < NUM_BANDS; ++b) {
        m_bands[b].centerFreqHz = centerFreqs[b];
    }

    reset();
    updateFilterbank();
}

void JZAdaptiveRoomDereverberationEngine::reset() {
    for (size_t b = 0; b < NUM_BANDS; ++b) {
        m_bands[b].x1L = m_bands[b].x2L = m_bands[b].y1L = m_bands[b].y2L = 0.0f;
        m_bands[b].x1R = m_bands[b].x2R = m_bands[b].y1R = m_bands[b].y2R = 0.0f;
        m_bands[b].fastEnergyL = m_bands[b].fastEnergyR = 0.0f;
        m_bands[b].decayModelL = m_bands[b].decayModelR = 0.0f;
    }
    m_avgSuppressionDb = 0.0f;
    m_reverbTailEnergy = 0.0f;
}

void JZAdaptiveRoomDereverberationEngine::setParameters(const Parameters& params) {
    m_params = params;
    m_params.dereverbAmount = std::max(0.0f, std::min(1.0f, m_params.dereverbAmount));
    m_params.roomRt60Sec = std::max(0.2f, std::min(4.0f, m_params.roomRt60Sec));
    m_params.transientPreserve = std::max(0.0f, std::min(1.0f, m_params.transientPreserve));
    m_params.maxSuppressionDb = std::max(-30.0f, std::min(-6.0f, m_params.maxSuppressionDb));
    m_params.directBoostDb = std::max(0.0f, std::min(6.0f, m_params.directBoostDb));
    updateFilterbank();
}

void JZAdaptiveRoomDereverberationEngine::setDereverbAmount(float amt) {
    m_params.dereverbAmount = std::max(0.0f, std::min(1.0f, amt));
}

void JZAdaptiveRoomDereverberationEngine::setRoomRt60Sec(float rt60) {
    m_params.roomRt60Sec = std::max(0.2f, std::min(4.0f, rt60));
    updateFilterbank();
}

void JZAdaptiveRoomDereverberationEngine::updateFilterbank() {
    // Statistical Sabine/Eyring decay coefficient: e^(-6.91 / (RT60 * Fs))
    m_decayCoeff = std::exp(-6.907755f / (m_params.roomRt60Sec * m_sampleRate));

    for (size_t b = 0; b < NUM_BANDS; ++b) {
        float f0 = m_bands[b].centerFreqHz;
        float Q = 1.8f;
        float w0 = 2.0f * static_cast<float>(M_PI) * (f0 / m_sampleRate);
        float sin_w = std::sin(w0);
        float cos_w = std::cos(w0);
        float alpha = sin_w / (2.0f * Q);

        // Constant skirt bandpass
        float a0 = 1.0f + alpha;
        m_bands[b].b0 = alpha / a0;
        m_bands[b].b1 = 0.0f;
        m_bands[b].b2 = -alpha / a0;
        m_bands[b].a1 = (-2.0f * cos_w) / a0;
        m_bands[b].a2 = (1.0f - alpha) / a0;
    }
}

void JZAdaptiveRoomDereverberationEngine::process(
    const float* inL, const float* inR,
    float* outL, float* outR,
    size_t numSamples) {

    if (!inL || !outL || numSamples == 0) return;
    const bool stereo = (inR != nullptr && outR != nullptr);

    float minGainLinear = std::pow(10.0f, m_params.maxSuppressionDb / 20.0f);
    float directGainLinear = std::pow(10.0f, m_params.directBoostDb / 20.0f);

    float totalGainLinearSum = 0.0f;
    float maxTailEnergyObserved = 0.0f;

    for (size_t i = 0; i < numSamples; ++i) {
        float sampleL = inL[i];
        float sampleR = stereo ? inR[i] : sampleL;

        float bandSumL = 0.0f;
        float bandSumR = 0.0f;
        float sampleAvgGain = 0.0f;

        for (size_t b = 0; b < NUM_BANDS; ++b) {
            BandState& band = m_bands[b];

            // 1. Filter band Left
            float bOutL = band.b0 * sampleL + band.b1 * band.x1L + band.b2 * band.x2L
                          - band.a1 * band.y1L - band.a2 * band.y2L;
            band.x2L = band.x1L; band.x1L = sampleL;
            band.y2L = band.y1L; band.y1L = bOutL;

            // Fast envelope tracking
            float absBL = std::abs(bOutL);
            band.fastEnergyL = band.fastEnergyL * 0.98f + absBL * 0.02f;

            // Sabine/Eyring late reverberation model tracker
            if (band.fastEnergyL > band.decayModelL) {
                // Direct sound attack
                band.decayModelL = band.fastEnergyL;
            } else {
                // Decaying late tail
                band.decayModelL *= m_decayCoeff;
            }

            // Estimate late diffuse reverberation fraction
            float lateTailL = std::max(0.0f, band.decayModelL - band.fastEnergyL);
            if (lateTailL > maxTailEnergyObserved) maxTailEnergyObserved = lateTailL;

            float tailRatioL = (band.decayModelL > 1e-5f) ? (lateTailL / band.decayModelL) : 0.0f;
            float gainL = 1.0f - m_params.dereverbAmount * tailRatioL * (1.0f - m_params.transientPreserve * 0.4f);
            gainL = std::max(minGainLinear, std::min(1.0f, gainL));

            if (tailRatioL < 0.15f) {
                gainL *= directGainLinear;
            }

            bandSumL += bOutL * gainL;
            sampleAvgGain += gainL;

            // 2. Filter band Right
            if (stereo) {
                float bOutR = band.b0 * sampleR + band.b1 * band.x1R + band.b2 * band.x2R
                              - band.a1 * band.y1R - band.a2 * band.y2R;
                band.x2R = band.x1R; band.x1R = sampleR;
                band.y2R = band.y1R; band.y1R = bOutR;

                float absBR = std::abs(bOutR);
                band.fastEnergyR = band.fastEnergyR * 0.98f + absBR * 0.02f;

                if (band.fastEnergyR > band.decayModelR) {
                    band.decayModelR = band.fastEnergyR;
                } else {
                    band.decayModelR *= m_decayCoeff;
                }

                float lateTailR = std::max(0.0f, band.decayModelR - band.fastEnergyR);
                float tailRatioR = (band.decayModelR > 1e-5f) ? (lateTailR / band.decayModelR) : 0.0f;
                float gainR = 1.0f - m_params.dereverbAmount * tailRatioR * (1.0f - m_params.transientPreserve * 0.4f);
                gainR = std::max(minGainLinear, std::min(1.0f, gainR));

                if (tailRatioR < 0.15f) {
                    gainR *= directGainLinear;
                }

                bandSumR += bOutR * gainR;
            }
        }

        outL[i] = bandSumL;
        if (stereo) outR[i] = bandSumR;

        totalGainLinearSum += (sampleAvgGain / static_cast<float>(NUM_BANDS));
    }

    float meanGain = totalGainLinearSum / static_cast<float>(numSamples);
    m_avgSuppressionDb = (meanGain > 1e-5f) ? (20.0f * std::log10(meanGain)) : m_params.maxSuppressionDb;
    m_reverbTailEnergy = maxTailEnergyObserved;
}

} // namespace Jazz
