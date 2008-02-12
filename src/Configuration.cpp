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

#include <wx/filename.h>

#include "Configuration.h"
#include "Synth.h"
#include "FindFile.h"
#include "Globals.h"

#include <iostream>
#include <sstream>

using namespace std;

tConfigEntry::tConfigEntry(const char* pName, int IntegerValue)
  : Type(ConfigEntryTypeInt),
    Name(0),
    Value(IntegerValue),
    StrValue(0)
{
  if (pName)
  {
    Name = new char [strlen(pName) + 1];
    strcpy(Name, pName);
  }
}

tConfigEntry::tConfigEntry(const char* pName, const char* pStringValue)
  : Type(ConfigEntryTypeStr),
    Name(0),
    Value(0),
    StrValue(0)
{
  if (pName)
  {
    Name = new char [strlen(pName) + 1];
    strcpy(Name, pName);
  }

  if (pStringValue)
  {
    StrValue = new char[strlen(pStringValue) + 1];
    strcpy(StrValue, pStringValue);
  }
}

tConfigEntry::tConfigEntry(const char* pName, const string& StringValue)
{
  if (pName)
  {
    Name = new char [strlen(pName) + 1];
    strcpy(Name, pName);
  }

  StrValue = new char[strlen(StringValue.c_str()) + 1];
  strcpy(StrValue, StringValue.c_str());
}

tConfigEntry::tConfigEntry(const char* pName)
  : Type(ConfigEntryTypeEmpty),
    Name(0),
    Value(0),
    StrValue(0)
{
  if (pName)
  {
    Name = new char [strlen(pName) + 1];
    strcpy(Name, pName);
  }
}

tConfigEntry::~tConfigEntry()
{
  delete [] Name;
  delete [] StrValue; 
}

void tConfigEntry::SetStrValue(const char* pStringValue)
{
  delete [] StrValue; 
  if (pStringValue)
  {
    StrValue = new char[strlen(pStringValue) + 1];
    strcpy(StrValue, pStringValue);
  }
}







