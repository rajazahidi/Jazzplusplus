//=============================================================================
// GrandPianoSympatheticResonanceEngine.h
//
// Master Studio Pro Suite v32.0 - Apex Transcendence Sovereign Millennium Release
// Concert Grand Piano Sympathetic String & Damper Pedal Acoustic Resonance Engine.
//
// Features:
// - Physical 88-string bridge cross-coupling matrix (A0 to C8)
// - Damper pedal (MIDI CC 64) full-string un-damped harmonic wash
// - Harmonic ratio resonant excitation (octaves, fifths, fourths, major thirds)
// - Duplex scale inharmonic treble singing overtone synthesis
// - Spruce soundboard acoustic radiation & mechanical damper pedal noise
//=============================================================================

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

class JZGrandPianoSympatheticResonanceEngine {
public:
    explicit JZGrandPianoSympatheticResonanceEngine(float sampleRate = 48000.0f);
    ~JZGrandPianoSympatheticResonanceEngine() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return mSampleRate; }

    // Performance Controls
    void setDamperPedal(bool isDown);
    bool isDamperPedalDown() const { return mDamperDown; }

    void noteOn(int midiNote, float velocity);
    void noteOff(int midiNote);

    // Acoustic Resonance Parameters
    void setSympatheticDepth(float percent);     // 0.0% to 100.0% (nominally 65%)
    float getSympatheticDepth() const { return mSympatheticDepth; }

    void setSoundboardSize(float percent);       // 0.0% to 100.0% (nominally 80%)
    float getSoundboardSize() const { return mSoundboardSize; }

    void setDuplexScaleLevelDb(float db);        // -24.0 dB to 0.0 dB
    float getDuplexScaleLevelDb() const { return mDuplexLevelDb; }

    // Telemetry
    size_t getActiveResonatingStringsCount() const { return mActiveStringsCount; }
    float getResonanceEnergyDb() const { return mResonanceEnergyDb; }

    // Audio Processing
    void process(const float* inL, const float* inR,
                 float* outL, float* outR, size_t numSamples);

    void reset();

private:
    float mSampleRate;
    bool  mDamperDown;
    float mSympatheticDepth;
    float mLinearDepth;
    float mSoundboardSize;
    float mDuplexLevelDb;
    float mLinearDuplex;

    // Telemetry
    size_t mActiveStringsCount;
    float  mResonanceEnergyDb;

    // 88 Piano keys (MIDI 21 to 108)
    static constexpr size_t NUM_KEYS = 88;
    bool mKeyHeld[NUM_KEYS];

    // Modal resonators for sympathetic string coupling (Bank of 16 active modes)
    static constexpr size_t NUM_MODES = 16;
    struct ModalResonator {
        float f0;
        float decay;
        float s1, s2;
        ModalResonator() : f0(440.0f), decay(0.999f), s1(0.0f), s2(0.0f) {}
        void reset() { s1 = s2 = 0.0f; }
        float process(float in) {
            float out = in + decay * (1.995f * s1 - s2);
            s2 = s1;
            s1 = out;
            return out;
        }
    };

    ModalResonator mResonatorsL[NUM_MODES];
    ModalResonator mResonatorsR[NUM_MODES];

    // Soundboard radiation biquad filter (spruce body resonance)
    float mBodyFilterL[4];
    float mBodyFilterR[4];

    void initModalFrequencies();
};

} // namespace Jazz
