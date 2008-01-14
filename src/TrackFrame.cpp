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

#include "Resources.h"

#include "TrackFrame.h"
#include "TrackWindow.h"
#include "JazzPlusPlusApplication.h"
#include "ToolBar.h"
#include "PianoFrame.h"
#include "AboutDialog.h"

// These are the tool bar icons.
#include "Bitmaps/open.xpm"
#include "Bitmaps/save.xpm"
#include "Bitmaps/new.xpm"
#include "Bitmaps/repl.xpm"
#include "Bitmaps/delete.xpm"
#include "Bitmaps/quantize.xpm"
#include "Bitmaps/mixer.xpm"
#include "Bitmaps/play.xpm"
#include "Bitmaps/undo.xpm"
#include "Bitmaps/redo.xpm"
#include "Bitmaps/zoomin.xpm"
#include "Bitmaps/zoomout.xpm"
#include "Bitmaps/panic.xpm"
#include "Bitmaps/help.xpm"
#include "Bitmaps/pianowin.xpm"
#include "Bitmaps/metro.xpm"
#include "Bitmaps/playloop.xpm"
#include "Bitmaps/record.xpm"

//*****************************************************************************
// Description:
//   This is the track frame class definition.
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(JZTrackFrame, wxFrame)

  EVT_MENU(ID_PIANOWIN, JZTrackFrame::OnPianoWindow)

//  EVT_MENU(wxID_EXIT, JZTrackFrame::OnFileExit)

  EVT_MENU(wxID_HELP_CONTENTS, JZTrackFrame::OnHelpContents)

  EVT_MENU(wxID_ABOUT, JZTrackFrame::OnHelpAbout)

END_EVENT_TABLE()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZTrackFrame::JZTrackFrame(
  wxWindow* pParent,
  const wxString& Title,
  const wxPoint& Position,
  const wxSize& Size)
  : wxFrame(
      pParent,
      wxID_ANY,
      Title,
      Position,
      Size,
      wxDEFAULT_FRAME_STYLE | wxNO_FULL_REPAINT_ON_RESIZE),
    mpToolBar(0),
    mpFileMenu(0),
    mpEditMenu(0)//,
