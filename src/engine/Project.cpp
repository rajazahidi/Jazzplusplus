/*
**  JazzPlusPlus Midi Sequencer
**
** Some Code Copyright (C) 1994-2000 Andreas Voss and Per Sigmond, all rights reserved.
**    I don't know why it says "All Rights Reserved" and then is licensed GPL
**    But the GPL says I can't change the copyright notice.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
*/

#include "Project.h"
#include "../synth.h"
#include "../song.h"
#include "../filter.h"
#include "../player.h"
#include "../mstdfile.h"

jppProject::jppProject() {
    mRecInfo.Track = 0;
    mRecInfo.Muted = 0;
    mNumBars = 0;
    mMetronomeInfo.IsAccented = Config(C_MetroIsAccented);
    mMetronomeInfo.Veloc = Config(C_MetroVelocity);
    mMetronomeInfo.KeyNorm = Config(C_MetroNormalClick);
    mMetronomeInfo.KeyAcc = Config(C_MetroAccentedClick);
}

void jppProject::Save() {
    tStdWrite io;
    Song->Write(io, mSongFileName);
    mChanged = 0;
    Config.Put(C_StartUpSong, mSongFileName);
}

bool jppProject::IsPlaying() {
    return mIsPlaying;
}

bool jppProject::HasChanged() {
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

void jppProject::OpenSong(wxString newsong) {
    tStdRead io;
    Song->Clear();
    Song->Read(io, newsong);
}

void jppProject::Play() {
    Midi->StartPlay(mStartTime, mStopTime);
}

void jppProject::Stop() {
    // Stub
}

void jppProject::SetPlayPosition(long newposition) {
    mStartTime = newposition;
}

void jppProject::Mute(bool newmute) {
    mMuted = newmute;
}

void jppProject::SetLoop(bool newloop) {
    mLoop = newloop;
}

void jppProject::SetRecord(bool newrecord) {
    mRecord = newrecord;
}

void jppProject::SetLoopClock(long newclock) {
    mStopTime = newclock;
}

tMetronomeInfo jppProject::GetMetronome() {
    return mMetronomeInfo;
}

tRecordInfo jppProject::GetRecInfo() {
    return mRecInfo;
}


