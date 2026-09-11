//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Copyright (C) 2026 Raja Zahidi
// MIDI Effects Suite — Arpeggiator, Humanizer, Harmonizer, MIDI Echo
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//*****************************************************************************

#include "MidiEffects.h"
#include "ChordScaleData.h"

#ifndef JAZZ_UNIT_TEST
#include "Track.h"
#include "Events.h"
#endif

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <map>
#include <random>
#include <set>

using namespace std;

// ============================================================================
// Helper Random Number Generator
// ============================================================================
#ifndef JAZZ_UNIT_TEST
static int GetRandomInRange(int minVal, int maxVal)
{
  if (minVal >= maxVal)
  {
    return minVal;
  }
  return minVal + (rand() % (maxVal - minVal + 1));
}
#endif

// ============================================================================
// 1. JZMidiArpeggiator Implementation
// ============================================================================
int JZMidiArpeggiator::GetRateTicks(JZArpRate rate, int ticksPerQuarter)
{
  if (ticksPerQuarter <= 0)
  {
    ticksPerQuarter = 480;
  }

  switch (rate)
  {
    case eArpRate_Quarter:
      return ticksPerQuarter;
    case eArpRate_Eighth:
      return ticksPerQuarter / 2;
    case eArpRate_Sixteenth:
      return ticksPerQuarter / 4;
    case eArpRate_ThirtySecond:
      return ticksPerQuarter / 8;
    case eArpRate_EighthTriplet:
      return (ticksPerQuarter * 2) / 3;
    case eArpRate_SixteenthTriplet:
      return ticksPerQuarter / 3;
    default:
      return ticksPerQuarter / 4;
  }
}

vector<int> JZMidiArpeggiator::GeneratePitchSequence(
  const vector<int>& chordPitches,
  JZArpPattern pattern,
  int octaves,
  const string& scaleName,
  int scaleRoot)
{
  vector<int> result;
  if (chordPitches.empty())
  {
    return result;
  }

  // Remove duplicates and sort ascending
  set<int> uniquePitches(chordPitches.begin(), chordPitches.end());
  vector<int> sortedBase(uniquePitches.begin(), uniquePitches.end());

  if (octaves < 1)
  {
    octaves = 1;
  }
  if (octaves > 4)
  {
    octaves = 4;
  }

  // Multi-octave expansion
  vector<int> expanded;
  for (int oct = 0; oct < octaves; ++oct)
  {
    for (size_t i = 0; i < sortedBase.size(); ++i)
    {
      int p = sortedBase[i] + oct * 12;
      if (p >= 0 && p <= 127)
      {
        expanded.push_back(p);
      }
    }
  }

  if (expanded.empty())
  {
    return result;
  }

  // Apply optional scale filter if scale is provided
  if (!scaleName.empty())
  {
    const vector<JZScaleDefinition>& scales = JZChordScaleLibrary::GetScales();
    const JZScaleDefinition* pScale = 0;
    for (size_t i = 0; i < scales.size(); ++i)
    {
      if (scales[i].mName == scaleName)
      {
        pScale = &scales[i];
        break;
      }
    }

    if (pScale && !pScale->mIntervals.empty())
    {
      vector<int> scaleFiltered;
      for (size_t i = 0; i < expanded.size(); ++i)
      {
        int p = expanded[i];
        int chroma = ((p - scaleRoot) % 12 + 12) % 12;
        bool inScale = false;
        for (size_t j = 0; j < pScale->mIntervals.size(); ++j)
        {
          if (pScale->mIntervals[j] % 12 == chroma)
          {
            inScale = true;
            break;
          }
        }
        if (inScale)
        {
          scaleFiltered.push_back(p);
        }
      }
      if (!scaleFiltered.empty())
      {
        expanded = scaleFiltered;
      }
    }
  }

  switch (pattern)
  {
    case eArpUp:
      result = expanded;
      break;

    case eArpDown:
      result = expanded;
      reverse(result.begin(), result.end());
      break;

    case eArpUpDown:
      {
        result = expanded;
        if (expanded.size() > 2)
        {
          for (int i = (int)expanded.size() - 2; i > 0; --i)
          {
            result.push_back(expanded[i]);
          }
        }
      }
      break;

    case eArpDownUp:
      {
        result = expanded;
        reverse(result.begin(), result.end());
        if (expanded.size() > 2)
        {
          for (size_t i = 1; i < expanded.size() - 1; ++i)
          {
            result.push_back(expanded[i]);
          }
        }
      }
      break;

    case eArpRandom:
      {
        result = expanded;
        // Deterministic pseudo-random shuffle
        for (size_t i = result.size() - 1; i > 0; --i)
        {
          size_t j = rand() % (i + 1);
          swap(result[i], result[j]);
        }
      }
      break;

    case eArpChordPulse:
      result = expanded;
      break;
  }

  return result;
}

