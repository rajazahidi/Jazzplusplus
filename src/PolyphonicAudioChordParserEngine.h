//=============================================================================
// PolyphonicAudioChordParserEngine.h
//
// Master Studio Pro Suite v30.0 - Apex Colossus Sovereign Diamond Jubilee Release
// Real-Time Polyphonic Audio Chord Parser & Harmonic Voice-Leading Transcriber.
//
// Features:
// - 12-bin Pitch Class Profile (PCP) / Chromagram extraction (C through B)
// - Harmonic template correlation matching (Major, Minor, Dominant 7th, Maj7,
//   Min7, Diminished, Augmented, Sus4, Minor 9th)
// - Inversion & Bass note detection (e.g. C/E, G/B, Dm/F)
// - Polyphonic MIDI voicing generation with pitch allocation
// - Temporal hysteresis to prevent transient chord jitter
// - Audio confidence scoring (0.0 to 1.0)
//=============================================================================

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace Jazz {

enum class AudioChordType {
    NONE = 0,
    MAJOR,
    MINOR,
    DOMINANT_7TH,
    MAJOR_7TH,
    MINOR_7TH,
    DIMINISHED,
    AUGMENTED,
    SUS4,
    MINOR_9TH
};

struct DetectedChordInfo {
    int rootNote;             // 0 = C, 1 = C#, ..., 11 = B (-1 if none)
    AudioChordType chordType; // Chord quality
    int bassNote;             // 0 = C, ..., 11 = B (-1 if root)
    std::string chordName;    // e.g. "Cmaj7", "Am", "G7/B"
    float confidence;         // 0.0 to 1.0
    std::vector<int> voicingNotesMidi; // MIDI pitch numbers e.g. [60, 64, 67, 71]

    DetectedChordInfo()
        : rootNote(-1)
        , chordType(AudioChordType::NONE)
        , bassNote(-1)
        , chordName("N.C.")
        , confidence(0.0f)
    {}
};

class JZPolyphonicAudioChordParserEngine {
public:
    explicit JZPolyphonicAudioChordParserEngine(float sampleRate = 48000.0f);
    ~JZPolyphonicAudioChordParserEngine() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return mSampleRate; }

    // Parser Configuration
    void setDetectionSensitivity(float sensitivity); // 0.1 to 1.0
    float getDetectionSensitivity() const { return mSensitivity; }

    void setMinChordDurationMs(float ms);            // 50 ms to 500 ms
    float getMinChordDurationMs() const { return mMinDurationMs; }

    // Audio Analysis
    void process(const float* inL, const float* inR, size_t numSamples);
    void reset();

    // Query Results
    const DetectedChordInfo& getCurrentChord() const { return mCurrentChord; }
    std::string getCurrentChordName() const { return mCurrentChord.chordName; }
    int getCurrentRootNote() const { return mCurrentChord.rootNote; }
    AudioChordType getCurrentChordType() const { return mCurrentChord.chordType; }
    float getConfidence() const { return mCurrentChord.confidence; }
    const std::vector<int>& getVoicingMidiNotes() const { return mCurrentChord.voicingNotesMidi; }

    // Raw Chromagram (12 bins: C, C#, D, D#, E, F, F#, G, G#, A, A#, B)
    const std::vector<float>& getChromagram() const { return mChromagram; }

    static std::string noteIndexToName(int noteIdx);

private:
    float mSampleRate;
    float mSensitivity;
    float mMinDurationMs;

    DetectedChordInfo mCurrentChord;
    DetectedChordInfo mPendingChord;
    float mPendingDurationSamples;

    // 12-bin Pitch Class Profile / Chromagram
    std::vector<float> mChromagram;
    std::vector<float> mBassChromagram;

    // Filter banks for chromatic frequency tracking (65 Hz to 1046 Hz, 4 octaves)
    static constexpr size_t NUM_OCTAVES = 4;
    static constexpr size_t NUM_PITCH_CLASSES = 12;
    struct BandpassFilter {
        float b0, b1, b2, a1, a2;
        float x1, x2, y1, y2;
        BandpassFilter() : b0(0), b1(0), b2(0), a1(0), a2(0), x1(0), x2(0), y1(0), y2(0) {}
        void reset() { x1 = x2 = y1 = y2 = 0.0f; }
        float process(float in) {
            float out = b0 * in + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = in;
            y2 = y1; y1 = out;
            return out;
        }
    };
    BandpassFilter mPitchFilters[NUM_OCTAVES][NUM_PITCH_CLASSES];
    float mEnvelopeFollowers[NUM_OCTAVES][NUM_PITCH_CLASSES];

    void initFilterBank();
    void analyzeChromaAndChords(size_t numSamples);
    void classifyChordTemplate(const std::vector<float>& chroma, int bassNote);
    std::vector<int> generateVoicingMidi(int root, AudioChordType type, int bass);
};

} // namespace Jazz
