//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// AnalogConsoleChannelToleranceEngine.h - Analog Channel Tolerance & Crosstalk
//
// Analog console component tolerance modeling (TMT style) and inter-channel
// stereo crosstalk matrix (Neve 8048 / SSL 9000J). Introduces realistic analog
// component variances (1-3% capacitor/resistor drift across channels 1-64),
// adjacent fader crosstalk bleed, and transformer core saturation.
//*****************************************************************************

#ifndef JAZZ_ANALOG_CONSOLE_CHANNEL_TOLERANCE_ENGINE_H
#define JAZZ_ANALOG_CONSOLE_CHANNEL_TOLERANCE_ENGINE_H

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

enum class AnalogConsoleStyle {
    VINTAGE_BRITISH_80 = 0,    // Neve 8048 class-A discrete with Marinair transformers
    MODERN_PRECISION_9000 = 1   // SSL 9000J SuperAnalogue DC-coupled ultra-wide bandwidth
};

class JZAnalogConsoleChannelToleranceEngine {
public:
    struct Parameters {
        size_t channelIndex = 1;             // Desk channel strip (1 to 64)
        AnalogConsoleStyle consoleStyle = AnalogConsoleStyle::VINTAGE_BRITISH_80;
        float crosstalkAmountDb = -75.0f;    // Adjacent channel stereo bleed (-90 to -55 dB)
        float transformerDrive = 1.0f;       // MuMetal transformer saturation (0.0 to 2.5)
        float toleranceSpread = 1.0f;        // Component drift scaling (0.0 to 2.0x)
        float dryWet = 1.0f;
        bool bypassed = false;
    };

    JZAnalogConsoleChannelToleranceEngine(float sampleRate = 48000.0f);
    ~JZAnalogConsoleChannelToleranceEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setChannelIndex(size_t index);
    size_t getChannelIndex() const { return m_params.channelIndex; }

    void setConsoleStyle(AnalogConsoleStyle style);
    AnalogConsoleStyle getConsoleStyle() const { return m_params.consoleStyle; }

    void setCrosstalkAmountDb(float db);
    float getCrosstalkAmountDb() const { return m_params.crosstalkAmountDb; }

    void setTransformerDrive(float drive);
    float getTransformerDrive() const { return m_params.transformerDrive; }

    void setToleranceSpread(float spread);
    float getToleranceSpread() const { return m_params.toleranceSpread; }

    void setDryWet(float wet);
    float getDryWet() const { return m_params.dryWet; }

    void setBypass(bool bypass);
    bool isBypassed() const { return m_params.bypassed; }

    // Telemetry & Getters
    float getCapacitorTolerancePercent() const { return m_capTolerancePct; }
    float getResistorTolerancePercent() const { return m_resTolerancePct; }
    std::string getConsoleStyleName() const;

    // Audio Processing
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

private:
    void calculateChannelTolerances();

    float m_sampleRate = 48000.0f;
    Parameters m_params;

    float m_capTolerancePct = 0.0f;
    float m_resTolerancePct = 0.0f;
    float m_gainTrimL = 1.0f;
    float m_gainTrimR = 1.0f;

    // 1st-order transformer low-frequency thickening pole
    float m_transStateL = 0.0f;
    float m_transStateR = 0.0f;
};

} // namespace Jazz

namespace jazz {
    using JZAnalogConsoleChannelToleranceEngine = Jazz::JZAnalogConsoleChannelToleranceEngine;
    using AnalogConsoleStyle = Jazz::AnalogConsoleStyle;
}

#endif // JAZZ_ANALOG_CONSOLE_CHANNEL_TOLERANCE_ENGINE_H
