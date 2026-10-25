//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// AcousticGuitarBodyImpulseConvolverEngine.cpp - Acoustic Guitar Pickup Convolver
//*****************************************************************************

#include "AcousticGuitarBodyImpulseConvolverEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

JZAcousticGuitarBodyImpulseConvolverEngine::JZAcousticGuitarBodyImpulseConvolverEngine(float sampleRate) {
    initialize(sampleRate);
}

void JZAcousticGuitarBodyImpulseConvolverEngine::initialize(float sampleRate) {
    if (sampleRate < 8000.0f) sampleRate = 48000.0f;
    m_sampleRate = sampleRate;

    reset();
    updateAcousticFilters();
}

void JZAcousticGuitarBodyImpulseConvolverEngine::reset() {
    m_bodyX1L = m_bodyX2L = m_bodyY1L = m_bodyY2L = 0.0f;
    m_bodyX1R = m_bodyX2R = m_bodyY1R = m_bodyY2R = 0.0f;

    m_notchX1L = m_notchX2L = m_notchY1L = m_notchY2L = 0.0f;
    m_notchX1R = m_notchX2R = m_notchY1R = m_notchY2R = 0.0f;

    m_micX1L = m_micX2L = m_micY1L = m_micY2L = 0.0f;
    m_micX1R = m_micX2R = m_micY1R = m_micY2R = 0.0f;

    m_quackEnv = 0.0f;
    m_antiQuackReductDb = 0.0f;
}

void JZAcousticGuitarBodyImpulseConvolverEngine::setParameters(const Parameters& params) {
    m_params = params;
    m_params.bodyResonanceAmount = std::max(0.0f, std::min(1.0f, m_params.bodyResonanceAmount));
    m_params.antiQuackAmount = std::max(0.0f, std::min(1.0f, m_params.antiQuackAmount));
    m_params.soundholeNotchHz = std::max(60.0f, std::min(300.0f, m_params.soundholeNotchHz));
    m_params.dryWet = std::max(0.0f, std::min(1.0f, m_params.dryWet));

    updateAcousticFilters();
}

void JZAcousticGuitarBodyImpulseConvolverEngine::setMicProfile(GuitarMicProfile profile) {
    m_params.profile = profile;
    updateAcousticFilters();
}

void JZAcousticGuitarBodyImpulseConvolverEngine::setBodyResonanceAmount(float amount) {
    m_params.bodyResonanceAmount = std::max(0.0f, std::min(1.0f, amount));
    updateAcousticFilters();
}

void JZAcousticGuitarBodyImpulseConvolverEngine::setAntiQuackAmount(float amount) {
    m_params.antiQuackAmount = std::max(0.0f, std::min(1.0f, amount));
}

void JZAcousticGuitarBodyImpulseConvolverEngine::setSoundholeNotchHz(float hz) {
    m_params.soundholeNotchHz = std::max(60.0f, std::min(300.0f, hz));
    updateAcousticFilters();
}

std::string JZAcousticGuitarBodyImpulseConvolverEngine::getActiveProfileName() const {
    switch (m_params.profile) {
        case GuitarMicProfile::KM184_SMALL_CONDENSER: return "Neumann KM184 Small Condenser";
        case GuitarMicProfile::R121_VINTAGE_RIBBON:   return "Royer R-121 Vintage Ribbon";
        case GuitarMicProfile::C414_LARGE_CONDENSER:  return "AKG C414 Large Condenser";
        case GuitarMicProfile::DREADNOUGHT_CHAMBER:   return "Dreadnought Spruce Chamber";
    }
    return "Custom Profile";
}

