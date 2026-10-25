//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZSpatialAmbisonicsUpMixerEngine.h - Stereo-to-7.1.4 / 3D Ambisonics Up-Mixer
//
// Intelligent spatial soundfield up-mixing:
// Inter-channel decorrelation & direct-diffuse decomposition, 7.1.4 Dolby Atmos
// speaker layout routing (front, sides, rears, 4 ceiling height channels),
// phantom center speech anchoring, and 3D Ambisonics B-format (W, X, Y, Z).
//*****************************************************************************

#ifndef JAZZ_SPATIAL_AMBISONICS_UP_MIXER_ENGINE_H
#define JAZZ_SPATIAL_AMBISONICS_UP_MIXER_ENGINE_H

#include <vector>
#include <string>
#include <cstdint>

namespace Jazz {

struct Surround714Channels {
    std::vector<float> L, R;                     // Front Left / Right
    std::vector<float> C, LFE;                   // Center & Subwoofer
    std::vector<float> Ls, Rs;                   // Side Surround Left / Right
    std::vector<float> Lb, Rb;                   // Back Surround Left / Right
    std::vector<float> Tfl, Tfr;                 // Top Front Left / Right
    std::vector<float> Trl, Trr;                 // Top Rear Left / Right

    void resize(size_t n) {
        L.resize(n); R.resize(n); C.resize(n); LFE.resize(n);
        Ls.resize(n); Rs.resize(n); Lb.resize(n); Rb.resize(n);
        Tfl.resize(n); Tfr.resize(n); Trl.resize(n); Trr.resize(n);
    }

    size_t size() const { return L.size(); }
};

struct AmbisonicsBFormat {
    std::vector<float> W; // Omnidirectional pressure
    std::vector<float> X; // Front-Back gradient
    std::vector<float> Y; // Left-Right gradient
    std::vector<float> Z; // Up-Down height gradient

    void resize(size_t n) {
        W.resize(n); X.resize(n); Y.resize(n); Z.resize(n);
    }
};

class JZSpatialAmbisonicsUpMixerEngine {
public:
    JZSpatialAmbisonicsUpMixerEngine(float sampleRate = 48000.0f);
    ~JZSpatialAmbisonicsUpMixerEngine() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return m_sampleRate; }

    // Up-Mix Spatial Controls
    void setCenterExtraction(float amount); // 0.0 (phantom stereo) to 1.0 (hard center extraction)
    float getCenterExtraction() const { return m_centerExtraction; }

    void setSurroundSpill(float amount); // 0.0 (dry) to 1.0 (spacious surround)
    float getSurroundSpill() const { return m_surroundSpill; }

    void setHeightCeilingEnergy(float amount); // 0.0 (flat 2D) to 1.0 (immersive 3D height)
    float getHeightCeilingEnergy() const { return m_heightCeilingEnergy; }

    void setLfeCrossoverHz(float freqHz); // 80 Hz default
    float getLfeCrossoverHz() const { return m_lfeCrossoverHz; }

    // Real-Time Processing
    bool processUpMix714(const float* inL, const float* inR, size_t numSamples,
                         Surround714Channels& out714);

    bool processUpMixBFormat(const float* inL, const float* inR, size_t numSamples,
                            AmbisonicsBFormat& outBFormat);

    void reset();

private:
    float m_sampleRate;
    float m_centerExtraction;
    float m_surroundSpill;
    float m_heightCeilingEnergy;
    float m_lfeCrossoverHz;

    // LFE low-pass filter state
    float m_lfeFilterState;

    // Decorrelation delay lines for ambient surrounds and height ceiling
    std::vector<float> m_delaySideL, m_delaySideR;
    std::vector<float> m_delayHeightL, m_delayHeightR;
    size_t m_sideDelayIdx, m_heightDelayIdx;
};

} // namespace Jazz

#endif // JAZZ_SPATIAL_AMBISONICS_UP_MIXER_ENGINE_H