#ifndef JAZZ_UNIT_TEST
bool JZMidiArpeggiator::Apply(
  JZTrack* pTrack,
  int startClock,
  int endClock,
  int ticksPerQuarter,
  const JZArpSettings& settings)
{
  if (!pTrack)
  {
    return false;
  }

  int stepTicks = GetRateTicks(settings.rate, ticksPerQuarter);
  if (stepTicks <= 0)
  {
    stepTicks = ticksPerQuarter / 4;
  }

  int gateTicks = (stepTicks * settings.gatePercent) / 100;
  if (gateTicks < 1)
  {
    gateTicks = 1;
  }

  // Collect key-on events in the range
  JZEventIterator iterator(pTrack);
  JZEvent* pEvent = iterator.Range(startClock, endClock);
  vector<JZKeyOnEvent*> keyOnEvents;
  while (pEvent)
  {
    JZKeyOnEvent* k = pEvent->IsKeyOn();
    if (k)
    {
      keyOnEvents.push_back(k);
    }
    pEvent = iterator.Next();
  }

  if (keyOnEvents.empty())
  {
    return false;
  }

  // Group notes into simultaneous chord blocks (within 10 ticks tolerance)
  vector<vector<JZKeyOnEvent*> > chordGroups;
  vector<JZKeyOnEvent*> currentGroup;
  int currentClock = -1;

  for (size_t i = 0; i < keyOnEvents.size(); ++i)
  {
    JZKeyOnEvent* k = keyOnEvents[i];
    if (currentGroup.empty() || abs(k->GetClock() - currentClock) <= 10)
    {
      currentGroup.push_back(k);
      if (currentGroup.size() == 1)
      {
        currentClock = k->GetClock();
      }
    }
    else
    {
      chordGroups.push_back(currentGroup);
      currentGroup.clear();
      currentGroup.push_back(k);
      currentClock = k->GetClock();
    }
  }
  if (!currentGroup.empty())
  {
    chordGroups.push_back(currentGroup);
  }

  // Process each chord group
  for (size_t g = 0; g < chordGroups.size(); ++g)
  {
    const vector<JZKeyOnEvent*>& group = chordGroups[g];
    int chordStart = group[0]->GetClock();
    int chordDuration = group[0]->GetLength();
    int defaultVel = group[0]->GetVelocity();
    int ch = group[0]->GetChannel();

    // Determine group duration up to next group or chord length
    if (g + 1 < chordGroups.size())
    {
      int nextStart = chordGroups[g + 1][0]->GetClock();
      chordDuration = min(chordDuration, max(stepTicks, nextStart - chordStart));
    }

    vector<int> pitches;
    for (size_t n = 0; n < group.size(); ++n)
    {
      pitches.push_back(group[n]->GetKey());
      // Remove original event
      pTrack->Kill(group[n]);
    }

    vector<int> seq = GeneratePitchSequence(
      pitches,
      settings.pattern,
      settings.octaves,
      settings.scaleName,
      settings.scaleRoot);

    if (seq.empty())
    {
      continue;
    }

    int vel = (settings.velocity > 0) ? settings.velocity : defaultVel;
    int numSteps = chordDuration / stepTicks;
    if (numSteps < 1)
    {
      numSteps = 1;
    }

    if (settings.pattern == eArpChordPulse)
    {
      for (int step = 0; step < numSteps; ++step)
      {
        int noteClk = chordStart + step * stepTicks;
        for (size_t pi = 0; pi < seq.size(); ++pi)
        {
          pTrack->Put(new JZKeyOnEvent(noteClk, ch, seq[pi], vel, gateTicks));
        }
      }
    }
    else
    {
      for (int step = 0; step < numSteps; ++step)
      {
        int noteClk = chordStart + step * stepTicks;
        int pitch = seq[step % seq.size()];
        pTrack->Put(new JZKeyOnEvent(noteClk, ch, pitch, vel, gateTicks));
      }
    }
  }

  pTrack->Cleanup();
  return true;
}
#else
bool JZMidiArpeggiator::Apply(
  JZTrack*,
  int,
  int,
  int,
  const JZArpSettings&)
{
  return true;
}
#endif

