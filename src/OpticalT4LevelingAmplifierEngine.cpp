//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// OpticalT4LevelingAmplifierEngine.cpp - T4 Optical Attenuator Leveling Amplifier
//*****************************************************************************

#include "OpticalT4LevelingAmplifierEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

static constexpr float PI_F = 3.14159265358979323846f;

JZOpticalT4LevelingAmplifierEngine::JZOpticalT4LevelingAmplifierEngine(float sampleRate)
    : m_sampleRate(sampleRate)
{
    initialize(sampleRate);
}

void JZOpticalT4LevelingAmplifierEngine::initialize(float sampleRate)
{
    m_sampleRate = (sampleRate > 8000.0f) ? sampleRate : 48000.0f;
    reset();

    // Sidechain high-pass emphasis filter (R37 trimmer)
    float omega = 2.0f * PI_F * 2200.0f / m_sampleRate;
    m_emphHpA = 1.0f / (1.0f + omega);
}

void JZOpticalT4LevelingAmplifierEngine::reset()
{
    m_fastStage = 0.0f;
    m_slowStage = 0.0f;
    m_memoryHistory = 0.0f;
    m_photocellLuminescence = 0.0f;
    m_currentGrDb = 0.0f;
    m_emphState = 0.0f;
}

void JZOpticalT4LevelingAmplifierEngine::setParameters(const Parameters& params)
{
    m_params = params;
    m_params.peakReduction = std::max(0.0f, std::min(m_params.peakReduction, 100.0f));
    m_params.outputGain = std::max(0.0f, std::min(m_params.outputGain, 100.0f));
    m_params.tubeDrive = std::max(0.5f, std::min(m_params.tubeDrive, 2.5f));
    m_params.dryWet = std::max(0.0f, std::min(m_params.dryWet, 1.0f));
}

void JZOpticalT4LevelingAmplifierEngine::setPeakReduction(float pr)
{
    m_params.peakReduction = std::max(0.0f, std::min(pr, 100.0f));
}

void JZOpticalT4LevelingAmplifierEngine::setOutputGain(float gain)
{
    m_params.outputGain = std::max(0.0f, std::min(gain, 100.0f));
}

void JZOpticalT4LevelingAmplifierEngine::setMode(OpticalLevelerMode mode)
{
    m_params.mode = mode;
}

void JZOpticalT4LevelingAmplifierEngine::setEmphasis(float emphasis)
{
    m_params.emphasis = std::max(0.0f, std::min(emphasis, 1.0f));
}

void JZOpticalT4LevelingAmplifierEngine::setTubeDrive(float drive)
{
    m_params.tubeDrive = std::max(0.5f, std::min(drive, 2.5f));
}

void JZOpticalT4LevelingAmplifierEngine::setDryWet(float wet)
{
    m_params.dryWet = std::max(0.0f, std::min(wet, 1.0f));
}

void JZOpticalT4LevelingAmplifierEngine::setBypass(bool bypass)
{
    m_params.bypassed = bypass;
}

float JZOpticalT4LevelingAmplifierEngine::getThdPercent() const
{
    float driveFactor = m_params.tubeDrive - 0.5f;
    float grFactor = std::abs(m_currentGrDb) * 0.06f;
    return std::min(4.5f, 0.08f + driveFactor * 0.25f + grFactor * 0.12f);
}

void JZOpticalT4LevelingAmplifierEngine::process(
    const float* inL, const float* inR,
    float* outL, float* outR,
    size_t numSamples)
{
    if (!inL || !inR || !outL || !outR || numSamples == 0) return;

    if (m_params.bypassed) {
        std::copy(inL, inL + numSamples, outL);
        std::copy(inR, inR + numSamples, outR);
        return;
    }

    // T4 Optical ballistics coefficients
    float attackCoeff = 1.0f - std::exp(-1000.0f / (10.0f * m_sampleRate));    // 10ms attack
    float fastReleaseCoeff = 1.0f - std::exp(-1000.0f / (60.0f * m_sampleRate)); // 60ms fast recovery
    float slowBaseSec = 1.5f; // Base slow release (1.5s to 4.5s memory dependent)

    float prScaled = (m_params.peakReduction / 100.0f) * 2.8f;
    float makeupLinear = std::pow(10.0f, (m_params.outputGain - 30.0f) / 20.0f);
    float drive = m_params.tubeDrive;

    for (size_t i = 0; i < numSamples; ++i) {
        float xL = inL[i];
        float xR = inR[i];
        float scMono = (std::abs(xL) + std::abs(xR)) * 0.5f * prScaled;

        // Emphasis highpass filter (R37 trimmer boosts high frequencies into T4 cell)
        if (m_params.emphasis > 0.0f) {
            float hp = m_emphHpA * (m_emphState + scMono);
            m_emphState = hp - scMono;
            scMono = scMono * (1.0f - m_params.emphasis) + (hp * 1.5f) * m_params.emphasis;
        }

        // T4 Phosphor glow attack
        if (scMono > m_fastStage) {
            m_fastStage += attackCoeff * (scMono - m_fastStage);
        } else {
            m_fastStage += fastReleaseCoeff * (scMono - m_fastStage);
        }

        // Program-dependent memory accumulation
        m_memoryHistory += (scMono > m_memoryHistory ? attackCoeff * 0.05f : 0.0001f) * (scMono - m_memoryHistory);
        float currentSlowSec = slowBaseSec + m_memoryHistory * 2.5f;
        float slowReleaseCoeff = 1.0f - std::exp(-1.0f / (currentSlowSec * m_sampleRate));

        if (scMono > m_slowStage) {
            m_slowStage += attackCoeff * 0.6f * (scMono - m_slowStage);
        } else {
            m_slowStage += slowReleaseCoeff * (scMono - m_slowStage);
        }

        // Photocell luminescence output (dual-stage combination)
        m_photocellLuminescence = 0.5f * m_fastStage + 0.5f * m_slowStage;

        // Gain reduction calculation
        float maxScale = (m_params.mode == OpticalLevelerMode::LIMIT) ? 36.0f : 24.0f;
        float grDb = -std::min(maxScale, m_photocellLuminescence * maxScale);
        m_currentGrDb = grDb;

        float linGr = std::pow(10.0f, grDb / 20.0f);

        // Apply optical attenuation
        float compL = xL * linGr * drive;
        float compR = xR * linGr * drive;

        // Tube cathode follower output stage (12AX7 triode warmth)
        auto tubeWarmth = [](float s) {
            float sat = std::tanh(s);
            float harm2 = 0.04f * s * std::abs(s);
            return sat + harm2;
        };

        float wetL = tubeWarmth(compL) * makeupLinear;
        float wetR = tubeWarmth(compR) * makeupLinear;

        // Dry/Wet parallel mix
        outL[i] = xL * (1.0f - m_params.dryWet) + wetL * m_params.dryWet;
        outR[i] = xR * (1.0f - m_params.dryWet) + wetR * m_params.dryWet;
    }
}

} // namespace Jazz
