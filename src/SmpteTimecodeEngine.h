#ifndef JAZZ_SMPTE_TIMECODE_ENGINE_H
#define JAZZ_SMPTE_TIMECODE_ENGINE_H

#include <vector>
#include <string>
#include <cstdint>

namespace Jazz {

enum class SmpteFrameRate {
    FPS_24,       // Film
    FPS_25,       // EBU / PAL
    FPS_29_97_DF, // NTSC Drop-Frame
    FPS_29_97_ND, // NTSC Non-Drop
    FPS_30        // Audio / High-speed
};

struct SmpteTime {
    int hours;
    int minutes;
    int seconds;
    int frames;
    int subframes; // 0 to 99 (1/100th frame resolution)

    bool operator==(const SmpteTime& o) const {
        return hours == o.hours && minutes == o.minutes &&
               seconds == o.seconds && frames == o.frames;
    }
};

class JZSmpteTimecodeEngine {
public:
    JZSmpteTimecodeEngine(SmpteFrameRate fps = SmpteFrameRate::FPS_25, float sampleRate = 44100.0f);
    ~JZSmpteTimecodeEngine() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return m_sampleRate; }

    void setFrameRate(SmpteFrameRate rate);
    SmpteFrameRate getFrameRate() const { return m_frameRate; }
    float getFrameRateValue() const;

    void setTime(int hours, int minutes, int seconds, int frames, int subframes = 0);
    SmpteTime getTime() const { return m_currentTime; }
    std::string getFormattedTimeString() const;

    // Advance time by one audio sample
    void advanceSample();

    // Generate Biphase Mark encoded LTC Audio Signal [-1.0, 1.0]
    float generateLtcAudioSample();

    // Decode incoming LTC Audio Signal
    // Returns true when a new valid 80-bit SMPTE frame is successfully synchronized
    bool decodeLtcAudioSample(float sample);

    // MTC (MIDI Timecode) generation
    bool getNextMtcQuarterFrame(uint8_t& outDataByte);
    int getMtcFullFrameSysEx(uint8_t outSysEx[10]) const;

    // Status
    bool isLtcLocked() const { return m_ltcLocked; }
    void reset();

private:
    void encode80BitFrame(uint8_t bits[80]) const;
    void incrementFrame();

    float m_sampleRate;
    SmpteFrameRate m_frameRate;
    SmpteTime m_currentTime;

    // Generator state
    uint8_t m_currentFrameBits[80];
    int m_bitIndex;
    float m_samplePhaseInBit;
    float m_samplesPerBit;
    float m_currentLtcLevel;

    // Decoder state
    float m_prevSample;
    int m_samplesSinceEdge;
    bool m_ltcLocked;
    uint8_t m_rxBits[160];
    int m_rxBitIdx;
    SmpteTime m_decodedTime;

    // MTC quarter frame sequencer
    int m_mtcQuarterIndex;
    int m_samplesSinceQuarter;
};

} // namespace Jazz

#endif // JAZZ_SMPTE_TIMECODE_ENGINE_H
