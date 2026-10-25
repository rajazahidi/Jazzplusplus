//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// BinauralRoomImpulseInterpolationEngine.cpp - BRIR Convolver & SOFA Interpolator
//*****************************************************************************

#include "BinauralRoomImpulseInterpolationEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

static constexpr float PI_F = 3.14159265358979323846f;
static constexpr float DEG_TO_RAD = PI_F / 180.0f;
static constexpr float SPEED_OF_SOUND = 343.0f; // m/s
static constexpr float HEAD_RADIUS = 0.0875f;    // m

static const JZBinauralRoomImpulseInterpolationEngine::SpeakerCoordinate SPEAKER_LAYOUT[16] = {
    { -30.0f,   0.0f, 2.5f, "L" },
    {  30.0f,   0.0f, 2.5f, "R" },
    {   0.0f,   0.0f, 2.5f, "C" },
    {   0.0f, -15.0f, 2.5f, "LFE" },
    { -90.0f,   0.0f, 2.2f, "Lss" },
    {  90.0f,   0.0f, 2.2f, "Rss" },
    {-135.0f,   0.0f, 2.4f, "Lsr" },
    { 135.0f,   0.0f, 2.4f, "Rsr" },
    { -45.0f,  45.0f, 2.8f, "Ltf" },
    {  45.0f,  45.0f, 2.8f, "Rtf" },
    { -90.0f,  45.0f, 2.6f, "Ltm" },
    {  90.0f,  45.0f, 2.6f, "Rtm" },
    {-135.0f,  45.0f, 2.8f, "Ltr" },
    { 135.0f,  45.0f, 2.8f, "Rtr" },
    { -60.0f,   0.0f, 2.5f, "Lw" },
    {  60.0f,   0.0f, 2.5f, "Rw" }
};

JZBinauralRoomImpulseInterpolationEngine::JZBinauralRoomImpulseInterpolationEngine(float sampleRate)
    : m_sampleRate(sampleRate)
{
    initialize(sampleRate);
}

void JZBinauralRoomImpulseInterpolationEngine::initialize(float sampleRate)
{
    m_sampleRate = (sampleRate > 8000.0f) ? sampleRate : 48000.0f;

    for (size_t ch = 0; ch < NUM_CHANNELS; ++ch) {
        m_kernelL[ch].assign(BRIR_TAP_COUNT, 0.0f);
        m_kernelR[ch].assign(BRIR_TAP_COUNT, 0.0f);
        m_convBuffers[ch].assign(CONV_BUF_SIZE, 0.0f);
    }
    m_convWritePos = 0;

    reset();
    rebuildBrirImpulses();
}

void JZBinauralRoomImpulseInterpolationEngine::reset()
{
    for (size_t ch = 0; ch < NUM_CHANNELS; ++ch) {
        std::fill(m_convBuffers[ch].begin(), m_convBuffers[ch].end(), 0.0f);
    }
    m_convWritePos = 0;
}

void JZBinauralRoomImpulseInterpolationEngine::setParameters(const Parameters& params)
{
    m_params = params;
    m_params.yawDeg = std::max(-180.0f, std::min(m_params.yawDeg, 180.0f));
    m_params.pitchDeg = std::max(-90.0f, std::min(m_params.pitchDeg, 90.0f));
    m_params.rollDeg = std::max(-90.0f, std::min(m_params.rollDeg, 90.0f));
    m_params.roomDecaySec = std::max(0.1f, std::min(m_params.roomDecaySec, 1.5f));
    m_params.dryWet = std::max(0.0f, std::min(m_params.dryWet, 1.0f));

    rebuildBrirImpulses();
}

void JZBinauralRoomImpulseInterpolationEngine::setRoomPreset(BrirRoomPreset preset)
{
    m_params.roomPreset = preset;
    rebuildBrirImpulses();
}

void JZBinauralRoomImpulseInterpolationEngine::setHeadOrientation(float yaw, float pitch, float roll)
{
    m_params.yawDeg = std::max(-180.0f, std::min(yaw, 180.0f));
    m_params.pitchDeg = std::max(-90.0f, std::min(pitch, 90.0f));
    m_params.rollDeg = std::max(-90.0f, std::min(roll, 90.0f));
    rebuildBrirImpulses();
}

void JZBinauralRoomImpulseInterpolationEngine::setRoomDecaySec(float sec)
{
    m_params.roomDecaySec = std::max(0.1f, std::min(sec, 1.5f));
    rebuildBrirImpulses();
}

void JZBinauralRoomImpulseInterpolationEngine::setDryWet(float wet)
{
    m_params.dryWet = std::max(0.0f, std::min(wet, 1.0f));
}

void JZBinauralRoomImpulseInterpolationEngine::setBypass(bool bypass)
{
    m_params.bypassed = bypass;
}

std::string JZBinauralRoomImpulseInterpolationEngine::getRoomPresetName() const
{
    switch (m_params.roomPreset) {
        case BrirRoomPreset::STUDIO_CONTROL_ROOM_A:
            return "Mastering Control Room A (Abbey Road Style)";
        case BrirRoomPreset::CINEMA_DUB_STAGE:
            return "Dolby Atmos Cinema Dubbing Stage";
        case BrirRoomPreset::CUSTOM_SOFA_PROFILE:
            return "Measured Custom SOFA HRTF Profile";
    }
    return "Unknown";
}

