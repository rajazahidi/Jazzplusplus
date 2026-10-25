//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZSmpteVideoSyncEngine.cpp - Film Scoring Video Genlock & SMPTE Chase Sync
//*****************************************************************************

#include "SmpteVideoSyncEngine.h"
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <algorithm>

namespace Jazz {

JZSmpteVideoSyncEngine::JZSmpteVideoSyncEngine(VideoFrameRate rate, float sampleRate)
    : m_frameRate(rate)
    , m_sampleRate(sampleRate > 1000.0f ? sampleRate : 48000.0f)
    , m_lockState(VideoSyncLockState::UNLOCKED)
    , m_pllPhaseJitter(0.0f)
    , m_freewheelMaxFrames(8)
    , m_freewheelCounter(0)
    , m_lastPulseSample(0)
    , m_consecutiveLockedPulses(0)
    , m_nextMarkerId(1)
{
    setFrameRate(rate);
    reset();
}

void JZSmpteVideoSyncEngine::reset() {
    m_lockState = VideoSyncLockState::UNLOCKED;
    m_pllPhaseJitter = 0.0f;
    m_freewheelCounter = 0;
    m_lastPulseSample = 0;
    m_consecutiveLockedPulses = 0;
}

double JZSmpteVideoSyncEngine::getFrameRateValue() const {
    switch (m_frameRate) {
        case VideoFrameRate::FPS_23_976:   return 24000.0 / 1001.0; // ~23.97602
        case VideoFrameRate::FPS_24:       return 24.0;
        case VideoFrameRate::FPS_25:       return 25.0;
        case VideoFrameRate::FPS_29_97_DF:  return 30000.0 / 1001.0; // ~29.97003
        case VideoFrameRate::FPS_29_97_NDF: return 30000.0 / 1001.0;
        case VideoFrameRate::FPS_30:       return 30.0;
        case VideoFrameRate::FPS_59_94_DF:  return 60000.0 / 1001.0; // ~59.94006
        case VideoFrameRate::FPS_60:       return 60.0;
    }
    return 24.0;
}

bool JZSmpteVideoSyncEngine::isDropFrame() const {
    return (m_frameRate == VideoFrameRate::FPS_29_97_DF ||
            m_frameRate == VideoFrameRate::FPS_59_94_DF);
}

void JZSmpteVideoSyncEngine::setFrameRate(VideoFrameRate rate) {
    m_frameRate = rate;
    m_nominalSamplesPerFrame = static_cast<double>(m_sampleRate) / getFrameRateValue();
    reset();
}

void JZSmpteVideoSyncEngine::setAudioSampleRate(float sampleRate) {
    if (sampleRate > 1000.0f) {
        m_sampleRate = sampleRate;
        m_nominalSamplesPerFrame = static_cast<double>(m_sampleRate) / getFrameRateValue();
    }
}

uint64_t JZSmpteVideoSyncEngine::timecodeToTotalFrames(const VideoTimecode& tc) const {
    if (isDropFrame()) {
        int dropFrames = (m_frameRate == VideoFrameRate::FPS_59_94_DF) ? 4 : 2;
        int framesPerMin = (m_frameRate == VideoFrameRate::FPS_59_94_DF) ? 60 : 30;

        uint64_t totalMinutes = static_cast<uint64_t>(tc.hours) * 60 + tc.minutes;
        uint64_t totalFrames = ((totalMinutes * 60) + tc.seconds) * framesPerMin + tc.frames;

        // Subtract dropped frame counts
        uint64_t droppedCount = dropFrames * (totalMinutes - (totalMinutes / 10));
        return (totalFrames >= droppedCount) ? (totalFrames - droppedCount) : 0;
    }

    int nominalFps = static_cast<int>(std::round(getFrameRateValue()));
    uint64_t totalSec = static_cast<uint64_t>(tc.hours) * 3600 +
                        static_cast<uint64_t>(tc.minutes) * 60 +
                        static_cast<uint64_t>(tc.seconds);
    return totalSec * nominalFps + tc.frames;
}

VideoTimecode JZSmpteVideoSyncEngine::totalFramesToTimecode(uint64_t totalFrames) const {
    VideoTimecode tc = {0, 0, 0, 0, 0};

    if (isDropFrame()) {
        int dropFrames = (m_frameRate == VideoFrameRate::FPS_59_94_DF) ? 4 : 2;
        int framesPerMin = (m_frameRate == VideoFrameRate::FPS_59_94_DF) ? 60 : 30;
        int framesPer10Min = framesPerMin * 60 * 10 - dropFrames * 9; // e.g. 17982 for 29.97 DF
        int framesPerStandardMin = framesPerMin * 60 - dropFrames;     // e.g. 1798 for 29.97 DF

        uint64_t d = totalFrames / framesPer10Min;
        uint64_t m = totalFrames % framesPer10Min;

        uint64_t adjustedFrames = totalFrames + dropFrames * 9 * d;
        if (m > static_cast<uint64_t>(dropFrames)) {
            adjustedFrames += dropFrames * ((m - dropFrames) / framesPerStandardMin);
        }

        tc.frames = static_cast<int>(adjustedFrames % framesPerMin);
        uint64_t totalSeconds = adjustedFrames / framesPerMin;
        tc.seconds = static_cast<int>(totalSeconds % 60);
        uint64_t totalMinutes = totalSeconds / 60;
        tc.minutes = static_cast<int>(totalMinutes % 60);
        tc.hours = static_cast<int>(totalMinutes / 60);
        return tc;
    }

    int nominalFps = static_cast<int>(std::round(getFrameRateValue()));
    tc.frames = static_cast<int>(totalFrames % nominalFps);
    uint64_t totalSec = totalFrames / nominalFps;
    tc.seconds = static_cast<int>(totalSec % 60);
    uint64_t totalMin = totalSec / 60;
    tc.minutes = static_cast<int>(totalMin % 60);
    tc.hours = static_cast<int>(totalMin / 60);
    return tc;
}

uint64_t JZSmpteVideoSyncEngine::timecodeToSampleIndex(const VideoTimecode& tc) const {
    uint64_t frames = timecodeToTotalFrames(tc);
    double frameDurationSec = 1.0 / getFrameRateValue();
    double totalSec = static_cast<double>(frames) * frameDurationSec +
                      (static_cast<double>(tc.subframe) / 100.0) * frameDurationSec;
    return static_cast<uint64_t>(std::round(totalSec * m_sampleRate));
}

VideoTimecode JZSmpteVideoSyncEngine::sampleIndexToTimecode(uint64_t sampleIndex) const {
    double totalSec = static_cast<double>(sampleIndex) / m_sampleRate;
    double fps = getFrameRateValue();
    double rawFrames = totalSec * fps;
    uint64_t intFrames = static_cast<uint64_t>(std::floor(rawFrames));
    double subframeFrac = rawFrames - static_cast<double>(intFrames);

    VideoTimecode tc = totalFramesToTimecode(intFrames);
    tc.subframe = std::clamp(static_cast<int>(std::round(subframeFrac * 100.0)), 0, 99);
    return tc;
}

std::string JZSmpteVideoSyncEngine::formatTimecodeString(const VideoTimecode& tc) const {
    std::ostringstream ss;
    char sep = isDropFrame() ? ';' : ':';
    ss << std::setfill('0') << std::setw(2) << tc.hours << ":"
       << std::setfill('0') << std::setw(2) << tc.minutes << ":"
       << std::setfill('0') << std::setw(2) << tc.seconds << sep
       << std::setfill('0') << std::setw(2) << tc.frames << "."
       << std::setfill('0') << std::setw(2) << tc.subframe;
    return ss.str();
}

void JZSmpteVideoSyncEngine::feedIncomingVideoPulse(uint64_t audioSampleIndex) {
    if (m_lastPulseSample == 0) {
        m_lastPulseSample = audioSampleIndex;
        m_lockState = VideoSyncLockState::LOCKING;
        m_consecutiveLockedPulses = 1;
        return;
    }

    uint64_t deltaSamples = audioSampleIndex - m_lastPulseSample;
    m_lastPulseSample = audioSampleIndex;

    double expectedSamples = m_nominalSamplesPerFrame;
    double sampleDiff = static_cast<double>(deltaSamples) - expectedSamples;
    double jitterRatio = std::abs(sampleDiff) / expectedSamples;

    // Convert jitter into subframes (1 frame = 100 subframes)
    m_pllPhaseJitter = static_cast<float>(jitterRatio * 100.0);

    if (jitterRatio < 0.05) { // within 5% frame interval jitter tolerance
        m_consecutiveLockedPulses++;
        m_freewheelCounter = 0;
        if (m_consecutiveLockedPulses >= 3) {
            m_lockState = VideoSyncLockState::LOCKED_CHASE;
        }
    } else {
        if (m_lockState == VideoSyncLockState::LOCKED_CHASE) {
            m_freewheelCounter++;
            if (m_freewheelCounter <= m_freewheelMaxFrames) {
                m_lockState = VideoSyncLockState::FREEWHEELING;
            } else {
                m_lockState = VideoSyncLockState::UNLOCKED;
                m_consecutiveLockedPulses = 0;
            }
        } else {
            m_lockState = VideoSyncLockState::UNLOCKED;
            m_consecutiveLockedPulses = 0;
        }
    }
}

int JZSmpteVideoSyncEngine::addSceneCutMarker(const std::string& name, const VideoTimecode& tc, const std::string& transition) {
    SceneCutMarker marker;
    marker.id = m_nextMarkerId++;
    marker.name = name;
    marker.timecode = tc;
    marker.transitionType = transition;

    m_sceneCuts.push_back(marker);
    return marker.id;
}

bool JZSmpteVideoSyncEngine::findNearestSceneCut(const VideoTimecode& currentTc, SceneCutMarker& outNearest, int searchWindowFrames) const {
    if (m_sceneCuts.empty()) return false;

    uint64_t currentFrame = timecodeToTotalFrames(currentTc);
    uint64_t minDiff = UINT64_MAX;
    const SceneCutMarker* bestMatch = nullptr;

    for (const auto& marker : m_sceneCuts) {
        uint64_t mFrame = timecodeToTotalFrames(marker.timecode);
        uint64_t diff = (currentFrame >= mFrame) ? (currentFrame - mFrame) : (mFrame - currentFrame);
        if (diff <= static_cast<uint64_t>(searchWindowFrames) && diff < minDiff) {
            minDiff = diff;
            bestMatch = &marker;
        }
    }

    if (bestMatch) {
        outNearest = *bestMatch;
        return true;
    }
    return false;
}

} // namespace Jazz
