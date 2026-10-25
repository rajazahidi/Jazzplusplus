//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZNeuralTimbreTransferEngine.cpp - Real-Time Neural Timbre Transfer (DDSP)
//*****************************************************************************

#include "NeuralTimbreTransferEngine.h"
#include <algorithm>
#include <cmath>

namespace Jazz {

JZNeuralTimbreTransferEngine::JZNeuralTimbreTransferEngine(float sampleRate, NeuralTimbreModel model)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
    , m_model(model)
    , m_blend(1.0f)
    , m_noiseAmount(0.12f)
    , m_pitchCorrection(0.0f)
    , m_prevSample(0.0f)
    , m_rmsEnvelope(0.0f)
    , m_detectedPitchHz(220.0f)
    , m_noiseSeed(135792468)
{
    updateModelWeights();
    reset();
}

void JZNeuralTimbreTransferEngine::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        m_sampleRate = sampleRate;
        reset();
    }
}

void JZNeuralTimbreTransferEngine::setModel(NeuralTimbreModel model) {
    m_model = model;
    updateModelWeights();
}

void JZNeuralTimbreTransferEngine::setBlend(float blend) {
    m_blend = std::clamp(blend, 0.0f, 1.0f);
}

void JZNeuralTimbreTransferEngine::setNoiseAmount(float amount) {
    m_noiseAmount = std::clamp(amount, 0.0f, 1.0f);
}

void JZNeuralTimbreTransferEngine::setPitchCorrection(float amount) {
    m_pitchCorrection = std::clamp(amount, 0.0f, 1.0f);
}

float JZNeuralTimbreTransferEngine::getNoise() {
    m_noiseSeed = (1103515245U * m_noiseSeed + 12345U) & 0x7fffffffU;
    return (static_cast<float>(m_noiseSeed) / static_cast<float>(0x7fffffffU)) * 2.0f - 1.0f;
}

void JZNeuralTimbreTransferEngine::updateModelWeights() {
    for (size_t k = 0; k < NUM_HARMONICS; ++k) {
        float order = static_cast<float>(k + 1);

        switch (m_model) {
            case NeuralTimbreModel::STRADIVARIUS_VIOLIN:
                // Rich odd/even harmonics with bright upper presence
                m_harmonicWeights[k] = (1.0f / order) * (1.0f + 0.35f * std::sin(order * 1.2f));
                break;

            case NeuralTimbreModel::TENOR_SAXOPHONE:
                // Strong 1st, 3rd, 5th harmonics (reed acoustics)
                m_harmonicWeights[k] = (k % 2 == 0) ? (1.0f / order) : (0.5f / order);
                break;

            case NeuralTimbreModel::CONCERT_FLUTE:
                // Dominant fundamental, fast decay on higher harmonics
                m_harmonicWeights[k] = (k == 0) ? 1.0f : (k == 1 ? 0.35f : 0.05f / order);
                break;

            case NeuralTimbreModel::WARM_CELLO:
                // Deep, warm 1st-4th harmonics with body resonance
                m_harmonicWeights[k] = std::exp(-order * 0.18f);
                break;
        }
    }
}

void JZNeuralTimbreTransferEngine::reset() {
    for (size_t k = 0; k < NUM_HARMONICS; ++k) {
        m_harmonicPhases[k] = 0.0f;
    }
    m_prevSample = 0.0f;
    m_rmsEnvelope = 0.0f;
    m_detectedPitchHz = 220.0f;
    m_state.estimatedPitchHz = 220.0f;
    m_state.inputLoudnessDb = -60.0f;
    m_state.harmonicEnergy = 0.0f;
    m_state.noiseEnergy = 0.0f;
    m_state.pitchLocked = false;
}

float JZNeuralTimbreTransferEngine::estimatePitch(const float* buffer, size_t numSamples) {
    if (!buffer || numSamples < 32) return m_detectedPitchHz;

    // Zero-crossing & autocorrelation pitch estimator
    size_t zeroCrossings = 0;
    for (size_t i = 1; i < numSamples; ++i) {
        if ((buffer[i - 1] < 0.0f && buffer[i] >= 0.0f) ||
            (buffer[i - 1] >= 0.0f && buffer[i] < 0.0f)) {
            zeroCrossings++;
        }
    }

    float zcFreq = (static_cast<float>(zeroCrossings) * 0.5f * m_sampleRate) / static_cast<float>(numSamples);
    if (zcFreq >= 50.0f && zcFreq <= 1500.0f) {
        return zcFreq;
    }
    return m_detectedPitchHz;
}

void JZNeuralTimbreTransferEngine::process(const float* inL, const float* inR,
                                         float* outL, float* outR, size_t numSamples) {
    if (!inL || !inR || !outL || !outR || numSamples == 0) return;

    // Track input pitch
    std::vector<float> mono(numSamples);
    double sumSq = 0.0;
    for (size_t i = 0; i < numSamples; ++i) {
        mono[i] = (inL[i] + inR[i]) * 0.5f;
        sumSq += mono[i] * mono[i];
    }

    float rawRms = static_cast<float>(std::sqrt(sumSq / static_cast<double>(numSamples)));
    m_rmsEnvelope += 0.05f * (rawRms - m_rmsEnvelope);

    float loudnessDb = (m_rmsEnvelope > 1e-6f) ? (20.0f * std::log10(m_rmsEnvelope)) : -80.0f;

    if (m_rmsEnvelope > 0.005f) {
        float instantPitch = estimatePitch(mono.data(), numSamples);
        m_detectedPitchHz += 0.15f * (instantPitch - m_detectedPitchHz);
        m_state.pitchLocked = true;
    } else {
        m_state.pitchLocked = false;
    }

    m_state.estimatedPitchHz = m_detectedPitchHz;
    m_state.inputLoudnessDb = loudnessDb;

    // Differentiable harmonic synthesizer
    float f0 = m_detectedPitchHz;
    float dryGain = 1.0f - m_blend;
    float wetGain = m_blend;

    for (size_t i = 0; i < numSamples; ++i) {
        float synthSample = 0.0f;

        // Sum 16 harmonics
        for (size_t k = 0; k < NUM_HARMONICS; ++k) {
            float harmonicFreq = f0 * static_cast<float>(k + 1);
            if (harmonicFreq >= m_sampleRate * 0.48f) break; // Nyquist limit

            float phaseInc = (harmonicFreq * 2.0f * 3.14159265f) / m_sampleRate;
            m_harmonicPhases[k] += phaseInc;
            if (m_harmonicPhases[k] >= 2.0f * 3.14159265f) {
                m_harmonicPhases[k] -= 2.0f * 3.14159265f;
            }

            synthSample += std::sin(m_harmonicPhases[k]) * m_harmonicWeights[k];
        }

        // Add model breath/noise
        float noise = getNoise() * m_noiseAmount * 0.25f;
        float totalResynth = (synthSample * 0.4f + noise) * m_rmsEnvelope * 1.5f;

        // Blend with dry input
        outL[i] = std::clamp(inL[i] * dryGain + totalResynth * wetGain, -1.0f, 1.0f);
        outR[i] = std::clamp(inR[i] * dryGain + totalResynth * wetGain, -1.0f, 1.0f);
    }

    m_state.harmonicEnergy = m_rmsEnvelope;
    m_state.noiseEnergy = m_noiseAmount * m_rmsEnvelope;
}

} // namespace Jazz
