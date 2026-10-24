//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Loop Recording Takes & Comping System
//
// Cycle Recording Takes Folders, Swipe-Comping Region Selection,
// and Non-Destructive Master Track Flattening.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <algorithm>

class JZTrack;

// Selected Phrase Region from a specific Take
struct JZCompRegion
{
  int takeIndex;             // Index of the take (0 = Take 1, 1 = Take 2, ...)
  int startClock;            // Start clock position
  int endClock;              // End clock position

  JZCompRegion(int t = 0, int s = 0, int e = 768)
    : takeIndex(t),
      startClock(s),
      endClock(e)
  {
  }

  bool operator<(const JZCompRegion& other) const
  {
    return startClock < other.startClock;
  }
};

// Sub-Lane for a Single Loop Pass
struct JZTakeLane
{
  int takeIndex;
  std::string name;
  bool isMuted;

  JZTakeLane(int idx = 0, const std::string& n = "Take 1")
    : takeIndex(idx),
      name(n),
      isMuted(false)
  {
  }
};

// Track Comping Manager
class JZCompingManager
{
  public:

    JZCompingManager(int trackIndex = 0);
    ~JZCompingManager();

    void Clear();

    int GetTrackIndex() const { return mTrackIndex; }
    int GetTakeCount() const { return (int)mTakes.size(); }
    const JZTakeLane* GetTake(int index) const;

    // Create a new take for the next loop recording cycle
    int CreateNewTake(const std::string& takeName = "");

    // Swipe comping: designate active region for a take
    void SelectCompRegion(int takeIndex, int startClock, int endClock);

    const std::vector<JZCompRegion>& GetCompRegions() const { return mCompRegions; }

    // Identify which take is selected at a given timeline clock
    int GetActiveTakeAtClock(int clock) const;

    // Flatten selected comp regions into a single consolidated master track
    static int FlattenCompToTrack(
      const JZCompingManager& compManager,
      const std::vector<JZTrack*>& takeTracks,
      JZTrack* pMasterCompTrack);

  private:

    int mTrackIndex;
    std::vector<JZTakeLane> mTakes;
    std::vector<JZCompRegion> mCompRegions;
};
