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

#include "config.h"
#include "wx/wx.h"

#ifndef __PORTING
#include "rhythm.h"
#include "dialogs.h"
#include "gs_dlgs.h"
#include "genmeldy.h"
#endif // __PORTING

#include "proplistdlg.h"
#include "config.h"
#include "trackwin.h"
#include "track.h"
#include "song.h"
#include "mstdfile.h"
#include "pianowin.h"
#include "filter.h"
#include "mapper.h"
#include "command.h"
#include "harmony.h"
#include "jazz.h"
#include "toolbar.h"
#include "eventlst.h"
#include "arpeggio.h"
#include "dialogs/copyright.h"
#include "dialogs/midiThruDialog.h"
#include "dialogs/songSettings.h"
#include "dialogs/copyDialog.h"
#include "commands/copyCommand.h"
#include "gui/trackwinEnum.h"
#include "engine/Project.h"
#include "shuffle.h"
#include "about.h"
#include "resdlg.h"
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

#ifdef wx_msw
#include "winplay.h"
#include "winaudio.h"
#else // not wx_msw
#include "player.h"
#include "audiodrv.h"
#endif

extern jppProject* gProject;

// \todo: get rid of all this stuff, it's moving to jppProject
tTrackWin *TrackWin = 0;
static char *defsong = 0;
static char *defpattern = 0;
char *lasts = "noname.mid";

/*
 * Helper Classes
 */

class tLoadPattern : public tMarkDestin {
  public:
    tLoadPattern(tTrackWin *t, const char *fn)
      : tMarkDestin(t->Canvas, t, 0), fname(fn), tw(t) {}

    int LeftDown(wxMouseEvent &e);
  private:
    const char *fname;
    tTrackWin *tw;
};

int tLoadPattern::LeftDown(wxMouseEvent &e)
{
  tMarkDestin::LeftDown(e);
  tSong *sng = new tSong;
  tStdRead io;
  sng->Read(io, fname);

  sng->SetTicksPerQuarter(tw->Song->TicksPerQuarter);

  int dst_track = tw->y2Line((long)y);
  int num_tracks = sng->NumUsedTracks();
  long delta = tw->x2BarClock((long)x);
  tw->Song->NewUndoBuffer();
  for (int i = 0; i < num_tracks; i++) {
    tTrack *src = sng->GetTrack(i);
    tTrack *dst = tw->Song->GetTrack(dst_track++);
    tEventIterator iter(src);
    tEvent *e = iter.First();
    while (e) {
      e = e->Copy();
      e->Clock += delta;
      dst->Put(e);
      e = iter.Next();
    }
    dst->Cleanup();
  }
  delete sng;

  tw->Redraw();
  tw->MouseAction = 0;
  delete this;
  return 1;
}

class tClpPaste : public tMarkDestin {
  public:
    tClpPaste(tTrackWin *t, tSong *clp)
      : tMarkDestin(t->Canvas, t, 0), sng(clp), tw(t) {}

    int LeftDown(wxMouseEvent &e);
  private:
    tSong *sng;
    tTrackWin *tw;
};

int tClpPaste::LeftDown(wxMouseEvent &e)
{
  tMarkDestin::LeftDown(e);

  sng->SetTicksPerQuarter(tw->Song->TicksPerQuarter);

  int dst_track = tw->y2Line((long)y);
  int num_tracks = sng->NumUsedTracks();
  long delta = tw->x2BarClock((long)x);
  tw->Song->NewUndoBuffer();
  for (int i = 0; i < num_tracks; i++) {
    tTrack *src = sng->GetTrack(i);
    tTrack *dst = tw->Song->GetTrack(dst_track++);
    tEventIterator iter(src);
    tEvent *e = iter.First();
    while (e) {
      e = e->Copy();
      e->Clock += delta;
      dst->Put(e);
      e = iter.Next();
    }
    dst->Cleanup();
  }

  tw->Redraw();
  tw->MouseAction = 0;
  delete this;
  return 1;
}

//////////////////////////////////////////////////////////////////////////7
//wxwin 2 new style handlers
//these should replace OnMenuEvent eventually

// Events that are commented do not have appropriate methods yet

//dont forget the base class, there are some event bindings there also
BEGIN_EVENT_TABLE(tTrackWin, wxFrame)
    EVT_SIZE    (tEventWin::OnSize) //this is really lame, i dont know how to solve it( eventwin already has this handler but it isnt inherited)
    // File Menu
    EVT_MENU    (MEN_NEW,tTrackWin::OnNew)
    EVT_MENU    (MEN_LOAD, tTrackWin::OnLoad)
    EVT_MENU    (MEN_CLOSE, tTrackWin::OnClose)
    EVT_MENU    (MEN_SAVE,tTrackWin::OnSave)
    EVT_MENU    (MEN_SAVEAS,tTrackWin::OnSaveAs)
    EVT_MENU    (MEN_EXPORTMIDI, tTrackWin::OnExportMidi)
    EVT_MENU    (MEN_EXPORTSELMIDI, tTrackWin::OnExportSelMidi)
    EVT_MENU    (MEN_PREFERENCES, tTrackWin::OnPreferences)
    // This is the old one.  The one up there, just a few lines up from here, needs to be changed
    // to a handler that just closes the project
    //EVT_MENU    (MEN_CLOSE, tTrackWin::OnClose)

    //Edit Menu
    EVT_MENU    (MEN_UNDO,tTrackWin::OnUndo)
    EVT_MENU    (MEN_REDO,tTrackWin::OnRedo)
    EVT_MENU    (MEN_CLP_CUT,      tTrackWin::MenClpCopyEraseSrc)
    EVT_MENU    (MEN_CLP_COPY,      tTrackWin::MenClpCopyLeaveSrc)
    EVT_MENU    (MEN_CLP_PASTE,      tTrackWin::MenClpPaste)
    EVT_MENU    (MEN_REPLICATE, tTrackWin::MenCopy)
    EVT_MENU    (MEN_DELETE,    tTrackWin::MenDelete)
    EVT_MENU    (MEN_QUANTIZE  ,tTrackWin::MenQuantize)
    EVT_MENU    (MEN_SETCHAN,tTrackWin::MenSetChannel)
    EVT_MENU    (MEN_TRANSP,tTrackWin::MenTranspose)
    EVT_MENU    (MEN_VELOC,tTrackWin::MenVelocity)
    EVT_MENU    (MEN_LENGTH,tTrackWin::MenLength)
    EVT_MENU    (MEN_SHIFT,tTrackWin::OnShift)
    EVT_MENU    (MEN_CLEANUP,tTrackWin::MenCleanup)
    EVT_MENU    (MEN_SEARCHREP,tTrackWin::MenSearchReplace)
    EVT_MENU    (MEN_CLP_TRIM, tTrackWin::MenClpTrim)
    EVT_MENU    (MEN_SPLIT, tTrackWin::MenSplit)
    EVT_MENU    (MEN_SELECTIONSUB, tTrackWin::MenSelectionSub)

    // View Menu
    EVT_MENU    (MEN_ZOOMIN, tTrackWin::OnZoomIn)
    EVT_MENU    (MEN_ZOOMOUT, tTrackWin::OnZoomOut)

    // Project Menu
    EVT_MENU    (MEN_LOAD_TMPL,tTrackWin::OnLoadTemplate)
    EVT_MENU    (MEN_LOADPATTERN,      tTrackWin::MenLoadPattern)
    EVT_MENU    (MEN_SAVEPATTERN,      tTrackWin::MenSavePattern)
    EVT_MENU    (MEN_QUIT, tTrackWin::OnQuit)
    // Need a Mixer Event

    // Parts Menu
    // Need a Master Event
    // Need a Sound Event
    // Need a Vibrato Event
    // Need an Envelope Event

    // Bender Submenu
    // Need a Basic Event
    // Need a LFO1 Event
    // Need a LFO2 Event

    // Modulation Submenu

    // Need the rest of these Events

    // Settings Menu
    EVT_MENU    (MEN_FILTER,tTrackWin::OnFilter)
    EVT_MENU    (MEN_PIANOWIN, tTrackWin::OnPianowin)
    EVT_MENU    (MEN_SONG, tTrackWin::MenSongSettings)
    #ifndef __PORTING
    EVT_MENU    (MEN_METRONOME ,tTrackWin::MenMetronomeSettings)
    #endif // __PORTING
    //  EVT_MENU    (MEN_EFFECTS, tTrackWin::MenEffects)
    // Need a Timing Event
    // Need a MIDI Thru Event
    // Need a Synth Type Event
    EVT_MENU    (MEN_DEVICE,tTrackWin::OnDevice)
    // Save Settings Submenu
    EVT_MENU    (MEN_SAVE_THRU,tTrackWin::SaveThruSettings)
    EVT_MENU    (MEN_SAVE_TIM   ,tTrackWin::SaveTimingSettings)
    EVT_MENU    (MEN_SAVE_EFF     ,tTrackWin::SaveEffectSettings)
    EVT_MENU    (MEN_SAVE_GEO      ,tTrackWin::SaveGeoSettings)
    EVT_MENU    (MEN_SAVE_METRO      ,tTrackWin::SaveMetronomeSettings)
    EVT_MENU    (MEN_SAVE_ALL, tTrackWin::OnSaveAll)

    // Misc Menu
    EVT_MENU    (MEN_TMERGE      ,tTrackWin::MenMergeTracks)
    EVT_MENU    (MEN_TSPLIT,      tTrackWin::MenSplitTracks)
    EVT_MENU    (MEN_METERCH,tTrackWin::MenMeterChange)
    EVT_MENU    (MEN_RESET, tTrackWin::OnReset)
    EVT_MENU    (MEN_HARMONY, tTrackWin::OnHarmony)
    // Need a Random Rhythm Event
    EVT_MENU    (MEN_SHUFFLE,tTrackWin::OnShuffle)
    // Need a Melody Event
    EVT_MENU    (MEN_ARPEGGIO,tTrackWin::OnArpeggio)
    EVT_MENU    (MEN_MAPPER,tTrackWin::OnMapper)
    EVT_MENU    (MEN_EVENTLIST,tTrackWin::OnEventList)
    EVT_MENU    (MEN_COPYRIGHT, tTrackWin::MenCopyright)

    // Audio Menu
    EVT_MENU    (MEN_TWSETTING,tTrackWin::OnSettingsDialog)  // Is this Global Settings?
    // Need Sample Settings event
    // Need Load Set Event
    // Need Save Set Event
    // Need Save Set As Event
    // Need New Set Event

    // Help Menu
    EVT_MENU    (MEN_HELP_JAZZ, tTrackWin::OnHelpJazz)
    EVT_MENU    (MEN_HELP_TWIN, tTrackWin::OnHelpTrackwin)
    EVT_MENU    (MEN_HELP_MOUSE, tTrackWin::OnHelpMouse)
    EVT_MENU    (MEN_ABOUT, tTrackWin::OnAbout)

    // ToolBar Events -- TODO: Make Menu Items for these
    EVT_MENU    (MEN_PLAY, tTrackWin::OnPlay)
    EVT_MENU    (MEN_PLAYLOOP, tTrackWin::OnPlayLoop)
    EVT_MENU    (MEN_RECORD,   tTrackWin::OnRecord)
    EVT_MENU    (MEN_METRO_ON, tTrackWin::OnMetroOn)
