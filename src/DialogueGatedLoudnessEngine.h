//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// DialogueGatedLoudnessEngine.h - ITU-R BS.1770-4 / EBU R128 S1 Dialogue Gating
//
// Cinema & OTT streaming broadcast loudness compliance normalizer:
// Voice Activity Detection (VAD) speech gating (-10 LU relative gate),
// K-weighting pre-filtering, LRA range analysis, and automated delivery certification.
//*****************************************************************************

#ifndef JAZZ_DIALOGUE_GATED_LOUDNESS_ENGINE_H
#define JAZZ_DIALOGUE_GATED_LOUDNESS_ENGINE_H

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

enum class DeliveryStandard {
    NETFLIX_STREAMING = 0, // Target: -27.0 LKFS (+/- 2.0 LU), Max TP: -2.0 dBTP
    EBU_R128_EUROPE   = 1, // Target: -23.0 LUFS (+/- 0.5 LU), Max TP: -1.0 dBTP
    ATSC_A85_CALM_ACT = 2  // Target: -24.0 LKFS (+/- 1.0 LU), Max TP: -2.0 dBTP
};

class JZDialogueGatedLoudnessEngine {
public:
    struct LoudnessMetrics {
        float integratedDialogueLkfs = -70.0f;
        float integratedFullMixLufs = -70.0f;
        float momentaryLufs = -70.0f;
        float shortTermLufs = -70.0f;
        float loudnessRangeLra = 0.0f;
        float maxTruePeakDbtp = -70.0f;
        float speechActivityRatio = 0.0f; // 0.0 to 1.0
        bool isCompliant = false;
        float recommendedGainOffsetDb = 0.0f;
    };

    JZDialogueGatedLoudnessEngine(float sampleRate = 48000.0f);
    ~JZDialogueGatedLoudnessEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setTargetStandard(DeliveryStandard standard);
    DeliveryStandard getTargetStandard() const { return m_standard; }

    /**
     * @brief Ingest and analyze stereo audio block for gated dialogue loudness.
     */
    void processBuffer(const float* inL, const float* inR, size_t numSamples);

    /**
     * @brief Retrieve comprehensive compliance metrics.
     */
    LoudnessMetrics getMetrics() const;

    float getIntegratedDialogueLkfs() const { return m_metrics.integratedDialogueLkfs; }
    float getIntegratedFullMixLufs() const { return m_metrics.integratedFullMixLufs; }
    float getLoudnessRangeLra() const { return m_metrics.loudnessRangeLra; }
    float getMaxTruePeakDbtp() const { return m_metrics.maxTruePeakDbtp; }
    bool isCompliant() const { return m_metrics.isCompliant; }

    /**
     * @brief Export standardized QC delivery certificate string.
     */
    std::string generateComplianceCertificate() const;

private:
    float m_sampleRate = 48000.0f;
    DeliveryStandard m_standard = DeliveryStandard::NETFLIX_STREAMING;
    LoudnessMetrics m_metrics;

    // ITU-R BS.1770-4 K-Weighting Filter States
    // Stage 1: High-shelf pre-filter (+4 dB boost around 1.5 kHz)
    float m_shelfB0 = 1.0f, m_shelfB1 = 0.0f, m_shelfB2 = 0.0f;
    float m_shelfA1 = 0.0f, m_shelfA2 = 0.0f;
    float m_shelfX1L = 0.0f, m_shelfX2L = 0.0f, m_shelfY1L = 0.0f, m_shelfY2L = 0.0f;
    float m_shelfX1R = 0.0f, m_shelfX2R = 0.0f, m_shelfY1R = 0.0f, m_shelfY2R = 0.0f;

    // Stage 2: RLB High-pass weighting filter (cut below 100 Hz)
    float m_rlbB0 = 1.0f, m_rlbB1 = 0.0f, m_rlbB2 = 0.0f;
    float m_rlbA1 = 0.0f, m_rlbA2 = 0.0f;
    float m_rlbX1L = 0.0f, m_rlbX2L = 0.0f, m_rlbY1L = 0.0f, m_rlbY2L = 0.0f;
    float m_rlbX1R = 0.0f, m_rlbX2R = 0.0f, m_rlbY1R = 0.0f, m_rlbY2R = 0.0f;

    // Speech Bandpass (300 Hz - 3500 Hz for VAD)
    float m_vadX1L = 0.0f, m_vadX2L = 0.0f, m_vadY1L = 0.0f, m_vadY2L = 0.0f;
    float m_vadX1R = 0.0f, m_vadX2R = 0.0f, m_vadY1R = 0.0f, m_vadY2R = 0.0f;
    float m_vadB0 = 1.0f, m_vadB1 = 0.0f, m_vadB2 = 0.0f, m_vadA1 = 0.0f, m_vadA2 = 0.0f;

    // Block integration storage (100 ms blocks)
    std::vector<float> m_blockPowersFull;
    std::vector<float> m_blockPowersDialogue;
    std::vector<float> m_shortTermHistory;
    size_t m_blockSize = 4800; // 100 ms at 48 kHz
    size_t m_blockSampleIndex = 0;
    float m_accumBlockEnergyFull = 0.0f;
    float m_accumBlockEnergyDialogue = 0.0f;
    float m_accumSpeechBandEnergy = 0.0f;

    float m_peakOverSampled = 0.0f;

    void updateFilterCoefficients();
    void updateComplianceCalculations();
};

} // namespace Jazz

namespace jazz {
    using JZDialogueGatedLoudnessEngine = Jazz::JZDialogueGatedLoudnessEngine;
}

#endif // JAZZ_DIALOGUE_GATED_LOUDNESS_ENGINE_H
