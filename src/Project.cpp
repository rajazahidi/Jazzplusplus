//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Copyright (C) 1994-2000 Andreas Voss and Per Sigmond, all rights reserved.
// Modifications Copyright (C) 2004 Patrick Earl
// Modifications Copyright (C) 2008 Peter J. Stieber
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//*****************************************************************************

#include "WxWidgets.h"

#include "Project.h"
#include "RecordingInfo.h"
#include "Synth.h"
#include "Song.h"
#include "Globals.h"
#include "Filter.h"
#include "Player.h"
#include "StandardFile.h"

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZProject::JZProject()
  : mpMidiPlayer(0),
    mpSynth(0),
    mpSong(0),
    mpRecInfo(0),
    mChanged(false),
    mIsPlaying(false)
{
  mNumBars = 0;

  mMetronomeInfo.IsAccented = Config(C_MetroIsAccented);
  mMetronomeInfo.Veloc = Config(C_MetroVelocity);
  mMetronomeInfo.KeyNorm = Config(C_MetroNormalClick);
  mMetronomeInfo.KeyAcc = Config(C_MetroAccentedClick);

  mpSong = new JZSong;
  mpRecInfo = new JZRecordingInfo;
  gpSong = mpSong;
  mpSynth = NewSynth("GS");
  Synth = mpSynth;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZProject::~JZProject()
{
  delete mpMidiPlayer; 
  delete mpSynth;
  delete mpRecInfo;
  delete mpSong;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool JZProject::IsPlaying()
{
  return mIsPlaying;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool JZProject::HasChanged()
{
  return mChanged;
}

//-----------------------------------------------------------------------------
// Description:
//   Set the song file name.
//-----------------------------------------------------------------------------
void JZProject::SetSong(const wxString& SongFileName)
{
  mSongFileName = SongFileName;
}

//-----------------------------------------------------------------------------
// Description:
//   Set the pattern file name.
//-----------------------------------------------------------------------------
void JZProject::SetPattern(const wxString& PatternFileName)
{
  mPatternFileName = PatternFileName;
}

/**
 *
 *  Open a midi file.  Pass it a wxString containing the path to the file.
 *
 */
void JZProject::OpenSong(const wxString& SongFileName)
{
  tStdRead io;
  Clear();
  Read(io, SongFileName);
}

/**
 *
 *  Save a midi file.  Pass it a wxString containing the path to the file.
 *  Save will overwrite the file if it is already there!
 *
 */
void JZProject::Save(wxString newsong)
{
    tStdWrite io;
    Write(io, newsong);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZProject::Play()
{
  mIsPlaying = true;
  mpMidiPlayer->StartPlay(mStartTime, mStopTime);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZProject::Stop()
{
  mIsPlaying = false;
  mpMidiPlayer->Stop();
  // Stub
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZProject::SetPlayPosition(long newposition)
{
  mStartTime = newposition;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZProject::Mute(bool newmute)
{
  mMuted = newmute;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZProject::SetLoop(bool newloop)
{
  mLoop = newloop;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZProject::SetRecord(bool newrecord)
{
  mRecord = newrecord;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZProject::SetLoopClock(long newclock)
{
  mStopTime = newclock;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
tMetronomeInfo JZProject::GetMetronome()
{
  return mMetronomeInfo;
}

//-----------------------------------------------------------------------------
// Description:
//   Returns a constant pointer to the internal RecInfo member.
//-----------------------------------------------------------------------------
const JZRecordingInfo* JZProject::GetRecInfo()
{
  return mpRecInfo;
}

//-----------------------------------------------------------------------------
// Description:
//   Sets the internal mpRecInfo, used for recording apparently.
// JZProject will take ownership of this pointer, so don't delete it after
// you've made it!
//-----------------------------------------------------------------------------
void JZProject::SetRecInfo(JZRecordingInfo* pRecInfo)
{
  delete mpRecInfo;
  mpRecInfo = pRecInfo;
}