// ============================================================================
// 2. JZMidiHumanizer Implementation
// ============================================================================
#ifndef JAZZ_UNIT_TEST
bool JZMidiHumanizer::Apply(
  JZTrack* pTrack,
  int startClock,
  int endClock,
  const JZHumanizeSettings& settings)
{
  if (!pTrack)
  {
    return false;
  }

  JZEventIterator iterator(pTrack);
  JZEvent* pEvent = iterator.Range(startClock, endClock);
  vector<JZKeyOnEvent*> notesToModify;

  while (pEvent)
  {
    JZKeyOnEvent* k = pEvent->IsKeyOn();
    if (k)
    {
      notesToModify.push_back(k);
    }
    pEvent = iterator.Next();
  }

  if (notesToModify.empty())
  {
    return false;
  }

  for (size_t i = 0; i < notesToModify.size(); ++i)
  {
    JZKeyOnEvent* k = notesToModify[i];
    int clk = k->GetClock();
    int vel = k->GetVelocity();
    int len = k->GetLength();
    int ch = k->GetChannel();
    int pitch = k->GetKey();

    // 1. Swing offset
    if (settings.swingPercent != 50 && settings.swingGridTicks > 0)
    {
      int gridStep = clk / settings.swingGridTicks;
      if (gridStep % 2 == 1)
      {
        // Off-beat note gets swung
        double swingRatio = (settings.swingPercent - 50.0) / 50.0; // 0.0 to 0.5
        int swingDelta = static_cast<int>(settings.swingGridTicks * 0.33 * swingRatio);
        clk += swingDelta;
      }
    }

    // 2. Timing Jitter
    if (settings.timingJitterTicks > 0)
    {
      int jitter = GetRandomInRange(-settings.timingJitterTicks, settings.timingJitterTicks);
      clk = max(0, clk + jitter);
    }

    // 3. Velocity Jitter
    if (settings.velocityJitter > 0)
    {
      int vJitter = GetRandomInRange(-settings.velocityJitter, settings.velocityJitter);
      vel = min(127, max(1, vel + vJitter));
    }

    // Remove old and insert modified
    pTrack->Kill(k);
    pTrack->Put(new JZKeyOnEvent(clk, ch, pitch, vel, len));
  }

  pTrack->Cleanup();
  return true;
}
#else
bool JZMidiHumanizer::Apply(
  JZTrack*,
  int,
  int,
  const JZHumanizeSettings&)
{
  return true;
}
#endif

// ============================================================================
// 3. JZMidiHarmonizer Implementation
// ============================================================================
vector<int> JZMidiHarmonizer::GetHarmonizedPitches(
  int basePitch,
  JZHarmonizeInterval interval,
  int scaleRoot,
  const string& scaleName)
{
  vector<int> harmonized;

  // Simple chromatic octave shifts
  if (interval == eHarmOctaveAbove)
  {
    if (basePitch + 12 <= 127)
    {
      harmonized.push_back(basePitch + 12);
    }
    return harmonized;
  }
  if (interval == eHarmOctaveBelow)
  {
    if (basePitch - 12 >= 0)
    {
      harmonized.push_back(basePitch - 12);
    }
    return harmonized;
  }

  // Diatonic scale based harmonization
  const vector<JZScaleDefinition>& scales = JZChordScaleLibrary::GetScales();
  const JZScaleDefinition* pScale = 0;
  for (size_t i = 0; i < scales.size(); ++i)
  {
    if (scales[i].mName == scaleName)
    {
      pScale = &scales[i];
      break;
    }
  }

  // Fallback to Major if scale not found
  vector<int> scaleDegrees;
  if (pScale && !pScale->mIntervals.empty())
  {
    scaleDegrees = pScale->mIntervals;
  }
  else
  {
    int defMaj[] = {0, 2, 4, 5, 7, 9, 11};
    scaleDegrees.assign(defMaj, defMaj + 7);
  }

  int chroma = ((basePitch - scaleRoot) % 12 + 12) % 12;
  int octaveBase = basePitch - chroma;

  // Find nearest scale degree index
  int degreeIndex = 0;
  int minDiff = 99;
  for (size_t i = 0; i < scaleDegrees.size(); ++i)
  {
    int diff = abs(scaleDegrees[i] - chroma);
    if (diff < minDiff)
    {
      minDiff = diff;
      degreeIndex = (int)i;
    }
  }

  int scaleSize = (int)scaleDegrees.size();

  auto getPitchAtDegree = [&](int targetIndex) -> int {
    int octShift = 0;
    while (targetIndex >= scaleSize)
    {
      targetIndex -= scaleSize;
      octShift += 12;
    }
    while (targetIndex < 0)
    {
      targetIndex += scaleSize;
      octShift -= 12;
    }
    return octaveBase + scaleDegrees[targetIndex] + octShift;
  };

  switch (interval)
  {
    case eHarmThirdAbove:
      {
        int p = getPitchAtDegree(degreeIndex + 2);
        if (p >= 0 && p <= 127) harmonized.push_back(p);
      }
      break;

    case eHarmThirdBelow:
      {
        int p = getPitchAtDegree(degreeIndex - 2);
        if (p >= 0 && p <= 127) harmonized.push_back(p);
      }
      break;

    case eHarmFifthAbove:
      {
        int p = getPitchAtDegree(degreeIndex + 4);
        if (p >= 0 && p <= 127) harmonized.push_back(p);
      }
      break;

    case eHarmFifthBelow:
      {
        int p = getPitchAtDegree(degreeIndex - 4);
        if (p >= 0 && p <= 127) harmonized.push_back(p);
      }
      break;

    case eHarmDiatonicTriad:
      {
        int p3 = getPitchAtDegree(degreeIndex + 2);
        int p5 = getPitchAtDegree(degreeIndex + 4);
        if (p3 >= 0 && p3 <= 127) harmonized.push_back(p3);
        if (p5 >= 0 && p5 <= 127) harmonized.push_back(p5);
      }
      break;

    case eHarmDiatonicSeventh:
      {
        int p3 = getPitchAtDegree(degreeIndex + 2);
        int p5 = getPitchAtDegree(degreeIndex + 4);
        int p7 = getPitchAtDegree(degreeIndex + 6);
        if (p3 >= 0 && p3 <= 127) harmonized.push_back(p3);
        if (p5 >= 0 && p5 <= 127) harmonized.push_back(p5);
        if (p7 >= 0 && p7 <= 127) harmonized.push_back(p7);
      }
      break;

    default:
      break;
  }

  return harmonized;
}

