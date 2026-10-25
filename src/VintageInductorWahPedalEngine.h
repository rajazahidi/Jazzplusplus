//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// VintageInductorWahPedalEngine.h - Historic Fasel / Halo Toroidal Inductor
// Wah-Wah & Resonant Drive Engine
//
// Circuit-accurate physical modeling of the legendary 1967 Vox Clyde McCoy
// and Thomas Organ Cry Baby wah pedal:
// 500mH toroidal inductor non-linear magnetic flux B-H saturation,
// 100k ICAR S-taper potentiometer curve, 2N5172 BJT transistor feedback gain stage,
// variable foot treadle sweep (350Hz - 2.2kHz), and pre-filter analog drive.
//*****************************************************************************

#ifndef JAZZ_VINTAGE_INDUCTOR_WAH_PEDAL_ENGINE_H
#define JAZZ_VINTAGE_INDUCTOR_WAH_PEDAL_ENGINE_H

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

enum class WahInductorType {
    FASEL_RED = 0,    // Italian Red Fasel: High harmonic bite, rich treble chime
    FASEL_YELLOW,     // Italian Yellow Fasel: Vintage mellow, deep round vowel
    HALO_TOROIDAL     // Clyde McCoy Halo: Broad resonant throatiness, thick mids
};

class JZVintageInductorWahPedalEngine {
public:
    struct Parameters {
        float pedalPosition = 0.5f;          // Foot treadle position: 0.0 (heel, bass) to 1.0 (toe, treble)
        float driveDb = 0.0f;                // Input analog drive (-12.0 to +18.0 dB)
        float inductorSaturation = 0.5f;     // Inductor core non-linear saturation (0.0 to 1.0)
        float resonanceQ = 5.5f;             // Resonant filter Q peak (2.0 to 10.0)
        WahInductorType inductorType = WahInductorType::FASEL_RED;
        bool bypass = false;
    };

    JZVintageInductorWahPedalEngine(float sampleRate = 48000.0f);
    ~JZVintageInductorWahPedalEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setPedalPosition(float position0to1);
    float getPedalPosition() const { return m_params.pedalPosition; }

    void setDriveDb(float db);
    float getDriveDb() const { return m_params.driveDb; }

    void setInductorSaturation(float sat);
    float getInductorSaturation() const { return m_params.inductorSaturation; }

    void setResonanceQ(float q);
    float getResonanceQ() const { return m_params.resonanceQ; }

    void setInductorType(WahInductorType type);
    WahInductorType getInductorType() const { return m_params.inductorType; }
    std::string getInductorTypeName() const;

    void setBypass(bool bp);
    bool isBypassed() const { return m_params.bypass; }

    /**
     * @brief Process stereo audio buffer through vintage inductor wah pedal.
     */
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

    // Telemetry
    float getCenterFrequencyHz() const { return m_centerFreqHz; }
    float getThdPercent() const { return m_thdPercent; }

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    float m_centerFreqHz = 850.0f;
    float m_thdPercent = 0.5f;

    // Resonant bandpass filter states (State-Variable Filter topology)
    float m_ic1eqL = 0.0f, m_ic2eqL = 0.0f;
    float m_ic1eqR = 0.0f, m_ic2eqR = 0.0f;

    // Smoothing for pedal treadle movement
    float m_smoothedPos = 0.5f;

    void updateFilterCoeffs();
    float calculateIcarTaper(float pos) const;
};

} // namespace Jazz

namespace jazz {
    using JZVintageInductorWahPedalEngine = Jazz::JZVintageInductorWahPedalEngine;
}

#endif // JAZZ_VINTAGE_INDUCTOR_WAH_PEDAL_ENGINE_H
