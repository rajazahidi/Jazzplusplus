#include "Ara2HostBridge.h"
#include <algorithm>

namespace Jazz {

JZAra2HostBridge::JZAra2HostBridge(const std::string& documentName)
    : m_documentName(documentName)
    , m_nextId(1)
{
}

uint32_t JZAra2HostBridge::addMusicalContext(const std::string& name, double bpm, int num, int denom)
{
    uint32_t id = m_nextId++;
    AraMusicalContext ctx;
    ctx.contextId = id;
    ctx.name = name;
    ctx.tempoBpm = bpm > 0.0 ? bpm : 120.0;
    ctx.timeSigNumerator = num > 0 ? num : 4;
    ctx.timeSigDenominator = denom > 0 ? denom : 4;

    m_contexts[id] = ctx;
    return id;
}

bool JZAra2HostBridge::updateMusicalContext(uint32_t contextId, double bpm, int num, int denom)
{
    auto it = m_contexts.find(contextId);
    if (it == m_contexts.end()) return false;

    if (bpm > 0.0) it->second.tempoBpm = bpm;
    if (num > 0) it->second.timeSigNumerator = num;
    if (denom > 0) it->second.timeSigDenominator = denom;
    return true;
}

const AraMusicalContext* JZAra2HostBridge::getMusicalContext(uint32_t contextId) const
{
    auto it = m_contexts.find(contextId);
    if (it == m_contexts.end()) return nullptr;
    return &(it->second);
}

uint32_t JZAra2HostBridge::addAudioSource(
    const std::string& filePath, double sampleRate, int64_t sampleCount, int channels)
{
    uint32_t id = m_nextId++;
    AraAudioSource src;
    src.audioSourceId = id;
    src.audioFilePath = filePath;
    src.sampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    src.sampleCount = sampleCount >= 0 ? sampleCount : 0;
    src.channelCount = channels > 0 ? channels : 2;

    m_sources[id] = src;
    return id;
}

const AraAudioSource* JZAra2HostBridge::getAudioSource(uint32_t sourceId) const
{
    auto it = m_sources.find(sourceId);
    if (it == m_sources.end()) return nullptr;
    return &(it->second);
}

uint32_t JZAra2HostBridge::addAudioModification(uint32_t audioSourceId, const std::string& modName)
{
    if (m_sources.find(audioSourceId) == m_sources.end()) return 0;

    uint32_t id = m_nextId++;
    AraAudioModification mod;
    mod.modificationId = id;
    mod.audioSourceId = audioSourceId;
    mod.modificationName = modName;
    mod.hasPitchCorrections = false;
    mod.hasTimingModifications = false;
    mod.hasSpectralEdits = false;

    m_modifications[id] = mod;
    return id;
}

bool JZAra2HostBridge::setModificationFlags(uint32_t modId, bool pitch, bool timing, bool spectral)
{
    auto it = m_modifications.find(modId);
    if (it == m_modifications.end()) return false;

    it->second.hasPitchCorrections = pitch;
    it->second.hasTimingModifications = timing;
    it->second.hasSpectralEdits = spectral;
    return true;
}

const AraAudioModification* JZAra2HostBridge::getAudioModification(uint32_t modId) const
{
    auto it = m_modifications.find(modId);
    if (it == m_modifications.end()) return nullptr;
    return &(it->second);
}

uint32_t JZAra2HostBridge::addPlaybackRegion(
    uint32_t modId, double startTimeSec, double durationSec,
    int64_t srcStartSample, int64_t srcSampleLength)
{
    if (m_modifications.find(modId) == m_modifications.end()) return 0;

    uint32_t id = m_nextId++;
    AraPlaybackRegion reg;
    reg.regionId = id;
    reg.modificationId = modId;
    reg.timelineStartTimeSeconds = startTimeSec;
    reg.timelineDurationSeconds = durationSec;
    reg.sourceStartSample = srcStartSample;
    reg.sourceSampleLength = srcSampleLength;

    m_regions[id] = reg;
    return id;
}

const AraPlaybackRegion* JZAra2HostBridge::getPlaybackRegion(uint32_t regionId) const
{
    auto it = m_regions.find(regionId);
    if (it == m_regions.end()) return nullptr;
    return &(it->second);
}

int64_t JZAra2HostBridge::timelineTimeToSourceSample(uint32_t regionId, double timelineTimeSec) const
{
    const AraPlaybackRegion* reg = getPlaybackRegion(regionId);
    if (!reg) return -1;

    if (timelineTimeSec < reg->timelineStartTimeSeconds ||
        timelineTimeSec > reg->timelineStartTimeSeconds + reg->timelineDurationSeconds) {
        return -1; // Out of region boundaries
    }

    const AraAudioModification* mod = getAudioModification(reg->modificationId);
    if (!mod) return -1;

    const AraAudioSource* src = getAudioSource(mod->audioSourceId);
    if (!src || src->sampleRate <= 0.0) return -1;

    double offsetSec = timelineTimeSec - reg->timelineStartTimeSeconds;
    int64_t offsetSamples = static_cast<int64_t>(offsetSec * src->sampleRate);
    int64_t targetSample = reg->sourceStartSample + offsetSamples;

    if (targetSample >= src->sampleCount) {
        targetSample = src->sampleCount - 1;
    }
    return targetSample;
}

bool JZAra2HostBridge::validateDocumentGraph() const
{
    // Ensure all modifications point to valid sources
    for (const auto& kv : m_modifications) {
        if (m_sources.find(kv.second.audioSourceId) == m_sources.end()) {
            return false;
        }
    }
    // Ensure all regions point to valid modifications
    for (const auto& kv : m_regions) {
        if (m_modifications.find(kv.second.modificationId) == m_modifications.end()) {
            return false;
        }
    }
    return true;
}

} // namespace Jazz
