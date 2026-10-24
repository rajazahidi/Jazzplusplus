//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Dynamic Automation Curves & Visual Envelope Lanes Implementation
//*****************************************************************************

#include "AutomationEngine.h"
#include "Track.h"
#include "Events.h"

#include <cmath>
#include <algorithm>

using namespace std;

//-----------------------------------------------------------------------------
// JZAutomationCurve Implementation
//-----------------------------------------------------------------------------
JZAutomationCurve::JZAutomationCurve()
{
}

JZAutomationCurve::~JZAutomationCurve()
{
}

void JZAutomationCurve::Clear()
{
  mNodes.clear();
}

int JZAutomationCurve::AddNode(int clock, double value, JZInterpolationType interp, double tension)
{
  value = max(0.0, min(1.0, value));
  tension = max(-1.0, min(1.0, tension));

  // Check if a node already exists at exact clock, replace it
  for (size_t i = 0; i < mNodes.size(); ++i)
  {
    if (mNodes[i].clock == clock)
    {
      mNodes[i].value = value;
      mNodes[i].interp = interp;
      mNodes[i].tension = tension;
      return (int)i;
    }
  }

  JZAutomationNode node(clock, value, interp, tension);
  mNodes.push_back(node);
  sort(mNodes.begin(), mNodes.end());

  for (size_t i = 0; i < mNodes.size(); ++i)
  {
    if (mNodes[i].clock == clock)
    {
      return (int)i;
    }
  }
  return (int)mNodes.size() - 1;
}

bool JZAutomationCurve::RemoveNode(int index)
{
  if (index < 0 || index >= (int)mNodes.size())
  {
    return false;
  }
  mNodes.erase(mNodes.begin() + index);
  return true;
}

bool JZAutomationCurve::SetNodeValue(int index, double value)
{
  if (index < 0 || index >= (int)mNodes.size())
  {
    return false;
  }
  mNodes[index].value = max(0.0, min(1.0, value));
  return true;
}

bool JZAutomationCurve::SetNodeClock(int index, int clock)
{
  if (index < 0 || index >= (int)mNodes.size())
  {
    return false;
  }
  mNodes[index].clock = max(0, clock);
  sort(mNodes.begin(), mNodes.end());
  return true;
}

bool JZAutomationCurve::SetNodeInterp(int index, JZInterpolationType interp, double tension)
{
  if (index < 0 || index >= (int)mNodes.size())
  {
    return false;
  }
  mNodes[index].interp = interp;
  mNodes[index].tension = max(-1.0, min(1.0, tension));
  return true;
}

double JZAutomationCurve::InterpolateNodes(
  const JZAutomationNode& n1,
  const JZAutomationNode& n2,
  double t) const
{
  t = max(0.0, min(1.0, t));

  switch (n1.interp)
  {
    case INTERP_HOLD:
      return (t >= 1.0) ? n2.value : n1.value;

    case INTERP_LINEAR:
      return n1.value + t * (n2.value - n1.value);

    case INTERP_BEZIER:
    {
      // Cubic Bezier with tension shaping
      // Tension > 0 bends towards ease-out; Tension < 0 bends towards ease-in
      double p1 = 0.5 + (n1.tension * 0.4);
      p1 = max(0.05, min(0.95, p1));
      double u = 1.0 - t;
      double curveFactor = 3.0 * u * u * t * (p1) + 3.0 * u * t * t * (p1) + (t * t * t);
      curveFactor = max(0.0, min(1.0, curveFactor));
      return n1.value + curveFactor * (n2.value - n1.value);
    }

    case INTERP_EXPONENTIAL:
    {
      // Log/Exponential filter sweep curve
      double expFactor = (exp(t * 3.0) - 1.0) / (exp(3.0) - 1.0);
      return n1.value + expFactor * (n2.value - n1.value);
    }

    case INTERP_S_CURVE:
    {
      // Sigmoid smoothstep: 3*t^2 - 2*t^3
      double sFactor = t * t * (3.0 - 2.0 * t);
      return n1.value + sFactor * (n2.value - n1.value);
    }

    default:
      return n1.value + t * (n2.value - n1.value);
  }
}

