//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Karplus-Strong Physical Acoustic Resonator Implementation
//*****************************************************************************

#include "PhysicalResonator.h"

JZPhysicalResonator::JZPhysicalResonator(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f)
{
  Reset();
}

void JZPhysicalResonator::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
    Reset();
  }
}

void JZPhysicalResonator::SetConfig(const JZResonatorConfig& config)
{
  mConfig = config;
}

void JZPhysicalResonator::Reset()
{
  for (int s = 0; s < NUM_STRINGS; ++s)
  {
    mStrings[s].Reset();
  }
}

void JZPhysicalResonator::GetChordIntervals(int intervals[NUM_STRINGS]) const
{
  switch (mConfig.chordMode)
  {
    case RESONATOR_UNISON:
      intervals[0] = 0; intervals[1] = 0; intervals[2] = 0; intervals[3] = 0;
      break;
    case RESONATOR_MAJOR:
      intervals[0] = 0; intervals[1] = 4; intervals[2] = 7; intervals[3] = 12; // Root, 3rd, 5th, Octave
      break;
    case RESONATOR_MINOR:
      intervals[0] = 0; intervals[1] = 3; intervals[2] = 7; intervals[3] = 12; // Root, m3, 5th, Octave
      break;
    case RESONATOR_SUS4:
      intervals[0] = 0; intervals[1] = 5; intervals[2] = 7; intervals[3] = 12; // Root, 4th, 5th, Octave
      break;
    case RESONATOR_PENTATONIC:
    default:
      intervals[0] = 0; intervals[1] = 3; intervals[2] = 5; intervals[3] = 10; // Root, m3, 4th, m7
      break;
  }
}

void JZPhysicalResonator::Process(float* inOutL, float* inOutR, int numSamples)
{
  if (!inOutL || numSamples <= 0) return;

  int intervals[NUM_STRINGS];
  GetChordIntervals(intervals);

  float rootFreq = 440.0f * std::pow(2.0f, (mConfig.rootMidiNote - 69) / 12.0f);
  float dampAlpha = std::max(0.01f, std::min(0.95f, 1.0f - mConfig.damping * 0.75f));

  for (int i = 0; i < numSamples; ++i)
  {
    float rawInL = inOutL[i];
    float rawInR = inOutR ? inOutR[i] : rawInL;
    float monoExcite = 0.5f * (rawInL + rawInR);

    float resSumL = 0.0f;
    float resSumR = 0.0f;

    for (int s = 0; s < NUM_STRINGS; ++s)
    {
      auto& stringLine = mStrings[s];
      int bufLen = (int)stringLine.buffer.size();

      // String frequency and delay length
      float freq = rootFreq * std::pow(2.0f, (float)intervals[s] / 12.0f);
      freq = std::max(20.0f, std::min(mSampleRate * 0.45f, freq));
      float delaySamples = mSampleRate / freq;
      delaySamples = std::max(2.0f, std::min((float)bufLen - 2.0f, delaySamples));

      // Feedback gain for target decay time (T60: -60dB decay)
      float feedback = std::pow(0.001f, 1.0f / (freq * std::max(0.1f, mConfig.decayTimeSec)));
      feedback = std::min(0.999f, feedback);

      // Read fractional delayed sample
      float rPos = (float)stringLine.writePos - delaySamples;
      if (rPos < 0.0f) rPos += (float)bufLen;
      int idx0 = (int)rPos;
      int idx1 = (idx0 + 1) % bufLen;
      float frac = rPos - (float)idx0;
      float delayedSample = stringLine.buffer[idx0] + frac * (stringLine.buffer[idx1] - stringLine.buffer[idx0]);

      // Internal low-pass filter in string feedback loop (damping)
      stringLine.filterState += dampAlpha * (delayedSample - stringLine.filterState);
      float filteredFeedback = stringLine.filterState * feedback;

      // Inject mono excitation impulse into string
      stringLine.buffer[stringLine.writePos] = monoExcite + filteredFeedback;
      stringLine.writePos = (stringLine.writePos + 1) % bufLen;

      // Panning across stereo field
      float pan = (NUM_STRINGS > 1) ? ((float)s / (float)(NUM_STRINGS - 1) - 0.5f) * 2.0f * mConfig.stereoSpread : 0.0f;
      float panNorm = 0.5f * (pan + 1.0f);
      resSumL += delayedSample * std::cos(panNorm * 1.5707963f);
      resSumR += delayedSample * std::sin(panNorm * 1.5707963f);
    }

    // Mix Dry and Wet
    inOutL[i] = (rawInL * (1.0f - mConfig.dryWet)) + (resSumL * mConfig.dryWet * 0.7f);
    if (inOutR)
    {
      inOutR[i] = (rawInR * (1.0f - mConfig.dryWet)) + (resSumR * mConfig.dryWet * 0.7f);
    }
  }
}

void JZPhysicalResonator::ProcessMono(float* inOut, int numSamples)
{
  Process(inOut, nullptr, numSamples);
}
