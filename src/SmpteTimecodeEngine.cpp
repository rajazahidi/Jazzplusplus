#include "SmpteTimecodeEngine.h"
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <cmath>

namespace Jazz {

JZSmpteTimecodeEngine::JZSmpteTimecodeEngine(SmpteFrameRate fps, float sampleRate)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f)
    , m_frameRate(fps)
    , m_bitIndex(0)
    , m_samplePhaseInBit(0.0f)
    , m_currentLtcLevel(1.0f)
    , m_prevSample(0.0f)
    , m_samplesSinceEdge(0)
    , m_ltcLocked(false)
    , m_rxBitIdx(0)
    , m_mtcQuarterIndex(0)
    , m_samplesSinceQuarter(0)
{
    m_currentTime = {1, 0, 0, 0, 0};
    m_decodedTime = {0, 0, 0, 0, 0};

    setFrameRate(fps);
    reset();
}

void JZSmpteTimecodeEngine::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        m_sampleRate = sampleRate;
        setFrameRate(m_frameRate);
    }
}

float JZSmpteTimecodeEngine::getFrameRateValue() const {
    switch (m_frameRate) {
        case SmpteFrameRate::FPS_24:       return 24.0f;
        case SmpteFrameRate::FPS_25:       return 25.0f;
        case SmpteFrameRate::FPS_29_97_DF: return 29.97f;
        case SmpteFrameRate::FPS_29_97_ND: return 29.97f;
        case SmpteFrameRate::FPS_30:       return 30.0f;
    }
    return 25.0f;
}

void JZSmpteTimecodeEngine::setFrameRate(SmpteFrameRate rate) {
    m_frameRate = rate;
    float fps = getFrameRateValue();
    m_samplesPerBit = m_sampleRate / (fps * 80.0f);
    encode80BitFrame(m_currentFrameBits);
}

void JZSmpteTimecodeEngine::setTime(int hours, int minutes, int seconds, int frames, int subframes) {
    m_currentTime.hours = std::max(0, std::min(23, hours));
    m_currentTime.minutes = std::max(0, std::min(59, minutes));
    m_currentTime.seconds = std::max(0, std::min(59, seconds));
    int maxF = static_cast<int>(std::ceil(getFrameRateValue())) - 1;
    m_currentTime.frames = std::max(0, std::min(maxF, frames));
    m_currentTime.subframes = std::max(0, std::min(99, subframes));

    encode80BitFrame(m_currentFrameBits);
}

std::string JZSmpteTimecodeEngine::getFormattedTimeString() const {
    std::ostringstream ss;
    ss << std::setfill('0')
       << std::setw(2) << m_currentTime.hours << ":"
       << std::setw(2) << m_currentTime.minutes << ":"
       << std::setw(2) << m_currentTime.seconds << ":"
       << std::setw(2) << m_currentTime.frames;
    return ss.str();
}

void JZSmpteTimecodeEngine::encode80BitFrame(uint8_t bits[80]) const {
    for (int i = 0; i < 80; ++i) bits[i] = 0;

    int fr = m_currentTime.frames;
    int sc = m_currentTime.seconds;
    int mn = m_currentTime.minutes;
    int hr = m_currentTime.hours;

    // Frames units (bits 0..3) & tens (bits 8..9)
    int frU = fr % 10;
    int frT = fr / 10;
    for (int b = 0; b < 4; ++b) bits[b] = (frU >> b) & 1;
    for (int b = 0; b < 2; ++b) bits[8 + b] = (frT >> b) & 1;

    // Drop Frame Flag (bit 10)
    bits[10] = (m_frameRate == SmpteFrameRate::FPS_29_97_DF) ? 1 : 0;

    // Seconds units (bits 16..19) & tens (bits 24..26)
    int scU = sc % 10;
    int scT = sc / 10;
    for (int b = 0; b < 4; ++b) bits[16 + b] = (scU >> b) & 1;
    for (int b = 0; b < 3; ++b) bits[24 + b] = (scT >> b) & 1;

    // Minutes units (bits 32..35) & tens (bits 40..42)
    int mnU = mn % 10;
    int mnT = mn / 10;
    for (int b = 0; b < 4; ++b) bits[32 + b] = (mnU >> b) & 1;
    for (int b = 0; b < 3; ++b) bits[40 + b] = (mnT >> b) & 1;

    // Hours units (bits 48..51) & tens (bits 56..57)
    int hrU = hr % 10;
    int hrT = hr / 10;
    for (int b = 0; b < 4; ++b) bits[48 + b] = (hrU >> b) & 1;
    for (int b = 0; b < 2; ++b) bits[56 + b] = (hrT >> b) & 1;

    // Sync Word: 0011 1111 1111 1101 (bits 64..79)
    static const uint8_t sync[16] = {0,0,1,1, 1,1,1,1, 1,1,1,1, 1,1,0,1};
    for (int i = 0; i < 16; ++i) {
        bits[64 + i] = sync[i];
    }
}

