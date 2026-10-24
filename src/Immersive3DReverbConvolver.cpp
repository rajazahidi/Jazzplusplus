#include "Immersive3DReverbConvolver.h"
#include <algorithm>
#include <cmath>

namespace Jazz {

static const float SPEED_OF_SOUND = 343.0f; // m/s

JZImmersive3DReverbConvolver::JZImmersive3DReverbConvolver(float sampleRate)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f)
    , m_preset(ReverbSpacePreset::CONCERT_HALL)
    , m_roomLength(24.0f)
    , m_roomWidth(16.0f)
    , m_roomHeight(9.0f)
    , m_rt60(2.4f)
    , m_damping(0.35f)
    , m_earlyLateRatio(0.40f)
    , m_wetMix(0.30f)
    , m_erWriteIdx(0)
{
    // Mutually prime delay line lengths for 8-channel 3D FDN
    static const int baseLengths[FDN_LINES] = {1117, 1361, 1543, 1777, 1999, 2213, 2467, 2699};
    for (int i = 0; i < FDN_LINES; ++i) {
        m_fdnLengths[i] = static_cast<int>(baseLengths[i] * (m_sampleRate / 44100.0f));
        m_fdnBuffers[i].assign(m_fdnLengths[i] + 16, 0.0f);
        m_fdnWriteIdx[i] = 0;
        m_fdnFilterState[i] = 0.0f;
    }

    for (int i = 0; i < ER_BUFFER_SIZE; ++i) {
        m_erBuffer[i] = 0.0f;
    }

    setPreset(ReverbSpacePreset::CONCERT_HALL);
}

void JZImmersive3DReverbConvolver::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        m_sampleRate = sampleRate;
        updateFdnCoeffs();
    }
}

void JZImmersive3DReverbConvolver::setRoomDimensions(float lengthM, float widthM, float heightM) {
    m_roomLength = std::max(4.0f, std::min(100.0f, lengthM));
    m_roomWidth = std::max(3.0f, std::min(80.0f, widthM));
    m_roomHeight = std::max(2.5f, std::min(40.0f, heightM));
    updateFdnCoeffs();
}

void JZImmersive3DReverbConvolver::getRoomDimensions(float& lengthM, float& widthM, float& heightM) const {
    lengthM = m_roomLength;
    widthM = m_roomWidth;
    heightM = m_roomHeight;
}

void JZImmersive3DReverbConvolver::setDecayTimeRt60(float seconds) {
    m_rt60 = std::max(0.2f, std::min(12.0f, seconds));
    updateFdnCoeffs();
}

void JZImmersive3DReverbConvolver::setDamping(float damping) {
    m_damping = std::max(0.0f, std::min(1.0f, damping));
}

void JZImmersive3DReverbConvolver::setEarlyLateMix(float ratio) {
    m_earlyLateRatio = std::max(0.0f, std::min(1.0f, ratio));
}

void JZImmersive3DReverbConvolver::setWetMix(float mix) {
    m_wetMix = std::max(0.0f, std::min(1.0f, mix));
}

void JZImmersive3DReverbConvolver::setPreset(ReverbSpacePreset preset) {
    m_preset = preset;
    switch (preset) {
        case ReverbSpacePreset::CONCERT_HALL:
            setRoomDimensions(24.0f, 16.0f, 9.0f);
            setDecayTimeRt60(2.4f);
            setDamping(0.35f);
            break;
        case ReverbSpacePreset::CATHEDRAL:
            setRoomDimensions(36.0f, 22.0f, 18.0f);
            setDecayTimeRt60(5.2f);
            setDamping(0.25f);
            break;
        case ReverbSpacePreset::STUDIO_LIVE_ROOM:
            setRoomDimensions(9.0f, 6.5f, 3.8f);
            setDecayTimeRt60(0.85f);
            setDamping(0.55f);
            break;
        case ReverbSpacePreset::AMBIENT_ARENA:
            setRoomDimensions(50.0f, 35.0f, 22.0f);
            setDecayTimeRt60(6.8f);
            setDamping(0.30f);
            break;
    }
}

