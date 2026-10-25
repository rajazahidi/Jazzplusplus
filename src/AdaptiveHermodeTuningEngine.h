//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZAdaptiveHermodeTuningEngine.h - Dynamic Hermode Tuning & Just Intonation
//
// Real-time dynamic temperament retuning (Hermode Tuning):
// Dynamic chord root analysis, 0-cent beating pure Just Intonation harmonic
// retuning (Major 3rd -13.7c, Minor 3rd +15.6c, 5th +2.0c, Harmonic 7th -31.2c),
// horizontal melodic lead preservation, and 19/31/53-EDO microtonal scales.
//*****************************************************************************

#ifndef JAZZ_ADAPTIVE_HERMODE_TUNING_ENGINE_H
#define JAZZ_ADAPTIVE_HERMODE_TUNING_ENGINE_H

#include <vector>
#include <string>
#include <map>
#include <set>
#include <cstdint>

namespace Jazz {

enum class TemperamentMode {
    HERMODE_DYNAMIC_JUST,    // Dynamic real-time Just Intonation based on chord root
    EQUAL_TEMPERAMENT_12,    // Standard 12-TET (A4 = 440 Hz)
    EDO_19,                  // 19 Equal Divisions of the Octave
    EDO_31,                  // 31-EDO (Fokker pure 3rds)
    EDO_53                   // 53-EDO (Mercator / Turkish pure comma)
};

class JZAdaptiveHermodeTuningEngine {
public:
    JZAdaptiveHermodeTuningEngine(TemperamentMode mode = TemperamentMode::HERMODE_DYNAMIC_JUST,
                                 float concertPitchA4 = 440.0f);
    ~JZAdaptiveHermodeTuningEngine() = default;

    void setTemperamentMode(TemperamentMode mode);
    TemperamentMode getTemperamentMode() const { return m_mode; }

    void setConcertPitchA4(float freqHz);
    float getConcertPitchA4() const { return m_concertPitchA4; }

    void setTuningDepth(float depth); // 0.0 (12-TET) to 1.0 (Full Just Intonation)
    float getTuningDepth() const { return m_tuningDepth; }

    // MIDI Note Ingestion & Active Chord Analysis
    void noteOn(uint8_t note, uint8_t velocity);
    void noteOff(uint8_t note);
    void allNotesOff();

    size_t getActiveNoteCount() const { return m_activeNotes.size(); }
    std::string getDetectedChordName() const { return m_detectedChord; }
    int getDetectedRoot() const { return m_detectedRoot; }

    // Microtonal Pitch Evaluation
    float getPitchOffsetCents(uint8_t note) const;
    double noteToFrequencyHz(uint8_t note) const;

    void reset();

private:
    TemperamentMode m_mode;
    float m_concertPitchA4;
    float m_tuningDepth;

    std::set<uint8_t> m_activeNotes;
    int m_detectedRoot;
    std::string m_detectedChord;
    std::map<uint8_t, float> m_calculatedOffsets;

    void analyzeChordAndCalculateOffsets();
};

} // namespace Jazz

#endif // JAZZ_ADAPTIVE_HERMODE_TUNING_ENGINE_H
