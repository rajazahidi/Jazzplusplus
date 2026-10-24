#ifndef JAZZ_THROUGH_ZERO_FLANGER_H
#define JAZZ_THROUGH_ZERO_FLANGER_H

#include <vector>
#include <cmath>

namespace Jazz {

enum class FlangerPolarity {
    POSITIVE, // Additive at through-zero (+6 dB reinforcement)
    INVERTED  // Subtractive at through-zero (Complete infinite notch null)
};

class JZThroughZeroFlanger {
public:
    JZThroughZeroFlanger(float sampleRate = 44100.0f);
    ~JZThroughZeroFlanger() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return m_sampleRate; }

    void setRateHz(float rateHz);
    float getRateHz() const { return m_rateHz; }

    void setDepthMs(float depthMs);
    float getDepthMs() const { return m_depthMs; }

    void setCenterDelayMs(float centerMs);
    float getCenterDelayMs() const { return m_centerDelayMs; }

    void setFeedback(float feedback); // -0.95 to +0.95
    float getFeedback() const { return m_feedback; }

    void setPolarity(FlangerPolarity polarity) { m_polarity = polarity; }
    FlangerPolarity getPolarity() const { return m_polarity; }

    void setMix(float mix); // 0.0 (dry) to 1.0 (50/50 through-zero mix)
    float getMix() const { return m_mix; }

    void setTapeDrive(float drive); // 1.0 (linear) to 4.0 (warm tape saturation)
    float getTapeDrive() const { return m_tapeDrive; }

    void setStereoPhaseOffsetDeg(float deg); // 0 to 180 degrees
    float getStereoPhaseOffsetDeg() const { return m_stereoPhaseDeg; }

    void setManualMode(bool enable) { m_manualMode = enable; }
    bool isManualMode() const { return m_manualMode; }
    void setManualPosition(float pos); // -1.0 to +1.0 (manual tape reel brake pressing)

    // Real-time audio processing
    void process(float& left, float& right);
    void processBlock(float* left, float* right, int numSamples);

    // Current relative delay offset (0.0 ms = Through-Zero point)
    float getCurrentZeroOffsetMs() const { return m_currentOffsetMs; }

    void reset();

private:
    float fractionalDelay(const float* buffer, int bufSize, int writeIdx, float delaySamples) const;
    float tapeSaturate(float in) const;

    float m_sampleRate;
    float m_rateHz;
    float m_depthMs;
    float m_centerDelayMs;
    float m_feedback;
    FlangerPolarity m_polarity;
    float m_mix;
    float m_tapeDrive;
    float m_stereoPhaseDeg;

    bool m_manualMode;
    float m_manualPos;

    // Dual delay lines: Reference line and Modulated line
    static const int DELAY_SIZE = 4096;
    float m_dryDelayBufL[DELAY_SIZE];
    float m_dryDelayBufR[DELAY_SIZE];
    float m_wetDelayBufL[DELAY_SIZE];
    float m_wetDelayBufR[DELAY_SIZE];
    int m_writeIdx;

    float m_lfoPhaseL;
    float m_lfoPhaseR;
    float m_currentOffsetMs;

    float m_feedbackStateL;
    float m_feedbackStateR;
};

} // namespace Jazz

#endif // JAZZ_THROUGH_ZERO_FLANGER_H
