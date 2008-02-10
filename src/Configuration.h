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

#ifndef JZ_CONFIGURATION_H
#define JZ_CONFIGURATION_H

#include "WxWidgets.h"

#include "NamedValue.h"

class tDoubleCommand
{
  public:
    int Command[2];
};

enum ConfigNames
{
  C_Seq2Device = 0,
  C_MidiDriver,
  C_EnableAudio,
  C_WinInputDevice,
  C_WinOutputDevice,
  C_AlsaInputDevice,
  C_AlsaOutputDevice,
  C_AlsaAudioCard,
  C_AlsaAudioInputDevice,
  C_AlsaAudioOutputDevice,
  C_AlsaSyncInputDevice,
  C_AlsaSyncInputFormat,
  C_AlsaSyncOutput,
  C_AlsaSyncOutputFormat,
  C_AlsaSyncOutputDevice,
  C_SoftThru,
  C_HardThru,
  C_ClockSource,
  C_RealTimeOut,
  C_UseReverbMacro,
  C_UseChorusMacro,
  C_DrumChannel,
  C_BankControlNumber,
  C_BankControlNumber2,
  C_MaxBankTableEntries,
  C_PartsColumnsMax,
  C_PartsTracknamesRight,
  C_MaxVoiceNames,
  C_UseTwoCommandBankSelect,
  C_MetroIsAccented,
  C_MetroVelocity,
  C_MetroNormalClick,
  C_MetroAccentedClick,
  C_TrackWinXpos,
  C_TrackWinYpos,
  C_TrackWinWidth,
  C_TrackWinHeight,
  C_PianoWinXpos,
  C_PianoWinYpos,
  C_PianoWinWidth,
  C_PianoWinHeight,
  C_PartsDlgXpos,
  C_PartsDlgYpos,
  C_TrackDlgXpos,
  C_TrackDlgYpos,
  C_HarmonyXpos,
  C_HarmonyYpos,
  C_RhythmXpos,
  C_RhythmYpos,
  C_SynthConfig,
  C_SynthType,
  C_SendSynthReset,
  C_Include,
  C_BankTable,
  C_VoiceNames,
  C_DrumSets,
  C_CtrlNames,
  C_DrumNames,
  C_StartUpSong,
  C_OssBug1,
  C_OssBug2,
  C_SynthDialog,
  C_DuplexAudio,
  C_ThruInput,
  C_ThruOutput,
  C_EnableWelcome,
  NumConfigNames
};

enum ConfigEntryType
{
  ConfigEntryTypeInt = 0,
  ConfigEntryTypeStr,
  ConfigEntryTypeEmpty
};

// values for C_MidiDriver
enum TEMidiDriver
{
  eMidiDriverJazz = 0, // C_DRV_JAZZ  0
  eMidiDriverOss = 1,  // C_DRV_OSS   1
  eMidiDriverAlsa = 2  // C_DRV_ALSA  2
};

class tConfigEntry
{
  public:

    tConfigEntry( char* name, int ival )
      : Type(ConfigEntryTypeInt),
        Name(name),
        Value(ival),
        StrValue(0)
    {
    }

    tConfigEntry(char* name, char* sval);

    tConfigEntry(char* name)
      : Type(ConfigEntryTypeEmpty),
        Name(name),
        Value(0),
        StrValue(0)
    {
    }

    ~tConfigEntry();

  private:

    friend class tConfig;
    ConfigEntryType Type;
    char* Name;
    int Value;
    char* StrValue;

};

class tConfig
{
  private:

    tConfigEntry* Names[NumConfigNames];
    tNamedValue *DrumNames;
    int NumDrumNames;
    tNamedValue *DrumSets;
    int NumDrumSets;
    tNamedValue *VoiceNames;
    int NumVoiceNames;
    tNamedValue *CtrlNames;
    int NumCtrlNames;
    tDoubleCommand *BankTable;
    int NumBankEntries;

  public:

    tConfig();
    ~tConfig();
    void LoadConfig(wxString fname );
    wxString File();
    int Check( char* name );
    int Load( char* buf );

    tNamedValue& DrumName( int entry );
    tNamedValue& DrumSet( int entry );
    tNamedValue& VoiceName( int entry );
    tNamedValue& CtrlName( int entry );
    tDoubleCommand& BankEntry( int entry );

    char* Name( int entry )
    {
      assert( (entry >= 0) && (entry < NumConfigNames) );
      return Names[entry]->Name;
    }

    char*& StrValue( int entry )
    {
      assert( (entry >= 0) && (entry < NumConfigNames) );
      return Names[entry]->StrValue;
    }

    int& operator () ( char* name );
    int& operator () ( int name );

    bool Get(int entry, char *value);
    bool Get(int entry, long &value);

    bool Put(int entry, const char *value);
    bool Put(int entry, long value);
    bool Put(int entry );
    bool Put(int entry, int val);
};

extern tConfig Config;

#endif // !defined(JZ_CONFIGURATION_H)
