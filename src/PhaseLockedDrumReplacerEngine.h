//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// PhaseLockedDrumReplacerEngine.h - Phase-Locked Acoustic Drum Replacer
//
// Real-time acoustic drum transient tracker & sample replacer (Slate Trigger 2 /
// Superior Drummer 3 style). Features sub-sample cross-correlation phase-lock
// alignment between acoustic mic bleed and triggered replacement samples,
// bleed rejection filtering, and multi-velocity layer triggering.
//*****************************************************************************

#ifndef JAZZ_PHASE_LOCKED_DRUM_REPLACER_ENGINE_H
#define JAZZ_PHASE_LOCKED_DRUM_REPLACER_ENGINE_H

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

enum class DrumReplacerInstrument {
    SNARE = 0,
    KICK = 1,
    TOM = 2
};

class JZPhaseLockedDrumReplacerEngine {
public:
    struct Parameters {
        DrumReplacerInstrument instrument = DrumReplacerInstrument::SNARE;
        float triggerThresholdDb = -20.0f;     // Detection threshold (-40 to -6 dB)
        float sensitivity = 0.85f;            // Dynamic sensitivity (0.1 to 1.0)
        float retriggerHoldoffMs = 35.0f;     // Minimum time between hits (10 to 150 ms)
        bool enablePhaseLock = true;          // Sub-sample cross-correlation alignment
        bool invertPolarity = false;          // 180-degree phase flip
        float dryWet = 0.75f;                 // Blend between original acoustic mic and sample
        bool bypassed = false;
    };

    JZPhaseLockedDrumReplacerEngine(float sampleRate = 48000.0f);
    ~JZPhaseLockedDrumReplacerEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setInstrument(DrumReplacerInstrument inst);
    DrumReplacerInstrument getInstrument() const { return m_params.instrument; }

    void setTriggerThresholdDb(float db);
    float getTriggerThresholdDb() const { return m_params.triggerThresholdDb; }

    void setSensitivity(float sens);
    float getSensitivity() const { return m_params.sensitivity; }

    void setRetriggerHoldoffMs(float ms);
    float getRetriggerHoldoffMs() const { return m_params.retriggerHoldoffMs; }

    void setPhaseLockEnabled(bool enable);
    bool isPhaseLockEnabled() const { return m_params.enablePhaseLock; }

    void setInvertPolarity(bool invert);
    bool isInvertPolarity() const { return m_params.invertPolarity; }

    void setDryWet(float wet);
    float getDryWet() const { return m_params.dryWet; }

    void setBypass(bool bypass);
    bool isBypassed() const { return m_params.bypassed; }

    // Manual Hit Trigger (for unit tests / external MIDI)
    void triggerHit(float velocity = 1.0f);

    // Telemetry & Getters
    size_t getDetectedHitsCount() const { return m_hitsCount; }
    float getLastVelocity() const { return m_lastVelocity; }
    float getPhaseOffsetSamples() const { return m_phaseOffsetSamples; }
    bool isHitTriggered() const { return m_sampleActive; }

    // Audio Processing
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

private:
    void generateInternalSamplePool();
    void updateDetectionFilters();

    float m_sampleRate = 48000.0f;
    Parameters m_params;

    // Transient Detection Bandpass Biquad Filter (isolates drum body from cymbal spill)
    float m_bpB0 = 0.0f, m_bpB1 = 0.0f, m_bpB2 = 0.0f, m_bpA1 = 0.0f, m_bpA2 = 0.0f;
    float m_bpX1L = 0.0f, m_bpX2L = 0.0f, m_bpY1L = 0.0f, m_bpY2L = 0.0f;

    float m_envelope = 0.0f;
    size_t m_holdoffSamples = 0;
    size_t m_holdoffCounter = 0;

    // Internal Synthetic Multi-Velocity Sample Waveform
    std::vector<float> m_sampleSoft;
    std::vector<float> m_sampleHard;
    bool m_sampleActive = false;
    size_t m_samplePlayPos = 0;
    float m_currentHitGain = 0.0f;
    float m_currentHitHardness = 0.0f;

    // Cross-Correlation Phase-Lock Ring Buffer
    static constexpr size_t CORR_BUF_SIZE = 128;
    std::vector<float> m_inputHistory;
    size_t m_historyPos = 0;
    float m_phaseOffsetSamples = 0.0f;

    size_t m_hitsCount = 0;
    float m_lastVelocity = 0.0f;
};

} // namespace Jazz

namespace jazz {
    using JZPhaseLockedDrumReplacerEngine = Jazz::JZPhaseLockedDrumReplacerEngine;
    using DrumReplacerInstrument = Jazz::DrumReplacerInstrument;
}

#endif // JAZZ_PHASE_LOCKED_DRUM_REPLACER_ENGINE_H
