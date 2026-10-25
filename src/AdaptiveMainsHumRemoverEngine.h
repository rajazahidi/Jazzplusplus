//=============================================================================
// AdaptiveMainsHumRemoverEngine.h
//
// Master Studio Pro Suite v32.0 - Apex Transcendence Sovereign Millennium Release
// Adaptive Mains Hum (50/60Hz) & Electromagnetic Ground-Loop Harmonic De-Hummer.
//
// Features:
// - Software Phase-Locked Loop (PLL) tracking electrical grid frequency drift (+/- 2 Hz)
// - Array of 16 phase-coherent surgical notch filters tracking all harmonics
// - Independent odd-harmonic (transformer core) and even-harmonic (rectifier) modeling
// - Surgical high-Q notches (Q = 20 to 100) preserving natural musical bass
// - Hum Delta Solo audition mode
//=============================================================================

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

enum class GridFrequency {
    GRID_50HZ = 0, // Europe / Asia / UK / Africa
    GRID_60HZ      // Americas / Taiwan / Japan (parts)
};

class JZAdaptiveMainsHumRemoverEngine {
public:
    explicit JZAdaptiveMainsHumRemoverEngine(float sampleRate = 48000.0f);
    ~JZAdaptiveMainsHumRemoverEngine() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return mSampleRate; }

    // Configuration
    void setGridFrequency(GridFrequency grid);
    GridFrequency getGridFrequency() const { return mGrid; }
    std::string getGridFrequencyName() const;

    void setSuppressionDepthDb(float db);        // -6.0 dB to -60.0 dB
    float getSuppressionDepthDb() const { return mDepthDb; }

    void setNotchQ(float q);                     // 20.0 to 100.0 (nominally 50.0)
    float getNotchQ() const { return mNotchQ; }

    void setHarmonicCount(size_t count);         // 1 to 16 harmonics
    size_t getHarmonicCount() const { return mHarmonicCount; }

    void setHumSolo(bool solo) { mHumSolo = solo; }
    bool isHumSolo() const { return mHumSolo; }

    // Telemetry
    float getTrackedGridFrequencyHz() const { return mTrackedFreqHz; }
    float getDetectedHumLevelDb() const { return mDetectedHumDb; }
    bool isHumLocked() const { return mIsLocked; }

    // Audio Processing
    void process(const float* inL, const float* inR,
                 float* outL, float* outR, size_t numSamples);

    void reset();

private:
    float mSampleRate;
    GridFrequency mGrid;
    float mDepthDb;
    float mLinearDepth;
    float mNotchQ;
    size_t mHarmonicCount;
    bool  mHumSolo;

    // Telemetry
    float mTrackedFreqHz;
    float mDetectedHumDb;
    bool  mIsLocked;

    static constexpr size_t MAX_HARMONICS = 16;

    // Notch biquad filters per harmonic
    struct NotchBiquad {
        float b0, b1, b2, a1, a2;
        float x1, x2, y1, y2;
        NotchBiquad() : b0(1), b1(0), b2(0), a1(0), a2(0), x1(0), x2(0), y1(0), y2(0) {}
        void reset() { x1 = x2 = y1 = y2 = 0.0f; }
        float process(float in) {
            float out = b0 * in + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = in;
            y2 = y1; y1 = out;
            return out;
        }
    };

    NotchBiquad mNotchesL[MAX_HARMONICS];
    NotchBiquad mNotchesR[MAX_HARMONICS];

    void updateNotchCoefficients();
};

} // namespace Jazz
