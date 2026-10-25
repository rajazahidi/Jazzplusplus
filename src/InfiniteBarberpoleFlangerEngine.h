//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZInfiniteBarberpoleFlangerEngine.h - Infinite Barberpole & Through-Zero Flanger
//
// Continuous Shepard-Risset infinite auditory illusion & through-zero flanging:
// Multi-phase cross-faded comb filter bank with raised-cosine windowing,
// seamless upward/downward infinite sweeping without LFO reset clicks, and
// through-zero phase cancellation jet-plane notches.
//*****************************************************************************

#ifndef JAZZ_INFINITE_BARBERPOLE_FLANGER_ENGINE_H
#define JAZZ_INFINITE_BARBERPOLE_FLANGER_ENGINE_H

#include <vector>
#include <string>
#include <cstdint>
#include <cmath>

namespace Jazz {

enum class BarberpoleDirection {
    INFINITE_UPWARD,   // Comb notches sweep endlessly upwards
    INFINITE_DOWNWARD  // Comb notches sweep endlessly downwards
};

class JZInfiniteBarberpoleFlangerEngine {
public:
    JZInfiniteBarberpoleFlangerEngine(float sampleRate = 48000.0f);
    ~JZInfiniteBarberpoleFlangerEngine() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return m_sampleRate; }

    // Flange Modulation Controls
    void setRateHz(float rateHz); // Sweep speed (0.02 Hz to 5.0 Hz)
    float getRateHz() const { return m_rateHz; }

    void setDirection(BarberpoleDirection dir);
    BarberpoleDirection getDirection() const { return m_direction; }

    void setThroughZero(bool enable) { m_throughZero = enable; }
    bool isThroughZero() const { return m_throughZero; }

    void setFeedback(float feedback); // -0.95 to +0.95
    float getFeedback() const { return m_feedback; }

    void setMix(float wetDry); // 0.0 (dry) to 1.0 (wet)
    float getMix() const { return m_mix; }

    // Real-Time Audio Processing
    void process(const float* inL, const float* inR,
                 float* outL, float* outR, size_t numSamples);

    void reset();

private:
    float m_sampleRate;
    float m_rateHz;
    BarberpoleDirection m_direction;
    bool m_throughZero;
    float m_feedback;
    float m_mix;

    // Multi-phase comb filter bank (4 staggered delay phases)
    static const size_t NUM_PHASES = 4;
    double m_phaseAcc[NUM_PHASES];
    float m_feedbackStateL[NUM_PHASES];
    float m_feedbackStateR[NUM_PHASES];

    // Main delay ring buffers
    std::vector<float> m_delayBufferL;
    std::vector<float> m_delayBufferR;
    std::vector<float> m_dryDelayL, m_dryDelayR;
    size_t m_writeIdx;
};

} // namespace Jazz

#endif // JAZZ_INFINITE_BARBERPOLE_FLANGER_ENGINE_H
