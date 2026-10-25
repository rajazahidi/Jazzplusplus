//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// DynamicSubBassSpectralCarverEngine.h - Dynamic Kick-to-Bass Fundamental
// Frequency Surgical Sidechain Carver
//
// Dynamic low-end frequency unmasking between kick drum and bass guitar
// (Waves Factory TrackSpacer / FabFilter Pro-MB surgical sidechain carve style):
// Real-time kick fundamental frequency pitch tracking (35Hz to 120Hz),
// surgical dynamic notch carving (0dB to -24dB) out of target bass track during kick transient,
// ultra-fast recovery avoiding broadband pumping, and Carve Delta Solo mode.
//*****************************************************************************

#ifndef JAZZ_DYNAMIC_SUB_BASS_SPECTRAL_CARVER_ENGINE_H
#define JAZZ_DYNAMIC_SUB_BASS_SPECTRAL_CARVER_ENGINE_H

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

class JZDynamicSubBassSpectralCarverEngine {
public:
    struct Parameters {
        float carveDepthDb = -12.0f;         // Maximum carve attenuation depth (-3.0 to -24.0 dB)
        float carveQ = 5.0f;                 // Surgical notch selectivity Q (2.0 to 12.0)
        float sensitivity = 1.0f;            // Sidechain trigger sensitivity (0.2 to 3.0)
        float attackMs = 1.5f;               // Rapid carve attack (0.5 to 10.0 ms)
        float releaseMs = 65.0f;             // Punchy bass recovery release (20.0 to 250.0 ms)
        float minCarveFreqHz = 35.0f;        // Sub-bass search window (30.0 to 70.0 Hz)
        float maxCarveFreqHz = 130.0f;       // Upper sub-bass window (80.0 to 250.0 Hz)
        bool carveDeltaSolo = false;         // Audition carved audio portion only
        bool bypass = false;
    };

    JZDynamicSubBassSpectralCarverEngine(float sampleRate = 48000.0f);
    ~JZDynamicSubBassSpectralCarverEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setCarveDepthDb(float db);
    float getCarveDepthDb() const { return m_params.carveDepthDb; }

    void setCarveQ(float q);
    float getCarveQ() const { return m_params.carveQ; }

    void setSensitivity(float sens);
    float getSensitivity() const { return m_params.sensitivity; }

    void setReleaseMs(float ms);
    float getReleaseMs() const { return m_params.releaseMs; }

    void setCarveDeltaSolo(bool solo);
    bool isCarveDeltaSolo() const { return m_params.carveDeltaSolo; }

    void setBypass(bool bp);
    bool isBypassed() const { return m_params.bypass; }

    /**
     * @brief Process stereo bass audio buffer with external stereo sidechain kick input.
     * @param inBassL In-place or separate bass track input Left
     * @param inBassR Bass track input Right
     * @param sideKickL Sidechain kick drum track input Left
     * @param sideKickR Sidechain kick drum track input Right
     * @param outBassL Carved bass output Left
     * @param outBassR Carved bass output Right
     * @param numSamples Number of stereo frames
     */
    void processWithSidechain(const float* inBassL, const float* inBassR,
                              const float* sideKickL, const float* sideKickR,
                              float* outBassL, float* outBassR,
                              size_t numSamples);

    /**
     * @brief Internal self-sidechain fallback process.
     */
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

    // Real-time Telemetry
    float getTrackedKickFundamentalHz() const { return m_trackedKickFundamentalHz; }
    float getCurrentCarveDepthDb() const { return m_currentCarveDepthDb; }
    bool isKickTriggerActive() const { return m_kickTriggerActive; }

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    float m_trackedKickFundamentalHz = 55.0f;
    float m_currentCarveDepthDb = 0.0f;
    bool m_kickTriggerActive = false;

    float m_kickEnvelope = 0.0f;
    float m_prevSideSample = 0.0f;
    float m_zeroCrossingPeriod = 0.0f;

    // Surgical Carve Notch Biquad coefficients & states
    float m_carveB0 = 1.0f, m_carveB1 = 0.0f, m_carveB2 = 0.0f;
    float m_carveA1 = 0.0f, m_carveA2 = 0.0f;
    float m_carveX1L = 0.0f, m_carveX2L = 0.0f, m_carveY1L = 0.0f, m_carveY2L = 0.0f;
    float m_carveX1R = 0.0f, m_carveX2R = 0.0f, m_carveY1R = 0.0f, m_carveY2R = 0.0f;

    void updateCarveFilter(float fundamentalHz, float depthDb);
};

} // namespace Jazz

namespace jazz {
    using JZDynamicSubBassSpectralCarverEngine = Jazz::JZDynamicSubBassSpectralCarverEngine;
}

#endif // JAZZ_DYNAMIC_SUB_BASS_SPECTRAL_CARVER_ENGINE_H
