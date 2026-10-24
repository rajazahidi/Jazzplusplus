#ifndef JAZZ_MICROTONAL_TUNING_ENGINE_H
#define JAZZ_MICROTONAL_TUNING_ENGINE_H

#include <vector>
#include <string>
#include <cmath>

namespace Jazz {

enum class MicrotonalPreset {
    EQUAL_TEMPERAMENT_12,
    JUST_INTONATION,
    PYTHAGOREAN,
    QUARTER_COMMA_MEANTONE,
    EDO_19,
    EDO_31,
    MAQAM_RAST,
    MAQAM_BAYATI,
    GAMELAN_SLENDRO,
    GAMELAN_PELOG,
    CUSTOM_SCALA
};

struct ScaleNotePitch {
    bool isRatio;
    int numerator;
    int denominator;
    double cents;
    double ratioValue;
};

class JZMicrotonalTuningEngine {
public:
    JZMicrotonalTuningEngine();
    ~JZMicrotonalTuningEngine() = default;

    void setPreset(MicrotonalPreset preset);
    MicrotonalPreset getPreset() const { return m_preset; }

    bool loadScalaString(const std::string& sclContent);
    const std::string& getScaleDescription() const { return m_scaleDescription; }
    int getScaleSize() const { return static_cast<int>(m_scalePitches.size()); }

    void setReferencePitch(double freqHz = 440.0, int refMidiNote = 69);
    double getReferencePitch() const { return m_refFrequency; }
    int getReferenceMidiNote() const { return m_refMidiNote; }

    void setRootKey(int rootMidiNote = 60); // middle C = 60
    int getRootKey() const { return m_rootKey; }

    void setMasterTuningOffsetCents(double cents);
    double getMasterTuningOffsetCents() const { return m_masterOffsetCents; }

    // Core frequency computation for MIDI note (0..127) plus real-time pitch bend
    double getNoteFrequency(int midiNote, double pitchBendSemitones = 0.0) const;

    // Deviation in cents from standard 12-TET frequency
    double getCentsDeviationFrom12TET(int midiNote) const;

    // Quantize an arbitrary frequency to the nearest scale pitch
    double quantizeFrequency(double inputFreqHz) const;

    // Set custom note frequency directly (e.g. MTS SysEx retuning)
    void setNoteDirectFrequency(int midiNote, double freqHz);

    // Reset mapping back to selected preset
    void resetToPreset();

    // Utility: ratio to cents
    static double ratioToCents(double ratio);
    // Utility: cents to ratio
    static double centsToRatio(double cents);
    // Standard 12-TET frequency
    static double standard12TETFrequency(int midiNote, double refA4 = 440.0);

private:
    void rebuildFrequencyTable();
    void applyBuiltinPreset(MicrotonalPreset preset);

    MicrotonalPreset m_preset;
    std::string m_scaleDescription;
    std::vector<ScaleNotePitch> m_scalePitches;
    double m_octaveRatio;

    double m_refFrequency;
    int m_refMidiNote;
    int m_rootKey;
    double m_masterOffsetCents;

    double m_noteFrequencies[128];
    bool m_directOverride[128];
};

} // namespace Jazz

#endif // JAZZ_MICROTONAL_TUNING_ENGINE_H
