//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// PsychoacousticPhantomBassEngine.cpp - Phantom Missing Fundamental Bass Generator
//*****************************************************************************

#include "PsychoacousticPhantomBassEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

JZPsychoacousticPhantomBassEngine::JZPsychoacousticPhantomBassEngine(float sampleRate) {
    initialize(sampleRate);
}

void JZPsychoacousticPhantomBassEngine::initialize(float sampleRate) {
    if (sampleRate < 8000.0f) sampleRate = 48000.0f;
    m_sampleRate = sampleRate;

    // Envelope tracking coefficients (attack ~5ms, release ~40ms)
    m_envAttackCoeff = std::exp(-1.0f / (0.005f * m_sampleRate));
    m_envReleaseCoeff = std::exp(-1.0f / (0.040f * m_sampleRate));

    reset();
    updateCrossover();
}

void JZPsychoacousticPhantomBassEngine::reset() {
    m_lpFilterL.reset();
    m_lpFilterR.reset();
    m_hpFilterL.reset();
    m_hpFilterR.reset();

    m_subEnvL = 0.0f;
    m_subEnvR = 0.0f;
    m_subRms = 0.0f;
    m_harmonicsRms = 0.0f;
}

void JZPsychoacousticPhantomBassEngine::setParameters(const Parameters& params) {
    m_params = params;
    m_params.crossoverFreqHz = std::max(30.0f, std::min(160.0f, m_params.crossoverFreqHz));
    m_params.harmonicIntensity = std::max(0.0f, std::min(3.0f, m_params.harmonicIntensity));
    m_params.originalSubLevel = std::max(0.0f, std::min(1.0f, m_params.originalSubLevel));
    m_params.phantomHarmonicMix = std::max(0.0f, std::min(2.0f, m_params.phantomHarmonicMix));
    updateCrossover();
}

void JZPsychoacousticPhantomBassEngine::setCrossoverFreq(float hz) {
    m_params.crossoverFreqHz = std::max(30.0f, std::min(160.0f, hz));
    updateCrossover();
}

void JZPsychoacousticPhantomBassEngine::setHarmonicIntensity(float intensity) {
    m_params.harmonicIntensity = std::max(0.0f, std::min(3.0f, intensity));
}

void JZPsychoacousticPhantomBassEngine::setPhantomHarmonicMix(float mix) {
    m_params.phantomHarmonicMix = std::max(0.0f, std::min(2.0f, mix));
}

void JZPsychoacousticPhantomBassEngine::updateCrossover() {
    float f0 = m_params.crossoverFreqHz;
    float w0 = 2.0f * static_cast<float>(M_PI) * (f0 / m_sampleRate);
    float cos_w = std::cos(w0);
    float sin_w = std::sin(w0);
    float alpha = sin_w / (2.0f * 0.70710678f); // Butterworth Q

    // 1. Low-Pass Biquad Coefficients
    float b0_lp = (1.0f - cos_w) / 2.0f;
    float b1_lp = 1.0f - cos_w;
    float b2_lp = (1.0f - cos_w) / 2.0f;
    float a0_lp = 1.0f + alpha;
    float a1_lp = -2.0f * cos_w;
    float a2_lp = 1.0f - alpha;

    m_lpFilterL.b0 = b0_lp / a0_lp; m_lpFilterL.b1 = b1_lp / a0_lp; m_lpFilterL.b2 = b2_lp / a0_lp;
    m_lpFilterL.a1 = a1_lp / a0_lp; m_lpFilterL.a2 = a2_lp / a0_lp;
    m_lpFilterR.b0 = m_lpFilterL.b0; m_lpFilterR.b1 = m_lpFilterL.b1; m_lpFilterR.b2 = m_lpFilterL.b2;
    m_lpFilterR.a1 = m_lpFilterL.a1; m_lpFilterR.a2 = m_lpFilterL.a2;

    // 2. High-Pass Biquad Coefficients
    float b0_hp = (1.0f + cos_w) / 2.0f;
    float b1_hp = -(1.0f + cos_w);
    float b2_hp = (1.0f + cos_w) / 2.0f;
    float a0_hp = 1.0f + alpha;
    float a1_hp = -2.0f * cos_w;
    float a2_hp = 1.0f - alpha;

    m_hpFilterL.b0 = b0_hp / a0_hp; m_hpFilterL.b1 = b1_hp / a0_hp; m_hpFilterL.b2 = b2_hp / a0_hp;
    m_hpFilterL.a1 = a1_hp / a0_hp; m_hpFilterL.a2 = a2_hp / a0_hp;
    m_hpFilterR.b0 = m_hpFilterL.b0; m_hpFilterR.b1 = m_hpFilterL.b1; m_hpFilterR.b2 = m_hpFilterL.b2;
    m_hpFilterR.a1 = m_hpFilterL.a1; m_hpFilterR.a2 = m_hpFilterL.a2;
}

