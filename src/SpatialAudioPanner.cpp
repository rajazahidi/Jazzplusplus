//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// 3D Spatial Audio & Surround Sound Panner Implementation
//*****************************************************************************

#include "SpatialAudioPanner.h"

JZSpatialAudioPanner::JZSpatialAudioPanner(float sampleRate)
  : mSampleRate(sampleRate > 1000.0f ? sampleRate : 44100.0f),
    mMode(SPATIAL_BINAURAL_3D),
    mDelayWritePos(0),
    mShadowStateL(0.0f), mShadowStateR(0.0f),
    mAirStateL(0.0f), mAirStateR(0.0f)
{
  mDelayLineL.resize(512, 0.0f); // Up to ~11ms delay
  mDelayLineR.resize(512, 0.0f);
}

void JZSpatialAudioPanner::SetSampleRate(float sampleRate)
{
  if (sampleRate > 1000.0f)
  {
    mSampleRate = sampleRate;
  }
}

void JZSpatialAudioPanner::SetPosition(const JZSpatialPosition& pos)
{
  mPos = pos;
  // Normalize azimuth to 0-360
  while (mPos.azimuthDeg < 0.0f) mPos.azimuthDeg += 360.0f;
  while (mPos.azimuthDeg >= 360.0f) mPos.azimuthDeg -= 360.0f;
  mPos.elevationDeg = std::max(-90.0f, std::min(90.0f, mPos.elevationDeg));
  mPos.distanceMeters = std::max(0.1f, mPos.distanceMeters);
}

void JZSpatialAudioPanner::Reset()
{
  std::fill(mDelayLineL.begin(), mDelayLineL.end(), 0.0f);
  std::fill(mDelayLineR.begin(), mDelayLineR.end(), 0.0f);
  mDelayWritePos = 0;
  mShadowStateL = mShadowStateR = 0.0f;
  mAirStateL = mAirStateR = 0.0f;
}

void JZSpatialAudioPanner::ProcessBinaural(const float* inMono, float* outL, float* outR, int numSamples)
{
  if (!inMono || !outL || !outR || numSamples <= 0) return;

  const float speedOfSound = 343.0f; // m/s
  const float headRadius = 0.0875f;  // ~8.75 cm
  const float maxItdSec = headRadius / speedOfSound; // ~0.255 ms directly to side

  // Calculate angle relative to front-back axis in radians
  float azRad = mPos.azimuthDeg * 3.14159265f / 180.0f;
  float sinAz = std::sin(azRad);

  // Woodworth Interaural Time Difference (ITD)
  // Left ear is at 270 deg (sinAz = -1), Right ear is at 90 deg (sinAz = +1)
  float delaySecL = 0.0f;
  float delaySecR = 0.0f;

  if (sinAz >= 0.0f)
  {
    // Source on right side -> right ear leads, left ear delayed
    delaySecL = maxItdSec * (sinAz + std::asin(std::max(0.0f, sinAz)));
  }
  else
  {
    // Source on left side -> left ear leads, right ear delayed
    float absSin = std::fabs(sinAz);
    delaySecR = maxItdSec * (absSin + std::asin(std::max(0.0f, absSin)));
  }

  float delaySamplesL = delaySecL * mSampleRate;
  float delaySamplesR = delaySecR * mSampleRate;

  // Interaural Level Difference (ILD)
  float panNorm = 0.5f * (sinAz + 1.0f); // 0.0 = full left, 1.0 = full right
  float gainL = std::cos(panNorm * 1.5707963f);
  float gainR = std::sin(panNorm * 1.5707963f);

  // Distance attenuation: 1 / max(1, d)
  float distAtt = 1.0f / std::max(1.0f, mPos.distanceMeters);
  gainL *= distAtt;
  gainR *= distAtt;

  // Distance air absorption cutoff: higher distance -> lower cutoff
  float airCutoff = std::max(1500.0f, 20000.0f / std::max(1.0f, mPos.distanceMeters * 0.4f));
  float airAlpha = std::min(0.99f, 2.0f * 3.14159265f * airCutoff / mSampleRate);

  int bufLen = (int)mDelayLineL.size();

  for (int i = 0; i < numSamples; ++i)
  {
    float x = inMono[i];

    // Air absorption low-pass
    mAirStateL += airAlpha * (x - mAirStateL);
    mAirStateR += airAlpha * (x - mAirStateR);
    float filteredX = mAirStateL;

    // Write into circular delay lines
    mDelayLineL[mDelayWritePos] = filteredX;
    mDelayLineR[mDelayWritePos] = filteredX;

    // Read delayed left sample with linear interpolation
    float rPosL = (float)mDelayWritePos - delaySamplesL;
    if (rPosL < 0.0f) rPosL += (float)bufLen;
    int idxL0 = (int)rPosL;
    int idxL1 = (idxL0 + 1) % bufLen;
    float fracL = rPosL - (float)idxL0;
    float sampL = mDelayLineL[idxL0] + fracL * (mDelayLineL[idxL1] - mDelayLineL[idxL0]);

    // Read delayed right sample
    float rPosR = (float)mDelayWritePos - delaySamplesR;
    if (rPosR < 0.0f) rPosR += (float)bufLen;
    int idxR0 = (int)rPosR;
    int idxR1 = (idxR0 + 1) % bufLen;
    float fracR = rPosR - (float)idxR0;
    float sampR = mDelayLineR[idxR0] + fracR * (mDelayLineR[idxR1] - mDelayLineR[idxR0]);

    // Head shadow low-pass on shadowed ear
    float shadowCutoffL = (sinAz > 0.0f) ? std::max(1200.0f, 8000.0f * (1.0f - sinAz * 0.7f)) : 20000.0f;
    float shadowCutoffR = (sinAz < 0.0f) ? std::max(1200.0f, 8000.0f * (1.0f + sinAz * 0.7f)) : 20000.0f;

    float shadowAlphaL = std::min(0.99f, 2.0f * 3.14159265f * shadowCutoffL / mSampleRate);
    float shadowAlphaR = std::min(0.99f, 2.0f * 3.14159265f * shadowCutoffR / mSampleRate);

    mShadowStateL += shadowAlphaL * (sampL - mShadowStateL);
    mShadowStateR += shadowAlphaR * (sampR - mShadowStateR);

    outL[i] = mShadowStateL * gainL;
    outR[i] = mShadowStateR * gainR;

    mDelayWritePos = (mDelayWritePos + 1) % bufLen;
  }
}

