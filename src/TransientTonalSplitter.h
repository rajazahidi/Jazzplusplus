#ifndef JAZZ_TRANSIENT_TONAL_SPLITTER_H
#define JAZZ_TRANSIENT_TONAL_SPLITTER_H

#include <vector>
#include <cmath>

namespace Jazz {

enum class SplitterOutputMode {
    COMBINED_MIX,    // Recombined mix with independent transient & tonal gains
    SOLO_TRANSIENT,  // Listen to transient attacks only
    SOLO_TONAL       // Listen to sustained tonal body only
};

class JZTransientTonalSplitter {
public:
    JZTransientTonalSplitter(float sampleRate = 44100.0f);
    ~JZTransientTonalSplitter() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return m_sampleRate; }

    void setTransientGainDb(float gainDb);
    float getTransientGainDb() const { return m_transientGainDb; }

    void setTonalGainDb(float gainDb);
    float getTonalGainDb() const { return m_tonalGainDb; }

    void setSensitivity(float sensitivity); // 0.0 to 1.0
    float getSensitivity() const { return m_sensitivity; }

    void setTransientDecayMs(float decayMs); // 5.0 to 100.0 ms
    float getTransientDecayMs() const { return m_transientDecayMs; }

    void setOutputMode(SplitterOutputMode mode) { m_outputMode = mode; }
    SplitterOutputMode getOutputMode() const { return m_outputMode; }

    // Real-time audio processing
    // Splits input into transient and tonal parts, modifies gains, and outputs reconstructed audio
    void process(float& left, float& right);

    // Explicit split: decomposes stereo input into separate transient and tonal stereo pairs
    void splitExplicit(float inL, float inR,
                       float& outTransL, float& outTransR,
                       float& outTonalL, float& outTonalR);

    void processBlock(float* left, float* right, int numSamples);

    // Telemetry meters
    float getTransientRms() const { return m_transientRms; }
    float getTonalRms() const { return m_tonalRms; }
    float getCurrentTransientWeight() const { return m_currentTransWeight; }

    void reset();

private:
    float dbToLinear(float db) const;
    void updateBallistics();

    float m_sampleRate;
    float m_transientGainDb;
    float m_tonalGainDb;
    float m_sensitivity;
    float m_transientDecayMs;
    SplitterOutputMode m_outputMode;

    // Dual envelope tracking states
    float m_fastAttackCoeff;
    float m_fastReleaseCoeff;
    float m_slowAttackCoeff;
    float m_slowReleaseCoeff;

    float m_fastEnvL, m_fastEnvR;
    float m_slowEnvL, m_slowEnvR;

    float m_transientRms;
    float m_tonalRms;
    float m_currentTransWeight;
};

} // namespace Jazz

#endif // JAZZ_TRANSIENT_TONAL_SPLITTER_H
