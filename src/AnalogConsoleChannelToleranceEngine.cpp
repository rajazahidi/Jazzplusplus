//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// AnalogConsoleChannelToleranceEngine.cpp - Analog Channel Tolerance & Crosstalk
//*****************************************************************************

#include "AnalogConsoleChannelToleranceEngine.h"
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace Jazz {

JZAnalogConsoleChannelToleranceEngine::JZAnalogConsoleChannelToleranceEngine(float sampleRate)
    : m_sampleRate(sampleRate)
{
    initialize(sampleRate);
}

void JZAnalogConsoleChannelToleranceEngine::initialize(float sampleRate)
{
    m_sampleRate = (sampleRate > 8000.0f) ? sampleRate : 48000.0f;
    reset();
    calculateChannelTolerances();
}

void JZAnalogConsoleChannelToleranceEngine::reset()
{
    m_transStateL = 0.0f;
    m_transStateR = 0.0f;
}

void JZAnalogConsoleChannelToleranceEngine::setParameters(const Parameters& params)
{
    m_params = params;
    m_params.channelIndex = std::max(static_cast<size_t>(1), std::min(m_params.channelIndex, static_cast<size_t>(64)));
    m_params.crosstalkAmountDb = std::max(-95.0f, std::min(m_params.crosstalkAmountDb, -55.0f));
    m_params.transformerDrive = std::max(0.0f, std::min(m_params.transformerDrive, 2.5f));
    m_params.toleranceSpread = std::max(0.0f, std::min(m_params.toleranceSpread, 2.0f));
    m_params.dryWet = std::max(0.0f, std::min(m_params.dryWet, 1.0f));

    calculateChannelTolerances();
}

void JZAnalogConsoleChannelToleranceEngine::setChannelIndex(size_t index)
{
    m_params.channelIndex = std::max(static_cast<size_t>(1), std::min(index, static_cast<size_t>(64)));
    calculateChannelTolerances();
}

void JZAnalogConsoleChannelToleranceEngine::setConsoleStyle(AnalogConsoleStyle style)
{
    m_params.consoleStyle = style;
    calculateChannelTolerances();
}

void JZAnalogConsoleChannelToleranceEngine::setCrosstalkAmountDb(float db)
{
    m_params.crosstalkAmountDb = std::max(-95.0f, std::min(db, -55.0f));
}

void JZAnalogConsoleChannelToleranceEngine::setTransformerDrive(float drive)
{
    m_params.transformerDrive = std::max(0.0f, std::min(drive, 2.5f));
}

void JZAnalogConsoleChannelToleranceEngine::setToleranceSpread(float spread)
{
    m_params.toleranceSpread = std::max(0.0f, std::min(spread, 2.0f));
    calculateChannelTolerances();
}

void JZAnalogConsoleChannelToleranceEngine::setDryWet(float wet)
{
    m_params.dryWet = std::max(0.0f, std::min(wet, 1.0f));
}

void JZAnalogConsoleChannelToleranceEngine::setBypass(bool bypass)
{
    m_params.bypassed = bypass;
}

std::string JZAnalogConsoleChannelToleranceEngine::getConsoleStyleName() const
{
    switch (m_params.consoleStyle) {
        case AnalogConsoleStyle::VINTAGE_BRITISH_80:
            return "Vintage British 8048 (Neve Class-A Marinair)";
        case AnalogConsoleStyle::MODERN_PRECISION_9000:
            return "Modern Precision 9000J (SSL SuperAnalogue)";
    }
    return "Unknown";
}

void JZAnalogConsoleChannelToleranceEngine::calculateChannelTolerances()
{
    // Deterministic pseudo-random generation based on channel index
    uint32_t seed = static_cast<uint32_t>(m_params.channelIndex * 2654435761u);

    auto lcg = [&seed]() {
        seed = seed * 1664525u + 1013904223u;
        return static_cast<float>(seed & 0xFFFF) / 65535.0f * 2.0f - 1.0f; // -1.0 to +1.0
    };

    float capScale = (m_params.consoleStyle == AnalogConsoleStyle::VINTAGE_BRITISH_80) ? 2.2f : 0.8f;
    float resScale = (m_params.consoleStyle == AnalogConsoleStyle::VINTAGE_BRITISH_80) ? 1.0f : 0.4f;

    m_capTolerancePct = lcg() * capScale * m_params.toleranceSpread;
    m_resTolerancePct = lcg() * resScale * m_params.toleranceSpread;

    // Slight analog gain calibration drift (+/- 0.04 dB)
    m_gainTrimL = 1.0f + (m_resTolerancePct * 0.005f);
    m_gainTrimR = 1.0f - (m_resTolerancePct * 0.005f);
}

void JZAnalogConsoleChannelToleranceEngine::process(
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

    float crosstalkLinear = std::pow(10.0f, m_params.crosstalkAmountDb / 20.0f);
    float drive = m_params.transformerDrive;
    bool isBritish = (m_params.consoleStyle == AnalogConsoleStyle::VINTAGE_BRITISH_80);

    for (size_t i = 0; i < numSamples; ++i) {
        float xL = inL[i] * m_gainTrimL;
        float xR = inR[i] * m_gainTrimR;

        // 1. Inter-channel stereo crosstalk bleed
        float xtalkL = xL + xR * crosstalkLinear;
        float xtalkR = xR + xL * crosstalkLinear;

        // 2. Transformer hysteresis & saturation
        float transL = xtalkL;
        float transR = xtalkR;

        if (drive > 0.01f) {
            float driveScale = 1.0f + drive * (isBritish ? 0.65f : 0.35f);
            float satL = std::tanh(transL * driveScale);
            float satR = std::tanh(transR * driveScale);

            // Transformer core low-frequency thickening
            m_transStateL += 0.08f * (satL - m_transStateL);
            m_transStateR += 0.08f * (satR - m_transStateR);

            transL = satL + m_transStateL * 0.06f * drive;
            transR = satR + m_transStateR * 0.06f * drive;
        }

        // Dry/Wet blend
        outL[i] = inL[i] * (1.0f - m_params.dryWet) + transL * m_params.dryWet;
        outR[i] = inR[i] * (1.0f - m_params.dryWet) + transR * m_params.dryWet;
    }
}

} // namespace Jazz
