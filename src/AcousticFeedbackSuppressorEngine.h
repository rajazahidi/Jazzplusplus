//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// AcousticFeedbackSuppressorEngine.h - Live Acoustic Feedback Suppression
//
// Real-time automatic feedback howl detection and surgical notch hunting:
// Eliminates runaway acoustic feedback oscillations in live stage PA and
// vocal microphones (Sabine FBX / dbx AFS style) without degrading musical tone.
//*****************************************************************************

#ifndef JAZZ_ACOUSTIC_FEEDBACK_SUPPRESSOR_ENGINE_H
#define JAZZ_ACOUSTIC_FEEDBACK_SUPPRESSOR_ENGINE_H

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

class JZAcousticFeedbackSuppressorEngine {
public:
    static const size_t MAX_NOTCHES = 16;

    enum class NotchType {
        FIXED = 0,  // Stays locked once deployed for room acoustic modes
        LIVE  = 1   // Automatically releases when microphone moves away from feedback
    };

    struct NotchFilter {
        bool active = false;
        NotchType type = NotchType::LIVE;
        float frequencyHz = 1000.0f;
        float attenuationDb = -18.0f;
        float Q = 50.0f;
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
        float x1L = 0.0f, x2L = 0.0f, y1L = 0.0f, y2L = 0.0f;
        float x1R = 0.0f, x2R = 0.0f, y1R = 0.0f, y2R = 0.0f;
        float holdTimerSec = 0.0f;

        void reset() {
            x1L = x2L = y1L = y2L = 0.0f;
            x1R = x2R = y1R = y2R = 0.0f;
        }
    };

    struct Parameters {
        float sensitivityDb = 9.0f;          // Energy excess above ambient floor to trigger (3.0 to 18.0 dB)
        float defaultNotchDepthDb = -18.0f;  // Notch cutting depth (-6.0 to -36.0 dB)
        float defaultQ = 50.0f;              // Notch sharpness (20.0 to 90.0)
        float liveReleaseSec = 6.0f;         // Time before a LIVE notch is released (2.0 to 30.0 sec)
        bool panicLimiterEnabled = true;     // Fast broadband ducking on runaway squeal
        bool enabled = true;
    };

    JZAcousticFeedbackSuppressorEngine(float sampleRate = 48000.0f);
    ~JZAcousticFeedbackSuppressorEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setEnabled(bool enabled) { m_params.enabled = enabled; }
    bool isEnabled() const { return m_params.enabled; }

    void setSensitivityDb(float db);
    float getSensitivityDb() const { return m_params.sensitivityDb; }

    void setDefaultNotchDepthDb(float db);
    float getDefaultNotchDepthDb() const { return m_params.defaultNotchDepthDb; }

    void setDefaultQ(float q);
    float getDefaultQ() const { return m_params.defaultQ; }

    /**
     * @brief Clear all live and/or fixed notches.
     */
    void clearAllNotches();
    void clearLiveNotches();

    /**
     * @brief Manually or programmatically deploy a surgical notch.
     */
    bool deployNotch(float frequencyHz, NotchType type = NotchType::LIVE, float depthDb = -18.0f, float q = 50.0f);

    /**
     * @brief Process stereo audio buffer through feedback suppressor engine.
     */
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

    size_t getActiveNotchCount() const;
    float getNotchFrequency(size_t index) const;
    float getNotchAttenuationDb(size_t index) const;
    bool isPanicLimiterEngaged() const { return m_panicActive; }

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;
    NotchFilter m_notches[MAX_NOTCHES];

    // Spectral feedback hunting analysis
    static const size_t ANALYSIS_SIZE = 512;
    std::vector<float> m_analysisRingL;
    size_t m_ringWritePos = 0;
    size_t m_samplesSinceLastAnalysis = 0;

    float m_ambientEnergyFloor = 1e-4f;
    bool m_panicActive = false;
    float m_panicDuckingGain = 1.0f;

    void updateNotchCoefficients(NotchFilter& notch);
    void runFeedbackDetection();
};

} // namespace Jazz

namespace jazz {
    using JZAcousticFeedbackSuppressorEngine = Jazz::JZAcousticFeedbackSuppressorEngine;
}

#endif // JAZZ_ACOUSTIC_FEEDBACK_SUPPRESSOR_ENGINE_H
