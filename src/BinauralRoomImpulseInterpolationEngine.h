//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// BinauralRoomImpulseInterpolationEngine.h - BRIR Convolver & SOFA Interpolator
//
// 16-channel Binaural Room Impulse Response (BRIR) convolution engine for Dolby
// Atmos spatial headphone monitoring (Smyth Realiser A16 style). Features continuous
// SOFA spherical coordinate rotation (Yaw, Pitch, Roll) and 3D control room acoustic
// modeling for precision immersive monitoring.
//*****************************************************************************

#ifndef JAZZ_BINAURAL_ROOM_IMPULSE_INTERPOLATION_ENGINE_H
#define JAZZ_BINAURAL_ROOM_IMPULSE_INTERPOLATION_ENGINE_H

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

enum class BrirRoomPreset {
    STUDIO_CONTROL_ROOM_A = 0,  // Ultra-precise mastering control room
    CINEMA_DUB_STAGE = 1,       // Large theater Dolby Atmos dub stage
    CUSTOM_SOFA_PROFILE = 2     // Measured custom user HRTF/BRIR
};

class JZBinauralRoomImpulseInterpolationEngine {
public:
    static constexpr size_t NUM_CHANNELS = 16;
    static constexpr size_t BRIR_TAP_COUNT = 64;

    struct SpeakerCoordinate {
        float azimuthDeg;   // Azimuth angle (-180 to +180)
        float elevationDeg; // Elevation angle (-90 to +90)
        float distanceM;    // Distance in meters
        const char* name;
    };

    struct Parameters {
        BrirRoomPreset roomPreset = BrirRoomPreset::STUDIO_CONTROL_ROOM_A;
        float yawDeg = 0.0f;                 // Head orientation Yaw (-180 to +180 deg)
        float pitchDeg = 0.0f;               // Head orientation Pitch (-90 to +90 deg)
        float rollDeg = 0.0f;                // Head orientation Roll (-90 to +90 deg)
        float roomDecaySec = 0.35f;          // RT60 early reflection decay (0.1 to 1.2 sec)
        float dryWet = 1.0f;
        bool bypassed = false;
    };

    JZBinauralRoomImpulseInterpolationEngine(float sampleRate = 48000.0f);
    ~JZBinauralRoomImpulseInterpolationEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setRoomPreset(BrirRoomPreset preset);
    BrirRoomPreset getRoomPreset() const { return m_params.roomPreset; }

    void setHeadOrientation(float yaw, float pitch, float roll);
    float getYawDeg() const { return m_params.yawDeg; }
    float getPitchDeg() const { return m_params.pitchDeg; }
    float getRollDeg() const { return m_params.rollDeg; }

    void setRoomDecaySec(float sec);
    float getRoomDecaySec() const { return m_params.roomDecaySec; }

    void setDryWet(float wet);
    float getDryWet() const { return m_params.dryWet; }

    void setBypass(bool bypass);
    bool isBypassed() const { return m_params.bypassed; }

    // Telemetry & Getters
    size_t getActiveChannelCount() const { return NUM_CHANNELS; }
    std::string getRoomPresetName() const;

    // Audio Processing
    void process16ChannelsToBinaural(const float* const* in16Channels,
                                     float* outL, float* outR,
                                     size_t numSamples);

private:
    void rebuildBrirImpulses();

    float m_sampleRate = 48000.0f;
    Parameters m_params;

    // Pre-calculated BRIR convolution impulse kernels for Left and Right ears per channel
    std::vector<float> m_kernelL[NUM_CHANNELS];
    std::vector<float> m_kernelR[NUM_CHANNELS];

    // Delay lines for short BRIR convolution
    static constexpr size_t CONV_BUF_SIZE = 128;
    std::vector<float> m_convBuffers[NUM_CHANNELS];
    size_t m_convWritePos = 0;
};

} // namespace Jazz

namespace jazz {
    using JZBinauralRoomImpulseInterpolationEngine = Jazz::JZBinauralRoomImpulseInterpolationEngine;
    using BrirRoomPreset = Jazz::BrirRoomPreset;
}

#endif // JAZZ_BINAURAL_ROOM_IMPULSE_INTERPOLATION_ENGINE_H
