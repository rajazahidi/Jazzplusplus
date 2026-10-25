//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// ImmersiveBassManagementCrossoverEngine.cpp - 9.1.6 Dolby Atmos Bass Manager
//*****************************************************************************

#include "ImmersiveBassManagementCrossoverEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

JZImmersiveBassManagementCrossoverEngine::JZImmersiveBassManagementCrossoverEngine(float sampleRate) {
    initialize(sampleRate);
}

void JZImmersiveBassManagementCrossoverEngine::initialize(float sampleRate) {
    if (sampleRate < 8000.0f) sampleRate = 48000.0f;
    m_sampleRate = sampleRate;

    reset();
    updateCrossoverCoefficients();
}

void JZImmersiveBassManagementCrossoverEngine::reset() {
    for (size_t ch = 0; ch < NUM_CHANNELS; ++ch) {
        m_crossovers[ch].reset();
    }
    m_subEnergy = 0.0f;
}

void JZImmersiveBassManagementCrossoverEngine::setParameters(const Parameters& params) {
    m_params = params;
    m_params.crossoverFreqHz = std::max(40.0f, std::min(160.0f, m_params.crossoverFreqHz));
    m_params.lfeGainBoostDb = std::max(0.0f, std::min(14.0f, m_params.lfeGainBoostDb));
    updateCrossoverCoefficients();
}

void JZImmersiveBassManagementCrossoverEngine::setCrossoverFreqHz(float hz) {
    m_params.crossoverFreqHz = std::max(40.0f, std::min(160.0f, hz));
    updateCrossoverCoefficients();
}

void JZImmersiveBassManagementCrossoverEngine::setLfeGainBoostDb(float db) {
    m_params.lfeGainBoostDb = std::max(0.0f, std::min(14.0f, db));
}

std::string JZImmersiveBassManagementCrossoverEngine::getChannelName(size_t index) const {
    static const char* channelNames[NUM_CHANNELS] = {
        "L", "R", "C", "LFE",
        "Lss", "Rss", "Lrs", "Rrs",
        "Lw", "Rw",
        "Ltf", "Rtf", "Ltm", "Rtm", "Ltr", "Rtr"
    };
    if (index < NUM_CHANNELS) return channelNames[index];
    return "Unknown";
}

void JZImmersiveBassManagementCrossoverEngine::updateCrossoverCoefficients() {
    // 2nd-order Butterworth biquad (two in cascade form 4th-order Linkwitz-Riley)
    float f0 = m_params.crossoverFreqHz;
    float w0 = 2.0f * static_cast<float>(M_PI) * (f0 / m_sampleRate);
    float sin_w = std::sin(w0);
    float cos_w = std::cos(w0);
    float alpha = sin_w / (2.0f * 0.7071f);
    float a0 = 1.0f + alpha;

    float lpB0 = ((1.0f - cos_w) / 2.0f) / a0;
    float lpB1 = (1.0f - cos_w) / a0;
    float lpB2 = ((1.0f - cos_w) / 2.0f) / a0;
    float lpA1 = (-2.0f * cos_w) / a0;
    float lpA2 = (1.0f - alpha) / a0;

    float hpB0 = ((1.0f + cos_w) / 2.0f) / a0;
    float hpB1 = (-(1.0f + cos_w)) / a0;
    float hpB2 = ((1.0f + cos_w) / 2.0f) / a0;
    float hpA1 = (-2.0f * cos_w) / a0;
    float hpA2 = (1.0f - alpha) / a0;

    for (size_t ch = 0; ch < NUM_CHANNELS; ++ch) {
        LinkwitzRiley4thOrder& lr = m_crossovers[ch];
        lr.lpB0 = lpB0; lr.lpB1 = lpB1; lr.lpB2 = lpB2;
        lr.lpA1 = lpA1; lr.lpA2 = lpA2;

        lr.hpB0 = hpB0; lr.hpB1 = hpB1; lr.hpB2 = hpB2;
        lr.hpA1 = hpA1; lr.hpA2 = hpA2;
    }
}

