//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// PersonalizedPinnaAcousticModeler.h - Anthropometric HRTF & Ear Canal Modeler
//
// Personalized spatial binaural monitoring (Genelec Aural ID / Apple Spatial style):
// Anthropometric ear geometry customization (head circumference, pinna height,
// concha depth, ear canal resonance) eliminating front-back confusion and elevation blur.
//*****************************************************************************

#ifndef JAZZ_PERSONALIZED_PINNA_ACOUSTIC_MODELER_H
#define JAZZ_PERSONALIZED_PINNA_ACOUSTIC_MODELER_H

#include <vector>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

class JZPersonalizedPinnaAcousticModeler {
public:
    struct EarGeometry {
        float headCircumferenceCm = 56.0f; // 50.0 to 65.0 cm (determines ITD)
        float pinnaHeightMm = 62.0f;       // 45.0 to 78.0 mm (determines 6-10kHz notch)
        float conchaDepthMm = 16.0f;       // 8.0 to 25.0 mm (high-frequency cavity mode)
        float earCanalLengthMm = 25.0f;    // 18.0 to 34.0 mm (determines 3.4kHz resonance)
    };

    struct VirtualSource {
        float azimuthDeg = 30.0f;          // -180.0 to +180.0 degrees (horizontal)
        float elevationDeg = 0.0f;         // -45.0 to +90.0 degrees (vertical)
        float distanceMeters = 1.5f;       // 0.5 to 10.0 meters
    };

    JZPersonalizedPinnaAcousticModeler(float sampleRate = 48000.0f);
    ~JZPersonalizedPinnaAcousticModeler() = default;

    void initialize(float sampleRate);
    void reset();

    void setEarGeometry(const EarGeometry& geom);
    const EarGeometry& getEarGeometry() const { return m_geom; }

    void setVirtualSource(const VirtualSource& src);
    const VirtualSource& getVirtualSource() const { return m_source; }

    /**
     * @brief Process audio buffer through personalized anthropometric HRTF filters.
     */
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

    float getEarCanalResonanceHz() const { return m_earCanalResonanceHz; }
    float getPinnaNotchFrequencyHz() const { return m_pinnaNotchHz; }
    float getInterauralTimeDelayUs() const { return m_itdMicroseconds; }

private:
    float m_sampleRate = 48000.0f;
    EarGeometry m_geom;
    VirtualSource m_source;

    // Acoustic physical resonant frequencies
    float m_earCanalResonanceHz = 3430.0f;
    float m_pinnaNotchHz = 7200.0f;
    float m_itdMicroseconds = 240.0f;

    // Fractional ITD delay line for head shadow
    std::vector<float> m_itdDelayBufL;
    std::vector<float> m_itdDelayBufR;
    size_t m_itdWritePos = 0;
    static const size_t ITD_BUF_SIZE = 128;

    // Resonant filters (Ear Canal Peak & Pinna Notch) for Left and Right
    struct Biquad {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
        float a1 = 0.0f, a2 = 0.0f;
        float x1 = 0.0f, x2 = 0.0f, y1 = 0.0f, y2 = 0.0f;

        void reset() { x1 = x2 = y1 = y2 = 0.0f; }

        inline float processSample(float in) {
            float out = b0 * in + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = in;
            y2 = y1; y1 = out;
            return out;
        }
    };

    Biquad m_earCanalFilterL, m_earCanalFilterR;
    Biquad m_pinnaNotchFilterL, m_pinnaNotchFilterR;
    Biquad m_headShadowFilterL, m_headShadowFilterR;

    void updateAcousticFilters();
};

} // namespace Jazz

namespace jazz {
    using JZPersonalizedPinnaAcousticModeler = Jazz::JZPersonalizedPinnaAcousticModeler;
}

#endif // JAZZ_PERSONALIZED_PINNA_ACOUSTIC_MODELER_H
