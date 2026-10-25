//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// AcousticFeedbackSuppressorEngine.cpp - Live Acoustic Feedback Suppression
//*****************************************************************************

#include "AcousticFeedbackSuppressorEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

JZAcousticFeedbackSuppressorEngine::JZAcousticFeedbackSuppressorEngine(float sampleRate) {
    initialize(sampleRate);
}

void JZAcousticFeedbackSuppressorEngine::initialize(float sampleRate) {
    if (sampleRate < 8000.0f) sampleRate = 48000.0f;
    m_sampleRate = sampleRate;

    m_analysisRingL.assign(ANALYSIS_SIZE, 0.0f);
    m_ringWritePos = 0;
    m_samplesSinceLastAnalysis = 0;

    reset();
}

void JZAcousticFeedbackSuppressorEngine::reset() {
    for (size_t i = 0; i < MAX_NOTCHES; ++i) {
        m_notches[i].active = false;
        m_notches[i].holdTimerSec = 0.0f;
        m_notches[i].reset();
    }
    std::fill(m_analysisRingL.begin(), m_analysisRingL.end(), 0.0f);
    m_ringWritePos = 0;
    m_samplesSinceLastAnalysis = 0;
    m_ambientEnergyFloor = 1e-4f;
    m_panicActive = false;
    m_panicDuckingGain = 1.0f;
}

void JZAcousticFeedbackSuppressorEngine::setParameters(const Parameters& params) {
    m_params = params;
    m_params.sensitivityDb = std::max(2.0f, std::min(24.0f, m_params.sensitivityDb));
    m_params.defaultNotchDepthDb = std::max(-48.0f, std::min(-3.0f, m_params.defaultNotchDepthDb));
    m_params.defaultQ = std::max(10.0f, std::min(120.0f, m_params.defaultQ));
    m_params.liveReleaseSec = std::max(1.0f, std::min(60.0f, m_params.liveReleaseSec));
}

void JZAcousticFeedbackSuppressorEngine::setSensitivityDb(float db) {
    m_params.sensitivityDb = std::max(2.0f, std::min(24.0f, db));
}

void JZAcousticFeedbackSuppressorEngine::setDefaultNotchDepthDb(float db) {
    m_params.defaultNotchDepthDb = std::max(-48.0f, std::min(-3.0f, db));
}

void JZAcousticFeedbackSuppressorEngine::setDefaultQ(float q) {
    m_params.defaultQ = std::max(10.0f, std::min(120.0f, q));
}

void JZAcousticFeedbackSuppressorEngine::clearAllNotches() {
    for (size_t i = 0; i < MAX_NOTCHES; ++i) {
        m_notches[i].active = false;
        m_notches[i].reset();
    }
}

void JZAcousticFeedbackSuppressorEngine::clearLiveNotches() {
    for (size_t i = 0; i < MAX_NOTCHES; ++i) {
        if (m_notches[i].type == NotchType::LIVE) {
            m_notches[i].active = false;
            m_notches[i].reset();
        }
    }
}

void JZAcousticFeedbackSuppressorEngine::updateNotchCoefficients(NotchFilter& notch) {
    float f0 = std::max(50.0f, std::min(m_sampleRate * 0.45f, notch.frequencyHz));
    float Q = std::max(5.0f, std::min(120.0f, notch.Q));
    float A = std::pow(10.0f, notch.attenuationDb / 40.0f);

    float w0 = 2.0f * static_cast<float>(M_PI) * (f0 / m_sampleRate);
    float alpha = std::sin(w0) / (2.0f * Q);
    float cos_w = std::cos(w0);

    float b0 = 1.0f + alpha * A;
    float b1 = -2.0f * cos_w;
    float b2 = 1.0f - alpha * A;
    float a0 = 1.0f + alpha / A;
    float a1 = -2.0f * cos_w;
    float a2 = 1.0f - alpha / A;

    notch.b0 = b0 / a0;
    notch.b1 = b1 / a0;
    notch.b2 = b2 / a0;
    notch.a1 = a1 / a0;
    notch.a2 = a2 / a0;
}

