//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZVirtualStudioMonitorSimulator.h - Binaural Virtual Control Room Simulator
//
// World-class virtual studio monitoring for headphones (Slate VSX style):
// Iconic acoustic control rooms (Abbey Road 3, Ocean Way, Mastering Lab, Car),
// speaker models (NS-10M, Genelec 3-Way, Soffit Mains, Auratone 5C Mono),
// 60-degree stereo cross-feed with Interaural Time (ITD) / Level (ILD) delays.
//*****************************************************************************

#ifndef JAZZ_VIRTUAL_STUDIO_MONITOR_SIMULATOR_H
#define JAZZ_VIRTUAL_STUDIO_MONITOR_SIMULATOR_H

#include <vector>
#include <string>
#include <cstdint>

namespace Jazz {

enum class VirtualStudioRoom {
    ABBEY_ROAD_STUDIO_3,      // Iconic British control room acoustic
    OCEAN_WAY_NASHVILLE,      // Warm, open high-end American acoustic space
    MASTERING_LAB_HOLLYWOOD,  // Surgical, flat reference mastering room
    CAR_AUDIO_CHECK,          // Modern consumer vehicle cabin acoustic
    BOOMBOX_AURATONE_MONO     // Auratone 5C mixcube bandwidth-limited mono
};

enum class VirtualSpeakerModel {
    NEARFIELD_NS10M,          // Classic mid-forward white-cone nearfields
    MIDFIELD_GENELEC_COAXIAL, // High-resolution flat point-source 3-ways
    FARFIELD_SOFFIT_DUAL15,   // Massive wall-mounted dual-15" main monitors
    MONO_MIXCUBE              // Single-driver 5" mono check
};

class JZVirtualStudioMonitorSimulator {
public:
    JZVirtualStudioMonitorSimulator(float sampleRate = 48000.0f,
                                    VirtualStudioRoom room = VirtualStudioRoom::ABBEY_ROAD_STUDIO_3,
                                    VirtualSpeakerModel speaker = VirtualSpeakerModel::NEARFIELD_NS10M);
    ~JZVirtualStudioMonitorSimulator() = default;

    void setSampleRate(float sampleRate);
    float getSampleRate() const { return m_sampleRate; }

    void setRoomProfile(VirtualStudioRoom room);
    VirtualStudioRoom getRoomProfile() const { return m_room; }

    void setSpeakerModel(VirtualSpeakerModel speaker);
    VirtualSpeakerModel getSpeakerModel() const { return m_speaker; }

    // Acoustic Controls
    void setCrossfeedAmount(float percent); // 0% (standard headphones) to 100% (full 60-deg cross-feed)
    float getCrossfeedAmount() const { return m_crossfeedAmount; }

    void setRoomAmbienceAmount(float percent); // Early reflection density
    float getRoomAmbienceAmount() const { return m_roomAmbienceAmount; }

    void setBypass(bool bypass) { m_bypass = bypass; }
    bool isBypassed() const { return m_bypass; }

    // Real-Time Processing: Processes stereo mix into binaural virtual studio headphone output
    void process(const float* inL, const float* inR,
                 float* outL, float* outR, size_t numSamples);

    void reset();

private:
    float m_sampleRate;
    VirtualStudioRoom m_room;
    VirtualSpeakerModel m_speaker;
    float m_crossfeedAmount;
    float m_roomAmbienceAmount;
    bool m_bypass;

    // Cross-feed ITD ring buffers (Interaural Time Difference ~220us)
    std::vector<float> m_crossfeedDelayL;
    std::vector<float> m_crossfeedDelayR;
    size_t m_crossWriteIdx;
    size_t m_itdSamples;

    // Head-shadow ILD filter states
    float m_headShadowL, m_headShadowR;

    // Early room reflection delay taps
    std::vector<float> m_roomReflBufL, m_roomReflBufR;
    size_t m_reflWriteIdx;

    void updateAcousticParameters();
};

} // namespace Jazz

#endif // JAZZ_VIRTUAL_STUDIO_MONITOR_SIMULATOR_H
