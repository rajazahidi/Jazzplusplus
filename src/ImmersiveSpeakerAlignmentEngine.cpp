//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// ImmersiveSpeakerAlignmentEngine.cpp - 9.1.6 Dolby Atmos Speaker Distance Aligner
//*****************************************************************************

#include "ImmersiveSpeakerAlignmentEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

JZImmersiveSpeakerAlignmentEngine::JZImmersiveSpeakerAlignmentEngine(float sampleRate) {
    initialize(sampleRate);
}

void JZImmersiveSpeakerAlignmentEngine::initialize(float sampleRate) {
    if (sampleRate < 8000.0f) sampleRate = 48000.0f;
    m_sampleRate = sampleRate;

    static const char* channelNames[NUM_CHANNELS] = {
        "L", "R", "C", "LFE",
        "Lss", "Rss", "Lrs", "Rrs",
        "Lw", "Rw",
        "Ltf", "Rtf", "Ltm", "Rtm", "Ltr", "Rtr"
    };

    for (size_t ch = 0; ch < NUM_CHANNELS; ++ch) {
        m_speakers[ch].name = channelNames[ch];
        m_speakers[ch].distanceMeters = 2.5f;
        m_speakers[ch].gainTrimDb = 0.0f;
        m_speakers[ch].invertPhase = false;
        m_speakers[ch].isMuted = false;

        m_delayLines[ch].assign(MAX_DELAY_SAMPLES, 0.0f);
        m_writePos[ch] = 0;
        m_delaySamples[ch] = 0;
        m_linearGain[ch] = 1.0f;
    }

    reset();
    updateAlignmentDelays();
}

void JZImmersiveSpeakerAlignmentEngine::reset() {
    for (size_t ch = 0; ch < NUM_CHANNELS; ++ch) {
        std::fill(m_delayLines[ch].begin(), m_delayLines[ch].end(), 0.0f);
        m_writePos[ch] = 0;
    }
}

void JZImmersiveSpeakerAlignmentEngine::setSpeakerDistanceMeters(size_t channelIndex, float meters) {
    if (channelIndex < NUM_CHANNELS) {
        m_speakers[channelIndex].distanceMeters = std::max(0.5f, std::min(15.0f, meters));
        updateAlignmentDelays();
    }
}

float JZImmersiveSpeakerAlignmentEngine::getSpeakerDistanceMeters(size_t channelIndex) const {
    if (channelIndex < NUM_CHANNELS) return m_speakers[channelIndex].distanceMeters;
    return 2.5f;
}

void JZImmersiveSpeakerAlignmentEngine::setSpeakerGainTrimDb(size_t channelIndex, float trimDb) {
    if (channelIndex < NUM_CHANNELS) {
        m_speakers[channelIndex].gainTrimDb = std::max(-18.0f, std::min(18.0f, trimDb));
        updateAlignmentDelays();
    }
}

float JZImmersiveSpeakerAlignmentEngine::getSpeakerGainTrimDb(size_t channelIndex) const {
    if (channelIndex < NUM_CHANNELS) return m_speakers[channelIndex].gainTrimDb;
    return 0.0f;
}

void JZImmersiveSpeakerAlignmentEngine::setSpeakerPolarity(size_t channelIndex, bool invertPhase) {
    if (channelIndex < NUM_CHANNELS) {
        m_speakers[channelIndex].invertPhase = invertPhase;
        updateAlignmentDelays();
    }
}

std::string JZImmersiveSpeakerAlignmentEngine::getChannelName(size_t channelIndex) const {
    if (channelIndex < NUM_CHANNELS) return m_speakers[channelIndex].name;
    return "Unknown";
}

float JZImmersiveSpeakerAlignmentEngine::getChannelDelayMs(size_t channelIndex) const {
    if (channelIndex < NUM_CHANNELS) {
        return (static_cast<float>(m_delaySamples[channelIndex]) / m_sampleRate) * 1000.0f;
    }
    return 0.0f;
}

size_t JZImmersiveSpeakerAlignmentEngine::getChannelDelaySamples(size_t channelIndex) const {
    if (channelIndex < NUM_CHANNELS) return m_delaySamples[channelIndex];
    return 0;
}

void JZImmersiveSpeakerAlignmentEngine::updateAlignmentDelays() {
    const float speedOfSound = 343.0f; // m/s

    // Find maximum distance across active speakers
    float maxDistance = 0.0f;
    for (size_t ch = 0; ch < NUM_CHANNELS; ++ch) {
        if (m_speakers[ch].distanceMeters > maxDistance) {
            maxDistance = m_speakers[ch].distanceMeters;
        }
    }

    for (size_t ch = 0; ch < NUM_CHANNELS; ++ch) {
        // Delta distance: closer speakers need delay so acoustic wavefronts arrive simultaneously
        float deltaMeters = maxDistance - m_speakers[ch].distanceMeters;
        float delaySec = deltaMeters / speedOfSound;
        size_t delaySamps = static_cast<size_t>(delaySec * m_sampleRate);

        if (delaySamps >= MAX_DELAY_SAMPLES) delaySamps = MAX_DELAY_SAMPLES - 1;
        m_delaySamples[ch] = delaySamps;

        float lin = std::pow(10.0f, m_speakers[ch].gainTrimDb / 20.0f);
        if (m_speakers[ch].invertPhase) lin = -lin;
        if (m_speakers[ch].isMuted) lin = 0.0f;
        m_linearGain[ch] = lin;
    }
}

void JZImmersiveSpeakerAlignmentEngine::generateMeasurementChirp(
    float* chirpBuffer, size_t numSamples, float startHz, float endHz) {

    if (!chirpBuffer || numSamples == 0) return;

    float T = static_cast<float>(numSamples) / m_sampleRate;
    float k = std::pow(endHz / startHz, 1.0f / T);
    float lnK = std::log(k);

    for (size_t i = 0; i < numSamples; ++i) {
        float t = static_cast<float>(i) / m_sampleRate;
        float phase = 2.0f * static_cast<float>(M_PI) * startHz * ((std::pow(k, t) - 1.0f) / lnK);

        // Tukey window on chirp
        float win = 1.0f;
        size_t rampLen = numSamples / 20;
        if (i < rampLen) win = 0.5f * (1.0f - std::cos(static_cast<float>(M_PI) * i / rampLen));
        else if (i > numSamples - rampLen) win = 0.5f * (1.0f - std::cos(static_cast<float>(M_PI) * (numSamples - i) / rampLen));

        chirpBuffer[i] = 0.6f * std::sin(phase) * win;
    }
}

void JZImmersiveSpeakerAlignmentEngine::process16Channels(
    const float* const* inputs,
    float* const* outputs,
    size_t numSamples) {

    if (!inputs || !outputs || numSamples == 0) return;

    for (size_t ch = 0; ch < NUM_CHANNELS; ++ch) {
        const float* inCh = inputs[ch];
        float* outCh = outputs[ch];
        if (!inCh || !outCh) continue;

        size_t delay = m_delaySamples[ch];
        float gain = m_linearGain[ch];
        auto& line = m_delayLines[ch];
        size_t& wPos = m_writePos[ch];

        for (size_t i = 0; i < numSamples; ++i) {
            line[wPos] = inCh[i];
            size_t rPos = (wPos + MAX_DELAY_SAMPLES - delay) % MAX_DELAY_SAMPLES;
            outCh[i] = line[rPos] * gain;
            wPos = (wPos + 1) % MAX_DELAY_SAMPLES;
        }
    }
}

} // namespace Jazz
