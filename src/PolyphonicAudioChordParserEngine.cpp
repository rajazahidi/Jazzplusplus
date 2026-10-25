//=============================================================================
// PolyphonicAudioChordParserEngine.cpp
//
// Master Studio Pro Suite v30.0 - Apex Colossus Sovereign Diamond Jubilee Release
// Real-Time Polyphonic Audio Chord Parser & Harmonic Voice-Leading Transcriber.
//=============================================================================

#include "PolyphonicAudioChordParserEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

static const char* kPitchNames[12] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

std::string JZPolyphonicAudioChordParserEngine::noteIndexToName(int noteIdx) {
    if (noteIdx < 0 || noteIdx > 11) return "?";
    return kPitchNames[noteIdx];
}

JZPolyphonicAudioChordParserEngine::JZPolyphonicAudioChordParserEngine(float sampleRate)
    : mSampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
    , mSensitivity(0.70f)
    , mMinDurationMs(100.0f)
    , mPendingDurationSamples(0.0f)
    , mChromagram(12, 0.0f)
    , mBassChromagram(12, 0.0f)
{
    for (size_t o = 0; o < NUM_OCTAVES; ++o) {
        for (size_t p = 0; p < NUM_PITCH_CLASSES; ++p) {
            mEnvelopeFollowers[o][p] = 0.0f;
        }
    }
    initFilterBank();
}

void JZPolyphonicAudioChordParserEngine::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        mSampleRate = sampleRate;
        initFilterBank();
        reset();
    }
}

void JZPolyphonicAudioChordParserEngine::setDetectionSensitivity(float sensitivity) {
    mSensitivity = std::max(0.1f, std::min(1.0f, sensitivity));
}

void JZPolyphonicAudioChordParserEngine::setMinChordDurationMs(float ms) {
    mMinDurationMs = std::max(20.0f, std::min(1000.0f, ms));
}

void JZPolyphonicAudioChordParserEngine::reset() {
    mCurrentChord = DetectedChordInfo();
    mPendingChord = DetectedChordInfo();
    mPendingDurationSamples = 0.0f;
    std::fill(mChromagram.begin(), mChromagram.end(), 0.0f);
    std::fill(mBassChromagram.begin(), mBassChromagram.end(), 0.0f);

    for (size_t o = 0; o < NUM_OCTAVES; ++o) {
        for (size_t p = 0; p < NUM_PITCH_CLASSES; ++p) {
            mPitchFilters[o][p].reset();
            mEnvelopeFollowers[o][p] = 0.0f;
        }
    }
}

void JZPolyphonicAudioChordParserEngine::initFilterBank() {
    // 4 octaves starting from C2 (~65.4 Hz) to B5 (~987.8 Hz)
    // MIDI note 36 (C2) = 65.4064 Hz
    // Formula: f = 440 * 2^((midi - 69) / 12)
    const float twoPi = 2.0f * 3.14159265358979323846f;
    const float Q = 25.0f; // High Q bandpass for precise pitch isolation

    for (size_t o = 0; o < NUM_OCTAVES; ++o) {
        for (size_t p = 0; p < NUM_PITCH_CLASSES; ++p) {
            int midiNote = 36 + (int)(o * 12 + p);
            float freqHz = 440.0f * std::pow(2.0f, (float)(midiNote - 69) / 12.0f);

            // 2nd-order resonant bandpass biquad
            float w0 = twoPi * freqHz / mSampleRate;
            float alpha = std::sin(w0) / (2.0f * Q);

            float b0 = alpha;
            float b1 = 0.0f;
            float b2 = -alpha;
            float a0 = 1.0f + alpha;
            float a1 = -2.0f * std::cos(w0);
            float a2 = 1.0f - alpha;

            BandpassFilter& f = mPitchFilters[o][p];
            f.b0 = b0 / a0;
            f.b1 = b1 / a0;
            f.b2 = b2 / a0;
            f.a1 = a1 / a0;
            f.a2 = a2 / a0;
            f.reset();
        }
    }
}

void JZPolyphonicAudioChordParserEngine::process(const float* inL, const float* inR, size_t numSamples) {
    if (!inL || !inR || numSamples == 0) return;

    const float decayCoeff = 0.9992f;
    const float attackCoeff = 0.005f;

    for (size_t i = 0; i < numSamples; ++i) {
        float monoSample = 0.5f * (inL[i] + inR[i]);

        // Process through 4 octaves of 12 chromatic filters
        for (size_t o = 0; o < NUM_OCTAVES; ++o) {
            for (size_t p = 0; p < NUM_PITCH_CLASSES; ++p) {
                float filtOut = mPitchFilters[o][p].process(monoSample);
                float absVal = std::abs(filtOut);

                // Attack / decay envelope tracker
                if (absVal > mEnvelopeFollowers[o][p]) {
                    mEnvelopeFollowers[o][p] += attackCoeff * (absVal - mEnvelopeFollowers[o][p]);
                } else {
                    mEnvelopeFollowers[o][p] *= decayCoeff;
                }
            }
        }
    }

    analyzeChromaAndChords(numSamples);
}

