/*
**  The JAZZ++ Midi Sequencer
**
** Copyright (C) 1994-2000 Andreas Voss and Per Sigmond, all rights reserved.
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

#ifndef jazz_h
#define jazz_h

/* #define DEBUG(x) x */
#define DEBUG(x)

#ifndef util_h
#include "util.h"
#endif


#include "synth.h"


#include "wx/help.h"
#include "wx/cshelp.h"


#include <time.h>
#include <assert.h>

// values for C_MidiDriver
#define C_DRV_JAZZ  0
#define C_DRV_OSS   1
#define C_DRV_ALSA  2

enum ConfigNames {
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

enum ConfigEntryType {
   ConfigEntryTypeInt = 0,
   ConfigEntryTypeStr,
   ConfigEntryTypeEmpty
};

class tConfigEntry
{
      friend class tConfig;
      ConfigEntryType Type;
      char* Name;
      int Value;
      char* StrValue;
   public:
      tConfigEntry::~tConfigEntry();
      tConfigEntry( char* name, int ival )
	 : Name(name),
	   Value(ival),
	   StrValue(0),
	   Type( ConfigEntryTypeInt ) {}
      tConfigEntry( char* name, char* sval );
      tConfigEntry( char* name )
	 : Name(name),
	   Value(0),
	   StrValue(0),
	   Type( ConfigEntryTypeEmpty ) {}
};

class tConfig
{
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

      Bool Get(int entry, char *value);
      Bool Get(int entry, long &value);

      Bool Put(int entry, const char *value);
      Bool Put(int entry, long value);
      Bool Put(int entry );
      Bool Put(int entry, int val);
};

enum SynthTypeId {
   SynthTypeGM = 0,
   SynthTypeGS,
   SynthTypeXG,
   SynthTypeOther,
   NumSynthTypes
};

extern tNamedValue SynthTypes[];
extern tNamedValue SynthTypeFiles[];

/** a wrapper around the wx help classes*/
class tHelp
{
public:
  tHelp(const char *helpfile);
  ~tHelp();
  void ShowTopic(const char *topic);
  void DisplayContents();
private:
  // PAT - Changed this from wxHelpControllerHtml.
  wxHtmlHelpController *help;
  const char *helpfile;
};




extern tHelp *HelpInstance;

extern tConfig Config;



class tSong;
extern tSong *TheSong;
extern tSynth *Synth;


#define USED_WXFORM_BUTTONS 	(wxFORM_BUTTON_OK | \
				 wxFORM_BUTTON_CANCEL | \
				 wxFORM_BUTTON_HELP)

#ifndef MIN
#define MIN(x, y)       ( ((x) < (y)) ? (x) : (y) )
#endif

#ifndef MAX
#define MAX(x, y)       ( ((x) > (y)) ? (x) : (y) )
#endif


#if defined wx_xview || defined wx_msw
#define DELETE_THIS() delete this
#else
#if wxVERSION_NUMBER > 1603 && wxVERSION_NUMBER < 2000
#define DELETE_THIS() wxPostDelete(this)
#else
#define DELETE_THIS() delete this
#endif
#endif

wxString FindFile(const char *fname);
void InstallLicenseFile();

#endif
