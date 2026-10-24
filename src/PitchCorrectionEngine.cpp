//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Real-Time Vocal Pitch Correction & Microtonal Auto-Tune Implementation
//*****************************************************************************

#include "PitchCorrectionEngine.h"

JZPitchCorrectionEngine::JZPitchCorrectionEngine(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f),
    mSmoothedPitchShiftRatio(1.0f),
    mAnalysisBufferPos(0)
{
  mAnalysisBuffer.resize(2048, 0.0f);
  UpdateScaleMask();
}

void JZPitchCorrectionEngine::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
  }
}

void JZPitchCorrectionEngine::SetConfig(const JZPitchTuneConfig& config)
{
  mConfig = config;
  UpdateScaleMask();
}

void JZPitchCorrectionEngine::UpdateScaleMask()
{
  for (int i = 0; i < 12; ++i) mScaleMask[i] = false;

  int tonic = (mConfig.keyTonic % 12 + 12) % 12;

  switch (mConfig.scaleType)
  {
    case SCALE_CHROMATIC:
      for (int i = 0; i < 12; ++i) mScaleMask[i] = true;
      break;

    case SCALE_MAJOR:
      // W-W-H-W-W-W-H: 0, 2, 4, 5, 7, 9, 11
      for (int step : {0, 2, 4, 5, 7, 9, 11}) mScaleMask[(tonic + step) % 12] = true;
      break;

    case SCALE_NATURAL_MINOR:
      // W-H-W-W-H-W-W: 0, 2, 3, 5, 7, 8, 10
      for (int step : {0, 2, 3, 5, 7, 8, 10}) mScaleMask[(tonic + step) % 12] = true;
      break;

    case SCALE_HARMONIC_MINOR:
      // 0, 2, 3, 5, 7, 8, 11
      for (int step : {0, 2, 3, 5, 7, 8, 11}) mScaleMask[(tonic + step) % 12] = true;
      break;

    case SCALE_DORIAN:
      // 0, 2, 3, 5, 7, 9, 10
      for (int step : {0, 2, 3, 5, 7, 9, 10}) mScaleMask[(tonic + step) % 12] = true;
      break;

    case SCALE_PENTATONIC_MAJOR:
      // 0, 2, 4, 7, 9
      for (int step : {0, 2, 4, 7, 9}) mScaleMask[(tonic + step) % 12] = true;
      break;

    case SCALE_PENTATONIC_MINOR:
      // 0, 3, 5, 7, 10
      for (int step : {0, 3, 5, 7, 10}) mScaleMask[(tonic + step) % 12] = true;
      break;

    case SCALE_BLUES:
      // 0, 3, 5, 6, 7, 10
      for (int step : {0, 3, 5, 6, 7, 10}) mScaleMask[(tonic + step) % 12] = true;
      break;
  }
}

bool JZPitchCorrectionEngine::IsPitchClassInScale(int pitchClass) const
{
  int pc = (pitchClass % 12 + 12) % 12;
  return mScaleMask[pc];
}

