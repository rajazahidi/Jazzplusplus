//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZAutonomousMixAssistantEngine.cpp - Autonomous AI Multi-Track Mix Assistant
//*****************************************************************************

#include "AutonomousMixAssistantEngine.h"
#include <algorithm>
#include <cmath>

namespace Jazz {

JZAutonomousMixAssistantEngine::JZAutonomousMixAssistantEngine(float sampleRate)
    : m_sampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
    , m_nextTrackId(1)
{
}

void JZAutonomousMixAssistantEngine::setSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        m_sampleRate = sampleRate;
    }
}

int JZAutonomousMixAssistantEngine::registerTrack(const std::string& name, MixTrackRole role) {
    int id = m_nextTrackId++;
    TrackMixProfile prof;
    prof.trackId = id;
    prof.trackName = name;
    prof.role = role;
    prof.currentRmsDb = -60.0f;
    prof.currentPeakDb = -60.0f;
    prof.recommendedFaderTrimDb = 0.0f;
    prof.subBassEnergy = 0.0f;
    prof.lowMidEnergy = 0.0f;
    prof.vocalPresenceEnergy = 0.0f;
    prof.highAirEnergy = 0.0f;

    m_tracks[id] = prof;
    return id;
}

bool JZAutonomousMixAssistantEngine::unregisterTrack(int trackId) {
    return m_tracks.erase(trackId) > 0;
}

const TrackMixProfile* JZAutonomousMixAssistantEngine::getTrackProfile(int trackId) const {
    auto it = m_tracks.find(trackId);
    if (it != m_tracks.end()) {
        return &it->second;
    }
    return nullptr;
}

void JZAutonomousMixAssistantEngine::ingestTrackAudio(int trackId, const float* audioData, size_t numSamples) {
    auto it = m_tracks.find(trackId);
    if (it == m_tracks.end() || !audioData || numSamples == 0) return;

    TrackMixProfile& prof = it->second;

    double sumSq = 0.0;
    float peak = 0.0f;
    double subBassSum = 0.0;
    double lowMidSum = 0.0;
    double vocalPresSum = 0.0;
    double highAirSum = 0.0;

    // Filter states for energy estimation
    float lp60 = 0.0f, lp300 = 0.0f, lp2500 = 0.0f, lp8000 = 0.0f;

    for (size_t i = 0; i < numSamples; ++i) {
        float x = audioData[i];
        float a = std::abs(x);
        sumSq += x * x;
        if (a > peak) peak = a;

        // Multi-band leaky integrators
        lp60 += 0.01f * (x - lp60);
        lp300 += 0.04f * (x - lp300);
        lp2500 += 0.25f * (x - lp2500);
        lp8000 += 0.60f * (x - lp8000);

        subBassSum += std::abs(lp60);
        lowMidSum += std::abs(lp300 - lp60);
        vocalPresSum += std::abs(lp2500 - lp300);
        highAirSum += std::abs(x - lp8000);
    }

    double rms = std::sqrt(sumSq / static_cast<double>(numSamples));
    prof.currentRmsDb = (rms > 1e-6) ? static_cast<float>(20.0 * std::log10(rms)) : -60.0f;
    prof.currentPeakDb = (peak > 1e-6f) ? static_cast<float>(20.0 * std::log10(peak)) : -60.0f;

    prof.subBassEnergy = static_cast<float>(subBassSum / numSamples);
    prof.lowMidEnergy = static_cast<float>(lowMidSum / numSamples);
    prof.vocalPresenceEnergy = static_cast<float>(vocalPresSum / numSamples);
    prof.highAirEnergy = static_cast<float>(highAirSum / numSamples);
}