void JZSmpteTimecodeEngine::incrementFrame() {
    int maxF = static_cast<int>(std::ceil(getFrameRateValue()));
    m_currentTime.frames++;
    if (m_currentTime.frames >= maxF) {
        m_currentTime.frames = 0;
        m_currentTime.seconds++;
        if (m_currentTime.seconds >= 60) {
            m_currentTime.seconds = 0;
            m_currentTime.minutes++;
            if (m_currentTime.minutes >= 60) {
                m_currentTime.minutes = 0;
                m_currentTime.hours = (m_currentTime.hours + 1) % 24;
            }
        }
    }
    encode80BitFrame(m_currentFrameBits);
}

void JZSmpteTimecodeEngine::advanceSample() {
    m_samplePhaseInBit += 1.0f;
    if (m_samplePhaseInBit >= m_samplesPerBit) {
        m_samplePhaseInBit -= m_samplesPerBit;
        m_bitIndex++;
        if (m_bitIndex >= 80) {
            m_bitIndex = 0;
            incrementFrame();
        }
    }
}

float JZSmpteTimecodeEngine::generateLtcAudioSample() {
    uint8_t bit = m_currentFrameBits[m_bitIndex];
    float halfBit = m_samplesPerBit * 0.5f;

    // Bi-phase Mark (Manchester-like / FM0):
    // Transition at start of bit. If bit == 1, also transition at mid-bit.
    if (bit == 1) {
        if (m_samplePhaseInBit < halfBit) {
            m_currentLtcLevel = 1.0f;
        } else {
            m_currentLtcLevel = -1.0f;
        }
    } else {
        m_currentLtcLevel = 1.0f;
    }

    advanceSample();
    return m_currentLtcLevel * 0.85f;
}

bool JZSmpteTimecodeEngine::decodeLtcAudioSample(float sample) {
    m_samplesSinceEdge++;

    // Zero-crossing detection
    bool edgeDetected = (m_prevSample <= 0.0f && sample > 0.0f) ||
                        (m_prevSample >= 0.0f && sample < 0.0f);
    m_prevSample = sample;

    if (!edgeDetected) {
        return false;
    }

    float halfBitNom = m_samplesPerBit * 0.5f;
    float fullBitNom = m_samplesPerBit;

    bool isShort = std::abs(m_samplesSinceEdge - halfBitNom) < (halfBitNom * 0.45f);
    bool isLong  = std::abs(m_samplesSinceEdge - fullBitNom) < (fullBitNom * 0.45f);

    m_samplesSinceEdge = 0;

    static bool haveHalfBit = false;

    if (isShort) {
        if (haveHalfBit) {
            // Two short intervals = Bit '1'
            m_rxBits[m_rxBitIdx % 160] = 1;
            m_rxBitIdx++;
            haveHalfBit = false;
        } else {
            haveHalfBit = true;
            return false;
        }
    } else if (isLong) {
        // One long interval = Bit '0'
        m_rxBits[m_rxBitIdx % 160] = 0;
        m_rxBitIdx++;
        haveHalfBit = false;
    } else {
        // Jitter or noise
        haveHalfBit = false;
        return false;
    }

    // Check last 16 bits for Sync word: 0011 1111 1111 1101
    if (m_rxBitIdx >= 80) {
        static const uint8_t syncPattern[16] = {0,0,1,1, 1,1,1,1, 1,1,1,1, 1,1,0,1};
        bool syncFound = true;
        for (int i = 0; i < 16; ++i) {
            int readIdx = (m_rxBitIdx - 16 + i) % 160;
            if (m_rxBits[readIdx] != syncPattern[i]) {
                syncFound = false;
                break;
            }
        }

        if (syncFound) {
            m_ltcLocked = true;
            int startIdx = (m_rxBitIdx - 80 + 160) % 160;

            // Extract Frames
            int frU = 0, frT = 0;
            for (int b = 0; b < 4; ++b) frU |= (m_rxBits[(startIdx + b) % 160] << b);
            for (int b = 0; b < 2; ++b) frT |= (m_rxBits[(startIdx + 8 + b) % 160] << b);
            m_decodedTime.frames = frT * 10 + frU;

            // Extract Seconds
            int scU = 0, scT = 0;
            for (int b = 0; b < 4; ++b) scU |= (m_rxBits[(startIdx + 16 + b) % 160] << b);
            for (int b = 0; b < 3; ++b) scT |= (m_rxBits[(startIdx + 24 + b) % 160] << b);
            m_decodedTime.seconds = scT * 10 + scU;

            // Extract Minutes
            int mnU = 0, mnT = 0;
            for (int b = 0; b < 4; ++b) mnU |= (m_rxBits[(startIdx + 32 + b) % 160] << b);
            for (int b = 0; b < 3; ++b) mnT |= (m_rxBits[(startIdx + 40 + b) % 160] << b);
            m_decodedTime.minutes = mnT * 10 + mnU;

            // Extract Hours
            int hrU = 0, hrT = 0;
            for (int b = 0; b < 4; ++b) hrU |= (m_rxBits[(startIdx + 48 + b) % 160] << b);
            for (int b = 0; b < 2; ++b) hrT |= (m_rxBits[(startIdx + 56 + b) % 160] << b);
            m_decodedTime.hours = hrT * 10 + hrU;

            return true;
        }
    }

    return false;
}

