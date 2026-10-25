//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// DynamicHermodeTuningEngine.cpp - Real-Time Polyphonic Dynamic Hermode
// Micro-Tuning & Pure Just Intonation Retuner
//*****************************************************************************

#include "DynamicHermodeTuningEngine.h"
#include <cstdint>
#include <cmath>
#include <algorithm>

namespace Jazz {

JZDynamicHermodeTuningEngine::JZDynamicHermodeTuningEngine(float sampleRate)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f),
      m_detectedRootNote(-1),
      m_detectedChordName("None")
{
    initialize(m_sampleRate);
}

void JZDynamicHermodeTuningEngine::initialize(float sampleRate)
{
    m_sampleRate = sampleRate > 1000.0f ? sampleRate : 48000.0f;
    reset();
}

void JZDynamicHermodeTuningEngine::reset()
{
    m_activeNotes.clear();
    m_noteCentsOffsets.fill(0.0f);
    m_smoothedCentsOffsets.fill(0.0f);
    m_detectedRootNote = -1;
    m_detectedChordName = "None";
}

void JZDynamicHermodeTuningEngine::setParameters(const Parameters& params)
{
    m_params = params;
    m_params.tuningDepth = std::max(0.0f, std::min(m_params.tuningDepth, 1.0f));
    m_params.referenceA4Hz = std::max(415.0f, std::min(m_params.referenceA4Hz, 466.0f));
    recalculateTargetOffsets();
}

void JZDynamicHermodeTuningEngine::setTuningDepth(float depth)
{
    m_params.tuningDepth = std::max(0.0f, std::min(depth, 1.0f));
    recalculateTargetOffsets();
}

float JZDynamicHermodeTuningEngine::midiNoteTo12TetFrequency(uint8_t note, float refA4)
{
    return refA4 * std::pow(2.0f, static_cast<float>(note - 69) / 12.0f);
}

void JZDynamicHermodeTuningEngine::noteOn(uint8_t noteNumber, uint8_t velocity)
{
    if (noteNumber < 128) {
        if (velocity > 0) {
            m_activeNotes[noteNumber] = velocity;
        } else {
            m_activeNotes.erase(noteNumber);
        }
        updateTuningAnalysis();
    }
}

void JZDynamicHermodeTuningEngine::noteOff(uint8_t noteNumber)
{
    if (noteNumber < 128) {
        m_activeNotes.erase(noteNumber);
        updateTuningAnalysis();
    }
}

void JZDynamicHermodeTuningEngine::allNotesOff()
{
    m_activeNotes.clear();
    updateTuningAnalysis();
}

void JZDynamicHermodeTuningEngine::updateTuningAnalysis()
{
    if (m_activeNotes.empty() || !m_params.enabled) {
        m_detectedRootNote = -1;
        m_detectedChordName = "None";
        m_noteCentsOffsets.fill(0.0f);
        m_smoothedCentsOffsets.fill(0.0f);
        return;
    }

    // Identify lowest note as acoustic root / bass foundation
    auto lowestIt = m_activeNotes.begin();
    m_detectedRootNote = lowestIt->first;
    int rootPitchClass = m_detectedRootNote % 12;

    // Build pitch class presence bitmask
    uint16_t pcMask = 0;
    for (const auto& pair : m_activeNotes) {
        int pc = pair.first % 12;
        int interval = (pc - rootPitchClass + 12) % 12;
        pcMask |= (1 << interval);
    }

    // Determine basic chord quality
    bool hasMajorThird = (pcMask & (1 << 4)) != 0;
    bool hasMinorThird = (pcMask & (1 << 3)) != 0;
    bool hasFifth = (pcMask & (1 << 7)) != 0;
    bool hasMinorSeventh = (pcMask & (1 << 10)) != 0;

    if (hasMajorThird && hasFifth && hasMinorSeventh) {
        m_detectedChordName = "Dominant 7th";
    } else if (hasMajorThird && hasFifth) {
        m_detectedChordName = "Major Triad";
    } else if (hasMinorThird && hasFifth) {
        m_detectedChordName = "Minor Triad";
    } else {
        m_detectedChordName = "Polyphonic Interval";
    }

    recalculateTargetOffsets();
}

