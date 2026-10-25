//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// LoudnessRangeDynamicsSculptorEngine.h - LRA Sculptor & True-Peak Guard
//
// Broadcast Loudness Range (LRA) macro-dynamic contour sculptor and 8x oversampled
// True-Peak compliance limiter (TC Electronic Master X / NUGEN ISL style).
// Conforms audio mixes to strict streaming targets (Spotify, Apple Music, EBU R128)
// with intelligent macro-dynamic LRA control and inter-sample overshoot prevention.
//*****************************************************************************

#ifndef JAZZ_LOUDNESS_RANGE_DYNAMICS_SCULPTOR_ENGINE_H
#define JAZZ_LOUDNESS_RANGE_DYNAMICS_SCULPTOR_ENGINE_H

#include <vector>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

class JZLoudnessRangeDynamicsSculptorEngine {
public:
    struct Parameters {
        float targetLraLu = 8.0f;             // Desired Loudness Range in LU (4.0 to 18.0 LU)
        float targetLufs = -14.0f;            // Integrated LUFS streaming target (-24.0 to -9.0 LUFS)
        float truePeakCeilingDbtp = -1.0f;    // Inter-sample true-peak limit (-3.0 to 0.0 dBTP)
        float sculptingSpeed = 0.5f;          // Macro-dynamic adaptation speed (0.1 to 1.0)
        bool enableTruePeakLimiter = true;    // 8x oversampled brickwall limiter
        float dryWet = 1.0f;
        bool bypassed = false;
    };

    JZLoudnessRangeDynamicsSculptorEngine(float sampleRate = 48000.0f);
    ~JZLoudnessRangeDynamicsSculptorEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setTargetLraLu(float lra);
    float getTargetLraLu() const { return m_params.targetLraLu; }

    void setTargetLufs(float lufs);
    float getTargetLufs() const { return m_params.targetLufs; }

    void setTruePeakCeilingDbtp(float dbtp);
    float getTruePeakCeilingDbtp() const { return m_params.truePeakCeilingDbtp; }

    void setSculptingSpeed(float speed);
    float getSculptingSpeed() const { return m_params.sculptingSpeed; }

    void setTruePeakLimiterEnabled(bool enable);
    bool isTruePeakLimiterEnabled() const { return m_params.enableTruePeakLimiter; }

    void setDryWet(float wet);
    float getDryWet() const { return m_params.dryWet; }

    void setBypass(bool bypass);
    bool isBypassed() const { return m_params.bypassed; }

    // Telemetry & Getters
    float getCurrentLraLu() const { return m_currentLraLu; }
    float getIntegratedLufs() const { return m_integratedLufs; }
    float getShortTermLufs() const { return m_shortTermLufs; }
    float getTruePeakDbtp() const { return m_truePeakDbtp; }
    float getGainCorrectionDb() const { return m_currentGainCorrDb; }

    // Audio Processing
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

private:
    void updateKWeightingFilters();

    float m_sampleRate = 48000.0f;
    Parameters m_params;

    // ITU-R BS.1770-4 K-Weighting Stage 1: High-Shelf Filter
    float m_hsB0 = 1.0f, m_hsB1 = 0.0f, m_hsB2 = 0.0f, m_hsA1 = 0.0f, m_hsA2 = 0.0f;
    float m_hsX1L = 0.0f, m_hsX2L = 0.0f, m_hsY1L = 0.0f, m_hsY2L = 0.0f;
    float m_hsX1R = 0.0f, m_hsX2R = 0.0f, m_hsY1R = 0.0f, m_hsY2R = 0.0f;

    // ITU-R BS.1770-4 K-Weighting Stage 2: High-Pass RLB Filter
    float m_hpB0 = 1.0f, m_hpB1 = 0.0f, m_hpB2 = 0.0f, m_hpA1 = 0.0f, m_hpA2 = 0.0f;
    float m_hpX1L = 0.0f, m_hpX2L = 0.0f, m_hpY1L = 0.0f, m_hpY2L = 0.0f;
    float m_hpX1R = 0.0f, m_hpX2R = 0.0f, m_hpY1R = 0.0f, m_hpY2R = 0.0f;

    // Loudness Tracking Buffers
    float m_shortTermEnergy = 0.0f;
    float m_integratedEnergy = 0.0f;
    size_t m_integratedSampleCount = 0;

    std::vector<float> m_shortTermHistory;
    size_t m_historyWritePos = 0;

    float m_currentLraLu = 8.0f;
    float m_integratedLufs = -14.0f;
    float m_shortTermLufs = -14.0f;
    float m_truePeakDbtp = -1.0f;
    float m_currentGainCorrDb = 0.0f;
};

} // namespace Jazz

namespace jazz {
    using JZLoudnessRangeDynamicsSculptorEngine = Jazz::JZLoudnessRangeDynamicsSculptorEngine;
}

#endif // JAZZ_LOUDNESS_RANGE_DYNAMICS_SCULPTOR_ENGINE_H
