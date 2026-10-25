//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// ImmersiveBassManagementCrossoverEngine.h - 9.1.6 Dolby Atmos Bass Manager
//
// Multi-channel immersive speaker bass management (Dolby Atmos RMU / Trinnov style):
// 4th-order 24dB/oct Linkwitz-Riley crossover extracting & redirecting sub-80Hz bass
// from 15 satellite speakers to the LFE / Subwoofer channel with +10dB cinema calibration.
//*****************************************************************************

#ifndef JAZZ_IMMERSIVE_BASS_MANAGEMENT_CROSSOVER_ENGINE_H
#define JAZZ_IMMERSIVE_BASS_MANAGEMENT_CROSSOVER_ENGINE_H

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

class JZImmersiveBassManagementCrossoverEngine {
public:
    static const size_t NUM_CHANNELS = 16; // 9.1.6 Dolby Atmos layout
    static const size_t LFE_CHANNEL_INDEX = 3;

    struct Parameters {
        float crossoverFreqHz = 80.0f;       // Standard bass management boundary (60.0 to 120.0 Hz)
        float lfeGainBoostDb = 10.0f;        // Cinema LFE standard calibration gain (0.0 to 10.0 dB)
        bool bassManagementEnabled = true;   // Enable satellite bass redirection
        bool subPolarityInvert = false;      // Subwoofer 180-deg phase flip
        bool bypass = false;
    };

    struct LinkwitzRiley4thOrder {
        // Two cascaded 2nd-order Butterworth filters for 4th-order Linkwitz-Riley
        float lpB0, lpB1, lpB2, lpA1, lpA2;
        float hpB0, hpB1, hpB2, hpA1, hpA2;

        float lpX1_1 = 0, lpX2_1 = 0, lpY1_1 = 0, lpY2_1 = 0;
        float lpX1_2 = 0, lpX2_2 = 0, lpY1_2 = 0, lpY2_2 = 0;

        float hpX1_1 = 0, hpX2_1 = 0, hpY1_1 = 0, hpY2_1 = 0;
        float hpX1_2 = 0, hpX2_2 = 0, hpY1_2 = 0, hpY2_2 = 0;

        void reset() {
            lpX1_1 = lpX2_1 = lpY1_1 = lpY2_1 = 0.0f;
            lpX1_2 = lpX2_2 = lpY1_2 = lpY2_2 = 0.0f;
            hpX1_1 = hpX2_1 = hpY1_1 = hpY2_1 = 0.0f;
            hpX1_2 = hpX2_2 = hpY1_2 = hpY2_2 = 0.0f;
        }
    };

    JZImmersiveBassManagementCrossoverEngine(float sampleRate = 48000.0f);
    ~JZImmersiveBassManagementCrossoverEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setCrossoverFreqHz(float hz);
    float getCrossoverFreqHz() const { return m_params.crossoverFreqHz; }

    void setLfeGainBoostDb(float db);
    float getLfeGainBoostDb() const { return m_params.lfeGainBoostDb; }

    void setBassManagementEnabled(bool enabled) { m_params.bassManagementEnabled = enabled; }
    bool isBassManagementEnabled() const { return m_params.bassManagementEnabled; }

    /**
     * @brief Process all 16 discrete Dolby Atmos channels through crossover & bass redirection.
     * @param in16Channels Array of 16 input channel pointers (L, R, C, LFE, etc.)
     * @param out16Channels Array of 16 output channel pointers
     * @param numSamples Number of samples to process
     */
    void process16Channels(const float* const* in16Channels,
                           float* const* out16Channels,
                           size_t numSamples);

    float getSubwooferOutputEnergy() const { return m_subEnergy; }
    std::string getChannelName(size_t index) const;

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    LinkwitzRiley4thOrder m_crossovers[NUM_CHANNELS];
    float m_subEnergy = 0.0f;

    void updateCrossoverCoefficients();
};

} // namespace Jazz

namespace jazz {
    using JZImmersiveBassManagementCrossoverEngine = Jazz::JZImmersiveBassManagementCrossoverEngine;
}

#endif // JAZZ_IMMERSIVE_BASS_MANAGEMENT_CROSSOVER_ENGINE_H