void JZPolyphonicAudioChordParserEngine::analyzeChromaAndChords(size_t numSamples) {
    // Fold 4 octaves into 12-bin Pitch Class Profile (Chromagram)
    std::fill(mChromagram.begin(), mChromagram.end(), 0.0f);
    std::fill(mBassChromagram.begin(), mBassChromagram.end(), 0.0f);

    float maxEnergy = 0.0f;
    for (size_t p = 0; p < NUM_PITCH_CLASSES; ++p) {
        float chromaSum = 0.0f;
        for (size_t o = 0; o < NUM_OCTAVES; ++o) {
            float weight = (o == 0) ? 1.4f : 1.0f; // Give fundamental bass octave higher weighting
            chromaSum += mEnvelopeFollowers[o][p] * weight;
        }
        mChromagram[p] = chromaSum;
        mBassChromagram[p] = mEnvelopeFollowers[0][p]; // Octave 0 is bass line
        maxEnergy = std::max(maxEnergy, chromaSum);
    }

    // Normalize chromagram
    if (maxEnergy > 1e-5f) {
        for (size_t p = 0; p < NUM_PITCH_CLASSES; ++p) {
            mChromagram[p] /= maxEnergy;
        }
    } else {
        // Silence or too weak
        mCurrentChord = DetectedChordInfo();
        return;
    }

    // Determine bass note (lowest significant octave energy)
    int detectedBass = -1;
    float maxBassEnergy = 0.0f;
    for (size_t p = 0; p < NUM_PITCH_CLASSES; ++p) {
        if (mBassChromagram[p] > maxBassEnergy && mBassChromagram[p] > 0.1f * maxEnergy) {
            maxBassEnergy = mBassChromagram[p];
            detectedBass = (int)p;
        }
    }

    classifyChordTemplate(mChromagram, detectedBass);

    // Apply temporal hysteresis (min chord duration)
    float minDurationSamples = (mMinDurationMs * 0.001f) * mSampleRate;
    if (mPendingChord.chordType == mCurrentChord.chordType &&
        mPendingChord.rootNote == mCurrentChord.rootNote) {
        mPendingDurationSamples += (float)numSamples;
    } else {
        mPendingChord = mCurrentChord;
        mPendingDurationSamples = (float)numSamples;
    }

    if (mPendingDurationSamples >= minDurationSamples) {
        // Confirmed stable chord
        mCurrentChord.voicingNotesMidi = generateVoicingMidi(mCurrentChord.rootNote,
                                                            mCurrentChord.chordType,
                                                            mCurrentChord.bassNote);
    }
}

