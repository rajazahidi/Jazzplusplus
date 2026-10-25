//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// TapePrintThroughDemagSimulator.h - Tape Print-Through & Demagnetization
//
// Physical simulation of historic analog reel-to-reel magnetic tape:
// Inter-layer print-through pre-echo and post-echo (revolution period T = 2*pi*R/v),
// cumulative playback pass high-frequency demagnetization, and elasticity flutter.
//*****************************************************************************

#ifndef JAZZ_TAPE_PRINT_THROUGH_DEMAG_SIMULATOR_H
#define JAZZ_TAPE_PRINT_THROUGH_DEMAG_SIMULATOR_H

#include <vector>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

class JZTapePrintThroughDemagSimulator {
public:
    struct Parameters {
        float reelRadiusCm = 10.0f;           // Current reel wrap radius (5.0 to 18.0 cm)
        float tapeSpeedIps = 15.0f;           // Tape speed (7.5, 15.0, 30.0 ips)
        float printThroughLevelDb = -52.0f;   // Inter-layer print transfer level (-70 to -40 dB)
        float preEchoRatio = 0.65f;           // Pre-echo vs post-echo ratio (inner/outer wrap)
        int playbackPasses = 12;              // Accumulated playback passes (0 to 60)
        float highLossPerPassDb = 0.12f;      // High frequency loss per pass at 15kHz
    };

    JZTapePrintThroughDemagSimulator(float sampleRate = 48000.0f);
    ~JZTapePrintThroughDemagSimulator() = default;

    void initialize(float sampleRate);
    void reset();

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const { return m_params; }

    void setReelRadiusCm(float cm);
    float getReelRadiusCm() const { return m_params.reelRadiusCm; }

    void setPrintThroughLevelDb(float db);
    float getPrintThroughLevelDb() const { return m_params.printThroughLevelDb; }

    void setPlaybackPasses(int passes);
    int getPlaybackPasses() const { return m_params.playbackPasses; }

    /**
     * @brief Process stereo audio buffer through print-through and demag engine.
     */
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 size_t numSamples);

    float getRevolutionPeriodSec() const { return m_revolutionPeriodSec; }
    float getDemagnetizationLossDb() const { return m_demagLossDb; }

private:
    float m_sampleRate = 48000.0f;
    Parameters m_params;

    float m_revolutionPeriodSec = 1.649f;
    float m_demagLossDb = 0.0f;

    // Ring buffer holding at least 2 full reel revolutions (e.g. up to 4.0 seconds)
    std::vector<float> m_tapeReelBufL;
    std::vector<float> m_tapeReelBufR;
    size_t m_writePos = 0;
    size_t m_bufferCapacity = 192000; // 4 seconds at 48kHz

    // High-shelf filter for demagnetization loss
    float m_demagB0 = 1.0f, m_demagB1 = 0.0f, m_demagB2 = 0.0f;
    float m_demagA1 = 0.0f, m_demagA2 = 0.0f;
    float m_demagX1L = 0.0f, m_demagX2L = 0.0f, m_demagY1L = 0.0f, m_demagY2L = 0.0f;
    float m_demagX1R = 0.0f, m_demagX2R = 0.0f, m_demagY1R = 0.0f, m_demagY2R = 0.0f;

    void updateReelTiming();
    void updateDemagFilter();
};

} // namespace Jazz

namespace jazz {
    using JZTapePrintThroughDemagSimulator = Jazz::JZTapePrintThroughDemagSimulator;
}

#endif // JAZZ_TAPE_PRINT_THROUGH_DEMAG_SIMULATOR_H