bool JZAcousticFeedbackSuppressorEngine::deployNotch(
    float frequencyHz, NotchType type, float depthDb, float q) {

    // Check if a notch already exists near this frequency (+/- 3%)
    for (size_t i = 0; i < MAX_NOTCHES; ++i) {
        if (m_notches[i].active) {
            float diffRatio = std::abs(m_notches[i].frequencyHz - frequencyHz) / frequencyHz;
            if (diffRatio < 0.03f) {
                // Refresh hold timer and deepen notch if needed
                m_notches[i].holdTimerSec = 0.0f;
                if (depthDb < m_notches[i].attenuationDb) {
                    m_notches[i].attenuationDb = depthDb;
                    updateNotchCoefficients(m_notches[i]);
                }
                return true;
            }
        }
    }

    // Find an inactive slot
    for (size_t i = 0; i < MAX_NOTCHES; ++i) {
        if (!m_notches[i].active) {
            m_notches[i].active = true;
            m_notches[i].type = type;
            m_notches[i].frequencyHz = frequencyHz;
            m_notches[i].attenuationDb = depthDb;
            m_notches[i].Q = q;
            m_notches[i].holdTimerSec = 0.0f;
            m_notches[i].reset();
            updateNotchCoefficients(m_notches[i]);
            return true;
        }
    }

    // Replace oldest LIVE notch
    float maxAge = -1.0f;
    int replaceIdx = -1;
    for (size_t i = 0; i < MAX_NOTCHES; ++i) {
        if (m_notches[i].active && m_notches[i].type == NotchType::LIVE) {
            if (m_notches[i].holdTimerSec > maxAge) {
                maxAge = m_notches[i].holdTimerSec;
                replaceIdx = static_cast<int>(i);
            }
        }
    }

    if (replaceIdx >= 0) {
        m_notches[replaceIdx].active = true;
        m_notches[replaceIdx].type = type;
        m_notches[replaceIdx].frequencyHz = frequencyHz;
        m_notches[replaceIdx].attenuationDb = depthDb;
        m_notches[replaceIdx].Q = q;
        m_notches[replaceIdx].holdTimerSec = 0.0f;
        m_notches[replaceIdx].reset();
        updateNotchCoefficients(m_notches[replaceIdx]);
        return true;
    }

    return false;
}

size_t JZAcousticFeedbackSuppressorEngine::getActiveNotchCount() const {
    size_t count = 0;
    for (size_t i = 0; i < MAX_NOTCHES; ++i) {
        if (m_notches[i].active) count++;
    }
    return count;
}

float JZAcousticFeedbackSuppressorEngine::getNotchFrequency(size_t index) const {
    if (index < MAX_NOTCHES && m_notches[index].active) {
        return m_notches[index].frequencyHz;
    }
    return 0.0f;
}

float JZAcousticFeedbackSuppressorEngine::getNotchAttenuationDb(size_t index) const {
    if (index < MAX_NOTCHES && m_notches[index].active) {
        return m_notches[index].attenuationDb;
    }
    return 0.0f;
}

