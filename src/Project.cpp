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
#include <wx/config.h>
#include <wx/filename.h>

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
#else
#include "AudioDriver.h"
#endif
#ifdef DEV_ALSA
#include "AlsaPlayer.h"
#include "AlsaDriver.h"
#endif

#include <fstream>
#include <iostream>

using namespace std;

//*****************************************************************************
// Description:
//   This is the Jazz++ project class definition.  This is the top-level class
// for Jazz++.
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
wxString JZProject::mConfFileName = "jazz.cfg";

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZProject::JZProject()
  : mpConfig(0),
    mpMidiPlayer(0),
    mpSynth(0),
    mpRecInfo(0),
    mChanged(false),
    mIsPlaying(false)
{
  if (gLimitSteps.empty())
  {
    gLimitSteps.push_back(make_pair( "1/8",    8));
    gLimitSteps.push_back(make_pair( "1/12",  12));
    gLimitSteps.push_back(make_pair( "1/16",  16));
    gLimitSteps.push_back(make_pair( "1/24",  24));
    gLimitSteps.push_back(make_pair( "1/32",  32));
    gLimitSteps.push_back(make_pair( "1/48",  48));
    gLimitSteps.push_back(make_pair( "1/96",  96));
    gLimitSteps.push_back(make_pair("1/192", 192));
  }

  if (gModes.empty())
  {
    gModes.push_back(make_pair("Set",  8));
    gModes.push_back(make_pair("Add", 12));
    gModes.push_back(make_pair("Sub", 16));
  }

  if (gScaleNames.empty())
  {
    gScaleNames.push_back(make_pair("C",   0));
    gScaleNames.push_back(make_pair("C#",  1));
    gScaleNames.push_back(make_pair("D",   2));
    gScaleNames.push_back(make_pair("D#",  3));
    gScaleNames.push_back(make_pair("E",   4));
    gScaleNames.push_back(make_pair("F",   5));
    gScaleNames.push_back(make_pair("F#",  6));
    gScaleNames.push_back(make_pair("G",   7));
    gScaleNames.push_back(make_pair("G#",  8));
    gScaleNames.push_back(make_pair("A",   9));
    gScaleNames.push_back(make_pair("A#", 10));
    gScaleNames.push_back(make_pair("B",  11));
    gScaleNames.push_back(make_pair("None", gScaleChromatic));
    gScaleNames.push_back(make_pair("Selected", gScaleSelected));
  }

  if (gQntSteps.empty())
  {
    gQntSteps.push_back(make_pair("1/8",   8));
    gQntSteps.push_back(make_pair("1/12", 12));
    gQntSteps.push_back(make_pair("1/16", 16));
    gQntSteps.push_back(make_pair("1/24", 24));
    gQntSteps.push_back(make_pair("1/32", 32));
    gQntSteps.push_back(make_pair("1/48", 48));
    gQntSteps.push_back(make_pair("1/96", 96));
  }

  if (gSynthesizerTypes.empty())
  {
    gSynthesizerTypes.push_back(make_pair("GM", SynthTypeGM));
    gSynthesizerTypes.push_back(make_pair("GS", SynthTypeGS));
    gSynthesizerTypes.push_back(make_pair("XG", SynthTypeXG));
    gSynthesizerTypes.push_back(make_pair("Other", SynthTypeOther));
  }

  if (gSynthesierTypeFiles.empty())
  {
    gSynthesierTypeFiles.push_back(make_pair("gm.jzi", SynthTypeGM));
    gSynthesierTypeFiles.push_back(make_pair("gs.jzi", SynthTypeGS));
    gSynthesierTypeFiles.push_back(make_pair("xg.jzi", SynthTypeXG));
    gSynthesierTypeFiles.push_back(make_pair("other.jzi", SynthTypeOther));
  }

  mpConfig = new tConfig;
  gpConfig = mpConfig;

  ReadConfiguration();

  mNumBars = 0;

  mMetronomeInfo.IsAccented = mpConfig->GetValue(C_MetroIsAccented);
  mMetronomeInfo.Veloc = mpConfig->GetValue(C_MetroVelocity);
  mMetronomeInfo.KeyNorm = mpConfig->GetValue(C_MetroNormalClick);
  mMetronomeInfo.KeyAcc = mpConfig->GetValue(C_MetroAccentedClick);

  if (mpConfig->StrValue(C_SynthType))
  {
    mpSynth = NewSynth(mpConfig->StrValue(C_SynthType));
  }
  else
  {
    mpSynth = NewSynth("GM");
  }
  gpSynth = mpSynth;

  gpSong = this;
  mpRecInfo = new JZRecordingInfo;


  //--------------
  // Linux drivers
  //--------------
#ifndef __WXMSW__
  if (gpConfig->GetValue(C_MidiDriver) == eMidiDriverOss)
  {
#ifdef DEV_SEQUENCER2
    mpMidiPlayer = new tAudioPlayer(this);
    if (!mpMidiPlayer->Installed())
    {
      delete mpMidiPlayer;
      mpMidiPlayer = new tSeq2Player(this);
    }
    if (!mpMidiPlayer->Installed())
    {
      perror("/dev/music");
      cerr
        << "(dev_sequencer2)Jazz will start with no play/record ability."
        << endl;
      mpMidiPlayer = new tNullPlayer(this);
    }
#else
    cerr << "This programm lacks OSS driver support" << endl;
    mpMidiPlayer = new tNullPlayer(this);
#endif // DEV_SEQUENCER2
  }
  else if (gpConfig->GetValue(C_MidiDriver) == eMidiDriverAlsa)
  {
#ifdef DEV_ALSA
    mpMidiPlayer = new tAlsaAudioPlayer(this);
    if (!mpMidiPlayer->Installed())
    {
      delete mpMidiPlayer;
      cout << "creating alsa player" << endl;
      mpMidiPlayer = new tAlsaPlayer(this);
    }
    if (!mpMidiPlayer->Installed())
    {
      cerr
        << "Could not install alsa driver." << '\n'
        << "Jazz will start with no play/record ability."
        << endl;
      mpMidiPlayer = new tNullPlayer(this);
    }
#else
    cerr << "This programm lacks ALSA driver support" << endl;
    mpMidiPlayer = new tNullPlayer(this);
#endif
  }
  else if (gpConfig->GetValue(C_MidiDriver) == eMidiDriverJazz)
  {
#ifdef DEV_MPU401
    mpMidiPlayer = new tMpuPlayer(this);
    if (!mpMidiPlayer->Installed())
    {
      cerr
        << "Could not connect to midinet server at host \"
        << %midinethost << "\"\n"
        << "Jazz will start with no play/record ability."
        << endl;
      mpMidiPlayer = new tNullPlayer(this);
    }
#else
    cerr << "This programm lacks JAZZ/MPU401 driver support" << endl;
    mpMidiPlayer = new tNullPlayer(this);
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
  switch (mpConfig->GetValue(C_ClockSource))
  {
    case CsMidi:
      mpMidiPlayer = new tWinMidiPlayer(this);
      break;
    case CsMtc:
      mpMidiPlayer = new tWinMtcPlayer(this);
      break;
    case CsFsk:
    case CsInt:
    default:
      mpMidiPlayer = new tWinAudioPlayer(this);
      if (!mpMidiPlayer->Installed())
      {
        mpMidiPlayer->ShowError();
        delete mpMidiPlayer;
        mpMidiPlayer = new tWinIntPlayer(this);
      }
      break;
  }
  if (!mpMidiPlayer->Installed())
  {
    mpMidiPlayer->ShowError();
    mpMidiPlayer = new tNullPlayer(this);
  }
#endif // __WXMSW__

  if (!mpMidiPlayer)
  {
    mpMidiPlayer = new tNullPlayer(this);
  }

  gpMidiPlayer = mpMidiPlayer;

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
      mpConfig->Put(i + C_TrackWinXpos, atoi(wxTheApp->argv[opt]));
    }
    else
    {
      break;
    }
  }


  // Attempt to load the song given on command line or the file specified in
  // the configuration file.
  opt = GetOptionIndex( "-f" ) + 1;
  if (opt && (wxTheApp->argc > opt))
  {
    gpStartUpSong = wxTheApp->argv[opt];
  }
  else
  {
    gpStartUpSong = mpConfig->StrValue(C_StartUpSong);
  }
  FILE* fd = fopen(gpStartUpSong.c_str(), "r");
  if (fd)
  {
    fclose(fd);
    tStdRead io;
    Read(io, gpStartUpSong.c_str());
//    if (gpStartUpSong == string("jazz.mid"))
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
  delete mpConfig;
}

