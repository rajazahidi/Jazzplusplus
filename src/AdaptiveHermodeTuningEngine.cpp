//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZAdaptiveHermodeTuningEngine.cpp - Dynamic Hermode Tuning & Just Intonation
//*****************************************************************************

#include "AdaptiveHermodeTuningEngine.h"
#include <algorithm>
#include <cmath>

namespace Jazz {

JZAdaptiveHermodeTuningEngine::JZAdaptiveHermodeTuningEngine(TemperamentMode mode, float concertPitchA4)
    : m_mode(mode)
    , m_concertPitchA4(concertPitchA4 > 400.0f ? concertPitchA4 : 440.0f)
    , m_tuningDepth(1.0f) // 100% Hermode Just Intonation by default
    , m_detectedRoot(-1)
    , m_detectedChord("None")
{
}

void JZAdaptiveHermodeTuningEngine::setTemperamentMode(TemperamentMode mode) {
    m_mode = mode;
    analyzeChordAndCalculateOffsets();
}

void JZAdaptiveHermodeTuningEngine::setConcertPitchA4(float freqHz) {
    if (freqHz > 400.0f && freqHz < 480.0f) {
        m_concertPitchA4 = freqHz;
    }
}

void JZAdaptiveHermodeTuningEngine::setTuningDepth(float depth) {
    m_tuningDepth = std::clamp(depth, 0.0f, 1.0f);
    analyzeChordAndCalculateOffsets();
}

void JZAdaptiveHermodeTuningEngine::noteOn(uint8_t note, uint8_t velocity) {
    if (velocity > 0) {
        m_activeNotes.insert(note);
    } else {
        m_activeNotes.erase(note);
    }
    analyzeChordAndCalculateOffsets();
}

void JZAdaptiveHermodeTuningEngine::noteOff(uint8_t note) {
    m_activeNotes.erase(note);
    analyzeChordAndCalculateOffsets();
}

void JZAdaptiveHermodeTuningEngine::allNotesOff() {
    m_activeNotes.clear();
    m_detectedRoot = -1;
    m_detectedChord = "None";
    m_calculatedOffsets.clear();
}

void JZAdaptiveHermodeTuningEngine::reset() {
    allNotesOff();
}

void JZAdaptiveHermodeTuningEngine::analyzeChordAndCalculateOffsets() {
    m_calculatedOffsets.clear();

    if (m_mode == TemperamentMode::EQUAL_TEMPERAMENT_12 || m_activeNotes.empty()) {
        m_detectedChord = (m_activeNotes.empty()) ? "None" : "12-TET";
        return;
    }

    if (m_mode == TemperamentMode::EDO_19 || m_mode == TemperamentMode::EDO_31 || m_mode == TemperamentMode::EDO_53) {
        m_detectedChord = "Microtonal EDO";
        return;
    }

    // Lowest note acts as harmonic bass root candidate
    uint8_t lowestNote = *m_activeNotes.begin();
    int rootPitchClass = lowestNote % 12;
    m_detectedRoot = rootPitchClass;

    // Detect intervals relative to bass root
    std::set<int> pitchClasses;
    for (uint8_t n : m_activeNotes) {
        pitchClasses.insert((n - rootPitchClass + 120) % 12);
    }

    bool hasMajor3 = pitchClasses.count(4) > 0;
    bool hasMinor3 = pitchClasses.count(3) > 0;
    bool hasFifth = pitchClasses.count(7) > 0;
    bool hasDominant7 = pitchClasses.count(10) > 0;

    static const char* noteNames[12] = {"C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"};
    std::string rootName = noteNames[rootPitchClass];

    if (hasMajor3 && hasFifth) {
        m_detectedChord = rootName + " Major" + (hasDominant7 ? "7" : "");
    } else if (hasMinor3 && hasFifth) {
        m_detectedChord = rootName + " Minor";
    } else {
        m_detectedChord = rootName + " Dyad/Chord";
    }

    // Apply Hermode Just Intonation cent offsets
    for (uint8_t n : m_activeNotes) {
        int interval = (n - rootPitchClass + 120) % 12;
        float centOffset = 0.0f;

        switch (interval) {
            case 0:  // Root / Octave
                centOffset = 0.0f;
                break;
            case 4:  // Pure Major Third (5/4 ratio: 386.3 cents vs 400 cents in 12TET)
                centOffset = -13.69f;
                break;
            case 3:  // Pure Minor Third (6/5 ratio: 315.6 cents vs 300 cents in 12TET)
                centOffset = +15.64f;
                break;
            case 7:  // Pure Fifth (3/2 ratio: 702.0 cents vs 700 cents in 12TET)
                centOffset = +1.96f;
                break;
            case 10: // Harmonic Seventh (7/4 ratio: 968.8 cents vs 1000 cents in 12TET)
                centOffset = -31.17f;
                break;
            case 2:  // Major Second (9/8 ratio)
                centOffset = +3.91f;
                break;
            case 5:  // Pure Fourth (4/3 ratio)
                centOffset = -1.96f;
                break;
            default:
                centOffset = 0.0f;
                break;
        }

        m_calculatedOffsets[n] = centOffset * m_tuningDepth;
    }
}

float JZAdaptiveHermodeTuningEngine::getPitchOffsetCents(uint8_t note) const {
    if (m_mode == TemperamentMode::EQUAL_TEMPERAMENT_12) return 0.0f;

    if (m_mode == TemperamentMode::EDO_19) {
        // Approximate 12-TET note into 19-EDO step difference
        int pc = note % 12;
        static const float edo19Offsets[12] = {0.0f, -36.8f, -26.3f, +10.5f, +21.1f, -15.8f, -5.3f, -5.3f, -42.1f, -31.6f, +5.3f, +15.8f};
        return edo19Offsets[pc];
    }
    if (m_mode == TemperamentMode::EDO_31) {
        int pc = note % 12;
        static const float edo31Offsets[12] = {0.0f, -19.4f, -3.2f, +12.9f, -12.9f, +3.2f, -16.1f, +1.6f, -17.7f, -1.6f, +14.5f, -11.3f};
        return edo31Offsets[pc];
    }

    auto it = m_calculatedOffsets.find(note);
    if (it != m_calculatedOffsets.end()) {
        return it->second;
    }
    return 0.0f;
}

double JZAdaptiveHermodeTuningEngine::noteToFrequencyHz(uint8_t note) const {
    double baseSemitones = static_cast<double>(note) - 69.0; // Distance from A4 (MIDI 69)
    float cents = getPitchOffsetCents(note);
    double totalSemitones = baseSemitones + (static_cast<double>(cents) / 100.0);
    return static_cast<double>(m_concertPitchA4) * std::pow(2.0, totalSemitones / 12.0);
}

} // namespace Jazz
