#include "AdmBwfSpatialMetadataEngine.h"
#include <algorithm>
#include <sstream>
#include <cstring>
#include <iomanip>

namespace Jazz {

JZAdmBwfSpatialMetadataEngine::JZAdmBwfSpatialMetadataEngine(const std::string& programmeTitle)
    : m_programmeTitle(programmeTitle)
    , m_nextObjectId(1)
{
}

uint32_t JZAdmBwfSpatialMetadataEngine::addBedChannel(const std::string& speakerLabel, int channelIndex)
{
    uint32_t id = m_nextObjectId++;
    AdmAudioObject obj;
    obj.objectId = id;
    obj.objectName = speakerLabel;
    obj.trackChannelIndex = channelIndex;
    obj.azimuthDeg = 0.0f;
    obj.elevationDeg = 0.0f;
    obj.distance = 1.0f;
    obj.spreadWidth = 0.0f;
    obj.binauralMode = BinauralRenderMode::ROOM_MID;
    obj.isBedChannel = true;

    m_objects[id] = obj;
    return id;
}

uint32_t JZAdmBwfSpatialMetadataEngine::addDynamicObject(
    const std::string& objectName, int channelIndex,
    float azimuth, float elevation, float spread, BinauralRenderMode mode)
{
    uint32_t id = m_nextObjectId++;
    AdmAudioObject obj;
    obj.objectId = id;
    obj.objectName = objectName;
    obj.trackChannelIndex = channelIndex;
    obj.azimuthDeg = std::clamp(azimuth, -180.0f, 180.0f);
    obj.elevationDeg = std::clamp(elevation, -90.0f, 90.0f);
    obj.distance = 1.0f;
    obj.spreadWidth = std::clamp(spread, 0.0f, 1.0f);
    obj.binauralMode = mode;
    obj.isBedChannel = false;

    m_objects[id] = obj;
    return id;
}

bool JZAdmBwfSpatialMetadataEngine::updateObjectCoordinates(
    uint32_t objectId, float azimuth, float elevation, float distance)
{
    auto it = m_objects.find(objectId);
    if (it == m_objects.end()) return false;

    it->second.azimuthDeg = std::clamp(azimuth, -180.0f, 180.0f);
    it->second.elevationDeg = std::clamp(elevation, -90.0f, 90.0f);
    it->second.distance = std::clamp(distance, 0.0f, 1.0f);
    return true;
}

const AdmAudioObject* JZAdmBwfSpatialMetadataEngine::getObject(uint32_t objectId) const
{
    auto it = m_objects.find(objectId);
    if (it == m_objects.end()) return nullptr;
    return &(it->second);
}

std::string JZAdmBwfSpatialMetadataEngine::generateAxmlMetadataXml() const
{
    std::ostringstream xml;
    xml << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    xml << "<ituBS2076 version=\"1.0\">\n";
    xml << "  <audioFormatExtended version=\"ITU-R_BS.2076-1\">\n";
    xml << "    <audioProgramme audioProgrammeID=\"APR_1001\" audioProgrammeName=\"" << m_programmeTitle << "\">\n";
    xml << "      <audioContentIDRef>ACO_1001</audioContentIDRef>\n";
    xml << "    </audioProgramme>\n";
    xml << "    <audioContent audioContentID=\"ACO_1001\" audioContentName=\"MainContent\">\n";

    for (const auto& kv : m_objects) {
        std::string objIdStr = "AO_" + std::to_string(1000 + kv.second.objectId);
        xml << "      <audioObjectIDRef>" << objIdStr << "</audioObjectIDRef>\n";
    }

    xml << "    </audioContent>\n";

    for (const auto& kv : m_objects) {
        const auto& o = kv.second;
        std::string objIdStr = "AO_" + std::to_string(1000 + o.objectId);
        xml << "    <audioObject audioObjectID=\"" << objIdStr << "\" audioObjectName=\"" << o.objectName << "\">\n";
        xml << "      <audioTrackUIDRef>ATU_" << std::setfill('0') << std::setw(4) << o.objectId << "</audioTrackUIDRef>\n";
        xml << "      <position>\n";
        xml << "        <azimuth>" << o.azimuthDeg << "</azimuth>\n";
        xml << "        <elevation>" << o.elevationDeg << "</elevation>\n";
        xml << "        <distance>" << o.distance << "</distance>\n";
        xml << "      </position>\n";
        xml << "      <spread>" << o.spreadWidth << "</spread>\n";
        xml << "    </audioObject>\n";
    }

    xml << "  </audioFormatExtended>\n";
    xml << "</ituBS2076>\n";

    return xml.str();
}

std::vector<uint8_t> JZAdmBwfSpatialMetadataEngine::generateChnaChunkBinary() const
{
    // RIFF 'chna' chunk binary structure:
    // uint16_t numTracks
    // uint16_t numUIDs
    // Array of (uint16_t trackIndex, char[12] uid, char[14] trackFormatID, char[11] packFormatID, uint8_t pad)
    std::vector<uint8_t> chna;
    uint16_t numTracks = static_cast<uint16_t>(m_objects.size());
    uint16_t numUIDs = numTracks;

    const uint8_t* pTracks = reinterpret_cast<const uint8_t*>(&numTracks);
    const uint8_t* pUIDs = reinterpret_cast<const uint8_t*>(&numUIDs);

    chna.insert(chna.end(), pTracks, pTracks + 2);
    chna.insert(chna.end(), pUIDs, pUIDs + 2);

    for (const auto& kv : m_objects) {
        uint16_t trk = static_cast<uint16_t>(kv.second.trackChannelIndex);
        const uint8_t* pTrk = reinterpret_cast<const uint8_t*>(&trk);
        chna.insert(chna.end(), pTrk, pTrk + 2);

        // 12-char UID string (e.g. "ATU_00000001")
        char uidBuf[12] = {0};
        std::snprintf(uidBuf, sizeof(uidBuf), "ATU_%08X", kv.second.objectId);
        chna.insert(chna.end(), uidBuf, uidBuf + 12);
    }

    return chna;
}

bool JZAdmBwfSpatialMetadataEngine::validateAtmosCompliance(std::string& outErrorMessage) const
{
    if (m_objects.empty()) {
        outErrorMessage = "No audio bed or objects defined in ADM session.";
        return false;
    }

    // Maximum Atmos object limit is 128 channels
    if (m_objects.size() > 128) {
        outErrorMessage = "Exceeded maximum Dolby Atmos 128 object limit.";
        return false;
    }

    outErrorMessage = "Compliant";
    return true;
}

} // namespace Jazz
