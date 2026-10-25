//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZDynamicResonanceSuppressorEngine.h - Dynamic Spectral Resonance Suppressor
//
// Dynamic spectral resonance suppression & de-bleeding (Oeksound Soothe 2 style):
// Threshold-free spectral resonance tracking, multi-band dynamic notch damping,
// sharpness/selectivity Q controls, and delta-monitoring for harsh frequencies.
//*****************************************************************************

#ifndef JAZZ_DYNAMIC_RESONANCE_SUPPRESSOR_ENGINE_H
#define JAZZ_DYNAMIC_RESONANCE_SUPPRESSOR_ENGINE_H

#include <vector>
#include <string>
#include <cstdint>
#include <cmath>

namespace Jazz {

struct ResonancePeakInfo {
    float frequencyHz;
    float deviationDb;     // Peak protrusion above spectral baseline
    float suppressionDb;   // Active dynamic attenuation applied
    float qFactor;
};

class JZDynamicResonanceSuppressorEngine {
public:
    JZDynamicResonanceSuppressorEngine(float sampleRate = 48000.0f);
    ~JZDynamicResonanceSuppressorEngine() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return m_sampleRate; }

    // Core Processing Controls
    void setDepth(float depth); // 0.0 (bypass) to 1.0 (maximum suppression)
    float getDepth() const { return m_depth; }

    void setSharpness(float sharpness); // 0.5 (broad) to 10.0 (surgical notches)
    float getSharpness() const { return m_sharpness; }

    void setSelectivity(float selectivity); // 0.0 (damp all) to 1.0 (surgical spikes only)
    float getSelectivity() const { return m_selectivity; }

    void setAttackReleaseMs(float attackMs, float releaseMs);

    // Monitoring Mode: Delta mode allows listening strictly to the removed harshness
    void setDeltaMode(bool delta) { m_deltaMode = delta; }
    bool isDeltaMode() const { return m_deltaMode; }

    // Real-Time Audio Processing
    void process(const float* inL, const float* inR,
                 float* outL, float* outR, size_t numSamples);

    // Telemetry & Active Resonance Detection
    size_t getActiveResonanceCount() const { return m_detectedPeaks.size(); }
    float getMaxSuppressionDb() const { return m_maxSuppressionDb; }
    const std::vector<ResonancePeakInfo>& getDetectedPeaks() const { return m_detectedPeaks; }

    void reset();

private:
    float m_sampleRate;
    float m_depth;
    float m_sharpness;
    float m_selectivity;
    float m_attackMs;
    float m_releaseMs;
    bool m_deltaMode;
    float m_maxSuppressionDb;

    std::vector<ResonancePeakInfo> m_detectedPeaks;

    // Multi-band dynamic band-pass filter states (8 tracking bands: 500Hz to 12kHz)
    static const size_t NUM_BANDS = 8;
    float m_bandCenterFreqs[NUM_BANDS];
    float m_bandEnvelopesL[NUM_BANDS];
    float m_bandEnvelopesR[NUM_BANDS];
    float m_bandGainsDb[NUM_BANDS];

    // Filter states for each band
    float m_filterState1L[NUM_BANDS], m_filterState2L[NUM_BANDS];
    float m_filterState1R[NUM_BANDS], m_filterState2R[NUM_BANDS];

    void initBands();
};

} // namespace Jazz

#endif // JAZZ_DYNAMIC_RESONANCE_SUPPRESSOR_ENGINE_H