void JZPsychoacousticPhantomBassEngine::process(
    const float* inL, const float* inR,
    float* outL, float* outR,
    size_t numSamples) {

    if (!inL || !outL || numSamples == 0) return;
    const bool stereo = (inR != nullptr && outR != nullptr);

    float sumSubSq = 0.0f;
    float sumHarmSq = 0.0f;

    for (size_t i = 0; i < numSamples; ++i) {
        float sampleL = inL[i];
        float sampleR = stereo ? inR[i] : sampleL;

        // Step 1: Split into Sub-Fundamental and Highs
        float subL = m_lpFilterL.processSample(sampleL);
        float highL = m_hpFilterL.processSample(sampleL);

        float subR = stereo ? m_lpFilterR.processSample(sampleR) : subL;
        float highR = stereo ? m_hpFilterR.processSample(sampleR) : highL;

        // Step 2: Dynamic envelope tracking
        float absSubL = std::abs(subL);
        if (absSubL > m_subEnvL) m_subEnvL = m_envAttackCoeff * m_subEnvL + (1.0f - m_envAttackCoeff) * absSubL;
        else m_subEnvL = m_envReleaseCoeff * m_subEnvL + (1.0f - m_envReleaseCoeff) * absSubL;

        float absSubR = std::abs(subR);
        if (absSubR > m_subEnvR) m_subEnvR = m_envAttackCoeff * m_subEnvR + (1.0f - m_envAttackCoeff) * absSubR;
        else m_subEnvR = m_envReleaseCoeff * m_subEnvR + (1.0f - m_envReleaseCoeff) * absSubR;

        // Step 3: Chebyshev polynomial overtone series generation
        // Normalize low-pass input
        float normL = std::tanh(subL * 1.5f);
        float normSqL = normL * normL;
        float t2_L = 2.0f * normSqL - 1.0f;                  // 2nd Harmonic (2 * f0)
        float t3_L = 4.0f * normL * normSqL - 3.0f * normL;  // 3rd Harmonic (3 * f0)
        float t4_L = 8.0f * normSqL * normSqL - 8.0f * normSqL + 1.0f; // 4th Harmonic (4 * f0)

        float harmL = (t2_L * m_params.harmonic2ndWeight +
                       t3_L * m_params.harmonic3rdWeight +
                       t4_L * m_params.harmonic4thWeight) * m_subEnvL * m_params.harmonicIntensity;

        float normR = std::tanh(subR * 1.5f);
        float normSqR = normR * normR;
        float t2_R = 2.0f * normSqR - 1.0f;
        float t3_R = 4.0f * normR * normSqR - 3.0f * normR;
        float t4_R = 8.0f * normSqR * normSqR - 8.0f * normSqR + 1.0f;

        float harmR = (t2_R * m_params.harmonic2ndWeight +
                       t3_R * m_params.harmonic3rdWeight +
                       t4_R * m_params.harmonic4thWeight) * m_subEnvR * m_params.harmonicIntensity;

        // Step 4: Recombination
        float subRetainedL = m_params.cutOriginalSub ? 0.0f : (subL * m_params.originalSubLevel);
        float subRetainedR = m_params.cutOriginalSub ? 0.0f : (subR * m_params.originalSubLevel);

        outL[i] = highL + subRetainedL + harmL * m_params.phantomHarmonicMix;
        if (stereo) {
            outR[i] = highR + subRetainedR + harmR * m_params.phantomHarmonicMix;
        }

        sumSubSq += (subL * subL + subR * subR) * 0.5f;
        sumHarmSq += (harmL * harmL + harmR * harmR) * 0.5f;
    }

    m_subRms = std::sqrt(sumSubSq / static_cast<float>(numSamples));
    m_harmonicsRms = std::sqrt(sumHarmSq / static_cast<float>(numSamples));
}

} // namespace Jazz
