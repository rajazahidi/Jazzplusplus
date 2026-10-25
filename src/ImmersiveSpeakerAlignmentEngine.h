//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// ImmersiveSpeakerAlignmentEngine.h - 9.1.6 Dolby Atmos Speaker Distance Aligner
//
// 16-Channel Immersive Speaker Room Alignment (Trinnov / Dolby DARDT style):
// Sub-sample acoustic time-of-flight delay compensation, SPL calibration trim,
// and polar phase alignment across 9.1.6 Dolby Atmos / Sony 360RA monitor setups.
//*****************************************************************************

#ifndef JAZZ_IMMERSIVE_SPEAKER_ALIGNMENT_ENGINE_H
#define JAZZ_IMMERSIVE_SPEAKER_ALIGNMENT_ENGINE_H

#include <vector>
#include <string>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Jazz {

class JZImmersiveSpeakerAlignmentEngine {
public:
    static const size_t NUM_CHANNELS = 16; // 9.1.6 Dolby Atmos Layout

    struct SpeakerChannelConfig {
        std::string name;           // e.g. "L", "R", "C", "LFE", "Lss", "Rss", "Ltf", etc.
        float distanceMeters = 2.0f;// Distance to sweet spot (1.0 to 12.0 m)
        float gainTrimDb = 0.0f;    // SPL level trim (-12.0 to +12.0 dB)
        bool invertPhase = false;   // 180-deg polarity inversion
        bool isMuted = false;
    };

    JZImmersiveSpeakerAlignmentEngine(float sampleRate = 48000.0f);
    ~JZImmersiveSpeakerAlignmentEngine() = default;

    void initialize(float sampleRate);
    void reset();

    void setSpeakerDistanceMeters(size_t channelIndex, float meters);
    float getSpeakerDistanceMeters(size_t channelIndex) const;

    void setSpeakerGainTrimDb(size_t channelIndex, float trimDb);
    float getSpeakerGainTrimDb(size_t channelIndex) const;

    void setSpeakerPolarity(size_t channelIndex, bool invertPhase);

    /**
     * @brief Process all 16 discrete channels through delay alignment and gain trim.
     */
    void process16Channels(const float* const* inputs,
                           float* const* outputs,
                           size_t numSamples);

    /**
     * @brief Generate logarithmic acoustic calibration chirp for room measurement.
     */
    void generateMeasurementChirp(float* chirpBuffer, size_t numSamples, float startHz = 20.0f, float endHz = 20000.0f);

    float getChannelDelayMs(size_t channelIndex) const;
    size_t getChannelDelaySamples(size_t channelIndex) const;
    std::string getChannelName(size_t channelIndex) const;

private:
    float m_sampleRate = 48000.0f;
    SpeakerChannelConfig m_speakers[NUM_CHANNELS];

    // Delay lines per channel (up to 40ms delay = ~1920 samples)
    static const size_t MAX_DELAY_SAMPLES = 2048;
    std::vector<float> m_delayLines[NUM_CHANNELS];
    size_t m_writePos[NUM_CHANNELS];
    size_t m_delaySamples[NUM_CHANNELS];
    float m_linearGain[NUM_CHANNELS];

    void updateAlignmentDelays();
};

} // namespace Jazz

namespace jazz {
    using JZImmersiveSpeakerAlignmentEngine = Jazz::JZImmersiveSpeakerAlignmentEngine;
}

#endif // JAZZ_IMMERSIVE_SPEAKER_ALIGNMENT_ENGINE_H
