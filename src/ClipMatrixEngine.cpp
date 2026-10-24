//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Non-Linear Session View Clip Matrix & Live Launcher Grid Implementation
//*****************************************************************************

#include "ClipMatrixEngine.h"

JZClipMatrixEngine::JZClipMatrixEngine()
  : mTicksPerBar(384),
    mGlobalTick(0),
    mIsRecording(false)
{
  for (int t = 0; t < NUM_TRACKS; ++t)
  {
    mQueuedBarLaunch[t] = -1;
    for (int s = 0; s < NUM_SCENES; ++s)
    {
      mMatrix[t][s].trackIndex = t;
      mMatrix[t][s].sceneIndex = s;
      mMatrix[t][s].state = CELL_EMPTY;
    }
  }
}

void JZClipMatrixEngine::SetCell(int track, int scene, const JZClipCell& cell)
{
  if (track >= 0 && track < NUM_TRACKS && scene >= 0 && scene < NUM_SCENES)
  {
    mMatrix[track][scene] = cell;
    mMatrix[track][scene].trackIndex = track;
    mMatrix[track][scene].sceneIndex = scene;
    if (mMatrix[track][scene].state == CELL_EMPTY)
    {
      mMatrix[track][scene].state = CELL_STOPPED;
    }
  }
}

const JZClipCell& JZClipMatrixEngine::GetCell(int track, int scene) const
{
  static JZClipCell sEmpty;
  if (track >= 0 && track < NUM_TRACKS && scene >= 0 && scene < NUM_SCENES)
  {
    return mMatrix[track][scene];
  }
  return sEmpty;
}

JZClipCell& JZClipMatrixEngine::GetCell(int track, int scene)
{
  static JZClipCell sEmpty;
  if (track >= 0 && track < NUM_TRACKS && scene >= 0 && scene < NUM_SCENES)
  {
    return mMatrix[track][scene];
  }
  return sEmpty;
}

void JZClipMatrixEngine::ClearCell(int track, int scene)
{
  if (track >= 0 && track < NUM_TRACKS && scene >= 0 && scene < NUM_SCENES)
  {
    mMatrix[track][scene] = JZClipCell();
    mMatrix[track][scene].trackIndex = track;
    mMatrix[track][scene].sceneIndex = scene;
    mMatrix[track][scene].state = CELL_EMPTY;
  }
}

void JZClipMatrixEngine::LaunchCell(int track, int scene, int /*quantizeBars*/)
{
  if (track < 0 || track >= NUM_TRACKS || scene < 0 || scene >= NUM_SCENES) return;

  if (mMatrix[track][scene].state != CELL_EMPTY)
  {
    mQueuedBarLaunch[track] = scene;
    mMatrix[track][scene].state = CELL_QUEUED_PLAY;
  }
}

void JZClipMatrixEngine::LaunchScene(int scene, int quantizeBars)
{
  if (scene < 0 || scene >= NUM_SCENES) return;

  for (int t = 0; t < NUM_TRACKS; ++t)
  {
    if (mMatrix[t][scene].state != CELL_EMPTY)
    {
      LaunchCell(t, scene, quantizeBars);
    }
    else
    {
      StopTrack(t, quantizeBars);
    }
  }
}

void JZClipMatrixEngine::StopTrack(int track, int /*quantizeBars*/)
{
  if (track >= 0 && track < NUM_TRACKS)
  {
    mQueuedBarLaunch[track] = -2; // -2 signals queued stop
    for (int s = 0; s < NUM_SCENES; ++s)
    {
      if (mMatrix[track][s].state == CELL_PLAYING)
      {
        mMatrix[track][s].state = CELL_QUEUED_STOP;
      }
    }
  }
}

void JZClipMatrixEngine::StopAll(int quantizeBars)
{
  for (int t = 0; t < NUM_TRACKS; ++t)
  {
    StopTrack(t, quantizeBars);
  }
}

void JZClipMatrixEngine::StartRecording()
{
  mIsRecording = true;
  mRecordedEvents.clear();
}

void JZClipMatrixEngine::StopRecording()
{
  mIsRecording = false;
}

void JZClipMatrixEngine::AdvanceTicks(int deltaTicks, std::vector<JZMatrixMidiNote>& outTriggeredNotes)
{
  if (deltaTicks <= 0) return;

  int prevTick = mGlobalTick;
  mGlobalTick += deltaTicks;

  // Check for bar boundary transition
  int prevBar = prevTick / mTicksPerBar;
  int currentBar = mGlobalTick / mTicksPerBar;

  if (currentBar > prevBar)
  {
    // Bar quantize boundary crossed! Apply queued scene launches
    for (int t = 0; t < NUM_TRACKS; ++t)
    {
      int targetScene = mQueuedBarLaunch[t];
      if (targetScene >= 0 && targetScene < NUM_SCENES)
      {
        // Stop any currently playing cell on this track
        for (int s = 0; s < NUM_SCENES; ++s)
        {
          if (mMatrix[t][s].state == CELL_PLAYING || mMatrix[t][s].state == CELL_QUEUED_STOP)
          {
            mMatrix[t][s].state = CELL_STOPPED;
            mMatrix[t][s].playheadTicks = 0;
          }
        }

        // Activate new cell
        auto& activeCell = mMatrix[t][targetScene];
        activeCell.state = CELL_PLAYING;
        activeCell.playheadTicks = 0;

        // If recording to arrangement, log event
        if (mIsRecording)
        {
          JZSessionArrangementEvent recEvt;
          recEvt.trackIndex = t;
          recEvt.startTick = currentBar * mTicksPerBar;
          recEvt.durationTicks = activeCell.lengthTicks;
          recEvt.clipName = activeCell.name;
          recEvt.notes = activeCell.notes;
          mRecordedEvents.push_back(recEvt);
        }

        mQueuedBarLaunch[t] = -1;
      }
      else if (targetScene == -2)
      {
        // Queued stop
        for (int s = 0; s < NUM_SCENES; ++s)
        {
          mMatrix[t][s].state = (mMatrix[t][s].state == CELL_EMPTY) ? CELL_EMPTY : CELL_STOPPED;
          mMatrix[t][s].playheadTicks = 0;
        }
        mQueuedBarLaunch[t] = -1;
      }
    }
  }

  // Advance playheads of all currently playing cells and collect note triggers
  for (int t = 0; t < NUM_TRACKS; ++t)
  {
    for (int s = 0; s < NUM_SCENES; ++s)
    {
      auto& cell = mMatrix[t][s];
      if (cell.state != CELL_PLAYING) continue;

      int tStart = cell.playheadTicks;
      int tEnd = tStart + deltaTicks;

      for (const auto& note : cell.notes)
      {
        if (note.startTick >= tStart && note.startTick < tEnd)
        {
          outTriggeredNotes.push_back(note);
        }
      }

      cell.playheadTicks += deltaTicks;
      if (cell.playheadTicks >= cell.lengthTicks)
      {
        if (cell.isLooping && cell.lengthTicks > 0)
        {
          cell.playheadTicks %= cell.lengthTicks;
        }
        else
        {
          cell.state = CELL_STOPPED;
          cell.playheadTicks = 0;
        }
      }
    }
  }
}
