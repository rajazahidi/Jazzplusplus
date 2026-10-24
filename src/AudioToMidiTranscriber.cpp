//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Audio-to-MIDI Melody & Chord Transcription Implementation
//*****************************************************************************

#include "AudioToMidiTranscriber.h"

JZAudioToMidiTranscriber::JZAudioToMidiTranscriber(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f)
{
}

void JZAudioToMidiTranscriber::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
  }
}

void JZAudioToMidiTranscriber::SetConfig(const JZTranscribeConfig& config)
{
  mConfig = config;
}

float JZAudioToMidiTranscriber::DetectPitchInWindow(
  const float* buffer,
  int numSamples,
  float& outConfidence) const
{
  outConfidence = 0.0f;
  if (!buffer || numSamples < 256) return 0.0f;

  int minLag = (int)(mSampleRate / mConfig.maxFreqHz);
  int maxLag = (int)(mSampleRate / mConfig.minFreqHz);
  minLag = std::max(2, minLag);
  maxLag = std::min(numSamples / 2, maxLag);

  if (maxLag <= minLag) return 0.0f;

  float totalEnergy = 0.0f;
  for (int i = 0; i < maxLag; ++i) totalEnergy += buffer[i] * buffer[i];

  if (totalEnergy < mConfig.energyThreshold * 0.1f) return 0.0f;

  int bestLag = -1;
  float minDiff = 1e9f;

  for (int lag = minLag; lag <= maxLag; ++lag)
  {
    float diffSum = 0.0f;
    for (int j = 0; j < maxLag; ++j)
    {
      float d = buffer[j] - buffer[j + lag];
      diffSum += d * d;
    }
    if (diffSum < minDiff)
    {
      minDiff = diffSum;
      bestLag = lag;
    }
  }

  if (bestLag < minLag || bestLag >= maxLag) return 0.0f;

  // Sub-sample parabolic interpolation
  float y1 = 0.0f, y2 = minDiff, y3 = 0.0f;
  for (int j = 0; j < maxLag; ++j)
  {
    float d1 = buffer[j] - buffer[j + bestLag - 1];
    float d3 = buffer[j] - buffer[j + bestLag + 1];
    y1 += d1 * d1;
    y3 += d3 * d3;
  }

  float interpLag = (float)bestLag;
  float denom = 2.0f * (y1 - 2.0f * y2 + y3);
  if (std::fabs(denom) > 1e-6f)
  {
    interpLag += (y1 - y3) / denom;
  }

  float confidence = 1.0f - std::min(1.0f, minDiff / std::max(1e-5f, totalEnergy * 2.0f));
  outConfidence = confidence;

  if (confidence >= mConfig.pitchConfidenceMin)
  {
    return mSampleRate / interpLag;
  }

  return 0.0f;
}

int JZAudioToMidiTranscriber::TranscribeAudio(
  const std::vector<float>& audioBuffer,
  float bpm,
  std::vector<JZMatrixMidiNote>& outNotes)
{
  outNotes.clear();
  if (audioBuffer.empty() || bpm <= 10.0f) return 0;

  int windowSize = (int)(mSampleRate * 0.03f); // 30ms analysis hop
  int hopSize = windowSize / 2;
  int totalSamples = (int)audioBuffer.size();

  float ticksPerSecond = (bpm * 96.0f) / 60.0f; // 96 PPQN
  int minDurationSamples = (int)(mConfig.minNoteDurationMs * 0.001f * mSampleRate);

  int currentNote = -1;
  int noteStartSample = 0;
  float notePeakAmp = 0.0f;

  for (int i = 0; i < totalSamples - windowSize; i += hopSize)
  {
    float conf = 0.0f;
    float freqHz = DetectPitchInWindow(&audioBuffer[i], windowSize, conf);

    int pitchMidi = -1;
    if (freqHz >= mConfig.minFreqHz && freqHz <= mConfig.maxFreqHz)
    {
      pitchMidi = (int)std::floor(69.0 + 12.0 * (std::log(freqHz / 440.0) / std::log(2.0)) + 0.5);
    }

    // Measure peak amplitude in window for velocity
    float windowPeak = 0.0f;
    for (int j = 0; j < windowSize; ++j)
    {
      windowPeak = std::max(windowPeak, std::fabs(audioBuffer[i + j]));
    }

    if (pitchMidi == currentNote && pitchMidi >= 0)
    {
      // Continuation of current note
      notePeakAmp = std::max(notePeakAmp, windowPeak);
    }
    else
    {
      // End previous note if active
      if (currentNote >= 0)
      {
        int durSamples = i - noteStartSample;
        if (durSamples >= minDurationSamples)
        {
          int startTick = (int)((float)noteStartSample / mSampleRate * ticksPerSecond);
          int durTicks = std::max(24, (int)((float)durSamples / mSampleRate * ticksPerSecond));
          int vel = std::max(20, std::min(127, (int)(notePeakAmp * 127.0f)));

          outNotes.emplace_back(currentNote, vel, startTick, durTicks);
        }
      }

      // Start new note
      currentNote = pitchMidi;
      noteStartSample = i;
      notePeakAmp = windowPeak;
    }
  }

  // Finalize last active note
  if (currentNote >= 0)
  {
    int durSamples = totalSamples - noteStartSample;
    if (durSamples >= minDurationSamples)
    {
      int startTick = (int)((float)noteStartSample / mSampleRate * ticksPerSecond);
      int durTicks = std::max(24, (int)((float)durSamples / mSampleRate * ticksPerSecond));
      int vel = std::max(20, std::min(127, (int)(notePeakAmp * 127.0f)));

      outNotes.emplace_back(currentNote, vel, startTick, durTicks);
    }
  }

  return (int)outNotes.size();
}
