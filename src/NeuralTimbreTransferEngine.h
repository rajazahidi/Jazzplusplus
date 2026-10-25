//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZNeuralTimbreTransferEngine.h - Real-Time Neural Timbre Transfer (DDSP)
//
// Real-time neural instrument resynthesis and timbre transfer (RAVE/DDSP style):
// Zero-latency causal pitch tracking, differentiable harmonic oscillator bank,
// stochastic noise modeling, and instrument timbre transfer (Violin, Sax, Flute, Cello).
//*****************************************************************************

#ifndef JAZZ_NEURAL_TIMBRE_TRANSFER_ENGINE_H
#define JAZZ_NEURAL_TIMBRE_TRANSFER_ENGINE_H

#include <vector>
#include <string>
#include <cstdint>
#include <cmath>

namespace Jazz {

enum class NeuralTimbreModel {
    STRADIVARIUS_VIOLIN, // Expressive bowing, rich odd/even harmonic body
    TENOR_SAXOPHONE,     // Reed breath turbulence, warm woody horn body
    CONCERT_FLUTE,       // Air jet turbulence, pure fundamental tone
    WARM_CELLO           // Deep resonant wood body, sustained rich bowing
};

struct NeuralTimbreState {
    float estimatedPitchHz;
    float inputLoudnessDb;
    float harmonicEnergy;
    float noiseEnergy;
    bool pitchLocked;
};

class JZNeuralTimbreTransferEngine {
public:
    JZNeuralTimbreTransferEngine(float sampleRate = 48000.0f,
                                 NeuralTimbreModel model = NeuralTimbreModel::STRADIVARIUS_VIOLIN);
    ~JZNeuralTimbreTransferEngine() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return m_sampleRate; }

    void setModel(NeuralTimbreModel model);
    NeuralTimbreModel getModel() const { return m_model; }

    // Resynthesis Controls
    void setBlend(float blend); // 0.0 (dry audio) to 1.0 (100% resynthesized timbre)
    float getBlend() const { return m_blend; }

    void setNoiseAmount(float amount); // Aspiration / bow friction (0.0 to 1.0)
    float getNoiseAmount() const { return m_noiseAmount; }

    void setPitchCorrection(float amount); // 0.0 (natural pitch) to 1.0 (quantized auto-pitch)
    float getPitchCorrection() const { return m_pitchCorrection; }

    // Real-Time Processing
    void process(const float* inL, const float* inR,
                 float* outL, float* outR, size_t numSamples);

    NeuralTimbreState getState() const { return m_state; }

    void reset();

private:
    float m_sampleRate;
    NeuralTimbreModel m_model;
    float m_blend;
    float m_noiseAmount;
    float m_pitchCorrection;

    NeuralTimbreState m_state;

    // Harmonic oscillator bank (16 harmonics)
    static const size_t NUM_HARMONICS = 16;
    float m_harmonicPhases[NUM_HARMONICS];
    float m_harmonicWeights[NUM_HARMONICS];

    // Pitch & envelope detection history
    float m_prevSample;
    float m_rmsEnvelope;
    float m_detectedPitchHz;

    uint32_t m_noiseSeed;
    float getNoise();

    void updateModelWeights();
    float estimatePitch(const float* buffer, size_t numSamples);
};

} // namespace Jazz

#endif // JAZZ_NEURAL_TIMBRE_TRANSFER_ENGINE_H
