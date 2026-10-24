#ifndef JAZZ_ARA2_HOST_BRIDGE_H
#define JAZZ_ARA2_HOST_BRIDGE_H

#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>

namespace Jazz {

struct AraMusicalContext {
    uint32_t contextId;
    std::string name;
    double tempoBpm;
    int timeSigNumerator;
    int timeSigDenominator;
};

struct AraAudioSource {
    uint32_t audioSourceId;
    std::string audioFilePath;
    double sampleRate;
    int64_t sampleCount;
    int channelCount;
};

struct AraAudioModification {
    uint32_t modificationId;
    uint32_t audioSourceId;
    std::string modificationName;
    bool hasPitchCorrections;
    bool hasTimingModifications;
    bool hasSpectralEdits;
};

struct AraPlaybackRegion {
    uint32_t regionId;
    uint32_t modificationId;
    double timelineStartTimeSeconds;
    double timelineDurationSeconds;
    int64_t sourceStartSample;
    int64_t sourceSampleLength;
};

class JZAra2HostBridge {
public:
    JZAra2HostBridge(const std::string& documentName = "Jazz_Master_Session");
    ~JZAra2HostBridge() = default;

    // Document Management
    const std::string& getDocumentName() const { return m_documentName; }
    void setDocumentName(const std::string& name) { m_documentName = name; }

    // Musical Context (Tempo / Meter / Grid)
    uint32_t addMusicalContext(const std::string& name, double bpm = 120.0, int num = 4, int denom = 4);
    bool updateMusicalContext(uint32_t contextId, double bpm, int num, int denom);
    const AraMusicalContext* getMusicalContext(uint32_t contextId) const;

    // Audio Source Registration
    uint32_t addAudioSource(const std::string& filePath, double sampleRate, int64_t sampleCount, int channels = 2);
    const AraAudioSource* getAudioSource(uint32_t sourceId) const;

    // Audio Modifications (Edits created by ARA plugin)
    uint32_t addAudioModification(uint32_t audioSourceId, const std::string& modName);
    bool setModificationFlags(uint32_t modId, bool pitch, bool timing, bool spectral);
    const AraAudioModification* getAudioModification(uint32_t modId) const;

    // Playback Regions on Host Timeline
    uint32_t addPlaybackRegion(uint32_t modId, double startTimeSec, double durationSec,
                               int64_t srcStartSample, int64_t srcSampleLength);
    const AraPlaybackRegion* getPlaybackRegion(uint32_t regionId) const;

    // Random Access Sample Querying
    // Translates timeline position (seconds) to source sample index
    int64_t timelineTimeToSourceSample(uint32_t regionId, double timelineTimeSec) const;

    // Document Consistency Audit
    bool validateDocumentGraph() const;
    size_t getSourceCount() const { return m_sources.size(); }
    size_t getModificationCount() const { return m_modifications.size(); }
    size_t getRegionCount() const { return m_regions.size(); }

private:
    std::string m_documentName;
    uint32_t m_nextId;

    std::unordered_map<uint32_t, AraMusicalContext> m_contexts;
    std::unordered_map<uint32_t, AraAudioSource> m_sources;
    std::unordered_map<uint32_t, AraAudioModification> m_modifications;
    std::unordered_map<uint32_t, AraPlaybackRegion> m_regions;
};

} // namespace Jazz

#endif // JAZZ_ARA2_HOST_BRIDGE_H