double JZAutomationCurve::GetValueAtClock(int clock) const
{
  if (mNodes.empty())
  {
    return 0.5;
  }
  if (clock <= mNodes.front().clock)
  {
    return mNodes.front().value;
  }
  if (clock >= mNodes.back().clock)
  {
    return mNodes.back().value;
  }

  // Binary search for surrounding nodes
  for (size_t i = 0; i < mNodes.size() - 1; ++i)
  {
    if (clock >= mNodes[i].clock && clock <= mNodes[i + 1].clock)
    {
      int clkSpan = mNodes[i + 1].clock - mNodes[i].clock;
      if (clkSpan <= 0)
      {
        return mNodes[i + 1].value;
      }
      double t = (double)(clock - mNodes[i].clock) / (double)clkSpan;
      return InterpolateNodes(mNodes[i], mNodes[i + 1], t);
    }
  }

  return mNodes.back().value;
}

int JZAutomationCurve::GetMidiValueAtClock(int clock, int minVal, int maxVal) const
{
  double norm = GetValueAtClock(clock);
  double mapped = (double)minVal + norm * (double)(maxVal - minVal);
  return (int)(mapped + 0.5);
}

int JZAutomationCurve::RenderToTrack(
  JZTrack* pTrack,
  int channel,
  JZAutomationType type,
  int fromClock,
  int toClock,
  int resolutionTicks,
  int customCC)
{
  if (!pTrack || mNodes.empty())
  {
    return 0;
  }

  if (resolutionTicks < 4) resolutionTicks = 4;
  int startClk = max(0, fromClock);
  int endClk = (toClock > startClk) ? toClock : mNodes.back().clock;
  if (endClk <= startClk) endClk = startClk + 192;

  int lastMidiVal = -999999;
  int eventsGenerated = 0;

  int ccNumber = 7;
  switch (type)
  {
    case AUTO_VOLUME: ccNumber = 7; break;
    case AUTO_PAN: ccNumber = 10; break;
    case AUTO_EXPRESSION: ccNumber = 11; break;
    case AUTO_FILTER_CUTOFF: ccNumber = 74; break;
    case AUTO_FILTER_RES: ccNumber = 71; break;
    case AUTO_REVERB_SEND: ccNumber = 91; break;
    case AUTO_CHORUS_SEND: ccNumber = 93; break;
    case AUTO_CUSTOM_CC: ccNumber = max(0, min(127, customCC)); break;
    default: ccNumber = 7; break;
  }

  for (int clk = startClk; clk <= endClk; clk += resolutionTicks)
  {
    if (type == AUTO_PITCH_BEND)
    {
      int bend = GetMidiValueAtClock(clk, -8192, 8191);
      // Density reduction / thinning
      if (abs(bend - lastMidiVal) >= 32 || clk == startClk || clk == endClk)
      {
        pTrack->Put(new JZPitchEvent(clk, channel, bend));
        lastMidiVal = bend;
        eventsGenerated++;
      }
    }
    else if (type == AUTO_TEMPO)
    {
      int bpm = GetMidiValueAtClock(clk, 20, 300);
      if (abs(bpm - lastMidiVal) >= 1 || clk == startClk || clk == endClk)
      {
        pTrack->Put(new JZSetTempoEvent(clk, bpm));
        lastMidiVal = bpm;
        eventsGenerated++;
      }
    }
    else
    {
      int val7bit = GetMidiValueAtClock(clk, 0, 127);
      if (abs(val7bit - lastMidiVal) >= 1 || clk == startClk || clk == endClk)
      {
        pTrack->Put(new JZControlEvent(clk, channel, (unsigned char)ccNumber, (unsigned char)val7bit));
        lastMidiVal = val7bit;
        eventsGenerated++;
      }
    }
  }

  return eventsGenerated;
}

