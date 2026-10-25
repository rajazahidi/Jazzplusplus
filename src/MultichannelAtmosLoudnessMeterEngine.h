//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// MultichannelAtmosLoudnessMeterEngine.h - ITU-R BS.1770-4 Multichannel 7.1.4
// Immersive Audio Loudness & Spatial True-Peak Metering Engine
//
// Implements ITU-R BS.1770-4 / EBU R128 immersive multichannel loudness measurement:
// Stage 1 acoustic head model high-shelf & Stage 2 RLB high-pass K-weighting filters,
// surround/top channel weighting (+1.5 dB), LFE exclusion, gated Integrated LUFS,
// Momentary LUFS (400ms), Short-Term LUFS (3s), Loudness Range (LRA), and 4x
// oversampled Spatial Inter-Sample True-Peak detection across all 12 channels.
//*****************************************************************************

#ifndef JAZZ_MULTICHANNEL_ATMOS_LOUDNESS_METER_ENGINE_H
#define JAZZ_MULTICHANNEL_ATMOS_LOUDNESS_METER_ENGINE_H

#include <vector>
#include <string>
#include <array>
#include <cmath>
#include <cstddef>

namespace Jazz {

enum class AtmosChannelIndex {
    LEFT = 0,
    RIGHT,
    CENTER,
    LFE,
    LEFT_SIDE_SURROUND,
    RIGHT_SIDE_SURROUND,
    LEFT_REAR_SURROUND,
    RIGHT_REAR_SURROUND,
    LEFT_TOP_FRONT,
    RIGHT_TOP_FRONT,
    LEFT_TOP_REAR,
    RIGHT_TOP_REAR,
    CHANNEL_COUNT
};

class JZMultichannelAtmosLoudnessMeterEngine {
public:
    static constexpr size_t NUM_CHANNELS = 12;

    struct LoudnessReadout {
        float momentaryLufs = -70.0f;       // 400ms sliding window
        float shortTermLufs = -70.0f;       // 3000ms sliding window
        float integratedLufs = -70.0f;      // Dual-gated integrated program loudness
        float loudnessRangeLu = 0.0f;       // LRA (Loudness Range in LU)
        float globalMaxTruePeakDbFs = -96.0f;
        std::array<float, NUM_CHANNELS> channelTruePeakDbFs;
    };

    JZMultichannelAtmosLoudnessMeterEngine(float sampleRate = 48000.0f);
    ~JZMultichannelAtmosLoudnessMeterEngine() = default;

    void initialize(float sampleRate);
    void reset();

    // Multichannel Audio Processing
    // Accepts an array of 12 pointers to audio channel buffers
    void process714(const float* const* channelBuffers, size_t numFrames);

    // Readouts & Metering
    const LoudnessReadout& getReadout() const { return m_readout; }
    float getMomentaryLufs() const { return m_readout.momentaryLufs; }
    float getShortTermLufs() const { return m_readout.shortTermLufs; }
    float getIntegratedLufs() const { return m_readout.integratedLufs; }
    float getGlobalMaxTruePeakDbFs() const { return m_readout.globalMaxTruePeakDbFs; }
    float getChannelTruePeakDbFs(AtmosChannelIndex ch) const;

    // Standard channel weighting factor: 1.0 (L/R/C), 0.0 (LFE), 1.41 (+1.5dB Surrounds/Tops)
    static float getChannelWeightingLinear(AtmosChannelIndex ch);
    static const char* getChannelName(AtmosChannelIndex ch);

private:
    float m_sampleRate;
    LoudnessReadout m_readout;

    // Biquad state for K-weighting Stage 1 (High Shelf) and Stage 2 (RLB High Pass)
    struct BiquadCoeffs {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
        float a1 = 0.0f, a2 = 0.0f;
    };
    struct BiquadState {
        float x1 = 0.0f, x2 = 0.0f;
        float y1 = 0.0f, y2 = 0.0f;
    };

    BiquadCoeffs m_stage1Coeffs; // High Shelf
    BiquadCoeffs m_stage2Coeffs; // RLB High Pass

    std::array<BiquadState, NUM_CHANNELS> m_stage1States;
    std::array<BiquadState, NUM_CHANNELS> m_stage2States;

    // Integrated block history for dual gating
    std::vector<double> m_gatedBlockMeanSquares;
    double m_momentaryMeanSquare;
    double m_shortTermMeanSquare;
    size_t m_samplesInCurrentBlock;
    double m_blockAccumulator;

    void calculateKWeightingCoefficients();
    float processBiquad(float inSample, const BiquadCoeffs& c, BiquadState& s);
};

} // namespace Jazz

#endif // JAZZ_MULTICHANNEL_ATMOS_LOUDNESS_METER_ENGINE_H