void JZAcousticGuitarBodyImpulseConvolverEngine::updateAcousticFilters() {
    // 1. 102 Hz Helmholtz soundboard cavity peaking filter (+6 dB * bodyResonanceAmount)
    float bodyF0 = 102.0f;
    float bodyGainDb = 6.0f * m_params.bodyResonanceAmount;
    float bodyA = std::pow(10.0f, bodyGainDb / 40.0f);
    float bodyW0 = 2.0f * static_cast<float>(M_PI) * (bodyF0 / m_sampleRate);
    float bodyAlpha = std::sin(bodyW0) / (2.0f * 4.2f);
    float bodyCos = std::cos(bodyW0);

    float b0_b = 1.0f + bodyAlpha * bodyA;
    float b1_b = -2.0f * bodyCos;
    float b2_b = 1.0f - bodyAlpha * bodyA;
    float a0_b = 1.0f + bodyAlpha / bodyA;
    float a1_b = -2.0f * bodyCos;
    float a2_b = 1.0f - bodyAlpha / bodyA;

    m_bodyB0 = b0_b / a0_b; m_bodyB1 = b1_b / a0_b; m_bodyB2 = b2_b / a0_b;
    m_bodyA1 = a1_b / a0_b; m_bodyA2 = a2_b / a0_b;

    // 2. Soundhole anti-feedback notch (-18 dB cut at soundholeNotchHz, Q=12.0)
    float notchF0 = m_params.soundholeNotchHz;
    float notchA = std::pow(10.0f, -18.0f / 40.0f);
    float notchW0 = 2.0f * static_cast<float>(M_PI) * (notchF0 / m_sampleRate);
    float notchAlpha = std::sin(notchW0) / (2.0f * 12.0f);
    float notchCos = std::cos(notchW0);

    float b0_n = 1.0f + notchAlpha * notchA;
    float b1_n = -2.0f * notchCos;
    float b2_n = 1.0f - notchAlpha * notchA;
    float a0_n = 1.0f + notchAlpha / notchA;
    float a1_n = -2.0f * notchCos;
    float a2_n = 1.0f - notchAlpha / notchA;

    m_notchB0 = b0_n / a0_n; m_notchB1 = b1_n / a0_n; m_notchB2 = b2_n / a0_n;
    m_notchA1 = a1_n / a0_n; m_notchA2 = a2_n / a0_n;

    // 3. Microphone profile acoustic coloration
    float micF0 = 6500.0f;
    float micGainDb = 3.5f;
    float micQ = 1.2f;

    switch (m_params.profile) {
        case GuitarMicProfile::KM184_SMALL_CONDENSER:
            micF0 = 8500.0f; micGainDb = 4.0f; micQ = 1.5f; break;
        case GuitarMicProfile::R121_VINTAGE_RIBBON:
            micF0 = 450.0f; micGainDb = 3.0f; micQ = 1.1f; break;
        case GuitarMicProfile::C414_LARGE_CONDENSER:
            micF0 = 3200.0f; micGainDb = 2.5f; micQ = 1.8f; break;
        case GuitarMicProfile::DREADNOUGHT_CHAMBER:
            micF0 = 220.0f; micGainDb = 4.5f; micQ = 2.2f; break;
    }

    float mA = std::pow(10.0f, micGainDb / 40.0f);
    float mW0 = 2.0f * static_cast<float>(M_PI) * (micF0 / m_sampleRate);
    float mAlpha = std::sin(mW0) / (2.0f * micQ);
    float mCos = std::cos(mW0);

    float b0_m = 1.0f + mAlpha * mA;
    float b1_m = -2.0f * mCos;
    float b2_m = 1.0f - mAlpha * mA;
    float a0_m = 1.0f + mAlpha / mA;
    float a1_m = -2.0f * mCos;
    float a2_m = 1.0f - mAlpha / mA;

    m_micB0 = b0_m / a0_m; m_micB1 = b1_m / a0_m; m_micB2 = b2_m / a0_m;
    m_micA1 = a1_m / a0_m; m_micA2 = a2_m / a0_m;
}

