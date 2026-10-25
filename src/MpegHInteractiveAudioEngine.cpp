//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// JZMpegHInteractiveAudioEngine.cpp - MPEG-H 3D Interactive Broadcast Authoring
//*****************************************************************************

#include "MpegHInteractiveAudioEngine.h"
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace Jazz {

JZMpegHInteractiveAudioEngine::JZMpegHInteractiveAudioEngine(const std::string& programTitle)
    : m_programTitle(programTitle)
    , m_nextElementId(1)
{
}

int JZMpegHInteractiveAudioEngine::addAudioElement(const std::string& name, MpegHElementType type,
                                                  const std::string& langCode,
                                                  float minGainDb, float maxGainDb) {
    int id = m_nextElementId++;
    MpegHAudioElement elem;
    elem.elementId = id;
    elem.elementName = name;
    elem.type = type;
    elem.languageCode = langCode;
    elem.defaultGainDb = 0.0f;
    elem.minInteractiveGainDb = std::min(minGainDb, 0.0f);
    elem.maxInteractiveGainDb = std::max(maxGainDb, 0.0f);
    elem.allowUserPositioning = (type == MpegHElementType::COMMENTARY_ALT_LANG);
    elem.azimuthDeg = 0.0f;
    elem.elevationDeg = 0.0f;

    m_elements[id] = elem;
    m_viewerGainOffsets[id] = 0.0f;
    return id;
}

bool JZMpegHInteractiveAudioEngine::removeAudioElement(int elementId) {
    m_viewerGainOffsets.erase(elementId);
    return m_elements.erase(elementId) > 0;
}

const MpegHAudioElement* JZMpegHInteractiveAudioEngine::getElement(int elementId) const {
    auto it = m_elements.find(elementId);
    if (it != m_elements.end()) {
        return &it->second;
    }
    return nullptr;
}

bool JZMpegHInteractiveAudioEngine::setElementGainRange(int elementId, float minDb, float maxDb) {
    auto it = m_elements.find(elementId);
    if (it != m_elements.end()) {
        it->second.minInteractiveGainDb = std::min(minDb, 0.0f);
        it->second.maxInteractiveGainDb = std::max(maxDb, 0.0f);
        return true;
    }
    return false;
}

bool JZMpegHInteractiveAudioEngine::setElementPosition(int elementId, float azimuthDeg, float elevationDeg) {
    auto it = m_elements.find(elementId);
    if (it != m_elements.end()) {
        it->second.azimuthDeg = std::clamp(azimuthDeg, -180.0f, 180.0f);
        it->second.elevationDeg = std::clamp(elevationDeg, -90.0f, 90.0f);
        return true;
    }
    return false;
}

bool JZMpegHInteractiveAudioEngine::setElementLanguage(int elementId, const std::string& langCode) {
    auto it = m_elements.find(elementId);
    if (it != m_elements.end()) {
        it->second.languageCode = langCode;
        return true;
    }
    return false;
}

void JZMpegHInteractiveAudioEngine::simulateViewerGain(int elementId, float userGainDb) {
    auto it = m_elements.find(elementId);
    if (it != m_elements.end()) {
        float clamped = std::clamp(userGainDb, it->second.minInteractiveGainDb, it->second.maxInteractiveGainDb);
        m_viewerGainOffsets[elementId] = clamped;
    }
}

float JZMpegHInteractiveAudioEngine::getViewerGain(int elementId) const {
    auto it = m_viewerGainOffsets.find(elementId);
    if (it != m_viewerGainOffsets.end()) {
        return it->second;
    }
    return 0.0f;
}

std::string JZMpegHInteractiveAudioEngine::generateMpegHMetadataXml() const {
    std::ostringstream ss;
    ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
       << "<MpegHAuthoringMetadata version=\"1.3\" standard=\"ISO/IEC_23008-3\">\n"
       << "  <Program title=\"" << m_programTitle << "\">\n"
       << "    <AudioScene>\n";

    for (const auto& pair : m_elements) {
        const auto& el = pair.second;
        std::string typeStr;
        switch (el.type) {
            case MpegHElementType::DIALOGUE_PRIMARY:    typeStr = "Dialogue"; break;
            case MpegHElementType::COMMENTARY_ALT_LANG: typeStr = "Commentary"; break;
            case MpegHElementType::AMBIENCE_BED:        typeStr = "Ambience"; break;
            case MpegHElementType::MUSIC_SOUNDTRACK:     typeStr = "Music"; break;
        }

        ss << "      <AudioElement id=\"" << el.elementId << "\" name=\"" << el.elementName
           << "\" type=\"" << typeStr << "\" lang=\"" << el.languageCode << "\">\n"
           << "        <Interactivity allowed=\"true\" minGainDb=\"" << std::fixed << std::setprecision(1) << el.minInteractiveGainDb
           << "\" maxGainDb=\"" << el.maxInteractiveGainDb << "\"/>\n"
           << "        <Position azimuth=\"" << el.azimuthDeg << "\" elevation=\"" << el.elevationDeg << "\"/>\n"
           << "      </AudioElement>\n";
    }

    ss << "    </AudioScene>\n"
       << "  </Program>\n"
       << "</MpegHAuthoringMetadata>\n";

    return ss.str();
}

void JZMpegHInteractiveAudioEngine::reset() {
    m_elements.clear();
    m_viewerGainOffsets.clear();
    m_nextElementId = 1;
}

} // namespace Jazz
