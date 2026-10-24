#include "BinauralHrtfSpatializer.h"
#include <algorithm>
#include <cmath>

namespace Jazz {

static const float PI = 3.14159265358979323846f;
static const float DEG2RAD = PI / 180.0f;
static const float HEAD_RADIUS = 0.0875f; // meters
static const float SPEED_OF_SOUND = 343.0f; // m/s

JZBinauralHrtfSpatializer::JZBinauralHrtfSpatializer(float sampleRate)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f)
    , m_yaw(0.0f)
    , m_pitch(0.0f)
    , m_roll(0.0f)
    , m_crossTalkCancel(false)
    , m_roomAmbience(20.0f)
    , m_writeIdx(0)
    , m_itdSamplesL(0.0f)
    , m_itdSamplesR(0.0f)
    , m_ildL(0.0f)
    , m_ildR(0.0f)
    , m_ildCoeffL(1.0f)
    , m_ildCoeffR(1.0f)
    , m_ildGainL(1.0f)
    , m_ildGainR(1.0f)
    , m_pb0L(1.0f), m_pb1L(0.0f), m_pb2L(0.0f), m_pa1L(0.0f), m_pa2L(0.0f)
    , m_pb0R(1.0f), m_pb1R(0.0f), m_pb2R(0.0f), m_pa1R(0.0f), m_pa2R(0.0f)
    , m_roomWriteIdx(0)
{
    m_sourcePos.azimuthDeg = 0.0f;
    m_sourcePos.elevationDeg = 0.0f;
    m_sourcePos.distance = 1.5f;

    reset();
    updateHrtfFilters();
}

float JZBinauralHrtfSpatializer::getMaxItdMs() {
    // Woodworth formula max ITD: (a/c)*(pi/2 + 1)
    return (HEAD_RADIUS / SPEED_OF_SOUND) * (PI * 0.5f + 1.0f) * 1000.0f; // ~0.655 ms
}

void JZBinauralHrtfSpatializer::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        m_sampleRate = sampleRate;
        updateHrtfFilters();
    }
}

void JZBinauralHrtfSpatializer::setSourcePosition(float azimuthDeg, float elevationDeg, float distanceMeters) {
    m_sourcePos.azimuthDeg = std::max(-180.0f, std::min(180.0f, azimuthDeg));
    m_sourcePos.elevationDeg = std::max(-90.0f, std::min(90.0f, elevationDeg));
    m_sourcePos.distance = std::max(0.1f, distanceMeters);
    updateHrtfFilters();
}

void JZBinauralHrtfSpatializer::setHeadOrientation(float yawDeg, float pitchDeg, float rollDeg) {
    m_yaw = yawDeg;
    m_pitch = pitchDeg;
    m_roll = rollDeg;
    updateHrtfFilters();
}

void JZBinauralHrtfSpatializer::getHeadOrientation(float& yawDeg, float& pitchDeg, float& rollDeg) const {
    yawDeg = m_yaw;
    pitchDeg = m_pitch;
    rollDeg = m_roll;
}

void JZBinauralHrtfSpatializer::setRoomAmbience(float levelPercent) {
    m_roomAmbience = std::max(0.0f, std::min(100.0f, levelPercent));
}

void JZBinauralHrtfSpatializer::reset() {
    for (int i = 0; i < DELAY_SIZE; ++i) {
        m_delayBufL[i] = 0.0f;
        m_delayBufR[i] = 0.0f;
    }
    m_writeIdx = 0;
    m_ildL = 0.0f;
    m_ildR = 0.0f;
    m_pinnaL[0] = m_pinnaL[1] = 0.0f;
    m_pinnaR[0] = m_pinnaR[1] = 0.0f;
    for (int i = 0; i < 1024; ++i) {
        m_roomDelayL[i] = 0.0f;
        m_roomDelayR[i] = 0.0f;
    }
    m_roomWriteIdx = 0;
}

