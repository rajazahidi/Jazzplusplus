//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Multi-Track Stems Exporter & Batch Render Engine
//
// Studio stem bounce engine supporting batch individual track rendering,
// 16/24/32-bit WAV generation, silence trimming, stem normalization,
// and export job reporting.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <cstdint>
#include <cmath>
#include <algorithm>

// Configuration for an individual stem track
struct JZStemTrackConfig
{
  int trackId;
  std::string trackName;
  bool isMuted;
  bool isSoloed;
  bool includeEffects;      // True for Wet Stem (with FX), False for Dry
  float customGainDB;       // Gain adjustment in dB (default 0.0)

  JZStemTrackConfig(int id = 0, const std::string& name = "Stem", bool muted = false, bool soloed = false)
    : trackId(id),
      trackName(name),
      isMuted(muted),
      isSoloed(soloed),
      includeEffects(true),
      customGainDB(0.0f)
  {
  }
};

// Batch Export Options
struct JZStemsExportOptions
{
  std::string outputDirectory;
  int sampleRate;           // 44100, 48000, 96000
  int bitDepth;             // 16, 24, 32
  int startSample;          // Region start (0 for beginning)
  int endSample;            // Region end (-1 for full length)
  bool trimSilence;         // Auto-detect and trim lead-in/lead-out silence
  bool normalizePeak;       // Normalize stems to target peak
  float targetPeakDB;       // Target normalization ceiling (-0.1 dB typical)

  JZStemsExportOptions()
    : outputDirectory("Stems/"),
      sampleRate(44100),
      bitDepth(24),
      startSample(0),
      endSample(-1),
      trimSilence(true),
      normalizePeak(false),
      targetPeakDB(-0.1f)
  {
  }
};

// Result statistics for each exported stem
struct JZStemExportResult
{
  int trackId;
  std::string trackName;
  std::string fileName;
  float peakAmplitude;
  float peakAmplitudeDB;
  int sampleCount;
  size_t fileSizeBytes;
  bool success;
  std::string statusMessage;

  JZStemExportResult()
    : trackId(0),
      peakAmplitude(0.0f),
      peakAmplitudeDB(-96.0f),
      sampleCount(0),
      fileSizeBytes(0),
      success(false),
      statusMessage("")
  {
  }
};

// Stems Exporter Class
class JZStemsExporter
{
public:
  JZStemsExporter();
  ~JZStemsExporter() = default;

  void AddTrack(const JZStemTrackConfig& track);
  void ClearTracks();
  const std::vector<JZStemTrackConfig>& GetTracks() const { return mTracks; }

  void SetOptions(const JZStemsExportOptions& options);
  const JZStemsExportOptions& GetOptions() const { return mOptions; }
  JZStemsExportOptions& GetOptions() { return mOptions; }

  // Batch Export all registered tracks from provided memory audio buffers
  std::vector<JZStemExportResult> ExportStems(
    const std::map<int, std::vector<float>>& audioBuffersL,
    const std::map<int, std::vector<float>>& audioBuffersR);

  // Encode a single stereo stem into standard PCM / IEEE Float WAV bytes in memory
  static bool RenderWavToMemory(
    const std::vector<float>& leftChannel,
    const std::vector<float>& rightChannel,
    const JZStemsExportOptions& opts,
    std::vector<uint8_t>& outWavBytes,
    float& outPeakAmp);

  // Static WAV Header builder
  static void BuildWavHeader(
    std::vector<uint8_t>& header,
    int numChannels,
    int sampleRate,
    int bitsPerSample,
    int dataBytes);

  // Utility: Detect non-silent audio region [start, end]
  static void DetectSilenceBounds(
    const std::vector<float>& left,
    const std::vector<float>& right,
    float thresholdDB,
    int& outStartSample,
    int& outEndSample);

private:
  std::vector<JZStemTrackConfig> mTracks;
  JZStemsExportOptions mOptions;
};