void JZAcousticFeedbackSuppressorEngine::runFeedbackDetection() {
    // 1. Calculate block energy
    float blockEnergy = 0.0f;
    for (size_t i = 0; i < ANALYSIS_SIZE; ++i) {
        blockEnergy += m_analysisRingL[i] * m_analysisRingL[i];
    }
    blockEnergy /= static_cast<float>(ANALYSIS_SIZE);

    m_ambientEnergyFloor = 0.98f * m_ambientEnergyFloor + 0.02f * blockEnergy;
    if (m_ambientEnergyFloor < 1e-5f) m_ambientEnergyFloor = 1e-5f;

    // Check for panic limiter condition
    if (m_params.panicLimiterEnabled && blockEnergy > 0.75f) {
        m_panicActive = true;
        m_panicDuckingGain = 0.25f; // Fast ducking
    } else {
        m_panicDuckingGain = m_panicDuckingGain * 0.99f + 0.01f * 1.0f;
        if (m_panicDuckingGain > 0.95f) m_panicActive = false;
    }

    // 2. Runaway feedback peak detection via zero crossings & autocorrelation
    float threshold = m_ambientEnergyFloor * std::pow(10.0f, m_params.sensitivityDb / 10.0f);
    if (blockEnergy > threshold) {
        // Count zero crossings to estimate dominant frequency
        size_t zcCount = 0;
        for (size_t i = 1; i < ANALYSIS_SIZE; ++i) {
            if ((m_analysisRingL[i - 1] < 0.0f && m_analysisRingL[i] >= 0.0f) ||
                (m_analysisRingL[i - 1] >= 0.0f && m_analysisRingL[i] < 0.0f)) {
                zcCount++;
            }
        }
        float estFreq = (static_cast<float>(zcCount) / 2.0f) * (m_sampleRate / static_cast<float>(ANALYSIS_SIZE));
        if (estFreq >= 120.0f && estFreq <= 12000.0f) {
            deployNotch(estFreq, NotchType::LIVE, m_params.defaultNotchDepthDb, m_params.defaultQ);
        }
    }
}

void JZAcousticFeedbackSuppressorEngine::process(
    const float* inL, const float* inR,
    float* outL, float* outR,
    size_t numSamples) {

    if (!inL || !outL || numSamples == 0) return;
    const bool stereo = (inR != nullptr && outR != nullptr);

    if (!m_params.enabled) {
        for (size_t i = 0; i < numSamples; ++i) {
            outL[i] = inL[i];
            if (stereo) outR[i] = inR[i];
        }
        return;
    }

    for (size_t i = 0; i < numSamples; ++i) {
        float sL = inL[i];
        float sR = stereo ? inR[i] : sL;

        // Ingest into analysis ring buffer
        m_analysisRingL[m_ringWritePos] = sL;
        m_ringWritePos = (m_ringWritePos + 1) % ANALYSIS_SIZE;
        m_samplesSinceLastAnalysis++;

        // Pass through cascade of active notches
        for (size_t n = 0; n < MAX_NOTCHES; ++n) {
            if (!m_notches[n].active) continue;
            NotchFilter& notch = m_notches[n];

            // Biquad difference equation for Left channel
            float yL = notch.b0 * sL + notch.b1 * notch.x1L + notch.b2 * notch.x2L
                       - notch.a1 * notch.y1L - notch.a2 * notch.y2L;
            notch.x2L = notch.x1L; notch.x1L = sL;
            notch.y2L = notch.y1L; notch.y1L = yL;
            sL = yL;

            if (stereo) {
                float yR = notch.b0 * sR + notch.b1 * notch.x1R + notch.b2 * notch.x2R
                           - notch.a1 * notch.y1R - notch.a2 * notch.y2R;
                notch.x2R = notch.x1R; notch.x1R = sR;
                notch.y2R = notch.y1R; notch.y1R = yR;
                sR = yR;
            }
        }

        // Apply panic ducking gain
        outL[i] = sL * m_panicDuckingGain;
        if (stereo) outR[i] = sR * m_panicDuckingGain;
    }

    // Periodic analysis
    if (m_samplesSinceLastAnalysis >= ANALYSIS_SIZE / 2) {
        runFeedbackDetection();
        m_samplesSinceLastAnalysis = 0;

        // Advance hold timers on LIVE notches
        float dt = static_cast<float>(ANALYSIS_SIZE / 2) / m_sampleRate;
        for (size_t n = 0; n < MAX_NOTCHES; ++n) {
            if (m_notches[n].active && m_notches[n].type == NotchType::LIVE) {
                m_notches[n].holdTimerSec += dt;
                if (m_notches[n].holdTimerSec > m_params.liveReleaseSec) {
                    m_notches[n].active = false;
                    m_notches[n].reset();
                }
            }
        }
    }
}

} // namespace Jazz
