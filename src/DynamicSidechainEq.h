#ifndef JAZZ_DYNAMIC_SIDECHAIN_EQ_H
#define JAZZ_DYNAMIC_SIDECHAIN_EQ_H

#include <vector>
#include <cmath>

namespace Jazz {

enum class SidechainCarveMode {
    SPECTRAL_MULTI_BAND, // Frequency-specific smart carving (Trackspacer style)
    PARAMETRIC_TARGET    // Targeted parametric ducking at specified center frequency
};

class JZDynamicSidechainEq {
public:
    JZDynamicSidechainEq(float sampleRate = 44100.0f);
    ~JZDynamicSidechainEq() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return m_sampleRate; }

    void setCarveMode(SidechainCarveMode mode) { m_mode = mode; }
    SidechainCarveMode getCarveMode() const { return m_mode; }

    void setAmountPercent(float amount); // 0% to 100% carving depth
    float getAmountPercent() const { return m_amountPercent; }

    void setSidechainBandLimits(float lowCutHz, float highCutHz);
    float getLowCutHz() const { return m_lowCutHz; }
    float getHighCutHz() const { return m_highCutHz; }

    void setAttackMs(float attackMs);
    float getAttackMs() const { return m_attackMs; }

    void setReleaseMs(float releaseMs);
    float getReleaseMs() const { return m_releaseMs; }

    // Parametric target band (used when mode == PARAMETRIC_TARGET)
    void setTargetBand(float centerFreqHz, float Q = 1.4f);
    float getTargetCenterFreqHz() const { return m_targetFreqHz; }

    // Real-time processing: Main signal is modified based on Sidechain input
    void process(float& mainL, float& mainR, float sidechainL, float sidechainR);
    void processBlock(float* mainL, float* mainR, const float* scL, const float* scR, int numSamples);

    // Dynamic metering
    float getCurrentReductionDb() const { return m_currentMaxReductionDb; }
    float getBandReductionDb(int bandIdx) const;

    void reset();

private:
    void updateFilterCoeffs();

    float m_sampleRate;
    SidechainCarveMode m_mode;
    float m_amountPercent;
    float m_lowCutHz;
    float m_highCutHz;
    float m_attackMs;
    float m_releaseMs;
    float m_targetFreqHz;
    float m_targetQ;

    float m_attackCoeff;
    float m_releaseCoeff;
    float m_currentMaxReductionDb;

    // 5-Band Spectral Carve Filterbank:
    // Band 0: Sub (60 Hz), Band 1: Bass (180 Hz), Band 2: Low-Mid (600 Hz),
    // Band 3: High-Mid (2.5 kHz), Band 4: Air (8.0 kHz)
    static const int NUM_BANDS = 5;
    float m_bandCenterFreqs[NUM_BANDS];
    float m_sidechainEnvelopes[NUM_BANDS];
    float m_bandReductionsDb[NUM_BANDS];

    // Filter states for Sidechain analysis bandpass biquads
    float m_scBpStateL[NUM_BANDS][2];
    float m_scBpStateR[NUM_BANDS][2];
    float m_scBpB0[NUM_BANDS], m_scBpB1[NUM_BANDS], m_scBpB2[NUM_BANDS], m_scBpA1[NUM_BANDS], m_scBpA2[NUM_BANDS];

    // Filter states for Main audio dynamic cut peaking biquads
    float m_mainEqStateL[NUM_BANDS][2];
    float m_mainEqStateR[NUM_BANDS][2];

    // Sidechain highpass and lowpass state
    float m_scHpState[2];
    float m_scLpState[2];
    float m_scHpB0, m_scHpB1, m_scHpB2, m_scHpA1, m_scHpA2;
    float m_scLpB0, m_scLpB1, m_scLpB2, m_scLpA1, m_scLpA2;
};

} // namespace Jazz

#endif // JAZZ_DYNAMIC_SIDECHAIN_EQ_H
