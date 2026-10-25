//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// AcousticGuitarBodyImpulseConvolverEngine.h - Acoustic Guitar Pickup Convolver
//
// Acoustic guitar piezo-to-studio-microphone transformation (Fishman Aura / ToneDexter style):
// Anti-quack non-linear decompression, 102Hz Helmholtz body resonance, and
// studio microphone acoustic profile modeling (Neumann KM184, Royer R-121, C414).
//*****************************************************************************

#ifndef JAZZ_ACOUSTIC_GUITAR_BODY_IMPULSE_CONVOLVER_ENGINE_H
#define JAZZ_ACOUSTIC_GUITAR_BODY_IMPULSE_CONVOLVER_ENGINE_H

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

enum class GuitarMicProfile {
    KM184_SMALL_CONDENSER = 0, // Crisp acoustic sparkle, transparent transients
    R121_VINTAGE_RIBBON   = 1, // Warm woody mids, velvety rolled-off top end
    C414_LARGE_CONDENSER  = 2, // Full-bodied dreadnought projection, tight low end
    DREADNOUGHT_CHAMBER   = 3  // Deep acoustic spruce/rosewood body cavity
};

class JZAcousticGuitarBodyImpulseConvolverEngine {
public:
    struct Parameters {
        GuitarMicProfile profile = GuitarMicProfile::KM184_SMALL_CONDENSER;
        float bodyResonanceAmount = 0.65f;   // Helmholtz cavity coupling (0.0 to 1.0)
        float antiQuackAmount = 0.5f;        // Piezo transient compression (0.0 to 1.0)
        float soundholeNotchHz = 108.0f;     // Anti-feedback body notch (80.0 to 250.0 Hz)
        bool invertPolarity = false;         // Anti-feedback phase inversion
        float dryWet = 0.85f;                // Mic profile blend (0.0 = raw piezo, 1.0 = mic)
    };

    JZAcousticGuitarBodyImpulseConvolverEngine(float sampleRate = 48000.0f);
    ~JZAcousticGuitarBodyImpulseConvolverEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setMicProfile(GuitarMicProfile profile);
    GuitarMicProfile getMicProfile() const { return m_params.profile; }

    void setBodyResonanceAmount(float amount);
    float getBodyResonanceAmount() const { return m_params.bodyResonanceAmount; }

    void setAntiQuackAmount(float amount);
    float getAntiQuackAmount() const { return m_params.antiQuackAmount; }

    void setSoundholeNotchHz(float hz);
    float getSoundholeNotchHz() const { return m_params.soundholeNotchHz; }

    /**
     * @brief Process audio buffer through guitar body convolver.
     */
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

    float getHelmholtzResonanceHz() const { return 102.0f; }
    float getAntiQuackReductionDb() const { return m_antiQuackReductDb; }
    std::string getActiveProfileName() const;

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    // 102 Hz Helmholtz cavity resonator biquad
    float m_bodyB0 = 0.0f, m_bodyB1 = 0.0f, m_bodyB2 = 0.0f, m_bodyA1 = 0.0f, m_bodyA2 = 0.0f;
    float m_bodyX1L = 0.0f, m_bodyX2L = 0.0f, m_bodyY1L = 0.0f, m_bodyY2L = 0.0f;
    float m_bodyX1R = 0.0f, m_bodyX2R = 0.0f, m_bodyY1R = 0.0f, m_bodyY2R = 0.0f;

    // Anti-feedback soundhole notch biquad
    float m_notchB0 = 1.0f, m_notchB1 = 0.0f, m_notchB2 = 0.0f, m_notchA1 = 0.0f, m_notchA2 = 0.0f;
    float m_notchX1L = 0.0f, m_notchX2L = 0.0f, m_notchY1L = 0.0f, m_notchY2L = 0.0f;
    float m_notchX1R = 0.0f, m_notchX2R = 0.0f, m_notchY1R = 0.0f, m_notchY2R = 0.0f;

    // Microphone profile equalization biquads
    float m_micB0 = 1.0f, m_micB1 = 0.0f, m_micB2 = 0.0f, m_micA1 = 0.0f, m_micA2 = 0.0f;
    float m_micX1L = 0.0f, m_micX2L = 0.0f, m_micY1L = 0.0f, m_micY2L = 0.0f;
    float m_micX1R = 0.0f, m_micX2R = 0.0f, m_micY1R = 0.0f, m_micY2R = 0.0f;

    // Anti-quack transient detector envelope
    float m_quackEnv = 0.0f;
    float m_antiQuackReductDb = 0.0f;

    void updateAcousticFilters();
};

} // namespace Jazz

namespace jazz {
    using JZAcousticGuitarBodyImpulseConvolverEngine = Jazz::JZAcousticGuitarBodyImpulseConvolverEngine;
    using GuitarMicProfile = Jazz::GuitarMicProfile;
}

#endif // JAZZ_ACOUSTIC_GUITAR_BODY_IMPULSE_CONVOLVER_ENGINE_H
