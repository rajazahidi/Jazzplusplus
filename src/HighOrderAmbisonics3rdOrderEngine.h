//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// HighOrderAmbisonics3rdOrderEngine.h - 3rd-Order HOA Spherical Harmonics
//
// 16-Channel High-Order Ambisonics (Order 3) encoder, 3D Euler rotator,
// and binaural / 7.1.4 spatial decoder adhering to ACN ordering & SN3D normalization.
//*****************************************************************************

#ifndef JAZZ_HIGH_ORDER_AMBISONICS_3RD_ORDER_ENGINE_H
#define JAZZ_HIGH_ORDER_AMBISONICS_3RD_ORDER_ENGINE_H

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

class JZHighOrderAmbisonics3rdOrderEngine {
public:
    static const size_t NUM_HOA_CHANNELS = 16; // Order 3: (3 + 1)^2 = 16

    struct SpatialSource {
        float azimuthDeg = 0.0f;     // -180.0 to +180.0 deg (horizontal)
        float elevationDeg = 0.0f;   // -90.0 to +90.0 deg (vertical)
        float distanceMeters = 2.0f; // 0.5 to 20.0 m
    };

    struct Orientation {
        float yawDeg = 0.0f;         // Head rotation around Z axis
        float pitchDeg = 0.0f;       // Head rotation around Y axis
        float rollDeg = 0.0f;        // Head rotation around X axis
    };

    JZHighOrderAmbisonics3rdOrderEngine(float sampleRate = 48000.0f);
    ~JZHighOrderAmbisonics3rdOrderEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setSourcePosition(const SpatialSource& src);
    const SpatialSource& getSourcePosition() const { return m_source; }

    void setListenerOrientation(const Orientation& ori);
    const Orientation& getListenerOrientation() const { return m_orient; }

    /**
     * @brief Encode a mono audio stream into 16 3rd-order Ambisonics channels (ACN / SN3D).
     */
    void encodeMono(const float* inMono,
                    float* const* hoaOutputs,
                    size_t numSamples);

    /**
     * @brief Rotate an existing 16-channel HOA soundfield in 3D space by yaw/pitch/roll.
     */
    void rotateSoundfield(float* const* hoaBuffers, size_t numSamples);

    /**
     * @brief Decode 16 HOA channels to binaural stereo headphones using max-rE energy weighting.
     */
    void decodeToBinaural(const float* const* hoaInputs,
                          float* outL, float* outR,
                          size_t numSamples);

    float getEncodingWeight(size_t acnIndex) const;
    std::string getChannelAcnName(size_t acnIndex) const;

private:
    float m_sampleRate = 48000.0f;
    SpatialSource m_source;
    Orientation m_orient;

    // 16 ACN spherical harmonic weights
    float m_shWeights[NUM_HOA_CHANNELS];

    void computeSphericalHarmonics(float azRad, float elRad, float* outWeights);
};

} // namespace Jazz

namespace jazz {
    using JZHighOrderAmbisonics3rdOrderEngine = Jazz::JZHighOrderAmbisonics3rdOrderEngine;
}

#endif // JAZZ_HIGH_ORDER_AMBISONICS_3RD_ORDER_ENGINE_H