tConfig::tConfig()
  : mDrumNames(),
    mDrumSets(),
    mCtrlNames(),
    mVoiceNames(),
    mBankTable()
{
  for (int i = 0; i < NumConfigNames; ++i)
  {
    Names[i] = 0;
  }

  const char* pNoneString = "None";

  // search for midi device
  Names[C_Seq2Device] = new tConfigEntry(".device", -1);

  // use /dev/music
  Names[C_MidiDriver] = new tConfigEntry(".driver", 1);

  // Enable audio at startup
  Names[C_EnableAudio] = new tConfigEntry(".enable_audio", 1);

  // Windows midi devices
  Names[C_WinInputDevice] = new tConfigEntry(".win_input_device", -1);
  Names[C_WinOutputDevice] = new tConfigEntry(".win_output_device", -1);

  // ALSA midi devices
  Names[C_AlsaInputDevice] = new tConfigEntry(".alsa_input_device", -1);
  Names[C_AlsaOutputDevice] = new tConfigEntry(".alsa_output_device", -1);

  // ALSA audio devices
  Names[C_AlsaAudioInputDevice] = new tConfigEntry(".alsa_audio_input_device", "hw:0,0");
  Names[C_AlsaAudioOutputDevice] = new tConfigEntry(".alsa_audio_output_device", "hw:0,0");

  // emulate midi thru
  Names[C_SoftThru] = new tConfigEntry(".softthru", 1);

  // mpu401 hardware midi thru
  Names[C_HardThru] = new tConfigEntry(".hardthru", 1);

  // midi clock source (0 = internal)
  Names[C_ClockSource] = new tConfigEntry(".clocksource", 0);

  // send realtime midi messages to midi out
  Names[C_RealTimeOut] = new tConfigEntry(".realtime_out", 0);

  // use GS reverb macro
  Names[C_UseReverbMacro] = new tConfigEntry(".use_reverb_macro", 1);

  // use GS chorus macro
  Names[C_UseChorusMacro] = new tConfigEntry(".use_chorus_macro", 1);

  // Default drum channel is 10
  Names[C_DrumChannel] = new tConfigEntry(".drumchannel", 10);

  // Controller for bank select
  Names[C_BankControlNumber] = new tConfigEntry(".bank_control_number", 0);

  // Controller2 for bank select with two commands
  Names[C_BankControlNumber2] = new tConfigEntry(".bank_2nd_control_number",32);

  // Max number of entries in bank table (two commands)
  Names[C_MaxBankTableEntries] = new tConfigEntry(".max_bank_table_entries",256);

  // Number of columns to draw in Parts dialogs
  Names[C_PartsColumnsMax] = new tConfigEntry(".parts_columns_max",4);

  // Draw tracknames on the right too?
  Names[C_PartsTracknamesRight] = new tConfigEntry(".parts_tracknames_right",1);

  // Maximum number of voice names in .jazz
  Names[C_MaxVoiceNames] = new tConfigEntry(".max_voice_names",317);

  // Use two-command bank select?
  Names[C_UseTwoCommandBankSelect] = new tConfigEntry(".use_two_command_bank_select", 0);

  // Metronome settings
  Names[C_MetroIsAccented] = new tConfigEntry(".metronome_is_accented",1);
  Names[C_MetroVelocity] = new tConfigEntry(".metronome_velocity",127);
  Names[C_MetroNormalClick] = new tConfigEntry(".metronome_normal_click",37);
  Names[C_MetroAccentedClick] = new tConfigEntry(".metronome_accented_click",36);

  // Window geometry settings.
  Names[C_TrackWinXpos] = new tConfigEntry(".trackwin_xpos", 10);
  Names[C_TrackWinYpos] = new tConfigEntry(".trackwin_ypos", 10);
  Names[C_TrackWinWidth] = new tConfigEntry(".trackwin_width", 600);
  Names[C_TrackWinHeight] = new tConfigEntry(".trackwin_height", 400);
  Names[C_PianoWinXpos] = new tConfigEntry(".pianowin_xpos", 30);
  Names[C_PianoWinYpos] = new tConfigEntry(".pianowin_ypos", 30);
  Names[C_PianoWinWidth] = new tConfigEntry(".pianowin_width", 600);
  Names[C_PianoWinHeight] = new tConfigEntry(".pianowin_height", 400);
  Names[C_PartsDlgXpos] = new tConfigEntry(".partsdialog_xpos", 50);
  Names[C_PartsDlgYpos] = new tConfigEntry(".partsdialog_ypos", 50);
  Names[C_TrackDlgXpos] = new tConfigEntry(".trackdialog_xpos", 50);
  Names[C_TrackDlgYpos] = new tConfigEntry(".trackdialog_ypos", 50);
  Names[C_HarmonyXpos] = new tConfigEntry(".harmonybrowser_xpos", 100);
  Names[C_HarmonyYpos] = new tConfigEntry(".harmonybrowser_ypos", 100);
  Names[C_RhythmXpos] = new tConfigEntry(".randomrhythm_xpos", 150);
  Names[C_RhythmYpos] = new tConfigEntry(".randomrhythm_ypos", 150);

  // Show Dialog unless initialized
  Names[C_SynthDialog] = new tConfigEntry(".synth_dialog", 1);

  // Default synthesizer type
  Names[C_SynthType] = new tConfigEntry(
    ".synth_type",
    gSynthesizerTypes[SynthTypeGS].first.c_str());

  // Default synthesizer config file
  Names[C_SynthConfig] = new tConfigEntry(
    ".synth_config",
    gSynthesierTypeFiles[SynthTypeGS].first.c_str());

  // When to send synthesizer reset (0=never, 1=song start, 2=start play)
  Names[C_SendSynthReset] = new tConfigEntry(".send_synth_reset", 1);

  // Current include file
  Names[C_Include] = new tConfigEntry(".include","");

  // Entries with empty values
  Names[C_BankTable] = new tConfigEntry(".bank_table");
  Names[C_VoiceNames] = new tConfigEntry(".voicenames");
  Names[C_DrumSets] = new tConfigEntry(".drumsets");
  Names[C_CtrlNames] = new tConfigEntry(".ctrlnames");
  Names[C_DrumNames] = new tConfigEntry(".drumnames");

  // The startup song
  Names[C_StartUpSong] = new tConfigEntry(".startup_song", "jazz.mid");
  Names[C_OssBug1] = new tConfigEntry(".ossbug1", 0);
  Names[C_OssBug2] = new tConfigEntry(".ossbug2", 0);
  Names[C_DuplexAudio] = new tConfigEntry(".duplex_audio", 0);
  Names[C_ThruInput] = new tConfigEntry(".thru_input", 0);
  Names[C_ThruOutput] = new tConfigEntry(".thru_output", 0);

  //enable/disable welcome
  Names[C_EnableWelcome] = new tConfigEntry(".enable_welcome",1);

  // Other initialization

  for (int i = 0; i < 130; ++i)
  {
    mDrumNames.push_back(make_pair("", i));
  }

  mDrumSets.push_back(make_pair(pNoneString, 0));
  for (int i = 1; i < 130; ++i)
  {
    mDrumSets.push_back(make_pair("", i));
  }

  for (int i = 0; i < 130; ++i)
  {
    mCtrlNames.push_back(make_pair("", i));
  }

  mVoiceNames.push_back(make_pair(pNoneString, 0));
  mVoiceNames.push_back(make_pair("", 0));
}

