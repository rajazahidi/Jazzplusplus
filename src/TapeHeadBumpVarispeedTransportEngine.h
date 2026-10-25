//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// TapeHeadBumpVarispeedTransportEngine.h - Tape Head-Bump & Varispeed Engine
//
// Master tape electro-magnetic head gap geometry simulation (Studer A800 style):
// Speed-dependent head-bump bass resonance (55Hz at 15 IPS, 110Hz at 30 IPS),
// combined with continuous motor varispeed scrubbing and tape-stop wind-down.
//*****************************************************************************

#ifndef JAZZ_TAPE_HEAD_BUMP_VARISPEED_TRANSPORT_ENGINE_H
#define JAZZ_TAPE_HEAD_BUMP_VARISPEED_TRANSPORT_ENGINE_H

#include <vector>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

class JZTapeHeadBumpVarispeedTransportEngine {
public:
    struct Parameters {
        float tapeSpeedIps = 15.0f;          // 7.5, 15.0, or 30.0 IPS
        float headBumpAmount = 1.0f;         // Bass resonance boost (0.0 to 2.0)
        float varispeedFactor = 1.0f;        // Speed factor (0.0 = stopped, 1.0 = normal, 2.0 = fast)
        float tapeStopDecaySec = 1.2f;       // Motor brake inertia ramp (0.2 to 4.0 sec)
        bool tapeStopActive = false;         // Trigger wind-down stop
        float dryWet = 1.0f;
    };

    JZTapeHeadBumpVarispeedTransportEngine(float sampleRate = 48000.0f);
    ~JZTapeHeadBumpVarispeedTransportEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setTapeSpeedIps(float ips);
    float getTapeSpeedIps() const { return m_params.tapeSpeedIps; }

    void setHeadBumpAmount(float amount);
    float getHeadBumpAmount() const { return m_params.headBumpAmount; }

    void setVarispeedFactor(float factor);
    float getVarispeedFactor() const { return m_params.varispeedFactor; }

    void triggerTapeStop(bool stop);
    bool isTapeStopActive() const { return m_params.tapeStopActive; }

    /**
     * @brief Process stereo audio buffer through tape head-bump and transport engine.
     */
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

    float getHeadBumpCenterHz() const { return m_headBumpCenterHz; }
    float getCurrentSpeed() const { return m_currentMotorSpeed; }

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    // Head-bump peaking biquad filter
    float m_bumpB0 = 1.0f, m_bumpB1 = 0.0f, m_bumpB2 = 0.0f, m_bumpA1 = 0.0f, m_bumpA2 = 0.0f;
    float m_bumpX1L = 0.0f, m_bumpX2L = 0.0f, m_bumpY1L = 0.0f, m_bumpY2L = 0.0f;
    float m_bumpX1R = 0.0f, m_bumpX2R = 0.0f, m_bumpY1R = 0.0f, m_bumpY2R = 0.0f;

    // Varispeed fractional circular resampler buffer
    static const size_t VARISPEED_BUF_SIZE = 8192;
    std::vector<float> m_tapeBufL;
    std::vector<float> m_tapeBufR;
    size_t m_writePos = 0;
    float m_readPos = 0.0f;

    float m_headBumpCenterHz = 55.0f;
    float m_currentMotorSpeed = 1.0f;

    void updateHeadBumpFilter();
};

} // namespace Jazz

namespace jazz {
    using JZTapeHeadBumpVarispeedTransportEngine = Jazz::JZTapeHeadBumpVarispeedTransportEngine;
}

#endif // JAZZ_TAPE_HEAD_BUMP_VARISPEED_TRANSPORT_ENGINE_H
