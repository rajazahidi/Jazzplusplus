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

#include "wx/wx.h"
#include <unistd.h>

#include "engine/Project.h"

#include "synth.h"

#include "config.h"
#include "song.h"
#include "trackwin.h"
#include "filter.h"
#include "mstdfile.h"
#include "player.h"
#include "jazz.h"
#include "resdlg.h"

#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>

#include "about.h"

#ifdef wx_msw
#include "winplay.h"
#endif
#ifdef DEV_ALSA
#include "alsaplay.h"
#endif

#include "audio.h"

#ifdef DEV_ALSA
#include "alsadrv.h"
#endif

#ifdef DEV_SEQUENCER2
#include "audiodrv.h"
#endif

#ifdef wx_msw
#include "winaudio.h"
#endif


///////////////////////////////////////////////////////////////////
// globals
// These all need to be removed when they're not used anymore and have been
// replaced in jppProject
char *StartUpSong = 0;
tSong *TheSong;
tSynth *Synth;
tHelp *HelpInstance = 0;

tConfig Config; //JAVE valgrind says mismatched free()
///////////////////////////////////////////////////////////////////

// gProject is the global jppProject instance that we should use now
jppProject* gProject;


// -------------------------- global config -----------------------------------

tConfigEntry::tConfigEntry( char* name, char* sval )
   : Name(name),
     Value(0),
     Type( ConfigEntryTypeStr )
{
   if (!sval)
      StrValue = 0;
   else
   {
     StrValue = new char[strlen(sval)+1]; //JAVE these should be deleted somewhere FIXME(see destructor below)
      strcpy(StrValue,sval);
   }
}

tConfigEntry::~tConfigEntry()
{
  delete StrValue; 
}

tConfig::tConfig()
{
   int i;
   for (i = 0; i < NumConfigNames; i++)
     Names[i] = 0;

   BankTable = 0;

   // search for midi device
   Names[C_Seq2Device] = new tConfigEntry(".device", -1);

   // use /dev/music
   Names[C_MidiDriver] = new tConfigEntry(".driver", 1);

   // Enable audio at startup
   Names[C_EnableAudio] = new tConfigEntry(".enable_audio", 1);

   // Windows midi devices
   Names[C_WinInputDevice] = new tConfigEntry(".win_input_device", -1 );
   Names[C_WinOutputDevice] = new tConfigEntry(".win_output_device", -1 );

   // ALSA midi devices
   Names[C_AlsaInputDevice] = new tConfigEntry(".alsa_input_device", -1 );
   Names[C_AlsaOutputDevice] = new tConfigEntry(".alsa_output_device", -1 );

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

   // Geometry settings
   Names[C_TrackWinXpos] = new tConfigEntry(".trackwin_xpos",10);
   Names[C_TrackWinYpos] = new tConfigEntry(".trackwin_ypos",10);
   Names[C_TrackWinWidth] = new tConfigEntry(".trackwin_width",600);
   Names[C_TrackWinHeight] = new tConfigEntry(".trackwin_height",400);
   Names[C_PianoWinXpos] = new tConfigEntry(".pianowin_xpos",30);
   Names[C_PianoWinYpos] = new tConfigEntry(".pianowin_ypos",30);
   Names[C_PianoWinWidth] = new tConfigEntry(".pianowin_width",600);
   Names[C_PianoWinHeight] = new tConfigEntry(".pianowin_height",400);
   Names[C_PartsDlgXpos] = new tConfigEntry(".partsdialog_xpos",50);
   Names[C_PartsDlgYpos] = new tConfigEntry(".partsdialog_ypos",50);
   Names[C_TrackDlgXpos] = new tConfigEntry(".trackdialog_xpos",50);
   Names[C_TrackDlgYpos] = new tConfigEntry(".trackdialog_ypos",50);
   Names[C_HarmonyXpos] = new tConfigEntry(".harmonybrowser_xpos",100);
   Names[C_HarmonyYpos] = new tConfigEntry(".harmonybrowser_ypos",100);
   Names[C_RhythmXpos] = new tConfigEntry(".randomrhythm_xpos",150);
   Names[C_RhythmYpos] = new tConfigEntry(".randomrhythm_ypos",150);

   // Show Dialog unless initialized
   Names[C_SynthDialog] = new tConfigEntry(".synth_dialog", 1);

   // Default synthesizer type
   Names[C_SynthType] = new tConfigEntry(".synth_type",SynthTypes[SynthTypeGS].Name);

   // Default synthesizer config file
   Names[C_SynthConfig] = new tConfigEntry(".synth_config", SynthTypeFiles[SynthTypeGS].Name);

   // When to send synthesizer reset (0=never, 1=song start, 2=start play)
   Names[C_SendSynthReset] = new tConfigEntry(".send_synth_reset",1);

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

   NumDrumNames = 130;
   DrumNames = new tNamedValue [NumDrumNames];
   for (i = 0; i < (NumDrumNames-1); i++)
   {
      DrumNames[i].Value = i;
      DrumNames[i].Name  = "";
   }
   DrumNames[NumDrumNames-1].Name = 0;

   NumDrumSets = 130;
   DrumSets  = new tNamedValue [NumDrumSets];
   for (i = 0; i < (NumDrumSets-1); i++)
   {
      DrumSets[i].Value = i;
      DrumSets[i].Name  = "";
   }
   DrumSets[NumDrumSets-1].Name = 0;
   DrumSets[0].Name = "None";


   NumCtrlNames = 130;
   CtrlNames = new tNamedValue [NumCtrlNames];
   for (i = 0; i < (NumCtrlNames-1); i++)
   {
      CtrlNames[i].Value = i;
      CtrlNames[i].Name  = "";
   }
   CtrlNames[NumCtrlNames-1].Name = 0;

   NumVoiceNames = 2;
   VoiceNames = new tNamedValue[NumVoiceNames];
   VoiceNames[0].Name = "None";
   VoiceNames[1].Name = 0;
}

