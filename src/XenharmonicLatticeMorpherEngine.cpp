//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// XenharmonicLatticeMorpherEngine.cpp - Dynamic Xenharmonic Lattice & Scale Morpher
//*****************************************************************************

#include "XenharmonicLatticeMorpherEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

JZXenharmonicLatticeMorpherEngine::JZXenharmonicLatticeMorpherEngine(float rootPitchHz)
    : m_rootPitchHz(rootPitchHz) {
    if (m_rootPitchHz < 20.0f) m_rootPitchHz = 440.0f;
}

void JZXenharmonicLatticeMorpherEngine::setRootPitchHz(float hz) {
    if (hz > 20.0f && hz < 20000.0f) {
        m_rootPitchHz = hz;
    }
}

void JZXenharmonicLatticeMorpherEngine::setSourceTuning(XenharmonicTuning tuning) {
    m_sourceTuning = tuning;
}

void JZXenharmonicLatticeMorpherEngine::setTargetTuning(XenharmonicTuning tuning) {
    m_targetTuning = tuning;
}

void JZXenharmonicLatticeMorpherEngine::setMorphFactor(float morph) {
    m_morphFactor = std::max(0.0f, std::min(1.0f, morph));
}

float JZXenharmonicLatticeMorpherEngine::calculateTuningCents(XenharmonicTuning tuning, int midiNote) const {
    int noteOffset = midiNote - 69; // Offset from A4 (69)

    switch (tuning) {
        case XenharmonicTuning::TET_12_STANDARD:
            return static_cast<float>(noteOffset) * 100.0f;

        case XenharmonicTuning::BOHLEN_PIERCE_TRITAVE: {
            // Tritave ratio 3:1 (1901.955 cents) divided into 13 equal steps of 146.304 cents
            const float bpStepCents = 1901.955f / 13.0f;
            return static_cast<float>(noteOffset) * bpStepCents;
        }

        case XenharmonicTuning::PARTCH_43_TONE_DIAMOND: {
            // 11-Limit Just Intonation mapped to 12 chromatic degrees relative to A4
            // Just intervals from A:
            // A=0c (1/1), Bb=111.7c (16/15), B=203.9c (9/8), C=315.6c (6/5), C#=386.3c (5/4),
            // D=498.0c (4/3), D#=582.5c (7/5), E=702.0c (3/2), F=813.7c (8/5), F#=884.4c (5/3),
            // G=968.8c (7/4), G#=1088.3c (15/8)
            static const float justDegrees[12] = {
                0.0f, 111.73f, 203.91f, 315.64f, 386.31f, 498.04f,
                582.51f, 701.96f, 813.69f, 884.36f, 968.83f, 1088.27f
            };
            int octave = static_cast<int>(std::floor(static_cast<float>(noteOffset) / 12.0f));
            int pitchClass = (noteOffset % 12 + 12) % 12;
            return static_cast<float>(octave * 1200) + justDegrees[pitchClass];
        }

        case XenharmonicTuning::ARABIC_MAQAM_RAST: {
            // Quarter-tone Sikah / Rast (neutral 3rd at 350 cents, neutral 7th at 1050 cents)
            static const float rastDegrees[12] = {
                0.0f, 100.0f, 200.0f, 350.0f, 400.0f, 500.0f,
                600.0f, 700.0f, 800.0f, 900.0f, 1050.0f, 1100.0f
            };
            int octave = static_cast<int>(std::floor(static_cast<float>(noteOffset) / 12.0f));
            int pitchClass = (noteOffset % 12 + 12) % 12;
            return static_cast<float>(octave * 1200) + rastDegrees[pitchClass];
        }

        case XenharmonicTuning::GAMELAN_PELOG_SLENDRO: {
            // Slendro 5-tone equidistant scale (240 cents per step)
            float slendroStep = 1200.0f / 5.0f; // 240 cents
            float rawSteps = static_cast<float>(noteOffset) * (5.0f / 12.0f);
            return rawSteps * slendroStep;
        }

        default:
            return static_cast<float>(noteOffset) * 100.0f;
    }
}

float JZXenharmonicLatticeMorpherEngine::getNoteFrequencyHz(int midiNote) const {
    float srcCents = calculateTuningCents(m_sourceTuning, midiNote);
    float tgtCents = calculateTuningCents(m_targetTuning, midiNote);

    float morphedCents = srcCents * (1.0f - m_morphFactor) + tgtCents * m_morphFactor;
    return m_rootPitchHz * std::pow(2.0f, morphedCents / 1200.0f);
}

float JZXenharmonicLatticeMorpherEngine::getPitchDeviationCents(int midiNote) const {
    float tet12Cents = calculateTuningCents(XenharmonicTuning::TET_12_STANDARD, midiNote);
    float srcCents = calculateTuningCents(m_sourceTuning, midiNote);
    float tgtCents = calculateTuningCents(m_targetTuning, midiNote);

    float morphedCents = srcCents * (1.0f - m_morphFactor) + tgtCents * m_morphFactor;
    return morphedCents - tet12Cents;
}

std::vector<JZXenharmonicLatticeMorpherEngine::LatticeRatio>
JZXenharmonicLatticeMorpherEngine::getTonalityDiamondRatios() const {
    return {
        { 1, 1, 0.0f, "1/1 Tonic Unison" },
        { 16, 15, 111.7f, "16/15 Minor Second" },
        { 9, 8, 203.9f, "9/8 Major Second" },
        { 6, 5, 315.6f, "6/5 Minor Third" },
        { 5, 4, 386.3f, "5/4 Major Third" },
        { 4, 3, 498.0f, "4/3 Perfect Fourth" },
        { 7, 5, 582.5f, "7/5 Septimal Tritone" },
        { 3, 2, 702.0f, "3/2 Perfect Fifth" },
        { 8, 5, 813.7f, "8/5 Minor Sixth" },
        { 5, 3, 884.4f, "5/3 Major Sixth" },
        { 7, 4, 968.8f, "7/4 Harmonic Seventh" },
        { 15, 8, 1088.3f, "15/8 Major Seventh" },
        { 2, 1, 1200.0f, "2/1 Octave" }
    };
}

std::string JZXenharmonicLatticeMorpherEngine::getTuningName(XenharmonicTuning tuning) const {
    switch (tuning) {
        case XenharmonicTuning::TET_12_STANDARD: return "12-TET Western Standard";
        case XenharmonicTuning::BOHLEN_PIERCE_TRITAVE: return "Bohlen-Pierce (3:1 Tritave, 13-Tone)";
        case XenharmonicTuning::PARTCH_43_TONE_DIAMOND: return "Harry Partch 11-Limit Tonality Diamond";
        case XenharmonicTuning::ARABIC_MAQAM_RAST: return "Arabic Maqam Rast (Quarter-Tone Neutral 3rd/7th)";
        case XenharmonicTuning::GAMELAN_PELOG_SLENDRO: return "Indonesian Gamelan Slendro (5-EDO Equidistant)";
        default: return "Unknown Tuning";
    }
}

} // namespace Jazz