END_EVENT_TABLE()


#include "../bitmaps/open.xpm"
#include "../bitmaps/save.xpm"
#include "../bitmaps/new.xpm"
#include "../bitmaps/repl.xpm"
#include "../bitmaps/delete.xpm"
#include "../bitmaps/quantize.xpm"
#include "../bitmaps/mixer.xpm"
#include "../bitmaps/play.xpm"
#include "../bitmaps/undo.xpm"
#include "../bitmaps/redo.xpm"
#include "../bitmaps/zoomin.xpm"
#include "../bitmaps/zoomout.xpm"
#include "../bitmaps/panic.xpm"
#include "../bitmaps/help.xpm"
#include "../bitmaps/pianowin.xpm"
#include "../bitmaps/metro.xpm"
#include "../bitmaps/playloop.xpm"
#include "../bitmaps/record.xpm"

static tToolDef tdefs[] = {
    { MEN_LOAD,      FALSE, open_xpm,     "load song" },
    { MEN_SAVE,      FALSE, save_xpm,     "save song" },
    { MEN_NEW,       FALSE, new_xpm,      "new song" },
    TOOLDEF_SEPARATOR,
    { MEN_REPLICATE, FALSE, repl_xpm,     "replicate selection" },
    { MEN_DELETE,    FALSE, delete_xpm,   "delete selection"  },
    { MEN_QUANTIZE,  FALSE, quantize_xpm, "quantize selection" },
    { MEN_MIXER,     FALSE, mixer_xpm,    "mixer" },
    { MEN_PIANOWIN,  FALSE, pianowin_xpm, "show piano window"    },
    TOOLDEF_SEPARATOR,
    { MEN_PLAY,      FALSE, play_xpm,     "start play"},
    { MEN_PLAYLOOP,  FALSE, playloop_xpm, "loop play"},
    { MEN_RECORD,    FALSE, record_xpm,   "record"},
    { MEN_METRO_ON,  TRUE,  metro_xpm,    "metronome" },
    TOOLDEF_SEPARATOR,
    { MEN_ZOOMIN,    FALSE, zoomin_xpm,   "zoom in" },
    { MEN_ZOOMOUT,   FALSE, zoomout_xpm,  "zoom out"},
    { MEN_UNDO,      FALSE, undo_xpm,     "undo"},
    { MEN_REDO,      FALSE, redo_xpm,     "redo"},
    { MEN_RESET,     FALSE, panic_xpm,    "all notes off"},
    { MEN_HELP_JAZZ, FALSE, help_xpm,     "help" },
    { MEN_ABOUT,     FALSE, help_xpm,     "about" },
    TOOLDEF_END
};


tTrackWin::tTrackWin(wxFrame *frame, char *title, tSong *song, int x, int y, int width, int height)
  : tEventWin(frame, title, song, x, y, width, height)
{
    #ifndef __PORTING
        rhythm_win  = 0;
    #endif // __PORTING
    mapper_win  = 0;
    meldy_win   = 0;
    eventlst_win = 0;
    arpeggio_win = 0;
    prev_clock  = 0;
    prev_loop   = 0;
    prev_muted  = 0;
    prev_record = 0;
    paste_buffer = 0;

    defsong = copystring(lasts);
    defpattern = copystring("noname.mid");

    tool_bar=new tToolBar(this, tdefs);

    // Give the frame a status line for no reason currently
    CreateStatusBar();
    SetStatusText("Welcome to JazzPlusPlus");

    int i;
    int opt;

    opt = GetArgOpt( "-pianowin" ) + 1;
    for (i = 0; i < 4; i++, opt++) {
        if ((wxTheApp->argc > opt) && isdigit( wxTheApp->argv[opt][0] ))
            Config(i+C_PianoWinXpos) = atoi( wxTheApp->argv[opt] );
    else
        break;
    }

    NextWin = new tPianoWin(frame, "Piano Roll", Song, Config(C_PianoWinXpos), Config(C_PianoWinYpos), Config(C_PianoWinWidth), Config(C_PianoWinHeight) );

    tRecordInfo *RecInfo;

    RecInfo = new tRecordInfo();

    RecInfo->Track = 0;
    RecInfo->Muted = 0;

    gProject->SetRecInfo(RecInfo); // gProject will take ownership of this thing

    nBars = 0;

    CounterMode = CmProgram;
    NumberMode = NmMidiChannel;
    MetronomeInfo.IsAccented = Config(C_MetroIsAccented);
    MetronomeInfo.Veloc = Config(C_MetroVelocity);
    MetronomeInfo.KeyNorm = Config(C_MetroNormalClick);
    MetronomeInfo.KeyAcc = Config(C_MetroAccentedClick);
}

tTrackWin::~tTrackWin()
{
    delete mapper_win;
    #ifndef __PORTING
        delete rhythm_win;
        delete meldy_win;
    #endif // __PORTING
    delete [] defsong;
    delete [] defpattern;
    delete paste_buffer;
    delete arpeggio_win;
    delete eventlst_win;
}

// ************************************************************************
// Menubar
// ************************************************************************

void tTrackWin::CreateMenu()
{
    wxMenuBar *menu_bar = NULL;
    file_menu = new wxMenu;
    file_menu->Append(MEN_NEW,           "&New");
    file_menu->Append(MEN_LOAD,          "&Open");
    file_menu->Append(MEN_CLOSE,         "&Close");
    file_menu->Append(MEN_SAVE,          "&Save Project");
    file_menu->Append(MEN_SAVEAS,        "Save Project &as...");
    file_menu->AppendSeparator();
    file_menu->Append(MEN_EXPORTMIDI,    "Export as Midi...");
    file_menu->Append(MEN_EXPORTSELMIDI, "Export Selection as Midi...");
    file_menu->AppendSeparator();
    file_menu->Append(MEN_PREFERENCES,   "&Preferences...");
    file_menu->AppendSeparator();
    file_menu->Append(MEN_QUIT,          "E&xit");

    edit_menu = new wxMenu("");

    edit_menu->Append(MEN_UNDO,     "&Undo ...");
    edit_menu->Append(MEN_REDO,     "&Redo ...");
    edit_menu->AppendSeparator();
    edit_menu->Append(MEN_CLP_CUT,       "&Cut");
    edit_menu->Append(MEN_CLP_COPY,      "C&opy");
    edit_menu->Append(MEN_CLP_PASTE,     "&Paste");
    edit_menu->Append(MEN_CLP_TRIM,     "&Trim");
    edit_menu->AppendSeparator();
    edit_menu->Append(MEN_DELETE,        "&Delete");
    edit_menu->Append(MEN_DELETE,        "&Silence");
    edit_menu->AppendSeparator();
    edit_menu->Append(MEN_SPLIT,        "Split");
    edit_menu->Append(MEN_REPLICATE,     "&Duplicate");
    edit_menu->AppendSeparator();
    edit_menu->Append(MEN_SELECTIONSUB,  "Select (needs submenu)");
    edit_menu->AppendSeparator();

    /* Move Elsewhere
    edit_menu->Append(MEN_QUANTIZE,      "&Quantize ...");
    edit_menu->Append(MEN_SETCHAN,       "&Set MIDI Channel ...");
    edit_menu->Append(MEN_TRANSP,        "&Transpose ...");
    edit_menu->Append(MEN_VELOC,         "&Velocity ...");
    edit_menu->Append(MEN_LENGTH,        "&Length ...");
    edit_menu->Append(MEN_SHIFT,         "Shi&ft ...");
    edit_menu->Append(MEN_CLEANUP,       "C&leanup ...");
    edit_menu->Append(MEN_SEARCHREP,     "Search Re&place ...");
    */

    // Miscellaneous Menu is Stupid.
    // Now it's a View Menu
    misc_menu = new wxMenu;
    misc_menu->Append(MEN_TMERGE,   "Mer&ge Tracks ...");
    misc_menu->Append(MEN_TSPLIT,   "&Split Tracks ...");
    misc_menu->Append(MEN_METERCH,  "&Meterchange ...");
    misc_menu->Append(MEN_RESET,    "&Reset Midi");
    misc_menu->Append(MEN_HARMONY,  "&Harmony Browser...");
    misc_menu->Append(MEN_RHYTHM,   "Random R&hythm...");
    misc_menu->Append(MEN_SHUFFLE,  "Random Sh&uffle...");
    misc_menu->Append(MEN_GENMELDY, "Random Melod&y...");
    misc_menu->Append(MEN_ARPEGGIO, "Random Arpeggio...");
    misc_menu->Append(MEN_MAPPER,   "Ma&pper...");
    misc_menu->Append(MEN_EVENTLIST, "Event &List...");

    misc_menu->Append(MEN_COPYRIGHT,"&Set Music Copyright ...");

    // Move to Project Menu
    file_menu->Append(MEN_LOAD_TMPL,     "Load &Template...");
    file_menu->Append(MEN_LOADPATTERN,   "Load Pattern...");
    file_menu->Append(MEN_SAVEPATTERN,   "Save Pattern...");
    file_menu->AppendSeparator();

    parts_menu = new wxMenu("");
    parts_menu->Append(MEN_MIXER,    "&Mixer ...");
    parts_menu->Append(MEN_MASTER,   "Mas&ter ...");
    parts_menu->Append(MEN_SOUND,    "&Sound ...");
    parts_menu->Append(MEN_VIBRATO,  "&Vibrato ...");
    parts_menu->Append(MEN_ENVELOPE, "&Envelope ...");

    bender_menu = new wxMenu("");
    bender_menu->Append(MEN_BEND_BASIC,    "&Bender Basic...");
    bender_menu->Append(MEN_BEND_LFO1,    "&Bender LFO1...");
    bender_menu->Append(MEN_BEND_LFO2,    "&Bender LFO2...");
    parts_menu->Append(MEN_SUB_BENDER, "&Bender...", bender_menu );

    modulation_menu = new wxMenu("");
    modulation_menu->Append(MEN_MOD_BASIC,    "&Modulation Basic...");
    modulation_menu->Append(MEN_MOD_LFO1,    "&Modulation LFO1...");
    modulation_menu->Append(MEN_MOD_LFO2,    "&Modulation LFO2...");
    parts_menu->Append(MEN_SUB_MODUL, "&Modulation...", modulation_menu );

    caf_menu = new wxMenu("");
    caf_menu->Append(MEN_CAF_BASIC,    "&CAf Basic...");
    caf_menu->Append(MEN_CAF_LFO1,    "&CAf LFO1...");
    caf_menu->Append(MEN_CAF_LFO2,    "&CAf LFO2...");
    parts_menu->Append(MEN_SUB_CAF, "&CAf...", caf_menu );

    paf_menu = new wxMenu("");
    paf_menu->Append(MEN_PAF_BASIC,    "&PAf Basic...");
    paf_menu->Append(MEN_PAF_LFO1,    "&PAf LFO1...");
    paf_menu->Append(MEN_PAF_LFO2,    "&PAf LFO2...");
    parts_menu->Append(MEN_SUB_PAF, "&PAf...", paf_menu );

    cc1_menu = new wxMenu("");
    cc1_menu->Append(MEN_CC1_BASIC,    "&CC1 Basic...");
    cc1_menu->Append(MEN_CC1_LFO1,    "&CC1 LFO1...");
    cc1_menu->Append(MEN_CC1_LFO2,    "&CC1 LFO2...");
    parts_menu->Append(MEN_SUB_CC1, "&CC1...", cc1_menu );

    cc2_menu = new wxMenu("");
    cc2_menu->Append(MEN_CC2_BASIC,    "&CC2 Basic...");
    cc2_menu->Append(MEN_CC2_LFO1,    "&CC2 LFO1...");
    cc2_menu->Append(MEN_CC2_LFO2,    "&CC2 LFO2...");
    parts_menu->Append(MEN_SUB_CC2, "&CC2...", cc2_menu );

    parts_menu->Append(MEN_DRUM_PARAM,    "&Drum Parameters...");
    parts_menu->Append(MEN_PART_RSRV,    "&Partial Reserve...");
    parts_menu->Append(MEN_PART_MODE,    "&Part Mode...");

    setting_menu = new wxMenu("");
    setting_menu->Append(MEN_FILTER,    "&Filter ...");
    setting_menu->Append(MEN_TWSETTING, "&Window ...");
    setting_menu->Append(MEN_SONG,      "&Song ...");
    setting_menu->Append(MEN_METRONOME, "&Metronome ...");
    setting_menu->Append(MEN_EFFECTS,   "&Effects ...");
    setting_menu->Append(MEN_TIMING,    "&Timing ...");
    setting_menu->Append(MEN_MIDI_THRU, "&Midi Thru ...");
    setting_menu->Append(MEN_SYNTH_SETTINGS, "&Synth Type ...");

    #ifdef wx_msw
        setting_menu->Append(MEN_DEVICE,    "&Midi Device...");
    #else
        if (Config(C_MidiDriver) == C_DRV_OSS || Config(C_MidiDriver) == C_DRV_ALSA)
            setting_menu->Append(MEN_DEVICE,    "&Midi Device...");
    #endif
    save_settings_menu = new wxMenu;
    save_settings_menu->Append( MEN_SAVE_THRU, "&Midi Thru" );
    save_settings_menu->Append( MEN_SAVE_TIM, "&Timing" );
    save_settings_menu->Append( MEN_SAVE_EFF, "&Effect Macros" );
    save_settings_menu->Append( MEN_SAVE_GEO, "&Window Geometry" );
    save_settings_menu->Append( MEN_SAVE_METRO, "&Metronome" );
    // save_settings_menu->Append( MEN_SAVE_SYNTH, "&Synth Type" );
    save_settings_menu->Append( MEN_SAVE_ALL, "&Save All" );
    setting_menu->Append(MEN_SAVE_SET, "&Save settings", save_settings_menu );


    help_menu = new wxMenu;
    help_menu->Append(MEN_HELP_JAZZ, "&Jazz");
    help_menu->Append(MEN_HELP_TWIN, "&Trackwin");
    help_menu->Append(MEN_HELP_MOUSE, "&Mouse");
    help_menu->Append(MEN_ABOUT, "&About");

    menu_bar = new wxMenuBar();
    menu_bar->Append(file_menu,    "&File");
    menu_bar->Append(edit_menu,    "&Edit");
    menu_bar->Append(misc_menu,    "&View");
    menu_bar->Append(parts_menu,   "&Parts");
    menu_bar->Append(setting_menu, "&Settings");

    audio_menu = new wxMenu();
    audio_menu->Append(MEN_AUDIO_GLOBAL,        "&Global Settings ...");
    audio_menu->Append(MEN_AUDIO_SAMPLES,       "Sample Se&ttings ... ");
    audio_menu->Append(MEN_AUDIO_LOAD,          "&Load Set ...");
    audio_menu->Append(MEN_AUDIO_SAVE,          "&Save Set");
    audio_menu->Append(MEN_AUDIO_SAVE_AS,       "Save Set &As");
    audio_menu->Append(MEN_AUDIO_NEW,           "&New Set");
    menu_bar->Append(audio_menu, "&Audio");

    menu_bar->Append(help_menu,    "&Help");

    SetMenuBar(menu_bar);

    EnableDisableMenus();
}

