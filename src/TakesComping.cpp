//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Loop Recording Takes & Comping System Implementation
//*****************************************************************************

#include "TakesComping.h"
#include "Track.h"
#include "Events.h"

#include <sstream>
#include <algorithm>

using namespace std;

//-----------------------------------------------------------------------------
// JZCompingManager Implementation
//-----------------------------------------------------------------------------
JZCompingManager::JZCompingManager(int trackIndex)
  : mTrackIndex(trackIndex)
{
}

JZCompingManager::~JZCompingManager()
{
  Clear();
}

void JZCompingManager::Clear()
{
  mTakes.clear();
  mCompRegions.clear();
}

const JZTakeLane* JZCompingManager::GetTake(int index) const
{
  if (index < 0 || index >= (int)mTakes.size()) return 0;
  return &mTakes[index];
}

int JZCompingManager::CreateNewTake(const string& takeName)
{
  int newIdx = (int)mTakes.size();
  string name = takeName;
  if (name.empty())
  {
    ostringstream oss;
    oss << "Take " << (newIdx + 1);
    name = oss.str();
  }
  mTakes.push_back(JZTakeLane(newIdx, name));
  return newIdx;
}

void JZCompingManager::SelectCompRegion(int takeIndex, int startClock, int endClock)
{
  if (startClock >= endClock || takeIndex < 0 || takeIndex >= (int)mTakes.size())
  {
    return;
  }

  // Remove or trim existing regions that overlap with new selection
  vector<JZCompRegion> updated;
  for (size_t i = 0; i < mCompRegions.size(); ++i)
  {
    const JZCompRegion& reg = mCompRegions[i];
    // Completely inside new region: discard
    if (reg.startClock >= startClock && reg.endClock <= endClock)
    {
      continue;
    }
    // Completely outside: keep
    if (reg.endClock <= startClock || reg.startClock >= endClock)
    {
      updated.push_back(reg);
    }
    // Overlap left
    else if (reg.startClock < startClock && reg.endClock <= endClock)
    {
      updated.push_back(JZCompRegion(reg.takeIndex, reg.startClock, startClock));
    }
    // Overlap right
    else if (reg.startClock >= startClock && reg.endClock > endClock)
    {
      updated.push_back(JZCompRegion(reg.takeIndex, endClock, reg.endClock));
    }
    // Split middle
    else if (reg.startClock < startClock && reg.endClock > endClock)
    {
      updated.push_back(JZCompRegion(reg.takeIndex, reg.startClock, startClock));
      updated.push_back(JZCompRegion(reg.takeIndex, endClock, reg.endClock));
    }
  }

  updated.push_back(JZCompRegion(takeIndex, startClock, endClock));
  sort(updated.begin(), updated.end());
  mCompRegions = updated;
}

int JZCompingManager::GetActiveTakeAtClock(int clock) const
{
  for (size_t i = 0; i < mCompRegions.size(); ++i)
  {
    if (clock >= mCompRegions[i].startClock && clock < mCompRegions[i].endClock)
    {
      return mCompRegions[i].takeIndex;
    }
  }
  return -1;
}

int JZCompingManager::FlattenCompToTrack(
  const JZCompingManager& compManager,
  const vector<JZTrack*>& takeTracks,
  JZTrack* pMasterCompTrack)
{
  if (!pMasterCompTrack || compManager.mCompRegions.empty() || takeTracks.empty())
  {
    return 0;
  }

  int eventsCopied = 0;

  for (size_t r = 0; r < compManager.mCompRegions.size(); ++r)
  {
    const JZCompRegion& reg = compManager.mCompRegions[r];
    if (reg.takeIndex < 0 || reg.takeIndex >= (int)takeTracks.size()) continue;

    JZTrack* pSrc = takeTracks[reg.takeIndex];
    if (!pSrc) continue;

    for (int i = 0; i < pSrc->mEventCount; ++i)
    {
      JZEvent* pEv = pSrc->mppEvents[i];
      if (!pEv || pEv->IsKilled()) continue;

      int clk = pEv->GetClock();
      if (clk >= reg.startClock && clk < reg.endClock)
      {
        pMasterCompTrack->Put(pEv->Copy());
        eventsCopied++;
      }
    }
  }

  return eventsCopied;
}
