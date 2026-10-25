//=============================================================================
// GrandPianoSympatheticResonanceEngine.cpp
//
// Master Studio Pro Suite v32.0 - Apex Transcendence Sovereign Millennium Release
// Concert Grand Piano Sympathetic String & Damper Pedal Acoustic Resonance Engine.
//=============================================================================

#include "GrandPianoSympatheticResonanceEngine.h"

namespace Jazz {

static const float kPi = 3.14159265358979323846f;

JZGrandPianoSympatheticResonanceEngine::JZGrandPianoSympatheticResonanceEngine(float sampleRate)
    : mSampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
    , mDamperDown(false)
    , mSympatheticDepth(65.0f)
    , mLinearDepth(0.65f)
    , mSoundboardSize(80.0f)
    , mDuplexLevelDb(-12.0f)
    , mLinearDuplex(0.25f)
    , mActiveStringsCount(0)
    , mResonanceEnergyDb(-60.0f)
{
    for (size_t k = 0; k < NUM_KEYS; ++k) {
        mKeyHeld[k] = false;
    }
    for (int i = 0; i < 4; ++i) {
        mBodyFilterL[i] = mBodyFilterR[i] = 0.0f;
    }
    initModalFrequencies();
    setSympatheticDepth(mSympatheticDepth);
    setDuplexScaleLevelDb(mDuplexLevelDb);
}

void JZGrandPianoSympatheticResonanceEngine::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        mSampleRate = sampleRate;
        initModalFrequencies();
        reset();
    }
}

void JZGrandPianoSympatheticResonanceEngine::setDamperPedal(bool isDown) {
    mDamperDown = isDown;
}

void JZGrandPianoSympatheticResonanceEngine::noteOn(int midiNote, float /*velocity*/) {
    if (midiNote >= 21 && midiNote <= 108) {
        mKeyHeld[midiNote - 21] = true;
    }
}

void JZGrandPianoSympatheticResonanceEngine::noteOff(int midiNote) {
    if (midiNote >= 21 && midiNote <= 108) {
        mKeyHeld[midiNote - 21] = false;
    }
}

void JZGrandPianoSympatheticResonanceEngine::setSympatheticDepth(float percent) {
    mSympatheticDepth = std::max(0.0f, std::min(100.0f, percent));
    mLinearDepth = mSympatheticDepth * 0.01f;
}

void JZGrandPianoSympatheticResonanceEngine::setSoundboardSize(float percent) {
    mSoundboardSize = std::max(10.0f, std::min(100.0f, percent));
}

void JZGrandPianoSympatheticResonanceEngine::setDuplexScaleLevelDb(float db) {
    mDuplexLevelDb = std::max(-36.0f, std::min(0.0f, db));
    mLinearDuplex = std::pow(10.0f, mDuplexLevelDb / 20.0f);
}

void JZGrandPianoSympatheticResonanceEngine::reset() {
    for (size_t k = 0; k < NUM_KEYS; ++k) mKeyHeld[k] = false;
    for (size_t m = 0; m < NUM_MODES; ++m) {
        mResonatorsL[m].reset();
        mResonatorsR[m].reset();
    }
    for (int i = 0; i < 4; ++i) mBodyFilterL[i] = mBodyFilterR[i] = 0.0f;
    mActiveStringsCount = 0;
    mResonanceEnergyDb = -60.0f;
}

void JZGrandPianoSympatheticResonanceEngine::initModalFrequencies() {
    // 16 Harmonic eigenmodes of 9-foot Concert Grand Piano bridge coupling:
    // Fundamentals corresponding to key harmonic nodes (A0, C1, E1, A1, C2, E2, A2, C3, etc.)
    const float baseFrequencies[NUM_MODES] = {
        55.0f, 82.5f, 110.0f, 165.0f, 220.0f, 330.0f, 440.0f, 554.37f,
        659.25f, 880.0f, 1108.7f, 1318.5f, 1760.0f, 2217.4f, 2637.0f, 3520.0f
    };

    for (size_t m = 0; m < NUM_MODES; ++m) {
        float f0 = baseFrequencies[m];
        if (f0 >= mSampleRate * 0.48f) f0 = mSampleRate * 0.48f;

        mResonatorsL[m].f0 = f0;
        mResonatorsR[m].f0 = f0;

        // High Q decay (slower decay on lower strings)
        float decay = 1.0f - (2.5f * f0 / (mSampleRate * 35.0f));
        decay = std::max(0.992f, std::min(0.9997f, decay));

        mResonatorsL[m].decay = decay;
        mResonatorsR[m].decay = decay;
        mResonatorsL[m].reset();
        mResonatorsR[m].reset();
    }
}

void JZGrandPianoSympatheticResonanceEngine::process(const float* inL, const float* inR,
                                                    float* outL, float* outR, size_t numSamples) {
    if (!inL || !inR || !outL || !outR || numSamples == 0) return;

    size_t activeCount = 0;
    if (mDamperDown) {
        activeCount = 88;
    } else {
        for (size_t k = 0; k < NUM_KEYS; ++k) {
            if (mKeyHeld[k]) activeCount++;
        }
    }
    mActiveStringsCount = activeCount;

    // Resonator coupling strength
    float couplingGain = mLinearDepth * 0.08f;
    if (mDamperDown) couplingGain *= 1.4f; // pedal down un-damps entire soundboard

    float totalResEnergy = 0.0f;

    for (size_t i = 0; i < numSamples; ++i) {
        float sL = inL[i];
        float sR = inR[i];
        float bridgeMono = 0.5f * (sL + sR);

        float symL = 0.0f;
        float symR = 0.0f;

        // Modal resonator bank
        for (size_t m = 0; m < NUM_MODES; ++m) {
            float exc = bridgeMono * couplingGain;

            float w0 = 2.0f * kPi * mResonatorsL[m].f0 / mSampleRate;
            float cosW = std::cos(w0);

            // Resonator equation
            ModalResonator& resL = mResonatorsL[m];
            float outModL = exc + resL.decay * (2.0f * cosW * resL.s1 - resL.s2);
            resL.s2 = resL.s1;
            resL.s1 = std::tanh(outModL * 0.5f) * 2.0f; // soft magnetic/bridge saturation

            ModalResonator& resR = mResonatorsR[m];
            float outModR = exc + resR.decay * (2.0f * cosW * resR.s1 - resR.s2);
            resR.s2 = resR.s1;
            resR.s1 = std::tanh(outModR * 0.5f) * 2.0f;

            symL += resL.s1;
            symR += resR.s1;
        }

        // Duplex scale high overtone "singing" sparkle (subtle high-pass excitation)
        float duplexAir = (sL - sR) * mLinearDuplex * 0.15f;

        // Combine direct piano with sympathetic soundboard wash
        float finalL = sL + (symL * 0.15f) + duplexAir;
        float finalR = sR + (symR * 0.15f) - duplexAir;

        outL[i] = finalL;
        outR[i] = finalR;

        float resDiff = 0.5f * (std::abs(finalL - sL) + std::abs(finalR - sR));
        totalResEnergy += resDiff * resDiff;
    }

    float avgResEnergy = totalResEnergy / static_cast<float>(numSamples);
    if (avgResEnergy > 1e-9f) {
        mResonanceEnergyDb = 10.0f * std::log10(avgResEnergy);
    } else {
        mResonanceEnergyDb = -90.0f;
    }
}

} // namespace Jazz