void tTrackWin::EnableDisableMenus()
{
    bender_menu->Enable(MEN_BEND_LFO1, TRUE);
    bender_menu->Enable(MEN_BEND_LFO2, TRUE);
    parts_menu->Enable(MEN_SUB_MODUL, TRUE);
    parts_menu->Enable(MEN_SUB_CAF, TRUE);
    parts_menu->Enable(MEN_SUB_PAF, TRUE);
    parts_menu->Enable(MEN_SUB_CC1, TRUE);
    parts_menu->Enable(MEN_SUB_CC2, TRUE);

    bender_menu->Enable(MEN_BEND_LFO2, TRUE);
    modulation_menu->Enable(MEN_MOD_LFO2, TRUE);
    caf_menu->Enable(MEN_CAF_LFO2, TRUE);
    paf_menu->Enable(MEN_PAF_LFO2, TRUE);
    cc1_menu->Enable(MEN_CC1_LFO2, TRUE);
    cc2_menu->Enable(MEN_CC2_LFO2, TRUE);
    parts_menu->Enable(MEN_PART_RSRV, TRUE);
    setting_menu->Enable(MEN_EFFECTS, TRUE);

    if (Synth->IsGM())
    {
        bender_menu->Enable(MEN_BEND_LFO1, FALSE);
        bender_menu->Enable(MEN_BEND_LFO2, FALSE);
        parts_menu->Enable(MEN_SUB_MODUL, FALSE);
        parts_menu->Enable(MEN_SUB_CAF, FALSE);
        parts_menu->Enable(MEN_SUB_PAF, FALSE);
        parts_menu->Enable(MEN_SUB_CC1, FALSE);
        parts_menu->Enable(MEN_SUB_CC2, FALSE);
        setting_menu->Enable(MEN_EFFECTS, FALSE);
    }
    else if (Synth->IsXG())
    {
        bender_menu->Enable(MEN_BEND_LFO2, FALSE);
        modulation_menu->Enable(MEN_MOD_LFO2, FALSE);
        caf_menu->Enable(MEN_CAF_LFO2, FALSE);
        paf_menu->Enable(MEN_PAF_LFO2, FALSE);
        cc1_menu->Enable(MEN_CC1_LFO2, FALSE);
        cc2_menu->Enable(MEN_CC2_LFO2, FALSE);
        parts_menu->Enable(MEN_PART_RSRV, FALSE);
    }
}


/*
 * Event Handlers
 */


// Old Event Loop.  Replace this as soon as possible and remove it from the code.
/* PAT - This entire function doesn't seem to be called anywhere.  I commented
   it out with no effect.  It's just here for reference since a few of the
   items aren't moved elsewhere yet (ex. MEN_RHYTHM handling). */
/*
void tTrackWin::OnMenuCommand(int id)
{
  char *s;

  if (Midi->OnMenuCommand(id))
    return;

  switch (id)
  {
#ifndef __PORTING

    case MEN_RHYTHM:
      if (!rhythm_win)
	rhythm_win = new tRhythmWin(this, Song);
      rhythm_win->Show(TRUE);
      break;
    case MEN_GENMELDY:
      if (!meldy_win)
        meldy_win = new tGenMelody(this, (wxFrame **)&meldy_win);
      meldy_win->Show(TRUE);
      break;
    case MEN_DEBUG:
      //debug(this);
      break;
    case MEN_MIXER:
      MenMixer();
      break;
    case MEN_MASTER:
      MenMaster();
      break;
    case MEN_VIBRATO:
      MenVibrato();
      break;
    case MEN_SOUND:
      MenSound();
      break;
    case MEN_ENVELOPE:
      MenEnvelope();
      break;
    case MEN_BEND_BASIC:
      MenBendBasic();
      break;
    case MEN_BEND_LFO1:
      MenBendLfo1();
      break;
    case MEN_BEND_LFO2:
      MenBendLfo2();
      break;
    case MEN_MOD_BASIC:
      MenModBasic();
      break;
    case MEN_MOD_LFO1:
      MenModLfo1();
      break;
    case MEN_MOD_LFO2:
      MenModLfo2();
      break;
    case MEN_CAF_BASIC:
      MenCAfBasic();
      break;
    case MEN_CAF_LFO1:
      MenCAfLfo1();
      break;
    case MEN_CAF_LFO2:
      MenCAfLfo2();
      break;
    case MEN_PAF_BASIC:
      MenPAfBasic();
      break;
    case MEN_PAF_LFO1:
      MenPAfLfo1();
      break;
    case MEN_PAF_LFO2:
      MenPAfLfo2();
      break;
    case MEN_CC1_BASIC:
      MenCC1Basic();
      break;
    case MEN_CC1_LFO1:
      MenCC1Lfo1();
      break;
    case MEN_CC1_LFO2:
      MenCC1Lfo2();
      break;
    case MEN_CC2_BASIC:
      MenCC2Basic();
      break;
    case MEN_CC2_LFO1:
      MenCC2Lfo1();
      break;
    case MEN_CC2_LFO2:
      MenCC2Lfo2();
      break;
    case MEN_DRUM_PARAM:
      MenDrumParam();
      break;
    case MEN_PART_RSRV:
      MenPartRsrv();
      break;
    case MEN_PART_MODE:
      MenPartMode();
      break;
    case MEN_TIMING:
      MenTiming();
      break;
    case MEN_MIDI_THRU:
      MenMidiThru();
      break;
    case MEN_SYNTH_SETTING:
      MenSynthSettings();
      break;
#endif // __PORTING
  }
}
*/

// Has Event Macro
void tTrackWin::OnLoad(){
    if (MixerForm) {
        wxMessageBox("Quit parts dialog first.", "Info", wxOK);
    }

    wxString s;
    s = file_selector(defsong, "Load File", 0, tTrack::changed, "*.mid");
    //      s = file_selector(defsong, "Load File", 0, 0, "*.mid");
    if (s!=wxEmptyString)
    {
        //load the song
        tStdRead io;
        Song->Clear();
        Song->Read(io, s);
        SetTitle(s);
        NextWin->NewPosition(1, 0);
        Canvas->SetScrollRanges();
        NextWin->Canvas->SetScrollRanges();
        Redraw();
            tTrack::changed = 0;
    }
}