#ifndef JAZZ_UNIT_TEST
bool JZMidiHarmonizer::Apply(
  JZTrack* pTrack,
  int startClock,
  int endClock,
  const JZHarmonizeSettings& settings)
{
  if (!pTrack)
  {
    return false;
  }

  JZEventIterator iterator(pTrack);
  JZEvent* pEvent = iterator.Range(startClock, endClock);
  vector<JZKeyOnEvent*> originalNotes;

  while (pEvent)
  {
    JZKeyOnEvent* k = pEvent->IsKeyOn();
    if (k)
    {
      originalNotes.push_back(k);
    }
    pEvent = iterator.Next();
  }

  if (originalNotes.empty())
  {
    return false;
  }

  for (size_t i = 0; i < originalNotes.size(); ++i)
  {
    JZKeyOnEvent* k = originalNotes[i];
    int basePitch = k->GetKey();
    int clk = k->GetClock();
    int len = k->GetLength();
    int ch = k->GetChannel();
    int vel = (k->GetVelocity() * settings.velocityRatioPercent) / 100;
    vel = min(127, max(1, vel));

    vector<int> harmonyPitches = GetHarmonizedPitches(
      basePitch,
      settings.interval,
      settings.scaleRoot,
      settings.scaleName);

    for (size_t h = 0; h < harmonyPitches.size(); ++h)
    {
      pTrack->Put(new JZKeyOnEvent(clk, ch, harmonyPitches[h], vel, len));
    }
  }

  pTrack->Cleanup();
  return true;
}
#else
bool JZMidiHarmonizer::Apply(
  JZTrack*,
  int,
  int,
  const JZHarmonizeSettings&)
{
  return true;
}
#endif

// ============================================================================
// 4. JZMidiEcho Implementation
// ============================================================================
#ifndef JAZZ_UNIT_TEST
bool JZMidiEcho::Apply(
  JZTrack* pTrack,
  int startClock,
  int endClock,
  const JZMidiEchoSettings& settings)
{
  if (!pTrack || settings.repeatCount < 1 || settings.delayTicks <= 0)
  {
    return false;
  }

  JZEventIterator iterator(pTrack);
  JZEvent* pEvent = iterator.Range(startClock, endClock);
  vector<JZKeyOnEvent*> baseNotes;

  while (pEvent)
  {
    JZKeyOnEvent* k = pEvent->IsKeyOn();
    if (k)
    {
      baseNotes.push_back(k);
    }
    pEvent = iterator.Next();
  }

  if (baseNotes.empty())
  {
    return false;
  }

  for (size_t i = 0; i < baseNotes.size(); ++i)
  {
    JZKeyOnEvent* k = baseNotes[i];
    int origClk = k->GetClock();
    int origVel = k->GetVelocity();
    int origLen = k->GetLength();
    int ch = k->GetChannel();
    int origPitch = k->GetKey();

    double curVel = origVel;
    int curPitch = origPitch;

    for (int rep = 1; rep <= settings.repeatCount; ++rep)
    {
      curVel *= settings.feedback;
      curPitch += settings.pitchShiftPerRepeat;
      int noteVel = static_cast<int>(curVel + 0.5);

      if (noteVel < 1 || curPitch < 0 || curPitch > 127)
      {
        break;
      }

      int echoClk = origClk + rep * settings.delayTicks;
      pTrack->Put(new JZKeyOnEvent(echoClk, ch, curPitch, noteVel, origLen));
    }
  }

  pTrack->Cleanup();
  return true;
}
#else
bool JZMidiEcho::Apply(
  JZTrack*,
  int,
  int,
  const JZMidiEchoSettings&)
{
  return true;
}
#endif
