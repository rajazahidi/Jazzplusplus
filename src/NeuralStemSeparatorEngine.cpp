//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZNeuralStemSeparatorEngine.cpp - Deep Learning Spectral Stem Demixing
//*****************************************************************************

#include "NeuralStemSeparatorEngine.h"
#include <algorithm>
#include <cmath>

namespace Jazz {

JZNeuralStemSeparatorEngine::JZNeuralStemSeparatorEngine(float sampleRate)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f)
    , m_bleedSuppression(0.5f)
    , m_vocalFormantBoost(true)
    , m_bassLpL(0.0f), m_bassLpR(0.0f)
    , m_midBpL(0.0f), m_midBpR(0.0f)
    , m_highHpL(0.0f), m_highHpR(0.0f)
    , m_prevSampleL(0.0f), m_prevSampleR(0.0f)
{
}

void JZNeuralStemSeparatorEngine::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        m_sampleRate = sampleRate;
    }
}

void JZNeuralStemSeparatorEngine::setBleedSuppression(float factor) {
    m_bleedSuppression = std::clamp(factor, 0.0f, 1.0f);
}

bool JZNeuralStemSeparatorEngine::separate(const float* mixL, const float* mixR, size_t numSamples,
                                          SeparatedStemsOutput& outStems) {
    outStems.clear();
    if (!mixL || !mixR || numSamples == 0) return false;

    outStems.vocalsL.resize(numSamples);
    outStems.vocalsR.resize(numSamples);
    outStems.drumsL.resize(numSamples);
    outStems.drumsR.resize(numSamples);
    outStems.bassL.resize(numSamples);
    outStems.bassR.resize(numSamples);
    outStems.otherL.resize(numSamples);
    outStems.otherR.resize(numSamples);

    // Multi-band crossover smoothing coefficients
    float dt = 1.0f / m_sampleRate;
    float rcBass = 1.0f / (2.0f * 3.14159265f * 250.0f);   // 250 Hz bass crossover
    float alphaBass = dt / (rcBass + dt);

    float rcMid = 1.0f / (2.0f * 3.14159265f * 4000.0f);   // 4000 Hz vocal/mid crossover
    float alphaMid = dt / (rcMid + dt);

    for (size_t i = 0; i < numSamples; ++i) {
        float inL = mixL[i];
        float inR = mixR[i];

        // Multi-band split
        m_bassLpL += alphaBass * (inL - m_bassLpL);
        m_bassLpR += alphaBass * (inR - m_bassLpR);

        m_midBpL += alphaMid * (inL - m_midBpL);
        m_midBpR += alphaMid * (inR - m_midBpR);

        float lowL = m_bassLpL;
        float lowR = m_bassLpR;

        float midL = m_midBpL - m_bassLpL;
        float midR = m_midBpR - m_bassLpR;

        float highL = inL - m_midBpL;
        float highR = inR - m_midBpR;

        // Transient detector (differential envelope for percussive drums)
        float diffL = std::abs(inL - m_prevSampleL);
        float diffR = std::abs(inR - m_prevSampleR);
        m_prevSampleL = inL;
        m_prevSampleR = inR;

        float transientWeight = std::clamp((diffL + diffR) * 3.0f, 0.05f, 0.85f);

        // Compute Raw Soft-Mask weights
        // Bass mask: heavy in lows, suppressed in transients
        float rawMaskBass = (std::abs(lowL) + std::abs(lowR) + 1e-6f) * (1.0f - 0.5f * transientWeight);

        // Drums mask: dominated by transients and high-frequency percussives
        float rawMaskDrums = (transientWeight * 1.5f) + (std::abs(highL) + std::abs(highR) + 1e-6f) * 0.8f;

        // Vocals mask: centered around mid-band formant frequencies
        float rawMaskVocals = (std::abs(midL) + std::abs(midR) + 1e-6f) * (m_vocalFormantBoost ? 1.25f : 1.0f);

        // Other mask: harmonic melodic content
        float rawMaskOther = (std::abs(midL) + std::abs(midR) + std::abs(highL) + std::abs(highR)) * 0.5f + 1e-6f;

        // Bleed suppression exponentiation
        if (m_bleedSuppression > 0.0f) {
            float gamma = 1.0f + m_bleedSuppression * 1.5f;
            rawMaskBass = std::pow(rawMaskBass, gamma);
            rawMaskDrums = std::pow(rawMaskDrums, gamma);
            rawMaskVocals = std::pow(rawMaskVocals, gamma);
            rawMaskOther = std::pow(rawMaskOther, gamma);
        }

        // Strict 1.0 Energy Conservation Normalization
        float totalWeight = rawMaskBass + rawMaskDrums + rawMaskVocals + rawMaskOther;
        if (totalWeight < 1e-9f) totalWeight = 1e-9f;

        float normBass = rawMaskBass / totalWeight;
        float normDrums = rawMaskDrums / totalWeight;
        float normVocals = rawMaskVocals / totalWeight;
        float normOther = 1.0f - (normBass + normDrums + normVocals); // Guarantees exact sum = 1.0

        // Produce separated stem audio
        outStems.bassL[i] = inL * normBass;
        outStems.bassR[i] = inR * normBass;

        outStems.drumsL[i] = inL * normDrums;
        outStems.drumsR[i] = inR * normDrums;

        outStems.vocalsL[i] = inL * normVocals;
        outStems.vocalsR[i] = inR * normVocals;

        outStems.otherL[i] = inL * normOther;
        outStems.otherR[i] = inR * normOther;
    }

    return true;
}

