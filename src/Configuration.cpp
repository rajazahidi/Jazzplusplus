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

//*****************************************************************************
// Description:
//   This is the configuration entry class definition.
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZConfigEntry::JZConfigEntry(const char* pName, int IntegerValue)
  : mType(ConfigEntryTypeInt),
    mName(0),
    mValue(IntegerValue),
    mStrValue(0)
{
  if (pName)
  {
    mName = new char [strlen(pName) + 1];
    strcpy(mName, pName);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZConfigEntry::JZConfigEntry(const char* pName, const char* pStringValue)
  : mType(ConfigEntryTypeStr),
    mName(0),
    mValue(0),
    mStrValue(0)
{
  if (pName)
  {
    mName = new char [strlen(pName) + 1];
    strcpy(mName, pName);
  }

  if (pStringValue)
  {
    mStrValue = new char[strlen(pStringValue) + 1];
    strcpy(mStrValue, pStringValue);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZConfigEntry::JZConfigEntry(const char* pName, const string& StringValue)
{
  if (pName)
  {
    mName = new char [strlen(pName) + 1];
    strcpy(mName, pName);
  }

  mStrValue = new char[strlen(StringValue.c_str()) + 1];
  strcpy(mStrValue, StringValue.c_str());
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZConfigEntry::JZConfigEntry(const char* pName)
  : mType(ConfigEntryTypeEmpty),
    mName(0),
    mValue(0),
    mStrValue(0)
{
  if (pName)
  {
    mName = new char [strlen(pName) + 1];
    strcpy(mName, pName);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZConfigEntry::~JZConfigEntry()
{
  delete [] mName;
  delete [] mStrValue;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZConfigEntry::SetStrValue(const char* pStringValue)
{
  delete [] mStrValue;
  if (pStringValue)
  {
    mStrValue = new char[strlen(pStringValue) + 1];
    strcpy(mStrValue, pStringValue);
  }
}







//*****************************************************************************
// Description:
//   This is the configuration class definition.
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZConfiguration::JZConfiguration()
  : mFileName(),
    mDrumNames(),
    mDrumSets(),
    mCtrlNames(),
    mVoiceNames(),
    mBankTable()
{
  for (int i = 0; i < NumConfigNames; ++i)
  {
    mNames[i] = 0;
  }

  const char* pNoneString = "None";

  // search for midi device
  mNames[C_Seq2Device] = new JZConfigEntry(".device", -1);

  // use /dev/music
  mNames[C_MidiDriver] = new JZConfigEntry(".driver", 1);

  // Enable audio at startup.
  mNames[C_EnableAudio] = new JZConfigEntry(".enable_audio", 1);

  // Windows midi devices.
  mNames[C_WinInputDevice] = new JZConfigEntry(".win_input_device", -1);
  mNames[C_WinOutputDevice] = new JZConfigEntry(".win_output_device", -1);

  // ALSA midi devices.
  mNames[C_AlsaInputDevice] = new JZConfigEntry(".alsa_input_device", -1);
  mNames[C_AlsaOutputDevice] = new JZConfigEntry(".alsa_output_device", -1);

  // ALSA audio devices.
  mNames[C_AlsaAudioInputDevice] = new JZConfigEntry(
    ".alsa_audio_input_device",
    "hw:0,0");
  mNames[C_AlsaAudioOutputDevice] = new JZConfigEntry(
    ".alsa_audio_output_device",
    "hw:0,0");

  // Emulate MIDI thru.
  mNames[C_SoftThru] = new JZConfigEntry(".softthru", 1);

  // mpu401 hardware MIDI thru.
  mNames[C_HardThru] = new JZConfigEntry(".hardthru", 1);

  // MIDI clock source (0 = internal).
  mNames[C_ClockSource] = new JZConfigEntry(".clocksource", 0);

  // Send realtime MIDI messages to MIDI out.
  mNames[C_RealTimeOut] = new JZConfigEntry(".realtime_out", 0);

  // Use the GS reverb macro.
  mNames[C_UseReverbMacro] = new JZConfigEntry(".use_reverb_macro", 1);

  // Use the GS chorus macro.
  mNames[C_UseChorusMacro] = new JZConfigEntry(".use_chorus_macro", 1);

  // Default drum channel is 10.
  mNames[C_DrumChannel] = new JZConfigEntry(".drumchannel", 10);

  // Controller for bank select.
  mNames[C_BankControlNumber] = new JZConfigEntry(".bank_control_number", 0);

  // Controller2 for bank select with two commands.
  mNames[C_BankControlNumber2] = new JZConfigEntry(
    ".bank_2nd_control_number",
    32);

  // Maximum number of entries in bank table (two commands).
  mNames[C_MaxBankTableEntries] = new JZConfigEntry(
    ".max_bank_table_entries",
    256);

  // Number of columns to draw in Parts dialogs.
  mNames[C_PartsColumnsMax] = new JZConfigEntry(".parts_columns_max", 4);

  // Draw tracknames on the right too?
  mNames[C_PartsTracknamesRight] = new JZConfigEntry(
    ".parts_tracknames_right",
    1);

  // Maximum number of voice names in .jazz.
  mNames[C_MaxVoiceNames] = new JZConfigEntry(".max_voice_names", 317);

  // Use two-command bank select?
  mNames[C_UseTwoCommandBankSelect] = new JZConfigEntry(
    ".use_two_command_bank_select",
    0);

  // Metronome settings.
  mNames[C_MetroIsAccented] = new JZConfigEntry(
    ".metronome_is_accented",
    1);
  mNames[C_MetroVelocity] = new JZConfigEntry(
    ".metronome_velocity",
    127);
  mNames[C_MetroNormalClick] = new JZConfigEntry(
    ".metronome_normal_click",
    37);
  mNames[C_MetroAccentedClick] = new JZConfigEntry(
    ".metronome_accented_click",
    36);

  // Window geometry settings.
  mNames[C_TrackWinXpos] = new JZConfigEntry(".trackwin_xpos", 10);
  mNames[C_TrackWinYpos] = new JZConfigEntry(".trackwin_ypos", 10);
  mNames[C_TrackWinWidth] = new JZConfigEntry(".trackwin_width", 600);
  mNames[C_TrackWinHeight] = new JZConfigEntry(".trackwin_height", 400);
  mNames[C_PianoWinXpos] = new JZConfigEntry(".pianowin_xpos", 30);
  mNames[C_PianoWinYpos] = new JZConfigEntry(".pianowin_ypos", 30);
  mNames[C_PianoWinWidth] = new JZConfigEntry(".pianowin_width", 600);
  mNames[C_PianoWinHeight] = new JZConfigEntry(".pianowin_height", 400);
  mNames[C_PartsDlgXpos] = new JZConfigEntry(".partsdialog_xpos", 50);
  mNames[C_PartsDlgYpos] = new JZConfigEntry(".partsdialog_ypos", 50);
  mNames[C_TrackDlgXpos] = new JZConfigEntry(".trackdialog_xpos", 50);
  mNames[C_TrackDlgYpos] = new JZConfigEntry(".trackdialog_ypos", 50);
  mNames[C_HarmonyXpos] = new JZConfigEntry(".harmonybrowser_xpos", 100);
  mNames[C_HarmonyYpos] = new JZConfigEntry(".harmonybrowser_ypos", 100);
  mNames[C_RhythmXpos] = new JZConfigEntry(".randomrhythm_xpos", 150);
  mNames[C_RhythmYpos] = new JZConfigEntry(".randomrhythm_ypos", 150);

  // Show dialog unless initialized.
  mNames[C_SynthDialog] = new JZConfigEntry(".synth_dialog", 1);

  // Default synthesizer type.
  mNames[C_SynthType] = new JZConfigEntry(
    ".synth_type",
    gSynthesizerTypes[SynthTypeGS].first.c_str());

  // Default synthesizer configuration file.
  mNames[C_SynthConfig] = new JZConfigEntry(
    ".synth_config",
    gSynthesierTypeFiles[SynthTypeGS].first.c_str());

  // When to send synthesizer reset (0 = never, 1 = song start,
  // 2 = start play).
  mNames[C_SendSynthReset] = new JZConfigEntry(".send_synth_reset", 1);

  // Current include file.
  mNames[C_Include] = new JZConfigEntry(".include", "");

  // Entries with empty values.
  mNames[C_BankTable] = new JZConfigEntry(".bank_table");
  mNames[C_VoiceNames] = new JZConfigEntry(".voicenames");
  mNames[C_DrumSets] = new JZConfigEntry(".drumsets");
  mNames[C_CtrlNames] = new JZConfigEntry(".ctrlnames");
  mNames[C_DrumNames] = new JZConfigEntry(".drumnames");

  // The startup song.
  mNames[C_StartUpSong] = new JZConfigEntry(".startup_song", "jazz.mid");

  mNames[C_OssBug1] = new JZConfigEntry(".ossbug1", 0);
  mNames[C_OssBug2] = new JZConfigEntry(".ossbug2", 0);
  mNames[C_DuplexAudio] = new JZConfigEntry(".duplex_audio", 0);
  mNames[C_ThruInput] = new JZConfigEntry(".thru_input", 0);
  mNames[C_ThruOutput] = new JZConfigEntry(".thru_output", 0);

  // Enable/disable splash dialog.
  mNames[C_EnableWelcome] = new JZConfigEntry(".enable_welcome", 1);

  // Other initialization.

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

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZConfiguration::~JZConfiguration()
{
  for (int i = 0; i < NumConfigNames; ++i)
  {
    if (mNames[i])
    {
      delete mNames[i];
    }
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
const pair<string, int>& JZConfiguration::GetDrumName(unsigned entry) const
{
  assert((entry >= 0) && (entry < mDrumNames.size()));
  return mDrumNames[entry];
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
const pair<string, int>& JZConfiguration::GetDrumSet(unsigned entry) const
{
  assert((entry >= 0) && (entry < mDrumSets.size()));
  return mDrumSets[entry];
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
const pair<string, int>& JZConfiguration::GetVoiceName(unsigned entry) const
{
  assert((entry >= 0) && (entry < mVoiceNames.size()));
  return mVoiceNames[entry];
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
const pair<string, int>& JZConfiguration::GetCtrlName(unsigned entry) const
{
   assert((entry >= 0) && (entry < mCtrlNames.size()));
   return mCtrlNames[entry];
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZDoubleCommand& JZConfiguration::BankEntry(unsigned entry)
{
   assert((entry >= 0) && (entry < mBankTable.size()));
   return mBankTable[entry];
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZConfiguration::Check(const char* pName) const
{
  if (!pName || (pName[0] != '.'))
  {
    return -1;
  }

  for (int i = 0; i < NumConfigNames; i++)
  {
    if (!mNames[i])
    {
      continue;
    }
    if (!strncmp(pName, mNames[i]->GetName(), strlen(mNames[i]->GetName())))
    {
      // Found
      return i;
    }
  }
  return -1;
}

//-----------------------------------------------------------------------------
// Description:
//   Return the Jazz++ configuration file name, normally jazz.cfg.  If the
// value has not been set by an earlier call to LoadConfig, attempt to find
// the file using FindFile().
//-----------------------------------------------------------------------------
wxString JZConfiguration::GetFileName()
{
  if (!mFileName.empty())
  {
    return mFileName;
  }

  mFileName = FindFile("jazz.cfg");

  if (mFileName.empty())
  {
    mFileName = FindFile(".jazz");
  }

  return mFileName;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZConfiguration::Load(char* buf)
{
   int entry = Check(buf);

   if (entry < 0)
      return entry;

   char format[100];
   int result = 1;
   if (mNames[entry]->GetType() == ConfigEntryTypeInt)
   {
      sprintf(format, "%s %%d", mNames[entry]->GetName());
      int Value;
      result = sscanf(buf, format, &Value);
      mNames[entry]->SetValue(Value);
   }
   else if (mNames[entry]->GetType() == ConfigEntryTypeStr)
   {
      // allow whitespace inside entries like "C:\Program Files\JazzWare"
      int ofs = strlen(mNames[entry]->GetName());
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
      mNames[entry]->SetStrValue(pStringValue);
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

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
const int& JZConfiguration::GetValue(const char* pName) const
{
  int i = Check(pName);

  assert(i >= 0);

  return mNames[i]->GetValue();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
const int& JZConfiguration::GetValue(int Index) const
{
  assert((Index >= 0) && (Index < NumConfigNames));
  return mNames[Index]->GetValue();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool JZConfiguration::Get(int entry, char *value)
{
  assert((entry >= 0) && (entry < NumConfigNames));

  wxString FileName = GetFileName();
  if (FileName.IsEmpty())
  {
    return false;
  }

  FILE *fd = fopen(FileName.c_str(), "r");
  const char* name = GetName(entry);

  int  len = strlen(name);
  char buf[1000];
  bool found = false;
  while (!found && fgets(buf, sizeof(buf), fd) != NULL)
  {
    if (strncmp(buf, name, len) == 0)
    {
      while (isspace(buf[len]))
      {
        len++;
      }
      int end = strlen(buf) - 1;
      while (end > 0 && isspace(buf[end]))
      {
        buf[end--] = 0;
      }
      strcpy(value, buf + len);
      found = true;
    }
  }
  fclose(fd);
  return found;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool JZConfiguration::Get(int entry, long &value)
{
  char buf[512];
  if (Get(entry, buf))
  {
    sscanf(buf, " %ld ", &value);
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
// Description:
//   Write a configuration entry by making a temp file, and copying all
// entries to there.  If the name/value pair is found, replace it, otherwise
// write it.  Finally copy the temp file over the old configuration file.
//-----------------------------------------------------------------------------
bool JZConfiguration::Put(int Index, const char *value)
{
  assert((Index >= 0) && (Index < NumConfigNames));

  wxString FileName = GetFileName();
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
  const char* name = GetName(Index);

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

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool JZConfiguration::Put(int Index, long Value)
{
  ostringstream Oss;
  Oss << Value;
  return Put(Index, Oss.str().c_str());
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool JZConfiguration::Put(int Index)
{
  assert((Index >= 0) && (Index < NumConfigNames));
  mNames[Index]->SetValue(Index);
  long LongValue = mNames[Index]->GetValue();
  return Put(Index, LongValue);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool JZConfiguration::Put(int Index, int Value)
{
  assert((Index >= 0) && (Index < NumConfigNames));
  mNames[Index]->SetValue(Value);
  long LongValue = mNames[Index]->GetValue();
  return Put(Index, LongValue);
}

//-----------------------------------------------------------------------------
// Description:
//   Load the configuration from a file.  This code supports file inclusion.
//-----------------------------------------------------------------------------
void JZConfiguration::LoadConfig(const wxString& FileName)
{
  if (!::wxFileExists(FileName))
  {
    return;
  }

  mFileName = FileName;

  wxString OriginalCurrentWorkingDirectory = ::wxGetCwd();

  wxFileName FileNameObject(mFileName);

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

  cout
    << "JZConfiguration::LoadConfig:" << '\n'
    << "  \"" << mFileName << '"'
    << endl;

  FdArr[IncLevel] = fopen(mFileName.c_str(), "r");
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
            JZDoubleCommand DoubleCommand;
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
            wxString pathname = FindFile(GetStrValue(entry));
            cout << "include " << entry << endl;
            IncLevel++;
            assert(IncLevel < MaxIncs);
            if (pathname)
            {
              FdArr[IncLevel] = fopen(pathname, "r");
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
        if (VoiceIndex >= 0 && VoiceIndex < mVoiceNames.size())
        {
          int Value;
          sscanf(buf, " %d %n", &Value, &j);

          if (gpConfig->GetValue(C_UseTwoCommandBankSelect))
          {
            assert(0 <= Value && Value <= 65536);
          }
          else
          {
            assert(0 <= Value && Value <= 32639);
          }

          mVoiceNames[VoiceIndex + 1].second = Value + 1;

          // Remove the off \n.
          buf[strlen(buf) - 1] = 0;

          mVoiceNames[VoiceIndex + 1].first = buf + j;

          ++VoiceIndex;
        }
        else
        {
          cout
            << "Voice index \"" << VoiceIndex << "\" out of range."
            << endl;
        }
      }

      // Drumset names
      else if (pVector == &mDrumSets)
      {
        if (DrumsetIndex >= 0 && DrumsetIndex < 129)
        {
          int Value;
          sscanf(buf, " %d %n", &Value, &j);
          if (gpConfig->GetValue(C_UseTwoCommandBankSelect))
          {
            assert(0 <= Value && Value <= 65536);
          }
          else
          {
            assert(0 <= Value && Value <= 32639);
          }
          mDrumSets[DrumsetIndex + 1].second = Value + 1;

          buf[strlen(buf) - 1] = 0;        // cut off \n
          mDrumSets[DrumsetIndex + 1].first = buf + j;

          ++DrumsetIndex;
        }
        else
        {
          cout
            << "Drumset index \"" << DrumsetIndex << "\" out of range."
            << endl;
        }
      }

      // Controller names.
      else if (pVector == &mCtrlNames)
      {
        sscanf(buf, " %d %n", &i, &j);
        assert(0 <= i && i <= 127);
        buf[strlen(buf) - 1] = 0;        // cut off \n

        mCtrlNames[i + 1].first = buf + j;
      }

      // Drum instrument names.
      else if (pVector == &mDrumNames)
      {
        sscanf(buf, " %d %n", &i, &j);
        assert(0 <= i && i <= 127);
        buf[strlen(buf) - 1] = 0;        // cut off \n

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
