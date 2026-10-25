//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZSmpteVideoSyncEngine.h - Film Scoring Video Genlock & SMPTE Chase Sync
//
// Hollywood film scoring, broadcast television, video reference genlock,
// PLL jitter-smoothed chase synchronization, frame-accurate EDL scene-cut
// markers, and 23.976/24/25/29.97DF/30/59.94DF cinema timecode translation.
//*****************************************************************************

#ifndef JAZZ_SMPTE_VIDEO_SYNC_ENGINE_H
#define JAZZ_SMPTE_VIDEO_SYNC_ENGINE_H

#include <vector>
#include <string>
#include <cstdint>

namespace Jazz {

enum class VideoFrameRate {
    FPS_23_976,   // 23.976 fps (High-Definition Film / Netflix)
    FPS_24,       // 24.0 fps (Standard Theatrical Cinema)
    FPS_25,       // 25.0 fps (PAL / EBU Broadcast Europe)
    FPS_29_97_DF, // 29.97 fps (NTSC Broadcast Drop-Frame)
    FPS_29_97_NDF,// 29.97 fps (NTSC Non-Drop-Frame)
    FPS_30,       // 30.0 fps (High-Speed / Music Video)
    FPS_59_94_DF, // 59.94 fps (UHD / 4K / 8K Broadcast Drop-Frame)
    FPS_60        // 60.0 fps (Standard 60Hz)
};

enum class VideoSyncLockState {
    UNLOCKED,
    LOCKING,
    LOCKED_CHASE,
    FREEWHEELING
};

struct VideoTimecode {
    int hours;
    int minutes;
    int seconds;
    int frames;
    int subframe; // 0 to 99 (1/100th of a frame)

    bool operator==(const VideoTimecode& o) const {
        return hours == o.hours && minutes == o.minutes &&
               seconds == o.seconds && frames == o.frames;
    }
};

struct SceneCutMarker {
    int id;
    std::string name;
    VideoTimecode timecode;
    std::string transitionType; // "HardCut", "Dissolve", "Fade"
};

class JZSmpteVideoSyncEngine {
public:
    JZSmpteVideoSyncEngine(VideoFrameRate rate = VideoFrameRate::FPS_24, float sampleRate = 48000.0f);
    ~JZSmpteVideoSyncEngine() = default;

    // Rate & Sample configuration
    void setFrameRate(VideoFrameRate rate);
    VideoFrameRate getFrameRate() const { return m_frameRate; }
    double getFrameRateValue() const;
    bool isDropFrame() const;

    void setAudioSampleRate(float sampleRate);
    float getAudioSampleRate() const { return m_sampleRate; }

    // Timecode <-> Total Frames / Audio Samples Conversion
    uint64_t timecodeToTotalFrames(const VideoTimecode& tc) const;
    VideoTimecode totalFramesToTimecode(uint64_t totalFrames) const;
    uint64_t timecodeToSampleIndex(const VideoTimecode& tc) const;
    VideoTimecode sampleIndexToTimecode(uint64_t sampleIndex) const;

    std::string formatTimecodeString(const VideoTimecode& tc) const;

    // Video Genlock & PLL Chase Sync Engine
    void feedIncomingVideoPulse(uint64_t audioSampleIndex);
    VideoSyncLockState getLockState() const { return m_lockState; }
    float getPllPhaseJitterSubframes() const { return m_pllPhaseJitter; }
    void setFreewheelMaxFrames(int maxFrames) { m_freewheelMaxFrames = maxFrames; }
    int getFreewheelMaxFrames() const { return m_freewheelMaxFrames; }

    // EDL / Scene Cut Markers
    int addSceneCutMarker(const std::string& name, const VideoTimecode& tc, const std::string& transition = "HardCut");
    const std::vector<SceneCutMarker>& getSceneCuts() const { return m_sceneCuts; }
    bool findNearestSceneCut(const VideoTimecode& currentTc, SceneCutMarker& outNearest, int searchWindowFrames = 120) const;

    void reset();

private:
    VideoFrameRate m_frameRate;
    float m_sampleRate;
    VideoSyncLockState m_lockState;
    float m_pllPhaseJitter;
    int m_freewheelMaxFrames;
    int m_freewheelCounter;

    uint64_t m_lastPulseSample;
    double m_nominalSamplesPerFrame;
    int m_consecutiveLockedPulses;

    std::vector<SceneCutMarker> m_sceneCuts;
    int m_nextMarkerId;
};

} // namespace Jazz

#endif // JAZZ_SMPTE_VIDEO_SYNC_ENGINE_H
