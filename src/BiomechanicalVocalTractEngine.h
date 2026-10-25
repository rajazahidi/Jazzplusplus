//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZBiomechanicalVocalTractEngine.h - Bio-Mechanical Vocal Tract Aero-Acoustics
//
// Sampleless physical singing voice synthesis:
// Two-mass vocal fold model driven by subglottal lung pressure & Bernoulli flow,
// 8-section acoustic waveguide vocal tract (pharynx, velum, tongue, lips),
// formant vowel transitions (/a/, /e/, /i/, /o/, /u/), and nasal aero-acoustics.
//*****************************************************************************

#ifndef JAZZ_BIOMECHANICAL_VOCAL_TRACT_ENGINE_H
#define JAZZ_BIOMECHANICAL_VOCAL_TRACT_ENGINE_H

#include <vector>
#include <string>
#include <cstdint>
#include <cmath>

namespace Jazz {

enum class VocalTractVowel {
    VOWEL_A, // "ah" (Father) - Low F1, High F2
    VOWEL_E, // "eh" (Bed) - Mid F1, Mid F2
    VOWEL_I, // "ee" (Meet) - Low F1, Very High F2
    VOWEL_O, // "oh" (Boat) - Mid F1, Low F2
    VOWEL_U  // "oo" (Boot) - Very Low F1, Low F2
};

class JZBiomechanicalVocalTractEngine {
public:
    JZBiomechanicalVocalTractEngine(float sampleRate = 48000.0f);
    ~JZBiomechanicalVocalTractEngine() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return m_sampleRate; }

    // Glottal Pitch & Subglottal Pressure Controls
    void setPitchHz(float f0Hz);
    float getPitchHz() const { return m_pitchHz; }

    void setSubglottalPressure(float pressureCmH2O); // 4.0 (pianissimo) to 25.0 (fortissimo)
    float getSubglottalPressure() const { return m_subglottalPressure; }

    void setBreathiness(float breathiness); // 0.0 (clean modal voice) to 1.0 (breathy whisper)
    float getBreathiness() const { return m_breathiness; }

    void setNasality(float velumOpening); // 0.0 (oral only) to 1.0 (nasalized vowel)
    float getNasality() const { return m_nasality; }

    // Vowel Articulation
    void setVowel(VocalTractVowel vowel);
    VocalTractVowel getVowel() const { return m_currentVowel; }

    void setVowelBlend(VocalTractVowel v1, VocalTractVowel v2, float blendFactor);

    // Audio Generation (synthesizes audio samples)
    void generate(float* outAudio, size_t numSamples);

    void reset();

private:
    float m_sampleRate;
    float m_pitchHz;
    float m_subglottalPressure;
    float m_breathiness;
    float m_nasality;
    VocalTractVowel m_currentVowel;

    // Glottal pulse oscillator state (Liljencrants-Fant / 2-mass glottal model)
    float m_glottalPhase;
    float m_openQuotient;

    // 8-section vocal tract cross-sectional areas (cm^2)
    static const size_t TRACT_SECTIONS = 8;
    float m_targetAreas[TRACT_SECTIONS];
    float m_currentAreas[TRACT_SECTIONS];

    // Waveguide delay lines for acoustic wave propagation (forward and backward waves)
    float m_forwardWave[TRACT_SECTIONS];
    float m_backwardWave[TRACT_SECTIONS];
    float m_lipRadiationState;

    uint32_t m_noiseSeed;
    float getNoise();

    void updateVowelAreas(VocalTractVowel v, float target[TRACT_SECTIONS]) const;
};

} // namespace Jazz

#endif // JAZZ_BIOMECHANICAL_VOCAL_TRACT_ENGINE_H
