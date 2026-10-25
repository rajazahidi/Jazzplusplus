//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// MultibandTransientSculptorEngine.cpp - 4-Band Transient & Sustain Sculptor
//*****************************************************************************

#include "MultibandTransientSculptorEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

JZMultibandTransientSculptorEngine::JZMultibandTransientSculptorEngine(float sampleRate) {
    initialize(sampleRate);
}

void JZMultibandTransientSculptorEngine::initialize(float sampleRate) {
    if (sampleRate < 8000.0f) sampleRate = 48000.0f;
    m_sampleRate = sampleRate;

    reset();
    updateCrossoverFilters();
}

void JZMultibandTransientSculptorEngine::reset() {
    for (size_t c = 0; c < 3; ++c) {
        FilterBand& fb = m_filters[c];
        fb.lpX1L = fb.lpX2L = fb.lpY1L = fb.lpY2L = 0.0f;
        fb.lpX1R = fb.lpX2R = fb.lpY1R = fb.lpY2R = 0.0f;
        fb.hpX1L = fb.hpX2L = fb.hpY1L = fb.hpY2L = 0.0f;
        fb.hpX1R = fb.hpX2R = fb.hpY1R = fb.hpY2R = 0.0f;
        fb.fastEnvL = fb.slowEnvL = 0.0f;
        fb.fastEnvR = fb.slowEnvR = 0.0f;
        fb.transientActivity = 0.0f;
    }
}

void JZMultibandTransientSculptorEngine::setParameters(const Parameters& params) {
    m_params = params;
    for (size_t b = 0; b < NUM_BANDS; ++b) {
        m_params.bands[b].attackDb = std::max(-18.0f, std::min(18.0f, m_params.bands[b].attackDb));
        m_params.bands[b].sustainDb = std::max(-18.0f, std::min(18.0f, m_params.bands[b].sustainDb));
        m_params.bands[b].punch = std::max(0.4f, std::min(2.5f, m_params.bands[b].punch));
    }
    m_params.dryWet = std::max(0.0f, std::min(1.0f, m_params.dryWet));
    updateCrossoverFilters();
}

void JZMultibandTransientSculptorEngine::setBandAttackDb(size_t bandIdx, float db) {
    if (bandIdx < NUM_BANDS) {
        m_params.bands[bandIdx].attackDb = std::max(-18.0f, std::min(18.0f, db));
    }
}

float JZMultibandTransientSculptorEngine::getBandAttackDb(size_t bandIdx) const {
    if (bandIdx < NUM_BANDS) return m_params.bands[bandIdx].attackDb;
    return 0.0f;
}

void JZMultibandTransientSculptorEngine::setBandSustainDb(size_t bandIdx, float db) {
    if (bandIdx < NUM_BANDS) {
        m_params.bands[bandIdx].sustainDb = std::max(-18.0f, std::min(18.0f, db));
    }
}

float JZMultibandTransientSculptorEngine::getBandSustainDb(size_t bandIdx) const {
    if (bandIdx < NUM_BANDS) return m_params.bands[bandIdx].sustainDb;
    return 0.0f;
}

float JZMultibandTransientSculptorEngine::getBandTransientActivity(size_t bandIdx) const {
    if (bandIdx < 3) return m_filters[bandIdx].transientActivity;
    return 0.0f;
}

void JZMultibandTransientSculptorEngine::updateCrossoverFilters() {
    for (size_t c = 0; c < 3; ++c) {
        float f0 = m_params.crossoverFreqs[c];
        if (f0 >= m_sampleRate * 0.45f) f0 = m_sampleRate * 0.45f;

        float w0 = 2.0f * static_cast<float>(M_PI) * (f0 / m_sampleRate);
        float sin_w = std::sin(w0);
        float cos_w = std::cos(w0);
        float alpha = sin_w / (2.0f * 0.7071f);
        float a0 = 1.0f + alpha;

        // Low-Pass
        m_filters[c].lpB0 = ((1.0f - cos_w) / 2.0f) / a0;
        m_filters[c].lpB1 = (1.0f - cos_w) / a0;
        m_filters[c].lpB2 = ((1.0f - cos_w) / 2.0f) / a0;
        m_filters[c].lpA1 = (-2.0f * cos_w) / a0;
        m_filters[c].lpA2 = (1.0f - alpha) / a0;

        // High-Pass
        m_filters[c].hpB0 = ((1.0f + cos_w) / 2.0f) / a0;
        m_filters[c].hpB1 = (-(1.0f + cos_w)) / a0;
        m_filters[c].hpB2 = ((1.0f + cos_w) / 2.0f) / a0;
        m_filters[c].hpA1 = (-2.0f * cos_w) / a0;
        m_filters[c].hpA2 = (1.0f - alpha) / a0;
    }
}

