//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// DynamicWindTurbulenceSuppressorEngine.cpp - Dynamic Atmospheric Wind Buffeting
// & Diaphragm Overload Attenuator
//*****************************************************************************

#include "DynamicWindTurbulenceSuppressorEngine.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Jazz {

JZDynamicWindTurbulenceSuppressorEngine::JZDynamicWindTurbulenceSuppressorEngine(float sampleRate)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
{
    initialize(m_sampleRate);
}

void JZDynamicWindTurbulenceSuppressorEngine::initialize(float sampleRate)
{
    m_sampleRate = sampleRate > 1000.0f ? sampleRate : 48000.0f;
    reset();
}

void JZDynamicWindTurbulenceSuppressorEngine::reset()
{
    m_currentCutoffHz = m_params.minCutoffHz;
    m_windTurbulenceDb = -96.0f;
    m_gustReductionDb = 0.0f;
    m_gustDetected = false;

    m_subBassEnergyL = 0.0f;
    m_subBassEnergyR = 0.0f;
    m_midBandEnergyL = 0.0f;
    m_midBandEnergyR = 0.0f;
    m_gustEnvelope = 0.0f;

    m_detLpX1L = m_detLpX2L = m_detLpY1L = m_detLpY2L = 0.0f;
    m_detLpX1R = m_detLpX2R = m_detLpY1R = m_detLpY2R = 0.0f;
    m_detHpX1L = m_detHpX2L = m_detHpY1L = m_detHpY2L = 0.0f;
    m_detHpX1R = m_detHpX2R = m_detHpY1R = m_detHpY2R = 0.0f;

    m_hpX1L = m_hpX2L = m_hpY1L = m_hpY2L = 0.0f;
    m_hpX1R = m_hpX2R = m_hpY1R = m_hpY2R = 0.0f;

    updateDynamicHighpassCoeffs(m_params.minCutoffHz);
}

void JZDynamicWindTurbulenceSuppressorEngine::setParameters(const Parameters& params)
{
    m_params = params;
    m_params.minCutoffHz = std::max(10.0f, std::min(m_params.minCutoffHz, 100.0f));
    m_params.maxCutoffHz = std::max(m_params.minCutoffHz + 20.0f, std::min(m_params.maxCutoffHz, 800.0f));
    m_params.reductionDb = std::max(-48.0f, std::min(m_params.reductionDb, 0.0f));
    m_params.sensitivity = std::max(0.1f, std::min(m_params.sensitivity, 5.0f));
}

void JZDynamicWindTurbulenceSuppressorEngine::setReductionDb(float db)
{
    m_params.reductionDb = std::max(-48.0f, std::min(db, 0.0f));
}

void JZDynamicWindTurbulenceSuppressorEngine::setSensitivity(float sens)
{
    m_params.sensitivity = std::max(0.1f, std::min(sens, 5.0f));
}

void JZDynamicWindTurbulenceSuppressorEngine::setCutoffRange(float minHz, float maxHz)
{
    m_params.minCutoffHz = std::max(10.0f, std::min(minHz, 100.0f));
    m_params.maxCutoffHz = std::max(m_params.minCutoffHz + 20.0f, std::min(maxHz, 800.0f));
}

void JZDynamicWindTurbulenceSuppressorEngine::setDeltaSolo(bool deltaSolo)
{
    m_params.deltaSolo = deltaSolo;
}

void JZDynamicWindTurbulenceSuppressorEngine::setBypass(bool bp)
{
    m_params.bypass = bp;
}

void JZDynamicWindTurbulenceSuppressorEngine::updateDynamicHighpassCoeffs(float cutoffHz)
{
    float omega = 2.0f * static_cast<float>(M_PI) * cutoffHz / m_sampleRate;
    omega = std::max(0.001f, std::min(omega, static_cast<float>(M_PI) - 0.01f));
    float cosw = std::cos(omega);
    float alpha = std::sin(omega) / (2.0f * 0.7071f); // Q = 0.7071 (Butterworth)

    float a0 = 1.0f + alpha;
    m_hpB0 = ((1.0f + cosw) * 0.5f) / a0;
    m_hpB1 = (-(1.0f + cosw)) / a0;
    m_hpB2 = ((1.0f + cosw) * 0.5f) / a0;
    m_hpA1 = (-2.0f * cosw) / a0;
    m_hpA2 = (1.0f - alpha) / a0;
}