tConfig::~tConfig()
{
  for (int i = 0; i < NumConfigNames; ++i)
  {
    if (Names[i])
    {
      delete Names[i];
    }
  }
}

pair<string, int>& tConfig::DrumName(unsigned entry)
{
  assert((entry >= 0) && (entry < mDrumNames.size()));
  return mDrumNames[entry];
}

pair<string, int>& tConfig::DrumSet(unsigned entry)
{
  assert((entry >= 0) && (entry < mDrumSets.size()));
  return mDrumSets[entry];
}

pair<string, int>& tConfig::VoiceName(unsigned entry)
{
  assert((entry >= 0) && (entry < mVoiceNames.size()));
  return mVoiceNames[entry];
}

pair<string, int>& tConfig::CtrlName(unsigned entry)
{
   assert((entry >= 0) && (entry < mCtrlNames.size()));
   return mCtrlNames[entry];
}

tDoubleCommand& tConfig::BankEntry(unsigned entry)
{
   assert((entry >= 0) && (entry < mBankTable.size()));
   return mBankTable[entry];
}

int tConfig::Check(const char* pName) const
{
  if (!pName || (pName[0] != '.'))
  {
    return -1;
  }

  for (int i = 0; i < NumConfigNames; i++)
  {
    if (!Names[i])
    {
      continue;
    }
    if (!strncmp(pName, Names[i]->GetName(), strlen(Names[i]->GetName())))
    {
      // Found
      return i;
    }
  }
  return -1;
}

//   Return the Jazz++ configuration file name, normally jazz.cfg.
// Search in the path provided by FindFile()
wxString tConfig::File()
{
  wxString FileName = FindFile("jazz.cfg");
  if(FileName.IsEmpty())
  {
    FileName = FindFile(".jazz");
  }
  return FileName;
}

int tConfig::Load(char* buf)
{
   int entry = Check(buf);

   if (entry < 0)
      return entry;

   char format[100];
   int result = 1;
   if (Names[entry]->GetType() == ConfigEntryTypeInt)
   {
      sprintf(format, "%s %%d", Names[entry]->GetName());
      int Value;
      result = sscanf(buf, format, &Value);
      Names[entry]->SetValue(Value);
   }
   else if (Names[entry]->GetType() == ConfigEntryTypeStr)
   {
      // allow whitespace inside entries like "C:\Program Files\JazzWare"
      int ofs = strlen(Names[entry]->GetName());
      while (buf[ofs] == ' ' || buf[ofs] == '\t')  // not \n
        ofs++;
      int end = strlen(buf) - 1;
      while (end > ofs) {
        if (!isspace(buf[end]))
          break;
        end--;
      }
      int size = end - ofs + 1;

      char* pStringValue = new char[size + 1];
      memcpy(pStringValue, buf + ofs, size);
      pStringValue[size] = 0;
      Names[entry]->SetStrValue(pStringValue);
      delete [] pStringValue;
   }
   else
   {
      result = 1;
   }

   if (result <= 0)
   {
      return -1;
   }
   else
   {
      return entry;
   }
}

const int& tConfig::GetValue(const char* pName) const
{
  int i = Check(pName);

  assert(i >= 0);

  return Names[i]->GetValue();
}

const int& tConfig::GetValue(int Index) const
{
  assert((Index >= 0) && (Index < NumConfigNames));
  return Names[Index]->GetValue();
}

bool tConfig::Get(int entry, char *value)
{
   assert((entry >= 0) && (entry < NumConfigNames));

   wxString FileName = File();
   if (FileName.IsEmpty())
   {
     return false;
   }

   FILE *fd = fopen(FileName.c_str(), "r");
   const char* name = Name(entry);

   int  len = strlen(name);
   char buf[1000];
   bool found = false;
   while (!found && fgets(buf, sizeof(buf), fd) != NULL)
   {
      if (strncmp(buf, name, len) == 0)
      {
	 while (isspace(buf[len]))
	    len++;
	 int end = strlen(buf) - 1;
	 while (end > 0 && isspace(buf[end]))
	    buf[end--] = 0;
	 strcpy(value, buf + len);
	 found = true;
      }
   }
   fclose(fd);
   return found;
}