void JZMultibandTransientSculptorEngine::process(
    const float* inL, const float* inR,
    float* outL, float* outR,
    size_t numSamples) {

    if (!inL || !outL || numSamples == 0) return;
    const bool stereo = (inR != nullptr && outR != nullptr);

    float wet = m_params.dryWet;
    float dry = 1.0f - wet;

    // Linear gains for Attack and Sustain per band
    float attGain[NUM_BANDS], susGain[NUM_BANDS];
    for (size_t b = 0; b < NUM_BANDS; ++b) {
        attGain[b] = std::pow(10.0f, m_params.bands[b].attackDb / 20.0f);
        susGain[b] = std::pow(10.0f, m_params.bands[b].sustainDb / 20.0f);
    }

    for (size_t i = 0; i < numSamples; ++i) {
        float sL = inL[i];
        float sR = stereo ? inR[i] : sL;

        // Crossover 1: Split at 200 Hz into Low (Band 0) and Mid-High rest
        FilterBand& fb0 = m_filters[0];
        float b0_L = fb0.lpB0 * sL + fb0.lpB1 * fb0.lpX1L + fb0.lpB2 * fb0.lpX2L
                     - fb0.lpA1 * fb0.lpY1L - fb0.lpA2 * fb0.lpY2L;
        fb0.lpX2L = fb0.lpX1L; fb0.lpX1L = sL;
        fb0.lpY2L = fb0.lpY1L; fb0.lpY1L = b0_L;

        float rest1_L = fb0.hpB0 * sL + fb0.hpB1 * fb0.hpX1L + fb0.hpB2 * fb0.hpX2L
                        - fb0.hpA1 * fb0.hpY1L - fb0.hpA2 * fb0.hpY2L;
        fb0.hpX2L = fb0.hpX1L; fb0.hpX1L = sL;
        fb0.hpY2L = fb0.hpY1L; fb0.hpY1L = rest1_L;

        // Crossover 2: Split rest1 at 1200 Hz into Low-Mid (Band 1) and High rest
        FilterBand& fb1 = m_filters[1];
        float b1_L = fb1.lpB0 * rest1_L + fb1.lpB1 * fb1.lpX1L + fb1.lpB2 * fb1.lpX2L
                     - fb1.lpA1 * fb1.lpY1L - fb1.lpA2 * fb1.lpY2L;
        fb1.lpX2L = fb1.lpX1L; fb1.lpX1L = rest1_L;
        fb1.lpY2L = fb1.lpY1L; fb1.lpY1L = b1_L;

        float rest2_L = fb1.hpB0 * rest1_L + fb1.hpB1 * fb1.hpX1L + fb1.hpB2 * fb1.hpX2L
                        - fb1.hpA1 * fb1.hpY1L - fb1.hpA2 * fb1.hpY2L;
        fb1.hpX2L = fb1.hpX1L; fb1.hpX1L = rest1_L;
        fb1.hpY2L = fb1.hpY1L; fb1.hpY1L = rest2_L;

        // Crossover 3: Split rest2 at 5500 Hz into High-Mid (Band 2) and High (Band 3)
        FilterBand& fb2 = m_filters[2];
        float b2_L = fb2.lpB0 * rest2_L + fb2.lpB1 * fb2.lpX1L + fb2.lpB2 * fb2.lpX2L
                     - fb2.lpA1 * fb2.lpY1L - fb2.lpA2 * fb2.lpY2L;
        fb2.lpX2L = fb2.lpX1L; fb2.lpX1L = rest2_L;
        fb2.lpY2L = fb2.lpY1L; fb2.lpY1L = b2_L;

        float b3_L = fb2.hpB0 * rest2_L + fb2.hpB1 * fb2.hpX1L + fb2.hpB2 * fb2.hpX2L
                     - fb2.hpA1 * fb2.hpY1L - fb2.hpA2 * fb2.hpY2L;
        fb2.hpX2L = fb2.hpX1L; fb2.hpX1L = rest2_L;
        fb2.hpY2L = fb2.hpY1L; fb2.hpY1L = b3_L;

        float bandsL[NUM_BANDS] = { b0_L, b1_L, b2_L, b3_L };
        float procSumL = 0.0f;

        for (size_t b = 0; b < NUM_BANDS; ++b) {
            float x = bandsL[b];
            float absX = std::abs(x);

            // Fast envelope (~3ms) and slow envelope (~60ms)
            float& fastEnv = (b < 3) ? m_filters[b].fastEnvL : m_filters[2].fastEnvL;
            float& slowEnv = (b < 3) ? m_filters[b].slowEnvL : m_filters[2].slowEnvL;

            fastEnv = fastEnv * 0.95f + absX * 0.05f;
            slowEnv = slowEnv * 0.992f + absX * 0.008f;

            float diff = std::max(0.0f, fastEnv - slowEnv);
            float transMod = (slowEnv > 1e-5f) ? (diff / slowEnv) : 0.0f;
            transMod = std::min(2.0f, transMod);

            if (b < 3) m_filters[b].transientActivity = diff;

            // Apply attack & sustain gains
            float gainMod = (1.0f + (attGain[b] - 1.0f) * transMod) * susGain[b];
            procSumL += x * gainMod;
        }

        outL[i] = sL * dry + procSumL * wet;
        if (stereo) {
            outR[i] = sR * dry + procSumL * wet;
        }
    }
}

} // namespace Jazz
