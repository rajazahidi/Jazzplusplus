//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// AdaptiveRoomDereverberationEngine.h - Adaptive Spectral Dereverberation
//
// Suppresses room acoustic reflections and late diffuse tails (Zynaptiq UNVEIL /
// Cedar style): Statistical Sabine/Eyring decay modeling, multi-band spectral
// energy tracking, transient preservation, and up to -20 dB reflection attenuation.
//*****************************************************************************

#ifndef JAZZ_ADAPTIVE_ROOM_DEREVERBERATION_ENGINE_H
#define JAZZ_ADAPTIVE_ROOM_DEREVERBERATION_ENGINE_H

#include <vector>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

class JZAdaptiveRoomDereverberationEngine {
public:
    struct Parameters {
        float dereverbAmount = 0.7f;      // Suppression intensity (0.0 to 1.0)
        float roomRt60Sec = 1.2f;         // Estimated room reverberation decay (0.2 to 4.0 s)
        float transientPreserve = 0.85f;  // Consonant/transient protection factor
        float maxSuppressionDb = -18.0f;  // Floor attenuation limit (-6.0 to -30.0 dB)
        float directBoostDb = 1.5f;       // Vocal direct sound clarification boost
    };

    JZAdaptiveRoomDereverberationEngine(float sampleRate = 48000.0f);
    ~JZAdaptiveRoomDereverberationEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setDereverbAmount(float amt);
    float getDereverbAmount() const { return m_params.dereverbAmount; }

    void setRoomRt60Sec(float rt60);
    float getRoomRt60Sec() const { return m_params.roomRt60Sec; }

    /**
     * @brief Process stereo audio buffer through adaptive dereverberation engine.
     */
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

    float getAverageSuppressionDb() const { return m_avgSuppressionDb; }
    float getReverbTailEnergy() const { return m_reverbTailEnergy; }

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    // Multi-band filterbank (8 analysis bands spanning 100 Hz to 8000 Hz)
    static const size_t NUM_BANDS = 8;
    struct BandState {
        float centerFreqHz;
        float b0, b1, b2, a1, a2;
        float x1L, x2L, y1L, y2L;
        float x1R, x2R, y1R, y2R;

        // Energy tracking & late reverberation decay state
        float fastEnergyL = 0.0f;
        float fastEnergyR = 0.0f;
        float decayModelL = 0.0f;
        float decayModelR = 0.0f;
    };

    BandState m_bands[NUM_BANDS];

    float m_decayCoeff = 0.999f;
    float m_avgSuppressionDb = 0.0f;
    float m_reverbTailEnergy = 0.0f;

    void updateFilterbank();
};

} // namespace Jazz

namespace jazz {
    using JZAdaptiveRoomDereverberationEngine = Jazz::JZAdaptiveRoomDereverberationEngine;
}

#endif // JAZZ_ADAPTIVE_ROOM_DEREVERBERATION_ENGINE_H