void JZBinauralHrtfSpatializer::updateHrtfFilters() {
    // Relative azimuth and elevation taking head-tracking into account
    float relAz = m_sourcePos.azimuthDeg - m_yaw;
    while (relAz > 180.0f) relAz -= 360.0f;
    while (relAz < -180.0f) relAz += 360.0f;

    float relEl = m_sourcePos.elevationDeg - m_pitch;
    relEl = std::max(-90.0f, std::min(90.0f, relEl));

    float theta = relAz * DEG2RAD;
    float absTheta = std::abs(theta);

    // Woodworth formula for ITD
    // Delay for far ear: (a / c) * (theta + sin(theta))
    float farDelaySec = (HEAD_RADIUS / SPEED_OF_SOUND) * (absTheta + std::sin(absTheta));
    float farDelaySamples = farDelaySec * m_sampleRate;

    if (theta >= 0.0f) {
        // Source is to the Right -> Left ear is far ear
        m_itdSamplesR = 0.0f;
        m_itdSamplesL = std::min(static_cast<float>(DELAY_SIZE - 2), farDelaySamples);

        // ILD shadowing on Left ear
        float shadow = 0.5f * (1.0f - std::cos(absTheta));
        m_ildGainL = 1.0f - shadow * 0.70f; // up to -10.5 dB
        m_ildGainR = 1.0f;
        m_ildCoeffL = 0.25f + 0.75f * (1.0f - shadow * 0.6f);
        m_ildCoeffR = 1.0f;
    } else {
        // Source is to the Left -> Right ear is far ear
        m_itdSamplesL = 0.0f;
        m_itdSamplesR = std::min(static_cast<float>(DELAY_SIZE - 2), farDelaySamples);

        // ILD shadowing on Right ear
        float shadow = 0.5f * (1.0f - std::cos(absTheta));
        m_ildGainR = 1.0f - shadow * 0.70f;
        m_ildGainL = 1.0f;
        m_ildCoeffR = 0.25f + 0.75f * (1.0f - shadow * 0.6f);
        m_ildCoeffL = 1.0f;
    }

    // Distance attenuation: inverse law clamped
    float distAtten = 1.0f / std::max(0.5f, m_sourcePos.distance);
    m_ildGainL *= distAtten;
    m_ildGainR *= distAtten;

    // Pinna elevation spectral notch (6.5 kHz to 9.5 kHz)
    float notchFreq = 6500.0f + 1500.0f * (relEl / 90.0f);
    notchFreq = std::max(2000.0f, std::min(m_sampleRate * 0.45f, notchFreq));

    float w0 = 2.0f * PI * notchFreq / m_sampleRate;
    float alpha = std::sin(w0) / (2.0f * 3.0f); // Q = 3.0
    float cosw0 = std::cos(w0);

    float a0 = 1.0f + alpha;
    m_pb0L = m_pb0R = 1.0f / a0;
    m_pb1L = m_pb1R = (-2.0f * cosw0) / a0;
    m_pb2L = m_pb2R = 1.0f / a0;
    m_pa1L = m_pa1R = (-2.0f * cosw0) / a0;
    m_pa2L = m_pa2R = (1.0f - alpha) / a0;
}

float JZBinauralHrtfSpatializer::fractionalDelay(const float* buffer, int bufSize, int writeIdx, float delaySamples) const {
    float readPos = static_cast<float>(writeIdx) - delaySamples;
    while (readPos < 0.0f) readPos += static_cast<float>(bufSize);

    int idx0 = static_cast<int>(readPos) % bufSize;
    int idx1 = (idx0 + 1) % bufSize;
    float frac = readPos - std::floor(readPos);

    return buffer[idx0] * (1.0f - frac) + buffer[idx1] * frac;
}

