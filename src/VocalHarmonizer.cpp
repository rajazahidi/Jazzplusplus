//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Intelligent Vocal Polyphonic Harmonizer Implementation
//*****************************************************************************

#include "VocalHarmonizer.h"

JZVocalHarmonizer::JZVocalHarmonizer(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f)
{
  for (int v = 0; v < 4; ++v)
  {
    mVoiceBuffer[v].resize(4096, 0.0f);
    mWritePos[v] = 0;
    mGrainPhases[v] = 0.0f;
  }
  UpdateScaleDegrees();
}

void JZVocalHarmonizer::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
  }
}

void JZVocalHarmonizer::SetConfig(const JZHarmonizerConfig& config)
{
  mConfig = config;
  UpdateScaleDegrees();
}

void JZVocalHarmonizer::Reset()
{
  for (int v = 0; v < 4; ++v)
  {
    std::fill(mVoiceBuffer[v].begin(), mVoiceBuffer[v].end(), 0.0f);
    mWritePos[v] = 0;
    mGrainPhases[v] = 0.0f;
  }
}

void JZVocalHarmonizer::UpdateScaleDegrees()
{
  int tonic = (mConfig.keyTonic % 12 + 12) % 12;

  if (mConfig.isMinor)
  {
    // Natural Minor: 0, 2, 3, 5, 7, 8, 10
    static const int minSteps[7] = {0, 2, 3, 5, 7, 8, 10};
    for (int i = 0; i < 7; ++i) mScaleDegrees[i] = (tonic + minSteps[i]) % 12;
  }
  else
  {
    // Major: 0, 2, 4, 5, 7, 9, 11
    static const int majSteps[7] = {0, 2, 4, 5, 7, 9, 11};
    for (int i = 0; i < 7; ++i) mScaleDegrees[i] = (tonic + majSteps[i]) % 12;
  }
}

int JZVocalHarmonizer::CalculateDiatonicShift(int inputMidiNote, int diatonicInterval) const
{
  int pc = (inputMidiNote % 12 + 12) % 12;
  int oct = inputMidiNote / 12;

  // Find closest scale degree
  int currentDegree = 0;
  int minDiff = 999;
  for (int i = 0; i < 7; ++i)
  {
    int diff = std::abs(pc - mScaleDegrees[i]);
    if (diff < minDiff)
    {
      minDiff = diff;
      currentDegree = i;
    }
  }

  int targetDegree = currentDegree + diatonicInterval;
  int octOffset = 0;

  while (targetDegree >= 7) { targetDegree -= 7; octOffset++; }
  while (targetDegree < 0)  { targetDegree += 7; octOffset--; }

  int targetMidiNote = (oct + octOffset) * 12 + mScaleDegrees[targetDegree];
  return targetMidiNote - inputMidiNote;
}

void JZVocalHarmonizer::Process(const float* inMono, float* outL, float* outR, int numSamples)
{
  if (!inMono || !outL || !outR || numSamples <= 0) return;

  const int grainSize = 1024;

  for (int i = 0; i < numSamples; ++i)
  {
    float rawIn = inMono[i];
    float wetSumL = 0.0f;
    float wetSumR = 0.0f;

    for (int v = 0; v < 4; ++v)
    {
      const auto& voice = mConfig.voices[v];
      if (!voice.enabled) continue;

      auto& buf = mVoiceBuffer[v];
      int bufLen = (int)buf.size();

      // Write sample to delay buffer
      buf[mWritePos[v]] = rawIn;

      // Determine pitch shift semitones
      int semitones = voice.useDiatonicSnap ? CalculateDiatonicShift(60, voice.diatonicInterval) : voice.chromaticSemitones;
      float pitchRatio = std::pow(2.0f, (float)semitones / 12.0f);

      // Pitch shifting granular playback
      mGrainPhases[v] += (pitchRatio - 1.0f);
      while (mGrainPhases[v] >= (float)grainSize) mGrainPhases[v] -= (float)grainSize;
      while (mGrainPhases[v] < 0.0f)              mGrainPhases[v] += (float)grainSize;

      float rPos = (float)mWritePos[v] - mGrainPhases[v];
      if (rPos < 0.0f) rPos += (float)bufLen;
      int idx0 = (int)rPos;
      int idx1 = (idx0 + 1) % bufLen;
      float frac = rPos - (float)idx0;
      float voiceSample = buf[idx0] + frac * (buf[idx1] - buf[idx0]);

      // Hanning window on grain edges
      float grainNorm = mGrainPhases[v] / (float)grainSize;
      float win = 0.5f * (1.0f - std::cos(2.0f * 3.14159265f * grainNorm));
      voiceSample *= win;

      mWritePos[v] = (mWritePos[v] + 1) % bufLen;

      // Pan & Volume
      float volLinear = std::pow(10.0f, voice.volumeDB / 20.0f);
      float panNorm = 0.5f * (voice.stereoPan + 1.0f);
      float gL = std::cos(panNorm * 1.5707963f) * volLinear;
      float gR = std::sin(panNorm * 1.5707963f) * volLinear;

      wetSumL += voiceSample * gL;
      wetSumR += voiceSample * gR;
    }

    outL[i] = (rawIn * mConfig.dryGain) + (wetSumL * mConfig.wetGain);
    outR[i] = (rawIn * mConfig.dryGain) + (wetSumR * mConfig.wetGain);
  }
}