//    mpPianoFrame(0)
{
  CreateToolBar();

  CreateMenu();

  mpTrackWindow = new JZTrackWindow(this, wxPoint(0, 0), wxSize(600, 120));
  mpTrackWindow->Create();

//  mpPianoFrame = new JZPianoFrame(
//    this,
//    "Piano",
//    wxDefaultPosition,
//    wxSize(640, 480));
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZTrackFrame::~JZTrackFrame()
{
  delete mpToolBar;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZTrackFrame::CreateToolBar()
{
  JZToolDef ToolBarDefinitions[] =
  {
    { wxID_OPEN, false, open_xpm, "load song" },
    { wxID_SAVE, false, save_xpm, "save song" },
    { wxID_NEW,  false, new_xpm,  "new song" },
    { JZToolBar::eToolBarSeparator },
    { wxID_DUPLICATE, false, repl_xpm,     "duplicate selection" },
    { wxID_DELETE,    false, delete_xpm,   "delete selection" },
    { ID_QUANTIZE,    false, quantize_xpm, "quantize selection" },
    { ID_MIXER,       false, mixer_xpm,    "mixer" },
    { ID_PIANOWIN,    false, pianowin_xpm, "show piano window" },
    { JZToolBar::eToolBarSeparator },
    { ID_PLAY,      false, play_xpm,     "start play"},
    { ID_PLAY_LOOP, false, playloop_xpm, "loop play"},
    { ID_RECORD,    false, record_xpm,   "record"},
    { ID_METRONOME, true,  metro_xpm,    "metronome" },
    { JZToolBar::eToolBarSeparator },
    { wxID_ZOOM_IN,       false, zoomin_xpm,  "zoom in" },
    { wxID_ZOOM_OUT,      false, zoomout_xpm, "zoom out"},
    { wxID_UNDO,          false, undo_xpm,    "undo"},
    { wxID_REDO,          false, redo_xpm,    "redo"},
    { wxID_RESET,         false, panic_xpm,   "all notes off"},
    { wxID_HELP_CONTENTS, false, help_xpm,    "help" },
    { wxID_ABOUT,         false, help_xpm,    "about" },
    { JZToolBar::eToolBarEnd }
  };

  mpToolBar = new JZToolBar(this, ToolBarDefinitions);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZTrackFrame::CreateMenu()
{
  // Create the file menu.
  mpFileMenu = new wxMenu;

  mpFileMenu->Append(wxID_NEW,  "&New");
  mpFileMenu->Append(wxID_OPEN, "&Open");
  mpFileMenu->Append(wxID_CLOSE, "&Close");
  mpFileMenu->Append(wxID_SAVE, "&Save Project");
  mpFileMenu->Append(wxID_SAVEAS, "Save Project &as...");

  mpFileMenu->AppendSeparator();

  mpFileMenu->Append(ID_EXPORT_MIDI, "Export as Midi...");
  mpFileMenu->Append(
    ID_EXPORT_SELECTION_AS_MIDI,
    "Export Selection as Midi...");

  mpFileMenu->AppendSeparator();

  mpFileMenu->Append(wxID_PREFERENCES, "&Preferences...");

  mpFileMenu->AppendSeparator();

  mpFileMenu->Append(wxID_EXIT, "E&xit\tAlt+F4");

  mpEditMenu = new wxMenu;

  mpEditMenu->Append(wxID_UNDO, "&Undo...");
  mpEditMenu->Append(wxID_REDO, "&Redo...");

  mpEditMenu->AppendSeparator();

  mpEditMenu->Append(wxID_CUT, "&Cut");
  mpEditMenu->Append(wxID_COPY, "C&opy");
  mpEditMenu->Append(wxID_PASTE, "&Paste");
  mpEditMenu->Append(ID_TRIM, "&Trim");

  mpEditMenu->AppendSeparator();

  mpEditMenu->Append(wxID_DELETE, "&Delete");
  mpEditMenu->Append(wxID_DELETE, "&Silence");

  mpEditMenu->AppendSeparator();

#if 0
  mpEditMenu->Append(MEN_SPLIT, "Split");
  mpEditMenu->Append(MEN_REPLICATE, "&Duplicate");

  mpEditMenu->AppendSeparator();

  mpEditMenu->Append(MEN_SELECTIONSUB, "Select (needs submenu)");

  mpEditMenu->AppendSeparator();

  /* Move Elsewhere
  mpEditMenu->Append(MEN_QUANTIZE,      "&Quantize ...");
  mpEditMenu->Append(MEN_SETCHAN,       "&Set MIDI Channel ...");
  mpEditMenu->Append(MEN_TRANSP,        "&Transpose ...");
  mpEditMenu->Append(MEN_VELOC,         "&Velocity ...");
  mpEditMenu->Append(MEN_LENGTH,        "&Length ...");
  mpEditMenu->Append(MEN_SHIFT,         "Shi&ft ...");
  mpEditMenu->Append(MEN_CLEANUP,       "C&leanup ...");
  mpEditMenu->Append(MEN_SEARCHREP,     "Search Re&place ...");
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
  mpFileMenu->Append(MEN_LOAD_TMPL,     "Load &Template...");
  mpFileMenu->Append(MEN_LOADPATTERN,   "Load Pattern...");
  mpFileMenu->Append(MEN_SAVEPATTERN,   "Save Pattern...");
  mpFileMenu->AppendSeparator();

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

  #ifdef WX_MSW
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
#endif

  wxMenu* mpHelpMenu = new wxMenu;
//  mpHelpMenu->Append(MEN_HELP_JAZZ, "&Jazz");
//  mpHelpMenu->Append(MEN_HELP_TWIN, "&Trackwin");
//  mpHelpMenu->Append(MEN_HELP_MOUSE, "&Mouse");
  mpHelpMenu->Append(wxID_ABOUT, "&About");

  // Create a menu bar and add entries.
  wxMenuBar* pMenuBar = new wxMenuBar();
  pMenuBar->Append(mpFileMenu, "&File");
  pMenuBar->Append(mpEditMenu, "&Edit");
#if 0
  pMenuBar->Append(misc_menu, "&View");
  pMenuBar->Append(parts_menu, "&Parts");
  pMenuBar->Append(setting_menu, "&Settings");

  audio_menu = new wxMenu();
  audio_menu->Append(MEN_AUDIO_GLOBAL, "&Global Settings ...");
  audio_menu->Append(MEN_AUDIO_SAMPLES, "Sample Se&ttings ... ");
  audio_menu->Append(MEN_AUDIO_LOAD, "&Load Set ...");
  audio_menu->Append(MEN_AUDIO_SAVE, "&Save Set");
  audio_menu->Append(MEN_AUDIO_SAVE_AS, "Save Set &As");
  audio_menu->Append(MEN_AUDIO_NEW, "&New Set");
  pMenuBar->Append(audio_menu, "&Audio");
#endif

  pMenuBar->Append(mpHelpMenu , "&Help");

  SetMenuBar(pMenuBar);

//  EnableDisableMenus();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZTrackFrame::OnPianoWindow(wxCommandEvent& Event)
{
  JZPianoFrame* pPianoFrame = new JZPianoFrame(
    this,
    "Piano",
    wxDefaultPosition,
    wxSize(640, 480));

  pPianoFrame->Show(true);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZTrackFrame::OnHelpContents(wxCommandEvent& Event)
{
  GetJazzApplication().DisplayHelpContents();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZTrackFrame::OnHelpAbout(wxCommandEvent& Event)
{
  JZAboutDialog AboutDialog(this);
  AboutDialog.ShowModal();
}
