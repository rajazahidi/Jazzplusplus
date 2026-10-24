//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Audio-to-MIDI Melody & Chord Transcription Engine
//
// Transcribes raw audio recordings (sung vocals, acoustic guitar, brass)
// into discrete MIDI note sequences with transient onset detection, sub-sample
// pitch tracking (YIN/AMDF), note duration segmentation, and velocity mapping.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include "ClipMatrixEngine.h" // For JZMatrixMidiNote

// Configuration for Audio-to-MIDI Transcription
struct JZTranscribeConfig
{
  float minFreqHz;            // Minimum pitch to track (e.g. 65 Hz = C2)
  float maxFreqHz;            // Maximum pitch to track (e.g. 1050 Hz = C6)
  float energyThreshold;      // Onset sensitivity (default 0.005)
  float minNoteDurationMs;    // Ignore notes shorter than this (default 60 ms)
  float pitchConfidenceMin;   // 0.0 to 1.0 (default 0.55)
  int quantizeScaleTonic;     // -1 = Chromatic (no snap), 0 = C, 2 = D...

  JZTranscribeConfig()
    : minFreqHz(65.0f),
      maxFreqHz(1050.0f),
      energyThreshold(0.005f),
      minNoteDurationMs(60.0f),
      pitchConfidenceMin(0.55f),
      quantizeScaleTonic(-1)
  {
  }
};

// Audio-to-MIDI Transcriber Class
class JZAudioToMidiTranscriber
{
public:
  JZAudioToMidiTranscriber(float sampleRate = 44100.0f);
  ~JZAudioToMidiTranscriber() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  void SetConfig(const JZTranscribeConfig& config);
  const JZTranscribeConfig& GetConfig() const { return mConfig; }
  JZTranscribeConfig& GetConfig() { return mConfig; }

  // Transcribe full mono audio buffer into MIDI notes with tick timestamps
  int TranscribeAudio(
    const std::vector<float>& audioBuffer,
    float bpm,
    std::vector<JZMatrixMidiNote>& outNotes);

  // Single-window pitch estimator helper (returns frequency in Hz, 0.0 if unvoiced)
  float DetectPitchInWindow(const float* buffer, int numSamples, float& outConfidence) const;

private:
  float mSampleRate;
  JZTranscribeConfig mConfig;
};
