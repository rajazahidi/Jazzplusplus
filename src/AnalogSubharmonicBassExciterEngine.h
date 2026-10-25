//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// AnalogSubharmonicBassExciterEngine.h - Subharmonic Bass & Optical Exciter
//
// Dual-band psychoacoustic analog excitation (Peavey Kosmos / Aphex Aural Exciter style):
// Phase-locked sub-octave fundamental synthesis for chest-thumping bass,
// coupled with optical high-frequency harmonic air generation for vocal presence.
//*****************************************************************************

#ifndef JAZZ_ANALOG_SUBHARMONIC_BASS_EXCITER_ENGINE_H
#define JAZZ_ANALOG_SUBHARMONIC_BASS_EXCITER_ENGINE_H

#include <vector>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

class JZAnalogSubharmonicBassExciterEngine {
public:
    struct Parameters {
        float subharmonicGainDb = 0.0f;     // Sub-bass synthesized gain (-24.0 to +12.0 dB)
        float opticalAirGainDb = 0.0f;       // High-frequency harmonic air (-24.0 to +12.0 dB)
        float subCrossoverHz = 90.0f;        // Sub-bass excitation boundary (50.0 to 140.0 Hz)
        float airCrossoverHz = 4500.0f;      // Air exciter boundary (2500.0 to 9000.0 Hz)
        float drive = 1.25f;                 // Optical harmonic saturation drive (1.0 to 3.0)
        float dryWet = 1.0f;                 // Wet / dry blend
    };

    JZAnalogSubharmonicBassExciterEngine(float sampleRate = 48000.0f);
    ~JZAnalogSubharmonicBassExciterEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setSubharmonicGainDb(float db);
    float getSubharmonicGainDb() const { return m_params.subharmonicGainDb; }

    void setOpticalAirGainDb(float db);
    float getOpticalAirGainDb() const { return m_params.opticalAirGainDb; }

    void setSubCrossoverHz(float hz);
    float getSubCrossoverHz() const { return m_params.subCrossoverHz; }

    void setAirCrossoverHz(float hz);
    float getAirCrossoverHz() const { return m_params.airCrossoverHz; }

    /**
     * @brief Process stereo audio buffer through subharmonic bass and optical air exciter.
     */
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

    float getSubSynthesizerEnergy() const { return m_subEnergy; }
    float getOpticalAirEnergy() const { return m_airEnergy; }

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    // Sub-bass low-pass filter biquad
    float m_subLpB0 = 0.0f, m_subLpB1 = 0.0f, m_subLpB2 = 0.0f, m_subLpA1 = 0.0f, m_subLpA2 = 0.0f;
    float m_subLpX1 = 0.0f, m_subLpX2 = 0.0f, m_subLpY1 = 0.0f, m_subLpY2 = 0.0f;

    // Air exciter high-pass filter biquad
    float m_airHpB0 = 0.0f, m_airHpB1 = 0.0f, m_airHpB2 = 0.0f, m_airHpA1 = 0.0f, m_airHpA2 = 0.0f;
    float m_airHpX1L = 0.0f, m_airHpX2L = 0.0f, m_airHpY1L = 0.0f, m_airHpY2L = 0.0f;
    float m_airHpX1R = 0.0f, m_airHpX2R = 0.0f, m_airHpY1R = 0.0f, m_airHpY2R = 0.0f;

    // Subharmonic octave flip-flop and envelope tracker
    bool m_flipFlop = false;
    float m_prevSubSign = 0.0f;
    float m_subEnv = 0.0f;
    float m_subSinePhase = 0.0f;

    float m_subEnergy = 0.0f;
    float m_airEnergy = 0.0f;

    void updateCrossoverFilters();
};

} // namespace Jazz

namespace jazz {
    using JZAnalogSubharmonicBassExciterEngine = Jazz::JZAnalogSubharmonicBassExciterEngine;
}

#endif // JAZZ_ANALOG_SUBHARMONIC_BASS_EXCITER_ENGINE_H
