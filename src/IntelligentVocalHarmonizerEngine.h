//=============================================================================
// IntelligentVocalHarmonizerEngine.h
//
// Master Studio Pro Suite v31.0 - Apex Transcendence Sovereign Pinnacle Release
// Real-Time 4-Voice Intelligent Vocal Harmonizer & Key-Scale Follower Engine.
//
// Features:
// - YIN/autocorrelation fundamental vocal pitch tracking (70 Hz - 1000 Hz)
// - Diatonic key & scale follower (Major, Minor, Dorian, Mixolydian, Lydian, etc.)
// - 4 independent polyphonic vocal harmony voices with formant preservation
// - Independent 3D stereo panning and gain staging per harmony voice
// - Humanized micro-pitch detuning (+/- 12 cents) and vibrato drift
//=============================================================================

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

enum class HarmonizerScaleMode {
    MAJOR = 0,
    NATURAL_MINOR,
    DORIAN,
    MIXOLYDIAN,
    PHRYGIAN,
    LYDIAN,
    HARMONIC_MINOR
};

struct HarmonyVoiceConfig {
    int diatonicIntervalSteps; // e.g. +2 = 3rd above, +4 = 5th above, -2 = 3rd below
    float pan;                 // -1.0 (left) to +1.0 (right)
    float gainDb;              // -24.0 dB to +6.0 dB
    float formantShiftSemitones;
    bool enabled;

    HarmonyVoiceConfig()
        : diatonicIntervalSteps(2)
        , pan(0.0f)
        , gainDb(0.0f)
        , formantShiftSemitones(0.0f)
        , enabled(true)
    {}
};

class JZIntelligentVocalHarmonizerEngine {
public:
    explicit JZIntelligentVocalHarmonizerEngine(float sampleRate = 48000.0f);
    ~JZIntelligentVocalHarmonizerEngine() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return mSampleRate; }

    // Musical Scale Configuration
    void setKeyAndScale(int rootNote, HarmonizerScaleMode mode); // root: 0 = C, 11 = B
    int getRootNote() const { return mRootNote; }
    HarmonizerScaleMode getScaleMode() const { return mScaleMode; }
    std::string getScaleName() const;

    // 4-Voice Configuration
    void setVoiceConfig(size_t voiceIdx, int intervalSteps, float pan, float gainDb, bool enabled);
    const HarmonyVoiceConfig& getVoiceConfig(size_t voiceIdx) const;

    void setDryWet(float wetPercent); // 0.0% to 100.0%
    float getDryWet() const { return mDryWet; }

    void setHumanizeAmount(float percent); // 0.0% to 100.0%
    float getHumanizeAmount() const { return mHumanizeAmount; }

    // Telemetry
    float getDetectedInputPitchHz() const { return mDetectedPitchHz; }
    int getDetectedMidiNote() const { return mDetectedMidiNote; }
    bool isPitchTracked() const { return mPitchConfidence > 0.65f; }

    // Processing
    void process(const float* inL, const float* inR,
                 float* outL, float* outR, size_t numSamples);

    void reset();

private:
    float mSampleRate;
    int mRootNote;
    HarmonizerScaleMode mScaleMode;
    float mDryWet;
    float mHumanizeAmount;

    // Telemetry
    float mDetectedPitchHz;
    int mDetectedMidiNote;
    float mPitchConfidence;

    static constexpr size_t NUM_VOICES = 4;
    HarmonyVoiceConfig mVoices[NUM_VOICES];

    // Pitch shifter grain state per voice
    struct PitchGrainState {
        float phase1;
        float phase2;
        float grainSizeSamples;
        PitchGrainState() : phase1(0.0f), phase2(0.5f), grainSizeSamples(1024.0f) {}
        void reset() { phase1 = 0.0f; phase2 = 0.5f; }
    };
    PitchGrainState mVoiceGrains[NUM_VOICES];

    // Ring buffer for granular pitch shifting
    static constexpr size_t DELAY_BUF_SIZE = 4096;
    std::vector<float> mPitchDelayBuf;
    size_t mWriteIdx;

    // Micro-pitch LFO phases
    float mHumanizeLfoPhase[NUM_VOICES];

    // Pitch detector buffer
    static constexpr size_t PITCH_BUF_SIZE = 1024;
    std::vector<float> mPitchDetectorBuf;
    size_t mPitchWriteIdx;

    void trackPitch();
    int calculateDiatonicSemitones(int inputMidiNote, int diatonicIntervalSteps);
    bool isScaleTone(int pitchClass) const;
};

} // namespace Jazz