void JZImmersive3DReverbConvolver::reset() {
    for (int i = 0; i < ER_BUFFER_SIZE; ++i) {
        m_erBuffer[i] = 0.0f;
    }
    m_erWriteIdx = 0;
    for (int i = 0; i < FDN_LINES; ++i) {
        std::fill(m_fdnBuffers[i].begin(), m_fdnBuffers[i].end(), 0.0f);
        m_fdnWriteIdx[i] = 0;
        m_fdnFilterState[i] = 0.0f;
    }
}

void JZImmersive3DReverbConvolver::updateFdnCoeffs() {
    // 1. Calculate 3D geometric early reflection delay taps based on room boundaries
    auto calcTap = [this](float distanceMeters) -> int {
        float timeSec = distanceMeters / SPEED_OF_SOUND;
        int tapSamples = static_cast<int>(timeSec * m_sampleRate);
        return std::max(1, std::min(ER_BUFFER_SIZE - 1, tapSamples));
    };

    m_tapFront = calcTap(m_roomLength * 0.5f);
    m_tapBack  = calcTap(m_roomLength * 0.8f);
    m_tapLeft  = calcTap(m_roomWidth * 0.45f);
    m_tapRight = calcTap(m_roomWidth * 0.45f);
    m_tapFloor = calcTap(2.2f);
    m_tapCeil  = calcTap(m_roomHeight * 0.7f);

    m_gainFront = 0.65f / (1.0f + m_roomLength * 0.02f);
    m_gainBack  = 0.55f / (1.0f + m_roomLength * 0.03f);
    m_gainLeft  = 0.60f / (1.0f + m_roomWidth * 0.025f);
    m_gainRight = 0.60f / (1.0f + m_roomWidth * 0.025f);
    m_gainFloor = 0.45f;
    m_gainCeil  = 0.58f / (1.0f + m_roomHeight * 0.035f);

    // 2. Calculate FDN feedback gains: g_i = 10^(-3 * d_i / (RT60 * fs))
    for (int i = 0; i < FDN_LINES; ++i) {
        float delaySec = static_cast<float>(m_fdnLengths[i]) / m_sampleRate;
        float gain = std::pow(10.0f, (-3.0f * delaySec) / m_rt60);
        m_fdnFeedbackGains[i] = gain;
    }
}

