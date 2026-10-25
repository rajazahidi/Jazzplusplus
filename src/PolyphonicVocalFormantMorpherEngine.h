//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// PolyphonicVocalFormantMorpherEngine.h - Vocal Formant & Throat Resynthesizer
//
// Pitch-independent vocal formant manipulation (Antares Throat / Celemony style):
// Acoustic vocal tract length scaling (0.6x to 1.6x), 4-formant shifting (F1-F4),
// gender morphing, and glottal breathiness injection for lead & backing vocals.
//*****************************************************************************

#ifndef JAZZ_POLYPHONIC_VOCAL_FORMANT_MORPHER_ENGINE_H
#define JAZZ_POLYPHONIC_VOCAL_FORMANT_MORPHER_ENGINE_H

#include <vector>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <algorithm>

namespace Jazz {

class JZPolyphonicVocalFormantMorpherEngine {
public:
    struct Parameters {
        float throatLengthFactor = 1.0f;     // 0.6x (child/soprano) to 1.6x (deep baritone)
        float formantShiftSemitones = 0.0f;  // Independent formant shift (-12.0 to +12.0 st)
        float genderMorph = 0.0f;            // -1.0 (feminine) to +1.0 (masculine)
        float breathiness = 0.1f;            // Glottal aspiration noise (0.0 to 0.8)
        float dryWet = 1.0f;                 // Wet / dry blend
    };

    JZPolyphonicVocalFormantMorpherEngine(float sampleRate = 48000.0f);
    ~JZPolyphonicVocalFormantMorpherEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setThroatLengthFactor(float factor);
    float getThroatLengthFactor() const { return m_params.throatLengthFactor; }

    void setFormantShiftSemitones(float st);
    float getFormantShiftSemitones() const { return m_params.formantShiftSemitones; }

    void setGenderMorph(float gender);
    float getGenderMorph() const { return m_params.genderMorph; }

    /**
     * @brief Process stereo audio buffer through vocal formant resynthesizer.
     */
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

    float getFormantF1Hz() const { return m_f1; }
    float getFormantF2Hz() const { return m_f2; }
    float getFormantF3Hz() const { return m_f3; }

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    // 4 Formant Resonators (F1, F2, F3, F4)
    struct FormantFilter {
        float b0, b1, b2, a1, a2;
        float x1L, x2L, y1L, y2L;
        float x1R, x2R, y1R, y2R;

        void reset() {
            x1L = x2L = y1L = y2L = 0.0f;
            x1R = x2R = y1R = y2R = 0.0f;
        }

        inline float processL(float in) {
            float out = b0 * in + b1 * x1L + b2 * x2L - a1 * y1L - a2 * y2L;
            x2L = x1L; x1L = in; y2L = y1L; y1L = out;
            return out;
        }

        inline float processR(float in) {
            float out = b0 * in + b1 * x1R + b2 * x2R - a1 * y1R - a2 * y2R;
            x2R = x1R; x1R = in; y2R = y1R; y1R = out;
            return out;
        }
    };

    FormantFilter m_formants[4];
    float m_f1 = 700.0f, m_f2 = 1250.0f, m_f3 = 2800.0f, m_f4 = 3800.0f;

    uint32_t m_noiseSeed = 987654321;
    float getNoise();

    void updateFormantCoefficients();
};

} // namespace Jazz

namespace jazz {
    using JZPolyphonicVocalFormantMorpherEngine = Jazz::JZPolyphonicVocalFormantMorpherEngine;
}

#endif // JAZZ_POLYPHONIC_VOCAL_FORMANT_MORPHER_ENGINE_H