//-----------------------------------------------------------------------------
// JZAutomationLane Implementation
//-----------------------------------------------------------------------------
JZAutomationLane::JZAutomationLane(int trackIndex, JZAutomationType type, int customCC)
  : mTrackIndex(trackIndex),
    mType(type),
    mCustomCC(customCC),
    mIsVisible(true),
    mIsMuted(false)
{
  mName = GetAutomationTypeName(type);
}

const char* JZAutomationLane::GetAutomationTypeName(JZAutomationType type)
{
  switch (type)
  {
    case AUTO_VOLUME: return "Volume (CC 7)";
    case AUTO_PAN: return "Pan (CC 10)";
    case AUTO_EXPRESSION: return "Expression (CC 11)";
    case AUTO_FILTER_CUTOFF: return "Filter Cutoff (CC 74)";
    case AUTO_FILTER_RES: return "Resonance (CC 71)";
    case AUTO_REVERB_SEND: return "Reverb Send (CC 91)";
    case AUTO_CHORUS_SEND: return "Chorus Send (CC 93)";
    case AUTO_PITCH_BEND: return "Pitch Bend";
    case AUTO_TEMPO: return "Tempo (BPM)";
    case AUTO_CUSTOM_CC: return "Custom MIDI CC";
    default: return "Volume";
  }
}

//-----------------------------------------------------------------------------
// JZAutomationManager Implementation
//-----------------------------------------------------------------------------
JZAutomationManager& JZAutomationManager::Instance()
{
  static JZAutomationManager sInstance;
  return sInstance;
}

void JZAutomationManager::Clear()
{
  mLanes.clear();
}

JZAutomationLane* JZAutomationManager::AddLane(int trackIndex, JZAutomationType type, int customCC)
{
  JZAutomationLane* existing = GetLane(trackIndex, type, customCC);
  if (existing) return existing;

  mLanes.push_back(JZAutomationLane(trackIndex, type, customCC));
  return &mLanes.back();
}

JZAutomationLane* JZAutomationManager::GetLane(int trackIndex, JZAutomationType type, int customCC)
{
  for (size_t i = 0; i < mLanes.size(); ++i)
  {
    if (mLanes[i].GetTrackIndex() == trackIndex && mLanes[i].GetType() == type)
    {
      if (type != AUTO_CUSTOM_CC || mLanes[i].GetCustomCC() == customCC)
      {
        return &mLanes[i];
      }
    }
  }
  return 0;
}

bool JZAutomationManager::RemoveLane(int trackIndex, JZAutomationType type, int customCC)
{
  for (size_t i = 0; i < mLanes.size(); ++i)
  {
    if (mLanes[i].GetTrackIndex() == trackIndex && mLanes[i].GetType() == type)
    {
      if (type != AUTO_CUSTOM_CC || mLanes[i].GetCustomCC() == customCC)
      {
        mLanes.erase(mLanes.begin() + i);
        return true;
      }
    }
  }
  return false;
}

vector<JZAutomationLane*> JZAutomationManager::GetTrackLanes(int trackIndex)
{
  vector<JZAutomationLane*> res;
  for (size_t i = 0; i < mLanes.size(); ++i)
  {
    if (mLanes[i].GetTrackIndex() == trackIndex)
    {
      res.push_back(&mLanes[i]);
    }
  }
  return res;
}

int JZAutomationManager::RenderAllLanesToTrack(JZTrack* pTrack, int trackIndex, int channel, int fromClock, int toClock)
{
  if (!pTrack) return 0;
  int totalEvents = 0;
  vector<JZAutomationLane*> lanes = GetTrackLanes(trackIndex);
  for (size_t i = 0; i < lanes.size(); ++i)
  {
    if (!lanes[i]->IsMuted())
    {
      totalEvents += lanes[i]->GetCurve().RenderToTrack(
        pTrack,
        channel,
        lanes[i]->GetType(),
        fromClock,
        toClock,
        12,
        lanes[i]->GetCustomCC());
    }
  }
  return totalEvents;
}