bool tConfig::Get(int entry, long &value)
{
  char buf[512];
  if (Get(entry, buf))
  {
    sscanf(buf, " %ld ", &value);
    return true;
  }
  return false;
}

// Description:
//   Write a configuration entry by making a temp file, and copying all
// entries to there.  If the name/value pair is found, replace it, otherwise
// write it.  Finally copy the temp file over the old configuration file.
bool tConfig::Put(int Index, const char *value)
{
  assert((Index >= 0) && (Index < NumConfigNames));

  wxString FileName = File();
  if (FileName.IsEmpty())
  {
    return false;
  }

  char tempname[512];
  strcpy(tempname, FileName.c_str());
  strcat(tempname, ".tmp"); //make the temp file name
  FILE *out = fopen(tempname, "w");
  if (!out)
  {
    return false;
  }

  FILE* inp = fopen(FileName.c_str(), "r");
  const char* name = Name(Index);

  int  len = strlen(name);
  char buf[1000];
  bool found = false;
  while (fgets(buf, sizeof(buf), inp) != NULL)
  {
    if (strncmp(buf, name, len) == 0)
    {
      fprintf(out, "%s %s\n", name, value);
      found = true;
    }
    else
    {
      fputs(buf, out);
    }
  }
  if (!found)
  {
    fprintf(out, "%s %s\n", name, value);
  }
  fclose(inp);
  fclose(out);
  unlink(FileName.c_str());
  rename(tempname, FileName.c_str());
  return true;
}

bool tConfig::Put(int Index, long Value)
{
  ostringstream Oss;
  Oss << Value;
  return Put(Index, Oss.str().c_str());
}

bool tConfig::Put(int Index)
{
  assert((Index >= 0) && (Index < NumConfigNames));
  Names[Index]->SetValue(Index);
  long LongValue = Names[Index]->GetValue();
  return Put(Index, LongValue);
}

bool tConfig::Put(int Index, int Value)
{
  assert((Index >= 0) && (Index < NumConfigNames));
  Names[Index]->SetValue(Value);
  long LongValue = Names[Index]->GetValue();
  return Put(Index, LongValue);
}

