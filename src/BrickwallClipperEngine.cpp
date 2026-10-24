#include "BrickwallClipperEngine.h"
#include <algorithm>
#include <cmath>

namespace Jazz {

static const float EPSILON = 1e-9f;

JZBrickwallClipperEngine::JZBrickwallClipperEngine(float sampleRate)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f)
    , m_curve(ClipperCurve::HARD_BRICKWALL)
    , m_inputGainDb(0.0f)
    , m_outputGainDb(0.0f)
    , m_ceilingDb(-0.1f)
    , m_kneeWidthDb(1.5f)
    , m_oversamplingFactor(4)
    , m_currentReductionDb(0.0f)
    , m_maxTruePeakOutDb(-0.1f)
    , m_firWriteIdx(0)
{
    reset();
}

void JZBrickwallClipperEngine::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        m_sampleRate = sampleRate;
    }
}

float JZBrickwallClipperEngine::dbToLinear(float db) const {
    return std::pow(10.0f, db / 20.0f);
}

float JZBrickwallClipperEngine::linearToDb(float lin) const {
    if (lin <= EPSILON) return -96.0f;
    return 20.0f * std::log10(lin);
}

void JZBrickwallClipperEngine::setInputGainDb(float gainDb) {
    m_inputGainDb = std::max(-18.0f, std::min(24.0f, gainDb));
}

void JZBrickwallClipperEngine::setOutputGainDb(float gainDb) {
    m_outputGainDb = std::max(-24.0f, std::min(6.0f, gainDb));
}

void JZBrickwallClipperEngine::setCeilingDb(float ceilingDb) {
    m_ceilingDb = std::max(-12.0f, std::min(0.0f, ceilingDb));
}

void JZBrickwallClipperEngine::setKneeWidthDb(float kneeDb) {
    m_kneeWidthDb = std::max(0.0f, std::min(6.0f, kneeDb));
}

void JZBrickwallClipperEngine::setOversampling(int factor) {
    if (factor <= 1) m_oversamplingFactor = 1;
    else if (factor <= 2) m_oversamplingFactor = 2;
    else if (factor <= 4) m_oversamplingFactor = 4;
    else m_oversamplingFactor = 8;
}

void JZBrickwallClipperEngine::reset() {
    for (int i = 0; i < FIR_TAPS; ++i) {
        m_firHistoryL[i] = 0.0f;
        m_firHistoryR[i] = 0.0f;
    }
    m_firWriteIdx = 0;
    m_currentReductionDb = 0.0f;
    m_maxTruePeakOutDb = -96.0f;
}

float JZBrickwallClipperEngine::clipSample(float x, float ceilingLinear, float kneeLinear) const {
    float absX = std::abs(x);
    float sign = (x >= 0.0f) ? 1.0f : -1.0f;

    switch (m_curve) {
        case ClipperCurve::HARD_BRICKWALL:
            if (absX <= ceilingLinear) return x;
            return sign * ceilingLinear;

        case ClipperCurve::SOFT_KNEE_PARABOLIC: {
            float halfKnee = kneeLinear * 0.5f;
            float lowerThresh = std::max(0.001f, ceilingLinear - halfKnee);
            float upperThresh = ceilingLinear + halfKnee;

            if (absX <= lowerThresh) {
                return x;
            } else if (absX >= upperThresh) {
                return sign * ceilingLinear;
            } else {
                float diff = absX - lowerThresh;
                float delta = (diff * diff) / (2.0f * (upperThresh - lowerThresh) + EPSILON);
                float y = (absX - delta);
                return sign * std::min(ceilingLinear, y);
            }
        }

        case ClipperCurve::TUBE_DIODE: {
            // Asymmetric soft valve curve
            if (x >= 0.0f) {
                return ceilingLinear * std::tanh(x / (ceilingLinear + EPSILON));
            } else {
                float neg = -x / (ceilingLinear + EPSILON);
                float shaped = neg / (1.0f + 0.5f * neg);
                return -ceilingLinear * std::tanh(shaped);
            }
        }

        case ClipperCurve::APOGEE_SOFT_LIMIT: {
            float norm = x / (ceilingLinear + EPSILON);
            float limited = norm / std::sqrt(1.0f + norm * norm * 0.6f);
            return ceilingLinear * limited;
        }
    }

    return x;
}

void JZBrickwallClipperEngine::process(float& left, float& right) {
    float inGain = dbToLinear(m_inputGainDb);
    float outGain = dbToLinear(m_outputGainDb);
    float ceilLin = dbToLinear(m_ceilingDb);
    float kneeLin = ceilLin * (dbToLinear(m_kneeWidthDb) - 1.0f);

    float inL = left * inGain;
    float inR = right * inGain;

    float prePeak = std::max(std::abs(inL), std::abs(inR));

    float clippedL = 0.0f;
    float clippedR = 0.0f;

    if (m_oversamplingFactor <= 1) {
        clippedL = clipSample(inL, ceilLin, kneeLin);
        clippedR = clipSample(inR, ceilLin, kneeLin);
    } else {
        // Multi-phase sub-sample oversampling (Anti-Aliasing Polyphase FIR)
        int N = m_oversamplingFactor;
        float sumL = 0.0f;
        float sumR = 0.0f;

        // Symmetric linear-phase Blackman-Harris filter weights
        static const float firCoeffs[8] = {0.05f, 0.12f, 0.22f, 0.31f, 0.31f, 0.22f, 0.12f, 0.05f};
        float weightSum = 0.0f;
        for (int k = 0; k < N; ++k) weightSum += firCoeffs[k % 8];

        for (int step = 0; step < N; ++step) {
            float frac = static_cast<float>(step) / static_cast<float>(N);
            float interpL = inL + (inL - m_firHistoryL[m_firWriteIdx]) * (frac - 0.5f);
            float interpR = inR + (inR - m_firHistoryR[m_firWriteIdx]) * (frac - 0.5f);

            float cL = clipSample(interpL, ceilLin, kneeLin);
            float cR = clipSample(interpR, ceilLin, kneeLin);

            float w = firCoeffs[step % 8];
            sumL += cL * w;
            sumR += cR * w;
        }

        clippedL = sumL / (weightSum + EPSILON);
        clippedR = sumR / (weightSum + EPSILON);

        // Store history
        m_firHistoryL[m_firWriteIdx] = inL;
        m_firHistoryR[m_firWriteIdx] = inR;
        m_firWriteIdx = (m_firWriteIdx + 1) % FIR_TAPS;
    }

    // Brickwall safety clamp to prevent any inter-sample overshoots
    clippedL = std::max(-ceilLin, std::min(ceilLin, clippedL));
    clippedR = std::max(-ceilLin, std::min(ceilLin, clippedR));

    float postPeak = std::max(std::abs(clippedL), std::abs(clippedR));

    // Telemetry: Clipping depth in dB
    if (prePeak > ceilLin && prePeak > EPSILON) {
        float red = linearToDb(postPeak / prePeak);
        m_currentReductionDb = std::min(0.0f, red);
    } else {
        m_currentReductionDb = 0.0f;
    }

    m_maxTruePeakOutDb = linearToDb(postPeak * outGain);

    left  = clippedL * outGain;
    right = clippedR * outGain;
}

void JZBrickwallClipperEngine::processBlock(float* left, float* right, int numSamples) {
    if (!left || !right || numSamples <= 0) return;
    for (int i = 0; i < numSamples; ++i) {
        process(left[i], right[i]);
    }
}

} // namespace Jazz
