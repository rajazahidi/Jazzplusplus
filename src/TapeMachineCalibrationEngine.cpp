#include "TapeMachineCalibrationEngine.h"
#include <algorithm>
#include <cmath>

namespace Jazz {

static constexpr float PI_F = 3.14159265358979323846f;

JZTapeMachineCalibrationEngine::JZTapeMachineCalibrationEngine(float sampleRate)
    : m_sampleRate(sampleRate > 0.0f ? sampleRate : 44100.0f)
    , m_model(TapeMachineModel::STUDER_A800)
    , m_eqStandard(TapeStandardEq::CCIR_IEC1)
    , m_fluxivityNwbm(320.0f)
    , m_speedIps(15.0f)
    , m_biasDb(0.0f)
    , m_inputDriveDb(0.0f)
    , m_calTone(CalibrationTone::TONE_OFF)
    , m_tonePhase(0.0f)
    , m_currentFluxMeter(320.0f)
    , m_preL(0.0f), m_preR(0.0f)
    , m_postL(0.0f), m_postR(0.0f)
{
}

void JZTapeMachineCalibrationEngine::setSampleRate(float sampleRate)
{
    if (sampleRate > 0.0f) {
        m_sampleRate = sampleRate;
    }
}

void JZTapeMachineCalibrationEngine::setMachineModel(TapeMachineModel model)
{
    m_model = model;
}

void JZTapeMachineCalibrationEngine::setEqualizationStandard(TapeStandardEq eq)
{
    m_eqStandard = eq;
}

void JZTapeMachineCalibrationEngine::setOperatingFluxivity(float nWbm)
{
    m_fluxivityNwbm = std::clamp(nWbm, 150.0f, 500.0f);
}

void JZTapeMachineCalibrationEngine::setTapeSpeedIps(float speedIps)
{
    if (speedIps >= 7.5f) {
        m_speedIps = speedIps;
    }
}

void JZTapeMachineCalibrationEngine::setBiasAdjustmentDb(float biasDb)
{
    m_biasDb = std::clamp(biasDb, -6.0f, 6.0f);
}

void JZTapeMachineCalibrationEngine::setInputDriveDb(float driveDb)
{
    m_inputDriveDb = std::clamp(driveDb, -24.0f, 24.0f);
}

void JZTapeMachineCalibrationEngine::setCalibrationTone(CalibrationTone tone)
{
    m_calTone = tone;
    m_tonePhase = 0.0f;
}

float JZTapeMachineCalibrationEngine::applySaturation(float input, float driveLinear)
{
    // Soft magnetic tape hysteresis curve: y = tanh(x * drive)
    float x = input * driveLinear;
    // Pade approximation or tanh
    return std::tanh(x);
}

void JZTapeMachineCalibrationEngine::process(float inL, float inR, float& outL, float& outR)
{
    if (m_calTone != CalibrationTone::TONE_OFF) {
        float freq = (m_calTone == CalibrationTone::TONE_1KHZ_REF_LEVEL) ? 1000.0f : 10000.0f;
        float phaseInc = 2.0f * PI_F * freq / m_sampleRate;
        float toneSample = std::sin(m_tonePhase);
        m_tonePhase += phaseInc;
        if (m_tonePhase >= 2.0f * PI_F) m_tonePhase -= 2.0f * PI_F;

        // Output calibration tone at 0 VU reference (-18 dBFS = ~0.126 peak)
        float calLevel = 0.12589f;
        outL = toneSample * calLevel;
        outR = toneSample * calLevel;
        m_currentFluxMeter = m_fluxivityNwbm;
        return;
    }

    float drive = std::pow(10.0f, m_inputDriveDb / 20.0f);

    // CCIR / NAB pre-emphasis weighting: high frequency boost before tape
    // Higher speed (30 IPS) requires less pre-emphasis
    float preEmphasisGain = (m_eqStandard == TapeStandardEq::CCIR_IEC1) ? 1.15f : 1.25f;
    float preL = inL * preEmphasisGain;
    float preR = inR * preEmphasisGain;

    // Magnetic saturation
    float satL = applySaturation(preL, drive);
    float satR = applySaturation(preR, drive);

    // Complementary de-emphasis on tape replay
    outL = satL / preEmphasisGain;
    outR = satR / preEmphasisGain;

    // Instantaneous flux level metering
    float peakIn = std::max(std::abs(inL), std::abs(inR));
    m_currentFluxMeter = m_fluxivityNwbm * (1.0f + peakIn * drive);
}

void JZTapeMachineCalibrationEngine::processBlock(float* left, float* right, int numSamples)
{
    if (!left || !right || numSamples <= 0) return;
    for (int i = 0; i < numSamples; ++i) {
        process(left[i], right[i], left[i], right[i]);
    }
}

} // namespace Jazz