// Has Event Macro
void tTrackWin::OnSave(){
    if (strcmp(defsong, "noname.mid") == 0)
        OnSaveAs();
    else
    {
        tStdWrite io;
        Song->Write(io, defsong);
        tTrack::changed = 0;
        Config.Put(C_StartUpSong, defsong);
    }
}

// Has Event Macro
void tTrackWin::OnSaveAs()
{
    {
        wxString s = file_selector(defsong, "Save File", 1, 0, "*.mid");
        if (s)
        {
            tStdWrite io;
            Song->Write(io, s);
            SetTitle(s);
            tTrack::changed = 0;
            Config.Put(C_StartUpSong, s);
        }
    }
}

// Has Event Macro
void tTrackWin::OnNew(){
    if (MixerForm) {
        wxMessageBox("Quit parts dialog first.", "Info", wxOK);
        return;
    }
    if (wxMessageBox("Clear Song?", "Sure?", wxOK | wxCANCEL) == wxOK)
    {
        Song->Clear();
        Redraw();
        delete [] defsong;
        defsong = copystring("noname.mid");
        SetTitle(defsong);
        NextWin->NewPosition(1, 0);
    }
}

// Has Event Macro
void tTrackWin::OnLoadTemplate()
{
	if (MixerForm) {
	  wxMessageBox("Quit parts dialog first.", "Info", wxOK);
	  return;
	}
	wxString s;
	s = file_selector(defsong, "Load Template", 0, tTrack::changed, "*.mid");
	if (s!=wxEmptyString)
    {
	    tStdRead io;
	    Song->Clear();
	    Song->Read(io, s);
	    delete [] defsong;
	    defsong = copystring("noname.mid");
	    SetTitle(defsong);
	    NextWin->NewPosition(1, 0);
	    Canvas->SetScrollRanges();
	    NextWin->Canvas->SetScrollRanges();
	    Redraw();
	    tTrack::changed = 0;
    }
}

// Has Event Macro
void tTrackWin::MenLoadPattern()
{
    if (MouseAction)
        return;
    wxString fname = file_selector(defpattern, "Load Pattern", 0, 0, "*.mid");
    if (fname)
        MouseAction = new tLoadPattern(this, fname);
}

// Has Event Macro
void tTrackWin::MenSavePattern()
{
    if (!EventsSelected("please select events to be saved"))
        return;
    wxString fname = file_selector(defpattern, "Save Selected Range", 1, 0, "*.mid");
    if (fname) {
        int tracknr = 0;
        tSong *sng = new tSong;
        delete sng;
        sng = new tSong;
        tTrackIterator Tracks(Filter);
        tTrack *src = Tracks.First();
        while (src != 0)
        {
            tTrack *dst = sng->GetTrack(tracknr++);
            tEventIterator Events(src);
            long delta = Filter->FromClock;
            tEvent *e = Events.Range(Filter->FromClock, Filter->ToClock);
            while (e) {
                if (Filter->IsSelected(e)) {
                e = e->Copy();
                e->Clock -= delta;
                dst->Put(e);
                }
                e = Events.Next();
            }
        dst->Cleanup();
        src = Tracks.Next();
        }
        tStdWrite io;
        sng->Write(io, fname);
        delete sng;
        Config.Put(C_StartUpSong, fname);
    }
}

// Has Event Macro
void tTrackWin::OnQuit(){
    if ( OnClose() == FALSE)
        return;
    DELETE_THIS();
}

// Called from OnQuit
bool tTrackWin::OnClose()
{
    if (tTrack::changed) {
        if (wxMessageBox("Song has changed. Quit anyway?", "Quit ?", wxYES_NO) == wxNO)
            return FALSE;
    }
    if (Midi->Playing)
    {
        Midi->StopPlay();
    #ifndef wx_msw
        sleep(1);
    #endif
    }
    delete the_harmony_browser;
    delete Midi;
    delete NextWin;
    return TRUE;
}

void tTrackWin::OnExportMidi()
{
}

void tTrackWin::OnExportSelMidi()
{
}

void tTrackWin::OnPreferences()
{
}

void tTrackWin::MenClpTrim()
{
}

void tTrackWin::MenSplit()
{
}

void tTrackWin::MenSelectionSub()
{
}

// ----------------------------------------------------------------------------
//                              clipboard copy/paste
// ----------------------------------------------------------------------------

// Has Event Macro
void tTrackWin::MenClpCopyEraseSrc()
{
    MenClpCopy(TRUE);
}

// Has Event Macro
void tTrackWin::MenClpCopyLeaveSrc()
{
    MenClpCopy(FALSE);
}

// Called from Cut/Copy event handlers
void tTrackWin::MenClpCopy(bool erase)
{
    if (!EventsSelected("please select events to be copied"))
        return;
    Song->NewUndoBuffer();

    int tracknr = 0;

    delete paste_buffer;
    paste_buffer = new tSong;
    tTrackIterator Tracks(Filter);
    tTrack *src = Tracks.First();
    while (src != 0)
    {
        tTrack *dst = paste_buffer->GetTrack(tracknr++);
        tEventIterator Events(src);
        long delta = Filter->FromClock;
        tEvent *e = Events.Range(Filter->FromClock, Filter->ToClock);
        while (e) {
            if (Filter->IsSelected(e)) {
                tEvent *c = e->Copy();
                c->Clock -= delta;
                dst->Put(c);
                if (erase)
                    src->Kill(e);
            }
            e = Events.Next();
        }
        dst->Cleanup();
        if (erase)
            src->Cleanup();
        src = Tracks.Next();
    }
    paste_buffer->TicksPerQuarter = Song->TicksPerQuarter;
    if (erase)
        Redraw();
}

// Has Event Macro
void tTrackWin::MenClpPaste() {
    if (MouseAction || paste_buffer == NULL)
        return;
    MouseAction = new tClpPaste(this, paste_buffer);
}

// Has Event Macro
// This is really Edit->Replicate.  The Method name is misleading
void tTrackWin::MenCopy()
{
    if (!EventsSelected())
        return;
    MouseAction = new tCopyCommand(this);
}

///////////////////////////////////////

// tTrackWin::MenDelete GOES HERE!!!!!!!!!!!!

/*
       All of these event macros are defined and lack handlers

    EVT_MENU    (MEN_QUANTIZE  ,tTrackWin::MenQuantize)
    EVT_MENU    (MEN_SETCHAN,tTrackWin::MenSetChannel)
    EVT_MENU    (MEN_TRANSP,tTrackWin::MenTranspose)
    EVT_MENU    (MEN_VELOC,tTrackWin::MenVelocity)
    EVT_MENU    (MEN_LENGTH,tTrackWin::MenLength)

 */

///////////////////////////////////////

// has Event Macro
void tTrackWin::OnShift()
{
    MenShift(GetPianoWin()->SnapClocks());
}

///////////////////////////////////////

// tTrackWin::MenDelete GOES HERE!!!!!!!!!!!!

/*
       All of these event macros are defined and lack handlers

    EVT_MENU    (MEN_CLEANUP,tTrackWin::MenCleanup)
    EVT_MENU    (MEN_SEARCHREP,tTrackWin::MenSearchReplace)

 */

///////////////////////////////////////


/**create and show a copyright dialog*/
void tTrackWin::MenCopyright()
{
    tCopyrightDlg* dlg=new tCopyrightDlg(Song);
    dlg->Create();
}

#ifndef __PORTING

void tTrackWin::MenSynthSettings()
{
    tSynthSettingsDlg *dlg;
    if (DialogBox)
    {
        DialogBox->Show(TRUE);
        return;
    }
    DialogBox = new wxDialog(this, -1, "Synth Type" );
    dlg = new tSynthSettingsDlg(this);
    dlg->EditForm(DialogBox);
    DialogBox->Fit();
    DialogBox->Show(TRUE);
}

#endif // __PORTING


#ifndef __PORTING

void tTrackWin::MenSongSettings()
{
    tSongSettingsDlg *dlg;
    if (DialogBox)
    {
        DialogBox->Show(TRUE);
        return;
    }
    DialogBox = new wxDialogBox(this, "Song Settings", FALSE );
    dlg = new tSongSettingsDlg(this);
    dlg->EditForm(DialogBox);
    DialogBox->Fit();
    DialogBox->Show(TRUE);
}


void tTrackWin::MenMetronomeSettings()
{
    tMetronomeSettingsDlg *dlg;
    if (DialogBox)
    {
        DialogBox->Show(TRUE);
        return;
    }
    DialogBox = new wxDialogBox(this, "Metronome Settings", FALSE );
    dlg = new tMetronomeSettingsDlg(this);
    dlg->EditForm(DialogBox);
    DialogBox->Fit();
    DialogBox->Show(TRUE);
}


void tTrackWin::MenTiming()
{
    tTimingDlg *dlg;
    if (DialogBox)
    {
        DialogBox->Show(TRUE);
        return;
    }
    DialogBox = new wxDialogBox(this, "MIDI Timing Settings", FALSE );
    dlg = new tTimingDlg(this);
    dlg->EditForm(DialogBox);
    DialogBox->Fit();
    DialogBox->Show(TRUE);
}


void tTrackWin::MenMidiThru()
{
    tMidiThruDlg *dlg;
    if (DialogBox)
    {
        DialogBox->Show(TRUE);
        return;
    }
    DialogBox = new wxDialogBox(this, "Midi Thru Settings", FALSE );
    dlg = new tMidiThruDlg(this);
    dlg->EditForm(DialogBox);
    DialogBox->Fit();
    DialogBox->Show(TRUE);
}

#endif // __PORTING


// *************************************************************************************
// Split / Merge Tracks
// *************************************************************************************

void tTrackWin::MenMergeTracks()
{
    if (MixerForm) {
        wxMessageBox("Quit parts dialog first.", "Info", wxOK);
        return;
    }

    int choice = wxMessageBox("Merge all Tracks to Track 0?", "Sure?", wxOK | wxCANCEL);
    if (choice == wxOK)
    {
        int tn;
        Song->NewUndoBuffer();
        tTrack *dst = Song->GetTrack(0);
        for (tn = 1; tn < Song->nTracks; tn++)
        {
            tTrack *src = Song->GetTrack(tn);
            tEventIterator Iterator(src);
            tEvent *e = Iterator.First();
            while (e)
            {
                tEvent *c = e->Copy();
                src->Kill(e);
                dst->Put(c);
                e = Iterator.Next();
            }
            src->Cleanup();
        }
        dst->Cleanup();
        Redraw();
    }
}