void JZPolyphonicAudioChordParserEngine::classifyChordTemplate(const std::vector<float>& chroma, int bassNote) {
    // Template matching against 9 harmonic chord profiles
    struct ChordPattern {
        AudioChordType type;
        const char* suffix;
        std::vector<int> intervals;
    };

    static const ChordPattern kPatterns[] = {
        { AudioChordType::MAJOR,        "",      {0, 4, 7} },
        { AudioChordType::MINOR,        "m",     {0, 3, 7} },
        { AudioChordType::DOMINANT_7TH, "7",     {0, 4, 7, 10} },
        { AudioChordType::MAJOR_7TH,    "maj7",  {0, 4, 7, 11} },
        { AudioChordType::MINOR_7TH,    "m7",    {0, 3, 7, 10} },
        { AudioChordType::MINOR_9TH,    "m9",    {0, 3, 7, 10, 2} },
        { AudioChordType::DIMINISHED,   "dim",   {0, 3, 6} },
        { AudioChordType::AUGMENTED,    "aug",   {0, 4, 8} },
        { AudioChordType::SUS4,         "sus4",  {0, 5, 7} }
    };
    const size_t numPatterns = sizeof(kPatterns) / sizeof(kPatterns[0]);

    float bestScore = -1.0f;
    int bestRoot = -1;
    AudioChordType bestType = AudioChordType::NONE;
    std::string bestName = "N.C.";

    for (int root = 0; root < 12; ++root) {
        for (size_t pi = 0; pi < numPatterns; ++pi) {
            const ChordPattern& pat = kPatterns[pi];
            float matchScore = 0.0f;
            float nonMatchPenalty = 0.0f;

            std::vector<bool> isInChord(12, false);
            for (int interval : pat.intervals) {
                int pitch = (root + interval) % 12;
                isInChord[pitch] = true;
                matchScore += chroma[pitch];
            }

            // Penalize notes present in chromagram that are NOT in the chord template
            for (int pitch = 0; pitch < 12; ++pitch) {
                if (!isInChord[pitch]) {
                    nonMatchPenalty += chroma[pitch] * 0.45f;
                }
            }

            // Normalization by number of notes in template
            float finalScore = (matchScore - nonMatchPenalty) / (float)pat.intervals.size();

            // Bonus if bass note matches root
            if (bassNote == root) {
                finalScore += 0.12f;
            }

            if (finalScore > bestScore) {
                bestScore = finalScore;
                bestRoot = root;
                bestType = pat.type;
                std::string chordStr = std::string(kPitchNames[root]) + pat.suffix;
                if (bassNote >= 0 && bassNote != root) {
                    chordStr += "/" + std::string(kPitchNames[bassNote]);
                }
                bestName = chordStr;
            }
        }
    }

    float threshold = 0.35f * (1.1f - mSensitivity);
    if (bestScore > threshold && bestRoot >= 0) {
        mCurrentChord.rootNote = bestRoot;
        mCurrentChord.chordType = bestType;
        mCurrentChord.bassNote = (bassNote >= 0 && bassNote != bestRoot) ? bassNote : -1;
        mCurrentChord.chordName = bestName;
        mCurrentChord.confidence = std::min(1.0f, std::max(0.0f, bestScore));
    } else {
        mCurrentChord.rootNote = -1;
        mCurrentChord.chordType = AudioChordType::NONE;
        mCurrentChord.bassNote = -1;
        mCurrentChord.chordName = "N.C.";
        mCurrentChord.confidence = 0.0f;
        mCurrentChord.voicingNotesMidi.clear();
    }
}

std::vector<int> JZPolyphonicAudioChordParserEngine::generateVoicingMidi(int root, AudioChordType type, int bass) {
    std::vector<int> midiVoicing;
    if (root < 0 || type == AudioChordType::NONE) return midiVoicing;

    // Bass note in octave 2 (MIDI 36 to 47)
    int bassMidi = (bass >= 0) ? (36 + bass) : (36 + root);
    midiVoicing.push_back(bassMidi);

    // Root note in octave 3 or 4 (e.g. C4 = 60)
    int rootMidi = 60 + root;

    switch (type) {
        case AudioChordType::MAJOR:
            midiVoicing.push_back(rootMidi);
            midiVoicing.push_back(rootMidi + 4);
            midiVoicing.push_back(rootMidi + 7);
            break;
        case AudioChordType::MINOR:
            midiVoicing.push_back(rootMidi);
            midiVoicing.push_back(rootMidi + 3);
            midiVoicing.push_back(rootMidi + 7);
            break;
        case AudioChordType::DOMINANT_7TH:
            midiVoicing.push_back(rootMidi);
            midiVoicing.push_back(rootMidi + 4);
            midiVoicing.push_back(rootMidi + 7);
            midiVoicing.push_back(rootMidi + 10);
            break;
        case AudioChordType::MAJOR_7TH:
            midiVoicing.push_back(rootMidi);
            midiVoicing.push_back(rootMidi + 4);
            midiVoicing.push_back(rootMidi + 7);
            midiVoicing.push_back(rootMidi + 11);
            break;
        case AudioChordType::MINOR_7TH:
            midiVoicing.push_back(rootMidi);
            midiVoicing.push_back(rootMidi + 3);
            midiVoicing.push_back(rootMidi + 7);
            midiVoicing.push_back(rootMidi + 10);
            break;
        case AudioChordType::MINOR_9TH:
            midiVoicing.push_back(rootMidi);
            midiVoicing.push_back(rootMidi + 3);
            midiVoicing.push_back(rootMidi + 7);
            midiVoicing.push_back(rootMidi + 10);
            midiVoicing.push_back(rootMidi + 14);
            break;
        case AudioChordType::DIMINISHED:
            midiVoicing.push_back(rootMidi);
            midiVoicing.push_back(rootMidi + 3);
            midiVoicing.push_back(rootMidi + 6);
            break;
        case AudioChordType::AUGMENTED:
            midiVoicing.push_back(rootMidi);
            midiVoicing.push_back(rootMidi + 4);
            midiVoicing.push_back(rootMidi + 8);
            break;
        case AudioChordType::SUS4:
            midiVoicing.push_back(rootMidi);
            midiVoicing.push_back(rootMidi + 5);
            midiVoicing.push_back(rootMidi + 7);
            break;
        default:
            break;
    }

    return midiVoicing;
}

} // namespace Jazz