tConfig::~tConfig()
{
   int i;
   for (i = 0; i < NumConfigNames; i++)
   {
      delete [] Names[i];
   }
}

tNamedValue& tConfig::DrumName( int entry )
{
   assert( (entry >= 0) && (entry < NumDrumNames) );
   return DrumNames[entry];
}

tNamedValue& tConfig::DrumSet( int entry )
{
   assert( (entry >= 0) && (entry < NumDrumSets) );
   return DrumSets[entry];
}

tNamedValue& tConfig::VoiceName( int entry )
{
   assert( (entry >= 0) && (entry < NumVoiceNames) );
   return VoiceNames[entry];
}

tNamedValue& tConfig::CtrlName( int entry )
{
   assert( (entry >= 0) && (entry < NumCtrlNames) );
   return CtrlNames[entry];
}

tDoubleCommand& tConfig::BankEntry( int entry )
{
   assert( (entry >= 0) && (entry < NumBankEntries) );
   return BankTable[entry];
}

int tConfig::Check( char* name )
{
   if ( !name || (name[0] != '.') )
      return -1;

   for (int i = 0; i < NumConfigNames; i++)
   {
      if (!Names[i])
        continue;
      if (!strncmp(name,Names[i]->Name,strlen(Names[i]->Name)))
      {
	 // Found
	 return i;
      }
   }
   return -1;
}

/** return the config file name, normally jazz.cfg.
search in the path provided by FindFile()
*/
wxString tConfig::File()
{
  wxString fname = FindFile("jazz.cfg");
  if(fname.IsEmpty()) {
    fname = FindFile(".jazz");
  }
  return fname;
}