bool JZSmpteTimecodeEngine::getNextMtcQuarterFrame(uint8_t& outDataByte) {
    // 8 quarter frames per 2 full frames -> every (m_sampleRate / (fps * 4)) samples
    float quarterInterval = m_sampleRate / (getFrameRateValue() * 4.0f);
    m_samplesSinceQuarter++;

    if (m_samplesSinceQuarter >= static_cast<int>(quarterInterval)) {
        m_samplesSinceQuarter = 0;

        int piece = m_mtcQuarterIndex;
        int nibble = 0;

        switch (piece) {
            case 0: nibble = m_currentTime.frames & 0x0F; break;
            case 1: nibble = (m_currentTime.frames >> 4) & 0x01; break;
            case 2: nibble = m_currentTime.seconds & 0x0F; break;
            case 3: nibble = (m_currentTime.seconds >> 4) & 0x07; break;
            case 4: nibble = m_currentTime.minutes & 0x0F; break;
            case 5: nibble = (m_currentTime.minutes >> 4) & 0x07; break;
            case 6: nibble = m_currentTime.hours & 0x0F; break;
            case 7: {
                int rateCode = 1; // 25 fps
                if (m_frameRate == SmpteFrameRate::FPS_24) rateCode = 0;
                else if (m_frameRate == SmpteFrameRate::FPS_29_97_DF) rateCode = 2;
                else if (m_frameRate == SmpteFrameRate::FPS_30) rateCode = 3;
                nibble = ((m_currentTime.hours >> 4) & 0x01) | (rateCode << 1);
                break;
            }
        }

        outDataByte = static_cast<uint8_t>((piece << 4) | (nibble & 0x0F));
        m_mtcQuarterIndex = (m_mtcQuarterIndex + 1) % 8;
        return true;
    }

    return false;
}

int JZSmpteTimecodeEngine::getMtcFullFrameSysEx(uint8_t outSysEx[10]) const {
    outSysEx[0] = 0xF0; // SysEx start
    outSysEx[1] = 0x7F; // Universal Realtime
    outSysEx[2] = 0x7F; // Disregard channel
    outSysEx[3] = 0x01; // Sub-ID 1 (MTC)
    outSysEx[4] = 0x01; // Sub-ID 2 (Full Frame)

    int rateCode = 1; // 25 fps default
    if (m_frameRate == SmpteFrameRate::FPS_24) rateCode = 0;
    else if (m_frameRate == SmpteFrameRate::FPS_29_97_DF) rateCode = 2;
    else if (m_frameRate == SmpteFrameRate::FPS_30) rateCode = 3;

    outSysEx[5] = static_cast<uint8_t>((rateCode << 5) | (m_currentTime.hours & 0x1F));
    outSysEx[6] = static_cast<uint8_t>(m_currentTime.minutes & 0x3F);
    outSysEx[7] = static_cast<uint8_t>(m_currentTime.seconds & 0x3F);
    outSysEx[8] = static_cast<uint8_t>(m_currentTime.frames & 0x1F);
    outSysEx[9] = 0xF7; // SysEx end
    return 10;
}

void JZSmpteTimecodeEngine::reset() {
    m_bitIndex = 0;
    m_samplePhaseInBit = 0.0f;
    m_currentLtcLevel = 1.0f;
    m_prevSample = 0.0f;
    m_samplesSinceEdge = 0;
    m_ltcLocked = false;
    m_rxBitIdx = 0;
    m_mtcQuarterIndex = 0;
    m_samplesSinceQuarter = 0;
    encode80BitFrame(m_currentFrameBits);
}

} // namespace Jazz
