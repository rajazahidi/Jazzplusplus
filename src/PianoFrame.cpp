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

#include "PianoFrame.h"
#include "PianoWindow.h"
#include "Song.h"
#include "Track.h"
#include "Synth.h"
#include "StandardFile.h"
#include "Filter.h"
#include "Dialogs.h"
#include "Harmony.h"
#include "Command.h"
#include "Globals.h"
#include "Player.h"
#include "ControlEdit.h"
#include "GuitarFrame.h"
#include "HarmonyP.h"
#include "HarmonyBrowserAnalyzer.h"
#include "ToolBar.h"
#include "SelectControllerDialog.h"
#include "ResourceDialog.h"
#include "Help.h"
#include "Rectangle.h"

#include <sstream>

using namespace std;

// Mouse Actions Mapping
enum
{
  MA_PLAY = 1, // 0 represents no action.
  MA_CYCLE,
  MA_SELECT,
  MA_CONTSEL,
  MA_CUTPASTE,
  MA_LENGTH,
  MA_DIALOG,
  MA_LISTEN,
  MA_COPY,
  MA_VELOCITY
};

const int play_actions[12] =
{
  // left        middle           right
  MA_PLAY,       MA_CYCLE,        0,            // plain
  MA_CYCLE,      0,               0,            // shift
  0,             0,               0,            // ctrl
  0,             0,               0             // shift+ctrl
};

const int evnt_actions[12] =
{
  // left        middle          right
  MA_SELECT,     MA_CUTPASTE,    MA_LENGTH,      // plain
  MA_CONTSEL,    MA_COPY,        MA_LISTEN,      // shift
  MA_VELOCITY,   MA_DIALOG,      MA_VELOCITY,    // ctrl
  MA_CUTPASTE,   0,              MA_COPY         // shift+ctrl
};

const char mouse_help[] =
  "On Top Line:\n"
  "    Left Click:  Start/stop play\n"
  "        +Shift:  Start/stop cycle play\n"
  "    Middle Click:  Start/stop cycle play\n"
  "On Event Area:\n"
  "    Left Click:  Depends on mode\n"
  "        +Shift:  Continue selection\n"
  "        +Ctrl:  Increase velocity\n"
  "        +Ctrl+Shift:  Cut/paste event\n"
  "    Middle Click:  Cut/paste event\n"
  "        +Shift:  Copy event\n"
  "        +Ctrl:  Event dialog\n"
  "    Right Click:  Edit note length / change track\n"
  "        +Shift:  Play pitch\n"
  "        +Ctrl:  Decrease velocity\n"
  "        +Ctrl+Shift:  Copy event\n";


static int PianoFontSizes[] =
{
  6,  // Tiny
  7,  // Small
  8,  // Medium
  10, // Large
  12, // Huge
  -1, // End of list
};


// ************************************************************************
// Menubar
// ************************************************************************

#define ACT_SETTINGS                5
#define MEN_FILTER                  6
#define MEN_SNAP                    7
#define MEN_METERCH                 8
#define ACT_HELP_MOUSE              9

#define MEN_COPY                   10
#define MEN_SHIFT                  11
#define MEN_QUANTIZE               12
#define MEN_UNDO                   13
#define MEN_SETCHAN                14
#define MEN_TRANSP                 15
#define MEN_VELOC                  16
#define MEN_CUT                    17
#define MEN_LERI                   18
#define MEN_UPDN                   19
#define MEN_LENGTH                 20

#define MEN_ERASE                  21
#define MEN_VISIBLE                22

#define MEN_CTRL_EDIT              23
#define MEN_CTRL_PITCH             24
#define MEN_CTRL_CONTR             25
#define MEN_CTRL_VELOC             26
#define MEN_CTRL_NONE              27
#define MEN_CTRL_MODUL             28

#define MEN_GUITAR                 29
#define MEN_HELP_PWIN              30
#define MEN_CLEANUP                31

#define MEN_SNAP_8                 32
#define MEN_SNAP_8D                33
#define MEN_SNAP_16                34
#define MEN_SNAP_16D               35
#define MEN_RESET                  36
#define MEN_VIS_ALL_TRK            37
#define MEN_SEARCHREP              38
#define MEN_SHIFTL                 39
#define MEN_SHIFTR                 40

#define ACT_CLOSE                  41
#define MEN_CTRL_TEMPO             42

#define MEN_MSELECT                43
#define MEN_MLENGTH                44
#define MEN_MDIALOG                45
#define MEN_MCUTPASTE              46
#define MEN_REDO                   47
#define MEN_ZOOMIN                 48
#define MEN_ZOOMOUT                49

#define MEN_CTRL_POLY_AFTER        50
#define MEN_CTRL_CHANNEL_AFTER     51

#define MEN_SEQLENGTH              52
#define MEN_MIDIDELAY              53
#define MEN_CONVERT_TO_MODULATION  54

/*
static JZToolDef tdefs[] =
{
  { MEN_MSELECT,     TRUE, select_xpm,    "select events"},
  { MEN_MLENGTH,     TRUE, length_xpm,    "change length"},
  { MEN_MDIALOG,     TRUE, dialog_xpm,    "event dialog"},
  { MEN_MCUTPASTE,   TRUE, cutpaste_xpm,  "cut/paste events"},
  { JZToolBar::eToolBarSeparator },
  { MEN_SNAP_8,      TRUE, note8_xpm,     "snap 1/8"},
  { MEN_SNAP_8D,     TRUE, note83_xpm,    "snap 1/12"},
  { MEN_SNAP_16,     TRUE, note16_xpm,    "snap 1/16"},
  { MEN_SNAP_16D,    TRUE, note163_xpm,   "snap 1/24"},
  { JZToolBar::eToolBarSeparator },
  { MEN_CUT,         FALSE, cut_xpm,      "cut selection"},
  { MEN_ERASE,       FALSE, delete_xpm,   "delete selection"},
  { MEN_QUANTIZE,    FALSE, quantize_xpm, "quantize selection"},
  { MEN_SHIFTL,      FALSE, shiftl_xpm,   "shift selection left"},
  { MEN_SHIFTR,      FALSE, shiftr_xpm,   "shift selection right"},
  { MEN_VIS_ALL_TRK, TRUE,  evnts_xpm,    "show events from all tracks"},
  { JZToolBar::eToolBarSeparator },
  { MEN_ZOOMIN,      FALSE, zoomin_xpm,   "zoom in"},
  { MEN_ZOOMOUT,     FALSE, zoomout_xpm,  "zoom out"},
  { MEN_UNDO,        FALSE, undo_xpm,     "undo"},
  { MEN_REDO,        FALSE, redo_xpm,     "redo"},
  { MEN_RESET,       FALSE, panic_xpm,    "all notes off"},
  { MEN_HELP_PWIN,   FALSE, help_xpm,     "help"},
  { JZToolBar::eToolBarEnd }
};
*/


// These are the tool bar icons.
#include "Bitmaps/note8.xpm"
#include "Bitmaps/note83.xpm"
#include "Bitmaps/note16.xpm"
#include "Bitmaps/note163.xpm"
#include "Bitmaps/cut.xpm"
#include "Bitmaps/delete.xpm"
#include "Bitmaps/quantize.xpm"
#include "Bitmaps/evnts.xpm"
#include "Bitmaps/undo.xpm"
#include "Bitmaps/redo.xpm"
#include "Bitmaps/zoomin.xpm"
#include "Bitmaps/zoomout.xpm"
#include "Bitmaps/panic.xpm"
#include "Bitmaps/help.xpm"
#include "Bitmaps/shiftl.xpm"
#include "Bitmaps/shiftr.xpm"
#include "Bitmaps/select.xpm"
#include "Bitmaps/length.xpm"
#include "Bitmaps/dialog.xpm"
#include "Bitmaps/cutpaste.xpm"

// positions for controller editor
#define CtrlH(h)        ((h)/4)
#define CtrlY(h)        (h - CtrlH(h))

// -------------------------------------------------------------------------
// MousePiano
// -------------------------------------------------------------------------

class tListen : public wxTimer
{
  public:

    int Active;
    int Pitch, Channel;

    tListen()
      : Active(0)
    {
    }
    void KeyOn(
      JZTrack *t,
      int Pitch,
      int Channel,
      int Veloc = 64,
      int Millisec = 100);

    void Notify();

  private:

    JZTrack *track;
};

void tListen::KeyOn(JZTrack *t, int pitch, int channel, int veloc, int milli)
{
  if (!Active)
  {
    Pitch = pitch;
    Channel = channel;
    tKeyOn k(0, Channel, pitch, veloc);
    gpMidiPlayer->OutNow(t, &k);
    Active = 1;
    Start(milli);
    track = t;
  }
}

void tListen::Notify()
{
  Stop();
  tKeyOff k(0, Channel, Pitch);
  gpMidiPlayer->OutNow(track, &k);
  Active = 0;
}

static tListen Listen;

//*****************************************************************************
// Description:
//   This is the track piano class definition.
//*****************************************************************************
BEGIN_EVENT_TABLE(JZPianoFrame, wxFrame)

  EVT_MENU(MEN_ZOOMIN, JZPianoFrame::OnZoomIn)
  EVT_MENU(MEN_ZOOMOUT, JZPianoFrame::OnZoomOut)
  EVT_MENU(MEN_SNAP_8, JZPianoFrame::OnSnap8)
  EVT_MENU(MEN_SNAP_8D, JZPianoFrame::OnSnap8D)
  EVT_MENU(MEN_SNAP_16, JZPianoFrame::OnSnap16)
  EVT_MENU(MEN_SNAP_16D, JZPianoFrame::OnSnap16D)

  EVT_MENU(MEN_MSELECT, JZPianoFrame::OnMSelect) 
  EVT_MENU(MEN_MLENGTH, JZPianoFrame::OnMLength) 
  EVT_MENU(MEN_MDIALOG, JZPianoFrame::OnMDialog) 
  EVT_MENU(MEN_MCUTPASTE, JZPianoFrame::OnMCutPaste)
  EVT_MENU(MEN_GUITAR, JZPianoFrame::OnGuitar)

  EVT_MENU(MEN_RESET, JZPianoFrame::OnReset)
  EVT_MENU(MEN_VIS_ALL_TRK, JZPianoFrame::OnVisibleAllTracks)
  EVT_MENU(MEN_ERASE, JZPianoFrame::OnErase)
  EVT_MENU(MEN_CUT, JZPianoFrame::OnCut)
  EVT_MENU(MEN_COPY, JZPianoFrame::OnCopy)
  EVT_MENU(MEN_SHIFT, JZPianoFrame::OnShift)
  EVT_MENU(MEN_SHIFTL, JZPianoFrame::OnShiftLeft)
  EVT_MENU(MEN_SHIFTR, JZPianoFrame::OnShiftRight)
  EVT_MENU(MEN_LERI, JZPianoFrame::OnExchangeLeftRight)
  EVT_MENU(MEN_UPDN, JZPianoFrame::OnExchangeUpDown)
  EVT_MENU(MEN_QUANTIZE, JZPianoFrame::OnQuantize)
  EVT_MENU(MEN_UNDO, JZPianoFrame::OnUndo)
  EVT_MENU(MEN_REDO, JZPianoFrame::OnRedo)
  EVT_MENU(MEN_CTRL_PITCH, JZPianoFrame::OnCtrlPitch)
  EVT_MENU(MEN_CTRL_MODUL, JZPianoFrame::OnCtrlModulation)
  EVT_MENU(MEN_CTRL_CONTR, JZPianoFrame::OnSelectController)
  EVT_MENU(MEN_CTRL_VELOC, JZPianoFrame::OnCtrlVelocity)
  EVT_MENU(MEN_CTRL_TEMPO, JZPianoFrame::OnCtrlTempo)
  EVT_MENU(MEN_CTRL_NONE, JZPianoFrame::OnCtrlNone)
  EVT_MENU(MEN_CTRL_POLY_AFTER, JZPianoFrame::OnCtrlPolyAftertouchEdit)
  EVT_MENU(MEN_CTRL_CHANNEL_AFTER, JZPianoFrame::CtrlChannelAftertouchEdit)
// FIXME PAT - We need to bring these back once Dave has figured out what
//             he's doing with them in relation to the track window.
//  EVT_MENU(MEN_CLEANUP, JZPianoFrame::MenCleanup)
//  EVT_MENU(MEN_SEARCHREP, JZPianoFrame::MenSearchReplace)
//  EVT_MENU(MEN_TRANSP, JZPianoFrame::MenTranspose)
//  EVT_MENU(MEN_SETCHAN, JZPianoFrame::MenSetChannel)
  EVT_MENU(MEN_VELOC, JZPianoFrame::ActVelocityDialog)
