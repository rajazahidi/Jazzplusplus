//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// TapeHysteresisAzimuthEngine.h - Jiles-Atherton Tape Hysteresis & Azimuth Skew
//
// Models magnetic domain physics via non-linear Jiles-Atherton hysteresis loops
// (remanence, saturation, coercivity) combined with automatic stereo tape head
// azimuth skew phase alignment (Studer A800 / Ampex ATR-102 calibration).
//*****************************************************************************

#ifndef JAZZ_TAPE_HYSTERESIS_AZIMUTH_ENGINE_H
#define JAZZ_TAPE_HYSTERESIS_AZIMUTH_ENGINE_H

#include <vector>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

enum class TapeSpeed {
    IPS_7_5 = 0,  // 7.5 inches per second (warm, compressed, gentle top roll-off)
    IPS_15  = 1,  // 15 ips (industry studio standard punch & bass head-bump)
    IPS_30  = 2   // 30 ips (audiophile mastering, ultra-linear top end)
};

class JZTapeHysteresisAzimuthEngine {
public:
    struct Parameters {
        float driveDb = 3.0f;             // Input magnetic drive (-12 dB to +18 dB)
        float saturation = 0.75f;         // Magnetic flux saturation limit
        float remanence = 0.35f;          // Ferromagnetic remanence / memory
        TapeSpeed speed = TapeSpeed::IPS_15;
        bool autoAzimuthAlign = true;     // Sub-sample stereo phase skew alignment
        float manualAzimuthSkewUs = 0.0f; // Manual azimuth phase adjustment (-100 to +100 us)
    };

    JZTapeHysteresisAzimuthEngine(float sampleRate = 48000.0f);
    ~JZTapeHysteresisAzimuthEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setDriveDb(float db);
    float getDriveDb() const { return m_params.driveDb; }

    void setTapeSpeed(TapeSpeed speed);
    TapeSpeed getTapeSpeed() const { return m_params.speed; }

    void setAutoAzimuthAlign(bool enable);
    bool isAutoAzimuthAlign() const { return m_params.autoAzimuthAlign; }

    /**
     * @brief Process stereo audio buffer through Jiles-Atherton magnetic simulation and azimuth alignment.
     */
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

    float getDetectedSkewSamples() const { return m_detectedSkewSamples; }
    float getDetectedSkewMicroseconds() const;
    float getMagneticFluxSaturationPct() const { return m_fluxSaturationPct; }

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    // Jiles-Atherton magnetic domain state (magnetization M and magnetic field H)
    float m_magStateL = 0.0f;
    float m_magStateR = 0.0f;
    float m_prevInL = 0.0f;
    float m_prevInR = 0.0f;

    // Fractional delay line for stereo azimuth skew alignment
    std::vector<float> m_azimuthDelayBufR;
    size_t m_azimuthWritePos = 0;
    static const size_t AZIMUTH_BUF_SIZE = 128;
    float m_detectedSkewSamples = 0.0f;
    float m_smoothedSkewSamples = 0.0f;
    float m_fluxSaturationPct = 0.0f;

    // Head-bump low-frequency resonant filter
    float m_headBumpState1L = 0.0f, m_headBumpState2L = 0.0f;
    float m_headBumpState1R = 0.0f, m_headBumpState2R = 0.0f;

    // Jiles-Atherton differential solver step
    float solveJilesAtherton(float inSample, float prevIn, float& magState);
    void updateAzimuthAlignment(const float* inL, const float* inR, size_t numSamples);
};

} // namespace Jazz

namespace jazz {
    using JZTapeHysteresisAzimuthEngine = Jazz::JZTapeHysteresisAzimuthEngine;
}

#endif // JAZZ_TAPE_HYSTERESIS_AZIMUTH_ENGINE_H
