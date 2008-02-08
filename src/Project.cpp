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
#include "Synth.h"
#include "Song.h"
#include "Filter.h"
#include "Player.h"
#include "StandardFile.h"

jppProject::jppProject()
  : Midi(0),
    Synth(0),
    mRecInfo(0)
{
//  mRecInfo->Track = 0;
//  mRecInfo->Muted = 0;
  mNumBars = 0;
  mMetronomeInfo.IsAccented = Config(C_MetroIsAccented);
  mMetronomeInfo.Veloc = Config(C_MetroVelocity);
  mMetronomeInfo.KeyNorm = Config(C_MetroNormalClick);
  mMetronomeInfo.KeyAcc = Config(C_MetroAccentedClick);
}

jppProject::~jppProject()
{
  delete Midi; 
  delete mRecInfo;
}

bool jppProject::IsPlaying()
{
  return mIsPlaying;
}

bool jppProject::HasChanged()
{
  return mChanged;
}

void jppProject::SetSong(wxString newsong)
{
  mSongFileName = newsong;
}

void jppProject::SetPattern(wxString newpattern)
{
  mPatternFileName = newpattern;
}

void jppProject::Play()
{
  mIsPlaying = true;
  Midi->StartPlay(mStartTime, mStopTime);
}

/**
 *
 *  Open a midi file.  Pass it a wxString containing the path to the file.
 *
 */
void jppProject::OpenSong(wxString newsong)
{
  tStdRead io;
  Clear();
  Read(io, newsong);
}

/**
 *
 *  Save a midi file.  Pass it a wxString containing the path to the file.
 *  Save will overwrite the file if it is already there!
 *
 */
void jppProject::Save(wxString newsong)
{
    tStdWrite io;
    Write(io, newsong);
}

void jppProject::Stop()
{
  mIsPlaying = false;
  Midi->Stop();
  // Stub
}

void jppProject::SetPlayPosition(long newposition)
{
  mStartTime = newposition;
}

void jppProject::Mute(bool newmute)
{
  mMuted = newmute;
}

void jppProject::SetLoop(bool newloop)
{
  mLoop = newloop;
}

void jppProject::SetRecord(bool newrecord)
{
  mRecord = newrecord;
}

void jppProject::SetLoopClock(long newclock)
{
  mStopTime = newclock;
}

tMetronomeInfo jppProject::GetMetronome()
{
  return mMetronomeInfo;
}

/**
 *  Returns a const pointer to the internal RecInfo member.
 */
tRecordInfo* jppProject::GetRecInfo()
{
  return mRecInfo;
}

/**
 *
 * Sets the internal mRecInfo, used for recording apparently.  jppProject will take ownership
 * of this pointer, so don't destroy it after you've made it!
 *
 */
void jppProject::SetRecInfo(tRecordInfo* newRecInfo)
{
  delete mRecInfo;
  mRecInfo = newRecInfo;
}
