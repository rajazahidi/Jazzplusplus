#ifndef JAZZ_NEURAL_AUDIO_ENGINE_H
#define JAZZ_NEURAL_AUDIO_ENGINE_H

#include <vector>
#include <string>
#include <memory>
#include <cmath>

namespace Jazz {

enum class NeuralModelType {
    DEEP_STEM_SEPARATOR, // 4-Stem Deep Learning Unmixing (Vocals, Drums, Bass, Other)
    DDSP_SYNTHESIZER,    // Differentiable DSP Harmonic + Noise Resynthesis
    NEURAL_DENOISER      // Recurrent Spectral Subtraction Noise Suppression
};

struct NeuralTensorShape {
    int batch;
    int channels;
    int height;
    int width;

    int totalElements() const {
        return batch * channels * height * width;
    }
};

class JZNeuralAudioEngine {
public:
    JZNeuralAudioEngine(NeuralModelType modelType = NeuralModelType::DEEP_STEM_SEPARATOR);
    ~JZNeuralAudioEngine() = default;

    // Model Management
    bool loadModel(const std::string& modelName, NeuralModelType type);
    bool isModelLoaded() const { return m_isModelLoaded; }
    NeuralModelType getModelType() const { return m_modelType; }
    const std::string& getModelName() const { return m_modelName; }

    // Activation Functions (Neural DSP Building Blocks)
    static float computeGelu(float x);
    static float computeRelu(float x);
    static float computeSigmoid(float x);
    static float computeTanh(float x);

    // Audio Ingestion & STFT Spectrogram Framing
    void setStftParameters(int fftSize = 1024, int hopSize = 256);
    int getFftSize() const { return m_fftSize; }
    int getHopSize() const { return m_hopSize; }

    // Generates Short-Time Fourier Transform magnitude frames from input buffer
    void extractSpectrogram(const float* inputAudio, int numSamples,
                            std::vector<std::vector<float>>& outSpectrogram);

    // Neural 4-Stem Deep Inference Simulation
    // Input: Stereo mixdown buffer
    // Output: 4 separate stems (Vocals, Drums, Bass, Other) with exact energy conservation
    bool separateStems(const float* mixL, const float* mixR, int numSamples,
                       std::vector<float>& outVocalL, std::vector<float>& outVocalR,
                       std::vector<float>& outDrumL,  std::vector<float>& outDrumR,
                       std::vector<float>& outBassL,  std::vector<float>& outBassR,
                       std::vector<float>& outOtherL, std::vector<float>& outOtherR);

    // DDSP (Differentiable DSP) Harmonic Resynthesis
    // Generates pitched harmonic tones + shaped filtered noise based on pitch f0 and harmonic amplitudes
    void synthesizeDdsp(float fundamentalFreqHz, float loudnessNormalized,
                        const std::vector<float>& harmonicWeights,
                        float* outBuffer, int numSamples, float sampleRate = 44100.0f);

private:
    NeuralModelType m_modelType;
    std::string m_modelName;
    bool m_isModelLoaded;

    int m_fftSize;
    int m_hopSize;
};

} // namespace Jazz

#endif // JAZZ_NEURAL_AUDIO_ENGINE_H