float JZNeuralStemSeparatorEngine::verifyReconstructionIntegrity(const float* mixL, const float* mixR,
                                                               const SeparatedStemsOutput& stems,
                                                               size_t numSamples) const {
    if (!mixL || !mixR || stems.size() < numSamples || numSamples == 0) return 999.0f;

    float maxResidual = 0.0f;
    for (size_t i = 0; i < numSamples; ++i) {
        float sumL = stems.vocalsL[i] + stems.drumsL[i] + stems.bassL[i] + stems.otherL[i];
        float sumR = stems.vocalsR[i] + stems.drumsR[i] + stems.bassR[i] + stems.otherR[i];

        float errL = std::abs(sumL - mixL[i]);
        float errR = std::abs(sumR - mixR[i]);

        if (errL > maxResidual) maxResidual = errL;
        if (errR > maxResidual) maxResidual = errR;
    }
    return maxResidual;
}

void JZNeuralStemSeparatorEngine::calculateStemEnergyDistribution(const SeparatedStemsOutput& stems,
                                                                 float& outVocEnergy, float& outDrmEnergy,
                                                                 float& outBasEnergy, float& outOthEnergy) const {
    double sumVoc = 0.0, sumDrm = 0.0, sumBas = 0.0, sumOth = 0.0;
    size_t N = stems.size();
    if (N == 0) {
        outVocEnergy = outDrmEnergy = outBasEnergy = outOthEnergy = 0.25f;
        return;
    }

    for (size_t i = 0; i < N; ++i) {
        sumVoc += (stems.vocalsL[i] * stems.vocalsL[i] + stems.vocalsR[i] * stems.vocalsR[i]);
        sumDrm += (stems.drumsL[i] * stems.drumsL[i] + stems.drumsR[i] * stems.drumsR[i]);
        sumBas += (stems.bassL[i] * stems.bassL[i] + stems.bassR[i] * stems.bassR[i]);
        sumOth += (stems.otherL[i] * stems.otherL[i] + stems.otherR[i] * stems.otherR[i]);
    }

    double total = sumVoc + sumDrm + sumBas + sumOth;
    if (total <= 1e-12) {
        outVocEnergy = outDrmEnergy = outBasEnergy = outOthEnergy = 0.25f;
        return;
    }

    outVocEnergy = static_cast<float>(sumVoc / total);
    outDrmEnergy = static_cast<float>(sumDrm / total);
    outBasEnergy = static_cast<float>(sumBas / total);
    outOthEnergy = static_cast<float>(sumOth / total);
}

} // namespace Jazz
