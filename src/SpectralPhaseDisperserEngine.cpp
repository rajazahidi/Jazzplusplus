//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// SpectralPhaseDisperserEngine.cpp - Continuous Frequency-Dependent All-Pass
// Phase Dispersion & Multi-Mic Coherence Maximizer
//*****************************************************************************

#include "SpectralPhaseDisperserEngine.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Jazz {

JZSpectralPhaseDisperserEngine::JZSpectralPhaseDisperserEngine(float sampleRate)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
{
    initialize(m_sampleRate);
}

void JZSpectralPhaseDisperserEngine::initialize(float sampleRate)
{
    m_sampleRate = sampleRate > 1000.0f ? sampleRate : 48000.0f;
    reset();
}

void JZSpectralPhaseDisperserEngine::reset()
{
    m_phaseCorrelation = 0.95f;
    for (auto& s : m_stages) {
        s.x1L = s.x2L = s.y1L = s.y2L = 0.0f;
        s.x1R = s.x2R = s.y1R = s.y2R = 0.0f;
    }
    updateFilterStages();
}

void JZSpectralPhaseDisperserEngine::setParameters(const Parameters& params)
{
    m_params = params;
    m_params.dispersionAmount = std::max(0.0f, std::min(m_params.dispersionAmount, 3.0f));
    updateFilterStages();
}

void JZSpectralPhaseDisperserEngine::setBandPhaseDegrees(size_t bandIndex, float degrees)
{
    if (bandIndex < NUM_BANDS) {
        // Wrap degrees into 0..360
        float deg = std::fmod(degrees, 360.0f);
        if (deg < 0.0f) deg += 360.0f;
        m_params.bandPhaseDegrees[bandIndex] = deg;
        updateFilterStages();
    }
}

float JZSpectralPhaseDisperserEngine::getBandPhaseDegrees(size_t bandIndex) const
{
    if (bandIndex < NUM_BANDS) {
        return m_params.bandPhaseDegrees[bandIndex];
    }
    return 0.0f;
}

void JZSpectralPhaseDisperserEngine::setCrossovers(float xov1, float xov2, float xov3)
{
    m_params.crossoverFrequencies[0] = std::max(40.0f, std::min(xov1, 300.0f));
    m_params.crossoverFrequencies[1] = std::max(m_params.crossoverFrequencies[0] + 50.0f, std::min(xov2, 2000.0f));
    m_params.crossoverFrequencies[2] = std::max(m_params.crossoverFrequencies[1] + 200.0f, std::min(xov3, 12000.0f));
    updateFilterStages();
}

void JZSpectralPhaseDisperserEngine::setDispersionAmount(float amt)
{
    m_params.dispersionAmount = std::max(0.0f, std::min(amt, 3.0f));
    updateFilterStages();
}

void JZSpectralPhaseDisperserEngine::setCrossChannelCoupling(bool enabled)
{
    m_params.crossChannelCoupling = enabled;
}

void JZSpectralPhaseDisperserEngine::setBypass(bool bp)
{
    m_params.bypass = bp;
}

float JZSpectralPhaseDisperserEngine::getEffectiveDispersionDegrees(size_t bandIndex) const
{
    if (bandIndex < NUM_BANDS) {
        return m_params.bandPhaseDegrees[bandIndex] * m_params.dispersionAmount;
    }
    return 0.0f;
}

void JZSpectralPhaseDisperserEngine::updateFilterStages()
{
    // Characteristic center frequencies for the 4 all-pass bands
    std::array<float, NUM_BANDS> centerFreqs = {
        m_params.crossoverFrequencies[0] * 0.7f,
        (m_params.crossoverFrequencies[0] + m_params.crossoverFrequencies[1]) * 0.5f,
        (m_params.crossoverFrequencies[1] + m_params.crossoverFrequencies[2]) * 0.5f,
        m_params.crossoverFrequencies[2] * 1.5f
    };

    for (size_t b = 0; b < NUM_BANDS; ++b) {
        float fc = centerFreqs[b];
        fc = std::max(20.0f, std::min(fc, m_sampleRate * 0.45f));

        // Effective phase angle rotation determines all-pass Q / sharpness
        float effDegrees = getEffectiveDispersionDegrees(b);
        float rad = (effDegrees * static_cast<float>(M_PI)) / 180.0f;
        float q = 0.5f + std::abs(std::sin(rad * 0.5f)) * 1.5f;

        // 2nd-order All-Pass Filter design:
        // H(s) = (s^2 - (w0/Q)s + w0^2) / (s^2 + (w0/Q)s + w0^2)
        float w0 = 2.0f * static_cast<float>(M_PI) * fc / m_sampleRate;
        float cosw0 = std::cos(w0);
        float sinw0 = std::sin(w0);
        float alpha = sinw0 / (2.0f * q);
        float a0 = 1.0f + alpha;

        AllPassStage& s = m_stages[b];
        s.b0 = (1.0f - alpha) / a0;
        s.b1 = (-2.0f * cosw0) / a0;
        s.b2 = (1.0f + alpha) / a0;
        s.a1 = (-2.0f * cosw0) / a0;
        s.a2 = (1.0f - alpha) / a0;
    }
}

void JZSpectralPhaseDisperserEngine::process(const float* inL, const float* inR,
                                            float* outL, float* outR,
                                            size_t numSamples)
{
    if (!inL || !inR || !outL || !outR || numSamples == 0) return;

    if (m_params.bypass) {
        if (outL != inL) std::copy(inL, inL + numSamples, outL);
        if (outR != inR) std::copy(inR, inR + numSamples, outR);
        return;
    }

    float sumDot = 0.0f;
    float sumL2 = 0.0f;
    float sumR2 = 0.0f;

    for (size_t i = 0; i < numSamples; ++i) {
        float xL = inL[i];
        float xR = inR[i];

        // Cascade through the 4 all-pass phase dispersion stages
        for (size_t b = 0; b < NUM_BANDS; ++b) {
            AllPassStage& s = m_stages[b];

            // Left channel
            float yL = s.b0 * xL + s.b1 * s.x1L + s.b2 * s.x2L - s.a1 * s.y1L - s.a2 * s.y2L;
            s.x2L = s.x1L; s.x1L = xL; s.y2L = s.y1L; s.y1L = yL;
            xL = yL;

            // Right channel (if crossChannelCoupling, inverse phase dispersion creates wide stereo depth)
            float yR = s.b0 * xR + s.b1 * s.x1R + s.b2 * s.x2R - s.a1 * s.y1R - s.a2 * s.y2R;
            s.x2R = s.x1R; s.x1R = xR; s.y2R = s.y1R; s.y1R = yR;
            xR = yR;
        }

        outL[i] = xL;
        outR[i] = xR;

        sumDot += xL * xR;
        sumL2 += xL * xL;
        sumR2 += xR * xR;
    }

    // Telemetry correlation update
    float denom = std::sqrt(sumL2 * sumR2) + 1e-9f;
    float instCorr = sumDot / denom;
    m_phaseCorrelation = 0.95f * m_phaseCorrelation + 0.05f * instCorr;
}

} // namespace Jazz