void tTrackWin::MenSplitTracks()
{
    if (MixerForm) {
        wxMessageBox("Quit parts dialog first.", "Info", wxOK);
        return;
    }

    int choice = wxMessageBox("Split Track 0 by Midi-Channel to Track 1..16?", "Sure?", wxOK | wxCANCEL);
    if (choice == wxOK)
    {
        int ch;
        Song->NewUndoBuffer();
        tTrack *src = Song->GetTrack(0);
        tEventIterator Iterator(src);
        tEvent *e = Iterator.First();
        while (e)
        {
            tChannelEvent *ce = e->IsChannelEvent();
            if (ce)
            {
                int cn = ce->Channel;
                tTrack *dst = Song->GetTrack(cn + 1);
                if (dst)
                {
                    tEvent *cp = ce->Copy();
                    src->Kill(ce);
                    dst->Put(cp);
                    dst->Channel = cn + 1;
                }
            }
            e = Iterator.Next();
        }

        for (ch = 0; ch <= 16; ch++)
        {
            src = Song->GetTrack(ch);
            if (src)
                src->Cleanup();
        }
        Redraw();
    }
}


void tTrackWin::SaveMidiDeviceSettings( int dev )
{
    Config.Put( C_Seq2Device, dev );
}

void tTrackWin::SaveThruSettings()
{
    Config.Put( C_SoftThru );
    Config.Put( C_HardThru );
    Config.Put( C_ThruInput );
    Config.Put( C_ThruOutput );
}

void tTrackWin::SaveTimingSettings()
{
    Config.Put( C_RealTimeOut );
    Config.Put( C_ClockSource );
}

void tTrackWin::SaveEffectSettings()
{
    Config.Put( C_UseReverbMacro );
    Config.Put( C_UseChorusMacro );
}

void tTrackWin::SaveMetronomeSettings()
{
    Config.Put( C_MetroIsAccented, MetronomeInfo.IsAccented );
    Config.Put( C_MetroVelocity, MetronomeInfo.Veloc );
    Config.Put( C_MetroNormalClick, MetronomeInfo.KeyNorm );
    Config.Put( C_MetroAccentedClick, MetronomeInfo.KeyAcc );
}

void tTrackWin::SaveGeoSettings()
{
    GetPosition( &Config(C_TrackWinXpos), &Config(C_TrackWinYpos) );
    GetSize( &Config(C_TrackWinWidth), &Config(C_TrackWinHeight) );
    Config.Put( C_TrackWinXpos );
    Config.Put( C_TrackWinYpos );
    Config.Put( C_TrackWinWidth );
    Config.Put( C_TrackWinHeight );

    if (NextWin)
    {
        NextWin->GetPosition( &Config(C_PianoWinXpos), &Config(C_PianoWinYpos) );
        NextWin->GetSize( &Config(C_PianoWinWidth), &Config(C_PianoWinHeight) );
        Config.Put( C_PianoWinXpos );
        Config.Put( C_PianoWinYpos );
        Config.Put( C_PianoWinWidth );
        Config.Put( C_PianoWinHeight );
    }

    if (MixerForm)
    {
        MixerForm->GetPosition( &Config(C_PartsDlgXpos), &Config(C_PartsDlgYpos) );
    }
    Config.Put( C_PartsDlgXpos );
    Config.Put( C_PartsDlgYpos );
    Config.Put( C_TrackDlgXpos );
    Config.Put( C_TrackDlgYpos );

    #ifndef __PORTING
    if (rhythm_win)
    {
        rhythm_win->GetPosition( &Config(C_RhythmXpos), &Config(C_RhythmYpos) );
    }
    Config.Put( C_RhythmXpos );
    Config.Put( C_RhythmYpos );

    if (the_harmony_browser)
    {
        ((HBFrame *)the_harmony_browser)->GetPosition( &Config(C_HarmonyXpos), &Config(C_HarmonyYpos) );
    }
    Config.Put( C_HarmonyXpos );
    Config.Put( C_HarmonyYpos );
    #endif // __PORTING
}

void tTrackWin::OnFilter()
{
    Filter->Dialog(0);
}


static long TrackFontSizes[] =
{
  8,  // Tiny
  10, // Small
  12, // Medium
  14, // Large
  17, // Huge
  -1, // End of List
};

void tTrackWin::OnSettingsDialog()
{
  jppResourceDialog dialog(this, "windowSettings");
  
  dialog.Attach("use_colours", &UseColors);
  dialog.Attach("font_size", &FontSize, TrackFontSizes);

  if(dialog.ShowModal() == wxID_OK) {
    Setup();
    Canvas->SetScrollRanges();
    Redraw();
  }
}


void tTrackWin::OnDevice()
{
#ifdef wx_msw
    long idev = Config(C_WinInputDevice);
    long odev = Config(C_WinOutputDevice);
    tWinPlayer::SettingsDlg(idev, odev);
    wxMessageBox("Restart jazz to activate changes in device settings", "Info", wxOK);
#else
    if (Config(C_MidiDriver) == C_DRV_OSS || Config(C_MidiDriver) == C_DRV_ALSA)
    {
        int dev = Midi->FindMidiDevice();
        if (dev >= 0)
        {
            SaveMidiDeviceSettings( dev );
            if (Config(C_MidiDriver) == C_DRV_OSS)
                wxMessageBox("Restart jazz to activate changes in device settings", "Info", wxOK);
        }
        else
            wxMessageBox("No midi device found", "Info", wxOK);
    }
#endif
}

void tTrackWin::OnShuffle()
{
	if (!EventsSelected())
	  return;
	tShuffleDlg * dlg = new tShuffleDlg(this, Filter);
	dlg->Create();
}

void tTrackWin::OnHarmony()
{
    harmony_browser(this);
}

void tTrackWin::OnHelpJazz(){
    HelpInstance->DisplayContents();
}

void tTrackWin::OnHelpTrackwin (){
    HelpInstance->ShowTopic("Track Window");
}

void tTrackWin::OnReset(){
    Midi->AllNotesOff(1);
}

void tTrackWin::OnPlayLoop(){
    MousePlay(0, PlayLoopButton);
}

void tTrackWin::OnRecord(){
    MousePlay(0, RecordButton);
}

void tTrackWin::OnMetroOn(){
    MetronomeInfo.IsOn = !MetronomeInfo.IsOn;
}

void tTrackWin::OnSaveAll(){
    SaveThruSettings();
    SaveTimingSettings();
    SaveEffectSettings();
    SaveGeoSettings();
    SaveMetronomeSettings();
}

void tTrackWin::OnUndo(){
    //    case :
    Song->Undo();
    Redraw();
    NextWin->Redraw();
}

void tTrackWin::OnRedo(){
    //    case :
    Song->Redo();
    Redraw();
    NextWin->Redraw();
}

void tTrackWin::OnMapper(){
    if (!mapper_win)
        mapper_win = new tMapperWin(this, Song);
    mapper_win->Show(TRUE);
}

void tTrackWin::OnArpeggio(){
    if (!EventsSelected())
        return;
    if (!arpeggio_win)
        arpeggio_win = new tArpeggioWin(this, (wxFrame **)&arpeggio_win);
    arpeggio_win->Show(TRUE);
}

void tTrackWin::OnEventList(){
    //    case MEN_EVENTLIST:
    if (!EventsSelected())
        return;
    if (Filter->FromTrack != Filter->ToTrack) {
        wxMessageDialog(this,"you must select exacty 1 track", "Error", wxOK);
        return;
    }

    if (!eventlst_win)
        eventlst_win = new tEventList(this, (wxFrame **)&eventlst_win);
    //eventlst_win->Create();
    eventlst_win->Scan(Filter);
    eventlst_win->Show(TRUE);
}

void tTrackWin::OnZoomIn(){
    if (ClocksPerPixel>12)
        ZoomIn();
}

void tTrackWin::OnZoomOut(){
    if (ClocksPerPixel<120)
        ZoomOut();
}

void tTrackWin::OnPianowin(){
    NextWin->Show(TRUE);
}

void tTrackWin::OnPlay(){
    cout<<"tTrackWin::OnPlay\n";
    MousePlay(0, PlayButton);
}


void tTrackWin::MenSongSettings()
{
    tSongSettingsDlg *dlg;

    dlg = new tSongSettingsDlg(this);
    dlg->Create();
}

void tTrackWin::OnAbout(){
    extern const char *about_text;
    wxMessageBox( (char *) about_text, "About", wxOK);
}

void tTrackWin::OnHelpMouse(){
    wxMessageBox( (char *)
        "topline:\n"
        "  left: start/stop record/play\n"
        "    +shift: start/stop cycle record/play\n"
        "  middle: same as left+shift\n"
        "  right: start/stop record/play, mute selected track\n"
        "\n"
        "events:\n"
        "  left: select events\n"
        "    +shift: continue selection\n"
        "  right: open and position pianowin\n", "Help", wxOK);
}

#if 0
static void debug(tTrackWin *tw)
{
#ifndef wx_msw
  // debug: place a Reset Message into the song
  if (0) {
    tEvent *e = Synth->Reset();
    tTrack *t = tw->Song->GetTrack(0);
    e = e->Copy();
    e->Clock = tw->Song->TicksPerQuarter * 18;
    t->Put(e);
    t->Cleanup();
  }
  /*
  for (int i = 16; i < 20; i++)
  {
     Midi->OutNow( Synth->ReverbMacroSX( 0, i, 0 ) );
     printf("%d\n", i );
     sleep(1);
  }
  */
  for (int i = 64; i < 73; i++)
  {
     Midi->OutNow( Synth->ChorusMacroSX( 0, i, 0 ) );
     printf("%d\n", i );
     sleep(1);
  }
#endif
}
#endif

void tTrackWin::Setup()
{
    long x, y;

    tEventWin::Setup();

    wxDC* dc=new wxClientDC(Canvas);
    dc->GetTextExtent("H", &x, &y);
    LittleBit = (int)(x/2);

    dc->GetTextExtent("HXWjgi", &x, &y);
    hLine = (int)y + LittleBit;
    hTop = hFixedFont + 2 * LittleBit;

    dc->GetTextExtent("99", &x, &y);
    wNumber = (int)x + LittleBit;

    dc->GetTextExtent("Normal Trackname", &x, &y);
    wName = (int)x + LittleBit;

    dc->GetTextExtent("m", &x, &y);
    wState = (int)x + LittleBit;

    dc->GetTextExtent("999", &x, &y);
    wPatch = (int)x + 2 * LittleBit;

    wLeft = wNumber + wName + wState + wPatch + 1;
    cout <<" "<<wNumber<<" "<<wName<<" "<<wState<<" "<<wPatch<<" "<<wLeft<<endl;
    UnMark();
    delete dc;
}

#ifndef wxRELEASE_NUMBER

#define _MAXPATHLEN 500

// Return just the filename, not the path
// (basename)
char *wxFileNameFromPath (char *path)
{
  if (path)
    {
      register char *tcp;

      tcp = path + strlen (path);
      while (--tcp >= path)
	{
	  if (*tcp == '/' || *tcp == '\\'
#ifdef VMS
     || *tcp == ':' || *tcp == ']')
#else
     )
#endif
	    return tcp + 1;
	}                       /* while */
#ifdef wx_msw
      if (isalpha (*path) && *(path + 1) == ':')
	return path + 2;
#endif
    }
  return path;
}

