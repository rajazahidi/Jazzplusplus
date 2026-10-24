//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Real-Time Audio Time-Stretching & Pitch-Shifting Engine Implementation
//*****************************************************************************

#include "AudioTimeStretch.h"

#include <cmath>
#include <cstring>
#include <algorithm>

using namespace std;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

JZAudioTimeStretcher::JZAudioTimeStretcher()
{
}

JZAudioTimeStretcher::~JZAudioTimeStretcher()
{
}

float JZAudioTimeStretcher::ComputeCrossCorrelation(
  const float* pRef,
  const float* pCandidate,
  int length,
  int channels)
{
  float sum = 0.0f;
  int totalSamples = length * channels;
  for (int i = 0; i < totalSamples; i += channels)
  {
    sum += pRef[i] * pCandidate[i];
  }
  return sum;
}

int JZAudioTimeStretcher::ProcessWSOLA(
  const float* pInput,
  int inputFrames,
  float* pOutput,
  int maxOutputFrames,
  double stretchRatio,
  int channels,
  int sampleRate)
{
  if (!pInput || !pOutput || inputFrames <= 0 || maxOutputFrames <= 0 || channels <= 0)
  {
    return 0;
  }

  stretchRatio = max(0.25, min(4.0, stretchRatio));

  // If stretch ratio is 1.0 (no change), perform direct copy
  if (fabs(stretchRatio - 1.0) < 0.005)
  {
    int toCopy = min(inputFrames, maxOutputFrames);
    memcpy(pOutput, pInput, toCopy * channels * sizeof(float));
    return toCopy;
  }

  const int windowSize = 1024;
  const int synHop = 256;
  const int anaHop = (int)(synHop / stretchRatio);
  const int searchRange = 128;

  if (inputFrames < (windowSize + searchRange * 2))
  {
    int toCopy = min(inputFrames, maxOutputFrames);
    memcpy(pOutput, pInput, toCopy * channels * sizeof(float));
    return toCopy;
  }

  // Pre-calculate Hanning window
  vector<float> window(windowSize);
  for (int n = 0; n < windowSize; ++n)
  {
    window[n] = (float)(0.5 * (1.0 - cos(2.0 * M_PI * n / (double)(windowSize - 1))));
  }

  // Zero-initialize output buffer
  memset(pOutput, 0, maxOutputFrames * channels * sizeof(float));

  int inPos = 0;
  int outPos = 0;

  while ((outPos + windowSize) < maxOutputFrames && (inPos + windowSize + searchRange) < inputFrames)
  {
    int targetInPos = inPos;
    int bestDelta = 0;
    float maxCorr = -1e9f;

    // Search cross-correlation in range [-searchRange, +searchRange]
    if (outPos > 0)
    {
      const float* pRef = pOutput + (outPos * channels);
      for (int delta = -searchRange; delta <= searchRange; ++delta)
      {
        int candPos = targetInPos + delta;
        if (candPos < 0 || (candPos + windowSize) >= inputFrames) continue;

        const float* pCand = pInput + (candPos * channels);
        float corr = ComputeCrossCorrelation(pRef, pCand, min(windowSize, 256), channels);
        if (corr > maxCorr)
        {
          maxCorr = corr;
          bestDelta = delta;
        }
      }
    }

    int chosenInPos = targetInPos + bestDelta;
    if (chosenInPos < 0) chosenInPos = 0;
    if ((chosenInPos + windowSize) >= inputFrames) break;

    // Overlap-add windowed segment to output
    for (int n = 0; n < windowSize; ++n)
    {
      float winVal = window[n];
      for (int ch = 0; ch < channels; ++ch)
      {
        float inSamp = pInput[(chosenInPos + n) * channels + ch];
        pOutput[(outPos + n) * channels + ch] += inSamp * winVal;
      }
    }

    outPos += synHop;
    inPos += anaHop;
  }

  // Normalize overlap-add energy
  float normGain = (float)(synHop / (double)windowSize) * 2.0f;
  if (normGain > 1.5f) normGain = 1.0f;
  for (int s = 0; s < outPos * channels; ++s)
  {
    pOutput[s] *= normGain;
  }

  return outPos;
}

int JZAudioTimeStretcher::ProcessPitchShift(
  const float* pInput,
  int inputFrames,
  float* pOutput,
  int maxOutputFrames,
  double semitones,
  int channels,
  int sampleRate)
{
  if (!pInput || !pOutput || inputFrames <= 0 || maxOutputFrames <= 0)
  {
    return 0;
  }

  semitones = max(-12.0, min(12.0, semitones));
  if (fabs(semitones) < 0.02)
  {
    int toCopy = min(inputFrames, maxOutputFrames);
    memcpy(pOutput, pInput, toCopy * channels * sizeof(float));
    return toCopy;
  }

  // Pitch shift ratio: 2^(semitones / 12)
  double pitchRatio = pow(2.0, semitones / 12.0);

  // Resample input by pitchRatio
  int resampledFrames = (int)(inputFrames / pitchRatio);
  vector<float> resampled(resampledFrames * channels, 0.0f);

  for (int f = 0; f < resampledFrames; ++f)
  {
    double srcPos = (double)f * pitchRatio;
    int idx0 = (int)srcPos;
    int idx1 = min(inputFrames - 1, idx0 + 1);
    double frac = srcPos - (double)idx0;

    for (int ch = 0; ch < channels; ++ch)
    {
      float s0 = pInput[idx0 * channels + ch];
      float s1 = pInput[idx1 * channels + ch];
      resampled[f * channels + ch] = (float)(s0 + frac * (s1 - s0));
    }
  }

  // Compensate duration by WSOLA stretching with stretchRatio = pitchRatio
  // This keeps original duration while pitch is transposed!
  return ProcessWSOLA(
    resampled.data(),
    resampledFrames,
    pOutput,
    maxOutputFrames,
    pitchRatio,
    channels,
    sampleRate);
}

bool JZAudioTimeStretcher::WarpAudioBpm(
  const vector<float>& inputBuffer,
  vector<float>& outputBuffer,
  double sourceBpm,
  double targetBpm,
  int channels,
  int sampleRate)
{
  if (inputBuffer.empty() || sourceBpm <= 10.0 || targetBpm <= 10.0 || channels <= 0)
  {
    return false;
  }

  double stretchRatio = targetBpm / sourceBpm;
  int inputFrames = (int)inputBuffer.size() / channels;
  int estimatedOutputFrames = (int)(inputFrames * stretchRatio * 1.5) + 4096;

  outputBuffer.assign(estimatedOutputFrames * channels, 0.0f);

  int actualFrames = ProcessWSOLA(
    inputBuffer.data(),
    inputFrames,
    outputBuffer.data(),
    estimatedOutputFrames,
    stretchRatio,
    channels,
    sampleRate);

  if (actualFrames <= 0)
  {
    outputBuffer = inputBuffer;
    return false;
  }

  outputBuffer.resize(actualFrames * channels);
  return true;
}
