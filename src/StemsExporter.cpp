//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Multi-Track Stems Exporter & Batch Render Engine Implementation
//*****************************************************************************

#include "StemsExporter.h"
#include <cstdint>
#include <cstring>
#include <sstream>

JZStemsExporter::JZStemsExporter()
{
}

void JZStemsExporter::AddTrack(const JZStemTrackConfig& track)
{
  mTracks.push_back(track);
}

void JZStemsExporter::ClearTracks()
{
  mTracks.clear();
}

void JZStemsExporter::SetOptions(const JZStemsExportOptions& options)
{
  mOptions = options;
}

void JZStemsExporter::BuildWavHeader(
  std::vector<uint8_t>& header,
  int numChannels,
  int sampleRate,
  int bitsPerSample,
  int dataBytes)
{
  header.resize(44);
  int byteRate = sampleRate * numChannels * (bitsPerSample / 8);
  int blockAlign = numChannels * (bitsPerSample / 8);
  int chunkSize = 36 + dataBytes;

  // RIFF Chunk Descriptor
  header[0] = 'R'; header[1] = 'I'; header[2] = 'F'; header[3] = 'F';
  header[4] = (uint8_t)(chunkSize & 0xFF);
  header[5] = (uint8_t)((chunkSize >> 8) & 0xFF);
  header[6] = (uint8_t)((chunkSize >> 16) & 0xFF);
  header[7] = (uint8_t)((chunkSize >> 24) & 0xFF);
  header[8] = 'W'; header[9] = 'A'; header[10] = 'V'; header[11] = 'E';

  // "fmt " sub-chunk
  header[12] = 'f'; header[13] = 'm'; header[14] = 't'; header[15] = ' ';
  header[16] = 16; header[17] = 0; header[18] = 0; header[19] = 0; // Subchunk1Size (16 for PCM)
  uint16_t audioFormat = (bitsPerSample == 32) ? 3 : 1; // 1 = PCM, 3 = IEEE Float
  header[20] = (uint8_t)(audioFormat & 0xFF);
  header[21] = (uint8_t)((audioFormat >> 8) & 0xFF);
  header[22] = (uint8_t)(numChannels & 0xFF);
  header[23] = (uint8_t)((numChannels >> 8) & 0xFF);
  
  // SampleRate
  header[24] = (uint8_t)(sampleRate & 0xFF);
  header[25] = (uint8_t)((sampleRate >> 8) & 0xFF);
  header[26] = (uint8_t)((sampleRate >> 16) & 0xFF);
  header[27] = (uint8_t)((sampleRate >> 24) & 0xFF);

  // ByteRate
  header[28] = (uint8_t)(byteRate & 0xFF);
  header[29] = (uint8_t)((byteRate >> 8) & 0xFF);
  header[30] = (uint8_t)((byteRate >> 16) & 0xFF);
  header[31] = (uint8_t)((byteRate >> 24) & 0xFF);

  // BlockAlign
  header[32] = (uint8_t)(blockAlign & 0xFF);
  header[33] = (uint8_t)((blockAlign >> 8) & 0xFF);

  // BitsPerSample
  header[34] = (uint8_t)(bitsPerSample & 0xFF);
  header[35] = (uint8_t)((bitsPerSample >> 8) & 0xFF);

  // "data" sub-chunk
  header[36] = 'd'; header[37] = 'a'; header[38] = 't'; header[39] = 'a';
  header[40] = (uint8_t)(dataBytes & 0xFF);
  header[41] = (uint8_t)((dataBytes >> 8) & 0xFF);
  header[42] = (uint8_t)((dataBytes >> 16) & 0xFF);
  header[43] = (uint8_t)((dataBytes >> 24) & 0xFF);
}

void JZStemsExporter::DetectSilenceBounds(
  const std::vector<float>& left,
  const std::vector<float>& right,
  float thresholdDB,
  int& outStartSample,
  int& outEndSample)
{
  float thresholdAmp = std::pow(10.0f, thresholdDB / 20.0f);
  int n = (int)std::min(left.size(), right.size());
  outStartSample = 0;
  outEndSample = n > 0 ? n - 1 : 0;

  if (n == 0) return;

  // Find start
  for (int i = 0; i < n; ++i)
  {
    if (std::fabs(left[i]) >= thresholdAmp || std::fabs(right[i]) >= thresholdAmp)
    {
      outStartSample = i;
      break;
    }
  }

  // Find end
  for (int i = n - 1; i >= outStartSample; --i)
  {
    if (std::fabs(left[i]) >= thresholdAmp || std::fabs(right[i]) >= thresholdAmp)
    {
      outEndSample = i;
      break;
    }
  }
}

