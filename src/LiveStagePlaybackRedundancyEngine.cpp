//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// LiveStagePlaybackRedundancyEngine.cpp - Live Stage Playback Redundancy
//*****************************************************************************

#include "LiveStagePlaybackRedundancyEngine.h"
#include <cmath>
#include <algorithm>

namespace Jazz {

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

JZLiveStagePlaybackRedundancyEngine::JZLiveStagePlaybackRedundancyEngine(float sampleRate) {
    initialize(sampleRate);
}

void JZLiveStagePlaybackRedundancyEngine::initialize(float sampleRate) {
    if (sampleRate < 8000.0f) sampleRate = 48000.0f;
    m_sampleRate = sampleRate;

    m_bufAL.assign(BUFFER_CAPACITY, 0.0f);
    m_bufAR.assign(BUFFER_CAPACITY, 0.0f);
    m_bufBL.assign(BUFFER_CAPACITY, 0.0f);
    m_bufBR.assign(BUFFER_CAPACITY, 0.0f);

    reset();
    updateFilter();
}

void JZLiveStagePlaybackRedundancyEngine::reset() {
    m_activeRig = StageRigId::PRIMARY_RIG_A;
    m_previousRig = StageRigId::PRIMARY_RIG_A;

    std::fill(m_bufAL.begin(), m_bufAL.end(), 0.0f);
    std::fill(m_bufAR.begin(), m_bufAR.end(), 0.0f);
    std::fill(m_bufBL.begin(), m_bufBL.end(), 0.0f);
    std::fill(m_bufBR.begin(), m_bufBR.end(), 0.0f);
    m_writePosA = m_readPosA = m_countA = 0;
    m_writePosB = m_readPosB = m_countB = 0;

    m_pilotBiquadX1 = m_pilotBiquadX2 = 0.0f;
    m_pilotBiquadY1 = m_pilotBiquadY2 = 0.0f;

    m_pilotToneRms = 0.0f;
    m_silenceSampleCounter = 0;
    m_heartbeatAlive = true;
    m_failoverCount = 0;

    m_inCrossfade = false;
    m_crossfadeIndex = 0;
    m_midiPanicQueue.clear();

    m_crossfadeTotalSamples = static_cast<size_t>((m_config.crossfadeTimeMs / 1000.0f) * m_sampleRate);
    if (m_crossfadeTotalSamples < 4) m_crossfadeTotalSamples = 4;

    m_dropoutThresholdSamples = static_cast<size_t>((m_config.dropoutThresholdMs / 1000.0f) * m_sampleRate);
    if (m_dropoutThresholdSamples < 8) m_dropoutThresholdSamples = 8;
}

void JZLiveStagePlaybackRedundancyEngine::setConfiguration(const Configuration& config) {
    m_config = config;
    m_crossfadeTotalSamples = static_cast<size_t>((m_config.crossfadeTimeMs / 1000.0f) * m_sampleRate);
    if (m_crossfadeTotalSamples < 4) m_crossfadeTotalSamples = 4;

    m_dropoutThresholdSamples = static_cast<size_t>((m_config.dropoutThresholdMs / 1000.0f) * m_sampleRate);
    if (m_dropoutThresholdSamples < 8) m_dropoutThresholdSamples = 8;

    updateFilter();
}

void JZLiveStagePlaybackRedundancyEngine::updateFilter() {
    float f0 = m_config.pilotToneFreqHz;
    float Q = 8.0f;
    float w0 = 2.0f * static_cast<float>(M_PI) * (f0 / m_sampleRate);
    float cos_w0 = std::cos(w0);
    float sin_w0 = std::sin(w0);
    float alpha = sin_w0 / (2.0f * Q);

    float a0 = 1.0f + alpha;
    m_pilotB0 = alpha / a0;
    m_pilotB1 = 0.0f;
    m_pilotB2 = -alpha / a0;
    m_pilotA1 = (-2.0f * cos_w0) / a0;
    m_pilotA2 = (1.0f - alpha) / a0;
}

void JZLiveStagePlaybackRedundancyEngine::ingestRigAudio(StageRigId rig,
                                                        const float* inL, const float* inR,
                                                        size_t numSamples) {
    if (!inL || numSamples == 0) return;

    std::vector<float>& bufL = (rig == StageRigId::PRIMARY_RIG_A) ? m_bufAL : m_bufBL;
    std::vector<float>& bufR = (rig == StageRigId::PRIMARY_RIG_A) ? m_bufAR : m_bufBR;
    size_t& writePos = (rig == StageRigId::PRIMARY_RIG_A) ? m_writePosA : m_writePosB;
    size_t& readPos = (rig == StageRigId::PRIMARY_RIG_A) ? m_readPosA : m_readPosB;
    size_t& count = (rig == StageRigId::PRIMARY_RIG_A) ? m_countA : m_countB;

    for (size_t i = 0; i < numSamples; ++i) {
        if (count >= BUFFER_CAPACITY) {
            readPos = (readPos + 1) % BUFFER_CAPACITY;
            count--;
        }
        bufL[writePos] = inL[i];
        bufR[writePos] = inR ? inR[i] : inL[i];
        writePos = (writePos + 1) % BUFFER_CAPACITY;
        count++;
    }
}

void JZLiveStagePlaybackRedundancyEngine::ingestHeartbeatTone(const float* pilotSamples, size_t numSamples) {
    if (!pilotSamples || numSamples == 0) return;

    float sumSq = 0.0f;
    for (size_t i = 0; i < numSamples; ++i) {
        float x = pilotSamples[i];
        float y = m_pilotB0 * x + m_pilotB1 * m_pilotB2 + m_pilotB2 * m_pilotBiquadX2
                  - m_pilotA1 * m_pilotBiquadY1 - m_pilotA2 * m_pilotBiquadY2;

        m_pilotBiquadX2 = m_pilotBiquadX1;
        m_pilotBiquadX1 = x;
        m_pilotBiquadY2 = m_pilotBiquadY1;
        m_pilotBiquadY1 = y;

        sumSq += y * y;
    }

    m_pilotToneRms = std::sqrt(sumSq / static_cast<float>(numSamples));

    if (m_pilotToneRms > 0.04f) {
        m_silenceSampleCounter = 0;
        m_heartbeatAlive = true;
    } else {
        m_silenceSampleCounter += numSamples;
        if (m_silenceSampleCounter >= m_dropoutThresholdSamples) {
            m_heartbeatAlive = false;
            if (m_config.autoFailoverEnabled && m_activeRig == StageRigId::PRIMARY_RIG_A) {
                triggerFailover(StageRigId::BACKUP_RIG_B);
            }
        }
    }
}

void JZLiveStagePlaybackRedundancyEngine::triggerFailover(StageRigId targetRig) {
    if (m_activeRig != targetRig) {
        m_previousRig = m_activeRig;
        m_activeRig = targetRig;
        m_inCrossfade = true;
        m_crossfadeIndex = 0;
        m_failoverCount++;

        if (m_config.sendMidiPanicOnSwitch) {
            generateMidiPanicFlush();
        }
    }
}

void JZLiveStagePlaybackRedundancyEngine::forceRigSwitch(StageRigId rig) {
    triggerFailover(rig);
}

void JZLiveStagePlaybackRedundancyEngine::generateMidiPanicFlush() {
    m_midiPanicQueue.clear();
    // Emit CC 120 (All Sound Off), CC 123 (All Notes Off), and Pitch Bend center across all 16 MIDI channels
    for (uint8_t ch = 0; ch < 16; ++ch) {
        m_midiPanicQueue.push_back({ static_cast<uint8_t>(0xB0 | ch), 120, 0 }); // All Sound Off
        m_midiPanicQueue.push_back({ static_cast<uint8_t>(0xB0 | ch), 123, 0 }); // All Notes Off
        m_midiPanicQueue.push_back({ static_cast<uint8_t>(0xE0 | ch), 0, 64 });  // Pitch Bend Reset
    }
}

std::vector<JZLiveStagePlaybackRedundancyEngine::MidiMessage>
JZLiveStagePlaybackRedundancyEngine::pollMidiPanicEvents() {
    auto events = m_midiPanicQueue;
    m_midiPanicQueue.clear();
    return events;
}

void JZLiveStagePlaybackRedundancyEngine::processMasterOutput(float* outL, float* outR, size_t numSamples) {
    if (!outL || numSamples == 0) return;

    std::vector<float>& actL = (m_activeRig == StageRigId::PRIMARY_RIG_A) ? m_bufAL : m_bufBL;
    std::vector<float>& actR = (m_activeRig == StageRigId::PRIMARY_RIG_A) ? m_bufAR : m_bufBR;
    size_t& actRead = (m_activeRig == StageRigId::PRIMARY_RIG_A) ? m_readPosA : m_readPosB;
    size_t& actCount = (m_activeRig == StageRigId::PRIMARY_RIG_A) ? m_countA : m_countB;

    std::vector<float>& prevL = (m_previousRig == StageRigId::PRIMARY_RIG_A) ? m_bufAL : m_bufBL;
    std::vector<float>& prevR = (m_previousRig == StageRigId::PRIMARY_RIG_A) ? m_bufAR : m_bufBR;
    size_t& prevRead = (m_previousRig == StageRigId::PRIMARY_RIG_A) ? m_readPosA : m_readPosB;
    size_t& prevCount = (m_previousRig == StageRigId::PRIMARY_RIG_A) ? m_countA : m_countB;

    for (size_t i = 0; i < numSamples; ++i) {
        float sampleL = 0.0f;
        float sampleR = 0.0f;

        if (actCount > 0) {
            sampleL = actL[actRead];
            sampleR = actR[actRead];
            actRead = (actRead + 1) % BUFFER_CAPACITY;
            actCount--;
        }

        if (m_inCrossfade) {
            float fadeProgress = static_cast<float>(m_crossfadeIndex) / static_cast<float>(m_crossfadeTotalSamples);
            float prevSampleL = 0.0f;
            float prevSampleR = 0.0f;

            if (prevCount > 0) {
                prevSampleL = prevL[prevRead];
                prevSampleR = prevR[prevRead];
                prevRead = (prevRead + 1) % BUFFER_CAPACITY;
                prevCount--;
            }

            sampleL = prevSampleL * (1.0f - fadeProgress) + sampleL * fadeProgress;
            sampleR = prevSampleR * (1.0f - fadeProgress) + sampleR * fadeProgress;

            m_crossfadeIndex++;
            if (m_crossfadeIndex >= m_crossfadeTotalSamples) {
                m_inCrossfade = false;
            }
        }

        outL[i] = sampleL;
        if (outR) outR[i] = sampleR;
    }
}

} // namespace Jazz
