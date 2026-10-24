//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Real-Time Vocal Pitch Correction & Microtonal Auto-Tune Engine
//
// Fast autocorrelation pitch tracker (YIN / AMDF), musical scale quantizer,
// adjustable retune speed (natural to robotic hard-tune), and real-time
// microtonal pitch correction metrics.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

// Scale type for pitch correction snapping
enum JZPitchScaleType
{
  SCALE_CHROMATIC = 0,
  SCALE_MAJOR,
  SCALE_NATURAL_MINOR,
  SCALE_HARMONIC_MINOR,
  SCALE_DORIAN,
  SCALE_PENTATONIC_MAJOR,
  SCALE_PENTATONIC_MINOR,
  SCALE_BLUES
};

// Pitch correction parameters
struct JZPitchTuneConfig
{
  int keyTonic;                   // 0 = C, 1 = C#, 2 = D, ..., 11 = B
  JZPitchScaleType scaleType;
  float retuneSpeedMs;            // 0.0 ms (instant hard tune) to 150.0 ms (smooth natural)
  float correctionAmount;         // 0.0 (bypass) to 1.0 (100% full lock)
  float referenceA4;              // 440.0 Hz
  float minDetectFreq;            // 65.0 Hz (C2)
  float maxDetectFreq;            // 1050.0 Hz (C6)

  JZPitchTuneConfig()
    : keyTonic(0),
      scaleType(SCALE_CHROMATIC),
      retuneSpeedMs(15.0f),
      correctionAmount(1.0f),
      referenceA4(440.0f),
      minDetectFreq(70.0f),
      maxDetectFreq(1000.0f)
  {
  }
};

// Live pitch tracking feedback
struct JZPitchTrackInfo
{
  float detectedFreqHz;
  float detectedMidiNote;         // Fractional note (e.g. 60.32)
  int targetMidiNote;             // Quantized note in scale (e.g. 60)
  float targetFreqHz;
  float centsDeviation;           // Offset from detected to target (-50 to +50 cents)
  float confidence;               // 0.0 (unvoiced/noise) to 1.0 (pure tone)
  bool isVoiced;
  std::string noteName;

  JZPitchTrackInfo()
    : detectedFreqHz(0.0f),
      detectedMidiNote(0.0f),
      targetMidiNote(60),
      targetFreqHz(261.63f),
      centsDeviation(0.0f),
      confidence(0.0f),
      isVoiced(false),
      noteName("---")
  {
  }
};

// Pitch Correction Engine
class JZPitchCorrectionEngine
{
public:
  JZPitchCorrectionEngine(float sampleRate = 44100.0f);
  ~JZPitchCorrectionEngine() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  void SetConfig(const JZPitchTuneConfig& config);
  const JZPitchTuneConfig& GetConfig() const { return mConfig; }
  JZPitchTuneConfig& GetConfig() { return mConfig; }

  // Analyze audio block to detect pitch and calculate target scale note
  bool DetectPitch(const float* buffer, int numSamples, JZPitchTrackInfo& info);

  // Quantize a frequency in Hz to closest allowed note in current scale
  void QuantizeToScale(float inputFreqHz, int& outTargetMidiNote, float& outTargetFreqHz, float& outCentsOffset) const;

  // Process mono buffer: detects pitch and applies real-time phase pitch correction
  void Process(float* inOut, int numSamples, JZPitchTrackInfo* optInfo = nullptr);

  // Helper note name formatting
  static std::string MidiNoteToName(int noteNumber);

  // Check if pitch class is in scale
  bool IsPitchClassInScale(int pitchClass) const;

private:
  float mSampleRate;
  JZPitchTuneConfig mConfig;
  JZPitchTrackInfo mLastTrackInfo;

  float mSmoothedPitchShiftRatio; // Filtered pitch ratio according to retune speed
  std::vector<float> mAnalysisBuffer;
  int mAnalysisBufferPos;

  // Internal scale mask for 12 chromatic semitones
  bool mScaleMask[12];
  void UpdateScaleMask();
};