void JZImmersiveBassManagementCrossoverEngine::process16Channels(
    const float* const* in16Channels,
    float* const* out16Channels,
    size_t numSamples) {

    if (!in16Channels || !out16Channels || numSamples == 0) return;

    if (m_params.bypass) {
        for (size_t ch = 0; ch < NUM_CHANNELS; ++ch) {
            if (in16Channels[ch] && out16Channels[ch]) {
                for (size_t i = 0; i < numSamples; ++i) {
                    out16Channels[ch][i] = in16Channels[ch][i];
                }
            }
        }
        return;
    }

    float lfeGain = std::pow(10.0f, m_params.lfeGainBoostDb / 20.0f);
    float subPolarity = m_params.subPolarityInvert ? -1.0f : 1.0f;
    bool bassMgmt = m_params.bassManagementEnabled;

    float subAccumTotal = 0.0f;

    for (size_t i = 0; i < numSamples; ++i) {
        float redirectedSubBass = 0.0f;

        // Process all 15 satellite channels
        for (size_t ch = 0; ch < NUM_CHANNELS; ++ch) {
            if (ch == LFE_CHANNEL_INDEX) continue; // Skip LFE here

            const float* inCh = in16Channels[ch];
            float* outCh = out16Channels[ch];
            if (!inCh || !outCh) continue;

            float sample = inCh[i];

            if (bassMgmt) {
                LinkwitzRiley4thOrder& lr = m_crossovers[ch];

                // 1. High-Pass Filter: Stage 1
                float hp1 = lr.hpB0 * sample + lr.hpB1 * lr.hpX1_1 + lr.hpB2 * lr.hpX2_1
                            - lr.hpA1 * lr.hpY1_1 - lr.hpA2 * lr.hpY2_1;
                lr.hpX2_1 = lr.hpX1_1; lr.hpX1_1 = sample;
                lr.hpY2_1 = lr.hpY1_1; lr.hpY1_1 = hp1;

                // Stage 2
                float hp2 = lr.hpB0 * hp1 + lr.hpB1 * lr.hpX1_2 + lr.hpB2 * lr.hpX2_2
                            - lr.hpA1 * lr.hpY1_2 - lr.hpA2 * lr.hpY2_2;
                lr.hpX2_2 = lr.hpX1_2; lr.hpX1_2 = hp1;
                lr.hpY2_2 = lr.hpY1_2; lr.hpY1_2 = hp2;

                outCh[i] = hp2; // Satellite speaker gets high-pass content

                // 2. Low-Pass Filter: Stage 1
                float lp1 = lr.lpB0 * sample + lr.lpB1 * lr.lpX1_1 + lr.lpB2 * lr.lpX2_1
                            - lr.lpA1 * lr.lpY1_1 - lr.lpA2 * lr.lpY2_1;
                lr.lpX2_1 = lr.lpX1_1; lr.lpX1_1 = sample;
                lr.lpY2_1 = lr.lpY1_1; lr.lpY1_1 = lp1;

                // Stage 2
                float lp2 = lr.lpB0 * lp1 + lr.lpB1 * lr.lpX1_2 + lr.lpB2 * lr.lpX2_2
                            - lr.lpA1 * lr.lpY1_2 - lr.lpA2 * lr.lpY2_2;
                lr.lpX2_2 = lr.lpX1_2; lr.lpX1_2 = lp1;
                lr.lpY2_2 = lr.lpY1_2; lr.lpY1_2 = lp2;

                redirectedSubBass += lp2 * 0.25f; // Sum to subwoofer bus
            } else {
                outCh[i] = sample;
            }
        }

        // Process discrete LFE channel (index 3)
        const float* inLfe = in16Channels[LFE_CHANNEL_INDEX];
        float* outLfe = out16Channels[LFE_CHANNEL_INDEX];

        float nativeLfe = inLfe ? inLfe[i] : 0.0f;
        float totalSub = (nativeLfe * lfeGain + redirectedSubBass) * subPolarity;

        if (outLfe) {
            outLfe[i] = totalSub;
        }

        subAccumTotal += std::abs(totalSub);
    }

    m_subEnergy = subAccumTotal / static_cast<float>(numSamples);
}

} // namespace Jazz
