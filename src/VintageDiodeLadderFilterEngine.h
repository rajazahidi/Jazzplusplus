//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// VintageDiodeLadderFilterEngine.h - Historic 18dB/24dB Diode Ladder
// Non-Linear Resonant Filter Engine
//
// Circuit-accurate physical modeling of the legendary 1971 EMS VCS3, Synthi,
// and 1982 Roland TB-303 diode ladder filter:
// 4-stage diode ladder network with non-linear Shockley diode conduction,
// asymmetrical dynamic clipping, acid resonance squelch with low-end retention,
// accent overdrive punch, and thermal voltage drift.
//*****************************************************************************

#ifndef JAZZ_VINTAGE_DIODE_LADDER_FILTER_ENGINE_H
#define JAZZ_VINTAGE_DIODE_LADDER_FILTER_ENGINE_H

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <array>
#include <algorithm>

namespace Jazz {

enum class DiodeLadderSlope {
    SLOPE_18DB_OCT = 0,   // Classic 3-pole 18dB/octave (Roland TB-303 acid character)
    SLOPE_24DB_OCT        // 4-pole 24dB/octave (EMS VCS3 / Synthi steep cut)
};

class JZVintageDiodeLadderFilterEngine {
public:
    struct Parameters {
        float cutoffHz = 1000.0f;            // Cutoff frequency (20.0 to 20000.0 Hz)
        float resonance = 0.75f;             // Resonant squelch feedback (0.0 to 1.15)
        float driveDb = 3.0f;                // Pre-filter drive (-6.0 to +18.0 dB)
        float accentPunch = 0.5f;            // Dynamic acid accent boost (0.0 to 1.0)
        float bassCompensation = 0.65f;      // Bass pass-through retention at high resonance (0.0 to 1.0)
        DiodeLadderSlope slope = DiodeLadderSlope::SLOPE_18DB_OCT;
        bool bypass = false;
    };

    JZVintageDiodeLadderFilterEngine(float sampleRate = 48000.0f);
    ~JZVintageDiodeLadderFilterEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setCutoffHz(float hz);
    float getCutoffHz() const { return m_params.cutoffHz; }

    void setResonance(float res);
    float getResonance() const { return m_params.resonance; }

    void setDriveDb(float db);
    float getDriveDb() const { return m_params.driveDb; }

    void setAccentPunch(float punch);
    float getAccentPunch() const { return m_params.accentPunch; }

    void setBassCompensation(float comp);
    float getBassCompensation() const { return m_params.bassCompensation; }

    void setSlope(DiodeLadderSlope slope);
    DiodeLadderSlope getSlope() const { return m_params.slope; }

    void setBypass(bool bp);
    bool isBypassed() const { return m_params.bypass; }

    /**
     * @brief Process stereo audio buffer through vintage diode ladder filter.
     */
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

    // Telemetry
    float getThdPercent() const { return m_thdPercent; }
    bool isSelfOscillating() const { return m_params.resonance >= 1.0f; }

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    float m_thdPercent = 1.2f;

    // 4 Capacitor state variables for diode ladder (Left and Right)
    std::array<float, 4> m_stateL = {0.0f, 0.0f, 0.0f, 0.0f};
    std::array<float, 4> m_stateR = {0.0f, 0.0f, 0.0f, 0.0f};

    // Non-linear Shockley diode saturation function
    static float diodeNonLinearity(float v);
};

} // namespace Jazz

namespace jazz {
    using JZVintageDiodeLadderFilterEngine = Jazz::JZVintageDiodeLadderFilterEngine;
}

#endif // JAZZ_VINTAGE_DIODE_LADDER_FILTER_ENGINE_H
