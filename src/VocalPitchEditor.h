//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Vocal Graphical Pitch Editor & Note-Blob Pitch Manipulator (Melodyne/VariAudio Style)
//
// Segments vocal recordings into musical note blobs, tracks instantaneous fundamental
// pitch (f0) contours, and allows graphical micro-tuning (cents offset), pitch drift
// leveling, vibrato modulation depth scaling, and formant-locked pitch-shifting.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

// Individual Vocal Note-Blob Structure
struct JZVocalNoteBlob
{
  int id;
  float startTimeSec;
  float durationSec;
  float targetPitchMidi;      // Nominal center MIDI pitch (e.g. 60.0 for C4)
  float originalPitchCents;   // Initial detected pitch deviation in cents (-50 to +50)
  float tunedPitchCents;      // User-tuned pitch deviation in cents
  float vibratoDepth;         // Scaler 0.0 (flattened) to 2.0 (accentuated)
  float driftSlope;           // Linear slope of pitch drift across note (-1.0 to +1.0)
  bool  formantLock;          // Preserve natural formant filter characteristics

  JZVocalNoteBlob()
    : id(0),
      startTimeSec(0.0f),
      durationSec(0.25f),
      targetPitchMidi(60.0f),
      originalPitchCents(0.0f),
      tunedPitchCents(0.0f),
      vibratoDepth(1.0f),
      driftSlope(0.0f),
      formantLock(true)
  {
  }
};

class JZVocalPitchEditor
{
public:
  JZVocalPitchEditor(float sampleRate = 44100.0f);
  ~JZVocalPitchEditor() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  // Analyze vocal audio and segment into note blobs
  bool AnalyzeAudio(const std::vector<float>& vocalAudio);

  // Retrieve detected note blobs
  const std::vector<JZVocalNoteBlob>& GetNoteBlobs() const { return mNoteBlobs; }
  std::vector<JZVocalNoteBlob>& GetNoteBlobs() { return mNoteBlobs; }

  // Pitch manipulation operations
  void SnapAllToScale(const std::vector<int>& allowedMidiPitches);
  void FlattenVibratoAll(float targetDepth = 0.0f);
  void LevelDriftSlopeAll();
  void SetBlobPitchCents(int blobId, float centsOffset);
  void SetBlobVibrato(int blobId, float vibratoDepth);

  // Render processed vocal audio with pitch corrections applied
  bool RenderTunedAudio(
    const std::vector<float>& originalAudio,
    std::vector<float>& outTunedAudio);

private:
  float mSampleRate;
  std::vector<JZVocalNoteBlob> mNoteBlobs;

  float DetectPitchAtSample(const std::vector<float>& audio, size_t sampleIdx);
};