// Return just the directory, or NULL if no directory
char *wxPathOnly (char *path)
{
  if (path && *path)
    {
      static char buf[_MAXPATHLEN];

      // Local copy
      strcpy (buf, path);

      int l = strlen(path);
      bool done = FALSE;

      int i = l - 1;

      // Search backward for a backward or forward slash
      while (!done && i > -1)
      {
	if (path[i] == '/' || path[i] == '\\')
	{
	  done = TRUE;
	  buf[i] = 0;
	  return buf;
	}
	else i --;
      }

/* there's a bug here somewhere, so replaced with my original code.
      char *tcp;
      // scan back
      for (tcp = &buf[strlen (buf) - 1]; tcp >= buf; tcp--)
	{
	  // Search for Unix or Dos path sep {'\\', '/'}
	  if (*tcp == '\\' || *tcp == '/')
	    {
	      *tcp = '\0';
	      return buf;
	    }
	}                       // for()
*/
#ifdef wx_msw
      // Try Drive specifier
      if (isalpha (buf[0]) && buf[1] == ':')
	{
	  // A:junk --> A:. (since A:.\junk Not A:\junk)
	  buf[2] = '.';
	  buf[3] = '\0';
	  return buf;
	}
#endif
    }

  return NULL;
}
#endif

// ************************************************************************
// Painting
// ************************************************************************

const char *tTrackWin::CounterStr()
{
    const char *str;
    switch (CounterMode)
    {
        case CmProgram: str = "Prg"; break;
        case CmBank   : str = "Bnk"; break;
        case CmVolume : str = "Vol"; break;
        case CmPan    : str = "Pan"; break;
        case CmReverb : str = "Rev"; break;
        case CmChorus : str = "Cho"; break;
        default       : str = "???"; break;
    }
    return str;
}

void tTrackWin::DrawCounters(wxDC* dc)
{
    int i;
    const char *str = CounterStr();
    LineText(dc, xPatch, CanvasY-1, wPatch, str, hTop);

    dc->SetClippingRegion(xPatch, yEvents, xPatch + wPatch, yEvents + hEvents);
    for (i = FromLine; i < ToLine; i++)
    {
        tTrack *t = Song->GetTrack(i);
        if (t)
        {
            char buf[20];
            int  val;
            switch (CounterMode)
            {
                case CmProgram: val = t->GetPatch(); break;
                case CmBank   : val = t->GetBank(); break;
                case CmVolume : val = t->GetVolume(); break;
                case CmPan    : val = t->GetPan(); break;
                case CmReverb : val = t->GetReverb(); break;
                case CmChorus : val = t->GetChorus(); break;
                default : val = 0; break;
            }
            sprintf(buf, "%3d", val);
            LineText(dc,xPatch, Line2y(i), wPatch, buf);
        }
        else
            LineText(dc,xPatch, Line2y(i), wPatch, "?");
    }
    dc->DestroyClippingRegion();
}

/**
retrieves a string indicating the current use of the "numbers" column, which is the leftmost one.
T means track number, M means midi channel
*/
const char* tTrackWin::NumberStr()
{
    const char *str;
    switch (NumberMode)
    {
        case NmTrackNr     : str = "T"; break;
        case NmMidiChannel : str = "M"; break;
        default            : str = "?"; break;
    }
    return str;
}

/**
draws the "numbers" column(leftmost one) which either represents track numbers or midi channel depending on mode
*/
void tTrackWin::DrawNumbers(wxDC* dc)
{
    const char *str = NumberStr();
    int i;
    LineText(dc, xNumber, CanvasY-1, wNumber, str, hTop);

    dc->SetClippingRegion(xNumber, yEvents, xNumber + wNumber, yEvents + hEvents);
    for (i = FromLine; i < ToLine; i++)
    {
        tTrack *t = Song->GetTrack(i);
        if (t != 0)
        {
            if (t->GetAudioMode())
                LineText(dc,xNumber, Line2y(i), wNumber, "Au");
            else
            {
                char buf[20];
                int  val;
                switch (NumberMode)
                {
                    case NmTrackNr: val = i; break;
                    case NmMidiChannel : val = t->Channel; break;
                    default : val = 0; break;
                }
                sprintf(buf, "%02d", val);
                LineText(dc,xNumber, Line2y(i), wNumber, buf);
            }
        }
    }
    dc->DestroyClippingRegion();
}

/**
draws the "speed" tempo indicator
in the top left part of the canvas
*/
void tTrackWin::DrawSpeed(wxDC* dc,int Value, bool down)
{
    char buf[50];

    if (Value < 0)
        Value = Song->GetTrack(0)->GetDefaultSpeed();

    sprintf(buf, "speed: %3d", Value);

    LineText(dc, xName, CanvasY-1, wName, buf, hTop, down);
}

/**
onpaintsub is called by canvas, and draws the entire canvas
*/
void tTrackWin::OnPaintSub(wxDC* dc,long x, long y)
{
    cout <<"tTrackWin::OnPaintSub +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"<<endl;
    tEventWin::OnPaintSub(dc, x, y);

    xNumber  = CanvasX;
    xName    = xNumber  + wNumber;
    xState   = xName    + wName;
    xPatch   = xState   + wState;

    long StopClk;


    tBarInfo BarInfo(Song);


    char buf[20];

    dc->BeginDrawing();
    dc->DestroyClippingRegion();
    DrawPlayPosition(dc);
    dc->SetBackground(*wxWHITE_BRUSH);
    dc->Clear();

    // clear playposition and selection

    #define VLine(x) DrawLine(x,  CanvasY, x, yEvents+hEvents)
    #define HLine(y) DrawLine(CanvasX, y, CanvasX + CanvasW, y)

    dc->SetPen(*wxBLACK_PEN);

    // vertical lines
    dc->VLine(xNumber);
    dc->VLine(xName);
    dc->VLine(xState);
    dc->VLine(xPatch);
    // SN+ dc->VLine(xEvents);
    dc->VLine(xEvents-1);
    dc->HLine(yEvents);
    dc->HLine(yEvents-1);

    // Taktstriche und -nummern

    BarInfo.SetClock(FromClock);
    StopClk = x2Clock(CanvasX + CanvasW);
    nBars = 0;
    int intro = Song->GetIntroLength();
    dc->SetPen(*wxGREY_PEN);
    while (1)
    {
        x = Clock2x(BarInfo.Clock);
        if (x > CanvasX + CanvasW)
            break;
        if (x >= xEvents)   // so ne Art clipping
        {
            // SN+-      if ((BarInfo.BarNr % 4) == 0)
            int c;
            if (ClocksPerPixel > 48) c = 8; else c = 4;
            if (((BarInfo.BarNr - intro + 96) % c) == 0)
            {
                dc->SetPen(*wxBLACK_PEN);
                sprintf(buf, "%d", BarInfo.BarNr + 1 - intro);
                dc->DrawText(buf, x + LittleBit, yEvents - hLine);
                dc->SetPen(*wxGREY_PEN);
                dc->DrawLine(x, yEvents + 1 - hLine, x, yEvents + hEvents);
            }
            else
            {
                dc->SetPen(*wxLIGHT_GREY_PEN);
                dc->DrawLine(x, yEvents+1, x, yEvents+hEvents);
            }

            if (nBars < MaxBars)      // x-Koordinate fuer MouseAction->Snap()
                xBars[nBars++] = x;

        }
        BarInfo.Next();
    }
    dc->SetPen(*wxBLACK_PEN);

    // for each track show num, name, state, prg

    dc->SetClippingRegion(CanvasX, yEvents, CanvasX+CanvasW, yEvents + hEvents);
    int TrackNr = FromLine;
    for (y = Line2y(TrackNr); y < yEvents + hEvents; y += hLine)
    {
        // SN+    dc->HLine(y);
        dc->SetPen(*wxGREY_PEN);
        dc->DrawLine(xEvents+1,y,CanvasX + CanvasW, y);
        dc->SetPen(*wxBLACK_PEN);
        dc->DrawLine(CanvasX, y, xEvents,y);
        //
        tTrack *Track = Song->GetTrack(TrackNr);
        if (Track)
        {
            // TrackName, show the button pressed when dialog is open
            //dc->DrawText(Track->GetName(), xName + LittleBit, y + LittleBit);
            if (Track->DialogBox)
                LineText(dc,xName, y, wName, Track->GetName(), -1, TRUE);
            else
                LineText(dc,xName, y, wName, Track->GetName(), -1, FALSE);

            // TrackStatus
            //dc->DrawText(Track->GetStateChar(), xState + LittleBit, y + LittleBit);
            LineText(dc,xState, y, wState, Track->GetStateChar());
        }
        ++ TrackNr;
    }
    dc->DestroyClippingRegion();

    DrawNumbers(dc);
    DrawSpeed(dc);
    DrawCounters(dc);
    LineText(dc, xState, CanvasY-1, wState, "", hTop);

    DrawEvents(dc);

    if (Marked.x > 0)
        LineText(dc, (long)Marked.x, (long)Marked.y, (long)Marked.width, ">");
    dc->DestroyClippingRegion();
    DrawPlayPosition(dc);
    SnapSel->Draw(dc, xEvents, yEvents, wEvents, hEvents);//draw the selection box
    dc->EndDrawing();
}

void tTrackWin::DrawEvents(wxDC* dc){
  // draw events
  tBarInfo BarInfo(Song);
  dc->SetClippingRegion(xEvents, yEvents, wEvents, hEvents);
  int TrackNr = FromLine;
  for (int y = Line2y(TrackNr); y < yEvents + hEvents; y += hLine)
  {
    tTrack *Track = Song->GetTrack(TrackNr);
    if (Track)
    {
      tEventIterator Iterator(Track);
      long StopClk = x2Clock(CanvasX + CanvasW);
      tEvent *e = Iterator.Range(FromClock, StopClk);
      int y0 = y + LittleBit;
      int y1 = y + hLine - LittleBit;

      if (UseColors)
      {
#if 0
	while (e)       // slow!
	{
	  float x = Clock2x(e->Clock);
	  dc->SetPen(e->GetPen());
	  dc->DrawLine(x, y0, x, y1);
	  e = Iterator.Next();
	}
#else
	int xdone = -1;
	int h = y1 - y0;
	while (e)       // very slow!
	{
	  int x1 = Clock2x(e->Clock + e->GetLength());
	  if (x1 > xdone) {
	    int x0 = Clock2x(e->Clock);
	    if (x0 < xdone)
	      x0 = xdone;
	    int w = x1 - x0;
	    if (w < 2)
	      w = 2;
	    xdone = x0 + w;
	    dc->SetPen(*e->GetPen());
	    dc->SetBrush(*e->GetBrush());
	    dc->DrawRectangle(x0, y0, w, h);
	  }
	  e = Iterator.Next();
	}
#endif
	dc->SetPen(*wxBLACK_PEN);
      }
      else
      {
	float xblack = -1.0;
	while (e)
	{
	  int x = Clock2x(e->Clock);

	  // Avoid painting events ON the bar
	  if ( !(e->Clock % BarInfo.TicksPerBar) ) x = x + 1;

	  if (x > xblack)
	  {
	    dc->DrawLine(x, y0, x, y1);
#ifndef SLOW_MACHINE
	    xblack = x;
#else
	    xblack = x + 4;
#endif
	  }
	  e = Iterator.Next();
	}
      }
    }


    ++ TrackNr;
  }
}

