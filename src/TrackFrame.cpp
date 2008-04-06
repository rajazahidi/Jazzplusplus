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
#include "Player.h"
#include "RecordingInfo.h"
#include "JazzPlusPlusApplication.h"
#include "ToolBar.h"
#include "PianoFrame.h"
#include "Project.h"
#include "Globals.h"
#include "Configuration.h"
#include "Harmony.h"
#include "SynthesizerSettingsDialog.h"
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

#include <iostream>

using namespace std;

//*****************************************************************************
// Description:
//   This is the track frame class definition.
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(JZTrackFrame, wxFrame)

  EVT_MENU(wxID_OPEN, JZTrackFrame::OnFileOpen)

  EVT_MENU(wxID_SAVEAS, JZTrackFrame::OnFileSaveAs)

  EVT_MENU(wxID_EXIT, JZTrackFrame::OnFileExit)

  EVT_MENU(ID_PLAY, JZTrackFrame::OnPlay)

  EVT_MENU(ID_PLAY_LOOP, JZTrackFrame::OnPlayLoop)

  EVT_MENU(ID_PIANOWIN, JZTrackFrame::OnPianoWindow)

  EVT_MENU(ID_METRONOME_TOGGLE, JZTrackFrame::OnMetroOn)

  EVT_MENU(wxID_ZOOM_IN, JZTrackFrame::OnZoomIn)

  EVT_MENU(wxID_ZOOM_OUT, JZTrackFrame::OnZoomOut)

  EVT_MENU(ID_TOOLS_HARMONY_BROWSER, JZTrackFrame::OnToolsHarmonyBrowser)

  EVT_MENU(ID_SETTINGS_SYNTH, JZTrackFrame::OnSettingsSynthesizerType)

  EVT_MENU(wxID_HELP_CONTENTS, JZTrackFrame::OnHelpContents)

  EVT_MENU(wxID_ABOUT, JZTrackFrame::OnHelpAbout)

END_EVENT_TABLE()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZTrackFrame::JZTrackFrame(
  wxWindow* pParent,
  const wxString& Title,
  JZSong* pSong,
  const wxPoint& Position,
  const wxSize& Size)
  : wxFrame(pParent, wxID_ANY, Title, Position, Size),
    mpToolBar(0),
    mpFileMenu(0),
    mpEditMenu(0),
    mpToolsMenu(0),
