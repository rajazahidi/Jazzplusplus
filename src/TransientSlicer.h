//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Audio Transient Slicing & Groove Beat Slicer / Beat Detective
//
// Energy Differential Onset Detection, Non-Destructive Drum Slicing,
// and Automatic "Slice to MIDI Track" Chromatic Trigger Generation.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

class JZTrack;

// Transient Hit Point Slice Marker
struct JZSliceMarker
{
  int sampleIndex;           // Sample offset in audio buffer
  int clockPosition;         // Timeline tick position
  float peakStrength;        // Onset energy strength [0.0, 1.0]
  int midiPitch;             // Assigned chromatic trigger note (default 60 = C3, 61 = C#3, ...)

  JZSliceMarker(int s = 0, int clk = 0, float str = 1.0f, int p = 60)
    : sampleIndex(s),
      clockPosition(clk),
      peakStrength(str),
      midiPitch(p)
  {
  }

  bool operator<(const JZSliceMarker& other) const
  {
    return sampleIndex < other.sampleIndex;
  }
};

class JZTransientDetector
{
  public:

    // Detect transient hit onsets in mono audio buffer
    static std::vector<JZSliceMarker> DetectOnsets(
      const float* pSamples,
      int numSamples,
      int sampleRate = 44100,
      float sensitivity = 0.5f,
      int minDistanceMs = 60,
      int startClock = 0,
      double bpm = 120.0,
      int ticksPerQuarter = 96);

    // Slice audio drum loop and generate chromatic MIDI trigger track
    static int SliceToMidiTrack(
      JZTrack* pTargetTrack,
      const std::vector<JZSliceMarker>& slices,
      int channel = 9,
      int basePitch = 60,
      int defaultNoteLength = 48);

  private:

    static float CalculateWindowEnergy(
      const float* pSamples,
      int start,
      int windowSize);
};
