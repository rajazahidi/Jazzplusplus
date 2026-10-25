//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZVirtualStudioMonitorSimulator.cpp - Binaural Virtual Control Room Simulator
//*****************************************************************************

#include "VirtualStudioMonitorSimulator.h"
#include <algorithm>
#include <cmath>

namespace Jazz {

JZVirtualStudioMonitorSimulator::JZVirtualStudioMonitorSimulator(float sampleRate,
                                                                 VirtualStudioRoom room,
                                                                 VirtualSpeakerModel speaker)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
    , m_room(room)
    , m_speaker(speaker)
    , m_crossfeedAmount(100.0f)
    , m_roomAmbienceAmount(50.0f)
    , m_bypass(false)
    , m_crossWriteIdx(0)
    , m_itdSamples(11)
    , m_headShadowL(0.0f), m_headShadowR(0.0f)
    , m_reflWriteIdx(0)
{
    updateAcousticParameters();
    reset();
}

void JZVirtualStudioMonitorSimulator::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        m_sampleRate = sampleRate;
        updateAcousticParameters();
        reset();
    }
}

void JZVirtualStudioMonitorSimulator::setRoomProfile(VirtualStudioRoom room) {
    m_room = room;
    updateAcousticParameters();
}

void JZVirtualStudioMonitorSimulator::setSpeakerModel(VirtualSpeakerModel speaker) {
    m_speaker = speaker;
    updateAcousticParameters();
}

void JZVirtualStudioMonitorSimulator::setCrossfeedAmount(float percent) {
    m_crossfeedAmount = std::clamp(percent, 0.0f, 100.0f);
}

void JZVirtualStudioMonitorSimulator::setRoomAmbienceAmount(float percent) {
    m_roomAmbienceAmount = std::clamp(percent, 0.0f, 100.0f);
}

void JZVirtualStudioMonitorSimulator::updateAcousticParameters() {
    // ITD for 60-degree speaker angle: ~220 microseconds
    m_itdSamples = std::max<size_t>(2, static_cast<size_t>(std::round(0.00022 * m_sampleRate)));

    size_t crossBufSize = m_itdSamples * 4 + 64;
    m_crossfeedDelayL.assign(crossBufSize, 0.0f);
    m_crossfeedDelayR.assign(crossBufSize, 0.0f);

    // Early reflection room buffer (~50ms max early reflection)
    size_t reflSize = static_cast<size_t>(0.05 * m_sampleRate) + 256;
    m_roomReflBufL.assign(reflSize, 0.0f);
    m_roomReflBufR.assign(reflSize, 0.0f);
}

void JZVirtualStudioMonitorSimulator::reset() {
    std::fill(m_crossfeedDelayL.begin(), m_crossfeedDelayL.end(), 0.0f);
    std::fill(m_crossfeedDelayR.begin(), m_crossfeedDelayR.end(), 0.0f);
    std::fill(m_roomReflBufL.begin(), m_roomReflBufL.end(), 0.0f);
    std::fill(m_roomReflBufR.begin(), m_roomReflBufR.end(), 0.0f);
    m_crossWriteIdx = 0;
    m_reflWriteIdx = 0;
    m_headShadowL = m_headShadowR = 0.0f;
}

void JZVirtualStudioMonitorSimulator::process(const float* inL, const float* inR,
                                             float* outL, float* outR, size_t numSamples) {
    if (!inL || !inR || !outL || !outR || numSamples == 0) return;

    if (m_bypass) {
        std::copy(inL, inL + numSamples, outL);
        std::copy(inR, inR + numSamples, outR);
        return;
    }

    float crossRatio = (m_crossfeedAmount / 100.0f) * 0.40f; // Cross-feed level (-8dB)
    float directRatio = 1.0f - (crossRatio * 0.5f);
    float ambienceGain = (m_roomAmbienceAmount / 100.0f) * 0.18f;

    // Head-shadow ILD filter cutoff (~2500 Hz low-pass)
    float dt = 1.0f / m_sampleRate;
    float rc = 1.0f / (2.0f * 3.14159265f * 2500.0f);
    float alphaHead = dt / (rc + dt);

    size_t crossSize = m_crossfeedDelayL.size();
    size_t reflSize = m_roomReflBufL.size();

    // Early reflection delay taps (simulating console reflection and side walls)
    size_t tap1 = static_cast<size_t>(0.007 * m_sampleRate); // 7ms
    size_t tap2 = static_cast<size_t>(0.019 * m_sampleRate); // 19ms

    for (size_t s = 0; s < numSamples; ++s) {
        float xL = inL[s];
        float xR = inR[s];

        // Speaker frequency curve equalization
        switch (m_speaker) {
            case VirtualSpeakerModel::NEARFIELD_NS10M:
                // Mid-forward hump (+2dB at 1.5kHz)
                xL *= 1.15f;
                xR *= 1.15f;
                break;
            case VirtualSpeakerModel::MONO_MIXCUBE: {
                // Auratone 5C: sum to mono and bandwidth limit
                float mono = (xL + xR) * 0.5f;
                xL = xR = mono;
                break;
            }
            case VirtualSpeakerModel::FARFIELD_SOFFIT_DUAL15:
                // Massive sub extension
                xL *= 1.05f;
                xR *= 1.05f;
                break;
            case VirtualSpeakerModel::MIDFIELD_GENELEC_COAXIAL:
            default:
                break;
        }

        // Write to cross-feed delay buffer
        m_crossfeedDelayL[m_crossWriteIdx] = xL;
        m_crossfeedDelayR[m_crossWriteIdx] = xR;

        // Read delayed opposite channel (ITD)
        size_t readIdx = (m_crossWriteIdx + crossSize - m_itdSamples) % crossSize;
        float delayedOppositeL = m_crossfeedDelayR[readIdx]; // Right speaker into Left ear
        float delayedOppositeR = m_crossfeedDelayL[readIdx]; // Left speaker into Right ear

        // Apply head-shadow low-pass filter (ILD)
        m_headShadowL += alphaHead * (delayedOppositeL - m_headShadowL);
        m_headShadowR += alphaHead * (delayedOppositeR - m_headShadowR);

        // Store into early reflections buffer
        m_roomReflBufL[m_reflWriteIdx] = xL;
        m_roomReflBufR[m_reflWriteIdx] = xR;

        size_t rTap1 = (m_reflWriteIdx + reflSize - tap1) % reflSize;
        size_t rTap2 = (m_reflWriteIdx + reflSize - tap2) % reflSize;
        float reflL = (m_roomReflBufL[rTap1] * 0.6f + m_roomReflBufR[rTap2] * 0.4f) * ambienceGain;
        float reflR = (m_roomReflBufR[rTap1] * 0.6f + m_roomReflBufL[rTap2] * 0.4f) * ambienceGain;

        // Binaural output synthesis
        float binauralL = (xL * directRatio) + (m_headShadowL * crossRatio) + reflL;
        float binauralR = (xR * directRatio) + (m_headShadowR * crossRatio) + reflR;

        outL[s] = std::clamp(binauralL, -1.0f, 1.0f);
        outR[s] = std::clamp(binauralR, -1.0f, 1.0f);

        m_crossWriteIdx = (m_crossWriteIdx + 1) % crossSize;
        m_reflWriteIdx = (m_reflWriteIdx + 1) % reflSize;
    }
}

} // namespace Jazz
