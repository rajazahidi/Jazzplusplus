//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZSpectralAudioMorphEngine.h - Polyphonic Spectral Audio Morpher
//
// Cinematic sound design & music production spectral audio morphing (Kyma style):
// Optimal-transport frequency bin interpolation, formant envelope cross-synthesis,
// harmonic excitation transplantation, and chimeric audio transmutation.
//*****************************************************************************

#ifndef JAZZ_SPECTRAL_AUDIO_MORPH_ENGINE_H
#define JAZZ_SPECTRAL_AUDIO_MORPH_ENGINE_H

#include <vector>
#include <string>
#include <cstdint>
#include <cmath>

namespace Jazz {

enum class SpectralMorphMode {
    SPECTRAL_INTERPOLATION,  // Direct energy-preserving magnitude morph between A and B
    FORMANT_CROSS_SYNTHESIS, // Excitation of A filtered by formant envelope of B
    HYBRID_CHIMERA           // Transients of A combined with sustained tonal body of B
};

class JZSpectralAudioMorphEngine {
public:
    JZSpectralAudioMorphEngine(float sampleRate = 48000.0f, size_t fftSize = 512);
    ~JZSpectralAudioMorphEngine() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return m_sampleRate; }

    void setMorphMode(SpectralMorphMode mode);
    SpectralMorphMode getMorphMode() const { return m_morphMode; }

    // Morph Controls
    void setMorphFactor(float factor); // 0.0 (Source A) to 1.0 (Source B)
    float getMorphFactor() const { return m_morphFactor; }

    void setFormantPreservation(float amount); // 0.0 (unfiltered) to 1.0 (strong formant envelope)
    float getFormantPreservation() const { return m_formantPreservation; }

    void setSpectralTiltDb(float tiltDb);
    float getSpectralTiltDb() const { return m_spectralTiltDb; }

    // Real-Time Processing: Morphs stereo or mono Source A and Source B into output buffer
    void processMorph(const float* inA_L, const float* inA_R,
                      const float* inB_L, const float* inB_R,
                      float* outL, float* outR, size_t numSamples);

    void reset();

private:
    float m_sampleRate;
    size_t m_fftSize;
    SpectralMorphMode m_morphMode;
    float m_morphFactor;
    float m_formantPreservation;
    float m_spectralTiltDb;

    // Filter states for spectral envelope tracking
    float m_envL_A, m_envR_A;
    float m_envL_B, m_envR_B;
    float m_transL_A, m_transR_A;
    float m_prevA_L, m_prevA_R;
    float m_prevB_L, m_prevB_R;
};

} // namespace Jazz

#endif // JAZZ_SPECTRAL_AUDIO_MORPH_ENGINE_H
