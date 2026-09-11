//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Copyright (C) 1994-2000 Andreas Voss and Per Sigmond, all rights reserved.
// Modifications Copyright (C) 2004 Patrick Earl
// Modifications Copyright (C) 2008-2013 Peter J. Stieber
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

#include "Project.h"

#include "AsciiMidiFile.h"
#include "Filter.h"
#include "GetOptionIndex.h"
#include "Globals.h"
#include "Player.h"
#include "RecordingInfo.h"
#include "Synth.h"
#include "Song.h"
#include "StandardFile.h"
#include "SynthesizerTypeEnums.h"

#ifdef __WXMSW__
#include "WindowsPlayer.h"
#include "WindowsAudioInterface.h"
#elif __WXGTK__
#include "AudioDriver.h"
#elif __WXMAC__
#include "PortMidiPlayer.h"
#endif

#ifdef DEV_ALSA
#include "AlsaPlayer.h"
#include "AlsaDriver.h"
#endif

#include <wx/config.h>
#include <wx/file.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>

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
  : JZSong(),
    mpConfig(0),
    mpMidiPlayer(0),
    mpSynth(0),
    mpRecInfo(0),
    mChanged(false),
    mIsPlaying(false)
{
  if (gLimitSteps.empty())
  {
    gLimitSteps.insert(make_pair(  8, "1/8"));
    gLimitSteps.insert(make_pair( 12, "1/12"));
    gLimitSteps.insert(make_pair( 16, "1/16"));
    gLimitSteps.insert(make_pair( 24, "1/24"));
    gLimitSteps.insert(make_pair( 32, "1/32"));
    gLimitSteps.insert(make_pair( 48, "1/48"));
    gLimitSteps.insert(make_pair( 96, "1/96"));
    gLimitSteps.insert(make_pair(192, "1/192"));
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

  if (gQuantizationSteps.empty())
  {
    gQuantizationSteps.insert(make_pair(8,  "1/8"));
    gQuantizationSteps.insert(make_pair(12, "1/12"));
    gQuantizationSteps.insert(make_pair(16, "1/16"));
    gQuantizationSteps.insert(make_pair(24, "1/24"));
    gQuantizationSteps.insert(make_pair(32, "1/32"));
    gQuantizationSteps.insert(make_pair(48, "1/48"));
    gQuantizationSteps.insert(make_pair(96, "1/96"));
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

  mpConfig = new JZConfiguration;
  gpConfig = mpConfig;

  ReadConfiguration();

  mNumBars = 0;

  mMetronomeInfo.ReadFromConfiguration();

  if (mpConfig->GetStrValue(C_SynthType).empty())
  {
    mpSynth = NewSynth("GM");
  }
  else
  {
    mpSynth = NewSynth(mpConfig->GetStrValue(C_SynthType));
  }
  gpSynth = mpSynth;

  gpSong = this;
  mpRecInfo = new JZRecordingInfo;


  //--------------
  // Linux drivers
  //--------------
#ifdef __WXGTK__
  if (gpConfig->GetValue(C_MidiDriver) == eMidiDriverOss)
  {
#ifdef DEV_SEQUENCER2
    mpMidiPlayer = new JZAudioPlayer(this);
    if (!mpMidiPlayer->IsInstalled())
    {
      cerr << "JZAudioPlayer didn't install." << endl;

      delete mpMidiPlayer;
      mpMidiPlayer = new JZSeq2Player(this);
    }
    if (!mpMidiPlayer->IsInstalled())
    {
      cerr << "JZSeq2Player didn't install." << endl;

      perror("/dev/music");

      cerr << "Jazz will start with no play/record ability." << endl;
      delete mpMidiPlayer;
      mpMidiPlayer = new JZNullPlayer(this);
    }
#else
    cerr << "This programm lacks OSS driver support." << endl;
    cerr << "Jazz will start with no play/record ability." << endl;
    mpMidiPlayer = new JZNullPlayer(this);
#endif // DEV_SEQUENCER2
  }
  else if (gpConfig->GetValue(C_MidiDriver) == eMidiDriverAlsa)
  {
#ifdef DEV_ALSA
    mpMidiPlayer = new JZAlsaAudioPlayer(this);
    if (!mpMidiPlayer->IsInstalled())
    {
      cerr << "JZAlsaAudioPlayer didn't install." << endl;

      delete mpMidiPlayer;
      mpMidiPlayer = new JZAlsaPlayer(this);
    }
    if (!mpMidiPlayer->IsInstalled())
    {
      cerr << "JZAlsaPlayer didn't install." << endl;

      cerr
        << "Could not install alsa driver." << '\n'
        << "Jazz will start with no play/record ability."
        << endl;
      delete mpMidiPlayer;
      mpMidiPlayer = new JZNullPlayer(this);
    }
#else
    cerr << "This programm lacks ALSA driver support" << endl;
    cerr << "Jazz will start with no play/record ability." << endl;
    mpMidiPlayer = new JZNullPlayer(this);
#endif
  }
  else if (gpConfig->GetValue(C_MidiDriver) == eMidiDriverJazz)
  {
#ifdef DEV_MPU401
    mpMidiPlayer = new JZMpuPlayer(this);
    if (!mpMidiPlayer->IsInstalled())
    {
      cerr << "JZMpuPlayer didn't install." << endl;

      cerr
        << "Could not connect to midinet server at host \""
        << %midinethost << "\"\n"
        << "Jazz will start with no play/record ability."
        << endl;
      delete mpMidiPlayer;
      mpMidiPlayer = new JZNullPlayer(this);
    }
#else
    cerr << "This programm lacks JAZZ/MPU401 driver support" << endl;
    cerr << "Jazz will start with no play/record ability." << endl;
    mpMidiPlayer = new JZNullPlayer(this);
#endif
  }
  else
  {
    cerr
      << "No valid driver configured in config file." << '\n'
      << "Jazz will start with no play/record ability"
      << endl;
  }
#endif // defined(__WXGTK__)


#ifdef __WXMSW__
  //--------------------------
  // Microsoft Windows Drivers
  //--------------------------
  switch (mpConfig->GetValue(C_ClockSource))
  {
    case CsMidi:
      mpMidiPlayer = new JZWindowsMidiPlayer(this);
      break;
    case CsMtc:
      mpMidiPlayer = new JZWindowsMtcPlayer(this);
      break;
    case CsFsk:
    case CsInt:
    default:
      mpMidiPlayer = new JZWindowsAudioPlayer(this);
      if (!mpMidiPlayer->IsInstalled())
      {
        mpMidiPlayer->ShowError();
        delete mpMidiPlayer;
        mpMidiPlayer = new JZWindowsIntPlayer(this);
      }
      break;
  }
  if (!mpMidiPlayer->IsInstalled())
  {
    mpMidiPlayer->ShowError();
    delete mpMidiPlayer;
    mpMidiPlayer = new JZNullPlayer(this);
  }
#endif // __WXMSW__

#ifdef __WXMAC__
  //------------------
  // Macintosh Drivers
  //------------------
  JZPortMidiPlayer* pPortMidiPlayer = new JZPortMidiPlayer(this);
  pPortMidiPlayer->DeviceSelectionDialog();

  if (!pPortMidiPlayer->IsInstalled())
  {
    delete pPortMidiPlayer;
    cout << "Jazz++ will start with no play/record ability." << endl;
  }
  else
  {
    mpMidiPlayer = pPortMidiPlayer;
  }
#endif // __WXMAC__

  if (!mpMidiPlayer)
  {
    mpMidiPlayer = new JZNullPlayer(this);
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
      mpConfig->Put(i + C_TrackWinXpos, atoi(wxTheApp->argv[opt].c_str()));
    }
    else
    {
      break;
    }
  }

  // Attempt to load the song given on command line or the file specified in
  // the configuration file.
  string StartUpSong;
  opt = GetOptionIndex( "-f" ) + 1;
  if (opt && (wxTheApp->argc > opt))
  {
    StartUpSong = wxTheApp->argv[opt];
  }
  else
  {
    StartUpSong = mpConfig->GetStrValue(C_StartUpSong);
  }

  if (wxFileName::IsFileReadable(StartUpSong.c_str()))
  {
    JZStandardRead Io;
    Read(Io, StartUpSong.c_str());
    wxFileName SplFile(StartUpSong.c_str());
    SplFile.SetExt("spl");
    if (SplFile.FileExists() && mpMidiPlayer)
    {
      mpMidiPlayer->GetSampleSet()->Load(SplFile.GetFullPath());
    }
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

  wxString ConfigurationFilePath;
  if (pConfig)
  {
    pConfig->Read("/Paths/Conf", &ConfigurationFilePath);
  }

  std::vector<wxString> candidateDirs;
  if (!ConfigurationFilePath.empty())
  {
    candidateDirs.push_back(ConfigurationFilePath);
  }

  // 1. User config directory
  candidateDirs.push_back(wxStandardPaths::Get().GetUserDataDir());

  // 2. Next to executable in conf/ and exeDir
  wxString exeDir = ::wxPathOnly(wxStandardPaths::Get().GetExecutablePath());
  if (!exeDir.empty())
  {
    candidateDirs.push_back(exeDir + wxFileName::GetPathSeparator() + "conf");
    candidateDirs.push_back(exeDir);
  }

  // 3. Application data dir in conf/ and dataDir
  wxString dataDir = wxStandardPaths::Get().GetDataDir();
  if (!dataDir.empty())
  {
    candidateDirs.push_back(dataDir + wxFileName::GetPathSeparator() + "conf");
    candidateDirs.push_back(dataDir);
  }

  // 4. Resources dir
  wxString resDir = wxStandardPaths::Get().GetResourcesDir();
  if (!resDir.empty())
  {
    candidateDirs.push_back(resDir + wxFileName::GetPathSeparator() + "conf");
    candidateDirs.push_back(resDir);
  }

  // 5. Current working directory in conf/ and cwd
  candidateDirs.push_back(wxGetCwd() + wxFileName::GetPathSeparator() + "conf");
  candidateDirs.push_back(wxGetCwd());

  // 6. Environment variables
  if (getenv("JAZZ") != 0)
  {
    wxString jazzEnv = getenv("JAZZ");
    candidateDirs.push_back(jazzEnv + wxFileName::GetPathSeparator() + "conf");
    candidateDirs.push_back(jazzEnv);
  }

  // 7. System directories
  candidateDirs.push_back("/usr/local/share/jazz/conf");
  candidateDirs.push_back("/usr/share/jazz/conf");

  wxString foundConfFile;
  wxString foundConfDir;

  for (size_t i = 0; i < candidateDirs.size(); ++i)
  {
    wxString testFile =
      candidateDirs[i] +
      wxFileName::GetPathSeparator() +
      mConfFileName;

    if (::wxFileExists(testFile))
    {
      foundConfFile = testFile;
      foundConfDir = candidateDirs[i];
      break;
    }
  }

  if (!foundConfFile.empty())
  {
    if (pConfig)
    {
      pConfig->Write("/Paths/Conf", foundConfDir);
    }
    mpConfig->LoadConfig(foundConfFile);
    return;
  }

  // Fallback: prompt the user only if jazz.cfg cannot be found anywhere
  if (FindAndRegisterConfFilePath(ConfigurationFilePath))
  {
    wxString JazzCfgFile =
      ConfigurationFilePath +
      wxFileName::GetPathSeparator() +
      mConfFileName;

    if (::wxFileExists(JazzCfgFile))
    {
      mpConfig->LoadConfig(JazzCfgFile);
    }
  }
}

//-----------------------------------------------------------------------------
// Description:
//   The configuration file was not automatically found so give the user the
// opportunity to search for it.  If it is found, create a wxWidgets-style
// configuration entry so the code will find the configuration file path the
// next time it starts.
//
// Outputs:
//   wxString&:
//     A user selected path to the Jazz++ configuration file.
//
// Returns:
//   bool:
//     True if the configuration file path was set; false otherwise.
//-----------------------------------------------------------------------------
bool JZProject::FindAndRegisterConfFilePath(wxString& ConfFilePath) const
{
  wxString DialogTitle;
  DialogTitle = "Please Indicate the Location of " + mConfFileName;

  wxString defaultDir = ::wxPathOnly(wxStandardPaths::Get().GetExecutablePath());
  if (::wxDirExists(defaultDir + wxFileName::GetPathSeparator() + "conf"))
  {
    defaultDir = defaultDir + wxFileName::GetPathSeparator() + "conf";
  }

  // Use an open dialog to find the Jazz++ configuration file.
  wxFileDialog OpenDialog(
    0,
    DialogTitle,
    defaultDir,
    mConfFileName,
    wxString("*.cfg"),
    wxFD_OPEN);

  if (OpenDialog.ShowModal() == wxID_OK)
  {
    // Generate a c-style string that contains a path to the help file.
    wxString TempConfFilePath;
    TempConfFilePath = ::wxPathOnly(OpenDialog.GetPath());

    wxConfigBase* pConfig = wxConfigBase::Get();
    if (pConfig)
    {
      pConfig->Write("/Paths/Conf", TempConfFilePath);
    }

    // Return the user selected help file path.
    ConfFilePath = TempConfFilePath;

    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool JZProject::IsPlaying()
{
  if (mpMidiPlayer && !mpMidiPlayer->IsPlaying())
  {
    mIsPlaying = false;
  }
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
  JZStandardRead Io;
  Clear();
  Read(Io, SongFileName.ToStdString());
  mSongFileName = SongFileName;
  mpConfig->Put(C_StartUpSong, SongFileName.ToStdString());
  wxFileName SplFile(SongFileName);
  SplFile.SetExt("spl");
  if (mpMidiPlayer)
  {
    if (SplFile.FileExists())
    {
      mpMidiPlayer->GetSampleSet()->Load(SplFile.GetFullPath());
    }
    else
    {
      mpMidiPlayer->GetSampleSet()->Clear();
    }
  }
}

//-----------------------------------------------------------------------------
// Description:
//   Open and read an ASCII MIDI file.
//
// Inputs:
//   const wxString& SongFileName:
//     Song path and file name.
//-----------------------------------------------------------------------------
void JZProject::OpenAndReadAsciiMidiFile(const wxString& AsciiMidiFileName)
{
  JZAsciiRead AsciiRead;
  Clear();
  Read(AsciiRead, AsciiMidiFileName.ToStdString());
  mSongFileName = AsciiMidiFileName;
//  mpConfig->Put(C_StartUpSong, SongFileName);
}

//-----------------------------------------------------------------------------
// Description:
//   Save a MIDI file.  This function will overwrite the file if it already
// exists!
//
// Inputs:
//   const wxString& MidiFileName:
//     Song path and file name.
//-----------------------------------------------------------------------------
void JZProject::ExportMidiFile(const wxString& MidiFileName)
{
  JZStandardWrite Io;
  Write(Io, MidiFileName.ToStdString());
  mSongFileName = MidiFileName;
  mpConfig->Put(C_StartUpSong, MidiFileName.ToStdString());
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZProject::ExportAsciiMidiFile(const wxString& AsciiMidiFileName)
{
  JZAsciiWrite AsciiWrite;
  Write(AsciiWrite, AsciiMidiFileName.ToStdString());
  mSongFileName = AsciiMidiFileName;
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
void JZProject::Mute(bool Mute)
{
  mMuted = Mute;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZProject::SetLoop(bool Loop)
{
  mLoop = Loop;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZProject::SetRecord(bool Record)
{
  mRecord = Record;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZProject::SetLoopClock(int Clock)
{
  mStopTime = Clock;
}

//-----------------------------------------------------------------------------
// Description:
//   Returns a constant reference to the metronome.
//-----------------------------------------------------------------------------
const JZMetronomeInfo& JZProject::GetMetronomeInfo() const
{
  return mMetronomeInfo;
}

//-----------------------------------------------------------------------------
// Description:
//   Sets the metronome data.
//-----------------------------------------------------------------------------
void JZProject::SetMetronomeInfo(const JZMetronomeInfo& MetronomeInfo)
{
  mMetronomeInfo = MetronomeInfo;
}

//-----------------------------------------------------------------------------
// Description:
//   Toggles the "is on" state of the metronome.
//-----------------------------------------------------------------------------
void JZProject::ToggleMetronome()
{
  mMetronomeInfo.ToggleIsOn();
}

//-----------------------------------------------------------------------------
// Description:
//   Returns the "is on" state of the metronome.
//-----------------------------------------------------------------------------
bool JZProject::IsMetronomeOn() const
{
  return mMetronomeInfo.IsOn();
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
//-----------------------------------------------------------------------------
void JZProject::EditAudioGlobalSettings(wxWindow* pParent)
{
  if (mpMidiPlayer)
  {
    mpMidiPlayer->EditAudioGlobalSettings(pParent);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZProject::EditAudioSamples(wxWindow* pParent)
{
  if (mpMidiPlayer)
  {
    mpMidiPlayer->EditAudioSamples(pParent);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZProject::LoadSampleSet(wxWindow* pParent)
{
  if (mpMidiPlayer)
  {
    mpMidiPlayer->LoadSampleSet(pParent);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZProject::SaveSampleSetAs(wxWindow* pParent)
{
  if (mpMidiPlayer)
  {
    mpMidiPlayer->SaveSampleSetAs(pParent);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZProject::SaveSampleSet(wxWindow* pParent)
{
  if (mpMidiPlayer)
  {
    mpMidiPlayer->SaveSampleSet(pParent);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZProject::ClearSampleSet(wxWindow* pParent)
{
  if (mpMidiPlayer)
  {
    mpMidiPlayer->ClearSampleSet(pParent);
  }
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