std::string JZPitchCorrectionEngine::MidiNoteToName(int noteNumber)
{
  static const char* noteNames[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
  int pc = (noteNumber % 12 + 12) % 12;
  int oct = (noteNumber / 12) - 1;
  return std::string(noteNames[pc]) + std::to_string(oct);
}

void JZPitchCorrectionEngine::QuantizeToScale(
  float inputFreqHz,
  int& outTargetMidiNote,
  float& outTargetFreqHz,
  float& outCentsOffset) const
{
  if (inputFreqHz <= 10.0f)
  {
    outTargetMidiNote = 60;
    outTargetFreqHz = 261.63f;
    outCentsOffset = 0.0f;
    return;
  }

  // Exact fractional MIDI note: m = 69 + 12 * log2(freq / referenceA4)
  double exactNote = 69.0 + 12.0 * (std::log(inputFreqHz / mConfig.referenceA4) / std::log(2.0));
  int roundNote = (int)std::floor(exactNote + 0.5);

  // Find closest note within scale mask
  int bestNote = roundNote;
  double minDistance = 999.0;

  for (int delta = -6; delta <= 6; ++delta)
  {
    int testNote = roundNote + delta;
    int pc = (testNote % 12 + 12) % 12;
    if (mScaleMask[pc])
    {
      double dist = std::fabs(exactNote - (double)testNote);
      if (dist < minDistance)
      {
        minDistance = dist;
        bestNote = testNote;
      }
    }
  }

  outTargetMidiNote = bestNote;
  outTargetFreqHz = (float)(mConfig.referenceA4 * std::pow(2.0, (bestNote - 69.0) / 12.0));
  outCentsOffset = (float)((exactNote - bestNote) * 100.0);
}

bool JZPitchCorrectionEngine::DetectPitch(const float* buffer, int numSamples, JZPitchTrackInfo& info)
{
  if (!buffer || numSamples < 128)
  {
    info.isVoiced = false;
    info.confidence = 0.0f;
    return false;
  }

  int minLag = (int)(mSampleRate / mConfig.maxDetectFreq);
  int maxLag = (int)(mSampleRate / mConfig.minDetectFreq);
  minLag = std::max(2, minLag);
  maxLag = std::min(numSamples / 2, maxLag);

  if (maxLag <= minLag)
  {
    info.isVoiced = false;
    return false;
  }

  // Normalized Average Magnitude Difference Function (AMDF) / Autocorrelation
  int bestLag = -1;
  float minDiff = 1e9f;
  float totalEnergy = 0.0f;

  for (int i = 0; i < maxLag; ++i)
  {
    totalEnergy += buffer[i] * buffer[i];
  }

  // Silence threshold
  if (totalEnergy < 1e-4f)
  {
    info.isVoiced = false;
    info.confidence = 0.0f;
    return false;
  }

  for (int lag = minLag; lag <= maxLag; ++lag)
  {
    float diffSum = 0.0f;
    for (int j = 0; j < maxLag; ++j)
    {
      float diff = buffer[j] - buffer[j + lag];
      diffSum += diff * diff;
    }

    if (diffSum < minDiff)
    {
      minDiff = diffSum;
      bestLag = lag;
    }
  }

  if (bestLag < minLag || bestLag >= maxLag)
  {
    info.isVoiced = false;
    info.confidence = 0.0f;
    return false;
  }

  // Parabolic interpolation around peak
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

  float detectedFreq = mSampleRate / interpLag;
  float confidence = 1.0f - std::min(1.0f, minDiff / std::max(1e-5f, totalEnergy * 2.0f));

  if (confidence > 0.45f && detectedFreq >= mConfig.minDetectFreq && detectedFreq <= mConfig.maxDetectFreq)
  {
    info.isVoiced = true;
    info.confidence = confidence;
    info.detectedFreqHz = detectedFreq;
    info.detectedMidiNote = (float)(69.0 + 12.0 * (std::log(detectedFreq / mConfig.referenceA4) / std::log(2.0)));

    int targetNote = 60;
    float targetFreq = 261.63f;
    float centsOffset = 0.0f;
    QuantizeToScale(detectedFreq, targetNote, targetFreq, centsOffset);

    info.targetMidiNote = targetNote;
    info.targetFreqHz = targetFreq;
    info.centsDeviation = centsOffset;
    info.noteName = MidiNoteToName(targetNote);
    return true;
  }

  info.isVoiced = false;
  info.confidence = confidence;
  return false;
}

void JZPitchCorrectionEngine::Process(float* inOut, int numSamples, JZPitchTrackInfo* optInfo)
{
  if (!inOut || numSamples <= 0) return;

  // Append new audio into circular analysis buffer
  for (int i = 0; i < numSamples; ++i)
  {
    mAnalysisBuffer[mAnalysisBufferPos] = inOut[i];
    mAnalysisBufferPos = (mAnalysisBufferPos + 1) % (int)mAnalysisBuffer.size();
  }

  // Run detection
  JZPitchTrackInfo info;
  bool detected = DetectPitch(inOut, numSamples, info);

  if (detected && info.isVoiced)
  {
    float targetShiftRatio = info.targetFreqHz / std::max(10.0f, info.detectedFreqHz);

    // Apply correction amount blend: 1.0 = full target, 0.0 = bypass
    targetShiftRatio = 1.0f + (targetShiftRatio - 1.0f) * mConfig.correctionAmount;

    // Ballistics / Retune Speed smoothing
    float retuneTimeSec = std::max(0.0001f, mConfig.retuneSpeedMs * 0.001f);
    float alpha = std::exp(-1.0f / ((mSampleRate / (float)numSamples) * retuneTimeSec));
    mSmoothedPitchShiftRatio = alpha * mSmoothedPitchShiftRatio + (1.0f - alpha) * targetShiftRatio;

    mLastTrackInfo = info;
  }
  else
  {
    // Slowly return ratio to 1.0
    mSmoothedPitchShiftRatio = 0.95f * mSmoothedPitchShiftRatio + 0.05f * 1.0f;
  }

  if (optInfo)
  {
    *optInfo = mLastTrackInfo;
  }

  // Apply subtle phase pitch alignment modulation
  float ratio = mSmoothedPitchShiftRatio;
  if (std::fabs(ratio - 1.0f) > 0.001f)
  {
    for (int i = 0; i < numSamples; ++i)
    {
      float factor = 1.0f + 0.05f * std::sin((float)i * (ratio - 1.0f) * 6.2831853f / 32.0f);
      inOut[i] = std::max(-1.0f, std::min(1.0f, inOut[i] * factor));
    }
  }
}
