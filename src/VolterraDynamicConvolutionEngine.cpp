//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZVolterraDynamicConvolutionEngine.cpp - Non-Linear Volterra Kernel Convolver
//*****************************************************************************

#include "VolterraDynamicConvolutionEngine.h"
#include <algorithm>
#include <cmath>

namespace Jazz {

JZVolterraDynamicConvolutionEngine::JZVolterraDynamicConvolutionEngine(float sampleRate, VolterraModelType model)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
    , m_modelType(model)
    , m_inputDriveDb(0.0f)
    , m_inputDriveLinear(1.0f)
    , m_harmonic2ndOrderMix(0.35f)
    , m_harmonic3rdOrderMix(0.20f)
    , m_outputGainDb(0.0f)
    , m_outputGainLinear(1.0f)
    , m_writeIdx(0)
{
    initDefaultKernels();
    reset();
}

void JZVolterraDynamicConvolutionEngine::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        m_sampleRate = sampleRate;
        initDefaultKernels();
        reset();
    }
}

void JZVolterraDynamicConvolutionEngine::setModelType(VolterraModelType model) {
    m_modelType = model;
    initDefaultKernels();
}

void JZVolterraDynamicConvolutionEngine::setInputDriveDb(float driveDb) {
    m_inputDriveDb = std::clamp(driveDb, -24.0f, 24.0f);
    m_inputDriveLinear = std::pow(10.0f, m_inputDriveDb / 20.0f);
}

void JZVolterraDynamicConvolutionEngine::setHarmonic2ndOrderMix(float mix) {
    m_harmonic2ndOrderMix = std::clamp(mix, 0.0f, 1.0f);
}

void JZVolterraDynamicConvolutionEngine::setHarmonic3rdOrderMix(float mix) {
    m_harmonic3rdOrderMix = std::clamp(mix, 0.0f, 1.0f);
}

void JZVolterraDynamicConvolutionEngine::setOutputGainDb(float gainDb) {
    m_outputGainDb = std::clamp(gainDb, -24.0f, 24.0f);
    m_outputGainLinear = std::pow(10.0f, m_outputGainDb / 20.0f);
}

void JZVolterraDynamicConvolutionEngine::initDefaultKernels() {
    const size_t KERNEL_SIZE = 32;
    m_kernels.h1_linear.assign(KERNEL_SIZE, 0.0f);
    m_kernels.h2_quadratic.assign(KERNEL_SIZE, 0.0f);
    m_kernels.h3_cubic.assign(KERNEL_SIZE, 0.0f);

    // Exponential decay FIR impulse envelopes
    for (size_t i = 0; i < KERNEL_SIZE; ++i) {
        float decay = std::exp(-static_cast<float>(i) * 0.25f);

        switch (m_modelType) {
            case VolterraModelType::VINTAGE_TUBE_PREAMP:
                m_kernels.h1_linear[i] = (i == 0) ? 0.95f : decay * 0.04f;
                m_kernels.h2_quadratic[i] = (i == 0) ? 0.25f : decay * 0.03f; // Dominant even harmonics
                m_kernels.h3_cubic[i] = (i == 0) ? 0.08f : decay * 0.01f;
                break;

            case VolterraModelType::ANALOG_TAPE_HEAD:
                m_kernels.h1_linear[i] = (i == 0) ? 0.90f : decay * 0.05f;
                m_kernels.h2_quadratic[i] = (i == 0) ? 0.05f : decay * 0.01f;
                m_kernels.h3_cubic[i] = (i == 0) ? 0.30f : decay * 0.05f;     // Dominant odd harmonics
                break;

            case VolterraModelType::CONSOLE_TRANSFORMER:
                m_kernels.h1_linear[i] = (i == 0) ? 0.92f : decay * 0.03f;
                m_kernels.h2_quadratic[i] = (i == 0) ? 0.15f : decay * 0.02f;
                m_kernels.h3_cubic[i] = (i == 0) ? 0.20f : decay * 0.03f;
                break;

            case VolterraModelType::TRANSPARENT_CUSTOM:
                m_kernels.h1_linear[i] = (i == 0) ? 1.0f : 0.0f;
                m_kernels.h2_quadratic[i] = 0.0f;
                m_kernels.h3_cubic[i] = 0.0f;
                break;
        }
    }
}

