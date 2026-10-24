//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Intelligent Vocal Polyphonic Harmonizer
//
// Generates up to 4 diatonic harmony voices from a single monophonic vocal track
// (+3rd, +5th, -4th, Octave Bass) with musical scale tracking, formant preservation,
// independent stereo panning, and volume mixing.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

// Configuration for an individual harmony voice
struct JZHarmonyVoiceConfig
{
  bool enabled;
  int diatonicInterval;       // +2 = 3rd Up, +4 = 5th Up, -3 = 4th Down, -7 = Octave Down
  int chromaticSemitones;     // Fixed chromatic shift if scale snapping is disabled
  bool useDiatonicSnap;
  float volumeDB;             // -24.0 dB to +6.0 dB
  float stereoPan;            // -1.0 (Left) to +1.0 (Right)
  float formantShiftSemitones;// -5.0 to +5.0 semitones

  JZHarmonyVoiceConfig(int interval = 2, float pan = 0.0f, float volDB = -3.0f)
    : enabled(true),
      diatonicInterval(interval),
      chromaticSemitones(4),
      useDiatonicSnap(true),
      volumeDB(volDB),
      stereoPan(pan),
      formantShiftSemitones(0.0f)
  {
  }
};

// Full Harmonizer Configuration
struct JZHarmonizerConfig
{
  int keyTonic;               // 0 = C, 1 = C#, 2 = D, ..., 11 = B
  bool isMinor;               // True = Natural Minor, False = Major
  JZHarmonyVoiceConfig voices[4];
  float dryGain;              // 0.0 to 1.0 (default 1.0)
  float wetGain;              // 0.0 to 1.0 (default 0.7)

  JZHarmonizerConfig()
    : keyTonic(0),
      isMinor(false),
      dryGain(1.0f),
      wetGain(0.7f)
  {
    // Voice 1: High 3rd (panned slightly right)
    voices[0] = JZHarmonyVoiceConfig(2, 0.45f, -3.0f);
    // Voice 2: High 5th (panned slightly left)
    voices[1] = JZHarmonyVoiceConfig(4, -0.45f, -4.0f);
    // Voice 3: Low 4th (deep harmony, centered)
    voices[2] = JZHarmonyVoiceConfig(-3, -0.15f, -6.0f);
    // Voice 4: Octave Down Bass (warmth)
    voices[3] = JZHarmonyVoiceConfig(-7, 0.0f, -8.0f);
  }
};

// Vocal Harmonizer Class
class JZVocalHarmonizer
{
public:
  JZVocalHarmonizer(float sampleRate = 44100.0f);
  ~JZVocalHarmonizer() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  void SetConfig(const JZHarmonizerConfig& config);
  const JZHarmonizerConfig& GetConfig() const { return mConfig; }
  JZHarmonizerConfig& GetConfig() { return mConfig; }

  void Reset();

  // Process mono lead vocal into stereo harmonized choir
  void Process(const float* inMono, float* outL, float* outR, int numSamples);

  // Helper: Diatonic pitch transposition calculator
  int CalculateDiatonicShift(int inputMidiNote, int diatonicInterval) const;

private:
  float mSampleRate;
  JZHarmonizerConfig mConfig;

  // Grain delay buffers per voice
  std::vector<float> mVoiceBuffer[4];
  int mWritePos[4];
  float mGrainPhases[4];

  void UpdateScaleDegrees();
  int mScaleDegrees[7];
};
