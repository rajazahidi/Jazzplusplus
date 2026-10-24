#ifndef JAZZ_BRICKWALL_CLIPPER_ENGINE_H
#define JAZZ_BRICKWALL_CLIPPER_ENGINE_H

#include <vector>
#include <cmath>

namespace Jazz {

enum class ClipperCurve {
    HARD_BRICKWALL,     // Transparent pure hard clip for transient punch
    SOFT_KNEE_PARABOLIC,// Smooth musical transition into clipping
    TUBE_DIODE,         // Analog triode/pentode asymmetric warmth
    APOGEE_SOFT_LIMIT   // Broadcast studio tape-like soft limiting
};

class JZBrickwallClipperEngine {
public:
    JZBrickwallClipperEngine(float sampleRate = 44100.0f);
    ~JZBrickwallClipperEngine() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return m_sampleRate; }

    void setCurve(ClipperCurve curve) { m_curve = curve; }
    ClipperCurve getCurve() const { return m_curve; }

    void setInputGainDb(float gainDb);
    float getInputGainDb() const { return m_inputGainDb; }

    void setOutputGainDb(float gainDb);
    float getOutputGainDb() const { return m_outputGainDb; }

    void setCeilingDb(float ceilingDb); // e.g. -0.1 dBTP
    float getCeilingDb() const { return m_ceilingDb; }

    void setKneeWidthDb(float kneeDb); // 0.0 to 6.0 dB
    float getKneeWidthDb() const { return m_kneeWidthDb; }

    void setOversampling(int factor); // 1, 2, 4, 8
    int getOversampling() const { return m_oversamplingFactor; }

    // Real-time audio processing
    void process(float& left, float& right);
    void processBlock(float* left, float* right, int numSamples);

    // Dynamic telemetry
    float getCurrentClippingReductionDb() const { return m_currentReductionDb; }
    float getMaxTruePeakOutDb() const { return m_maxTruePeakOutDb; }

    void reset();

private:
    float clipSample(float x, float ceilingLinear, float kneeLinear) const;
    float dbToLinear(float db) const;
    float linearToDb(float lin) const;

    float m_sampleRate;
    ClipperCurve m_curve;
    float m_inputGainDb;
    float m_outputGainDb;
    float m_ceilingDb;
    float m_kneeWidthDb;
    int m_oversamplingFactor;

    float m_currentReductionDb;
    float m_maxTruePeakOutDb;

    // Polyphase FIR filter state for 4x / 8x oversampling anti-aliasing
    static const int FIR_TAPS = 15;
    float m_firHistoryL[FIR_TAPS];
    float m_firHistoryR[FIR_TAPS];
    int m_firWriteIdx;
};

} // namespace Jazz

#endif // JAZZ_BRICKWALL_CLIPPER_ENGINE_H
