//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// ImmersiveFolddownDownmixEngine.h - Immersive Audio Folddown & Downmixing
//
// ITU-R BS.775-3 & Dolby Atmos intelligent downmix matrix engine (9.1.6 / 7.1.4
// to 5.1 surround and 2.0 stereo). Features center channel acoustic buildup
// compensation, 90-degree Hilbert phase steering on surround/height channels
// to prevent comb filtering, and output true-peak summing protection.
//*****************************************************************************

#ifndef JAZZ_IMMERSIVE_FOLDDOWN_DOWNMIX_ENGINE_H
#define JAZZ_IMMERSIVE_FOLDDOWN_DOWNMIX_ENGINE_H

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

enum class DownmixTargetMode {
    STEREO_2_0 = 1,
    SURROUND_5_1 = 2
};

enum class LfeFolddownMode {
    LFE_DISCARD = 0,         // ITU-R standard for stereo: prevent monitor overload
    LFE_ATTENUATE_10DB = 1,  // Fold LFE into Left/Right with -10 dB pad
    LFE_PASSTHROUGH = 2      // In 5.1 mode, keep dedicated LFE channel
};

class JZImmersiveFolddownDownmixEngine {
public:
    static constexpr size_t NUM_INPUT_CHANNELS = 16;
    static constexpr size_t NUM_5_1_CHANNELS = 6;

    struct Parameters {
        DownmixTargetMode targetMode = DownmixTargetMode::STEREO_2_0;
        float centerTrimDb = -3.0f;          // Center channel buildup trim (-3.0, -4.5, -6.0 dB)
        float surroundTrimDb = -3.0f;        // Surround bed folddown level (-3.0 to -6.0 dB)
        float heightTrimDb = -4.5f;          // Height ceiling folddown level (-3.0 to -9.0 dB)
        LfeFolddownMode lfeMode = LfeFolddownMode::LFE_DISCARD;
        bool enableHilbertPhaseSteering = true; // 90° phase shift on height/surround to prevent cancellation
        bool enableOutputLimiter = true;     // Soft-knee safety limiter against summing overs
        bool bypassed = false;
    };

    JZImmersiveFolddownDownmixEngine(float sampleRate = 48000.0f);
    ~JZImmersiveFolddownDownmixEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setTargetMode(DownmixTargetMode mode);
    DownmixTargetMode getTargetMode() const { return m_params.targetMode; }

    void setCenterTrimDb(float db);
    float getCenterTrimDb() const { return m_params.centerTrimDb; }

    void setSurroundTrimDb(float db);
    float getSurroundTrimDb() const { return m_params.surroundTrimDb; }

    void setHeightTrimDb(float db);
    float getHeightTrimDb() const { return m_params.heightTrimDb; }

    void setLfeMode(LfeFolddownMode mode);
    LfeFolddownMode getLfeMode() const { return m_params.lfeMode; }

    void setHilbertPhaseSteering(bool enable);
    bool isHilbertPhaseSteeringEnabled() const { return m_params.enableHilbertPhaseSteering; }

    void setOutputLimiter(bool enable);
    bool isOutputLimiterEnabled() const { return m_params.enableOutputLimiter; }

    void setBypass(bool bypass);
    bool isBypassed() const { return m_params.bypassed; }

    // Telemetry & Getters
    float getOutputPeakLevelDb() const { return m_outputPeakLevelDb; }
    size_t getActiveChannelCount() const { return NUM_INPUT_CHANNELS; }
    std::string getChannelName(size_t index) const;

    // Audio Processing
    void processToStereo(const float* const* in16Channels,
                         float* outL, float* outR,
                         size_t numSamples);

    void processTo51(const float* const* in16Channels,
                     float* const* out6Channels,
                     size_t numSamples);

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    // Hilbert 90-degree phase shift allpass network states for height/surround channels
    float m_hilbertState[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    float m_outputPeakLevelDb = -60.0f;
};

} // namespace Jazz

namespace jazz {
    using JZImmersiveFolddownDownmixEngine = Jazz::JZImmersiveFolddownDownmixEngine;
    using DownmixTargetMode = Jazz::DownmixTargetMode;
    using LfeFolddownMode = Jazz::LfeFolddownMode;
}

#endif // JAZZ_IMMERSIVE_FOLDDOWN_DOWNMIX_ENGINE_H
