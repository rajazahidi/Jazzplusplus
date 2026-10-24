//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Dynamic 24-Band Vocoder & Robotic Speech Synthesizer
//
// Classic & modern robotic vocoder featuring 24-band analysis and synthesis
// filterbanks, speech envelope followers, internal polyphonic sawtooth carrier,
// formant shifting (-12 to +12 semitones), and unvoiced consonant injection.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

// Carrier generator source
enum JZVocoderCarrierMode
{
  VOCODER_CARRIER_INTERNAL_SAW = 0,   // Internal rich polyphonic sawtooth generator
  VOCODER_CARRIER_INTERNAL_PULSE,     // Internal robotic pulse wave
  VOCODER_CARRIER_EXTERNAL            // External audio carrier input
};

// Configuration for Vocoder
struct JZVocoderConfig
{
  JZVocoderCarrierMode carrierMode;
  int formantShiftBands;      // -8 to +8 band shift (formant pitch transformation)
  float bandAttackMs;         // Envelope follower attack (0.5 to 20 ms, default 3.0 ms)
  float bandReleaseMs;        // Envelope follower release (10 to 200 ms, default 35.0 ms)
  float unvoicedHighPassGain; // Adds vocal sibilance/consonants back (0.0 to 0.5)
  float dryWet;               // 0.0 to 1.0

  JZVocoderConfig()
    : carrierMode(VOCODER_CARRIER_INTERNAL_SAW),
      formantShiftBands(0),
      bandAttackMs(3.0f),
      bandReleaseMs(35.0f),
      unvoicedHighPassGain(0.12f),
      dryWet(1.0f)
  {
  }
};

// 2nd-order bandpass filter state
struct JZVocoderBandFilter
{
  float b0, b1, b2, a1, a2;
  float x1, x2, y1, y2;

  JZVocoderBandFilter() : b0(0), b1(0), b2(0), a1(0), a2(0), x1(0), x2(0), y1(0), y2(0) {}
  void Reset() { x1 = x2 = y1 = y2 = 0.0f; }

  inline float Process(float in)
  {
    float out = b0 * in + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1; x1 = in;
    y2 = y1; y1 = out;
    return out;
  }
};

// Vocoder Engine Class
class JZVocoderEngine
{
public:
  static const int NUM_BANDS = 24;

  JZVocoderEngine(float sampleRate = 44100.0f);
  ~JZVocoderEngine() = default;

  void SetSampleRate(float sampleRate);
  float GetSampleRate() const { return mSampleRate; }

  void SetConfig(const JZVocoderConfig& config);
  const JZVocoderConfig& GetConfig() const { return mConfig; }
  JZVocoderConfig& GetConfig() { return mConfig; }

  void Reset();

  // Internal carrier chord note triggering
  void SetCarrierPitch(int midiNote);

  // Process Vocoder: takes modulator (vocal speech) and optional external carrier
  void Process(
    const float* modulatorMono,
    float* outL,
    float* outR,
    int numSamples,
    const float* optCarrierMono = nullptr);

  // Active band envelopes for real-time visualization
  const float* GetBandEnvelopes() const { return mBandEnvelopes; }

private:
  float mSampleRate;
  JZVocoderConfig mConfig;
  int mCarrierMidiNote;
  double mCarrierPhase;

  // 24 Analysis filters (for Modulator) and 24 Synthesis filters (for Carrier)
  JZVocoderBandFilter mAnalysisFilters[NUM_BANDS];
  JZVocoderBandFilter mSynthesisFiltersL[NUM_BANDS];
  JZVocoderBandFilter mSynthesisFiltersR[NUM_BANDS];

  float mBandEnvelopes[NUM_BANDS];
  float mAlphaAttack;
  float mAlphaRelease;

  void InitializeFilterbanks();
  void UpdateCoefficients();
};