// ************************************************************************
// Utilities
// ************************************************************************

long tTrackWin::x2xBar(long x)
{
    for (int i = 1; i < nBars; i++)
        if (x < xBars[i])
            return xBars[i - 1];
    return -1;
}


long tTrackWin::x2wBar(long x)
{
    for (int i = 1; i < nBars; i++)
        if (x < xBars[i])
            return xBars[i] - xBars[i - 1];
    return 0;
}




tTrack *tTrackWin::y2Track(long y)
{
    return Song->GetTrack(y2Line(y));
}



void tTrackWin::Mark(long x, long y)
{
    Marked.x = x2xBar(x);
    Marked.y = y2yLine(y);
    Marked.width = x2wBar(x);
    Marked.height = hLine;
    wxDC* dc=new wxClientDC(this);
    LineText(dc,(int)Marked.x, (int)Marked.y, (int)Marked.width, ">");
}

void tTrackWin::UnMark()
{
    Marked.x = -1;
}


// ********************************************************************
// Snapper
// ********************************************************************

void tTrackWin::SnapSelStart(wxMouseEvent &e)
{
    SnapSel->SetXSnap(nBars, xBars);
    SnapSel->SetYSnap(Line2y(FromLine), yEvents + hEvents, hLine);
}


void tTrackWin::SnapSelStop(wxMouseEvent &e)
{
    if (SnapSel->Selected)
    {
        Filter->FromTrack = y2Line((long)SnapSel->r.y);
        Filter->ToTrack   = y2Line((long)(SnapSel->r.y + SnapSel->r.height - 1));
        Filter->FromClock = x2BarClock((long)SnapSel->r.x + 1);
        Filter->ToClock   = x2BarClock((long)(SnapSel->r.x + SnapSel->r.width + 1));
        NextWin->NewPosition(Filter->FromTrack, Filter->FromClock);
    }
}

// --------------------------------------------------------------------
// Tracknummer
// --------------------------------------------------------------------

/**
   handle clicks in the "numbers" column. normally means select the entire track
*/
void tTrackWin::MouseNumber(wxMouseEvent &e)
{


  if (e.LeftDown())
  {
    int x, y;
    e.GetPosition(&x, &y);
    tTrack *t = y2Track((long)y);
    if (t != 0)
    {
      tRect r;
      r.x = 0;
      r.y = y2yLine((long)y);
      r.width = Clock2x( Song->MaxQuarters * Song->TicksPerQuarter );
      r.height = hLine;
      SnapSel->Select(r, xEvents, yEvents, wEvents, hEvents);
      SnapSelStop(e);
    }
  }
  Canvas->Refresh();

}

// --------------------------------------------------------------------
// PatchCounter
// --------------------------------------------------------------------

class tPatchCounter : public tMouseCounter
{
    tTrackWin *tw;
  public:
    int Event(wxMouseEvent &e);
    tPatchCounter(tTrackWin *t, tRect *r, int v, int min, int max)
      : tMouseCounter(t, r, v, min, max)
    {
      tw = t;
    }
};

int tPatchCounter::Event(wxMouseEvent &e)
{
  wxDC* dc = new wxClientDC(tw);
  if (tMouseCounter::Event(e))
  {
    tTrack *t = tw->y2Track((long)r.y);
    if (t)
    {
      switch (tw->CounterMode)
      {
	case CmProgram : t->SetBank( t->GetBank() );
                         t->SetPatch(Value);
                         tw->DrawCounters(dc);
                         break;
	case CmBank    : t->SetBank(Value);
                         t->SetPatch( t->GetPatch() );
                         tw->DrawCounters(dc);
                         break;
#ifndef __PORTING
	case CmVolume  : t->SetVolume(Value);
			 tMixerDlg::SetSliderVal( t->Channel - 1, MxVol, Value );
			 break;
	case CmPan     : t->SetPan(Value);
			 tMixerDlg::SetSliderVal( t->Channel - 1, MxPan, Value );
			 break;
	case CmReverb  : t->SetReverb(Value);
			 tMixerDlg::SetSliderVal( t->Channel - 1, MxRev, Value );
			 break;
	case CmChorus  : t->SetChorus(Value);
			 tMixerDlg::SetSliderVal( t->Channel - 1, MxCho, Value );
			 break;
#endif // __PORTING
	default: break;
      }
    }
    tw->MouseAction = 0;
    delete dc;
    delete this;
  }
  return 0;
}


/**
   handle clicks in the "patch" colum
*/
void tTrackWin::MousePatch(wxMouseEvent &e)
{

  cout<<"tTrackWin::MousePatch"<<endl;
  int x, y;

  if (!e.LeftDown() && !e.RightDown())
    return;

  e.GetPosition(&x, &y);
  tTrack *t = y2Track((long)y);
  if (t)
  {
    tRect r;
    int Value;
    switch (CounterMode)
    {
      case CmProgram: Value = t->GetPatch(); break;
      case CmBank   : Value = t->GetBank(); break;
      case CmVolume : Value = t->GetVolume(); break;
      case CmPan    : Value = t->GetPan(); break;
      case CmReverb : Value = t->GetReverb(); break;
      case CmChorus : Value = t->GetChorus(); break;
      default       : Value = 0; break;
    }
    r.x = xPatch;
    r.y = y2yLine((long)y);
    r.width = wPatch;
    r.height = 0;

    tPatchCounter *PatchCounter;

    if ((CounterMode == CmBank) && Config(C_UseTwoCommandBankSelect))
    {
       PatchCounter = new tPatchCounter(this, &r, Value, 0, Config(C_MaxBankTableEntries) - 1);
    }
    else
    {
       PatchCounter = new tPatchCounter(this, &r, Value, 0, 128);
    }

    PatchCounter->Event(e);
    MouseAction = PatchCounter;
  }

  Canvas->Refresh();
}

// --------------------------------------------------------------------
// SpeedCounter
// --------------------------------------------------------------------

class tSpeedCounter : public tMouseCounter
{
    tTrackWin *tw;
  public:
    int Event(wxMouseEvent &e);
    tSpeedCounter(tTrackWin *t, tRect *r, int v, int min, int max)
      : tMouseCounter(t, r, v, min, max)
    {
      tw = t;
    }
    virtual void ShowValue(bool down);
};

int tSpeedCounter::Event(wxMouseEvent &e)
{
  if (tMouseCounter::Event(e))
  {
    tTrack *t = tw->Song->GetTrack(0);
    t->SetDefaultSpeed(Value);
    for (int i = 0; i < tw->Song->nTracks; i++)
      Midi->AdjustAudioLength(tw->Song->GetTrack(i));
    tw->NextWin->Redraw();
    tw->MouseAction = 0;
    delete this;
  }
  return 0;
}

void tSpeedCounter::ShowValue(bool down)
{
  wxDC* dc=new wxClientDC(tw);
  tw->DrawSpeed(dc,Value, down);
  delete dc;
}

void tTrackWin::MouseSpeed(wxMouseEvent &e)
{
  tRect r;
  int Value;

  if (!e.LeftDown() && !e.RightDown())
    return;

  if (Midi->GetAudioEnabled() && Midi->Playing)
    return;

  Value = Song->GetTrack(0)->GetDefaultSpeed();
  tSpeedCounter *SpeedCounter = new tSpeedCounter(this, &r, Value, 20, 250);
  SpeedCounter->Event(e);
  MouseAction = SpeedCounter;

  Canvas->Refresh(); //JAVE cause redrawing of the window(might be improved to send the actual invalidated region.)
}

// -----------------------------------------------------------------------
// Track-Status
// -----------------------------------------------------------------------

/**
 handle clicks in the "state" column
 */
void tTrackWin::MouseState(wxMouseEvent &e)
{
  cout<<"tTrackWin::MouseState"<<endl;

  if (e.LeftDown() || e.RightDown())
  {
    int x, y;
    e.GetPosition(&x, &y);
    tTrack *t = y2Track((long)y);
    if (t)
    {
      const char *down = t->GetStateChar();
      t->ToggleState(e.LeftDown() ? 1 : -1); // toggle
      const char *up   = t->GetStateChar();
//       wxDC* dc=new wxClientDC(this);
//       LineText(dc, xState, (long)y, wState, t->GetStateChar());
//       delete dc;
      tRect r;
      r.x = xState;
      r.y = y2yLine((long)y);
      r.width = wState;
      r.height = 0;
      MouseAction = new tMouseButton(this, &r, up, up);
      Canvas->Refresh(); //JAVE cause redrawing of the window(might be improved to send the actual invalidated region.)
    }
  }

}

// --------------------------------------------------------------------------
// Name
// --------------------------------------------------------------------------

class tTrackNameButton : public tMouseButton
{
  public:
    tTrackNameButton(tTrackWin *win, tTrack *trk, tRect *r, const char *name)
    : tMouseButton(win, r, name, name), tw(win), track(trk) {}
    virtual void Action() {
      track->Dialog(tw);
    }
  private:
    tTrackWin *tw;
    tTrack    *track;
};

/**
    handle clicks in the "name" column

    this does a lot of drawing, and should be remade to use invalidation
*/
void tTrackWin::MouseName(wxMouseEvent &e)
{

  cout<<"tTrackWin::MouseName"<<endl;
  static int from=0,to=0;
  static int  state = 0;
  static long last  = 0;
  long i;
  int x,y;
  wxDC* dc=new wxClientDC(Canvas);
  if (e.LeftDown())
  {
    e.GetPosition(&x, &y);
    tTrack *t = y2Track((long)y);
    if (t && !MixerForm) {
      //t->Dialog(this);
      tRect r;
      r.x = xName;
      r.y = y2yLine((long)y);
      r.width = wName;
      r.height = 0;
      MouseAction = new tTrackNameButton(this, t, &r, t->GetName());
    }
    else {
      wxMessageBox("Quit parts dialog first.", "Info", wxOK);
    }
  }

  // SN++
  if (e.RightDown() && !state) {

      e.GetPosition(&x, &y);
      tTrack *t = y2Track((long)y);
      if (t && !MixerForm) {
        dc->SetBrush(*wxTRANSPARENT_BRUSH);
	    dc->DrawRectangle(xName+1,y2yLine((long)y),wName-2,hLine-1);
	    dc->SetBrush(*wxWHITE_BRUSH);
	    if(!state) {
               from = y2Line((long)y);
	       state=1;
	       last = y2yLine((long)y);
	    }
      }
   }
   if (e.RightUp() && state) {
         state = 0;
         e.GetPosition(&x, &y);
         tTrack *t = y2Track((long)y);
         if (t && !MixerForm) {
           to = y2Line((long)y);
           Song->moveTrack(from,to);
	 }
	 Redraw();
   }
   if (e.Dragging() && state) {
      e.GetPosition(&x, &y);
      i = y2yLine((long)y);
      if (i!=last) {
        dc->SetBrush(*wxTRANSPARENT_BRUSH);
        dc->SetLogicalFunction(wxXOR);
        dc->SetPen(*wxGREY_PEN);
        dc->DrawRectangle(xName, last  ,wName-1, hLine-1);
        dc->DrawRectangle(xName, last+1,wName-2, hLine-3);
        dc->DrawRectangle(xName, i     ,wName-1, hLine-1);
        dc->DrawRectangle(xName, i+1   ,wName-2, hLine-3);
        dc->SetLogicalFunction(wxCOPY);
	dc->SetPen(*wxBLACK_PEN);
        dc->SetBrush(*wxWHITE_BRUSH);
        last = i;
      }
   }

   //Canvas->Refresh();
   delete dc;
}

