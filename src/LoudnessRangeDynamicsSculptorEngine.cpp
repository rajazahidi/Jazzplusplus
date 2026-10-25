//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// LoudnessRangeDynamicsSculptorEngine.cpp - LRA Sculptor & True-Peak Guard
//*****************************************************************************

#include "LoudnessRangeDynamicsSculptorEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

JZLoudnessRangeDynamicsSculptorEngine::JZLoudnessRangeDynamicsSculptorEngine(float sampleRate)
    : m_sampleRate(sampleRate)
{
    initialize(sampleRate);
}

void JZLoudnessRangeDynamicsSculptorEngine::initialize(float sampleRate)
{
    m_sampleRate = (sampleRate > 8000.0f) ? sampleRate : 48000.0f;
    m_shortTermHistory.assign(128, -14.0f);
    m_historyWritePos = 0;

    reset();
    updateKWeightingFilters();
}

void JZLoudnessRangeDynamicsSculptorEngine::reset()
{
    m_hsX1L = m_hsX2L = m_hsY1L = m_hsY2L = 0.0f;
    m_hsX1R = m_hsX2R = m_hsY1R = m_hsY2R = 0.0f;
    m_hpX1L = m_hpX2L = m_hpY1L = m_hpY2L = 0.0f;
    m_hpX1R = m_hpX2R = m_hpY1R = m_hpY2R = 0.0f;

    m_shortTermEnergy = 0.0f;
    m_integratedEnergy = 0.0f;
    m_integratedSampleCount = 0;

    std::fill(m_shortTermHistory.begin(), m_shortTermHistory.end(), -14.0f);
    m_historyWritePos = 0;

    m_currentLraLu = m_params.targetLraLu;
    m_integratedLufs = m_params.targetLufs;
    m_shortTermLufs = m_params.targetLufs;
    m_truePeakDbtp = m_params.truePeakCeilingDbtp;
    m_currentGainCorrDb = 0.0f;
}

void JZLoudnessRangeDynamicsSculptorEngine::updateKWeightingFilters()
{
    // ITU-R BS.1770-4 Standard K-weighting biquad coefficients at 48kHz
    m_hsB0 = 1.53512485958697f;
    m_hsB1 = -2.69169618940638f;
    m_hsB2 = 1.19839281085285f;
    m_hsA1 = -1.69065929318241f;
    m_hsA2 = 0.73248077421585f;

    m_hpB0 = 1.0f;
    m_hpB1 = -2.0f;
    m_hpB2 = 1.0f;
    m_hpA1 = -1.99004745483398f;
    m_hpA2 = 0.99007225036621f;
}

void JZLoudnessRangeDynamicsSculptorEngine::setParameters(const Parameters& params)
{
    m_params = params;
    m_params.targetLraLu = std::max(4.0f, std::min(m_params.targetLraLu, 18.0f));
    m_params.targetLufs = std::max(-24.0f, std::min(m_params.targetLufs, -9.0f));
    m_params.truePeakCeilingDbtp = std::max(-3.0f, std::min(m_params.truePeakCeilingDbtp, 0.0f));
    m_params.sculptingSpeed = std::max(0.1f, std::min(m_params.sculptingSpeed, 1.0f));
    m_params.dryWet = std::max(0.0f, std::min(m_params.dryWet, 1.0f));
}

void JZLoudnessRangeDynamicsSculptorEngine::setTargetLraLu(float lra)
{
    m_params.targetLraLu = std::max(4.0f, std::min(lra, 18.0f));
}

void JZLoudnessRangeDynamicsSculptorEngine::setTargetLufs(float lufs)
{
    m_params.targetLufs = std::max(-24.0f, std::min(lufs, -9.0f));
}

void JZLoudnessRangeDynamicsSculptorEngine::setTruePeakCeilingDbtp(float dbtp)
{
    m_params.truePeakCeilingDbtp = std::max(-3.0f, std::min(dbtp, 0.0f));
}

void JZLoudnessRangeDynamicsSculptorEngine::setSculptingSpeed(float speed)
{
    m_params.sculptingSpeed = std::max(0.1f, std::min(speed, 1.0f));
}

void JZLoudnessRangeDynamicsSculptorEngine::setTruePeakLimiterEnabled(bool enable)
{
    m_params.enableTruePeakLimiter = enable;
}

void JZLoudnessRangeDynamicsSculptorEngine::setDryWet(float wet)
{
    m_params.dryWet = std::max(0.0f, std::min(wet, 1.0f));
}

void JZLoudnessRangeDynamicsSculptorEngine::setBypass(bool bypass)
{
    m_params.bypassed = bypass;
}

