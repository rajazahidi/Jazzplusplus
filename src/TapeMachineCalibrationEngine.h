#ifndef JAZZ_TAPE_MACHINE_CALIBRATION_ENGINE_H
#define JAZZ_TAPE_MACHINE_CALIBRATION_ENGINE_H

#include <vector>
#include <string>
#include <cmath>

namespace Jazz {

enum class TapeStandardEq {
    NAB,      // National Association of Broadcasters (USA / Japan)
    CCIR_IEC1 // International Electrotechnical Commission (Europe)
};

enum class TapeMachineModel {
    STUDER_A800,
    AMPEX_ATR102,
    OTARI_MTR90
};

enum class CalibrationTone {
    TONE_OFF,
    TONE_1KHZ_REF_LEVEL,  // 1000 Hz 0 VU Operating Level Alignment
    TONE_10KHZ_AZIMUTH    // 10000 Hz High-Frequency Phase/Azimuth Alignment
};

class JZTapeMachineCalibrationEngine {
public:
    JZTapeMachineCalibrationEngine(float sampleRate = 44100.0f);
    ~JZTapeMachineCalibrationEngine() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return m_sampleRate; }

    // Machine Model & EQ Standards
    void setMachineModel(TapeMachineModel model);
    TapeMachineModel getMachineModel() const { return m_model; }

    void setEqualizationStandard(TapeStandardEq eq);
    TapeStandardEq getEqualizationStandard() const { return m_eqStandard; }

    // Operating Fluxivity (nWb/m - nanoWebers per meter)
    // 185 = Ampex Standard (0 VU)
    // 250 = +3 dB elevated level (Quantegy 456)
    // 320 = +6 dB elevated level (Ampex 499)
    // 355 = +9 dB elevated level (Quantegy GP9 / ATR Master Tape)
    void setOperatingFluxivity(float nWbm);
    float getOperatingFluxivity() const { return m_fluxivityNwbm; }

    // Tape Speed (7.5, 15.0, 30.0 IPS)
    void setTapeSpeedIps(float speedIps);
    float getTapeSpeedIps() const { return m_speedIps; }

    // Bias Trim & Headroom Drive
    void setBiasAdjustmentDb(float biasDb); // -3.0 to +3.0 dB
    float getBiasAdjustmentDb() const { return m_biasDb; }

    void setInputDriveDb(float driveDb);
    float getInputDriveDb() const { return m_inputDriveDb; }

    // Calibration Tone Generator
    void setCalibrationTone(CalibrationTone tone);
    CalibrationTone getCalibrationTone() const { return m_calTone; }

    // Real-Time Audio Processing
    void process(float inL, float inR, float& outL, float& outR);
    void processBlock(float* left, float* right, int numSamples);

    // Flux Level Metering (Operating Level in nWb/m)
    float getInstantaneousFluxNwbm() const { return m_currentFluxMeter; }

private:
    float m_sampleRate;
    TapeMachineModel m_model;
    TapeStandardEq m_eqStandard;
    float m_fluxivityNwbm;
    float m_speedIps;
    float m_biasDb;
    float m_inputDriveDb;
    CalibrationTone m_calTone;

    float m_tonePhase;
    float m_currentFluxMeter;

    // Filter states for NAB / CCIR pre-emphasis and de-emphasis
    float m_preL, m_preR;
    float m_postL, m_postR;

    float applySaturation(float input, float driveLinear);
};

} // namespace Jazz

#endif // JAZZ_TAPE_MACHINE_CALIBRATION_ENGINE_H
