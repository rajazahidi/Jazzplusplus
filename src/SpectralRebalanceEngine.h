#ifndef JAZZ_SPECTRAL_REBALANCE_ENGINE_H
#define JAZZ_SPECTRAL_REBALANCE_ENGINE_H

#include <vector>
#include <cmath>

namespace Jazz {

enum class RebalanceStem {
    VOCALS = 0,
    DRUMS,
    BASS,
    OTHER,
    COUNT
};

struct StemParameters {
    float gainDb;        // -24.0 dB to +12.0 dB
    bool mute;
    bool solo;
    float pan;           // -1.0 (Left) to +1.0 (Right)
};

class JZSpectralRebalanceEngine {
public:
    JZSpectralRebalanceEngine(float sampleRate = 44100.0f);
    ~JZSpectralRebalanceEngine() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return m_sampleRate; }

    void setStemGain(RebalanceStem stem, float gainDb);
    float getStemGain(RebalanceStem stem) const;

    void setStemMute(RebalanceStem stem, bool mute);
    bool isStemMuted(RebalanceStem stem) const;

    void setStemSolo(RebalanceStem stem, bool solo);
    bool isStemSolo(RebalanceStem stem) const;

    void setStemPan(RebalanceStem stem, float pan);
    float getStemPan(RebalanceStem stem) const;

    // Real-time audio processing
    void process(float& left, float& right);
    void processBlock(float* left, float* right, int numSamples);

    // Energy meters per stem for real-time visualization
    float getStemRms(RebalanceStem stem) const;

    void reset();

private:
    float m_sampleRate;
    StemParameters m_stems[static_cast<int>(RebalanceStem::COUNT)];
    float m_stemRms[static_cast<int>(RebalanceStem::COUNT)];

    // Multi-band filterbank states for real-time spectral decomposition
    // 3 crossover biquads: Low/Bass (250Hz), Mid/Vocal (3.5kHz), High/Air
    float m_lpBassL[2], m_lpBassR[2];
    float m_hpBassL[2], m_hpBassR[2];
    float m_lpMidL[2],  m_lpMidR[2];
    float m_hpMidL[2],  m_hpMidR[2];

    // Transient detection envelope state for Drum separation
    float m_transientEnv;
    float m_slowEnv;

    float dbToLinear(float db) const;
    void updateFilterCoeffs();

    // Biquad coefficients
    float m_b0Bass, m_b1Bass, m_b2Bass, m_a1Bass, m_a2Bass;
    float m_b0Mid,  m_b1Mid,  m_b2Mid,  m_a1Mid,  m_a2Mid;
};

} // namespace Jazz

#endif // JAZZ_SPECTRAL_REBALANCE_ENGINE_H
