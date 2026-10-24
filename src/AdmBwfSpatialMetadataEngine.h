#ifndef JAZZ_ADM_BWF_SPATIAL_METADATA_ENGINE_H
#define JAZZ_ADM_BWF_SPATIAL_METADATA_ENGINE_H

#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>

namespace Jazz {

enum class BinauralRenderMode {
    DIRECT_EAR,
    ROOM_NEAR,
    ROOM_MID,
    ROOM_FAR,
    BYPASS
};

struct AdmAudioObject {
    uint32_t objectId;
    std::string objectName;
    int trackChannelIndex;
    float azimuthDeg;     // -180.0 to +180.0 (X/Y plane)
    float elevationDeg;   // -90.0 to +90.0 (Z vertical)
    float distance;       // 0.0 to 1.0 normalized
    float spreadWidth;    // 0.0 (Point Source) to 1.0 (Diffuse Bed)
    BinauralRenderMode binauralMode;
    bool isBedChannel;
};

class JZAdmBwfSpatialMetadataEngine {
public:
    JZAdmBwfSpatialMetadataEngine(const std::string& programmeTitle = "Jazz_Atmos_Master");
    ~JZAdmBwfSpatialMetadataEngine() = default;

    // Programme & Master Info
    void setProgrammeTitle(const std::string& title) { m_programmeTitle = title; }
    const std::string& getProgrammeTitle() const { return m_programmeTitle; }

    // Audio Object & Bed Registration
    uint32_t addBedChannel(const std::string& speakerLabel, int channelIndex);
    uint32_t addDynamicObject(const std::string& objectName, int channelIndex,
                              float azimuth, float elevation, float spread = 0.0f,
                              BinauralRenderMode mode = BinauralRenderMode::ROOM_MID);

    bool updateObjectCoordinates(uint32_t objectId, float azimuth, float elevation, float distance);
    const AdmAudioObject* getObject(uint32_t objectId) const;
    size_t getObjectCount() const { return m_objects.size(); }

    // ITU-R BS.2076-1 <axml> Metadata Chunk Generation
    std::string generateAxmlMetadataXml() const;

    // RIFF <chna> (Channel Allocation Chunk) Binary Generation
    std::vector<uint8_t> generateChnaChunkBinary() const;

    // Validation & Dolby Atmos Compliance Check
    bool validateAtmosCompliance(std::string& outErrorMessage) const;

private:
    std::string m_programmeTitle;
    uint32_t m_nextObjectId;
    std::unordered_map<uint32_t, AdmAudioObject> m_objects;
};

} // namespace Jazz

#endif // JAZZ_ADM_BWF_SPATIAL_METADATA_ENGINE_H