void JZImmersive3DReverbConvolver::process7_1_4(float inL, float inR, float outChannels[12]) {
    float monoIn = 0.5f * (inL + inR);

    // Write into Early Reflections buffer
    m_erBuffer[m_erWriteIdx] = monoIn;

    auto readEr = [this](int tap) -> float {
        int idx = (m_erWriteIdx - tap + ER_BUFFER_SIZE) % ER_BUFFER_SIZE;
        return m_erBuffer[idx];
    };

    float erFront = readEr(m_tapFront) * m_gainFront;
    float erBack  = readEr(m_tapBack) * m_gainBack;
    float erLeft  = readEr(m_tapLeft) * m_gainLeft;
    float erRight = readEr(m_tapRight) * m_gainRight;
    float erFloor = readEr(m_tapFloor) * m_gainFloor;
    float erCeil  = readEr(m_tapCeil) * m_gainCeil;

    m_erWriteIdx = (m_erWriteIdx + 1) % ER_BUFFER_SIZE;

    // 8-channel Feedback Delay Network (FDN) processing
    float fdnOutputs[FDN_LINES];
    for (int i = 0; i < FDN_LINES; ++i) {
        int readIdx = (m_fdnWriteIdx[i] - m_fdnLengths[i] + m_fdnBuffers[i].size()) % m_fdnBuffers[i].size();
        float outSample = m_fdnBuffers[i][readIdx];

        // 1-pole damping lowpass filter
        m_fdnFilterState[i] = (1.0f - m_damping) * outSample + m_damping * m_fdnFilterState[i];
        fdnOutputs[i] = m_fdnFilterState[i];
    }

    // Orthogonal Householder matrix mixing: y = x - (2/N)*sum(x)
    float sumFdn = 0.0f;
    for (int i = 0; i < FDN_LINES; ++i) sumFdn += fdnOutputs[i];
    float householderFactor = (2.0f / static_cast<float>(FDN_LINES)) * sumFdn;

    for (int i = 0; i < FDN_LINES; ++i) {
        float mixed = fdnOutputs[i] - householderFactor;
        float feedbackSample = monoIn * 0.35f + mixed * m_fdnFeedbackGains[i];
        m_fdnBuffers[i][m_fdnWriteIdx[i]] = feedbackSample;
        m_fdnWriteIdx[i] = (m_fdnWriteIdx[i] + 1) % m_fdnBuffers[i].size();
    }

    // Blend early reflections and late tail
    float earlyW = 1.0f - m_earlyLateRatio;
    float lateW  = m_earlyLateRatio;

    // Output Channel Mapping:
    // 0: L, 1: R, 2: C, 3: LFE, 4: Lss, 5: Rss, 6: Lsr, 7: Rsr, 8: Ltf, 9: Rtf, 10: Ltr, 11: Rtr
    outChannels[0]  = (inL  + (erFront * 0.7f + erLeft * 0.6f + erFloor * 0.4f) * earlyW + fdnOutputs[0] * lateW * m_wetMix);
    outChannels[1]  = (inR  + (erFront * 0.7f + erRight * 0.6f + erFloor * 0.4f) * earlyW + fdnOutputs[1] * lateW * m_wetMix);
    outChannels[2]  = (monoIn * 0.7f + erFront * earlyW + (fdnOutputs[0] + fdnOutputs[1]) * 0.5f * lateW * m_wetMix);
    outChannels[3]  = (monoIn * 0.3f + erFloor * 0.5f * earlyW); // LFE
    outChannels[4]  = (erLeft * earlyW + fdnOutputs[2] * lateW * m_wetMix);
    outChannels[5]  = (erRight * earlyW + fdnOutputs[3] * lateW * m_wetMix);
    outChannels[6]  = (erBack * 0.7f + erLeft * 0.4f) * earlyW + fdnOutputs[4] * lateW * m_wetMix;
    outChannels[7]  = (erBack * 0.7f + erRight * 0.4f) * earlyW + fdnOutputs[5] * lateW * m_wetMix;

    // 4 Height Ceiling Speakers (Crucial for 3D Dolby Atmos acoustics!)
    outChannels[8]  = (erCeil * 0.8f + erFront * 0.3f) * earlyW + fdnOutputs[6] * lateW * m_wetMix; // Ltf
    outChannels[9]  = (erCeil * 0.8f + erFront * 0.3f) * earlyW + fdnOutputs[7] * lateW * m_wetMix; // Rtf
    outChannels[10] = (erCeil * 0.8f + erBack * 0.4f) * earlyW + fdnOutputs[6] * lateW * m_wetMix;  // Ltr
    outChannels[11] = (erCeil * 0.8f + erBack * 0.4f) * earlyW + fdnOutputs[7] * lateW * m_wetMix;  // Rtr
}

void JZImmersive3DReverbConvolver::processStereo(float& left, float& right) {
    float channels[12];
    process7_1_4(left, right, channels);

    // Downmix 7.1.4 into stereo
    left  = channels[0] + channels[2] * 0.707f + channels[4] * 0.6f + channels[6] * 0.5f + channels[8] * 0.5f + channels[10] * 0.4f;
    right = channels[1] + channels[2] * 0.707f + channels[5] * 0.6f + channels[7] * 0.5f + channels[9] * 0.5f + channels[11] * 0.4f;
}

} // namespace Jazz