bool JZStemsExporter::RenderWavToMemory(
  const std::vector<float>& leftChannel,
  const std::vector<float>& rightChannel,
  const JZStemsExportOptions& opts,
  std::vector<uint8_t>& outWavBytes,
  float& outPeakAmp)
{
  outPeakAmp = 0.0f;
  int numSamples = (int)std::min(leftChannel.size(), rightChannel.size());
  if (numSamples <= 0) return false;

  int startIdx = std::max(0, opts.startSample);
  int endIdx = (opts.endSample >= 0 && opts.endSample < numSamples) ? opts.endSample : numSamples - 1;

  if (opts.trimSilence)
  {
    int sTrim = 0, eTrim = endIdx;
    DetectSilenceBounds(leftChannel, rightChannel, -60.0f, sTrim, eTrim);
    startIdx = std::max(startIdx, sTrim);
    endIdx = std::min(endIdx, eTrim);
  }

  if (startIdx > endIdx)
  {
    startIdx = 0;
    endIdx = numSamples - 1;
  }

  int activeSampleCount = endIdx - startIdx + 1;

  // Measure peak
  for (int i = startIdx; i <= endIdx; ++i)
  {
    outPeakAmp = std::max(outPeakAmp, std::max(std::fabs(leftChannel[i]), std::fabs(rightChannel[i])));
  }

  // Calculate normalization scale
  float normGain = 1.0f;
  if (opts.normalizePeak && outPeakAmp > 1.0e-5f)
  {
    float targetLinear = std::pow(10.0f, opts.targetPeakDB / 20.0f);
    normGain = targetLinear / outPeakAmp;
    outPeakAmp = targetLinear;
  }

  int bits = (opts.bitDepth == 16 || opts.bitDepth == 24 || opts.bitDepth == 32) ? opts.bitDepth : 24;
  int bytesPerSample = bits / 8;
  int numChannels = 2;
  int dataBytes = activeSampleCount * numChannels * bytesPerSample;

  std::vector<uint8_t> header;
  BuildWavHeader(header, numChannels, opts.sampleRate, bits, dataBytes);

  outWavBytes.clear();
  outWavBytes.reserve(header.size() + dataBytes);
  outWavBytes.insert(outWavBytes.end(), header.begin(), header.end());

  for (int i = startIdx; i <= endIdx; ++i)
  {
    float lSample = std::max(-1.0f, std::min(1.0f, leftChannel[i] * normGain));
    float rSample = std::max(-1.0f, std::min(1.0f, rightChannel[i] * normGain));

    if (bits == 16)
    {
      int16_t l16 = (int16_t)(lSample * 32767.0f);
      int16_t r16 = (int16_t)(rSample * 32767.0f);
      uint8_t* pl = (uint8_t*)&l16;
      uint8_t* pr = (uint8_t*)&r16;
      outWavBytes.push_back(pl[0]); outWavBytes.push_back(pl[1]);
      outWavBytes.push_back(pr[0]); outWavBytes.push_back(pr[1]);
    }
    else if (bits == 24)
    {
      int32_t l24 = (int32_t)(lSample * 8388607.0f);
      int32_t r24 = (int32_t)(rSample * 8388607.0f);
      outWavBytes.push_back((uint8_t)(l24 & 0xFF));
      outWavBytes.push_back((uint8_t)((l24 >> 8) & 0xFF));
      outWavBytes.push_back((uint8_t)((l24 >> 16) & 0xFF));
      outWavBytes.push_back((uint8_t)(r24 & 0xFF));
      outWavBytes.push_back((uint8_t)((r24 >> 8) & 0xFF));
      outWavBytes.push_back((uint8_t)((r24 >> 16) & 0xFF));
    }
    else // 32-bit float
    {
      uint8_t* pl = (uint8_t*)&lSample;
      uint8_t* pr = (uint8_t*)&rSample;
      for (int b = 0; b < 4; ++b) outWavBytes.push_back(pl[b]);
      for (int b = 0; b < 4; ++b) outWavBytes.push_back(pr[b]);
    }
  }

  return true;
}

std::vector<JZStemExportResult> JZStemsExporter::ExportStems(
  const std::map<int, std::vector<float>>& audioBuffersL,
  const std::map<int, std::vector<float>>& audioBuffersR)
{
  std::vector<JZStemExportResult> results;

  // Check if any track is soloed
  bool anySolo = false;
  for (const auto& trk : mTracks)
  {
    if (trk.isSoloed) { anySolo = true; break; }
  }

  for (const auto& trk : mTracks)
  {
    JZStemExportResult res;
    res.trackId = trk.trackId;
    res.trackName = trk.trackName;

    // Filter muted or non-soloed tracks
    if (trk.isMuted || (anySolo && !trk.isSoloed))
    {
      res.success = false;
      res.statusMessage = trk.isMuted ? "Skipped (Muted)" : "Skipped (Not Soloed)";
      results.push_back(res);
      continue;
    }

    auto itL = audioBuffersL.find(trk.trackId);
    auto itR = audioBuffersR.find(trk.trackId);

    if (itL == audioBuffersL.end() || itL->second.empty())
    {
      res.success = false;
      res.statusMessage = "No audio data found for track";
      results.push_back(res);
      continue;
    }

    std::vector<float> processedL = itL->second;
    std::vector<float> processedR = (itR != audioBuffersR.end()) ? itR->second : processedL;

    // Apply custom gain if set
    if (std::fabs(trk.customGainDB) > 0.001f)
    {
      float linGain = std::pow(10.0f, trk.customGainDB / 20.0f);
      for (auto& s : processedL) s *= linGain;
      for (auto& s : processedR) s *= linGain;
    }

    std::vector<uint8_t> wavData;
    float peakAmp = 0.0f;
    bool ok = RenderWavToMemory(processedL, processedR, mOptions, wavData, peakAmp);

    if (ok)
    {
      res.success = true;
      res.fileName = trk.trackName + ".wav";
      res.fileSizeBytes = wavData.size();
      res.sampleCount = (int)processedL.size();
      res.peakAmplitude = peakAmp;
      res.peakAmplitudeDB = (peakAmp > 1.0e-5f) ? (20.0f * std::log10(peakAmp)) : -96.0f;
      res.statusMessage = "Successfully Rendered (" + std::to_string(mOptions.bitDepth) + "-bit WAV)";
    }
    else
    {
      res.success = false;
      res.statusMessage = "Render failed";
    }

    results.push_back(res);
  }

  return results;
}
