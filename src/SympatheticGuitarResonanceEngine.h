//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZSympatheticGuitarResonanceEngine.h - Acoustic Guitar Sympathetic Resonance
//
// Physical modeling of acoustic guitar bridge coupling & sympathetic resonance:
// 6-string inter-string impedance matrix, Helmholtz soundhole air cavity (102Hz),
// spruce soundboard wood modes, and capo acoustic impedance shift.
//*****************************************************************************

#ifndef JAZZ_SYMPATHETIC_GUITAR_RESONANCE_ENGINE_H
#define JAZZ_SYMPATHETIC_GUITAR_RESONANCE_ENGINE_H

#include <vector>
#include <string>
#include <cstdint>
#include <cmath>

namespace Jazz {

struct GuitarStringState {
    int stringNumber;     // 1 to 6 (1=E4, 2=B3, 3=G3, 4=D3, 5=A2, 6=E2)
    float baseFrequencyHz;
    float currentResonanceEnergy;
    bool isPlucked;
};

class JZSympatheticGuitarResonanceEngine {
public:
    JZSympatheticGuitarResonanceEngine(float sampleRate = 48000.0f);
    ~JZSympatheticGuitarResonanceEngine() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return m_sampleRate; }

    // Physical Body Controls
    void setSympatheticCoupling(float amount); // 0.0 (isolated) to 1.0 (rich acoustic halo)
    float getSympatheticCoupling() const { return m_coupling; }

    void setBodyResonance(float amount); // Helmholtz & wood plate resonance (0.0 to 1.0)
    float getBodyResonance() const { return m_bodyResonance; }

    void setCapoFret(int fret); // 0 (open) to 7th fret
    int getCapoFret() const { return m_capoFret; }

    // Pluck excitation
    void pluckString(int stringNumber, float velocity = 0.8f);

    // Audio Processing
    void process(const float* inL, const float* inR,
                 float* outL, float* outR, size_t numSamples);

    std::vector<GuitarStringState> getStringStates() const;

    void reset();

private:
    float m_sampleRate;
    float m_coupling;
    float m_bodyResonance;
    int m_capoFret;

    static const size_t NUM_STRINGS = 6;
    float m_nominalFreqs[NUM_STRINGS];
    float m_stringFrequencies[NUM_STRINGS];
    float m_stringEnvelopes[NUM_STRINGS];

    // Biquad resonators for each string's sympathetic vibration
    float m_filterState1[NUM_STRINGS], m_filterState2[NUM_STRINGS];

    // Helmholtz air cavity resonator (102 Hz) filter states
    float m_helmholtzState1, m_helmholtzState2;

    void updateFrequencies();
};

} // namespace Jazz

#endif // JAZZ_SYMPATHETIC_GUITAR_RESONANCE_ENGINE_H