void JZAcousticGuitarBodyImpulseConvolverEngine::process(
    const float* inL, const float* inR,
    float* outL, float* outR,
    size_t numSamples) {

    if (!inL || !outL || numSamples == 0) return;
    const bool stereo = (inR != nullptr && outR != nullptr);

    float wet = m_params.dryWet;
    float dry = 1.0f - wet;
    float polarity = m_params.invertPolarity ? -1.0f : 1.0f;
    float antiQuack = m_params.antiQuackAmount;

    float maxReductObserved = 0.0f;

    for (size_t i = 0; i < numSamples; ++i) {
        float sL = inL[i];
        float sR = stereo ? inR[i] : sL;

        // 1. Anti-quack non-linear decompression
        float absLevel = std::max(std::abs(sL), std::abs(sR));
        m_quackEnv = m_quackEnv * 0.992f + absLevel * 0.008f;

        float quackComp = 1.0f;
        if (m_quackEnv > 0.25f && antiQuack > 0.01f) {
            float excess = (m_quackEnv - 0.25f) * antiQuack * 1.5f;
            quackComp = 1.0f / (1.0f + excess);
            float redDb = -20.0f * std::log10(1.0f / quackComp);
            if (redDb < maxReductObserved) maxReductObserved = redDb;
        }

        sL *= quackComp;
        if (stereo) sR *= quackComp;

        // 2. 102 Hz Helmholtz body cavity resonance
        float bodyL = m_bodyB0 * sL + m_bodyB1 * m_bodyX1L + m_bodyB2 * m_bodyX2L
                      - m_bodyA1 * m_bodyY1L - m_bodyA2 * m_bodyY2L;
        m_bodyX2L = m_bodyX1L; m_bodyX1L = sL;
        m_bodyY2L = m_bodyY1L; m_bodyY1L = bodyL;

        float bodyR = sR;
        if (stereo) {
            bodyR = m_bodyB0 * sR + m_bodyB1 * m_bodyX1R + m_bodyB2 * m_bodyX2R
                    - m_bodyA1 * m_bodyY1R - m_bodyA2 * m_bodyY2R;
            m_bodyX2R = m_bodyX1R; m_bodyX1R = sR;
            m_bodyY2R = m_bodyY1R; m_bodyY1R = bodyR;
        }

        // 3. Soundhole anti-feedback notch filter
        float notchL = m_notchB0 * bodyL + m_notchB1 * m_notchX1L + m_notchB2 * m_notchX2L
                       - m_notchA1 * m_notchY1L - m_notchA2 * m_notchY2L;
        m_notchX2L = m_notchX1L; m_notchX1L = bodyL;
        m_notchY2L = m_notchY1L; m_notchY1L = notchL;

        float notchR = bodyR;
        if (stereo) {
            notchR = m_notchB0 * bodyR + m_notchB1 * m_notchX1R + m_notchB2 * m_notchX2R
                     - m_notchA1 * m_notchY1R - m_notchA2 * m_notchY2R;
            m_notchX2R = m_notchX1R; m_notchX1R = bodyR;
            m_notchY2R = m_notchY1R; m_notchY1R = notchR;
        }

        // 4. Studio microphone profile equalization
        float micL = m_micB0 * notchL + m_micB1 * m_micX1L + m_micB2 * m_micX2L
                     - m_micA1 * m_micY1L - m_micA2 * m_micY2L;
        m_micX2L = m_micX1L; m_micX1L = notchL;
        m_micY2L = m_micY1L; m_micY1L = micL;

        float micR = notchR;
        if (stereo) {
            micR = m_micB0 * notchR + m_micB1 * m_micX1R + m_micB2 * m_micX2R
                   - m_micA1 * m_micY1R - m_micA2 * m_micY2R;
            m_micX2R = m_micX1R; m_micX1R = notchR;
            m_micY2R = m_micY1R; m_micY1R = micR;
        }

        // Output mix with polarity control
        outL[i] = (inL[i] * dry + micL * wet) * polarity;
        if (stereo) {
            outR[i] = (inR[i] * dry + micR * wet) * polarity;
        }
    }

    m_antiQuackReductDb = maxReductObserved;
}

} // namespace Jazz