//  EVT_MENU(MEN_LENGTH, JZPianoFrame::MenLength)
  EVT_MENU(MEN_SEQLENGTH, JZPianoFrame::ActSequenceLengthDialog)
  EVT_MENU(MEN_MIDIDELAY, JZPianoFrame::ActMidiDelayDialog)

//  EVT_MENU(MEN_CONVERT_TO_MODULATION, JZPianoFrame::MenConvertToModulation)
  EVT_MENU(ACT_SETTINGS, JZPianoFrame::ActSettingsDialog)
  EVT_MENU(MEN_FILTER, JZPianoFrame::OnFilter)
  EVT_MENU(MEN_SNAP, JZPianoFrame::SnapDlg)

  // These are all "Patrick Approved"
  EVT_CLOSE(JZPianoFrame::ActCloseEvent)
  EVT_MENU(ACT_CLOSE, JZPianoFrame::ActClose)
  EVT_MENU(ACT_HELP_MOUSE, JZPianoFrame::ActHelpMouse)

//  EVT_MENU(wxID_EXIT, JZPianoFrame::OnFileExit)

//  EVT_MENU(wxID_HELP_CONTENTS, JZPianoFrame::OnHelpContents)

//  EVT_MENU(wxID_ABOUT, JZPianoFrame::OnHelpAbout)

END_EVENT_TABLE()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZPianoFrame::JZPianoFrame(
  wxWindow* pParent,
  const wxString& Title,
  JZSong* pSong,
  const wxPoint& Position,
  const wxSize& Size)
  : wxFrame(
      pParent,
      wxID_ANY,
      Title,
      Position,
      Size,
      wxDEFAULT_FRAME_STYLE | wxNO_FULL_REPAINT_ON_RESIZE),
    Canvas(0),
    mpFilter(0),
