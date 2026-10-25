//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZMpegHInteractiveAudioEngine.h - MPEG-H 3D Interactive Broadcast Authoring
//
// Next-Generation broadcast audio authoring (ISO/IEC 23008-3 / ATSC 3.0 standard):
// Interactive audio elements (Dialogue, Commentary, Ambience, Music),
// viewer accessibility dialogue gain limits (-6dB to +6dB), language track
// switching, and standardized MPEG-H production metadata packaging.
//*****************************************************************************

#ifndef JAZZ_MPEG_H_INTERACTIVE_AUDIO_ENGINE_H
#define JAZZ_MPEG_H_INTERACTIVE_AUDIO_ENGINE_H

#include <vector>
#include <string>
#include <map>
#include <cstdint>

namespace Jazz {

enum class MpegHElementType {
    DIALOGUE_PRIMARY,    // Main speech channel (accessibility boost)
    COMMENTARY_ALT_LANG, // Switchable secondary language audio
    AMBIENCE_BED,        // Stadium / room background sound field
    MUSIC_SOUNDTRACK     // Music cues / theme song
};

struct MpegHAudioElement {
    int elementId;
    std::string elementName;
    MpegHElementType type;
    std::string languageCode;     // e.g. "eng", "zlm", "spa", "deu"
    float defaultGainDb;
    float minInteractiveGainDb;   // e.g. -6.0 dB
    float maxInteractiveGainDb;   // e.g. +6.0 dB
    bool allowUserPositioning;    // 3D spatial panning interactivity
    float azimuthDeg;
    float elevationDeg;
};

class JZMpegHInteractiveAudioEngine {
public:
    JZMpegHInteractiveAudioEngine(const std::string& programTitle = "MainBroadcast");
    ~JZMpegHInteractiveAudioEngine() = default;

    void setProgramTitle(const std::string& title) { m_programTitle = title; }
    std::string getProgramTitle() const { return m_programTitle; }

    // Audio Element Authoring
    int addAudioElement(const std::string& name, MpegHElementType type,
                        const std::string& langCode = "eng",
                        float minGainDb = -6.0f, float maxGainDb = 6.0f);

    bool removeAudioElement(int elementId);
    size_t getElementCount() const { return m_elements.size(); }
    const MpegHAudioElement* getElement(int elementId) const;

    // Element Controls
    bool setElementGainRange(int elementId, float minDb, float maxDb);
    bool setElementPosition(int elementId, float azimuthDeg, float elevationDeg);
    bool setElementLanguage(int elementId, const std::string& langCode);

    // Interactive Viewer Simulation
    // Simulates an end-user adjusting dialogue or switching language on their TV
    void simulateViewerGain(int elementId, float userGainDb);
    float getViewerGain(int elementId) const;

    // Metadata Export: Generates ISO/IEC 23008-3 compliant MPEG-H Authoring XML
    std::string generateMpegHMetadataXml() const;

    void reset();

private:
    std::string m_programTitle;
    int m_nextElementId;
    std::map<int, MpegHAudioElement> m_elements;
    std::map<int, float> m_viewerGainOffsets;
};

} // namespace Jazz

#endif // JAZZ_MPEG_H_INTERACTIVE_AUDIO_ENGINE_H
