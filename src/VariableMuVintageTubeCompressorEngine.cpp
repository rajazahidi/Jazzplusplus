//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// VariableMuVintageTubeCompressorEngine.cpp - Variable-Mu Tube Compressor
//*****************************************************************************

#include "VariableMuVintageTubeCompressorEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

static constexpr float PI_F = 3.14159265358979323846f;
static constexpr float SQRT2_INV = 0.7071067811865475f;

JZVariableMuVintageTubeCompressorEngine::JZVariableMuVintageTubeCompressorEngine(float sampleRate)
    : m_sampleRate(sampleRate)
{
    initialize(sampleRate);
}

void JZVariableMuVintageTubeCompressorEngine::initialize(float sampleRate)
{
    m_sampleRate = (sampleRate > 8000.0f) ? sampleRate : 48000.0f;
    reset();
    updateBallistics();
    updateSidechainFilters();
}

void JZVariableMuVintageTubeCompressorEngine::reset()
{
    m_envL = 0.0f;
    m_envR = 0.0f;
    m_envSlowL = 0.0f;
    m_envSlowR = 0.0f;
    m_currentGrDb = 0.0f;
    m_currentBiasVolts = -6.3f;
    m_scStateL = 0.0f;
    m_scStateR = 0.0f;
}

void JZVariableMuVintageTubeCompressorEngine::setParameters(const Parameters& params)
{
    m_params = params;
    m_params.thresholdDb = std::max(-36.0f, std::min(m_params.thresholdDb, 0.0f));
    m_params.inputDrive = std::max(0.5f, std::min(m_params.inputDrive, 3.0f));
    m_params.outputGainDb = std::max(-12.0f, std::min(m_params.outputGainDb, 18.0f));
    m_params.dryWet = std::max(0.0f, std::min(m_params.dryWet, 1.0f));

    updateBallistics();
    updateSidechainFilters();
}

void JZVariableMuVintageTubeCompressorEngine::setThresholdDb(float db)
{
    m_params.thresholdDb = std::max(-36.0f, std::min(db, 0.0f));
}

void JZVariableMuVintageTubeCompressorEngine::setTimeConstant(VariableMuTimeConstant tc)
{
    m_params.timeConstant = tc;
    updateBallistics();
}

void JZVariableMuVintageTubeCompressorEngine::setInputDrive(float drive)
{
    m_params.inputDrive = std::max(0.5f, std::min(drive, 3.0f));
}

void JZVariableMuVintageTubeCompressorEngine::setOutputGainDb(float db)
{
    m_params.outputGainDb = std::max(-12.0f, std::min(db, 18.0f));
}

void JZVariableMuVintageTubeCompressorEngine::setSidechainHpHz(float hz)
{
    m_params.sidechainHpHz = std::max(0.0f, std::min(hz, 250.0f));
    updateSidechainFilters();
}

void JZVariableMuVintageTubeCompressorEngine::setStereoLink(bool linked)
{
    m_params.stereoLink = linked;
}

void JZVariableMuVintageTubeCompressorEngine::setMidSideMode(bool ms)
{
    m_params.midSideMode = ms;
}

void JZVariableMuVintageTubeCompressorEngine::setDryWet(float wet)
{
    m_params.dryWet = std::max(0.0f, std::min(wet, 1.0f));
}

void JZVariableMuVintageTubeCompressorEngine::setBypass(bool bypass)
{
    m_params.bypassed = bypass;
}

float JZVariableMuVintageTubeCompressorEngine::getThdPercent() const
{
    // Estimated THD based on 6386 triode drive and gain reduction
    float driveFactor = m_params.inputDrive - 0.5f;
    float grFactor = std::abs(m_currentGrDb) * 0.08f;
    return std::min(5.0f, 0.05f + driveFactor * 0.35f + grFactor * 0.15f);
}

void JZVariableMuVintageTubeCompressorEngine::updateBallistics()
{
    float attackMs = 0.4f;
    float releaseSecFast = 0.8f;
    float releaseSecSlow = 5.0f;

    switch (m_params.timeConstant) {
        case VariableMuTimeConstant::FAST_0_3S:
            attackMs = 0.2f; releaseSecFast = 0.3f; releaseSecSlow = 0.3f;
            break;
        case VariableMuTimeConstant::MEDIUM_0_8S:
            attackMs = 0.2f; releaseSecFast = 0.8f; releaseSecSlow = 0.8f;
            break;
        case VariableMuTimeConstant::SLOW_2_0S:
            attackMs = 0.4f; releaseSecFast = 2.0f; releaseSecSlow = 2.0f;
            break;
        case VariableMuTimeConstant::WARM_5_0S:
            attackMs = 0.8f; releaseSecFast = 5.0f; releaseSecSlow = 5.0f;
            break;
        case VariableMuTimeConstant::AUTO_PROGRAM_1:
            attackMs = 0.4f; releaseSecFast = 2.0f; releaseSecSlow = 10.0f;
            break;
        case VariableMuTimeConstant::AUTO_PROGRAM_2:
            attackMs = 0.2f; releaseSecFast = 0.8f; releaseSecSlow = 25.0f;
            break;
    }

    m_attackCoeffFast = 1.0f - std::exp(-1000.0f / (attackMs * m_sampleRate));
    m_releaseCoeffFast = 1.0f - std::exp(-1.0f / (releaseSecFast * m_sampleRate));
    m_releaseCoeffSlow = 1.0f - std::exp(-1.0f / (releaseSecSlow * m_sampleRate));
}

