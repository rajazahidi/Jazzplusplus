//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// DynamicHermodeTuningEngine.h - Real-Time Polyphonic Dynamic Hermode
// Micro-Tuning & Pure Just Intonation Retuner
//
// Dynamically eliminates harmonic beating in orchestral, choral, and acoustic
// instruments. Analyzes polyphonic chord structures in real time, determines root
// reference pitch, and calculates cents pitch offsets for 5-limit / 7-limit Just
// Intonation (Major 3rd -13.7c, Minor 3rd +15.6c, Perfect 5th +2.0c, Harmonic 7th -31.2c)
// with continuous smoothing to eliminate pitch artifacts.
//*****************************************************************************

#ifndef JAZZ_DYNAMIC_HERMODE_TUNING_ENGINE_H
#define JAZZ_DYNAMIC_HERMODE_TUNING_ENGINE_H

#include <vector>
#include <string>
#include <array>
#include <map>
#include <cstdint>
#include <cstddef>

namespace Jazz {

enum class HermodeTuningMode {
    JUST_INTONATION_5_LIMIT = 0, // Classic Pure 3rd & 5th (Ptolemaic)
    JUST_INTONATION_7_LIMIT,     // Includes Pure Harmonic 7th
    PYTHAGOREAN_PURE_FIFTHS      // Pure 3:2 Fifths
};

struct HermodeNoteOffset {
    uint8_t midiNote = 60;
    float frequencyHz = 261.6256f;
    float centsOffset = 0.0f;
    std::string intervalName = "Root";
};

class JZDynamicHermodeTuningEngine {
public:
    struct Parameters {
        HermodeTuningMode mode = HermodeTuningMode::JUST_INTONATION_5_LIMIT;
        float tuningDepth = 1.0f;          // 0.0 = Standard 12-TET, 1.0 = Pure Just Intonation
        float referenceA4Hz = 440.0f;      // Chamber concert pitch
        float smoothingTimeMs = 25.0f;     // Glide smoothing to prevent audible clicks
        bool enabled = true;
    };

    JZDynamicHermodeTuningEngine(float sampleRate = 48000.0f);
    ~JZDynamicHermodeTuningEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setTuningDepth(float depth);
    float getTuningDepth() const { return m_params.tuningDepth; }

    // Polyphonic MIDI Note Tracking
    void noteOn(uint8_t noteNumber, uint8_t velocity);
    void noteOff(uint8_t noteNumber);
    void allNotesOff();

    // Updates dynamic chord analysis and retuning offsets
    void updateTuningAnalysis();

    // Note Inspection
    float getNoteCentsOffset(uint8_t noteNumber) const;
    float getNoteTunedFrequencyHz(uint8_t noteNumber) const;
    size_t getActiveNoteCount() const { return m_activeNotes.size(); }
    int getDetectedRootMidiNote() const { return m_detectedRootNote; }
    std::string getDetectedChordName() const { return m_detectedChordName; }

    // Helpers
    static float midiNoteTo12TetFrequency(uint8_t note, float refA4 = 440.0f);

private:
    float m_sampleRate;
    Parameters m_params;

    std::map<uint8_t, uint8_t> m_activeNotes; // note -> velocity
    std::array<float, 128> m_noteCentsOffsets;
    std::array<float, 128> m_smoothedCentsOffsets;

    int m_detectedRootNote;
    std::string m_detectedChordName;

    void recalculateTargetOffsets();
};

} // namespace Jazz

#endif // JAZZ_DYNAMIC_HERMODE_TUNING_ENGINE_H