//-----------------------------------------------------------------------------
// Description:
//   This function reads the Jazz++ configuration file (jazz.cfg).
//-----------------------------------------------------------------------------
void JZProject::ReadConfiguration()
{
  wxConfigBase* pConfig = wxConfigBase::Get();

  // Get the current working directory and append a directory separator.
  wxString CurrentWorkingDirectory =
    ::wxGetCwd() + wxFileName::GetPathSeparator();

  // Jazz++ should be distributed with a conf subdirectory under the
  // executable directory.  This will be our initial guess for the location of
  // the Jazz++ configuration file.
  wxString ConfFileDirectoryGuess =
    CurrentWorkingDirectory + "conf" + wxFileName::GetPathSeparator();

  // Attempt to obtain the path to the Jazz++ configuration file from the
  // wxWidgets Jazz++ configuration file.
  wxString ConfFilePath;
  bool WasConfPathRead = false;
  if (pConfig)
  {
    WasConfPathRead = pConfig->Read(
      "/Paths/Conf",
      &ConfFilePath,
      ConfFileDirectoryGuess);
  }

  // Construct a full Jazz++ configuration path and file name.
  wxString ConfFileNameAndPath = ConfFilePath + mConfFileName;

  // Test for the existence of the Jazz++ configuration file.
  ifstream Is;
  Is.open(ConfFileNameAndPath.c_str());
  if (!Is)
  {
    // Close and clear the stream.
    Is.close();
    Is.clear();

    // Return a valid path to the data.
    FindAndRegisterConfFilePath(ConfFilePath);
    ConfFileNameAndPath = ConfFilePath + mConfFileName;

    // Try one more time.
    Is.open(ConfFileNameAndPath.c_str());
    if (!Is)
    {
      wxMessageBox(
        "Could not find configuration file.",
        "Warning",
        wxOK);
    }
    Is.close();
    Is.clear();
  }

  cout
    << "JZProject::ReadConfiguration() ConfFileNameAndPath:" << '\n'
    << "  \"" << ConfFileNameAndPath << '"'
    << endl;

  if (!ConfFileNameAndPath.IsEmpty())
  {
    mpConfig->LoadConfig(ConfFileNameAndPath);
    DEBUG(
      if (BankTable != (tDoubleCommand *) NULL)
      {
        for (int i = 0; BankTable[i].Command[0] >= 0; i++)
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
      "Could not find configuration file.",
      "Warning",
      wxOK);
  }
}

//-----------------------------------------------------------------------------
// Description:
//   The configuration file was not automatically found so give the user the
// opportunity to search for it.  If it is found, create a wxWidgets-style
// configuration entry so the code will find the configuration file path the
// next time it starts.
//
// Returns:
//   wxString&:
//     A user selected path to the Jazz++ configuration file.
//-----------------------------------------------------------------------------
void JZProject::FindAndRegisterConfFilePath(wxString& ConfFilePath)
{
  wxString DialogTitle;
  DialogTitle = "Please Indicate the Location of " + mConfFileName;

  // Use an open dialog to find the Jazz++ configuration file.
  // wxFD_CHANGE_DIR - Change the current working directory to the directory
  // where the file(s) chosen by the user are.
  wxFileDialog OpenDialog(
    0,
    DialogTitle,
    "",
    mConfFileName,
    "*.cfg",
    wxFD_OPEN | wxFD_CHANGE_DIR);
  if (OpenDialog.ShowModal() == wxID_OK)
  {
    // Generate a c-style string that contains a path to the help file.
    wxString TempConfFilePath;
    TempConfFilePath = ::wxPathOnly(OpenDialog.GetPath());
    TempConfFilePath += ::wxFileName::GetPathSeparator();

    wxConfigBase* pConfig = wxConfigBase::Get();
    if (pConfig)
    {
      pConfig->Write("/Paths/Conf", TempConfFilePath);
    }

    // Return the user selected help file path.
    ConfFilePath = TempConfFilePath;
  }
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

//-----------------------------------------------------------------------------
// Description:
//   Open a MIDI file.
//
// Inputs:
//   const wxString& SongFileName:
//     Song path and file name.
//-----------------------------------------------------------------------------
void JZProject::OpenSong(const wxString& SongFileName)
{
  tStdRead Io;
  Clear();
  Read(Io, SongFileName);
  mpConfig->Put(C_StartUpSong, SongFileName.c_str());
}

//-----------------------------------------------------------------------------
// Description:
//   Save a MIDI file.  This function will overwrite the file if it already
// exists!
//
// Inputs:
//   const wxString& SongFileName:
//     Song path and file name.
//-----------------------------------------------------------------------------
void JZProject::Save(const wxString& SongFileName)
{
  tStdWrite Io;
  Write(Io, SongFileName);
  mpConfig->Put(C_StartUpSong, SongFileName.c_str());
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
  mpMidiPlayer->StopPlay();
  // Stub
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZProject::SetPlayPosition(int newposition)
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
void JZProject::SetLoopClock(int newclock)
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
JZRecordingInfo* JZProject::GetRecInfo()
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
