//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZVolterraDynamicConvolutionEngine.h - Non-Linear Volterra Kernel Convolver
//
// Dynamic non-linear Volterra series convolution (Acustica Audio/Nebula style):
// 1st-order linear, 2nd-order quadratic (even harmonics / tube warmth), and
// 3rd-order cubic (odd harmonics / transformer core) dynamic impulse modeling
// with input drive-dependent multi-vector dynamic interpolation.
//*****************************************************************************

#ifndef JAZZ_VOLTERRA_DYNAMIC_CONVOLUTION_ENGINE_H
#define JAZZ_VOLTERRA_DYNAMIC_CONVOLUTION_ENGINE_H

#include <vector>
#include <string>
#include <cstdint>
#include <cmath>

namespace Jazz {

enum class VolterraModelType {
    VINTAGE_TUBE_PREAMP,    // Dominant 2nd harmonic (asymmetric triode saturation)
    ANALOG_TAPE_HEAD,       // Dominant 3rd harmonic (symmetric magnetic core saturation)
    CONSOLE_TRANSFORMER,    // Low-end transformer hysteresis and odd/even harmonics
    TRANSPARENT_CUSTOM      // User-defined impulse kernels
};

struct VolterraKernels {
    std::vector<float> h1_linear;    // 1st-order linear kernel
    std::vector<float> h2_quadratic; // 2nd-order even harmonic kernel
    std::vector<float> h3_cubic;     // 3rd-order odd harmonic kernel
};

class JZVolterraDynamicConvolutionEngine {
public:
    JZVolterraDynamicConvolutionEngine(float sampleRate = 48000.0f,
                                      VolterraModelType model = VolterraModelType::VINTAGE_TUBE_PREAMP);
    ~JZVolterraDynamicConvolutionEngine() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return m_sampleRate; }

    void setModelType(VolterraModelType model);
    VolterraModelType getModelType() const { return m_modelType; }

    // Drive & Non-linear Mixing Controls
    void setInputDriveDb(float driveDb);
    float getInputDriveDb() const { return m_inputDriveDb; }

    void setHarmonic2ndOrderMix(float mix); // 0.0 to 1.0 (even harmonics)
    float getHarmonic2ndOrderMix() const { return m_harmonic2ndOrderMix; }

    void setHarmonic3rdOrderMix(float mix); // 0.0 to 1.0 (odd harmonics)
    float getHarmonic3rdOrderMix() const { return m_harmonic3rdOrderMix; }

    void setOutputGainDb(float gainDb);
    float getOutputGainDb() const { return m_outputGainDb; }

    // Custom Kernel Loading
    void setCustomKernels(const std::vector<float>& h1,
                          const std::vector<float>& h2,
                          const std::vector<float>& h3);

    // Audio Processing
    void process(const float* inL, const float* inR,
                 float* outL, float* outR, size_t numSamples);

    // Measure total harmonic distortion (THD in dB) generated on a 1kHz sine test
    float measureHarmonicDistortionThdDb() const;

    void reset();

private:
    float m_sampleRate;
    VolterraModelType m_modelType;
    float m_inputDriveDb;
    float m_inputDriveLinear;
    float m_harmonic2ndOrderMix;
    float m_harmonic3rdOrderMix;
    float m_outputGainDb;
    float m_outputGainLinear;

    VolterraKernels m_kernels;

    // Delay ring buffers for non-linear convolution
    std::vector<float> m_history1L, m_history1R;
    std::vector<float> m_history2L, m_history2R;
    std::vector<float> m_history3L, m_history3R;
    size_t m_writeIdx;

    void initDefaultKernels();
};

} // namespace Jazz

#endif // JAZZ_VOLTERRA_DYNAMIC_CONVOLUTION_ENGINE_H