void JZAutonomousMixAssistantEngine::analyzeMixCollisions() {
    m_collisions.clear();

    // Scan track pairs for well-known industry masking clashes
    for (auto& pairA : m_tracks) {
        TrackMixProfile& trkA = pairA.second;

        for (auto& pairB : m_tracks) {
            TrackMixProfile& trkB = pairB.second;
            if (trkA.trackId >= trkB.trackId) continue;

            // Collision Scenario 1: Kick Drum vs Bass Instrument (60Hz - 80Hz clash)
            if ((trkA.role == MixTrackRole::KICK_DRUM && trkB.role == MixTrackRole::BASS_INSTRUMENT) ||
                (trkB.role == MixTrackRole::KICK_DRUM && trkA.role == MixTrackRole::BASS_INSTRUMENT)) {

                TrackMixProfile& kick = (trkA.role == MixTrackRole::KICK_DRUM) ? trkA : trkB;
                TrackMixProfile& bass = (trkA.role == MixTrackRole::BASS_INSTRUMENT) ? trkA : trkB;

                if (kick.subBassEnergy > 0.02f && bass.subBassEnergy > 0.02f) {
                    SpectralMaskingCollision col;
                    col.dominantTrackId = kick.trackId;
                    col.dominantTrackName = kick.trackName;
                    col.maskedTrackId = bass.trackId;
                    col.maskedTrackName = bass.trackName;
                    col.centerFrequencyHz = 65.0f;
                    col.maskingSeverity = std::clamp((bass.subBassEnergy / (kick.subBassEnergy + 1e-6f)) * 0.7f, 0.2f, 1.0f);
                    col.recommendedCarveDb = -3.5f * col.maskingSeverity;
                    col.recommendedQ = 2.8f;
                    m_collisions.push_back(col);
                }
            }

            // Collision Scenario 2: Lead Vocal vs Guitars/Synths (1.5kHz - 3.5kHz vocal intelligibility clash)
            if ((trkA.role == MixTrackRole::LEAD_VOCAL && trkB.role == MixTrackRole::GUITARS_SYNTHS) ||
                (trkB.role == MixTrackRole::LEAD_VOCAL && trkA.role == MixTrackRole::GUITARS_SYNTHS)) {

                TrackMixProfile& vocal = (trkA.role == MixTrackRole::LEAD_VOCAL) ? trkA : trkB;
                TrackMixProfile& guitar = (trkA.role == MixTrackRole::GUITARS_SYNTHS) ? trkA : trkB;

                if (vocal.vocalPresenceEnergy > 0.01f && guitar.vocalPresenceEnergy > 0.01f) {
                    SpectralMaskingCollision col;
                    col.dominantTrackId = vocal.trackId;
                    col.dominantTrackName = vocal.trackName;
                    col.maskedTrackId = guitar.trackId;
                    col.maskedTrackName = guitar.trackName;
                    col.centerFrequencyHz = 2400.0f;
                    col.maskingSeverity = std::clamp((guitar.vocalPresenceEnergy / (vocal.vocalPresenceEnergy + 1e-6f)) * 0.8f, 0.25f, 1.0f);
                    col.recommendedCarveDb = -2.8f * col.maskingSeverity;
                    col.recommendedQ = 2.2f;
                    m_collisions.push_back(col);
                }
            }
        }
    }
}

void JZAutonomousMixAssistantEngine::applyDynamicCarve(int trackId, float* audioData, size_t numSamples) {
    if (!audioData || numSamples == 0) return;

    for (const auto& col : m_collisions) {
        if (col.maskedTrackId == trackId) {
            // Apply recommended notch/carve filter at collision frequency
            float f0 = col.centerFrequencyHz;
            float q = col.recommendedQ;
            float gainDb = col.recommendedCarveDb;

            float A = std::pow(10.0f, gainDb / 40.0f);
            float w0 = 2.0f * 3.14159265f * f0 / m_sampleRate;
            float alpha = std::sin(w0) / (2.0f * q);

            float b0 = 1.0f + alpha * A;
            float b1 = -2.0f * std::cos(w0);
            float b2 = 1.0f - alpha * A;
            float a0 = 1.0f + alpha / A;
            float a1 = -2.0f * std::cos(w0);
            float a2 = 1.0f - alpha / A;

            float invA0 = 1.0f / a0;
            b0 *= invA0; b1 *= invA0; b2 *= invA0;
            a1 *= invA0; a2 *= invA0;

            float s1 = 0.0f, s2 = 0.0f;
            for (size_t i = 0; i < numSamples; ++i) {
                float x = audioData[i];
                float y = b0 * x + s1;
                s1 = b1 * x - a1 * y + s2;
                s2 = b2 * x - a2 * y;
                audioData[i] = y;
            }
        }
    }
}

void JZAutonomousMixAssistantEngine::reset() {
    m_collisions.clear();
}

} // namespace Jazz
