//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Audio Transient Slicing & Groove Beat Slicer / Beat Detective
// Implementation
//*****************************************************************************

#include "TransientSlicer.h"
#include "Track.h"
#include "Events.h"

#include <cmath>
#include <algorithm>

using namespace std;

float JZTransientDetector::CalculateWindowEnergy(
  const float* pSamples,
  int start,
  int windowSize)
{
  float sumSq = 0.0f;
  for (int i = 0; i < windowSize; ++i)
  {
    float val = pSamples[start + i];
    sumSq += (val * val);
  }
  return sumSq / (float)windowSize;
}

vector<JZSliceMarker> JZTransientDetector::DetectOnsets(
  const float* pSamples,
  int numSamples,
  int sampleRate,
  float sensitivity,
  int minDistanceMs,
  int startClock,
  double bpm,
  int ticksPerQuarter)
{
  vector<JZSliceMarker> slices;
  if (!pSamples || numSamples <= 512 || sampleRate <= 0)
  {
    return slices;
  }

  sensitivity = max(0.05f, min(0.95f, sensitivity));
  if (minDistanceMs < 20) minDistanceMs = 20;
  if (bpm <= 10.0) bpm = 120.0;
  if (ticksPerQuarter <= 0) ticksPerQuarter = 96;

  int minDistanceSamples = (sampleRate * minDistanceMs) / 1000;
  int windowSize = 256;
  int hopSize = 128;

  double secondsPerTick = (60.0 / bpm) / (double)ticksPerQuarter;

  // Always mark beginning as first slice
  slices.push_back(JZSliceMarker(0, startClock, 1.0f, 60));

  float prevEnergy = 0.0f;
  int lastSliceSample = 0;

  // Dynamic threshold based on sensitivity
  float energyThreshold = 0.005f * (1.0f - (sensitivity * 0.85f));

  for (int s = windowSize; s < numSamples - windowSize; s += hopSize)
  {
    float curEnergy = CalculateWindowEnergy(pSamples, s, windowSize);
    float diff = curEnergy - prevEnergy;

    if (diff > energyThreshold && (s - lastSliceSample) >= minDistanceSamples)
    {
      double timeSec = (double)s / (double)sampleRate;
      int clk = startClock + (int)(timeSec / secondsPerTick);
      float strength = min(1.0f, diff * 10.0f);
      int assignedPitch = 60 + (int)slices.size();
      if (assignedPitch > 100) assignedPitch = 60 + ((int)slices.size() % 40);

      slices.push_back(JZSliceMarker(s, clk, strength, assignedPitch));
      lastSliceSample = s;
    }

    prevEnergy = curEnergy;
  }

  return slices;
}

int JZTransientDetector::SliceToMidiTrack(
  JZTrack* pTargetTrack,
  const vector<JZSliceMarker>& slices,
  int channel,
  int basePitch,
  int defaultNoteLength)
{
  if (!pTargetTrack || slices.empty())
  {
    return 0;
  }

  int count = 0;
  for (size_t i = 0; i < slices.size(); ++i)
  {
    int clk = slices[i].clockPosition;
    int pitch = basePitch + (int)i;
    if (pitch > 120) pitch = 60 + ((int)i % 40);

    int vel = (int)(75.0f + (slices[i].peakStrength * 45.0f));
    vel = max(1, min(127, vel));

    int len = defaultNoteLength;
    if (i + 1 < slices.size())
    {
      int nextClk = slices[i + 1].clockPosition;
      if (nextClk > clk)
      {
        len = min(defaultNoteLength, nextClk - clk);
      }
    }

    pTargetTrack->Put(new JZKeyOnEvent(clk, channel, (unsigned char)pitch, (unsigned char)vel, (unsigned short)len));
    count++;
  }

  return count;
}