void JZSpatialAudioPanner::ProcessSurround51(const float* inMono, JZSurround51Buffer& outSurround, int numSamples)
{
  if (!inMono || numSamples <= 0) return;

  outSurround.Resize(numSamples);

  float az = mPos.azimuthDeg;
  float distAtt = 1.0f / std::max(1.0f, mPos.distanceMeters);

  // Equal power panning vector for 5.1 speakers:
  // L (30 deg), C (0 deg), R (330 deg), Ls (110 deg), Rs (250 deg), LFE (<80Hz)
  float wL = 0.0f, wC = 0.0f, wR = 0.0f, wLs = 0.0f, wRs = 0.0f, wLFE = 0.15f;

  if (az >= 0.0f && az < 90.0f)
  {
    // Front right quadrant
    float frac = az / 90.0f;
    wC = std::cos(frac * 1.57f);
    wR = std::sin(frac * 1.57f);
  }
  else if (az >= 90.0f && az < 180.0f)
  {
    // Rear right quadrant
    float frac = (az - 90.0f) / 90.0f;
    wR = std::cos(frac * 1.57f);
    wRs = std::sin(frac * 1.57f);
  }
  else if (az >= 180.0f && az < 270.0f)
  {
    // Rear left quadrant
    float frac = (az - 180.0f) / 90.0f;
    wRs = std::cos(frac * 1.57f);
    wLs = std::sin(frac * 1.57f);
  }
  else
  {
    // Front left quadrant
    float frac = (az - 270.0f) / 90.0f;
    wLs = std::cos(frac * 1.57f);
    wL = std::sin(frac * 1.57f);
  }

  for (int i = 0; i < numSamples; ++i)
  {
    float s = inMono[i] * distAtt;
    outSurround.L[i] = s * wL;
    outSurround.C[i] = s * wC;
    outSurround.R[i] = s * wR;
    outSurround.Ls[i] = s * wLs;
    outSurround.Rs[i] = s * wRs;
    outSurround.LFE[i] = s * wLFE;
  }
}
