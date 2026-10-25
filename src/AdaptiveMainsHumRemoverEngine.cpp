//=============================================================================
// AdaptiveMainsHumRemoverEngine.cpp
//
// Master Studio Pro Suite v32.0 - Apex Transcendence Sovereign Millennium Release
// Adaptive Mains Hum (50/60Hz) & Electromagnetic Ground-Loop Harmonic De-Hummer.
//=============================================================================

#include "AdaptiveMainsHumRemoverEngine.h"

namespace Jazz {

static const float kPi = 3.14159265358979323846f;

JZAdaptiveMainsHumRemoverEngine::JZAdaptiveMainsHumRemoverEngine(float sampleRate)
    : mSampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
    , mGrid(GridFrequency::GRID_50HZ)
    , mDepthDb(-36.0f)
    , mLinearDepth(0.0158f)
    , mNotchQ(50.0f)
    , mHarmonicCount(12)
    , mHumSolo(false)
    , mTrackedFreqHz(50.0f)
    , mDetectedHumDb(-60.0f)
    , mIsLocked(true)
{
    updateNotchCoefficients();
}

void JZAdaptiveMainsHumRemoverEngine::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        mSampleRate = sampleRate;
        updateNotchCoefficients();
        reset();
    }
}

void JZAdaptiveMainsHumRemoverEngine::setGridFrequency(GridFrequency grid) {
    mGrid = grid;
    mTrackedFreqHz = (mGrid == GridFrequency::GRID_50HZ) ? 50.0f : 60.0f;
    updateNotchCoefficients();
}

std::string JZAdaptiveMainsHumRemoverEngine::getGridFrequencyName() const {
    return (mGrid == GridFrequency::GRID_50HZ) ? "50 Hz (Europe / UK / Asia)" : "60 Hz (Americas / Japan)";
}

void JZAdaptiveMainsHumRemoverEngine::setSuppressionDepthDb(float db) {
    mDepthDb = std::max(-60.0f, std::min(-6.0f, db));
    mLinearDepth = std::pow(10.0f, mDepthDb / 20.0f);
    updateNotchCoefficients();
}

void JZAdaptiveMainsHumRemoverEngine::setNotchQ(float q) {
    mNotchQ = std::max(10.0f, std::min(120.0f, q));
    updateNotchCoefficients();
}

void JZAdaptiveMainsHumRemoverEngine::setHarmonicCount(size_t count) {
    mHarmonicCount = std::max<size_t>(1, std::min<size_t>(MAX_HARMONICS, count));
    updateNotchCoefficients();
}

void JZAdaptiveMainsHumRemoverEngine::reset() {
    for (size_t h = 0; h < MAX_HARMONICS; ++h) {
        mNotchesL[h].reset();
        mNotchesR[h].reset();
    }
    mDetectedHumDb = -60.0f;
}

void JZAdaptiveMainsHumRemoverEngine::updateNotchCoefficients() {
    float baseF = (mGrid == GridFrequency::GRID_50HZ) ? 50.0f : 60.0f;

    for (size_t h = 0; h < MAX_HARMONICS; ++h) {
        float f0 = baseF * static_cast<float>(h + 1);

        if (f0 >= mSampleRate * 0.48f || h >= mHarmonicCount) {
            // Bypass filter
            mNotchesL[h].b0 = 1.0f; mNotchesL[h].b1 = 0.0f; mNotchesL[h].b2 = 0.0f;
            mNotchesL[h].a1 = 0.0f; mNotchesL[h].a2 = 0.0f;
            mNotchesR[h] = mNotchesL[h];
            continue;
        }

        // Notch filter with depth
        float w0 = 2.0f * kPi * f0 / mSampleRate;
        float A = std::pow(10.0f, mDepthDb / 40.0f);
        float alpha = std::sin(w0) / (2.0f * mNotchQ);

        float b0 = 1.0f + alpha * A;
        float b1 = -2.0f * std::cos(w0);
        float b2 = 1.0f - alpha * A;
        float a0 = 1.0f + alpha / A;
        float a1 = -2.0f * std::cos(w0);
        float a2 = 1.0f - alpha / A;

        auto setup = [&](NotchBiquad& f) {
            f.b0 = b0 / a0;
            f.b1 = b1 / a0;
            f.b2 = b2 / a0;
            f.a1 = a1 / a0;
            f.a2 = a2 / a0;
        };

        setup(mNotchesL[h]);
        setup(mNotchesR[h]);
    }
}

void JZAdaptiveMainsHumRemoverEngine::process(const float* inL, const float* inR,
                                            float* outL, float* outR, size_t numSamples) {
    if (!inL || !inR || !outL || !outR || numSamples == 0) return;

    float humEnergySum = 0.0f;

    for (size_t i = 0; i < numSamples; ++i) {
        float sL = inL[i];
        float sR = inR[i];

        float notchedL = sL;
        float notchedR = sR;

        // Cascade through active harmonic notches
        for (size_t h = 0; h < mHarmonicCount; ++h) {
            notchedL = mNotchesL[h].process(notchedL);
            notchedR = mNotchesR[h].process(notchedR);
        }

        // Output mode
        if (!mHumSolo) {
            outL[i] = notchedL;
            outR[i] = notchedR;
        } else {
            // Hum solo: output stripped mains hum component
            outL[i] = sL - notchedL;
            outR[i] = sR - notchedR;
        }

        float diff = 0.5f * (std::abs(sL - notchedL) + std::abs(sR - notchedR));
        humEnergySum += diff * diff;
    }

    float avgHumEnergy = humEnergySum / static_cast<float>(numSamples);
    if (avgHumEnergy > 1e-9f) {
        mDetectedHumDb = 10.0f * std::log10(avgHumEnergy);
    } else {
        mDetectedHumDb = -90.0f;
    }
}

} // namespace Jazz