void JZVariableMuVintageTubeCompressorEngine::updateSidechainFilters()
{
    if (m_params.sidechainHpHz <= 10.0f) {
        m_scHpA = 0.0f;
    } else {
        float omega = 2.0f * PI_F * m_params.sidechainHpHz / m_sampleRate;
        m_scHpA = 1.0f / (1.0f + omega);
    }
}

void JZVariableMuVintageTubeCompressorEngine::process(
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

    float makeupGain = std::pow(10.0f, m_params.outputGainDb / 20.0f);
    float drive = m_params.inputDrive;
    float threshDb = m_params.thresholdDb;

    for (size_t i = 0; i < numSamples; ++i) {
        float xL = inL[i];
        float xR = inR[i];

        // Mid/Side encoding if enabled
        float procL = xL;
        float procR = xR;
        if (m_params.midSideMode) {
            procL = (xL + xR) * SQRT2_INV;
            procR = (xL - xR) * SQRT2_INV;
        }

        // 1. Sidechain highpass filtering
        float scL = procL;
        float scR = procR;
        if (m_scHpA > 0.0f) {
            float hpL = m_scHpA * (m_scStateL + procL);
            m_scStateL = hpL - procL;
            scL = hpL;

            float hpR = m_scHpA * (m_scStateR + procR);
            m_scStateR = hpR - procR;
            scR = hpR;
        }

        float absL = std::abs(scL) * drive;
        float absR = std::abs(scR) * drive;

        if (m_params.stereoLink) {
            float linkMax = std::max(absL, absR);
            absL = linkMax;
            absR = linkMax;
        }

        // Fast peak envelope follower
        if (absL > m_envL) {
            m_envL += m_attackCoeffFast * (absL - m_envL);
        } else {
            m_envL += m_releaseCoeffFast * (absL - m_envL);
        }

        if (absR > m_envR) {
            m_envR += m_attackCoeffFast * (absR - m_envR);
        } else {
            m_envR += m_releaseCoeffFast * (absR - m_envR);
        }

        // Slow sustained envelope follower (for auto-release program dependency)
        m_envSlowL += (absL > m_envSlowL ? m_attackCoeffFast * 0.2f : m_releaseCoeffSlow) * (absL - m_envSlowL);
        m_envSlowR += (absR > m_envSlowR ? m_attackCoeffFast * 0.2f : m_releaseCoeffSlow) * (absR - m_envSlowR);

        // Effective detection level
        float effL = (m_params.timeConstant >= VariableMuTimeConstant::AUTO_PROGRAM_1)
                     ? (0.65f * m_envL + 0.35f * m_envSlowL) : m_envL;
        float effR = (m_params.timeConstant >= VariableMuTimeConstant::AUTO_PROGRAM_1)
                     ? (0.65f * m_envR + 0.35f * m_envSlowR) : m_envR;

        // Convert to dB
        float dbL = 20.0f * std::log10(effL + 1e-6f);
        float dbR = 20.0f * std::log10(effR + 1e-6f);

        // Continuous variable-mu ratio calculation:
        // Remote cutoff: ratio starts at 1.5:1 at the knee and ramps up smoothly to ~20:1
        float overDbL = dbL - threshDb;
        float grDbL = 0.0f;
        if (overDbL > 0.0f) {
            float ratio = 1.5f + 18.5f * (overDbL / (overDbL + 12.0f));
            grDbL = -overDbL * (1.0f - 1.0f / ratio);
        }

        float overDbR = dbR - threshDb;
        float grDbR = 0.0f;
        if (overDbR > 0.0f) {
            float ratio = 1.5f + 18.5f * (overDbR / (overDbR + 12.0f));
            grDbR = -overDbR * (1.0f - 1.0f / ratio);
        }

        m_currentGrDb = std::min(grDbL, grDbR);
        m_currentBiasVolts = -6.3f + m_currentGrDb * 0.85f;

        float linGainL = std::pow(10.0f, grDbL / 20.0f);
        float linGainR = std::pow(10.0f, grDbR / 20.0f);

        // 2. Apply variable-mu gain reduction and 6386 triode tube saturation
        float compL = procL * linGainL * drive;
        float compR = procR * linGainR * drive;

        // Non-linear triode saturation (asymmetry 2nd harmonic + soft clip 3rd harmonic)
        auto tubeSat = [](float in) {
            float sat = std::tanh(in);
            // Slight 2nd harmonic generation (triode push-pull imbalance)
            float secondHarm = 0.045f * in * std::abs(in);
            return sat + secondHarm;
        };

        float wetL = tubeSat(compL) * makeupGain;
        float wetR = tubeSat(compR) * makeupGain;

        // Mid/Side decoding if enabled
        if (m_params.midSideMode) {
            float decodedL = (wetL + wetR) * SQRT2_INV;
            float decodedR = (wetL - wetR) * SQRT2_INV;
            wetL = decodedL;
            wetR = decodedR;
        }

        // Parallel blend
        outL[i] = xL * (1.0f - m_params.dryWet) + wetL * m_params.dryWet;
        outR[i] = xR * (1.0f - m_params.dryWet) + wetR * m_params.dryWet;
    }
}

} // namespace Jazz