//    mpFileMenu(0),
//    mpEditMenu(0)
    Song(pSong),
    mpFont(0),
    mpFixedFont(0),
    MousePlay(play_actions),
    MouseEvnt(evnt_actions),
    mpToolBar(0),
    mpGuitarFrame(0)
{
  CreateToolBar();

//  CreateMenu();

  mpFilter = new tFilter(Song);

  MouseAction = 0;
  DialogBox = 0;
  MixerForm = 0;

  mTopInfoHeight = 40;
  mLeftInfoWidth = 100;
  mEventsX = mLeftInfoWidth;
  mEventsY = mTopInfoHeight;
  mEventsWidth = mEventsHeight = 0;
  mTrackHeight = 0;
  LittleBit = 1;

  FontSize = PianoFontSizes[1]; // Must be an entry in the array.
  UseColors = 1;

  PlayClock = -1;

  hFixedFont = 0;
  CanvasX = CanvasY = CanvasW = CanvasH = 0;
  FromClock = ToClock = 0;
  FromLine = ToLine = 0;

  InitColors();
  
  CreateMenu();

  mpToolBar->ToggleTool(MEN_MSELECT, TRUE);
  MouseEvnt.SetLeftAction(MA_SELECT);

  ClocksPerPixel = 4;

  TrackNr = 0;
  Track = Song->GetTrack(TrackNr);
  SnapDenomiator = 16;
  mpToolBar->ToggleTool(MEN_SNAP_16, TRUE);
  nSnaps = 0;

  for (int i = 0; i < eMaxTrackCount; i++)
  {
    mFromLines[i] = 64;
  }

  DrumFont = 0;

  VisibleKeyOn = 1;
  VisiblePitch = 0;
  VisibleController = 0;
  VisibleProgram = 0;
  VisibleTempo = 0;
  VisibleSysex = 0;
  VisiblePlayTrack = 0;
  VisibleDrumNames = 1;
  VisibleAllTracks = 0;
  VisibleHBChord = 1;
  VisibleMono = 0;

  MouseLine = -1;

  CtrlEdit = 0;

  Canvas = new JZPianoWindow(this);

  SnapSel = new tSnapSelection(Canvas);

  Setup();
//  Canvas->SetScrollRanges();
//  Canvas->SetScrollPosition(0, 0); //this wasnt here before wx2, why?
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZPianoFrame::~JZPianoFrame()
{
  delete mpFont;
  delete mpFixedFont;

  if (CtrlEdit)
    delete CtrlEdit;
  if (Canvas)
    delete Canvas;
  if (SnapSel)
    delete SnapSel;

  delete mpFilter;

  delete MixerForm;
  if (mpGuitarFrame)
  {
    delete mpGuitarFrame;
  }
  
  delete mpToolBar;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZPianoFrame::CreateToolBar()
{
  JZToolDef ToolBarDefinitions[] =
  {
    { ID_SELECT,           true, select_xpm,  "select events" },
    { ID_CHANGE_LENGTH,    true, length_xpm,   "change length"},
    { ID_EVENT_DIALOG,     true, dialog_xpm,   "event dialog"},
    { ID_CUT_PASTE_EVENTS, true, cutpaste_xpm, "cut/paste events"},
    { JZToolBar::eToolBarSeparator },
    { ID_SNAP_8,   true, note8_xpm,   "snap 1/8"},
    { ID_SNAP_8D,  true, note83_xpm,  "snap 1/12"},
    { ID_SNAP_16,  true, note16_xpm,  "snap 1/16"},
    { ID_SNAP_16D, true, note163_xpm, "snap 1/24"},
    { JZToolBar::eToolBarSeparator },
    { wxID_CUT,        false, cut_xpm,      "cut selection"},
    { wxID_DELETE,     false, delete_xpm,   "delete selection" },
    { ID_QUANTIZE,     false, quantize_xpm, "quantize selection" },
    { ID_SHIFT_LEFT,   false, shiftl_xpm,   "shift selection left"},
    { ID_SHIFT_RIGHT,                     false, shiftr_xpm, "shift selection right"},
    { ID_SHOW_ALL_EVENTS_FROM_ALL_TRACKS, true,  evnts_xpm,  "show events from all tracks"},
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

void JZPianoFrame::Setup()
{
  // This section is from JZEventFrame::Setup()

  int lx,ly;

  wxDC *dc = new wxClientDC(Canvas);

  dc->SetFont(wxNullFont);
  delete mpFixedFont;
  mpFixedFont = new wxFont(12, wxSWISS, wxNORMAL, wxNORMAL);

  dc->SetFont(*mpFixedFont);
  dc->GetTextExtent("M", &lx, &ly);
  hFixedFont = (int)ly;

  delete mpFont;
  mpFont = new wxFont(FontSize, wxSWISS, wxNORMAL, wxNORMAL);
  dc->SetFont(*mpFont);

  dc->GetTextExtent("M", &lx, &ly);
  LittleBit = (int)(lx/2);

  dc->GetTextExtent("HXWjgi", &lx, &ly);
  mTrackHeight = (int)ly + LittleBit;
  delete dc;


  // This section is from JZPianoFrame::Setup()

  int x, y;

  dc=new wxClientDC(Canvas);
  dc->SetFont(*mpFixedFont);
  dc->GetTextExtent("H", &x, &y);
  mTopInfoHeight = hFixedFont + 2 * LittleBit;

  dc->SetFont(*mpFont);
  dc->GetTextExtent("H", &x, &y);
  LittleBit = (int)(x/2);

  delete DrumFont;
  DrumFont = new wxFont(FontSize+3, wxSWISS, wxNORMAL, wxNORMAL);
  dc->SetFont(*DrumFont);
  dc->GetTextExtent("Low Conga mid 2 or so", &x, &y);
  wPiano = (int)x + LittleBit;

  mLeftInfoWidth = wPiano;
  delete dc;
}

// show the guitar edit window.
void JZPianoFrame::OnGuitar(wxCommandEvent& Event)
{
  if (!mpGuitarFrame)
  {
    mpGuitarFrame = new JZGuitarFrame(this);
  }
  mpGuitarFrame->Show(TRUE);
}




void JZPianoFrame::NewPosition(int track, int clock)
{
  mFromLines[TrackNr] = FromLine;

  // change track
  if (track >= 0)
  {
    TrackNr = track;
    Track = Song->GetTrack(TrackNr);
    SetTitle(Track->GetName());
  }

  // change position
  if (clock >= 0)
  {
    int x = Clock2x(clock);
    Canvas->SetScrollPosition(x - mLeftInfoWidth, Line2y(mFromLines[TrackNr]));
  }

// SN++ Ist geaendert. OnPaint zeichnet immer neu -> Bug Fix bei ZoomOut!
/*
  // OnPaint() redraws only if clock has changed
  if (CtrlEdit && track >= 0)
    CtrlEdit->ReInit(Track, FromClock, ClocksPerPixel);
*/
  Redraw();
}


void JZPianoFrame::CreateMenu()
{
  wxMenu *win_menu = new wxMenu;
  win_menu->Append(ACT_CLOSE, "&Close");

  wxMenu *edit_menu = new wxMenu("",wxMENU_TEAROFF);
  edit_menu->Append(MEN_ERASE, "&Delete");
  edit_menu->Append(MEN_COPY, "&Copy");
  edit_menu->Append(MEN_CUT, "&Cut");
  edit_menu->Append(MEN_SHIFT, "&Shift ...");
  edit_menu->Append(MEN_QUANTIZE, "&Quantize ...");
  edit_menu->Append(MEN_SETCHAN, "&Set MIDI Channel ...");
  edit_menu->Append(MEN_TRANSP, "&Transpose ...");
  edit_menu->Append(MEN_VELOC, "&Velocity ...");
  edit_menu->Append(MEN_LENGTH, "&Length ...");

  edit_menu->Append(MEN_SEQLENGTH, "&Sequence Length ...");
  edit_menu->Append(MEN_MIDIDELAY, "&Midi Delay ...");
  edit_menu->Append(MEN_CONVERT_TO_MODULATION, "&Convert to Modulation(experimental)");

  edit_menu->Append(MEN_LERI, "&Left <-> Right");
  edit_menu->Append(MEN_UPDN, "&Up <-> Down");
  edit_menu->Append(MEN_CLEANUP, "&Cleanup ...");
  edit_menu->Append(MEN_SEARCHREP, "&Search Replace ...");

  wxMenu *setting_menu = new wxMenu("",wxMENU_TEAROFF);
  setting_menu->Append(MEN_FILTER,    "&Filter ...");
  setting_menu->Append(ACT_SETTINGS,  "&Window ...");
  setting_menu->Append(MEN_VISIBLE,   "&Events...");
  setting_menu->Append(MEN_SNAP,      "&Snap ...");
  setting_menu->Append(MEN_METERCH,   "&Meterchange ...");

  wxMenu *misc_menu = new wxMenu("",wxMENU_TEAROFF);
  misc_menu->Append(MEN_UNDO,   "&Undo");
  misc_menu->Append(MEN_REDO,   "&Redo");
  misc_menu->Append(MEN_CTRL_PITCH,        "Edit &Pitch");
  misc_menu->Append(MEN_CTRL_VELOC,        "Edit &Velocity");
  misc_menu->Append(MEN_CTRL_MODUL,        "Edit &Modulation");

  misc_menu->Append(MEN_CTRL_POLY_AFTER, "Edit &Key Aftertouch");
  misc_menu->Append(MEN_CTRL_CHANNEL_AFTER,  "Edit &Chn Aftertouch");

  misc_menu->Append(MEN_CTRL_CONTR,        "Edit &Controller ...");
  misc_menu->Append(MEN_CTRL_TEMPO,        "Edit &Tempo");
  misc_menu->Append(MEN_CTRL_NONE,        "Edit &None");
  misc_menu->Append(MEN_GUITAR,                "&Guitar board");

  wxMenu *help_menu = new wxMenu("",wxMENU_TEAROFF);
  help_menu->Append(MEN_HELP_PWIN, "&Pianowin");
  help_menu->Append(ACT_HELP_MOUSE, "&Mouse");

  wxMenuBar *menu_bar = new wxMenuBar;
  menu_bar->Append(win_menu,    "&Window");
  menu_bar->Append(edit_menu,    "&Edit");
  menu_bar->Append(setting_menu, "&Settings");
  menu_bar->Append(misc_menu,    "&Misc");
  menu_bar->Append(help_menu,    "&Help");

  SetMenuBar(menu_bar);
}



void JZPianoFrame::OnMenuCommand(int id)
{
  int cw, ch;
  GetClientSize(&cw, &ch);

  switch (id)
  {
    case MEN_HELP_PWIN:
      gpHelpInstance->ShowTopic("Piano Window");
      break;
  }
}


void JZPianoFrame::OnFilter(wxCommandEvent& Event)
{
  mpFilter->Dialog(0);
}


/**activate velocity edit*/
void JZPianoFrame::OnCtrlVelocity(wxCommandEvent& Event)
{
  delete CtrlEdit;
  int cw, ch;
  GetClientSize(&cw, &ch);

  CtrlEdit = new tVelocEdit(
    this,
    "Velocity",
    wPiano,
    0,
    CtrlY(ch),
    CanvasW,
    CtrlH(ch));

  CtrlEdit->ReInit(Track, FromClock, ClocksPerPixel);
  Redraw();
}


void JZPianoFrame::CtrlChannelAftertouchEdit(wxCommandEvent& Event)
{
  delete CtrlEdit;
  int cw, ch;
  GetClientSize(&cw, &ch);
  CtrlEdit = new tChannelAfterEdit(this, "Channel Aftertouch", wPiano, 0, CtrlY(ch), CanvasW,CtrlH(ch));
  CtrlEdit->ReInit(Track, FromClock, ClocksPerPixel);
  Redraw();
  
}

void JZPianoFrame::OnCtrlPolyAftertouchEdit(wxCommandEvent& Event)
{
  int cw, ch;
  GetClientSize(&cw, &ch);
  delete CtrlEdit;
  CtrlEdit = new tPolyAfterEdit(this, "Key Aftertouch", wPiano, 0, CtrlY(ch), CanvasW,CtrlH(ch));
  CtrlEdit->ReInit(Track, FromClock, ClocksPerPixel);
  Redraw();  
}

void JZPianoFrame::OnCtrlNone(wxCommandEvent& Event)
{
  delete CtrlEdit;
  CtrlEdit = 0;
  Redraw();  
}

void JZPianoFrame::OnCtrlTempo(wxCommandEvent& Event)
{
  tEventIterator Iterator(Track);
  Track->Sort();
  JZEvent* pEvent = Iterator.Range(0, (unsigned) Track->GetLastClock() + 1);
  tSetTempo *t;
  int Min = 240;
  int Max = 20;
  while (pEvent)
  {
    if ((t = pEvent->IsSetTempo()) != 0)
    {
      if (t->GetBPM() < Min)
      {
        Min = t->GetBPM();
      }
      if (t->GetBPM() > Max)
      {
        Max = t->GetBPM();
      }
    }
    pEvent = Iterator.Next();
  }
  if (Min - 50 > 20)
  {
    Min -= 50;
  }
  else
  {
    Min = 20;
  }
  if (Max + 50 < 240)
  {
    Max += 50;
  }
  else
  {
    Max = 240;
  }
  delete CtrlEdit;
  int cw, ch;
  GetClientSize(&cw, &ch);
  CtrlEdit = new tTempoEdit(
    Min,
    Max,
    this,
    "Tempo",
    wPiano,
    0,
    CtrlY(ch),
    CanvasW,
    CtrlH(ch));
  CtrlEdit->ReInit(Track, FromClock, ClocksPerPixel);
  Redraw();  
}

void JZPianoFrame::OnSelectController(wxCommandEvent& Event)
{
  int i = SelectControllerDlg();
  if (i > 0)
    {
      delete CtrlEdit;
      int cw, ch;
      GetClientSize(&cw, &ch);
      CtrlEdit = new tCtrlEdit(
        i - 1,
        this,
        gpConfig->CtrlName(i).first.c_str(),
        wPiano,
        0,
        CtrlY(ch),
        CanvasW,
        CtrlH(ch));
      CtrlEdit->ReInit(Track, FromClock, ClocksPerPixel);
      Redraw();
    }  
}

void JZPianoFrame::OnCtrlModulation(wxCommandEvent& Event)
{
  delete CtrlEdit;
  int cw, ch;
  GetClientSize(&cw, &ch);
  CtrlEdit = new tCtrlEdit(1, this, "Modulation", wPiano, 0, CtrlY(ch), CanvasW, CtrlH(ch));
  CtrlEdit->ReInit(Track, FromClock, ClocksPerPixel);
  Redraw();  
}

void JZPianoFrame::OnCtrlPitch(wxCommandEvent& Event)
{
  int cw, ch;
  GetClientSize(&cw, &ch);
  delete CtrlEdit;
  CtrlEdit = new tPitchEdit(this, "Pitch", wPiano, 0, CtrlY(ch), CanvasW, CtrlH(ch));
  
  CtrlEdit->ReInit(Track, FromClock, ClocksPerPixel);
  Redraw();  
}

/**
   redo undone actions
*/
void JZPianoFrame::OnRedo(wxCommandEvent& Event)
{
  Song->Redo();
  Redraw();
  if (CtrlEdit && Track >= 0)
    CtrlEdit->ReInit(Track, FromClock, ClocksPerPixel);  
}

/**
 undo actions 
*/
void JZPianoFrame::OnUndo(wxCommandEvent& Event)
{
  Song->Undo();
  Redraw();
  if (CtrlEdit && Track >= 0)
    CtrlEdit->ReInit(Track, FromClock, ClocksPerPixel);  
}

/** quantize selected events*/
void JZPianoFrame::OnQuantize(wxCommandEvent& Event)
{
  if (EventsSelected())
    {
      tCmdQuantize cmd(mpFilter, SnapClocks(), 0, 0);
      cmd.Execute(1);
      Redraw();
    }  
}

/** flip events up and down*/
void JZPianoFrame::OnExchangeUpDown(wxCommandEvent& Event)
{
  if (EventsSelected())
    {
      tCmdExchUpDown cmd(mpFilter);
      cmd.Execute(1);
      Redraw();
    }  
}

/**flip events left to righ*/
void JZPianoFrame::OnExchangeLeftRight(wxCommandEvent& Event)
{
  if (EventsSelected())
    {
      tCmdExchLeftRight cmd(mpFilter);
      cmd.Execute(1);
      Redraw();
    }  
}

/**shift events snapclock clocks to left*/
void JZPianoFrame::OnShiftLeft(wxCommandEvent& Event)
{
  if (EventsSelected())
    {
      int steps =  -SnapClocks();
      tCmdShift cmd(mpFilter, steps);
      cmd.Execute();
      Redraw();
    }
}

/**shift events snapclock clocks to right*/
void JZPianoFrame::OnShiftRight(wxCommandEvent& Event)
{
  if (EventsSelected())
    {
      int steps = SnapClocks();
      tCmdShift cmd(mpFilter, steps);
      cmd.Execute();
      Redraw();
    }
}

void JZPianoFrame::OnShift(wxCommandEvent& Event)
{
  // FIXME PAT - Bring this back once Dave has figured out what's he's doing
  // with the trackwin stuff.
  //MenShift(SnapClocks());
}

void JZPianoFrame::OnCut(wxCommandEvent& Event)
{
  CutOrCopy(MEN_CUT);
}

void JZPianoFrame::OnCopy(wxCommandEvent& Event)
{
  CutOrCopy(MEN_COPY);
}

/**helper for cut and copy events*/
void JZPianoFrame::CutOrCopy(int id)
{
  if (EventsSelected())
  {
    PasteBuffer.Clear();
    tCmdCopyToBuffer cmd(mpFilter, &PasteBuffer);
    mpFilter->OtherSelected = VisibleTempo;
    cmd.Execute(0);        // no UNDO
    if (id == MEN_CUT)
    {
      tCmdErase cmd(mpFilter);
      cmd.Execute(1);        // with UNDO
      Redraw();
    }
    mpFilter->OtherSelected = 0;
    if (mpGuitarFrame)
    {
      mpGuitarFrame->Update();
//      mpGuitarFrame->Redraw();
    }
  }  
}


void JZPianoFrame::OnErase(wxCommandEvent& Event)
{
  if (EventsSelected())
  {
    tCmdErase cmd(mpFilter);
    cmd.Execute(1);        // with UNDO
    Redraw();
  }  
}

/**togle display of events from all tracks, or just from the current track */
void JZPianoFrame::OnVisibleAllTracks(wxCommandEvent& Event)
{
  VisibleAllTracks = !VisibleAllTracks;
  //VisibleAllTracks = mpToolBar->GetToolState(MEN_VIS_ALL_TRK);
  Redraw();  
}

/**send a midi reset*/
void JZPianoFrame::OnReset(wxCommandEvent& Event)
{
  gpMidiPlayer->AllNotesOff(1);
}

//INSER_EVENT_HANDLER_HERE

/*

//these wont compile nicely

case MEN_VISIBLE:   VisibleDialog(); break;

case MEN_METERCH:        MenMeterChange(); break;

*/

void JZPianoFrame::OnMSelect(wxCommandEvent& Event)
{
  PressRadio(MEN_MSELECT);
  MouseEvnt.SetLeftAction(MA_SELECT);
}

void JZPianoFrame::OnMLength(wxCommandEvent& Event)
{
  PressRadio(MEN_MLENGTH);
  MouseEvnt.SetLeftAction(MA_LENGTH);
}

void JZPianoFrame::OnMDialog(wxCommandEvent& Event)
{
  PressRadio(MEN_MDIALOG);
  MouseEvnt.SetLeftAction(MA_DIALOG);
}

void JZPianoFrame::OnMCutPaste(wxCommandEvent& Event)
{
  PressRadio(MEN_MCUTPASTE);
  MouseEvnt.SetLeftAction(MA_CUTPASTE);
}

  void JZPianoFrame::OnZoomIn(wxCommandEvent& Event){
      if (ClocksPerPixel > 1) {
        ZoomIn();
        // TrackNr for Ctr-Editor
        NewPosition(TrackNr,FromClock);
      }
}

void JZPianoFrame::OnZoomOut(wxCommandEvent& Event){
  if (ClocksPerPixel<32) {
    ZoomOut();
    NewPosition(TrackNr, FromClock);
  }
}


void JZPianoFrame::OnSnap8(wxCommandEvent& Event){
  PasteBuffer.Clear(); SetSnapDenom(8);
}


void JZPianoFrame::OnSnap8D(wxCommandEvent& Event){
  PasteBuffer.Clear(); SetSnapDenom(12);
}

void JZPianoFrame::OnSnap16(wxCommandEvent& Event){
  PasteBuffer.Clear(); SetSnapDenom(16);
}

void JZPianoFrame::OnSnap16D(wxCommandEvent& Event){
  PasteBuffer.Clear(); SetSnapDenom(24);
}








// ********************************************************************
// Visible
// ********************************************************************

int JZPianoFrame::IsVisible(JZEvent* pEvent)
{
  switch (pEvent->Stat)
  {
    case StatKeyOn:
      return VisibleKeyOn;
    case StatPitch:
      return VisiblePitch;
    case StatControl:
      return VisibleController;
    case StatProgram:
      return VisibleProgram;
    case StatSetTempo:
      return VisibleTempo;
    case StatSysEx:
      return VisibleSysex;
    case StatPlayTrack:
      return VisiblePlayTrack;
    case StatEndOfTrack:
      return true;
    case StatText:
      return true;
    case StatChnPressure:
      return VisibleMono;
  }
  return 0;
}


int JZPianoFrame::IsVisible(JZTrack *t)
{
  if (!VisibleAllTracks)
    return t == Track;

  return (
    Track->Channel == gpConfig->GetValue(C_DrumChannel)) ==
    (t->Channel == gpConfig->GetValue(C_DrumChannel));
}

#ifdef OBSOLETE

class tVisibleDlg : public wxForm
{
  JZPianoFrame *win;
  public:

    tVisibleDlg(JZPianoFrame *p) : wxForm( USED_WXFORM_BUTTONS ), win(p) {}
    void EditForm(wxPanel *panel);
    virtual void OnOk();
    virtual void OnHelp();
};

void tVisibleDlg::OnOk()
{
  win->SetVisibleAllTracks(win->VisibleAllTracks);
  // win->Redraw();
  wxForm::OnOk();
}

void tVisibleDlg::OnHelp()
{
  gpHelpInstance->ShowTopic("Events");
}


void tVisibleDlg::EditForm(wxPanel *panel)
{
  Add(wxMakeFormMessage("Select Events to be shown"));
  Add(wxMakeFormNewLine());
  Add(wxMakeFormBool("NoteOn", &win->VisibleKeyOn));
  //Add(wxMakeFormBool("Pitch", &win->VisiblePitch));
  Add(wxMakeFormBool("Controller", &win->VisibleController));
  Add(wxMakeFormBool("Program", &win->VisibleProgram));
  Add(wxMakeFormBool("Tempo", &win->VisibleTempo));
  Add(wxMakeFormBool("SysEx", &win->VisibleSysex));
  Add(wxMakeFormBool("PlayTrack", &win->VisiblePlayTrack));
  Add(wxMakeFormNewLine());
  Add(wxMakeFormBool("Show drumnames on drumtracks", &win->VisibleDrumNames));
  Add(wxMakeFormNewLine());
  Add(wxMakeFormBool("Show events from all Tracks", &win->VisibleAllTracks));
  Add(wxMakeFormNewLine());
  Add(wxMakeFormBool("Show harmonies from Harmony Browser", &win->VisibleHBChord));
  AssociatePanel(panel);
}


void JZPianoFrame::VisibleDialog()
{
  wxDialogBox *panel = new wxDialogBox(this, "Select Events", FALSE );
  tVisibleDlg * dlg = new tVisibleDlg(this);
  dlg->EditForm(panel);
  panel->Fit();
  panel->Show(TRUE);
}
#endif // OBSOLETE

// ********************************************************************
// Painting
// ********************************************************************

const int isBlack[12] = {0,1,0,1,0,0,1,0,1,0,1,0};
#define IsBlack(Key)  isBlack[(Key) % 12]


void JZPianoFrame::OnPaintSub(wxDC* dc, int x, int y)
{
//  int OldFromClock = FromClock;

  OnEventWinPaintSub(x, y);

// SN++ Da Jazz nun eine ReDo Funktion hat. Behebt gleichzeitig ein kleines
//                Update Problem beim mehrfachen ZoomOut.
//                Aktives Ctrl-Fenster neu zeichnen bzw. reinitialisieren.

//  if (CtrlEdit && OldFromClock != FromClock)
//    CtrlEdit->ReInit(Track, FromClock, ClocksPerPixel);

  if (CtrlEdit)
    CtrlEdit->ReInit(Track, FromClock, ClocksPerPixel);

//
  xPiano  = CanvasX;

  int StopClk;
  JZBarInfo BarInfo(Song);
  char buf[20];

//OBSOLETE  dc->BeginDrawing();
  dc->DestroyClippingRegion();
  dc->SetBackground(*wxWHITE_BRUSH);
  DrawPlayPosition(dc);
  SnapSel->Draw(*dc, mEventsX, mEventsY, mEventsWidth, mEventsHeight);
  dc->Clear();


  ///////////////////////////////////////////////////////////////
  // horizontal lines(ripped from drawpianoroll code)

//     for (y = Line2y(FromLine); y < mEventsY + mEventsHeight; y += mTrackHeight)
//      if (y > mEventsY)        // cheaper than clipping
//        dc->DrawLine(mEventsX+1, y, mEventsX + mEventsWidth, y);

  dc->SetPen(*wxGREY_PEN);
  wxBrush blackKeysBrush=wxBrush(wxColor(250,240,240),wxSOLID);
  int Pitch = 127 - FromLine;
  y = Line2y(FromLine);
    while (Pitch >= 0 && y < mEventsY + mEventsHeight)
    {
      if (IsBlack(Pitch))
      {
        dc->SetBrush(blackKeysBrush);//*wxLIGHT_GREY_PEN
        dc->DrawRectangle(CanvasX, y, 2000, mTrackHeight);
      }
      else if ((Pitch % 12) == 0)
      {
        dc->SetPen(*wxCYAN_PEN);
        dc->DrawLine(CanvasX, y + mTrackHeight, 2000, y + mTrackHeight);

      }
      else if (!IsBlack(Pitch - 1))
      {
        dc->SetPen(*wxGREEN_PEN);
        dc->DrawLine(CanvasX, y + mTrackHeight, 2000, y + mTrackHeight);
      }

      y += mTrackHeight;
      --Pitch;
    }


  ///////////////////////////////////////////////////////////////


  MouseLine = -1;

  #define VLine(x) DrawLine(x, CanvasY, x, mEventsY + mEventsHeight)
  #define HLine(y) DrawLine(CanvasX, y, CanvasX + CanvasW, y)

  dc->SetPen(*wxBLACK_PEN);

  // vertical lines

  dc->VLine(xPiano);
  dc->VLine(mEventsX);
  dc->VLine(mEventsX-1);
  dc->HLine(mEventsY);
  dc->HLine(mEventsY-1);
  dc->HLine(mEventsY + mEventsHeight);

  // draw vlines and bar numbers

  dc->SetFont(*mpFixedFont);
  BarInfo.SetClock(FromClock);
  StopClk = x2Clock(CanvasX + CanvasW);
  int clk = BarInfo.Clock;
  int intro = Song->GetIntroLength();
  while (clk < StopClk)
  {
    clk = BarInfo.Clock;
    x = Clock2x(clk);
    // vertical lines and bar numbers
    int i;
    dc->SetPen(*wxBLACK_PEN);
    sprintf(buf, "%d", BarInfo.BarNr + 1 - intro);
    if (x > mEventsX)
    {
      dc->DrawText(buf, x + LittleBit, mEventsY - hFixedFont - 2);
      dc->SetPen(*wxGREY_PEN);
      dc->DrawLine(x, mEventsY - hFixedFont, x, mEventsY + mEventsHeight);
    }

    dc->SetPen(*wxLIGHT_GREY_PEN);
    for (i = 0; i < BarInfo.CountsPerBar; i++)
    {
      clk += BarInfo.TicksPerBar / BarInfo.CountsPerBar;
      x = Clock2x(clk);
      if (x > mEventsX)
      {
        dc->DrawLine(x, mEventsY + 1, x, mEventsY + mEventsHeight);
      }
    }
    BarInfo.Next();
  }

  LineText(dc, CanvasX, CanvasY, wPiano, mTopInfoHeight);

  dc->SetPen(*wxBLACK_PEN);
  DrawPianoRoll(dc);

  // Draw chords from harmony-browser.
  if (VisibleHBChord && gpHarmonyBrowser && !Track->IsDrumTrack())
  {
    HBAnalyzer *an = gpHarmonyBrowser->getAnalyzer();
    if (an != 0)
    {
      wxBrush cbrush = *wxBLUE_BRUSH;
      wxBrush sbrush = *wxBLUE_BRUSH;
#ifdef __WXMSW__
      cbrush.SetColour(191,191,255);
      sbrush.SetColour(191,255,191);
#else
      cbrush.SetColour(220,220,255);
      sbrush.SetColour(230,255,230);
#endif

      //dc->SetClippingRegion(mEventsX, mEventsY, mEventsWidth, mEventsHeight);
      dc->SetLogicalFunction(wxXOR);
      dc->SetPen(*wxTRANSPARENT_PEN);

      int steps = an->Steps();
      for (int step = 0; step < steps; step ++)
      {
        int start = an->Step2Clock(step);
        int stop  = an->Step2Clock(step + 1);
        if (stop > FromClock && start < ToClock)
        {
          // this chord is visible
          HBContext *context = an->GetContext(step);
          HBChord chord = context->Chord();
          HBChord scale = context->Scale();

          int x = Clock2x(start);
          if (x < mEventsX)        // clip to left border
            x = mEventsX;
          int w = Clock2x(stop) - x;
          if (w <= 0)
            continue;

          int h = mTrackHeight;
          for (int i = 0; i < 12; i++)
          {
            int pitch = i;
            wxBrush *brush = 0;
            if (chord.Contains(i))
            {
              brush = &cbrush;
            }
            else if (scale.Contains(i))
            {
              brush = &sbrush;
            }
            if (brush)
            {
              dc->SetBrush(*brush);
              while (pitch < 127)
              {
                int y = Pitch2y(pitch);
                if (y >= mEventsY && y <= mEventsY + mEventsHeight - h) // y-clipping
                {
                  dc->DrawRectangle(x, y, w, h);
                }
                pitch += 12;
              }
            }
          }
        }
      }


      //dc->DestroyClippingRegion();
      dc->SetLogicalFunction(wxCOPY);
      dc->SetPen(*wxBLACK_PEN);
      dc->SetBrush(*wxBLACK_BRUSH);


      //delete an; PORTING

    }

  }
  /////////end draw choords

  if (VisibleAllTracks)
  {
    int i;
    for (i = 0; i < Song->nTracks; i++)
    {
      JZTrack *t = Song->GetTrack(i);
      if (t != Track && IsVisible(t))
        DrawEvents(dc, t, StatKeyOn, wxLIGHT_GREY_BRUSH, TRUE);
    }
  }

  if (VisibleKeyOn)
    DrawEvents(dc,Track, StatKeyOn, wxRED_BRUSH, FALSE);
  if (VisiblePitch)
    DrawEvents(dc, Track, StatPitch, wxBLUE_BRUSH, FALSE);
  if (VisibleController)
    DrawEvents(dc, Track, StatControl, wxCYAN_BRUSH, FALSE);
  if (VisibleProgram)
    DrawEvents(dc, Track, StatProgram, wxGREEN_BRUSH, FALSE);
  if (VisibleTempo)
    DrawEvents(dc, Track, StatSetTempo, wxGREEN_BRUSH, FALSE);
  if (VisibleSysex)
    DrawEvents(dc, Track, StatSysEx, wxGREEN_BRUSH, FALSE);
  if (VisiblePlayTrack)
    DrawEvents(dc, Track, StatPlayTrack, wxLIGHT_GREY_BRUSH, FALSE);
  
  DrawEvents(dc, Track, StatEndOfTrack, wxRED_BRUSH, FALSE);
  DrawEvents(dc, Track, StatText, wxBLACK_BRUSH, FALSE);

  dc->SetPen(*wxBLACK_PEN);
  dc->SetBrush(*wxBLACK_BRUSH);
  dc->SetBackground(*wxWHITE_BRUSH);        // xor-bug

  SnapSel->Draw(*dc, mEventsX, mEventsY, mEventsWidth, mEventsHeight);

  DrawPlayPosition(dc);
//OBSOLETE  dc->EndDrawing();
}




void JZPianoFrame::DrawPianoRoll(wxDC* dc)
{
  char buf[20];

  dc->SetBrush(*wxLIGHT_GREY_BRUSH);
  dc->DrawRectangle(xPiano, mEventsY, wPiano, mEventsHeight); //draw grey bg for keyboard
  dc->SetBrush(*wxBLACK_BRUSH);

//  dc->SetTextBackground(*wxLIGHT_GREY);

  int wBlack = wPiano * 2 / 3;
  int Pitch = 127 - FromLine;
  int y = Line2y(FromLine);

  if (
    VisibleKeyOn &&
    !Track->GetAudioMode() &&
    (!Track->IsDrumTrack() || !VisibleDrumNames))
  {
    dc->SetFont(*mpFixedFont);

    while (Pitch >= 0 && y < mEventsY + mEventsHeight)
    {
      if (IsBlack(Pitch))
      {
        dc->DrawRectangle(CanvasX, y, wBlack, mTrackHeight);
        dc->DrawLine(CanvasX + wBlack, y + mTrackHeight/2, CanvasX + wPiano, y + mTrackHeight/2);
        dc->SetPen(*wxWHITE_PEN);
        dc->DrawLine(CanvasX + wBlack+1, y + mTrackHeight/2+1, CanvasX + wPiano, y + mTrackHeight/2+1);
        dc->DrawLine(CanvasX, y, CanvasX + wBlack, y);
        dc->SetPen(*wxBLACK_PEN);
      }
      else if ((Pitch % 12) == 0)
      {
        dc->DrawLine(CanvasX, y + mTrackHeight, CanvasX + wPiano, y + mTrackHeight);
        dc->SetPen(*wxWHITE_PEN);
        dc->DrawLine(CanvasX, y + mTrackHeight + 1, CanvasX + wPiano, y + mTrackHeight + 1);
        dc->SetPen(*wxBLACK_PEN);
        sprintf(buf, "%d", Pitch / 12);
        dc->DrawText(buf, CanvasX + wBlack + LittleBit, y + mTrackHeight / 2);
      }
      else if (!IsBlack(Pitch - 1))
      {
        dc->DrawLine(CanvasX, y + mTrackHeight, CanvasX + wPiano, y + mTrackHeight);
        dc->SetPen(*wxWHITE_PEN);
        dc->DrawLine(CanvasX, y + mTrackHeight + 1, CanvasX + wPiano, y + mTrackHeight + 1);
        dc->SetPen(*wxBLACK_PEN);
      }

      y += mTrackHeight;
      --Pitch;
    }
  }
  else if (Track->GetAudioMode())
  {
    dc->SetFont(*DrumFont);
    while (Pitch >= 0 && y < mEventsY + mEventsHeight)
    {
      dc->DrawText(gpMidiPlayer->GetSampleName(Pitch), CanvasX + LittleBit, y);
      y += mTrackHeight;
      --Pitch;
    }
  }
  else
  {
    // Draw text?
    if (VisibleKeyOn && VisibleDrumNames)
    {
      dc->SetFont(*DrumFont);
      while (Pitch >= 0 && y < mEventsY + mEventsHeight)
      {
        dc->DrawText(
          gpConfig->DrumName(Pitch + 1).first.c_str(),
          CanvasX + LittleBit,
          y);

        y += mTrackHeight;

        --Pitch;
      }
    }
    else if (VisibleController)
    {
      dc->SetFont(*DrumFont);
      while (Pitch >= 0 && y < mEventsY + mEventsHeight)
      {
        dc->DrawText(
          gpConfig->CtrlName(Pitch + 1).first.c_str(),
          CanvasX + LittleBit,
          y);

        y += mTrackHeight;
        --Pitch;
      }
    }
    else if (VisibleProgram)
    {
      dc->SetFont(*DrumFont);
      while (Pitch >= 0 && y < mEventsY + mEventsHeight)
      {
        dc->DrawText(
          gpConfig->VoiceName(Pitch + 1).first.c_str(),
          CanvasX + LittleBit,
          y);

        y += mTrackHeight;
        --Pitch;
      }
    }
    else if (VisibleSysex)
    {
      dc->SetFont(*DrumFont);
      while (Pitch >= 0 && y < mEventsY + mEventsHeight)
      {
        dc->DrawText(
          tSynthSysex::GetSysexGroupName(Pitch + 1),
          CanvasX + LittleBit,
          y);
        y += mTrackHeight;
        --Pitch;
      }
    }
    else if (VisiblePitch)
    {
    }
  }

  //dc->DestroyClippingRegion();
  //dc->SetTextBackground(*wxWHITE);
  dc->SetFont(*mpFont);
}


void JZPianoFrame::DrawEvent(wxDC* dc, JZEvent* pEvent, const wxBrush* Brush, int xoor, int force_color)
{
  if (pEvent->IsKeyPressure() || pEvent->IsChnPressure())
  {
    return;
  }

  int length = pEvent->GetLength() / ClocksPerPixel;
  // Always draw at least two pixels to avoid invisible (behind a
  // vertical line) or zero-length events:
  if (length < 3)
    length = 3;
//OBSOLETE  dc->BeginDrawing();
  if (xoor)
    dc->SetLogicalFunction(wxXOR);
  int x = Clock2x(pEvent->GetClock());
  int y = Pitch2y(pEvent->GetPitch());
  if (!xoor)        
  {
    dc->SetBrush(*wxWHITE_BRUSH);
    dc->DrawRectangle(x, y + LittleBit, length, mTrackHeight - 2 * LittleBit);
  }

  // show velocity as colors
  if (force_color != 0 && UseColors && pEvent->IsKeyOn()) {
    int vel = pEvent->IsKeyOn()->Veloc;

    // Next line is "Patrick Approved."
    dc->SetBrush(color_brush[ vel * NUM_COLORS / 128 ]);
  }
  else
  {
    dc->SetBrush(*Brush);
  }
  // end velocity colors

  dc->DrawRectangle(x, y + LittleBit, length, mTrackHeight - 2 * LittleBit);

  if (xoor)
  {
    dc->SetLogicalFunction(wxCOPY);
  }
  dc->SetBrush(*wxBLACK_BRUSH);
//OBSOLETE  dc->EndDrawing();
}




void JZPianoFrame::DrawEvents(wxDC* dc, JZTrack *t, int Stat, const wxBrush* Brush, int force_color)
{
  //dc->SetClippingRegion(mEventsX, mEventsY, mEventsWidth, mEventsHeight);
  dc->SetBrush(*Brush);

  tEventIterator Iterator(t);
  JZEvent* pEvent = Iterator.First();
  int FromPitch = 127 - ToLine;
  int ToPitch   = 127 - FromLine;

  // Coordinate for Linien

  int x0 = Clock2x(0);
  int y0 = Line2y(64);
  char buf[20];

  while (pEvent)
  {
    if (pEvent->Stat == Stat)
    {
      int Pitch   = pEvent->GetPitch();
      int Length = pEvent->GetLength();
      int Clock  = pEvent->GetClock();

      int x1 = Clock2x(Clock);
      int y1 = Line2y(127 - Pitch);
//        if (pEvent->IsPlayTrack()) {
//          y1=Line2y(127-pEvent->IsPlayTrack()->track); //JAVE so the y position of playtrack events tell which track they play (the drawing should rather be polymorpic in my opinion)
      //use pitch instead
      //      }
      // event partially visible?
      if (Clock + Length >= FromClock && FromPitch < Pitch && Pitch <= ToPitch)
      {
        int DrawLength = Length/ClocksPerPixel;
        // do clipping ourselves
        if (x1 < mEventsX)
        {
          DrawLength -= mEventsX - x1;
          x1 = mEventsX;
        }
        // Always draw at least two pixels to avoid invisible (behind a
        // vertical line) or zero-length events:
        if (DrawLength < 3)
        {
          DrawLength = 3;
        }

        // show velocity as colors
        if (!force_color && UseColors && pEvent->IsKeyOn())
        {
          int vel = pEvent->IsKeyOn()->Veloc;
          dc->SetBrush(color_brush[ vel * NUM_COLORS / 128 ]);
        }
        else
        {
          dc->SetBrush(*Brush);
        }
        // end velocity colors

        dc->DrawRectangle(x1, y1 + LittleBit, DrawLength, mTrackHeight - 2 * LittleBit);
        //shouldnt it be in drawevent? odd. 

        if (pEvent->IsPlayTrack())
        {
          dc->SetPen(*wxBLACK_PEN);
          ostringstream Oss;
          Oss << "Track:" << pEvent->IsPlayTrack()->track; 
          dc->DrawText(Oss.str().c_str(), x1, y1 + LittleBit);
        }
      }
      
      if (Clock + Length >= FromClock)
      {
        //thesse events are always visible in vertical
        if (pEvent->IsEndOfTrack())
        {
          dc->SetPen(*wxRED_PEN);
          dc->VLine(x1); //draw a vertical bar
          dc->SetPen(*wxBLACK_PEN);
          sprintf(buf, "EOT"); 
          dc->DrawText(buf, x1, y1 + LittleBit);
        }
        
        if (pEvent->IsText())
        {
          dc->SetPen(*wxGREEN_PEN);
          dc->VLine(x1); //draw a vertical bar
          dc->SetPen(*wxBLACK_PEN);
          sprintf(buf, (const char*)pEvent->IsText()->GetText());
          int textX;
          int textY;
          
          dc->GetTextExtent((const char*)pEvent->IsText()->GetText(), &textX, &textY); 
          dc->SetBrush(*wxWHITE_BRUSH);
          int textlabely = CanvasY + mTopInfoHeight;//text labels drawn at top
          dc->DrawRectangle(x1-textX, textlabely + LittleBit, textX, textY);//mTrackHeight - 2 * LittleBit);
          dc->DrawText(buf, x1-textX, textlabely + LittleBit);
        }
      }

      x0 = x1;
      y0 = y1;

      if (Clock > ToClock)
      {
        break;
      }
    }
    pEvent = Iterator.Next();
  }
  dc->SetBrush(*wxBLACK_BRUSH);
  //dc->DestroyClippingRegion();
}



// ********************************************************************
// Utilities
// ********************************************************************

int JZPianoFrame::SnapClocks()
{
  int clk = Song->TicksPerQuarter * 4L / SnapDenomiator;
  if (clk < 1)
    return 1;
  return clk;
}

int JZPianoFrame::y2Pitch(int y)
{
  int pitch = 127 - y2Line(y);
  if (pitch < 0)
    return 0;
  if (pitch > 127)
    return 127;
  return pitch;
}


int JZPianoFrame::Pitch2y(int Pitch)
{
  return Line2y(127 - Pitch);
}


JZEvent *JZPianoFrame::FindEvent(JZTrack *Track, int Clock, int Pitch)
// Pitch == -1: search for any pitches
{
  tEventIterator Iterator(Track);
  JZEvent* pEvent = Iterator.First();
  while (pEvent)
  {
    if (pEvent->GetClock() <= Clock)
    {
      if ((pEvent->GetClock() + pEvent->GetLength() >= Clock)
           && (pEvent->GetPitch() == Pitch || Pitch == -1)
           && IsVisible(pEvent))
      {
        return pEvent;
      }
    }
    else
    {
      return 0;
    }
    pEvent = Iterator.Next();
  }
  return 0;
}


void JZPianoFrame::kill_keys_aftertouch(JZTrack *t, JZEvent* pEvent)
{
  int key,channel;
  tEventIterator iter(t);
  tKeyPressure *a;
  tKeyOn *k = pEvent->IsKeyOn();
  if (!k)
  {
    return;
  }
  if (k->Length < 2)
  {
    return;
  }
  key = k->Key;
  channel = k->Channel;
  pEvent = iter.Range(k->GetClock() + 1, k->GetClock() + k->Length);
  while (pEvent)
  {
    a = pEvent->IsKeyPressure();
    if (a)
    {
      if (a->Key == key && a->Channel == channel)
      {
        t->Kill(pEvent);
      }
    }
    pEvent = iter.Next();
  }
}

void JZPianoFrame::paste_keys_aftertouch(JZTrack *t, JZEvent* pEvent)
{
  int key,channel;
  tEventIterator iter(t);
  tKeyPressure *a;
  tKeyOn *k = pEvent->IsKeyOn();
  if (!k) return;
  channel = k->Channel;
  if (k->Length < 2) return;
  key = k->Key;
  pEvent = iter.Range(k->GetClock() + 1, k->GetClock() + k->Length);
  while (pEvent)
  {
    a = pEvent->IsKeyPressure();
    if (a)
    {
      if (a->Key == key && a->Channel == channel)
      {
        PasteBuffer.Put(pEvent->Copy());
      }
    }
    pEvent = iter.Next();
  }
}

int JZPianoFrame::nKeyOnEvents()
{
  int count = 0;

  tEventIterator Iterator(&PasteBuffer);
  JZEvent* pEvent = Iterator.First();
  while (pEvent)
  {
    if (pEvent->IsKeyOn())
    {
      count++;
    }
    pEvent = Iterator.Next();
  }
  return count;
}



void JZPianoFrame::Copy(JZTrack *t, JZEvent* pEvent, int Kill)
{
  if (!pEvent)
  {
    return;
  }

  Song->NewUndoBuffer();
  PasteBuffer.Clear();
  PasteBuffer.Put(pEvent->Copy());

  if (pEvent->IsKeyOn())
  {
    paste_keys_aftertouch(t, pEvent);
  }

  if (Kill)
  {
    tKeyOn *k = pEvent->IsKeyOn();
    if (k)
    {
      kill_keys_aftertouch(t,pEvent);
      if (Track->GetAudioMode())
      {
        gpMidiPlayer->ListenAudio(k->Key, 0);
      }
      else
      {
        Listen.KeyOn(Track, k->Key, k->Channel, k->Veloc, k->Length);
      }
    }

    wxClientDC dc(Canvas);
    Canvas->PrepareDC(dc);
    DrawEvent(&dc, pEvent, wxWHITE_BRUSH, 0);
    t->Kill(pEvent);
    t->Cleanup();
  }

  if (mpGuitarFrame)
  {
    mpGuitarFrame->Update();
//    mpGuitarFrame->Redraw();
  }

  // SN++ Veloc- oder Aftertouch-Editor updaten
  if (CtrlEdit)
    CtrlEdit->UpDate();
}


void JZPianoFrame::Paste(JZTrack *t, int Clock, int Pitch)
{
  if (PasteBuffer.nEvents == 0)
  {
    int len = SnapClocks() - 4;
    if (len < 2)
      len = 2;          
    tKeyOn* pEvent = new tKeyOn(0, 0, 64, 64, len);
    PasteBuffer.Put(pEvent);
  }
  // SN++
  if (nKeyOnEvents() > 1)
    Pitch = -1;        // don't change Pitch

  Song->NewUndoBuffer();
  tEventIterator Iterator(&PasteBuffer);
  JZEvent* pEvent = Iterator.First();
  if (pEvent)
  {
    // SN++
    JZEvent *a = pEvent;
    while (a)
    {
      if (pEvent->IsChnPressure())
      {
        a = Iterator.Next();
        pEvent = a;
      }
      else
      {
        a = NULL;
      }
    }

    int DeltaClock = Clock - pEvent->GetClock();
    int  DeltaPitch = 0;
    if (Pitch >= 0)
      DeltaPitch = Pitch - pEvent->GetPitch();
    while (pEvent)
    {
      JZEvent *c = pEvent->Copy();
      c->SetPitch(c->GetPitch() + DeltaPitch);
      c->SetClock(c->GetClock() + DeltaClock);
      if (t->ForceChannel && c->IsChannelEvent())
        c->IsChannelEvent()->Channel = t->Channel - 1;
      tKeyOn *k = c->IsKeyOn();
      if (k)
      {
        if (Track->GetAudioMode())
        {
          gpMidiPlayer->ListenAudio(k->Key, 0);
        }
        else
        {
          Listen.KeyOn(Track, k->Key, k->Channel, k->Veloc, k->Length);
        }
      }
      wxClientDC dc(Canvas);
      Canvas->PrepareDC(dc);
      DrawEvent(&dc, c, c->GetBrush(), 0, 1);
      t->Put(c);
      pEvent = Iterator.Next();
    }
    t->Cleanup();
    // SN++ Veloc- oder Aftertouch-Editor updaten
    if (CtrlEdit)
      CtrlEdit->UpDate();
  }
}

// ********************************************************************
// Mouse
// ********************************************************************

/*
 * left drag: Events markieren for Menu
 * left click:
 *   mit shift: copy
 *   ohne shift: cut
 *   auf Leer: paste
 * left click + ctrl:
 *   Note-Dialog
 * right drag:
 *   linke haelfte  : verschieben
 *   rechte halefte : Laenge einstellen
 * right click:
 *   Focus TrackWin
 *
 * PianoRoll:
 *   Left: midiout
 */

// ------------------------------------------------------------
// tMousePlay - Click in pianoroll
// -----------------------------------------------------------

class tMousePlay : public tMouseAction
{
    int Pitch, Veloc, Channel;
    JZPianoFrame *Win;
  public:
    tMousePlay(JZPianoFrame *win, wxMouseEvent& Event);
    int ProcessEvent(wxMouseEvent& Event);
};

tMousePlay::tMousePlay(JZPianoFrame *win, wxMouseEvent& Event)
{
  Win = win;

  Pitch = 0;
  Channel = Win->Track->Channel ? Win->Track->Channel - 1 : 0;
  ProcessEvent(Event);
}


int tMousePlay::ProcessEvent(wxMouseEvent& Event)
{
  int x, y;

  int OldPitch = Pitch;
  Win->LogicalMousePosition(Event, &x, &y);

  if (Event.LeftDown())
  {
    Pitch = Win->y2Pitch(y);
    Veloc = 64;
  }
  else if (Event.MiddleDown())
  {
    Pitch = Win->y2Pitch(y);
    Veloc = 80;
  }
  else if (Event.RightDown())
  {
    Pitch = Win->y2Pitch(y);
    Veloc = 110;
  }
  else if (Event.ButtonUp())
  {
    Pitch = 0;
  }
  else if (Event.Dragging())
  {
    Pitch = Win->y2Pitch(y);
  }
  else
  {
    return 0;
  }

  if (Win->Track->GetAudioMode())
  {
    if (Pitch && Pitch != OldPitch)
      gpMidiPlayer->ListenAudio(Pitch, 0);
  }
  else
  {
    if (OldPitch && OldPitch != Pitch)
    {
      tKeyOff of(0, Channel, OldPitch);
      gpMidiPlayer->OutNow(Win->Track, &of);
      OldPitch = 0;
    }

    if (Pitch && Pitch != OldPitch)
    {
      tKeyOn on(0, Channel, Pitch, Veloc);
      gpMidiPlayer->OutNow(Win->Track, &on);
      OldPitch = 0;
    }
  }

  if (!Pitch)
  {
    Win->MouseAction = 0;
    delete this;
    return 1;        // done
  }
  return 0;
}


void JZPianoFrame::MousePiano(wxMouseEvent& Event)
{
  if (Event.ButtonDown())
  {
    MouseAction = new tMousePlay(this, Event);
  }
}


int JZPianoFrame::Channel()
{
  return Track->Channel ? Track->Channel - 1 : 0;
}

// -------------------------------------------------------------------------
// tKeyLengthDragger
// -------------------------------------------------------------------------

class tKeyLengthDragger : public tMouseAction
{
    tKeyOn    *KeyOn;
    tKeyOn    *Copy;
    JZPianoFrame *Win;
    JZTrack    *Track;
    wxDC      *dc;

  public:
    tKeyLengthDragger(tKeyOn *k, JZPianoFrame *w);
    int Dragging(wxMouseEvent& Event);
    int ButtonUp(wxMouseEvent& Event);
    int Event(wxMouseEvent& Event);
};


tKeyLengthDragger::tKeyLengthDragger(tKeyOn *k, JZPianoFrame *w)
{
  KeyOn = k;
  Copy  = k->Copy() -> IsKeyOn();
  Win   = w;

  // SN++ BUG FIX: undo/redo
  Win->Song->NewUndoBuffer();
  //
  wxClientDC dc(Win->Canvas);
  Win->Canvas->PrepareDC(dc); //to translate scrolled coordinates
  Win->DrawEvent(&dc, Copy, wxWHITE_BRUSH, 0);
  Win->DrawEvent(&dc, Copy, Copy->GetBrush(), 1, 1);
}

int tKeyLengthDragger::Event(wxMouseEvent& Event)
{
  if (Event.Dragging())
  {
    return Dragging(Event);
  }
  else if (Event.ButtonUp())
  {
    return ButtonUp(Event);
  }
  return 0;
}

int tKeyLengthDragger::Dragging(wxMouseEvent& Event)
{
  int fx, fy;

  wxClientDC dc(Win->Canvas);
  Win->Canvas->PrepareDC(dc); //to translate scrolled coordinates
  Win->DrawEvent(&dc, Copy, Copy->GetBrush(), 1, 1);
  Win->LogicalMousePosition(Event, &fx, &fy);
  int Clock = Win->x2Clock(fx);
  int  Length = Clock - Copy->GetClock();
  if (Length <= 0)
  {
    Length = 1;
  }
  Copy->Length = Length;

  Win->DrawEvent(&dc, Copy, Copy->GetBrush(), 1, 1);
  return 0;
}

int tKeyLengthDragger::ButtonUp(wxMouseEvent& Event)
{
  // PAT - Since we repaint below, these calls are basically redundant.
  //wxClientDC dc(Win->Canvas);
  //Win->Canvas->PrepareDC(dc);
  //Win->DrawEvent(&dc, Copy, Copy->GetBrush(), 1, 1);
  //Win->DrawEvent(&dc, Copy, Copy->GetBrush(), 0, 1);

  // SN++ Key_Aftertouch
  if (Copy->Length < KeyOn->Length) {
    int key,channel;
    tEventIterator iter(Win->Track);
    tKeyPressure *a;
    key = Copy->Key;
    channel = Copy->Channel;
    JZEvent* pEvent = iter.Range(
      Copy->GetClock() + Copy->Length,
      Copy->GetClock() + KeyOn->Length);
    while (pEvent)
    {
      a = pEvent->IsKeyPressure();
      if (a)
      {
        if (a->Key == key && a->Channel == channel)
        {
          Win->Track->Kill(pEvent);
        }
      }
      pEvent = iter.Next();
    }
  }
  //

  Win->Track->Kill(KeyOn);
  Win->Track->Put(Copy);
  Win->Track->Cleanup();  Win->MouseAction = 0;

  // SN++ Veloc- oder Aftertouch-Editor updaten
  if (Win->CtrlEdit)
           Win->CtrlEdit->UpDate();


  // allways repaint
  Win->Redraw();

  delete this;
  return 0;
}


// -------------------------------------------------------------------------
// tPlayTrackLengthDragger JAVE this is just copied from tKeyLengthDragger,
// the need to be inherited somehow
// -------------------------------------------------------------------------

class tPlayTrackLengthDragger : public tMouseAction
{
    tPlayTrack    *KeyOn;
    tPlayTrack    *Copy;
    JZPianoFrame *Win;
    JZTrack    *Track;
    wxDC      *dc;

  public:
    tPlayTrackLengthDragger(tPlayTrack *k, JZPianoFrame *w);
    int Dragging(wxMouseEvent& Event);
    int ButtonUp(wxMouseEvent& Event);
    int Event(wxMouseEvent& Event);
};


tPlayTrackLengthDragger::tPlayTrackLengthDragger(tPlayTrack *k, JZPianoFrame *w)
{
  KeyOn = k;
  Copy  = k->Copy() -> IsPlayTrack();
  Win   = w;

  // SN++ BUG FIX: undo/redo
  Win->Song->NewUndoBuffer();
  //
  wxClientDC dc(Win->Canvas);
  Win->Canvas->PrepareDC(dc);
  Win->DrawEvent(&dc, Copy, wxWHITE_BRUSH, 0);
  Win->DrawEvent(&dc, Copy, Copy->GetBrush(), 1, 1);
}

int tPlayTrackLengthDragger::Event(wxMouseEvent& Event)
{
  if (Event.Dragging())
  {
    return Dragging(Event);
  }
  else if (Event.ButtonUp())
  {
    return ButtonUp(Event);
  }
  return 0;
}

int tPlayTrackLengthDragger::Dragging(wxMouseEvent& Event)
{
  int fx, fy;
  wxClientDC dc(Win->Canvas);
  Win->Canvas->PrepareDC(dc);
  Win->DrawEvent(&dc, Copy, Copy->GetBrush(), 1, 1);
  Win->LogicalMousePosition(Event, &fx, &fy);
  int Clock = Win->x2Clock(fx);
  int  Length = Clock - Copy->GetClock();
  if (Length <= 0)
    Length = 1;
  Copy->eventlength = Length; 

  Win->DrawEvent(&dc, Copy, Copy->GetBrush(), 1, 1);
  return 0;
}

int tPlayTrackLengthDragger::ButtonUp(wxMouseEvent& Event)
{
  wxClientDC dc(Win->Canvas);
  Win->Canvas->PrepareDC(dc);
  Win->DrawEvent(&dc, Copy, Copy->GetBrush(), 1, 1);
  Win->DrawEvent(&dc, Copy, Copy->GetBrush(), 0, 1);

  Win->Track->Kill(KeyOn);
  Win->Track->Put(Copy);
  Win->Track->Cleanup();  Win->MouseAction = 0;

  // SN++ Veloc- oder Aftertouch-Editor updaten
  if (Win->CtrlEdit)
           Win->CtrlEdit->UpDate();


  // allways repaint
  Win->Redraw();

  delete this;
  return 0;
}



// --------------------------------------------------------------------
// VelocCounter
// --------------------------------------------------------------------

class tVelocCounter : public tMouseCounter
{
    JZPianoFrame *Win;
    tKeyOn    *KeyOn;
  public:
    int Event(wxMouseEvent& Event);
    tVelocCounter(JZPianoFrame *w, JZRectangle* r, tKeyOn* pEvent)
      : tMouseCounter(w, r, pEvent->Veloc, 1, 127)
    {
      Win = w;
      KeyOn = pEvent;

      // SN++ BUG FIX: undo/redo
      Win->Song->NewUndoBuffer();
      //
      wxClientDC dc(Win->Canvas);
      dc.SetFont(*(Win->mpFixedFont));
    }
};



int tVelocCounter::Event(wxMouseEvent& Event)
{
  if (tMouseCounter::Event(Event))
  {
    tKeyOn *Copy = (tKeyOn *)KeyOn->Copy();
    Copy->Veloc = Value;
    Win->Track->Kill(KeyOn);
    Win->Track->Put(Copy);
    Win->Track->Cleanup();

    wxClientDC dc(Win->Canvas);
    Win->Canvas->PrepareDC(dc);
    Win->DrawEvent(&dc, Copy, Copy->GetBrush(), 0, 1);

    if (Win->CtrlEdit)
      Win->CtrlEdit->UpDate();

    Win->MouseAction = 0;

    dc.SetFont(*(Win->mpFont));
    delete this;
  }
  return 0;
}




// --------------------------------------------------------------------


void JZPianoFrame::MouseCutPaste(wxMouseEvent& Event, bool cut)
{

  //converts physical coords to logical(scrolled) coords
  wxClientDC dc(Canvas);
  Canvas->PrepareDC(dc);
  wxPoint point = Event.GetLogicalPosition(dc);

  int x = point.x;
  int y = point.y;

  int Clock = x2Clock(x);
  int Pitch = y2Pitch(y);
  JZEvent *m = FindEvent(Track, Clock, Pitch);
  if (m)
  {
    Copy(Track, m, cut);
  }
  else
  {
    Paste(Track, SnapClock(Clock), Pitch);
  }

  // allways redraw
  Redraw();
}


void JZPianoFrame::MouseEvents(wxMouseEvent& Event)
{
  int action = MouseEvnt.Action(Event);

  if (action)
  {
    int x, y;
    LogicalMousePosition(Event, &x, &y);

    int Clock = x2Clock(x);
    int Pitch = y2Pitch(y);
    JZEvent *m = FindEvent(Track, Clock, Pitch);
    tKeyOn *k = 0;
    tPlayTrack *p = 0;
    if (m){ //both these events are drag length
      k = m->IsKeyOn();
      p = m->IsPlayTrack();
    }
    switch (action)
    {
      case MA_CUTPASTE:
        MouseCutPaste(Event, 1);
        break;

      case MA_COPY:
        MouseCutPaste(Event, 0);
        break;

      case MA_LENGTH :
        if (k)
        {
          if (!Track->GetAudioMode())
            MouseAction = new tKeyLengthDragger(k, this);
        }
        else
        {
          if(p)
          {
            MouseAction = new tPlayTrackLengthDragger(p, this);
          }
          else if (VisibleAllTracks)
          {
            // event not found, maybe change to another Track
            int i;
            for (i = 0; i < Song->nTracks; i++)
            {
              JZTrack *t = Song->GetTrack(i);
              if (IsVisible(t) && FindEvent(t, Clock, Pitch))
              {
                NewPosition(i, -1L);
                break;
              }
            }
          }
        }
        break;


      case MA_DIALOG:
        EventDialog(m, this, Track, Clock, Track->Channel - 1, Pitch);
        break;


      case MA_LISTEN:
        MousePiano(Event);
        break;

      case MA_SELECT:
      case MA_CONTSEL:
        OnEventWinMouseEvent(Event);
        break;

      case MA_VELOCITY:
        if (k)
        {
          JZRectangle r;
          r.x = CanvasX + LittleBit;
          r.y = CanvasY;
          r.SetWidth(wPiano - 2 * LittleBit);
          r.SetHeight(mTopInfoHeight);

          tVelocCounter *VelocCounter = new tVelocCounter(this, &r, k);
          VelocCounter->Event(Event);
          MouseAction = VelocCounter;
        }
        break;

    }
  }
}

// ------------------------------------------------------------------------
// dispatch Mouseevent
// ------------------------------------------------------------------------

//   Indicate which key on the pianoroll that the mouse is hovering over by
// highlighting it.  This function is bad because it draws directly in the dc,
// rather it should invalidate and let OnDraw do the actual painting.
// Currently the code doesn't work because it doesn't care about scrolling
// (because I get the dc the wrong way).
void JZPianoFrame::ShowPitch(int pitch)
{
  // This is the current position of the mouse, MouseLine is the last position
  int line = y2Line(Pitch2y(pitch));
  if (line >= FromLine && line != MouseLine)
  {
    wxClientDC dc(Canvas);
    Canvas->PrepareDC(dc); //to translate scrolled coordinates
    dc.SetLogicalFunction(wxXOR);

    //dc.SetBrush(wxBLACK_BRUSH);
    dc.SetBrush(*wxBLUE_BRUSH);
    if (MouseLine >= 0) 
    {
      // Erase the previous highlight.
      dc.DrawRectangle(xPiano, Line2y(MouseLine) + LittleBit, wPiano, mTrackHeight - 2 * LittleBit);
    }
    MouseLine = line;

    // Draw the new position.
    dc.DrawRectangle(xPiano, Line2y(MouseLine) + LittleBit, wPiano, mTrackHeight - 2*LittleBit);

    dc.SetLogicalFunction(wxCOPY);
  }
}


int JZPianoFrame::OnMouseEvent(wxMouseEvent& Event)
{
  if (Event.Moving() && !Event.Dragging() && !MouseAction)
  {
    int fx, fy;
    LogicalMousePosition(Event, &fx, &fy);
    int pitch = y2Pitch(fy);
    ShowPitch(pitch);
    if (mpGuitarFrame)
      mpGuitarFrame->ShowPitch(pitch);
  }

  // dispatch

  if (!MouseAction)
  {
    int x, y;
    LogicalMousePosition(Event, &x, &y);

    if (y > mEventsY)        // click in event area?
    {
      if (xPiano < x && x < xPiano + wPiano)
      {
        MousePiano(Event);
      }
      else if (mEventsX < x && x < mEventsX + mEventsWidth)
      {
        MouseEvents(Event);
      }
      else
      {
        OnEventWinMouseEvent(Event);
      }
    }
    else if (x > mEventsX)
    {
      // click in top line
      int action = MousePlay.Action(Event);

      if (action)
      {
        if (!gpMidiPlayer->Playing)
        {
          int Clock, LoopClock;
          if (action == MA_CYCLE)
          {
            if (SnapSel->Selected)
            {
              Clock = mpFilter->FromClock;
              LoopClock = mpFilter->ToClock;
            }
            else
            {
              Clock = x2BarClock(x, 0);
              LoopClock = x2BarClock(x, 4);
            }
          }
          else
          {
            Clock = SnapClock(x2Clock(x));
            LoopClock = 0;
          }
          gpMidiPlayer->SetRecordInfo(0);
          gpMidiPlayer->StartPlay(Clock, LoopClock);
        }
        else
        {
          // Stop Record/Play
          gpMidiPlayer->StopPlay();
        }
      }
    }
  }
  else
  {
    OnEventWinMouseEvent(Event);
  }

  return 0;
}


bool JZPianoFrame::OnKeyEvent(wxKeyEvent& Event)
{
  if (Event.ControlDown())
  {
    switch (Event.GetKeyCode())
    {
      case 'Z':
        OnMenuCommand(MEN_UNDO);
        return true;
      case 'Y':
        OnMenuCommand(MEN_REDO);
        return true;
      case 'X':
        OnMenuCommand(MEN_CUT);
        return true;
      case 'C':
      case WXK_INSERT:
        OnMenuCommand(MEN_COPY);
        return true;
    }
  }
  else if (Event.ShiftDown())
  {
    switch (Event.GetKeyCode())
    {
      case WXK_UP:
        if (TrackNr > 0)
        {
          TrackNr --;
          NewPosition(TrackNr, -1L);
        }
        return true;
      case WXK_DOWN:
        if (TrackNr < Song->nTracks - 1)
        {
          TrackNr ++;
          NewPosition(TrackNr, -1L);
        }
        return true;
    }
  }
  else
  {
    switch(Event.GetKeyCode())
    {
      case WXK_DELETE:
        OnMenuCommand(MEN_ERASE);
        return true;
    }
  }

  return false;
}

// ------------------------------------------------------------------------
// Snapper
// ------------------------------------------------------------------------


void JZPianoFrame::SnapSelStop(wxMouseEvent& Event)
{
  if (SnapSel->Selected)
  {

    int fr = y2Pitch((SnapSel->r.y + SnapSel->r.height - 1));
    int to = y2Pitch(SnapSel->r.y + 1);


    mpFilter->FltEvents[FltKeyOn].Selected = VisibleKeyOn;
    mpFilter->FltEvents[FltKeyOn].FromValue = fr;
    mpFilter->FltEvents[FltKeyOn].ToValue   = to;

    mpFilter->FltEvents[FltPitch].Selected = VisiblePitch;
    mpFilter->FltEvents[FltPitch].FromValue = (fr << 7) - 8192;
    mpFilter->FltEvents[FltPitch].ToValue   = ((to + 1) << 7) - 8192;

    mpFilter->FltEvents[FltControl].Selected = VisibleController;
    mpFilter->FltEvents[FltControl].FromValue = fr;
    mpFilter->FltEvents[FltControl].ToValue   = to;

    mpFilter->FltEvents[FltProgram].Selected = VisibleProgram;
    mpFilter->FltEvents[FltProgram].FromValue = fr;
    mpFilter->FltEvents[FltProgram].ToValue   = to;

    mpFilter->FltEvents[FltTempo].Selected = VisibleTempo;
    mpFilter->FltEvents[FltTempo].FromValue = fr;
    mpFilter->FltEvents[FltTempo].ToValue   = to;

    mpFilter->FltEvents[FltSysEx].Selected = VisibleSysex;
    mpFilter->FltEvents[FltSysEx].FromValue = fr;
    mpFilter->FltEvents[FltSysEx].ToValue   = to;

    // SN++ Aftertouch (gehoeren to KeyOn Events).
    mpFilter->FltEvents[FltKeyPressure].Selected  = VisibleKeyOn;
    mpFilter->FltEvents[FltKeyPressure].FromValue = fr;
    mpFilter->FltEvents[FltKeyPressure].ToValue   = to;

    // SN++ Channel Aftertouch
    mpFilter->FltEvents[FltChnPressure].Selected  = VisibleMono;
    mpFilter->FltEvents[FltChnPressure].FromValue = fr;
    mpFilter->FltEvents[FltChnPressure].ToValue   = to;


    mpFilter->FromTrack = TrackNr;
    mpFilter->ToTrack   = TrackNr;
    mpFilter->FromClock = SnapClock(x2Clock(SnapSel->r.x + 1));
    mpFilter->ToClock   = SnapClock(x2Clock((SnapSel->r.x + SnapSel->r.width + 1)));
  }

  // SN++ Veloc- oder Aftertouch-Editor updaten
  if (CtrlEdit)
           CtrlEdit->UpDate();
}



void JZPianoFrame::SnapSelStart(wxMouseEvent &)
{
  nSnaps = 0;
  int clk = SnapClock(FromClock, 0);
  int qnt = SnapClocks();
  while (clk <= ToClock && nSnaps < MaxSnaps)
  {
    xSnaps[nSnaps++] = Clock2x(clk);
    clk += qnt;
  }
  if (nSnaps < MaxSnaps)
  {
    SnapSel->SetXSnap(nSnaps, xSnaps);
  }
  else
  {
    SnapSel->SetXSnap(0,0,0);
  }
  SnapSel->SetYSnap(FromLine * mTrackHeight + mTopInfoHeight, mEventsY + mEventsHeight, mTrackHeight);
}


int JZPianoFrame::SnapClock(int clk, int up)
{
  int qnt = SnapClocks();
  clk -= (clk % qnt);
  if (up)
    clk += qnt;
  return clk;
}


// **************************************************************************
// Snap
// **************************************************************************


void JZPianoFrame::SnapDlg(wxCommandEvent& Event)
{
  tSnapDlg * dlg = new tSnapDlg(this, &SnapDenomiator);
  dlg->Create();
}


void JZPianoFrame::PressRadio(int id)
{
  static const int ids[] = {
      //MEN_SNAP_8,
      //MEN_SNAP_8D,
      //MEN_SNAP_16,
      //MEN_SNAP_16D,
      MEN_MSELECT,
      MEN_MLENGTH,
      MEN_MDIALOG,
      MEN_MCUTPASTE,
      0
  };
  for (const int *pid = ids; *pid; pid++)
    if (*pid != id && mpToolBar->GetToolState(*pid))
      mpToolBar->ToggleTool(*pid, FALSE);
#ifndef wx_xt
  if (id > 0 && !mpToolBar->GetToolState(id))
    mpToolBar->ToggleTool(id, TRUE);
#endif
}



void JZPianoFrame::SetSnapDenom(int value)
{
  const int N = 4;
  const struct { int id; int val; } tab[N] =
  {
    { MEN_SNAP_8,    8 },
    { MEN_SNAP_8D,  12 },
    { MEN_SNAP_16,  16 },
    { MEN_SNAP_16D, 24 },
  };

  int i, id = 0;
  // find the button
  for (i = 0; i < N; i++)
  {
    if (tab[i].val == value)
      id = tab[i].id;
  }

  // toggle toolbar buttons
  for (i = 0; i < N; i++)
    if (tab[i].id != id && mpToolBar->GetToolState(tab[i].id))
      mpToolBar->ToggleTool(tab[i].id, FALSE);
#ifndef wx_xt
  if (id > 0 && !mpToolBar->GetToolState(id))
    mpToolBar->ToggleTool(id, TRUE);
#endif

  SnapDenomiator = value;
  //MouseEvnt.SetLeftAction(MA_CUTPASTE);
}



void JZPianoFrame::SetVisibleAllTracks(bool value)
{
  mpToolBar->ToggleTool(MEN_VIS_ALL_TRK, value);
  VisibleAllTracks = value;
  Redraw();
}

///////////////////////////////////////////////////////////////////////////////
// Function slurped from JZEventFrame
///////////////////////////////////////////////////////////////////////////////

void JZPianoFrame::NewPlayPosition(int Clock)
{
  int scroll_clock = (FromClock + 5 * ToClock) / 6L;

  if (
    !SnapSel->Active &&
    ((Clock > scroll_clock) || (Clock < FromClock)) && (Clock >= 0L))
  {
    // Avoid permenent redraws when end of scroll range is reached
    if (Clock > FromClock && ToClock >= Song->MaxQuarters * Song->TicksPerQuarter)
    {
      return;
    }

    int x = Clock2x(Clock);
    Canvas->SetScrollPosition(x - mLeftInfoWidth, CanvasY);
  }

  if (!SnapSel->Active)        // sets clipping
  {
    if (PlayClock != Clock)
    {
 //     int oldplayclock=PlayClock;
//      PlayClock = Clock;
//        wxRect invalidateRect;
//        invalidateRect.x=Clock2x(oldplayclock)-1;
//        invalidateRect.y=CanvasY;
//        invalidateRect.width=3;
//        invalidateRect.height= 100000000;
//       //       DrawPlayPosition();
//        Canvas->Refresh(TRUE,&invalidateRect);

//              invalidateRect.x=Clock2x(PlayClock)-1;
//       Canvas->Refresh(TRUE,&invalidateRect);
      //       DrawPlayPosition();

      Canvas->Refresh();


    }
  }
}

/** draw the "play position", by placing a vertical line where the "play clock" is */
void JZPianoFrame::DrawPlayPosition(wxDC* dc)
{
   if (!SnapSel->Active && PlayClock >= FromClock && PlayClock < ToClock)
   {
    //    wxDC* dc=new wxClientDC(this);
  //    dc->SetLogicalFunction(wxXOR);
    dc->SetBrush(*wxBLACK_BRUSH);
    dc->SetPen(*wxBLACK_PEN);
    int x = Clock2x(PlayClock);
    //dc->DrawRectangle(x, CanvasY, 2*LittleBit, mTopInfoHeight);
    dc->DrawLine(x,  CanvasY,x,  mEventsY + mEventsHeight); //draw a line, 2 pixwels wide
    dc->DrawLine(x+1,CanvasY,x+1,mEventsY + mEventsHeight);
    dc->SetLogicalFunction(wxCOPY);
      }
}

void JZPianoFrame::Redraw()
{
  Canvas->Refresh();
}

int JZPianoFrame::Clock2x(int clk)
{
  return mEventsX + (clk - FromClock) / ClocksPerPixel;
}

int JZPianoFrame::Line2y(int Line)
{
  return Line * mTrackHeight + mTopInfoHeight;
}

void JZPianoFrame::ActSettingsDialog(wxCommandEvent& Event)
{
  jppResourceDialog dialog(this, "windowSettings");
  
  dialog.Attach("use_colours", &UseColors);
  dialog.Attach("font_size", &FontSize, PianoFontSizes);

  if(dialog.ShowModal() == wxID_OK)
  {
    Setup();
    Canvas->SetScrollRanges();
    Redraw();
  }
}

/////JAVE 
/** this is a test to see how to implement a dialog with patricks system
 it replaces tMidiDelayDlg, which isnt necesarily a good idea
*/
void JZPianoFrame::ActMidiDelayDialog(wxCommandEvent& Event)
{
  if (!EventsSelected())
    return;
  int scale = 50; //in percent
  int clockDelay = 10;
  int repeat = 6;

  jppResourceDialog dialog(this, "midiDelay");
  
  dialog.Attach("scale", &scale);
  dialog.Attach("clockDelay", &clockDelay);
  dialog.Attach("repeat", &repeat);

  if(dialog.ShowModal() == wxID_OK) {
    //execute the command
    tCmdMidiDelay cmd(mpFilter, scale/100.0,clockDelay,repeat);
    cmd.Execute();
    Canvas->SetScrollRanges();
    Redraw();
  }
}
////////////
/////JAVE 
/**call the sequence lenght command */
void JZPianoFrame::ActSequenceLengthDialog(wxCommandEvent& Event)
{
  if (!EventsSelected())
    return;
  int scale = 100; //in percent

  jppResourceDialog dialog(this, "sequenceLength");
  
  dialog.Attach("scale", &scale);
  
  if(dialog.ShowModal() == wxID_OK) {
    //execute the command
    tCmdSeqLength cmd(mpFilter, (1.0*scale)/100.0);
    cmd.Execute();
    Canvas->SetScrollRanges();
    Redraw();
  }
}
////////////////


void JZPianoFrame::ActVelocityDialog(wxCommandEvent& Event)
{
  int FromValue = 64;
  int ToValue = 0;
  int modes[] =
{
  8,  // set
  12,  // add
  16,  // subtract
  -1, // End of list
};

  int Mode = modes[0];


  if (!EventsSelected())
    return;
  jppResourceDialog dialog(this, "velocity");
   dialog.Attach("start",&FromValue);
   dialog.Attach("stop",&ToValue);
   dialog.Attach("mode",&Mode,modes);

  if(dialog.ShowModal() == wxID_OK) {
    //execute the command
    tCmdVelocity cmd(mpFilter, FromValue, ToValue, Mode);
    cmd.Execute();
    Canvas->SetScrollRanges();
    Redraw();
  }



}



int JZPianoFrame::EventsSelected(const char *msg)
{
  if (!SnapSel->Selected)
  {
    if (msg == 0)
      msg = "please select some events first";
    wxMessageBox((char *)msg, "Error", wxOK);
    return 0;
  }
  return 1;
}

void JZPianoFrame::ZoomIn()
{

  if (ClocksPerPixel >= 2) {
    ClocksPerPixel /= 2;
    int x = CanvasX * 2;
    int y = CanvasY;

    OnEventWinPaintSub(x, y);
    Canvas->SetScrollRanges();
    Canvas->SetScrollPosition(x, y);
    if (x == 0)
      Redraw();

  }
}

void JZPianoFrame::ZoomOut()
{
  if (ClocksPerPixel <= 120)
  {
    ClocksPerPixel *= 2;
    int x = CanvasX / 2;
    int y = CanvasY;

    OnEventWinPaintSub(x, y);
    Canvas->SetScrollRanges();
    Canvas->SetScrollPosition(x, y);
    if (x == 0)
      Redraw();
  }
}

int JZPianoFrame::x2Clock(int x)
{
  return (x - mEventsX) * ClocksPerPixel + FromClock;
}

int JZPianoFrame::y2Line(int y, int up)
{
  if (up)
  {
    y += mTrackHeight;
  }
  y -= mTopInfoHeight;
  return y / mTrackHeight;
}

int JZPianoFrame::x2BarClock(int x, int next)
{
  int clk = x2Clock(x);
  JZBarInfo b(Song);
  b.SetClock(clk);
  while (next--)
    b.Next();
  return b.Clock;
}

int JZPianoFrame::OnEventWinMouseEvent(wxMouseEvent& Event)
{
  if (!MouseAction)
  {
    // create SnapSel?

    int x;
    int y;
    LogicalMousePosition(Event, &x, &y);
    if (mEventsX < x && x < mEventsX + mEventsWidth && mEventsY < y && y < mEventsY + mEventsHeight)
    {
      if (Event.LeftDown())
      {
        {
          SnapSelStart(Event);

          if (SnapSel->Selected){
            Refresh(); //redraw the whole window instead(inefficient, we should rather invalidate a rect)
          }
          SnapSel->Event(Event);
          MouseAction = SnapSel;
        }
      }
    }
  }
  else
  {
    // MouseAction active

    if (MouseAction->Event(Event))
    {
      // MouseAction finished

      if (MouseAction == SnapSel)
      {
        SnapSelStop(Event);
        Redraw(); //ineficcient, invalidate rect first instead
        MouseAction = 0;
        return 1;
      }

      MouseAction = 0;
    }
  }
  return 0;
}

void JZPianoFrame::OnEventWinPaintSub(int x, int y)
{
  CanvasX = x;
  CanvasY = y;
  int xc, yc;
  GetClientSize(&xc, &yc);
  CanvasW = xc;
  CanvasH = yc;

  mEventsX = CanvasX + mLeftInfoWidth;
  mEventsY = CanvasY + mTopInfoHeight;
  mEventsWidth = CanvasW - mLeftInfoWidth;
  mEventsHeight = CanvasH - mTopInfoHeight;

  FromLine = CanvasY / mTrackHeight; 
  ToLine   = (CanvasY + CanvasH - mTopInfoHeight) / mTrackHeight;
  FromClock = CanvasX * ClocksPerPixel;
  ToClock = x2Clock(CanvasX + CanvasW);
}

int JZPianoFrame::y2yLine(int y, int up)
{
  if (up)
  {
    y += mTrackHeight;
  }
  y -= mTopInfoHeight;
  y -= y % mTrackHeight;
  y += mTopInfoHeight;
  return y;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZPianoFrame::GetVirtualEventSize(int& Width, int& Height)
{
  int TotalClockTics = Song->MaxQuarters * Song->TicksPerQuarter;
  Width = TotalClockTics / ClocksPerPixel + mLeftInfoWidth;
  Height = 127 * mTrackHeight + mTopInfoHeight;
}

bool JZPianoFrame::OnCharHook(wxKeyEvent& Event)
{
  return OnKeyEvent(Event);
}























void JZPianoFrame::LogicalMousePosition(wxMouseEvent& Event, int* x, int* y)
{
  Event.GetPosition(x, y);
  *x += CanvasX;
  *y += CanvasY;
}


///////////////////////////////////////////////////////////////////////////////
// The rest of this file is "Patrick Approved."  Take that how you want. :)  //
///////////////////////////////////////////////////////////////////////////////

/* Generate some colours to represent note velocity.  The current settings use
   dark blue for quiet and bright red for loud. */
void JZPianoFrame::InitColors()
{
  int i, c;
  for (i = 0; i < NUM_COLORS; i++) {
    c = 256 * i / NUM_COLORS; 
    color_brush[i].SetColour(c, 0, 127-c/2);
    color_brush[i].SetStyle(wxSOLID);
  }
}

/* Draws the a 3D button with text in it.  Used to draw the little area in the
   top left of the window. */
void JZPianoFrame::LineText(
  wxDC* dc,
  int x,
  int y,
  int w,
  int h,
  wxString str,
  bool down)
{
  dc->SetBrush(*wxLIGHT_GREY_BRUSH); // Fill
  dc->SetPen(*wxLIGHT_GREY_PEN);     // Outline
  dc->DrawRectangle(x, y, w, h);

  x += 1;
  y += 1;
  w -= 2;
  h -= 2;

  // Draw the top and left lines of the 3D button.
  if (down)
  {
    dc->SetPen(*wxBLACK_PEN);
  }
  else
  {
    dc->SetPen(*wxWHITE_PEN);
  }

  dc->DrawLine(x, y, x+w, y);
  dc->DrawLine(x, y, x, y+h);

  // Draw the bottom and right lines of the 3D button.
  if (down)
  {
    dc->SetPen(*wxWHITE_PEN);
  }
  else
  {
    dc->SetPen(*wxBLACK_PEN);
  }

  dc->DrawLine(x+w, y, x+w, y+h);
  dc->DrawLine(x, y+h, x+w, y+h);

  // Print the message in the button.
  dc->DrawText(str, x + LittleBit, y + LittleBit);
}

/* This is an an event handler for tMouseCounter. */
void JZPianoFrame::ButtonLabelDisplay(const wxString& Text, bool IsButtonDown)
{
  wxClientDC Dc(Canvas);
  LineText(&Dc, 0, 0, wPiano, mTopInfoHeight, Text, IsButtonDown);
}


/* This section of the file consists of various action handlers in alphabetical
   order.  Most of these are called from menus or toolbar buttons.  If they
   are called in a different way, that will be noted. */

void JZPianoFrame::ActClose(wxCommandEvent& Event)
{
  Show(FALSE);
}

/* This event is generated when the window's close button is pressed. */
void JZPianoFrame::ActCloseEvent(wxCloseEvent& Event)
{
  // This strange code brought to you by the documentation for wxCloseEvent.
  if (Event.CanVeto())
  {
    Show(FALSE);
    Event.Veto();
  }
  else
  {
    Destroy();
  }
}

void JZPianoFrame::ActHelpMouse(wxCommandEvent& Event)
{
  wxMessageBox(mouse_help, "Mouse Help");
}