void JZVolterraDynamicConvolutionEngine::setCustomKernels(const std::vector<float>& h1,
                                                        const std::vector<float>& h2,
                                                        const std::vector<float>& h3) {
    m_modelType = VolterraModelType::TRANSPARENT_CUSTOM;
    m_kernels.h1_linear = h1;
    m_kernels.h2_quadratic = h2;
    m_kernels.h3_cubic = h3;
    reset();
}

void JZVolterraDynamicConvolutionEngine::reset() {
    size_t sz = std::max<size_t>(64, m_kernels.h1_linear.size());
    m_history1L.assign(sz, 0.0f);
    m_history1R.assign(sz, 0.0f);
    m_history2L.assign(sz, 0.0f);
    m_history2R.assign(sz, 0.0f);
    m_history3L.assign(sz, 0.0f);
    m_history3R.assign(sz, 0.0f);
    m_writeIdx = 0;
}

void JZVolterraDynamicConvolutionEngine::process(const float* inL, const float* inR,
                                                float* outL, float* outR, size_t numSamples) {
    if (!inL || !inR || !outL || !outR || numSamples == 0) return;

    size_t kSize = m_kernels.h1_linear.size();
    size_t bufSize = m_history1L.size();

    for (size_t s = 0; s < numSamples; ++s) {
        float xL = inL[s] * m_inputDriveLinear;
        float xR = inR[s] * m_inputDriveLinear;

        // Quadratic (even) and Cubic (odd) non-linear terms
        float x2L = xL * xL;
        float x2R = xR * xR;
        float x3L = xL * xL * xL;
        float x3R = xR * xR * xR;

        // Store into ring history
        m_history1L[m_writeIdx] = xL;
        m_history1R[m_writeIdx] = xR;
        m_history2L[m_writeIdx] = x2L;
        m_history2R[m_writeIdx] = x2R;
        m_history3L[m_writeIdx] = x3L;
        m_history3R[m_writeIdx] = x3R;

        // Volterra expansion convolution
        float y1L = 0.0f, y1R = 0.0f;
        float y2L = 0.0f, y2R = 0.0f;
        float y3L = 0.0f, y3R = 0.0f;

        for (size_t k = 0; k < kSize; ++k) {
            size_t rIdx = (m_writeIdx + bufSize - k) % bufSize;
            y1L += m_kernels.h1_linear[k] * m_history1L[rIdx];
            y1R += m_kernels.h1_linear[k] * m_history1R[rIdx];

            y2L += m_kernels.h2_quadratic[k] * m_history2L[rIdx];
            y2R += m_kernels.h2_quadratic[k] * m_history2R[rIdx];

            y3L += m_kernels.h3_cubic[k] * m_history3L[rIdx];
            y3R += m_kernels.h3_cubic[k] * m_history3R[rIdx];
        }

        // Sum Volterra orders with user harmonic controls
        float totalL = y1L + (y2L * m_harmonic2ndOrderMix) + (y3L * m_harmonic3rdOrderMix);
        float totalR = y1R + (y2R * m_harmonic2ndOrderMix) + (y3R * m_harmonic3rdOrderMix);

        // Soft-saturation ceiling limiter
        totalL = std::tanh(totalL) * m_outputGainLinear;
        totalR = std::tanh(totalR) * m_outputGainLinear;

        outL[s] = totalL;
        outR[s] = totalR;

        m_writeIdx = (m_writeIdx + 1) % bufSize;
    }
}

float JZVolterraDynamicConvolutionEngine::measureHarmonicDistortionThdDb() const {
    float fundamental = m_inputDriveLinear * 0.95f;
    float evenHarmonic = (m_inputDriveLinear * m_inputDriveLinear) * 0.25f * m_harmonic2ndOrderMix;
    float oddHarmonic = (m_inputDriveLinear * m_inputDriveLinear * m_inputDriveLinear) * 0.08f * m_harmonic3rdOrderMix;

    float harmonicPower = evenHarmonic * evenHarmonic + oddHarmonic * oddHarmonic;
    float fundPower = fundamental * fundamental;

    if (fundPower <= 1e-9f || harmonicPower <= 1e-9f) return -90.0f;
    float thdRatio = std::sqrt(harmonicPower / fundPower);
    float thdDb = 20.0f * std::log10(thdRatio);
    return std::clamp(thdDb, -90.0f, 0.0f);
}

} // namespace Jazz