// --------------------------------------------------------------------------
// Event-Bereich
// --------------------------------------------------------------------------

void tTrackWin::MouseEvents(wxMouseEvent &e)
{
  //  cout <<"tTrackWin::MouseEvents"<<endl;
  if (e.RightDown())
  {

    int x, y;
    e.GetPosition(&x, &y);
    int TrackNr = y2Line((long)y);
    long Clock  = x2BarClock((long)x);
    NextWin->NewPosition(TrackNr, Clock);
    NextWin->Show(TRUE);
  }
  else
    tEventWin::OnMouseEvent(e);
}

// --------------------------------------------------------------------------
// Playbar
// --------------------------------------------------------------------------

/** handle clicks in the play bar
 * not playing:
 *  events selected:
 *    left : start rec/play
 *    right: mute + start rec/play
 *  no events selected:
 *    left+right: start play
 * playing:
 *  left+right: stop
 */

void tTrackWin::MousePlay(wxMouseEvent *e, MousePlayMode mode)
{
        cout<<"tTrackWin::MousePlay"<<endl;
#ifndef __PORTING
  /// \todo
  /* { does this ever get deleted? Do todo lists in doxygen work? }
  */
  wxDC* dc=new wxClientDC(Canvas);
#endif // __PORTING

    if (mode == Mouse && !e->ButtonDown())
        return;

    // This is a little hack to keep it working for now.  All this stuff needs to be moved.
    tRecordInfo* RecInfo = gProject->GetRecInfo();

    if (!gProject->IsPlaying())
    {
        switch (mode) {
            case Mouse:
                int x, y;
                e->GetPosition(&x, &y);
                gProject->SetPlayPosition(x2BarClock((long)x));
                gProject->Mute((e->RightDown() != 0));
                if (SnapSel->Selected && (e->ShiftDown() || e->MiddleDown()))
                    gProject->SetLoop(TRUE);
                else
                    gProject->SetLoop(FALSE);
                    prev_record = SnapSel->Selected;
                break;

            case SpaceBar:
                // do it again, sam
                break;

            case PlayButton:
            	cout << "tTrackwin::PlayButton" << endl;
                gProject->SetLoop(FALSE);
                gProject->SetRecord(FALSE);
                break;

            case PlayLoopButton:
                if (!EventsSelected("please select loop range first"))
                    return;
                gProject->SetLoop(TRUE);
                gProject->SetRecord(FALSE);
                break;

            case RecordButton:
                if (!EventsSelected("please select record track/bar first"))
                    return;
                tBarInfo bi(gProject);

                bi.SetClock(Filter->FromClock);

                if (bi.BarNr > 0)
                    bi.SetBar(bi.BarNr - 1);
                gProject->SetPlayPosition(bi.Clock);
                gProject->SetRecord(TRUE);
                gProject->SetLoop(FALSE);
                break;
        }

        // todo: figure out if we should have getters for these instead and make them private
        // jppProject members
        bool loop   = gProject->mLoop;
        bool muted  = gProject->mMuted;
        bool record = gProject->mRecord;

        // possible to record?


        if (record && SnapSel->Selected)
        {
            RecInfo->TrackNr   = Filter->FromTrack;

            RecInfo->Track     = gProject->GetTrack(RecInfo->TrackNr);

            RecInfo->FromClock = Filter->FromClock;
            RecInfo->ToClock   = Filter->ToClock;

            if (muted)
            {
                RecInfo->Muted = 1;
                RecInfo->Track->SetState(tsMute);
            #ifndef __PORTING
                LineText(dc,xState, Line2y(RecInfo.TrackNr), wState, RecInfo.Track->GetStateChar());
            #endif // __PORTING
            }
            else
                RecInfo->Muted = 0;
        }
        else
            RecInfo->Track = 0;

        // possible to loop?
        long loop_clock = 0;
        if (loop && SnapSel->Selected)
        {
            prev_clock = Filter->FromClock;
            loop_clock = Filter->ToClock;
        }

        // GO!
		cout << "Go!" << endl;
        //if (RecInfo->Track)  // recording?
            //gProject->Midi->SetRecordInfo(RecInfo);
        //else
            //gProject->Midi->SetRecordInfo(0);
        cout<<"Midi->StartPlay"<<endl;

        gProject->mStartTime = prev_clock;
        gProject->mStopTime = loop_clock;
        gProject->Play();

    }//if(!Midi->Playing)
    else
    {
        gProject->Stop();
        if (RecInfo->Track)
        {
            if (RecInfo->Muted)
            {
                RecInfo->Track->SetState(tsPlay);
            #ifndef __PORTING
                LineText(dc,xState, Line2y(RecInfo->TrackNr), wState, RecInfo->Track->GetStateChar());
            #endif // __PORTING
            }
            if (!RecInfo->Track->GetAudioMode() && !gProject->Midi->RecdBuffer.IsEmpty())
            {
                //int choice = wxMessageBox("Keep recorded events?", "You played", wxOK | wxCANCEL);
                //if (choice == wxOK)
                {
                    wxBeginBusyCursor();
                    gProject->NewUndoBuffer();
                    RecInfo->Track->MergeRange(&gProject->Midi->RecdBuffer, RecInfo->FromClock, RecInfo->ToClock, RecInfo->Muted);
                    wxEndBusyCursor();
                    Redraw();
                    NextWin->Redraw();
                }
            }
        }
    }
}

int tTrackWin::OnKeyEvent(wxKeyEvent &e)

{
  if (e.ControlDown()) {
    switch (e.KeyCode()) {
      case 'Z':
        OnMenuCommand(MEN_UNDO);
        return 1;
      case 'Y':
        OnMenuCommand(MEN_REDO);
        return 1;
      case 'X':
        OnMenuCommand(MEN_CLP_CUT);
        return 1;
      case 'V':
        OnMenuCommand(MEN_CLP_PASTE);
        return 1;
      case 'Q':
        OnMenuCommand(MEN_QUIT);
        return 1;
      case 'C':
      case WXK_INSERT:
        OnMenuCommand(MEN_CLP_COPY);
        return 1;
    }
  }

  else if (e.ShiftDown()) {
    switch (e.KeyCode()) {
      case WXK_INSERT:
        OnMenuCommand(MEN_CLP_PASTE);
        return 1;
      case WXK_DELETE:
        OnMenuCommand(MEN_CLP_CUT);
        return 1;
    }
  }

  else {
    switch (e.KeyCode()) {

      case ' ':
        MousePlay(0, SpaceBar);
        return 1;

      case WXK_DELETE:
        OnMenuCommand(MEN_DELETE);
        return 1;
    }
  }

  return 0;
}

/**
 event handler for mouse events. this in turn dispatches to different handlers, depending on mode and so on.

in wx1.68 it was enough to just override this funtcion, in wx2 you need an event macro also, it seems

it is called from the "canvas" event table

*/

int tTrackWin::OnMouseEvent(wxMouseEvent &e)
{
  long x, y;
  e.GetPosition(&x, &y);

  //  cout<<"tTrackWin::OnMouseEvent x y:number name state patch events "<<x<<" "<<y<<" "<<xNumber<<" "<<xName<<" "<<xState<<" "<<xPatch<<" "<<xEvents<<" "<<endl;
  if (!MouseAction)
  {

    // determine in which area we clicked, and call the apropriate handler

    if (y > yEvents) //did we click below the  "play bar"?
    {
      if (xNumber < x && x < (xNumber + wNumber)) //did we click on the leftmost "number" column?
	MouseNumber(e);
      else if (xName < x && x < (xName + wName)) //did we click on the name column
	MouseName(e);
      else if(xState < x && x < (xState + wState))//did we click on the state column
	MouseState(e);
      else if (xPatch < x && x < (xPatch + wPatch))//did we click on the patch column
	MousePatch(e);
      else if (xEvents < x && x < (xEvents + wEvents)) //did we click somewhere in the event field?
	MouseEvents(e);
      else
	tEventWin::OnMouseEvent(e); //otherwise call the base class implementation(i cant figure out when that is supposed to be)
    }
    else
    {
      // Playbar

      if (xNumber < x && x < xNumber + wNumber)
      {
	if (e.LeftDown())
	{
 	  NumberMode = (tNumberModes)(((int)NumberMode + 1) % (int)NmModes);
	  wxDC* dc=new wxClientDC(this);

	  DrawNumbers(dc);
	  tRect r;
	  r.x = xNumber;
	  r.y = CanvasY-1;
	  r.width = wNumber;
	  r.height = hTop;
	  MouseAction = new tMouseButton(this, &r, NumberStr());
	}
      }
      else if (xName < x && x < xName + wName)
	MouseSpeed(e);
      else if(xState < x && x < xState + wState)
	;
      else if (xPatch < x && x < xPatch + wPatch)
      {
        if (e.ButtonDown())
	{
	  if (e.LeftDown())
	    CounterMode = (tCounterModes)(((int)CounterMode + 1) % (int)CmModes);
	  else if (e.RightDown())
	    CounterMode = (tCounterModes)(((int)CounterMode + (int)CmModes - 1) % (int)CmModes);
	  wxDC* dc=new wxClientDC(this);

	  DrawCounters(dc);
	  tRect r;
	  r.x = xPatch;
	  r.y = CanvasY-1;
	  r.width = wPatch;
	  r.height = hTop;
	  MouseAction = new tMouseButton(this, &r, CounterStr());
	}
      }
      else if (xEvents < x && x < xEvents + wEvents)
	MousePlay(&e, Mouse);
      else
	tEventWin::OnMouseEvent(e);
    }
  }

  else
    tEventWin::OnMouseEvent(e);

  return 0;
}


void tTrackWin::ButtonLabelDisplay(wxString text, bool down) {
}