//-----------------------------------------------------------------------------
// Description:
//   Load the configuration from a file.  This code supports file inclusion.
//-----------------------------------------------------------------------------
void tConfig::LoadConfig(const wxString& FileName)
{
  wxString OriginalCurrentWorkingDirectory = ::wxGetCwd();

  wxFileName FileNameObject(FileName);

  wxString Path = FileNameObject.GetPath();
  ::wxSetWorkingDirectory(Path);

  char buf[1000];
  int i, j;
  unsigned BankIndex = 0, VoiceIndex = 0, DrumsetIndex = 0;

  vector<pair<string, int> >* pVector = 0;

  const int MaxIncs = 10;

  FILE *FdArr[MaxIncs];
  int IncLevel = 0;

  for (i = 0; i < MaxIncs; ++i)
  {
    FdArr[i] = NULL;
  }

  cout << "tConfig::LoadConfig \"" << FileName << '"' << endl;

  FdArr[IncLevel] = fopen(FileName.c_str(), "r");
  cout << FileName << endl;
  if (FdArr[IncLevel] == NULL)
  {
    wxMessageBox(
      "Error reading config file.\n"
        "Please check permissions and set the environment variable\n"
        "JAZZ to the installation directory",
      "Warning",
      wxOK);
    ::wxSetWorkingDirectory(OriginalCurrentWorkingDirectory);
    return;
  }

  while (1)
  {
    // Read a line from the current file
    if (fgets(buf, sizeof(buf), FdArr[IncLevel]) == NULL)
    {
      fclose(FdArr[IncLevel]);
      FdArr[IncLevel] = NULL;
      --IncLevel;
      if (IncLevel < 0)
      {
        // Last line of jazz.cfg (.jazz)
        break;
      }
      else
      {
        // Last line of current include-file
        continue;
      }
    }

    int entry;

    // Read keyword lines
    if ((entry = gpConfig->Load(buf)) >= 0)
    {
      switch (entry)
      {
        case C_BankTable:

          pVector = 0;

          // If this is the first bank table entry, create the table.
          if (mBankTable.empty())
          {
            mBankTable.clear();
            tDoubleCommand DoubleCommand;
            for (i = 0; i <= GetValue(C_MaxBankTableEntries); ++i)
            {
              DoubleCommand.Command[0] = -1;
              DoubleCommand.Command[1] = -1;
              mBankTable.push_back(DoubleCommand);
            }
          }
          break;
        case C_VoiceNames:
          if (mVoiceNames.size() == 2)
          {
            mVoiceNames.clear();

            mVoiceNames.push_back(make_pair("None", i));
            for (i = 0; i < GetValue(C_MaxVoiceNames); i++)
            {
              mVoiceNames.push_back(make_pair("", i));
            }
          }
          pVector = &mVoiceNames;
          break;
        case C_DrumSets:
          pVector = &mDrumSets;
          break;
        case C_CtrlNames:
          pVector = &mCtrlNames;
          break;
        case C_DrumNames:
          pVector = &mDrumNames;
          break;
        case C_SynthConfig:
        case C_Include:
          {
            // include file
            wxString pathname = FindFile(StrValue(entry));
            cout << "include "<< entry<<endl;
            IncLevel++;
            assert(IncLevel < MaxIncs);
            if (pathname)
            {
              FdArr[IncLevel] = fopen(pathname,"r");
            }
            else
            {
              FdArr[IncLevel] = NULL;
            }

            if (FdArr[IncLevel] == NULL)
            {
              wxString String;
              String
                << "Could not open config include file \"" << buf << "\"";
              wxMessageBox(String, "Warning", wxOK);
              --IncLevel;
            }
          }
          break;

        default:
          break;
      }
    }

    // Read named value entries
    else if (pVector && isdigit(buf[0]))
    {
      // Voice names
      if (pVector == &mVoiceNames)
      {
        assert(0 <= VoiceIndex && VoiceIndex < mVoiceNames.size());

        long val;
        sscanf(buf, " %ld %n", &val, &j);
        if (gpConfig->GetValue(C_UseTwoCommandBankSelect))
        {
          assert(0 <= val && val <= 65536);
        }
        else
        {
          assert(0 <= val && val <= 32639);
        }
        mVoiceNames[VoiceIndex + 1].second = val + 1;

        buf[strlen(buf) - 1] = 0;	// cut off \n
        mVoiceNames[VoiceIndex + 1].first = buf + j;

        ++VoiceIndex;
      }

      // Drumset names
      else if (pVector == &mDrumSets)
      {
        long val;
        assert(0 <= DrumsetIndex && DrumsetIndex < 129);
        sscanf(buf, " %ld %n", &val, &j);
        if (gpConfig->GetValue(C_UseTwoCommandBankSelect))
        {
          assert(0 <= val && val <= 65536);
        }
        else
        {
          assert(0 <= val && val <= 32639);
        }
        mDrumSets[DrumsetIndex + 1].second = val + 1;

        buf[strlen(buf) - 1] = 0;	// cut off \n
        mDrumSets[DrumsetIndex + 1].first = buf + j;

        ++DrumsetIndex;
      }

      // Controller names.
      else if (pVector == &mCtrlNames)
      {
        sscanf(buf, " %d %n", &i, &j);
        assert(0 <= i && i <= 127);
        buf[strlen(buf) - 1] = 0;	// cut off \n

        mCtrlNames[i + 1].first = buf + j;
      }

      // Drum instrument names.
      else if (pVector == &mDrumNames)
      {
        sscanf(buf, " %d %n", &i, &j);
        assert(0 <= i && i <= 127);
        buf[strlen(buf) - 1] = 0;	// cut off \n

        mDrumNames[i + 1].first = buf + j;
      }
      else
      {
        wxString String;
        String
          << "LoadConfig: error reading line" << "\n"
          << buf;
        wxMessageBox(String, "Warning", wxOK);
      }
    }

    // Read bank table entries.
    else if (pVector == 0 && !mBankTable.empty()&& isdigit(buf[0]))
    {
      assert(0 <= BankIndex && BankIndex < mBankTable.size());

      sscanf(buf, " %d %d", &i, &j);

      assert(0 <= i && i <= 255);
      assert(0 <= j && j <= 255);

      mBankTable[BankIndex].Command[0] = i;
      mBankTable[BankIndex].Command[1] = j;

      ++BankIndex;
    }
  }

  ::wxSetWorkingDirectory(OriginalCurrentWorkingDirectory);
}
