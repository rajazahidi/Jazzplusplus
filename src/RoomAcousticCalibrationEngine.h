//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZRoomAcousticCalibrationEngine.h - Room & Headphone Acoustic Calibration
//
// Studio acoustic room calibration (Farina log-sine sweep deconvolution),
// multi-target EQ curves (Harman, B&K 1974, Cinema X-Curve, Flat Reference),
// standing-wave resonant mode inverse IIR/FIR filter bank, and SOFA
// binaural headphone correction profiles.
//*****************************************************************************

#ifndef JAZZ_ROOM_ACOUSTIC_CALIBRATION_ENGINE_H
#define JAZZ_ROOM_ACOUSTIC_CALIBRATION_ENGINE_H

#include <vector>
#include <string>
#include <map>
#include <cmath>

namespace Jazz {

enum class AcousticTargetCurve {
    FLAT_REFERENCE,      // 0 dB perfectly flat
    HARMAN_STUDIO_CURVE, // +4.0 dB bass rise (<100Hz), -0.8 dB/oct high roll-off (>2.5kHz)
    BK_1974_CURVE,       // Brüel & Kjær standard studio listening curve
    CINEMA_X_CURVE       // SMPTE 202M theatrical cinema auditorium curve
};

struct AcousticModeFilter {
    float centerFreqHz;
    float deviationDb;     // Measured room anomaly (+dB peak or -dB null)
    float correctionGainDb;// Inverse correction gain (clamped by max boost limit)
    float qFactor;
};

struct RoomCalibrationProfile {
    std::string profileName;
    AcousticTargetCurve targetCurve;
    std::vector<AcousticModeFilter> filters;
    float maxBoostLimitDb;
    float averageDeviationDb;
};

class JZRoomAcousticCalibrationEngine {
public:
    JZRoomAcousticCalibrationEngine(float sampleRate = 48000.0f);
    ~JZRoomAcousticCalibrationEngine() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return m_sampleRate; }

    // Log-Sine Sweep Generator (Farina Method 20Hz - 20,000Hz)
    void generateLogSineSweep(std::vector<float>& outSweep, float durationSec = 3.0f,
                             float startFreq = 20.0f, float endFreq = 20000.0f);

    // Room Impulse Response Analysis & Inverse EQ Synthesis
    bool analyzeImpulseResponse(const std::vector<float>& recordedSweep,
                                RoomCalibrationProfile& outProfile);

    // Target Curve & Headphone Profile
    void setTargetCurve(AcousticTargetCurve curve);
    AcousticTargetCurve getTargetCurve() const { return m_targetCurve; }

    void setMaxBoostLimitDb(float limitDb);
    float getMaxBoostLimitDb() const { return m_maxBoostLimitDb; }

    void loadHeadphoneCorrectionProfile(const std::string& modelName,
                                        const std::vector<float>& targetFrequencies,
                                        const std::vector<float>& eqGainsDb);
    bool hasHeadphoneProfile(const std::string& modelName) const;

    // Real-Time Monitor Filtering
    void process(float& left, float& right);
    void setBypass(bool bypass) { m_bypass = bypass; }
    bool isBypassed() const { return m_bypass; }

    void reset();

private:
    float m_sampleRate;
    AcousticTargetCurve m_targetCurve;
    float m_maxBoostLimitDb;
    bool m_bypass;

    std::vector<AcousticModeFilter> m_activeFilters;
    std::map<std::string, std::vector<AcousticModeFilter>> m_headphoneProfiles;

    // Biquad filter states for stereo channels
    std::vector<float> m_stateX1L, m_stateX2L, m_stateY1L, m_stateY2L;
    std::vector<float> m_stateX1R, m_stateX2R, m_stateY1R, m_stateY2R;

    void updateFilterStates();
    float getTargetGainAtFrequency(float freqHz) const;
};

} // namespace Jazz

#endif // JAZZ_ROOM_ACOUSTIC_CALIBRATION_ENGINE_H