//    mpPianoFrame(0),
    mPreviousClock(0),
    mPreviouslyRecording(false)
{
  CreateToolBar();

  CreateMenu();

  mpTrackWindow = new JZTrackWindow(
    this,
    pSong,
    wxPoint(0, 0),
    wxSize(600, 120));

  gpTrackWindow = mpTrackWindow;

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
    { ID_PLAY,             false, play_xpm,     "start play"},
    { ID_PLAY_LOOP,        false, playloop_xpm, "loop play"},
    { ID_RECORD,           false, record_xpm,   "record"},
    { ID_METRONOME_TOGGLE, true, metro_xpm,    "metronome" },
    { JZToolBar::eToolBarSeparator },
    { wxID_ZOOM_IN,       false, zoomin_xpm,  "zoom in" },
    { wxID_ZOOM_OUT,      false, zoomout_xpm, "zoom out"},
    { wxID_UNDO,          false, undo_xpm,    "undo"},
    { wxID_REDO,          false, redo_xpm,    "redo"},
    { wxID_RESET,         false, panic_xpm,   "all notes off"},
    { wxID_HELP_CONTENTS, false, help_xpm,    "help" },
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
  mpFileMenu->Append(wxID_OPEN, "&Open...");
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

#if 0
  mpEditMenu->AppendSeparator();

  mpEditMenu->Append(MEN_SPLIT, "Split");
  mpEditMenu->Append(MEN_REPLICATE, "&Duplicate");

  mpEditMenu->AppendSeparator();

  mpEditMenu->Append(MEN_SELECTIONSUB, "Select (needs submenu)");

  mpEditMenu->AppendSeparator();

  /* Move Elsewhere
  mpEditMenu->Append(MEN_QUANTIZE,      "&Quantize...");
  mpEditMenu->Append(MEN_SETCHAN,       "&Set MIDI Channel...");
  mpEditMenu->Append(MEN_TRANSP,        "&Transpose...");
  mpEditMenu->Append(MEN_VELOC,         "&Velocity...");
  mpEditMenu->Append(MEN_LENGTH,        "&Length...");
  mpEditMenu->Append(MEN_SHIFT,         "Shi&ft...");
  mpEditMenu->Append(MEN_CLEANUP,       "C&leanup...");
  mpEditMenu->Append(MEN_SEARCHREP,     "Search Re&place...");
  */

  // Miscellaneous Menu is Stupid.
  // Now it's a View Menu
  misc_menu = new wxMenu;
  misc_menu->Append(MEN_TMERGE,   "Mer&ge Tracks...");
  misc_menu->Append(MEN_TSPLIT,   "&Split Tracks...");
  misc_menu->Append(MEN_METERCH,  "&Meterchange...");
  misc_menu->Append(MEN_RESET,    "&Reset Midi");
  misc_menu->Append(MEN_HARMONY,  "&Harmony Browser...");
  misc_menu->Append(MEN_RHYTHM,   "Random R&hythm...");
  misc_menu->Append(MEN_SHUFFLE,  "Random Sh&uffle...");
  misc_menu->Append(MEN_GENMELDY, "Random Melod&y...");
  misc_menu->Append(MEN_ARPEGGIO, "Random Arpeggio...");
  misc_menu->Append(MEN_MAPPER,   "Ma&pper...");
  misc_menu->Append(MEN_EVENTLIST, "Event &List...");
  misc_menu->Append(MEN_COPYRIGHT,"&Set Music Copyright...");
#endif

  mpToolsMenu = new wxMenu;
  mpToolsMenu->Append(ID_TOOLS_HARMONY_BROWSER,  "&Harmony Browser...");

#if 0
  // Move to Project Menu
  mpFileMenu->Append(MEN_LOAD_TMPL,     "Load &Template...");
  mpFileMenu->Append(MEN_LOADPATTERN,   "Load Pattern...");
  mpFileMenu->Append(MEN_SAVEPATTERN,   "Save Pattern...");
  mpFileMenu->AppendSeparator();

  parts_menu = new wxMenu;
  parts_menu->Append(MEN_MIXER,    "&Mixer...");
  parts_menu->Append(MEN_MASTER,   "Mas&ter...");
  parts_menu->Append(MEN_SOUND,    "&Sound...");
  parts_menu->Append(MEN_VIBRATO,  "&Vibrato...");
  parts_menu->Append(MEN_ENVELOPE, "&Envelope...");

  bender_menu = new wxMenu;
  bender_menu->Append(MEN_BEND_BASIC,    "&Bender Basic...");
  bender_menu->Append(MEN_BEND_LFO1,    "&Bender LFO1...");
  bender_menu->Append(MEN_BEND_LFO2,    "&Bender LFO2...");
  parts_menu->Append(MEN_SUB_BENDER, "&Bender...", bender_menu );

  modulation_menu = new wxMenu;
  modulation_menu->Append(MEN_MOD_BASIC,    "&Modulation Basic...");
  modulation_menu->Append(MEN_MOD_LFO1,    "&Modulation LFO1...");
  modulation_menu->Append(MEN_MOD_LFO2,    "&Modulation LFO2...");
  parts_menu->Append(MEN_SUB_MODUL, "&Modulation...", modulation_menu );

  caf_menu = new wxMenu;
  caf_menu->Append(MEN_CAF_BASIC,    "&CAf Basic...");
  caf_menu->Append(MEN_CAF_LFO1,    "&CAf LFO1...");
  caf_menu->Append(MEN_CAF_LFO2,    "&CAf LFO2...");
  parts_menu->Append(MEN_SUB_CAF, "&CAf...", caf_menu );

  paf_menu = new wxMenu;
  paf_menu->Append(MEN_PAF_BASIC,    "&PAf Basic...");
  paf_menu->Append(MEN_PAF_LFO1,    "&PAf LFO1...");
  paf_menu->Append(MEN_PAF_LFO2,    "&PAf LFO2...");
  parts_menu->Append(MEN_SUB_PAF, "&PAf...", paf_menu );

  cc1_menu = new wxMenu;
  cc1_menu->Append(MEN_CC1_BASIC,    "&CC1 Basic...");
  cc1_menu->Append(MEN_CC1_LFO1,    "&CC1 LFO1...");
  cc1_menu->Append(MEN_CC1_LFO2,    "&CC1 LFO2...");
  parts_menu->Append(MEN_SUB_CC1, "&CC1...", cc1_menu );

  cc2_menu = new wxMenu;
  cc2_menu->Append(MEN_CC2_BASIC,    "&CC2 Basic...");
  cc2_menu->Append(MEN_CC2_LFO1,    "&CC2 LFO1...");
  cc2_menu->Append(MEN_CC2_LFO2,    "&CC2 LFO2...");
  parts_menu->Append(MEN_SUB_CC2, "&CC2...", cc2_menu );

  parts_menu->Append(MEN_DRUM_PARAM,    "&Drum Parameters...");
  parts_menu->Append(MEN_PART_RSRV,    "&Partial Reserve...");
  parts_menu->Append(MEN_PART_MODE,    "&Part Mode...");
#endif

  wxMenu* pSettingMenu = new wxMenu;
#if 0
  pSettingMenu->Append(MEN_FILTER,    "&Filter...");
  pSettingMenu->Append(MEN_TWSETTING, "&Window...");
  pSettingMenu->Append(MEN_SONG,      "&Song...");
  pSettingMenu->Append(MEN_METRONOME, "&Metronome...");
  pSettingMenu->Append(MEN_EFFECTS,   "&Effects...");
  pSettingMenu->Append(MEN_TIMING,    "&Timing...");
  pSettingMenu->Append(MEN_MIDI_THRU, "&Midi Thru...");
#endif
  pSettingMenu->Append(ID_SETTINGS_SYNTH, "&Synthesizer Type...");

#if 0
#ifdef __WXMSW__
  pSettingMenu->Append(MEN_DEVICE, "&Midi Device...");
#else
  if (
    gpConfig->GetValue(C_MidiDriver) == eMidiDriverOss ||
    gpConfig->GetValue(C_MidiDriver) == eMidiDriverAlsa)
  {
    pSettingMenu->Append(MEN_DEVICE, "&Midi Device...");
  }
#endif
  save_settings_menu = new wxMenu;
  save_settings_menu->Append( MEN_SAVE_THRU, "&Midi Thru" );
  save_settings_menu->Append( MEN_SAVE_TIM, "&Timing" );
  save_settings_menu->Append( MEN_SAVE_EFF, "&Effect Macros" );
  save_settings_menu->Append( MEN_SAVE_GEO, "&Window Geometry" );
  save_settings_menu->Append( MEN_SAVE_METRO, "&Metronome" );
  // save_settings_menu->Append( MEN_SAVE_SYNTH, "&Synth Type" );
  save_settings_menu->Append( MEN_SAVE_ALL, "&Save All" );
  pSettingMenu->Append(MEN_SAVE_SET, "&Save settings", save_settings_menu );
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
  pMenuBar->Append(mpToolsMenu, "&Tools");

#if 0
  pMenuBar->Append(misc_menu, "&View");
  pMenuBar->Append(parts_menu, "&Parts");
#endif

  pMenuBar->Append(pSettingMenu, "&Settings");

#if 0
  audio_menu = new wxMenu;
  audio_menu->Append(MEN_AUDIO_GLOBAL, "&Global Settings...");
  audio_menu->Append(MEN_AUDIO_SAMPLES, "Sample Se&ttings... ");
  audio_menu->Append(MEN_AUDIO_LOAD, "&Load Set...");
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
bool JZTrackFrame::OnClose()
{
//  if (JZTrack::changed)
//  {
//    if (
//      ::wxMessageBox(
//        "Song has changed. Quit anyway?",
//        "Quit ?",
//        wxYES_NO) == wxNO)
//    {
//      return false;
//    }
//  }
//  if (gpProject->IsPlaying())
//  {
//    gpProject->Stop();
//#ifndef WX_MSW
//    sleep(1);
//#endif
//  }

  delete gpHarmonyBrowser;

  return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZTrackFrame::OnFileOpen(wxCommandEvent& Event)
{
  // Use an open dialog to find the Jazz++ configuration file.
  // wxFD_CHANGE_DIR - Change the current working directory to the directory
  // where the file(s) chosen by the user are.
  wxFileDialog OpenDialog(
    0,
    "Load MIDI File",
    "",
    "",
    "*.mid",
    wxFD_OPEN | wxFD_CHANGE_DIR);
  if (OpenDialog.ShowModal() == wxID_OK)
  {
    wxString FileName = OpenDialog.GetPath();
    gpProject->OpenSong(FileName);
    SetTitle(FileName);
//    NextWin->NewPosition(1, 0);
    mpTrackWindow->SetScrollRanges(0, 0);
//    mpTrackWindow->SetScrollPosition(0, 0);
//    NextWin->Canvas->SetScrollRanges();
    mpTrackWindow->Refresh(false);
//    JZTrack::changed = false;
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZTrackFrame::OnFileSaveAs(wxCommandEvent& Event)
{
  // wxFD_OVERWRITE_PROMPT - For save dialog only: prompt for a confirmation
  // if a file will be overwritten.
  wxFileDialog SaveAsDialog(
    0,
    "Save MIDI File",
    "",
    "",
    "*.mid",
    wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  if (SaveAsDialog.ShowModal() == wxID_OK)
  {
    wxString FileName = SaveAsDialog.GetPath();
    gpProject->Save(FileName);
    SetTitle(FileName);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZTrackFrame::OnFileExit(wxCommandEvent& Event)
{
  if (OnClose() == false)
  {
    return;
  }
  Close();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZTrackFrame::OnPlay(wxCommandEvent& Event)
{
  wxMouseEvent MouseEvent;
  MousePlay(MouseEvent, ePlayButton);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZTrackFrame::OnPlayLoop(wxCommandEvent& Event)
{
  wxMouseEvent MouseEvent;
  MousePlay(MouseEvent, ePlayLoopButton);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZTrackFrame::OnPianoWindow(wxCommandEvent& Event)
{
  JZPianoFrame* pPianoFrame = new JZPianoFrame(
    this,
    "Piano",
    gpSong,
    wxDefaultPosition,
    wxSize(640, 480));

  pPianoFrame->Show(true);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZTrackFrame::OnMetroOn(wxCommandEvent& Event)
{
  gpProject->ToggleMetronome();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZTrackFrame::OnZoomIn(wxCommandEvent& Event)
{
  mpTrackWindow->ZoomIn();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZTrackFrame::OnZoomOut(wxCommandEvent& Event)
{
  mpTrackWindow->ZoomOut();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZTrackFrame::OnToolsHarmonyBrowser(wxCommandEvent& Event)
{
  CreateHarmonyBrowser(this);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZTrackFrame::OnSettingsSynthesizerType(wxCommandEvent& Event)
{
  JZSynthesizerDialog SynthesizerDialog(this);
  SynthesizerDialog.ShowModal();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZTrackFrame::OnHelpContents(wxCommandEvent& Event)
{
  ::wxGetApp().DisplayHelpContents();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZTrackFrame::OnHelpAbout(wxCommandEvent& Event)
{
  JZAboutDialog AboutDialog(this);
  AboutDialog.ShowModal();
}

//-----------------------------------------------------------------------------
// Description:
//   Handle clicks in the play bar.
// not playing:
//   events selected:
//     left : start rec/play
//     right: mute + start rec/play
//   no events selected:
//     left+right: start play
// playing:
//   left+right: stop
//-----------------------------------------------------------------------------
void JZTrackFrame::MousePlay(wxMouseEvent& Event, TEMousePlayMode Mode)
{
  mpTrackWindow->MousePlay(Event, Mode);

/*
  if (Mode == eMouse && !Event.ButtonDown())
  {
    return;
  }

  // This is a little hack to keep it working for now.
  // All this stuff needs to be moved.
  JZRecordingInfo* pRecInfo = gpProject->GetRecInfo();

  if (!gpProject->IsPlaying())
  {
    switch (Mode)
    {
      case eMouse:
        int x, y;
        Event.GetPosition(&x, &y);
        gpProject->SetPlayPosition(x2BarClock((long)x));
        gpProject->Mute((Event.RightDown() != 0));
        if (mpSnapSel->Selected && (Event.ShiftDown() || Event.MiddleDown()))
        {
          gpProject->SetLoop(true);
        }
        else
        {
          gpProject->SetLoop(false);
          mPreviouslyRecording = mpSnapSel->Selected;
        }
        break;

      case eSpaceBar:
        break;

      case ePlayButton:
        gpProject->SetLoop(false);
        gpProject->SetRecord(false);
        break;

      case ePlayLoopButton:
        if (!EventsSelected("please select loop range first"))
        {
          return;
        }
        gpProject->SetLoop(true);
        gpProject->SetRecord(false);
        break;

      case eRecordButton:
        if (!EventsSelected("please select record track/bar first"))
        {
          return;
        }
        JZBarInfo bi(gpProject);

        bi.SetClock(mpFilter->FromClock);

        if (bi.BarNr > 0)
        {
          bi.SetBar(bi.BarNr - 1);
        }
        gpProject->SetPlayPosition(bi.Clock);
        gpProject->SetRecord(true);
        gpProject->SetLoop(false);
        break;
    }

    // todo: Figure out if we should have getters for these instead
    // and make them private jppProject members
    bool loop   = gpProject->mLoop;
    bool muted  = gpProject->mMuted;
    bool record = gpProject->mRecord;

    // Is it possible to record?
    if (record && mpSnapSel->Selected)
    {
      pRecInfo->mTrackIndex = mpFilter->FromTrack;

      pRecInfo->mpTrack = gpProject->GetTrack(pRecInfo->mTrackIndex);

      pRecInfo->mFromClock = mpFilter->FromClock;
      pRecInfo->mToClock   = mpFilter->ToClock;

      if (muted)
      {
        pRecInfo->mIsMuted = true;
        pRecInfo->mpTrack->SetState(tsMute);
#ifdef OBSOLETE
        LineText(
          *pDc,
          xState,
          Line2y(pRecInfo.mTrackIndex),
          wState,
          pRecInfo.Track->GetStateChar());
#endif
      }
      else
      {
        pRecInfo->mIsMuted = false;
      }
    }
    else
    {
      pRecInfo->mpTrack = 0;
    }

    // Is it possible to loop?
    int loop_clock = 0;
    if (loop && mpSnapSel->Selected)
    {
      mPreviousClock = mpFilter->FromClock;
      loop_clock = mpFilter->ToClock;
    }

    //if (pRecInfo->Track)  // recording?
      //gpProject->Midi->SetRecordInfo(pRecInfo);
    //else
      //gpProject->Midi->SetRecordInfo(0);

    gpProject->mStartTime = mPreviousClock;
    gpProject->mStopTime = loop_clock;
    gpProject->Play();

  } //if(!Midi->Playing)
  else
  {
    gpProject->Stop();
    if (pRecInfo->mpTrack)
    {
      if (pRecInfo->mIsMuted)
      {
//        wxDC* pDc = new wxClientDC(mpTrackWindow);

        pRecInfo->mpTrack->SetState(tsPlay);
//        LineText(
//          *pDc,
//          xState,
//          Line2y(pRecInfo->mTrackIndex),
//          wState,
//          pRecInfo->mpTrack->GetStateChar());

//        delete pDc;
      }
      if (
        !pRecInfo->mpTrack->GetAudioMode() &&
        !gpProject->GetPlayer()->RecdBuffer.IsEmpty())
      {
        //int choice = wxMessageBox("Keep recorded events?", "You played", wxOK | wxCANCEL);
        //if (choice == wxOK)
        {
          wxBeginBusyCursor();
          gpProject->NewUndoBuffer();
          pRecInfo->mpTrack->MergeRange(
            &gpProject->GetPlayer()->RecdBuffer,
            pRecInfo->mFromClock,
            pRecInfo->mToClock,
            pRecInfo->mIsMuted);
          wxEndBusyCursor();
          Refresh();
          NextWin->Refresh();
        }
      }
    }
  }
*/
}
