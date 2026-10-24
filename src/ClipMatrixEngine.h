//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Non-Linear Session View Clip Matrix & Live Launcher Grid
//
// Pattern-based session launcher engine (Ableton Live / Logic Live Loops paradigm)
// supporting quantized scene triggering (1 Bar, 1/2 Bar, 1/4 Bar), per-track
// clip exclusivity, live looping, and arrangement recording.
//*****************************************************************************

#pragma once

#include <vector>
#include <string>
#include <memory>
#include <algorithm>

// State of an individual clip cell in matrix
enum JZClipCellState
{
  CELL_EMPTY = 0,
  CELL_STOPPED,
  CELL_QUEUED_PLAY,
  CELL_PLAYING,
  CELL_QUEUED_STOP
};

// Launch trigger behavior
enum JZClipLaunchMode
{
  LAUNCH_TRIGGER = 0,        // Start playing on next quantize boundary
  LAUNCH_GATE,               // Play while held, stop on release
  LAUNCH_TOGGLE              // Alternate play/stop on each trigger
};

// Simplified MIDI note event in matrix cell
struct JZMatrixMidiNote
{
  int pitch;                 // 0 to 127
  int velocity;              // 1 to 127
  int startTick;             // Offset within clip loop
  int durationTicks;

  JZMatrixMidiNote(int p = 60, int v = 100, int start = 0, int dur = 96)
    : pitch(p), velocity(v), startTick(start), durationTicks(dur)
  {
  }
};

// Individual Clip Cell
struct JZClipCell
{
  int trackIndex;
  int sceneIndex;
  std::string name;
  std::string colorHex;
  JZClipCellState state;
  JZClipLaunchMode launchMode;
  int lengthTicks;           // e.g. 384 = 1 Bar (96 PPQN * 4 beats)
  int playheadTicks;
  bool isLooping;
  std::vector<JZMatrixMidiNote> notes;

  JZClipCell()
    : trackIndex(0),
      sceneIndex(0),
      name("Clip"),
      colorHex("#00e5ff"),
      state(CELL_EMPTY),
      launchMode(LAUNCH_TRIGGER),
      lengthTicks(384),
      playheadTicks(0),
      isLooping(true)
  {
  }
};

// Recorded arrangement event from live session
struct JZSessionArrangementEvent
{
  int trackIndex;
  int startTick;
  int durationTicks;
  std::string clipName;
  std::vector<JZMatrixMidiNote> notes;
};

// Clip Matrix Engine
class JZClipMatrixEngine
{
public:
  static const int NUM_TRACKS = 8;
  static const int NUM_SCENES = 8;

  JZClipMatrixEngine();
  ~JZClipMatrixEngine() = default;

  // Cell management
  void SetCell(int track, int scene, const JZClipCell& cell);
  const JZClipCell& GetCell(int track, int scene) const;
  JZClipCell& GetCell(int track, int scene);
  void ClearCell(int track, int scene);

  // Live Performance Launch Controls
  void LaunchCell(int track, int scene, int quantizeBars = 1);
  void LaunchScene(int scene, int quantizeBars = 1);
  void StopTrack(int track, int quantizeBars = 1);
  void StopAll(int quantizeBars = 1);

  // Time & Tick Transport
  void AdvanceTicks(int deltaTicks, std::vector<JZMatrixMidiNote>& outTriggeredNotes);

  // Session Recording to Linear Arrangement
  void StartRecording();
  void StopRecording();
  bool IsRecording() const { return mIsRecording; }
  const std::vector<JZSessionArrangementEvent>& GetRecordedArrangement() const { return mRecordedEvents; }

  int GetGlobalTick() const { return mGlobalTick; }
  int GetBarLengthTicks() const { return mTicksPerBar; }

private:
  int mTicksPerBar;          // Default 384 (96 PPQN * 4)
  int mGlobalTick;
  bool mIsRecording;

  JZClipCell mMatrix[NUM_TRACKS][NUM_SCENES];
  int mQueuedBarLaunch[NUM_TRACKS]; // Queued scene per track, or -2 for stop, -1 for none

  std::vector<JZSessionArrangementEvent> mRecordedEvents;
};