void JZBinauralRoomImpulseInterpolationEngine::rebuildBrirImpulses()
{
    for (size_t ch = 0; ch < NUM_CHANNELS; ++ch) {
        std::fill(m_kernelL[ch].begin(), m_kernelL[ch].end(), 0.0f);
        std::fill(m_kernelR[ch].begin(), m_kernelR[ch].end(), 0.0f);

        const auto& spk = SPEAKER_LAYOUT[ch];

        // Rotate speaker azimuth and elevation relative to head orientation
        float rotatedAzimuthDeg = spk.azimuthDeg - m_params.yawDeg;
        while (rotatedAzimuthDeg > 180.0f) rotatedAzimuthDeg -= 360.0f;
        while (rotatedAzimuthDeg < -180.0f) rotatedAzimuthDeg += 360.0f;

        float rotatedElevationDeg = spk.elevationDeg - m_params.pitchDeg;
        rotatedElevationDeg = std::max(-90.0f, std::min(rotatedElevationDeg, 90.0f));

        float azRad = rotatedAzimuthDeg * DEG_TO_RAD;
        float dist = spk.distanceM;

        // Woodworth spherical head model ITD
        float itdSec = (HEAD_RADIUS / SPEED_OF_SOUND) * std::sin(azRad);
        float itdSamples = itdSec * m_sampleRate;

        int delayL = std::max(0, std::min(static_cast<int>(BRIR_TAP_COUNT - 8), static_cast<int>(8.0f + itdSamples * 0.5f)));
        int delayR = std::max(0, std::min(static_cast<int>(BRIR_TAP_COUNT - 8), static_cast<int>(8.0f - itdSamples * 0.5f)));

        // ILD attenuation
        float ildL = 1.0f - 0.35f * std::max(0.0f, std::sin(azRad));
        float ildR = 1.0f - 0.35f * std::max(0.0f, -std::sin(azRad));
        float distAtten = 1.0f / dist;

        // Direct path
        m_kernelL[ch][delayL] = distAtten * ildL * 0.85f;
        m_kernelR[ch][delayR] = distAtten * ildR * 0.85f;

        // Early room reflections (floor, ceiling, wall bounces)
        float decayRate = 1.0f / (m_params.roomDecaySec * 35.0f);
        int reflectionTaps[3] = { 16, 28, 42 };
        float reflectionGains[3] = { 0.28f, 0.18f, 0.12f };

        for (int r = 0; r < 3; ++r) {
            int tapL = delayL + reflectionTaps[r];
            int tapR = delayR + reflectionTaps[r];
            float damp = std::exp(-static_cast<float>(reflectionTaps[r]) * decayRate);

            if (tapL < static_cast<int>(BRIR_TAP_COUNT)) {
                m_kernelL[ch][tapL] += reflectionGains[r] * damp * distAtten;
            }
            if (tapR < static_cast<int>(BRIR_TAP_COUNT)) {
                m_kernelR[ch][tapR] += reflectionGains[r] * damp * distAtten;
            }
        }
    }
}

void JZBinauralRoomImpulseInterpolationEngine::process16ChannelsToBinaural(
    const float* const* in16Channels,
    float* outL, float* outR,
    size_t numSamples)
{
    if (!in16Channels || !outL || !outR || numSamples == 0) return;

    if (m_params.bypassed) {
        if (in16Channels[0]) std::copy(in16Channels[0], in16Channels[0] + numSamples, outL);
        if (in16Channels[1]) std::copy(in16Channels[1], in16Channels[1] + numSamples, outR);
        return;
    }

    for (size_t i = 0; i < numSamples; ++i) {
        float accumL = 0.0f;
        float accumR = 0.0f;

        // Push new sample into delay line for each active channel
        for (size_t ch = 0; ch < NUM_CHANNELS; ++ch) {
            float smp = in16Channels[ch] ? in16Channels[ch][i] : 0.0f;
            m_convBuffers[ch][m_convWritePos] = smp;

            // Short BRIR FIR convolution
            for (size_t tap = 0; tap < BRIR_TAP_COUNT; ++tap) {
                int readIdx = static_cast<int>(m_convWritePos) - static_cast<int>(tap);
                if (readIdx < 0) readIdx += CONV_BUF_SIZE;

                float pastSmp = m_convBuffers[ch][readIdx];
                accumL += pastSmp * m_kernelL[ch][tap];
                accumR += pastSmp * m_kernelR[ch][tap];
            }
        }

        m_convWritePos = (m_convWritePos + 1) % CONV_BUF_SIZE;

        // Direct Stereo bypass reference for dry/wet
        float dryL = in16Channels[0] ? in16Channels[0][i] : 0.0f;
        float dryR = in16Channels[1] ? in16Channels[1][i] : 0.0f;

        outL[i] = dryL * (1.0f - m_params.dryWet) + accumL * m_params.dryWet;
        outR[i] = dryR * (1.0f - m_params.dryWet) + accumR * m_params.dryWet;
    }
}

} // namespace Jazz
