//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// XenharmonicLatticeMorpherEngine.h - Dynamic Xenharmonic Lattice & Scale Morpher
//
// Multidimensional Just Intonation lattice & continuous scale interpolation:
// Harry Partch 43-tone tonality diamond, Bohlen-Pierce (3:1 tritave), Arabic Maqam,
// Gamelan Pelog/Slendro, and real-time morphing between arbitrary temperaments.
//*****************************************************************************

#ifndef JAZZ_XENHARMONIC_LATTICE_MORPHER_ENGINE_H
#define JAZZ_XENHARMONIC_LATTICE_MORPHER_ENGINE_H

#include <vector>
#include <string>
#include <cmath>
#include <cstdint>

namespace Jazz {

enum class XenharmonicTuning {
    TET_12_STANDARD = 0,    // Standard Western 12-Tone Equal Temperament
    BOHLEN_PIERCE_TRITAVE,  // 13 divisions of 3:1 tritave (odd harmonics 3, 5, 7, 9)
    PARTCH_43_TONE_DIAMOND, // Harry Partch 11-limit Just Intonation diamond (43 tones)
    ARABIC_MAQAM_RAST,      // Quarter-tone neutral 3rd & 7th (Sikah / Rast)
    GAMELAN_PELOG_SLENDRO   // Indonesian non-octave equidistant metallophone scales
};

class JZXenharmonicLatticeMorpherEngine {
public:
    struct LatticeRatio {
        int numerator;
        int denominator;
        float cents;
        std::string name;
    };

    JZXenharmonicLatticeMorpherEngine(float rootPitchHz = 440.0f); // Default A4 = 440 Hz
    ~JZXenharmonicLatticeMorpherEngine() = default;

    void setRootPitchHz(float hz);
    float getRootPitchHz() const { return m_rootPitchHz; }

    void setSourceTuning(XenharmonicTuning tuning);
    XenharmonicTuning getSourceTuning() const { return m_sourceTuning; }

    void setTargetTuning(XenharmonicTuning tuning);
    XenharmonicTuning getTargetTuning() const { return m_targetTuning; }

    void setMorphFactor(float morph); // 0.0 = 100% source, 1.0 = 100% target
    float getMorphFactor() const { return m_morphFactor; }

    /**
     * @brief Get precise morphed frequency in Hertz for given MIDI note.
     */
    float getNoteFrequencyHz(int midiNote) const;

    /**
     * @brief Get pitch deviation in cents from 12-TET for pitch-bend retuning.
     */
    float getPitchDeviationCents(int midiNote) const;

    /**
     * @brief Retrieve harmonic lattice ratios for active target scale.
     */
    std::vector<LatticeRatio> getTonalityDiamondRatios() const;

    std::string getTuningName(XenharmonicTuning tuning) const;

private:
    float m_rootPitchHz = 440.0f;
    XenharmonicTuning m_sourceTuning = XenharmonicTuning::TET_12_STANDARD;
    XenharmonicTuning m_targetTuning = XenharmonicTuning::PARTCH_43_TONE_DIAMOND;
    float m_morphFactor = 1.0f; // Default 100% target

    float calculateTuningCents(XenharmonicTuning tuning, int midiNote) const;
};

} // namespace Jazz

namespace jazz {
    using JZXenharmonicLatticeMorpherEngine = Jazz::JZXenharmonicLatticeMorpherEngine;
}

#endif // JAZZ_XENHARMONIC_LATTICE_MORPHER_ENGINE_H