void JZDynamicHermodeTuningEngine::recalculateTargetOffsets()
{
    if (m_detectedRootNote < 0 || !m_params.enabled) {
        m_noteCentsOffsets.fill(0.0f);
        m_smoothedCentsOffsets.fill(0.0f);
        return;
    }

    int rootPitchClass = m_detectedRootNote % 12;

    for (const auto& pair : m_activeNotes) {
        uint8_t note = pair.first;
        int interval = ((note % 12) - rootPitchClass + 12) % 12;

        float idealCentsOffset = 0.0f;
        switch (m_params.mode) {
            case HermodeTuningMode::JUST_INTONATION_5_LIMIT:
            case HermodeTuningMode::JUST_INTONATION_7_LIMIT:
                switch (interval) {
                    case 0: idealCentsOffset = 0.0f; break;          // Unison / Octave
                    case 1: idealCentsOffset = +11.731f; break;      // Minor 2nd (16:15)
                    case 2: idealCentsOffset = +3.910f; break;       // Major 2nd (9:8)
                    case 3: idealCentsOffset = +15.641f; break;      // Minor 3rd (6:5) -> raise by +15.6 cents
                    case 4: idealCentsOffset = -13.686f; break;      // Major 3rd (5:4) -> lower by -13.7 cents
                    case 5: idealCentsOffset = -1.955f; break;       // Perfect 4th (4:3)
                    case 6: idealCentsOffset = -17.488f; break;      // Tritone
                    case 7: idealCentsOffset = +1.955f; break;       // Perfect 5th (3:2) -> raise by +2.0 cents
                    case 8: idealCentsOffset = +13.686f; break;      // Minor 6th (8:5)
                    case 9: idealCentsOffset = -15.641f; break;      // Major 6th (5:3)
                    case 10:                                         // 7th
                        if (m_params.mode == HermodeTuningMode::JUST_INTONATION_7_LIMIT) {
                            idealCentsOffset = -31.174f;             // Harmonic 7th (7:4) -> lower by -31.2 cents
                        } else {
                            idealCentsOffset = -3.910f;              // Pythagorean minor 7th (9:5)
                        }
                        break;
                    case 11: idealCentsOffset = -11.731f; break;     // Major 7th (15:8)
                    default: idealCentsOffset = 0.0f; break;
                }
                break;
            case HermodeTuningMode::PYTHAGOREAN_PURE_FIFTHS:
                switch (interval) {
                    case 0: idealCentsOffset = 0.0f; break;
                    case 7: idealCentsOffset = +1.955f; break;
                    case 2: idealCentsOffset = +3.910f; break;
                    case 9: idealCentsOffset = +5.865f; break;
                    case 4: idealCentsOffset = +7.820f; break;
                    default: idealCentsOffset = 0.0f; break;
                }
                break;
        }

        float appliedOffset = idealCentsOffset * m_params.tuningDepth;
        m_noteCentsOffsets[note] = appliedOffset;
        m_smoothedCentsOffsets[note] = appliedOffset;
    }
}

float JZDynamicHermodeTuningEngine::getNoteCentsOffset(uint8_t noteNumber) const
{
    if (noteNumber < 128) {
        return m_smoothedCentsOffsets[noteNumber];
    }
    return 0.0f;
}

float JZDynamicHermodeTuningEngine::getNoteTunedFrequencyHz(uint8_t noteNumber) const
{
    if (noteNumber >= 128) return 0.0f;
    float baseFreq = midiNoteTo12TetFrequency(noteNumber, m_params.referenceA4Hz);
    float cents = getNoteCentsOffset(noteNumber);
    return baseFreq * std::pow(2.0f, cents / 1200.0f);
}

} // namespace Jazz
