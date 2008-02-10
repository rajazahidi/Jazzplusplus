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
#include "GetOptionIndex.h"

#ifdef __WXMSW__
#include "WindowsPlayer.h"
#include "WindowsAudioInterface.h"
#endif
#ifdef DEV_ALSA
#include "AlsaPlayer.h"
#include "AlsaDriver.h"
#endif

#include <iostream>

using namespace std;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZProject::JZProject()
  : mpMidiPlayer(0),
    mpSynth(0),
    mpRecInfo(0),
    mpSong(0),
    mChanged(false),
    mIsPlaying(false)
{
//  const char* pConfigurationFileName = "/tmp/jazz.cfg";
//  Config.File();
//  cout
//    << "WARNING: Hardcoded the jazz.cfg path, because jazz mysteriously"
//    << " isn't loading the configuration file."
//    <<  endl;
  wxString ConfigurationFileName = Config.File();

  cout
    << "JZJazzPlusPlusApplication::OnInit() ConfigurationFileName:" << '\n'
    << '"' << ConfigurationFileName << '"'
    << endl;

  if (!ConfigurationFileName.IsEmpty())
  {
    Config.LoadConfig(ConfigurationFileName);
    DEBUG(
      if (BankTable != (tDoubleCommand *) NULL)
      {
        for (int i = 0; BankTable[i].Command[0]>=0; i++)
        {
          cerr
            << "Bank " << i << ": "
            << BankTable[i].Command[0]
            << ' ' << BankTable[i].Command[1]
            << endl;
        }
      }
    )
  }
  else
  {
    wxMessageBox(
      "Could not find configuration file.\n"
      "Please set the environment variable JAZZ to the installation directory",
      "Warning",
      wxOK);
  }

  mNumBars = 0;

  mMetronomeInfo.IsAccented = Config(C_MetroIsAccented);
  mMetronomeInfo.Veloc = Config(C_MetroVelocity);
  mMetronomeInfo.KeyNorm = Config(C_MetroNormalClick);
  mMetronomeInfo.KeyAcc = Config(C_MetroAccentedClick);

  if (Config.StrValue(C_SynthType))
  {
    mpSynth = NewSynth(Config.StrValue(C_SynthType));
  }
  else
  {
    mpSynth = NewSynth("GM");
  }
  gpSynth = mpSynth;

  mpSong = new JZSong;
  mpRecInfo = new JZRecordingInfo;
  gpSong = mpSong;


  //--------------
  // Linux drivers
  //--------------
#ifndef __WXMSW__
  if (Config(C_MidiDriver) == eMidiDriverOss)
  {
#ifdef DEV_SEQUENCER2
    mpMidiPlayer = new tAudioPlayer(mpSong);
    if (!mpMidiPlayer->Installed())
    {
      delete mpMidiPlayer;
      mpMidiPlayer = new tSeq2Player(mpSong);
    }
    if (!mpMidiPlayer->Installed())
    {
      perror("/dev/music");
      cerr
        << "(dev_sequencer2)Jazz will start with no play/record ability."
        << endl;
      mpMidiPlayer = new tNullPlayer(mpSong);
    }
#else
    cerr << "This programm lacks OSS driver support" << endl;
    mpMidiPlayer = new tNullPlayer(mpSong);
#endif // DEV_SEQUENCER2
  }
  else if (Config(C_MidiDriver) == eMidiDriverAlsa)
  {
#ifdef DEV_ALSA
    mpMidiPlayer = new tAlsaAudioPlayer(mpSong);
    if (!mpMidiPlayer->Installed())
    {
      delete mpMidiPlayer;
      cout << "creating alsa player" << endl;
      mpMidiPlayer = new tAlsaPlayer(mpSong);
    }
    if (!mpMidiPlayer->Installed())
    {
      cerr
        << "Could not install alsa driver." << '\n'
        << "Jazz will start with no play/record ability."
        << endl;
      mpMidiPlayer = new tNullPlayer(mpSong);
    }
#else
    cerr << "This programm lacks ALSA driver support" << endl;
    mpMidiPlayer = new tNullPlayer(mpSong);
#endif
  }
  else if (Config(C_MidiDriver) == eMidiDriverJazz)
  {
#ifdef DEV_MPU401
    mpMidiPlayer = new tMpuPlayer(mpSong);
    if (!mpMidiPlayer->Installed())
    {
      cerr
        << "Could not connect to midinet server at host \"
        << %midinethost << "\"\n"
        << "Jazz will start with no play/record ability."
        << endl;
      mpMidiPlayer = new tNullPlayer(mpSong);
    }
#else
    cerr << "This programm lacks JAZZ/MPU401 driver support" << endl;
    mpMidiPlayer = new tNullPlayer(mpSong);
#endif
  }
  else
  {
    cerr
      << "No valid driver configured in config file." << '\n'
      << "Jazz will start with no play/record ability"
      << endl;
  }
#endif // !defined(__WXMSW__)


#ifdef __WXMSW__
  //--------------------------
  // Microsoft Windows Drivers
  //--------------------------
  switch (Config(C_ClockSource))
  {
    case CsMidi:
      mpMidiPlayer = new tWinMidiPlayer(mpSong);
      break;
    case CsMtc:
      mpMidiPlayer = new tWinMtcPlayer(mpSong);
      break;
    case CsFsk:
    case CsInt:
    default:
      mpMidiPlayer = new tWinAudioPlayer(mpSong);
      if (!mpMidiPlayer->Installed())
      {
	mpMidiPlayer->ShowError();
        delete mpMidiPlayer;
	mpMidiPlayer = new tWinIntPlayer(mpSong);
      }
      break;
  }
  if (!mpMidiPlayer->Installed())
  {
    mpMidiPlayer->ShowError();
    mpMidiPlayer = new tNullPlayer(mpSong);
  }
#endif // __WXMSW__

  if (!mpMidiPlayer)
  {
    mpMidiPlayer = new tNullPlayer(mpSong);
  }

  //-------------------------------------
  // This is the end of the driver setup.
  //-------------------------------------

  int i;
  int opt;

  opt = GetOptionIndex( "-trackwin" ) + 1;
  for (i = 0; i < 4; i++, opt++)
  {
    if ((wxTheApp->argc > opt) && isdigit(wxTheApp->argv[opt][0]))
    {
      Config(i + C_TrackWinXpos) = atoi(wxTheApp->argv[opt]);
    }
    else
    {
      break;
    }
  }


  // Attempt to load the song given on commandline or load "jazz.mid".
  cout << "load song" << endl;
  opt = GetOptionIndex( "-f" ) + 1;
  if (opt && (wxTheApp->argc > opt))
  {
    gpStartUpSong = copystring(wxTheApp->argv[opt]);
  }
  else
  {
    gpStartUpSong = copystring(Config.StrValue(C_StartUpSong));
  }
  FILE *fd = fopen(gpStartUpSong, "r");
  if (fd)
  {
    fclose(fd);
    tStdRead io;
    mpSong->Read(io, gpStartUpSong);
//    if (strcmp(gpStartUpSong, "jazz.mid"))
//    {
//      lasts = gpStartUpSong;
//    }
  }





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