int tConfig::Load( char* buf )
{
   int entry = Check( buf );

   if (entry < 0)
      return entry;

   char format[100];
   int result = 1;
   if (Names[entry]->Type == ConfigEntryTypeInt)
   {
      sprintf( format, "%s %%d", Names[entry]->Name );
      result = sscanf( buf, format, &Names[entry]->Value );
   }
   else if (Names[entry]->Type == ConfigEntryTypeStr)
   {
      // allow whitespace inside entries like "C:\Program Files\JazzWare"
      int ofs = strlen(Names[entry]->Name);
      while (buf[ofs] == ' ' || buf[ofs] == '\t')  // not \n
        ofs++;
      int end = strlen(buf) - 1;
      while (end > ofs) {
        if (!isspace(buf[end]))
          break;
        end--;
      }
      int size = end - ofs + 1;
      //if (Names[entry]->StrValue)
      delete [] Names[entry]->StrValue;
      Names[entry]->StrValue = new char[size+1];
      memcpy(Names[entry]->StrValue, buf + ofs, size);
      Names[entry]->StrValue[size] = 0;
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

int& tConfig::operator () ( char* name )
{
   int i = Check( name );

   assert( i >= 0 );

   return Names[i]->Value;
}

int& tConfig::operator () ( int name )
{
   assert( (name >= 0) && (name < NumConfigNames) );
   return Names[name]->Value;
}

bool tConfig::Get(int entry, char *value)
{
   assert( (entry >= 0) && (entry < NumConfigNames) );

   wxString fname = File();
   if (fname.IsEmpty())
     return FALSE;

   FILE *fd = fopen(fname.c_str(), "r");
   char *name = Name(entry);

   int  len = strlen(name);
   char buf[1000];
   bool found = FALSE;
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
	 found = TRUE;
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
    return TRUE;
  }
  return FALSE;
}
/** write a conf entry by making a temp file, and copying all entries to there. 
if the name/value pair is found, replace it. otherwise write it. lastly copy temp file over the old file*/
bool tConfig::Put(int entry, const char *value)
{
   assert( (entry >= 0) && (entry < NumConfigNames) );

   wxString fname = File();
   if (fname.IsEmpty())
     return FALSE;

   char tempname[512];
   strcpy(tempname, fname.c_str());
   strcat(tempname, ".tmp"); //make the temp file name
   FILE *out = fopen(tempname, "w");
   if (!out)
      return FALSE;

   FILE *inp = fopen(fname.c_str(), "r");
   char *name = Name(entry);

   int  len = strlen(name);
   char buf[1000];
   bool found = FALSE;
   while (fgets(buf, sizeof(buf), inp) != NULL)
   {
      if (strncmp(buf, name, len) == 0)
      {
	 fprintf(out, "%s %s\n", name, value);
	 found = TRUE;
      }
      else
	 fputs(buf, out);
   }
   if (!found)
      fprintf(out, "%s %s\n", name, value);
   fclose(inp);
   fclose(out);
   unlink(fname.c_str());
   rename(tempname, fname.c_str());
   return TRUE;
}


bool tConfig::Put(int entry, long value)
{
   char buf[50];
   sprintf(buf, "%ld", value);
   return Put(entry, buf);
}

bool tConfig::Put( int entry )
{
   return Put( entry, (long) operator()( entry ) );
}

bool tConfig::Put( int entry, int val )
{
   operator()( entry ) = val;
   return Put( entry );
}

/** load a config file from  a file

itr supports stuff like file inclusion(somehow)
*/
void tConfig::LoadConfig(wxString fname)
{
   char buf[1000];
   int i, j, bank_index = 0, voice_index = 0, drumset_index = 0;
   tNamedValue *nv = 0;

#define MAX_INCS 10
   FILE *FdArr[MAX_INCS];
   int IncLevel = 0;

   for (i = 0; i < MAX_INCS; i++)
      FdArr[i] = NULL;


   cout <<"tConfig::LoadConfig '"<<fname<<"'"<<endl;
   //char * strfname = strdup(fname);
   //for some bizare reason, the fname that gets passed to this function when loading jazz.cfg, gets corrupted when sent to fopen
   //strdup:ing it, resolves the problem, sometimes but not always. FIXME
   FdArr[IncLevel] = fopen(fname, "r");
   cout <<fname<<endl;
   if (FdArr[IncLevel] == NULL) {
     wxMessageBox("Error reading config file.\nPlease check permissions and set the environment variable\nJAZZ to the installation directory", "Warning", wxOK);
     return;
   }

   while ( 1 )
   {
      // Read a line from the current file
      if (fgets(buf, sizeof(buf), FdArr[IncLevel]) == NULL)
      {
	 fclose( FdArr[IncLevel] );
	 FdArr[IncLevel] = NULL;
	 IncLevel--;
	 if (IncLevel < 0)
	    // Last line of jazz.cfg (.jazz)
	    break;
	 else
	    // Last line of current include-file
	    continue;
      }

      int entry;

      // Read keyword lines
      if ( (entry = Config.Load(buf)) >= 0 )
      {
	 switch (entry)
	 {
	  case C_BankTable:
	     nv = (tNamedValue *) NULL;
	     if (BankTable == (tDoubleCommand *) NULL)
	     {
                NumBankEntries = (*this)(C_MaxBankTableEntries)+1;
		BankTable = new tDoubleCommand [NumBankEntries];
		for (i = 0; i <= (*this)(C_MaxBankTableEntries); i++)
		{
		   BankTable[i].Command[0]  = -1;
		   BankTable[i].Command[1]  = -1;
		}
	     }
	     break;
	  case C_VoiceNames:
	     if (NumVoiceNames == 2)
	     {
		if (VoiceNames)
		   delete[] VoiceNames;

		NumVoiceNames = (*this)(C_MaxVoiceNames)+2;
		VoiceNames = new tNamedValue [NumVoiceNames];
		for (i = 0; i < (NumVoiceNames-1); i++)
		{
		   VoiceNames[i].Value = i;
		   VoiceNames[i].Name  = "";
		}
		VoiceNames[0].Name = "None";
		VoiceNames[NumVoiceNames-1].Name = 0;
	     }
	     nv = VoiceNames;
	     break;
	  case C_DrumSets:
	     nv = DrumSets;
	     break;
	  case C_CtrlNames:
	     nv = CtrlNames;
	     break;
	  case C_DrumNames:
	     nv = DrumNames;
	     break;
	  case C_SynthConfig:
	  case C_Include:
	  {
	     // include file
	     wxString pathname = FindFile( StrValue(entry) );
	     cout << "include "<< entry<<endl;
	     IncLevel++;
	     assert( IncLevel < MAX_INCS );
             if (pathname)
	       FdArr[IncLevel] = fopen(pathname,"r");
             else
	       FdArr[IncLevel] = NULL;
	     if (FdArr[IncLevel] == NULL)
	     {
		char str[100];
		sprintf(str,"Could not open config include file '%s'\n", buf );
		wxMessageBox(str, "Warning", wxOK);
		IncLevel--;
	     }
	  }
	  break;

	  default:
	     break;
	 }
      }

      // Read named value entries
      else if (nv && isdigit(buf[0]))
      {
	 // Voice names
	 if (nv == VoiceNames) {
            long val;
	    assert( 0 <= voice_index && voice_index <= NumVoiceNames);
	    sscanf(buf, " %ld %n", &val, &j);
            if (Config(C_UseTwoCommandBankSelect))
            {
	      assert(0 <= val && val <= 65536);
            }
            else
            {
	      assert(0 <= val && val <= 32639);
            }
	    buf[ strlen(buf)-1 ] = 0;	// cut off \n
	    nv[voice_index+1].Value = val + 1;
	    nv[voice_index+1].Name = strdup(buf + j);
	    voice_index++;
	 }

	 // Drumset names
	 else if (nv == DrumSets) {
            long val;
	    assert( 0 <= drumset_index && drumset_index <= 129);
	    sscanf(buf, " %ld %n", &val, &j);
            if (Config(C_UseTwoCommandBankSelect))
            {
	      assert(0 <= val && val <= 65536);
            }
            else
            {
	      assert(0 <= val && val <= 32639);
            }
	    buf[ strlen(buf)-1 ] = 0;	// cut off \n
	    nv[drumset_index+1].Value = val + 1;
	    nv[drumset_index+1].Name = strdup(buf + j);
	    drumset_index++;
	 }

	 // Controller names or drum instrument names
	 else if ( (nv == CtrlNames) || (nv == DrumNames) ) {
	    sscanf(buf, " %d %n", &i, &j);
	    assert(0 <= i && i <= 127);
	    buf[ strlen(buf)-1 ] = 0;	// cut off \n
	    nv[i+1].Name = strdup(buf + j);
	 }
	 else
	 {
		char str[200];
		sprintf(str, "LoadConfig: error reading line\n%s", buf );
		wxMessageBox(str, "Warning", wxOK);
	 }
      }

      // Read bank table entries
      else if (nv == (tNamedValue *) NULL &&
	       BankTable != (tDoubleCommand *) NULL &&
	       isdigit(buf[0]))
      {
	 assert( 0 <= bank_index && bank_index < (*this)(C_MaxBankTableEntries));
	 sscanf(buf, " %d %d", &i, &j);
	 assert(0 <= i && i <= 255);
	 assert(0 <= j && j <= 255);
	 BankTable[bank_index].Command[0] = i;
	 BankTable[bank_index].Command[1] = j;
	 bank_index++;
      }
   }
}

/**
find a file, looking in 
1. where the HOME environment var is pointing 
2. where the JAZZ environment var is pointing
3. where the jazz executable was started

this function was converted to use wxString instead of char*
it is safe to return localy allocated wxStrings because of reference counting
*/
wxString FindFile(const char *fname)
{
  wxString buf;//static char buf[256];

  if (wxFileExists((char *)fname))
  {
    cout<<"imediate hit "<<fname<<endl;
    return fname;
  }

  wxString home;
  if (getenv("HOME") != 0)
  {
    home=getenv("HOME");
    buf<<home<<"/"<<fname;//sprintf(buf, "%s/%s", home, fname);
    cout<<"home "<<buf<<endl;
    if (wxFileExists(buf))
      return buf;
  }
  if (getenv("HOME") != 0)
  {
    buf="";
    home = getenv("JAZZ");
    buf << home << "/" << fname;//    sprintf(buf, "%s/%s", home, fname);
    cout<<"jazz "<<buf<<endl;
    if (wxFileExists(buf))
      return buf;
  }

  // look where the executable was started
  home = wxPathOnly((const char *)wxTheApp->argv[0]);
    buf="";
  buf<<home<<"/"<<fname;//sprintf(buf, "%s/%s", home, fname);
  cout<<"startup "<<buf<<endl;
  if (wxFileExists(buf))
    return buf;

  //look in the compiled-in path
  buf="";
  buf<<JAZZ_DATADIR<<"/"<<fname;
  cout <<"compiled in path "<<buf<<"  "<<wxFileExists(buf)<<endl;
  if (wxFileExists(buf))
    return buf;
  

  return wxEmptyString;
}

wxString FindDialog(wxString name) {
  return FindFile("dialogs/" + name);
}

tNamedValue SynthTypes[] =
{
   tNamedValue("GM", SynthTypeGM),
   tNamedValue("GS", SynthTypeGS),
   tNamedValue("XG", SynthTypeXG),
   tNamedValue("Other", SynthTypeOther),
   tNamedValue(0, 0)
};

tNamedValue SynthTypeFiles[] =
{
   tNamedValue("gm.jzi", SynthTypeGM),
   tNamedValue("gs.jzi", SynthTypeGS),
   tNamedValue("xg.jzi", SynthTypeXG),
   tNamedValue("other.jzi", SynthTypeOther),
   tNamedValue(0, 0)
};



tHelp::tHelp(const char *fname) {
   // PAT - Changed this from wxHelpControllerHtml.
   help = new wxHtmlHelpController();
   help->Initialize((char *)fname);
   cout <<"tHelp::tHelp "<<fname<<endl;
   helpfile = copystring(fname);
}

tHelp::~tHelp() {
  delete [] helpfile;
}

void tHelp::ShowTopic(const char *topic) {
   help->LoadFile((char *)helpfile);
   help->KeywordSearch((char *)topic);
}

void tHelp::DisplayContents() {
   help->LoadFile((char *)helpfile);
   help->DisplayContents();
}

//--------------------------- jazz application -------------------------------
/** 
This class must be defined to comply with the wxwindows framework.
*/
class tApp: public wxApp
{
  public:
    bool OnInit(void);
    int OnExit(void);
};


// This statement initialises the whole application
IMPLEMENT_APP(tApp)


static void welcome()
{
  wxMessageBox( (char*) about_text, "Welcome to JAZZ", wxOK );
}



/** this is the main entry point of the program
    (similar to void main() in a c prg
wxTheApp is created by the framework

it loads config files, and creates the main window
 */
bool tApp::OnInit(void)
{
  wxApp::OnInit(); //call base class definition(needed for command line parsing)
#ifndef wx_msw
  if (GetArgOpt("-h") || GetArgOpt("-v")) {
	char *myname = wxTheApp->argv[0];
	printf("\n%s\n", about_text );
	printf("\n\nOptions (in any order): \n");
	printf("%s -h                                   -- Print this text\n", myname );
	printf("%s -v                                   -- Print this text\n", myname );
	printf("%s -f filename                          -- Input midi file \n", myname );
	printf("%s -trackwin xpos ypos [width height]   -- Position/size of main window\n", myname );
	printf("%s -pianowin xpos ypos [width height]   -- Position/size of piano roll window\n", myname );
	exit(0);
  }
#endif

	// Instantiate the gProject object
    gProject = new jppProject();

  //  const char *fname = "/tmp/jazz.cfg";//Config.File();
  //cout<<"WARNING hardcoded jazz.cfg  path, because jazz mysteriously dont load cfg file"<<endl;
  wxString fname = Config.File();
  cout <<"tApp::oninit() configfilename '"<<fname<<"'"<<endl;

  if (!fname.IsEmpty())
  {
    Config.LoadConfig(fname);
    DEBUG(
        if (BankTable != (tDoubleCommand *) NULL)
           for (int i = 0; BankTable[i].Command[0]>=0; i++)
               fprintf(stderr, "Bank %d: %d %d\n", i,
		    BankTable[i].Command[0], BankTable[i].Command[1]);
    )
  }
  else
  {
    wxMessageBox("Could not find config file.\nPlease set the environment variable JAZZ to the installation directory", "Warning", wxOK);
  }


  Synth = NewSynth( Config.StrValue(C_SynthType) );

  wxPathList HelpPathList;
  HelpPathList.AddEnvList("WXHELPFILES");
  HelpPathList.AddEnvList("PATH");
  HelpPathList.AddEnvList("JAZZ");
  HelpPathList.EnsureFileAccessible((char *)wxTheApp->argv[0]);

#ifdef wx_msw
  HelpInstance = new tHelp( HelpPathList.FindValidPath("jazz.hlp") );
#else
  HelpInstance = new tHelp( HelpPathList.FindValidPath("jazz") );
#endif



  TheSong = new tSong;
  Midi = new tNullPlayer(TheSong);


  // ---------------------- XML Dialogs -------------------------
  jppResourceDialog::LoadResource(FindDialog("windowSettings.xrc"));



  // --------------------- Linux drivers ------------------------
#ifndef wx_msw
  if (Config(C_MidiDriver) == C_DRV_OSS)
  {
#ifdef DEV_SEQUENCER2
    Midi = new tAudioPlayer(TheSong);
    if (!Midi->Installed())
    {
      delete Midi;
      Midi = new tSeq2Player(TheSong);
    }
    if (!Midi->Installed())
    {
      perror("/dev/music");
      fprintf(stderr, "(dev_sequencer2)Jazz will start with no play/record ability\n");
      Midi = new tNullPlayer(TheSong);
    }
#else
    fprintf(stderr, "This programm lacks OSS driver support\n");
    Midi = new tNullPlayer(TheSong);
#endif // DEV_SEQUENCER2
  }

  else if (Config(C_MidiDriver) == C_DRV_ALSA) {
#ifdef DEV_ALSA
#ifndef __PORTING
    Midi = new tAlsaAudioPlayer(TheSong);
    if (!Midi->Installed())
    {
      delete Midi;
#endif //__PORTING
      cout<<"creating alsa player"<<endl;
      Midi = new tAlsaPlayer(TheSong);
#ifndef __PORTING
    }
#endif //__PORTING
    if (!Midi->Installed())
    {
      fprintf(stderr, "Could not install alsa driver.\nJazz will start with no play/record ability\n");
      Midi = new tNullPlayer(TheSong);
    }
#else
    fprintf(stderr, "This programm lacks ALSA driver support\n");
    Midi = new tNullPlayer(TheSong);
#endif
  }

  else if (Config(C_MidiDriver) == C_DRV_JAZZ) {
#ifdef DEV_MPU401
    Midi = new tMpuPlayer(TheSong);
    if (!Midi->Installed())
    {
      fprintf(stderr, "Could not connect to midinet server at host '%s'\n", midinethost );
      fprintf(stderr, "Jazz will start with no play/record ability\n");
      Midi = new tNullPlayer(TheSong);
    }
#else
    fprintf(stderr, "This programm lacks JAZZ/MPU401 driver support\n");
    Midi = new tNullPlayer(TheSong);
#endif
  }
  else
    fprintf(stderr, "No valid driver configured in config file\n.Jazz will start with no play/record ability\n");
#endif // ifndef wx_msw


  // ------------------------ MS drivers ------------------------

#ifdef wx_msw
  // create after Trackwin is there
  switch (Config(C_ClockSource))
  {
    case CsMidi:
      Midi = new tWinMidiPlayer(TheSong);
      break;
    case CsMtc:
      Midi = new tWinMtcPlayer(TheSong);
      break;
    case CsFsk:
    case CsInt:
    default:
      Midi = new tWinAudioPlayer(TheSong);
      if (!Midi->Installed())
      {
	Midi->ShowError();
        delete Midi;
	Midi = new tWinIntPlayer(TheSong);
      }
      break;
  }
  if (!Midi->Installed())
  {
    Midi->ShowError();
    Midi = new tNullPlayer(TheSong);
  }
#endif // ifdef wx_msw

  // ---------------------- end drivers ----------------------



  int i;
  int opt;

  opt = GetArgOpt( "-trackwin" ) + 1;
  for (i = 0; i < 4; i++, opt++) {
	if ((wxTheApp->argc > opt) && isdigit( wxTheApp->argv[opt][0] ))
		Config(i+C_TrackWinXpos) = atoi( wxTheApp->argv[opt] );
	else
		break;
  }



  // load song given on commandline or load "jazz.mid"
  cout <<"load song"<<endl;
  opt = GetArgOpt( "-f" ) + 1;
  if (opt && (wxTheApp->argc > opt))
    StartUpSong = copystring(wxTheApp->argv[opt]);
  else
    StartUpSong = copystring(Config.StrValue(C_StartUpSong));
  FILE *fd = fopen(StartUpSong, "r");
  if (fd)
  {
    fclose(fd);
    tStdRead io;
    TheSong->Read(io, StartUpSong);
    if (strcmp(StartUpSong, "jazz.mid"))
      lasts = StartUpSong;
  }


#ifndef wx_msw

  if(Config(C_EnableWelcome)){
    welcome();
  }
#endif

  TrackWin = new tTrackWin(0, "Jazz!", TheSong, Config(C_TrackWinXpos), Config(C_TrackWinYpos), Config(C_TrackWinWidth), Config(C_TrackWinHeight) );
  TrackWin->Create();
  TrackWin->Show(TRUE);


  if (fd)
  {
    TrackWin->SetTitle( lasts );
    TrackWin->Redraw();
  }

#ifdef wx_msw
  welcome();
#endif

  Midi->LoadDefaultSettings();

#ifndef __PORTING 
  if (Config(C_SynthDialog))
    TrackWin->MenSynthSettings();
#endif // __PORTING


  return 1;//TrackWin;
}

int tApp::OnExit(void) {
	delete gProject;
}


// end jazz.cpp