void JZBinauralHrtfSpatializer::processPointSource(float inMono, float& outLeft, float& outRight) {
    // Write into delay lines
    m_delayBufL[m_writeIdx] = inMono;
    m_delayBufR[m_writeIdx] = inMono;

    // Read with Woodworth ITD
    float sigL = fractionalDelay(m_delayBufL, DELAY_SIZE, m_writeIdx, m_itdSamplesL);
    float sigR = fractionalDelay(m_delayBufR, DELAY_SIZE, m_writeIdx, m_itdSamplesR);

    m_writeIdx = (m_writeIdx + 1) % DELAY_SIZE;

    // Head-shadowing ILD filter (1-pole low-pass + gain)
    m_ildL = m_ildCoeffL * sigL + (1.0f - m_ildCoeffL) * m_ildL;
    m_ildR = m_ildCoeffR * sigR + (1.0f - m_ildCoeffR) * m_ildR;

    float sL = m_ildL * m_ildGainL;
    float sR = m_ildR * m_ildGainR;

    // Pinna elevation notch biquad
    float pinL = m_pb0L * sL + m_pinnaL[0];
    m_pinnaL[0] = m_pb1L * sL - m_pa1L * pinL + m_pinnaL[1];
    m_pinnaL[1] = m_pb2L * sL - m_pa2L * pinL;

    float pinR = m_pb0R * sR + m_pinnaR[0];
    m_pinnaR[0] = m_pb1R * sR - m_pa1R * pinR + m_pinnaR[1];
    m_pinnaR[1] = m_pb2R * sR - m_pa2R * pinR;

    // Room early reflections for out-of-head localization
    m_roomDelayL[m_roomWriteIdx] = pinL;
    m_roomDelayR[m_roomWriteIdx] = pinR;

    if (m_roomAmbience > 1.0f) {
        float roomGain = (m_roomAmbience * 0.01f) * 0.25f;
        // 4 reflection taps: 7ms, 14ms, 21ms, 31ms
        int tap1 = (m_roomWriteIdx - static_cast<int>(m_sampleRate * 0.007f) + 1024) % 1024;
        int tap2 = (m_roomWriteIdx - static_cast<int>(m_sampleRate * 0.014f) + 1024) % 1024;
        int tap3 = (m_roomWriteIdx - static_cast<int>(m_sampleRate * 0.021f) + 1024) % 1024;

        pinL += roomGain * (0.6f * m_roomDelayR[tap1] - 0.4f * m_roomDelayL[tap2] + 0.3f * m_roomDelayR[tap3]);
        pinR += roomGain * (0.6f * m_roomDelayL[tap1] - 0.4f * m_roomDelayR[tap2] + 0.3f * m_roomDelayL[tap3]);
    }

    m_roomWriteIdx = (m_roomWriteIdx + 1) % 1024;

    // Cross-talk cancellation (mild 180-deg delayed inverted acoustic crosstalk)
    if (m_crossTalkCancel) {
        float xtalk = 0.15f;
        float cleanL = pinL - xtalk * pinR;
        float cleanR = pinR - xtalk * pinL;
        pinL = cleanL;
        pinR = cleanR;
    }

    outLeft = pinL;
    outRight = pinR;
}

void JZBinauralHrtfSpatializer::processStereo(float inL, float inR, float& outL, float& outR) {
    // Process Left channel at -30 deg, Right channel at +30 deg
    SpatialPoint3D savedPos = m_sourcePos;

    setSourcePosition(-30.0f, savedPos.elevationDeg, savedPos.distance);
    float l_L = 0.0f, l_R = 0.0f;
    processPointSource(inL, l_L, l_R);

    setSourcePosition(30.0f, savedPos.elevationDeg, savedPos.distance);
    float r_L = 0.0f, r_R = 0.0f;
    processPointSource(inR, r_L, r_R);

    outL = l_L + r_L;
    outR = l_R + r_R;

    m_sourcePos = savedPos;
    updateHrtfFilters();
}

void JZBinauralHrtfSpatializer::process7_1_4(const float channels[12], float& outL, float& outR) {
    if (!channels) {
        outL = outR = 0.0f;
        return;
    }

    // 7.1.4 positions:
    // 0: L (-30, 0), 1: R (+30, 0), 2: C (0, 0), 3: LFE (0, -10),
    // 4: Lss (-90, 0), 5: Rss (+90, 0), 6: Lsr (-135, 0), 7: Rsr (+135, 0),
    // 8: Ltf (-30, 45), 9: Rtf (+30, 45), 10: Ltr (-135, 45), 11: Rtr (+135, 45)
    static const float azList[12] = {-30.0f, 30.0f, 0.0f, 0.0f, -90.0f, 90.0f, -135.0f, 135.0f, -30.0f, 30.0f, -135.0f, 135.0f};
    static const float elList[12] = {0.0f, 0.0f, 0.0f, -10.0f, 0.0f, 0.0f, 0.0f, 0.0f, 45.0f, 45.0f, 45.0f, 45.0f};

    SpatialPoint3D savedPos = m_sourcePos;
    float totalL = 0.0f;
    float totalR = 0.0f;

    for (int ch = 0; ch < 12; ++ch) {
        if (std::abs(channels[ch]) > 1e-6f) {
            setSourcePosition(azList[ch], elList[ch], 1.5f);
            float curL = 0.0f, curR = 0.0f;
            processPointSource(channels[ch], curL, curR);
            totalL += curL;
            totalR += curR;
        }
    }

    m_sourcePos = savedPos;
    updateHrtfFilters();

    outL = totalL;
    outR = totalR;
}

} // namespace Jazz