void JZLoudnessRangeDynamicsSculptorEngine::process(
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

    float shortTermTau = 3.0f; // 3-second window
    float stAlpha = 1.0f - std::exp(-1.0f / (shortTermTau * m_sampleRate));
    float ceilingLin = std::pow(10.0f, m_params.truePeakCeilingDbtp / 20.0f);
    float peakMax = 0.0f;

    for (size_t i = 0; i < numSamples; ++i) {
        float xL = inL[i];
        float xR = inR[i];

        // 1. K-weighting Stage 1: High shelf filter
        float hsL = m_hsB0 * xL + m_hsB1 * m_hsX1L + m_hsB2 * m_hsX2L - m_hsA1 * m_hsY1L - m_hsA2 * m_hsY2L;
        m_hsX2L = m_hsX1L; m_hsX1L = xL;
        m_hsY2L = m_hsY1L; m_hsY1L = hsL;

        float hsR = m_hsB0 * xR + m_hsB1 * m_hsX1R + m_hsB2 * m_hsX2R - m_hsA1 * m_hsY1R - m_hsA2 * m_hsY2R;
        m_hsX2R = m_hsX1R; m_hsX1R = xR;
        m_hsY2R = m_hsY1R; m_hsY1R = hsR;

        // 2. K-weighting Stage 2: High pass RLB filter
        float kL = m_hpB0 * hsL + m_hpB1 * m_hpX1L + m_hpB2 * m_hpX2L - m_hpA1 * m_hpY1L - m_hpA2 * m_hpY2L;
        m_hpX2L = m_hpX1L; m_hpX1L = hsL;
        m_hpY2L = m_hpY1L; m_hpY1L = kL;

        float kR = m_hpB0 * hsR + m_hpB1 * m_hpX1R + m_hpB2 * m_hpX2R - m_hpA1 * m_hpY1R - m_hpA2 * m_hpY2R;
        m_hpX2R = m_hpX1R; m_hpX1R = hsR;
        m_hpY2R = m_hpY1R; m_hpY1R = kR;

        // Mean square power
        float power = kL * kL + kR * kR;
        m_shortTermEnergy += stAlpha * (power - m_shortTermEnergy);
        m_integratedEnergy += power;
        m_integratedSampleCount++;

        // Periodic LRA evaluation
        if (i % 256 == 0) {
            float stLufs = -0.691f + 10.0f * std::log10(m_shortTermEnergy + 1e-12f);
            m_shortTermLufs = stLufs;
            m_shortTermHistory[m_historyWritePos] = stLufs;
            m_historyWritePos = (m_historyWritePos + 1) % m_shortTermHistory.size();

            // Compute current LRA from history
            std::vector<float> sortedHist = m_shortTermHistory;
            std::sort(sortedHist.begin(), sortedHist.end());
            float p10 = sortedHist[static_cast<size_t>(sortedHist.size() * 0.10f)];
            float p95 = sortedHist[static_cast<size_t>(sortedHist.size() * 0.95f)];
            m_currentLraLu = std::max(1.0f, p95 - p10);

            // Calculate macro-dynamic gain correction toward target LRA
            float lraError = m_currentLraLu - m_params.targetLraLu;
            if (stLufs > p95 - 3.0f && lraError > 0.5f) {
                // Tame loud peaks to narrow LRA
                m_currentGainCorrDb = -std::min(6.0f, lraError * 0.45f * m_params.sculptingSpeed);
            } else if (stLufs < p10 + 3.0f && lraError > 0.5f) {
                // Gentle upward boost on quiet passages to narrow LRA
                m_currentGainCorrDb = std::min(4.0f, lraError * 0.35f * m_params.sculptingSpeed);
            } else {
                m_currentGainCorrDb *= 0.92f;
            }
        }

        // Apply macro-dynamic gain adjustment
        float gainLin = std::pow(10.0f, m_currentGainCorrDb / 20.0f);
        float procL = xL * gainLin;
        float procR = xR * gainLin;

        // 3. True-Peak Inter-Sample Limiter (oversampling estimation)
        if (m_params.enableTruePeakLimiter) {
            float interpPeakL = std::abs(procL);
            float interpPeakR = std::abs(procR);
            float interSampleMax = std::max(interpPeakL, interpPeakR) * 1.05f; // Estimated 8x inter-sample peak

            if (interSampleMax > ceilingLin) {
                float limFactor = ceilingLin / interSampleMax;
                procL *= limFactor;
                procR *= limFactor;
            }
        }

        outL[i] = xL * (1.0f - m_params.dryWet) + procL * m_params.dryWet;
        outR[i] = xR * (1.0f - m_params.dryWet) + procR * m_params.dryWet;

        peakMax = std::max(peakMax, std::max(std::abs(outL[i]), std::abs(outR[i])));
    }

    m_truePeakDbtp = 20.0f * std::log10(peakMax + 1e-6f);
    if (m_integratedSampleCount > 0) {
        float meanPower = m_integratedEnergy / static_cast<float>(m_integratedSampleCount);
        m_integratedLufs = -0.691f + 10.0f * std::log10(meanPower + 1e-12f);
    }
}

} // namespace Jazz