void JZDynamicWindTurbulenceSuppressorEngine::process(const float* inL, const float* inR,
                                                      float* outL, float* outR,
                                                      size_t numSamples)
{
    if (!inL || !inR || !outL || !outR || numSamples == 0) return;

    if (m_params.bypass) {
        if (outL != inL) std::copy(inL, inL + numSamples, outL);
        if (outR != inR) std::copy(inR, inR + numSamples, outR);
        m_currentCutoffHz = m_params.minCutoffHz;
        m_gustReductionDb = 0.0f;
        m_gustDetected = false;
        return;
    }

    float attAlpha = std::exp(-1.0f / (m_sampleRate * (m_params.attackMs * 0.001f)));
    float relAlpha = std::exp(-1.0f / (m_sampleRate * (m_params.releaseMs * 0.001f)));

    // Detection LP filter at 150 Hz
    float detOmega = 2.0f * static_cast<float>(M_PI) * 150.0f / m_sampleRate;
    float detAlpha = std::sin(detOmega) / (2.0f * 0.7071f);
    float detCos = std::cos(detOmega);
    float detA0 = 1.0f + detAlpha;
    float lpB0 = ((1.0f - detCos) * 0.5f) / detA0;
    float lpB1 = (1.0f - detCos) / detA0;
    float lpB2 = ((1.0f - detCos) * 0.5f) / detA0;
    float lpA1 = (-2.0f * detCos) / detA0;
    float lpA2 = (1.0f - detAlpha) / detA0;

    for (size_t i = 0; i < numSamples; ++i) {
        float xL = inL[i];
        float xR = inR[i];

        // 1. Detect sub-bass gust energy below 150Hz
        float subL = lpB0 * xL + lpB1 * m_detLpX1L + lpB2 * m_detLpX2L - lpA1 * m_detLpY1L - lpA2 * m_detLpY2L;
        m_detLpX2L = m_detLpX1L; m_detLpX1L = xL; m_detLpY2L = m_detLpY1L; m_detLpY1L = subL;

        float subR = lpB0 * xR + lpB1 * m_detLpX1R + lpB2 * m_detLpX2R - lpA1 * m_detLpY1R - lpA2 * m_detLpY2R;
        m_detLpX2R = m_detLpX1R; m_detLpX1R = xR; m_detLpY2R = m_detLpY1R; m_detLpY1R = subR;

        // Sub-bass vs full-band ratio
        float subMag = (std::abs(subL) + std::abs(subR)) * 0.5f;
        float midMag = (std::abs(xL - subL) + std::abs(xR - subR)) * 0.5f + 1e-6f;

        // Wind gusts are characterized by dominant low-frequency energy bursts with low coherence
        float windRatio = (subMag / midMag) * m_params.sensitivity;
        float gustInstant = (subMag > 0.05f && windRatio > 1.2f) ? std::min(1.0f, (windRatio - 1.2f) * 0.5f + subMag * 2.0f) : 0.0f;

        // Envelope ballistics
        if (gustInstant > m_gustEnvelope) {
            m_gustEnvelope = attAlpha * m_gustEnvelope + (1.0f - attAlpha) * gustInstant;
        } else {
            m_gustEnvelope = relAlpha * m_gustEnvelope + (1.0f - relAlpha) * gustInstant;
        }

        m_gustDetected = (m_gustEnvelope > 0.08f);

        // Calculate dynamic cutoff frequency
        float targetCutoff = m_params.minCutoffHz + m_gustEnvelope * (m_params.maxCutoffHz - m_params.minCutoffHz);
        m_currentCutoffHz = 0.95f * m_currentCutoffHz + 0.05f * targetCutoff;

        // Update high-pass filter coeffs periodically or smoothly
        if ((i & 15) == 0) {
            updateDynamicHighpassCoeffs(m_currentCutoffHz);
        }

        // Apply highpass filtering to eliminate wind rumble
        float yL = m_hpB0 * xL + m_hpB1 * m_hpX1L + m_hpB2 * m_hpX2L - m_hpA1 * m_hpY1L - m_hpA2 * m_hpY2L;
        m_hpX2L = m_hpX1L; m_hpX1L = xL; m_hpY2L = m_hpY1L; m_hpY1L = yL;

        float yR = m_hpB0 * xR + m_hpB1 * m_hpX1R + m_hpB2 * m_hpX2R - m_hpA1 * m_hpY1R - m_hpA2 * m_hpY2R;
        m_hpX2R = m_hpX1R; m_hpX1R = xR; m_hpY2R = m_hpY1R; m_hpY1R = yR;

        // Secondary sub-band gust damping if diaphragm overload occurs
        float suppressionGain = 1.0f;
        if (m_gustDetected) {
            float depthLinear = std::pow(10.0f, m_params.reductionDb / 20.0f);
            suppressionGain = 1.0f - m_gustEnvelope * (1.0f - depthLinear);
        }
        m_gustReductionDb = 20.0f * std::log10(std::max(0.001f, suppressionGain));

        float filteredL = yL * suppressionGain;
        float filteredR = yR * suppressionGain;

        if (m_params.deltaSolo) {
            // Delta Solo: Output removed wind noise only
            outL[i] = xL - filteredL;
            outR[i] = xR - filteredR;
        } else {
            outL[i] = filteredL;
            outR[i] = filteredR;
        }
    }

    m_windTurbulenceDb = 20.0f * std::log10(std::max(1e-5f, m_gustEnvelope));
}

} // namespace Jazz
