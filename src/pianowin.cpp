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
#pragma hdrstop

#include "pianowin.h"
#include "song.h"
#include "mstdfile.h"
#include "filter.h"


#include "dialogs.h"



#include "harmony.h"
#include "command.h"
#include "player.h"
#include "wx/timer.h"
#include "jazz.h"
#include "ctrledit.h"
#include "guitar.h"

#include "harmonyp.h"
#include "hbanalyz.h"
#include "toolbar.h"

#ifdef wx_xt
#define wxbMessageBox wxMessageBox
#endif

// ************************************************************************
// Menubar
// ************************************************************************

#define MEN_SETTINGS	5
#define MEN_FILTER	6
#define MEN_SNAP	7
#define MEN_METERCH	8
#define MEN_HELP_MOUSE	9

#define MEN_COPY	10
#define MEN_SHIFT	11
#define MEN_QUANTIZE	12
#define MEN_UNDO	13
#define MEN_SETCHAN	14
#define MEN_TRANSP	15
#define MEN_VELOC	16
#define MEN_CUT		17
#define MEN_LERI	18
#define MEN_UPDN	19
#define MEN_LENGTH	20


#define MEN_ERASE	21
#define MEN_VISIBLE     22

#define MEN_CTRL_EDIT   23
#define MEN_CTRL_PITCH  24
#define MEN_CTRL_CONTR  25
#define MEN_CTRL_VELOC  26
#define MEN_CTRL_NONE   27
#define MEN_CTRL_MODUL  28

#define MEN_GUITAR	29
#define MEN_HELP_PWIN   30
#define MEN_CLEANUP     31

#define MEN_SNAP_8      32
#define MEN_SNAP_8D     33
#define MEN_SNAP_16     34
#define MEN_SNAP_16D    35
#define MEN_RESET       36
#define MEN_VIS_ALL_TRK 37
#define MEN_SEARCHREP   38
#define MEN_SHIFTL      39
#define MEN_SHIFTR      40

#define MEN_CLOSE	41
#define MEN_CTRL_TEMPO  42

#define MEN_MSELECT     43
#define MEN_MLENGTH     44
#define MEN_MDIALOG     45
#define MEN_MCUTPASTE   46
#define MEN_REDO        47
#define MEN_ZOOMIN      48
#define MEN_ZOOMOUT	49
// SN++
#define MEN_CTRL_POLY_AFTER 50
#define MEN_CTRL_CHANNEL_AFTER  51

#define MEN_SEQLENGTH	52
#define MEN_MIDIDELAY	53
#define MEN_CONVERT_TO_MODULATION	54

// Toolbar Icons

#ifdef wx_x
#include "../bitmaps/note8.xpm"
#include "../bitmaps/note83.xpm"
#include "../bitmaps/note16.xpm"
#include "../bitmaps/note163.xpm"
#include "../bitmaps/cut.xpm"
#include "../bitmaps/delete.xpm"
#include "../bitmaps/quantize.xpm"
#include "../bitmaps/evnts.xpm"
#include "../bitmaps/undo.xpm"
#include "../bitmaps/redo.xpm"
#include "../bitmaps/zoomin.xpm"
#include "../bitmaps/zoomout.xpm"
#include "../bitmaps/panic.xpm"
#include "../bitmaps/help.xpm"
#include "../bitmaps/shiftl.xpm"
#include "../bitmaps/shiftr.xpm"
#include "../bitmaps/select.xpm"
#include "../bitmaps/length.xpm"
#include "../bitmaps/dialog.xpm"
#include "../bitmaps/cutpaste.xpm"

static tToolDef tdefs[] = {
  { MEN_MSELECT,  	TRUE,  0, select_xpm , "select events"},
  { MEN_MLENGTH,  	TRUE,  0, length_xpm , "change length"},
  { MEN_MDIALOG,  	TRUE,  0, dialog_xpm , "event dialog"},
  { MEN_MCUTPASTE,  	TRUE,  1, cutpaste_xpm , "cut/paste events"},

  { MEN_SNAP_8,  	TRUE,  0, note8_xpm , "snap 1/8"},
  { MEN_SNAP_8D,  	TRUE,  0, note83_xpm , "snap 1/12"},
  { MEN_SNAP_16,  	TRUE,  0, note16_xpm , "snap 1/16"},
  { MEN_SNAP_16D,  	TRUE,  1, note163_xpm , "snap 1/24"},

  { MEN_CUT,   		FALSE, 0, cut_xpm  , "cut selection"},
  { MEN_ERASE, 		FALSE, 0, delete_xpm  , "delete selection"},
  { MEN_QUANTIZE, 	FALSE, 0, quantize_xpm  , "quantize selection"},
  { MEN_SHIFTL, 	FALSE, 0, shiftl_xpm  , "shift selection left"},
  { MEN_SHIFTR, 	FALSE, 0, shiftr_xpm  , "shift selection right"},
  { MEN_VIS_ALL_TRK, 	TRUE,  1, evnts_xpm  , "show events from all tracks"},

  { MEN_ZOOMIN,		FALSE, 0, tb_zoomin  , "zoom in"}, //havent changed this yet
  { MEN_ZOOMOUT,	FALSE, 0, zoomout_xpm  , "zoom out"},
  { MEN_UNDO,		FALSE, 0, undo_xpm  , "undo"},
  { MEN_REDO,		FALSE, 0, redo_xpm , "redo"},
  { MEN_RESET, 		FALSE, 0, panic_xpm , "all notes off"},
  { MEN_HELP_PWIN, 	FALSE, 0, help_xpm , "help"}
};

#else

static tToolDef tdefs[] = {
  { MEN_MSELECT,  	TRUE,  0, "select_xpm", "select events" },
  { MEN_MLENGTH,  	TRUE,  0, "length_xpm", "change length" },
  { MEN_MDIALOG,  	TRUE,  0, "dialog_xpm", "event dialog" },
  { MEN_MCUTPASTE,  	TRUE,  1, "cutpaste_xpm", "cut/paste events" },

  { MEN_SNAP_8,  	TRUE,  0, "note8_xpm", "snap 1/8" },
  { MEN_SNAP_8D,  	TRUE,  0, "note83_xpm", "snap 1/12" },
  { MEN_SNAP_16,  	TRUE,  0, "note16_xpm", "snap 1/16"},
  { MEN_SNAP_16D,  	TRUE,  1, "note163_xpm", "snap 1/24"},

  { MEN_CUT,   		FALSE, 0, "cut_xpm", "cut selection"  },
  { MEN_ERASE, 		FALSE, 0, "delete_xpm", "delete selection" },
  { MEN_QUANTIZE, 	FALSE, 0, "quantize_xpm", "quantize selection"  },
  { MEN_SHIFTL, 	FALSE, 0, "shiftl_xpm", "shift selection left" },
  { MEN_SHIFTR, 	FALSE, 0, "shiftr_xpm", "shift selection right"  },
  { MEN_VIS_ALL_TRK, 	TRUE,  1, "evnts_xpm", "show events from all tracks"  },

  { MEN_ZOOMIN,		FALSE, 0, "zoomin_xpm", "zoom in"  },
  { MEN_ZOOMOUT,	FALSE, 0, "zoomout_xpm", "zoom out" },
  { MEN_UNDO,		FALSE, 0, "undo_xpm", "undo"  },
  { MEN_REDO,		FALSE, 0, "redo_xpm" , "redo" },
  { MEN_RESET, 		FALSE, 0, "panic_xpm", "all notes off" },
  { MEN_HELP_PWIN, 	FALSE, 0, "help_xpm", "help" }
};

#endif


// positions for controller editor
#define CtrlY(h)	(3*(h)/4)
#define CtrlH(h)	((h)/4)

// mouse actions mapping

#define MA_PLAY		1
#define MA_CYCLE	2

#define MA_SELECT	3
#define MA_CONTSEL	4

#define MA_CUTPASTE	5
#define MA_LENGTH	6
#define MA_DIALOG	7
#define MA_LISTEN	8
#define MA_COPY		9
#define MA_VELOCITY    10


const int play_actions[12] = {
  // left	middle		right
  MA_PLAY,	MA_CYCLE,	0,		// plain
  MA_CYCLE,	0,		0,		// shift
  0,		0,		0,		// ctrl
  0,		0,		0		// shift+ctrl
};

const int evnt_2_actions[12] = {
  // left	middle		right
  MA_SELECT,	0,		MA_LENGTH,	// plain
  MA_CONTSEL,	0,		MA_LISTEN,	// shift
  MA_VELOCITY,	0,		MA_VELOCITY,	// ctrl
  0,		0,		0		// shift+ctrl
};

const int evnt_3_actions[12] = {
  // left	middle		right
  MA_SELECT,	MA_CUTPASTE,	MA_LENGTH,	// plain
  MA_CONTSEL,	MA_COPY,	MA_LISTEN,	// shift
  MA_VELOCITY,	MA_DIALOG,	MA_VELOCITY,	// ctrl
  MA_CUTPASTE,	0,		MA_COPY		// shift+ctrl
};

#if 0
const char mouse_2_help[] =
	"topline:\n"
	"  left: start/stop play\n"
	"    +shift: start/stop cycle play\n"
	"\n"
	"events:\n"
        "  left: select events\n"
        "    +shift: continue selection\n"
        "    +ctrl: cut/paste event\n"
        "  right: note length / change track\n"
        "    +shift: note dialog\n"
        "    +ctrl: play pitch\n";

#endif

const char mouse_help[] =
	"on topline:\n"
	"  left: start/stop play\n"
	"    +shift: start/stop cycle play\n"
	"  middle: same as left+shift\n"
	"on events:\n"
        "  left: depends on mode\n"
        "    +shift: continue selection\n"
        "    +ctrl: increase velocity\n"
        "    +ctrl+shift: cut/paste event\n"
        "  middle: cut/paste event\n"
        "    +shift: copy event\n"
        "    +ctrl: event dialog\n"
        "  right: edit note length / change track\n"
        "    +shift: play pitch\n"
        "    +ctrl: decrease velocity\n"
        "    +ctrl+shift: copy\n";

// -------------------------------------------------------------------------
// MousePiano
// -------------------------------------------------------------------------

class tListen : public wxTimer
{
  public:
    int Active;
    int Pitch, Channel;
    tListen::tListen() { Active = 0; }
    void KeyOn(tTrack *t, int Pitch, int Channel, int Veloc = 64, int Millisec = 100);
    void Notify();
  private:
    tTrack *track;
};

void tListen::KeyOn(tTrack *t, int pitch, int channel, int veloc, int milli)
{
  if (!Active)
  {
    Pitch = pitch;
    Channel = channel;
    tKeyOn k(0, Channel, pitch, veloc);
    Midi->OutNow(t, &k);
    Active = 1;
    Start(milli);
    track = t;
  }
}

void tListen::Notify()
{
  Stop();
  tKeyOff k(0, Channel, Pitch);
  Midi->OutNow(track, &k);
  Active = 0;
}

static tListen Listen;

// **************************************************************************
// Pianowin
// **************************************************************************

tPianoWin::tPianoWin(wxFrame *frame, char *title, tSong *song, int x, int y, int width, int height)
  : tEventWin(frame, title, song, x, y, width, height ),
    MousePlay(play_actions),
    //MouseEvnt(MouseButtons == 2 ? evnt_2_actions : evnt_3_actions)
    MouseEvnt(evnt_3_actions)
{
  int i;
  InitColors();
  
  tToolBar *hack= new tToolBar(this, tdefs, 20);

  tool_bar = hack->GetToolBar();

  tool_bar->ToggleTool(MEN_MSELECT, TRUE);
  MouseEvnt.SetLeftAction(MA_SELECT);

  ClocksPerPixel = 4;
#ifdef wx_msw
  FontSize = 6;
#else
  FontSize = 7;
#endif
  TrackNr = 0;
  Track = Song->GetTrack(TrackNr);
  SnapDenomiator = 16;
  tool_bar->ToggleTool(MEN_SNAP_16, TRUE);
  nSnaps = 0;

  for (i = 0; i < MaxTracks; i++)
    FromLines[i] = 64;

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
  // SN++
  VisibleMono = 0;

  MouseLine = -1;

  CtrlEdit  = 0;
  GuitarWin = 0;

}


tPianoWin::~tPianoWin()
{
  int i;
  delete CtrlEdit;
  delete GuitarWin;
  for (i = 0; i < NUM_COLORS; i++)
    delete color_brush[i];
}

// SN+ Colors
void tPianoWin::InitColors()
{
  int i, n = NUM_COLORS/2;
  for (i = 0; i < NUM_COLORS; i++)
    color_brush[i]  = new wxBrush("BLACK", wxSOLID);
#if 0
  for (i = 0; i < n; i++) {
    int c = 255 * i / n;
    color_brush[i]->SetColour  (c,   255,   0);
    color_brush[i+n]->SetColour(255, 255-c, 0);
  }
#else
  for (i = 0; i < NUM_COLORS; i++) {
    int c = 255 * i / NUM_COLORS;
    color_brush[i]->SetColour  (c, 255-c, 16);
  }
#endif
}

#ifndef __PORTING

bool tPianoWin::OnClose()
{
  Show(FALSE);
  return FALSE;
}
#endif // __PORTING


void tPianoWin::OnSize(wxSizeEvent& event)
{

  //have a look at trackwin::onsize, which works somewhat



  int cw, ch;
  GetClientSize(&cw, &ch);
  int tw = 0;
  int th = 0;
#ifndef __PORTING
  if (tool_bar)
    tool_bar->GetMaxSize(&tw, &th);
#endif // __PORTING

  if (Canvas && CtrlEdit) //CtrlEdit is the optional edit area for controller values, below the piano roll
  {
    int ctrl_w, ctrl_h;
    Canvas->SetSize(0, (int)th, cw, CtrlY(ch) - (int)th);
    Canvas->GetClientSize(&ctrl_w, &ctrl_h); // dont count Scrollbar??
    CtrlEdit->SetSize(wPiano, 0, CtrlY(ch), ctrl_w, CtrlH(ch));
  }
  else if (Canvas)
    Canvas->SetSize(0, (int)th, cw, ch - (int)th);
#ifndef __PORTING
  if (tool_bar)
    tool_bar->SetSize(0, 0, (int)cw, (int)th);
#endif // __PORTING



}


void tPianoWin::Setup()
{
  int x, y;

  tEventWin::Setup();

  wxDC* dc=new wxClientDC(Canvas);
  dc->SetFont(*FixedFont);
  dc->GetTextExtent("H", &x, &y);
  hTop = hFixedFont + 2 * LittleBit;

  dc->SetFont(*Font);
  dc->GetTextExtent("H", &x, &y);
  LittleBit = (int)(x/2);

  delete DrumFont;
  DrumFont = new wxFont(FontSize+3, wxSWISS, wxNORMAL, wxNORMAL);
  dc->SetFont(*DrumFont);
  dc->GetTextExtent("Low Conga mid 2 or so", &x, &y);
  wPiano = (int)x + LittleBit;

  wLeft = wPiano;
  delete dc;
}


void tPianoWin::NewPosition(int track, long clock)
{
  FromLines[TrackNr] = FromLine;

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
    long x = Clock2x(clock);
    Canvas->SetScrollPosition(x - wLeft, Line2y(FromLines[TrackNr]));
  }

// SN++ Ist geaendert. OnPaint zeichnet immer neu -> Bug Fix bei ZoomOut!
/*
  // OnPaint() redraws only if clock has changed
  if (CtrlEdit && track >= 0)
    CtrlEdit->ReInit(Track, FromClock, ClocksPerPixel);
*/
  Redraw();
}




void tPianoWin::CreateMenu()
{
  wxMenuBar *menu_bar = NULL;

  wxMenu *win_menu = new wxMenu;
  win_menu->Append(MEN_CLOSE, "&Close");

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
  setting_menu->Append(MEN_SETTINGS,  "&Window ...");
  setting_menu->Append(MEN_VISIBLE,   "&Events...");
  setting_menu->Append(MEN_SNAP,      "&Snap ...");
  setting_menu->Append(MEN_METERCH,   "&Meterchange ...");

  wxMenu *misc_menu = new wxMenu("",wxMENU_TEAROFF);
  misc_menu->Append(MEN_UNDO,   "&Undo");
  misc_menu->Append(MEN_REDO,   "&Redo");
  misc_menu->Append(MEN_CTRL_PITCH,	"Edit &Pitch");
  misc_menu->Append(MEN_CTRL_VELOC,	"Edit &Velocity");
  misc_menu->Append(MEN_CTRL_MODUL,	"Edit &Modulation");
// SN++
  misc_menu->Append(MEN_CTRL_POLY_AFTER, "Edit &Key Aftertouch");
  misc_menu->Append(MEN_CTRL_CHANNEL_AFTER,  "Edit &Chn Aftertouch");
//
  misc_menu->Append(MEN_CTRL_CONTR,	"Edit &Controller ...");
  misc_menu->Append(MEN_CTRL_TEMPO,	"Edit &Tempo");
  misc_menu->Append(MEN_CTRL_NONE,	"Edit &None");
  misc_menu->Append(MEN_GUITAR,		"&Guitar board");

  wxMenu *help_menu = new wxMenu("",wxMENU_TEAROFF);
  help_menu->Append(MEN_HELP_PWIN, "&Pianowin");
  help_menu->Append(MEN_HELP_MOUSE, "&Mouse");

  menu_bar = new wxMenuBar;
  menu_bar->Append(win_menu,    "&Window");
  menu_bar->Append(edit_menu,    "&Edit");
  menu_bar->Append(setting_menu, "&Settings");
  menu_bar->Append(misc_menu,    "&Misc");
  menu_bar->Append(help_menu,    "&Help");

  SetMenuBar(menu_bar);
}



void tPianoWin::OnMenuCommand(int id)
{
  int cw, ch;
  GetClientSize(&cw, &ch);

  switch (id)
  {

  


 #ifndef __PORTING



    case MEN_HELP_MOUSE:
      wxMessageBox((char *)mouse_help, "Help", wxOK);
      break;

    case MEN_HELP_PWIN:
      HelpInstance->ShowTopic("Piano Window");
      break;

#endif // __PORTING

  }
}


BEGIN_EVENT_TABLE(tPianoWin, wxFrame)
  EVT_SIZE    (tPianoWin::OnSize) 
  EVT_MENU    (MEN_ZOOMIN, tPianoWin::OnZoomIn)
  EVT_MENU    (MEN_ZOOMOUT, tPianoWin::OnZoomOut)
  EVT_MENU    ( MEN_SNAP_8  , tPianoWin::OnSnap8)
  EVT_MENU    ( MEN_SNAP_8D , tPianoWin::OnSnap8D)
  EVT_MENU    ( MEN_SNAP_16 , tPianoWin::OnSnap16)
  EVT_MENU    ( MEN_SNAP_16D, tPianoWin::OnSnap16D)

  EVT_MENU    ( MEN_MSELECT  ,  tPianoWin::OnMSelect) 
  EVT_MENU    ( MEN_MLENGTH  ,  tPianoWin::OnMLength) 
  EVT_MENU    ( MEN_MDIALOG  ,  tPianoWin::OnMDialog) 
  EVT_MENU    ( MEN_MCUTPASTE,tPianoWin::OnMCutPaste)
  EVT_MENU    ( MEN_GUITAR, tPianoWin::OnGuitar)

  EVT_MENU    (MEN_RESET,tPianoWin::OnReset)
  EVT_MENU    (MEN_VIS_ALL_TRK,tPianoWin::OnVisibleAllTracks)
  EVT_MENU    (MEN_ERASE,tPianoWin::OnErase)
  EVT_MENU    (MEN_CUT,tPianoWin::OnCut)
  EVT_MENU    (MEN_COPY,tPianoWin::OnCopy)
  EVT_MENU    (MEN_SHIFT,tPianoWin::OnShift)
  EVT_MENU    (MEN_SHIFTL,tPianoWin::OnShiftLeft)
  EVT_MENU    (MEN_SHIFTR,tPianoWin::OnShiftRight)
  EVT_MENU    (MEN_LERI,tPianoWin::OnExchangeLeftRight)
  EVT_MENU    (MEN_UPDN,tPianoWin::OnExchangeUpDown)
  EVT_MENU    (MEN_QUANTIZE,tPianoWin::OnQuantize)
  EVT_MENU    (MEN_UNDO,tPianoWin::OnUndo)
  EVT_MENU    (MEN_REDO,tPianoWin::OnRedo)
  EVT_MENU    (MEN_CTRL_PITCH,tPianoWin::OnCtrlPitch)
  EVT_MENU    (MEN_CTRL_MODUL,tPianoWin::OnCtrlModulation)
  EVT_MENU    (MEN_CTRL_CONTR,tPianoWin::OnSelectController)
  EVT_MENU    (MEN_CTRL_VELOC,tPianoWin::OnCtrlVelocity)
  EVT_MENU    (MEN_CTRL_TEMPO,tPianoWin::OnCtrlTempo)
  EVT_MENU    (MEN_CTRL_NONE,tPianoWin::OnCtrlNone)
  EVT_MENU    (MEN_CTRL_POLY_AFTER,tPianoWin::OnCtrlPolyAftertouchEdit)
  EVT_MENU    (MEN_CTRL_CHANNEL_AFTER,tPianoWin::CtrlChannelAftertouchEdit)
  EVT_MENU    (MEN_CLEANUP,tPianoWin::MenCleanup)
  EVT_MENU    (MEN_SEARCHREP,  tPianoWin::MenSearchReplace)
  EVT_MENU    (MEN_TRANSP, 	tPianoWin::MenTranspose)
  EVT_MENU    (MEN_SETCHAN,	tPianoWin::MenSetChannel)
  EVT_MENU    (MEN_VELOC ,	tPianoWin::MenVelocity)
  EVT_MENU    (MEN_LENGTH, tPianoWin::MenLength)
  EVT_MENU    (MEN_SEQLENGTH,  	tPianoWin::MenSeqLength)
  EVT_MENU    (MEN_MIDIDELAY, tPianoWin::MenMidiDelay)
  EVT_MENU    (MEN_CONVERT_TO_MODULATION ,	tPianoWin::MenConvertToModulation)
  EVT_MENU    (MEN_SETTINGS,tPianoWin::OnSettingsDialog)
  EVT_MENU    (MEN_FILTER,tPianoWin::OnFilter)
  EVT_MENU    (MEN_SNAP    ,tPianoWin::SnapDlg)
END_EVENT_TABLE()
void tPianoWin::OnFilter(){
  Filter->Dialog(0);
  
  }

void tPianoWin::OnSettingsDialog(){
  SettingsDialog(1);
  
  }

/**activate velocity edit*/
void tPianoWin::OnCtrlVelocity(){
      delete CtrlEdit;
      int cw, ch;
      GetClientSize(&cw, &ch);
      CtrlEdit = new tVelocEdit(this, "Velocity", wPiano, 0, CtrlY(ch), CanvasW, CtrlH(ch));
      CtrlEdit->ReInit(Track, FromClock, ClocksPerPixel);
      Redraw();
  }


void tPianoWin::CtrlChannelAftertouchEdit(){
  delete CtrlEdit;
  int cw, ch;
  GetClientSize(&cw, &ch);
  CtrlEdit = new tChannelAfterEdit(this, "Channel Aftertouch", wPiano, 0, CtrlY(ch), CanvasW,CtrlH(ch));
  CtrlEdit->ReInit(Track, FromClock, ClocksPerPixel);
  Redraw();
  
  }
void tPianoWin::OnCtrlPolyAftertouchEdit(){
  int cw, ch;
  GetClientSize(&cw, &ch);
  delete CtrlEdit;
  CtrlEdit = new tPolyAfterEdit(this, "Key Aftertouch", wPiano, 0, CtrlY(ch), CanvasW,CtrlH(ch));
  CtrlEdit->ReInit(Track, FromClock, ClocksPerPixel);
  Redraw();  
  }
void tPianoWin::OnCtrlNone(){
      delete CtrlEdit;
      CtrlEdit = 0;
      Redraw();  
  }
void tPianoWin::OnCtrlTempo(){
      tEventIterator Iterator(Track);
      Track->Sort();
      tEvent *e = Iterator.Range(0, (long unsigned) Track->GetLastClock() + 1);
      tSetTempo *t;
      int min = 240;
      int max = 20;
      while (e) {
        if ((t = e->IsSetTempo()) != 0) {
	  min = MIN( t->GetBPM(), min );
	  max = MAX( t->GetBPM(), max );
        }
        e = Iterator.Next();
      } // while e

      delete CtrlEdit;
      int cw, ch;
      GetClientSize(&cw, &ch);
      CtrlEdit = new tTempoEdit( MAX(min-50, 20), MIN(max+50,240), this, "Tempo", wPiano, 0, CtrlY(ch), CanvasW, CtrlH(ch));
      CtrlEdit->ReInit(Track, FromClock, ClocksPerPixel);
      Redraw();  
  }


void tPianoWin::OnSelectController(){
	int i = SelectControllerDlg();
	if (i > 0)
	{
	  delete CtrlEdit;
	  int cw, ch;
	  GetClientSize(&cw, &ch);
	  CtrlEdit = new tCtrlEdit(i-1, this, Config.CtrlName(i).Name, wPiano, 0, CtrlY(ch), CanvasW, CtrlH(ch));
	  CtrlEdit->ReInit(Track, FromClock, ClocksPerPixel);
	  Redraw();
	}  
  }

void tPianoWin::OnCtrlModulation(){
  delete CtrlEdit;
  int cw, ch;
  GetClientSize(&cw, &ch);
  CtrlEdit = new tCtrlEdit(1, this, "Modulation", wPiano, 0, CtrlY(ch), CanvasW, CtrlH(ch));
  CtrlEdit->ReInit(Track, FromClock, ClocksPerPixel);
  Redraw();  
}

void tPianoWin::OnCtrlPitch(){
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
void tPianoWin::OnRedo(){
      Song->Redo();
      Redraw();
      if (CtrlEdit && Track >= 0)
	CtrlEdit->ReInit(Track, FromClock, ClocksPerPixel);  
  }

/**
 undo actions 
*/
void tPianoWin::OnUndo(){
      Song->Undo();
      Redraw();
      if (CtrlEdit && Track >= 0)
	CtrlEdit->ReInit(Track, FromClock, ClocksPerPixel);  
  }

/** quantize selected events*/
void tPianoWin::OnQuantize(){
      if (EventsSelected())
      {
	tCmdQuantize cmd(Filter, SnapClocks(), 0, 0);
	cmd.Execute(1);
	Redraw();
      }  
  }
/** flip events up and down*/
void tPianoWin::OnExchangeUpDown(){
      if (EventsSelected())
      {
	tCmdExchUpDown cmd(Filter);
	cmd.Execute(1);
	Redraw();
      }  
  }

/**flip events left to righ*/
void tPianoWin::OnExchangeLeftRight(){
      if (EventsSelected())
      {
	tCmdExchLeftRight cmd(Filter);
	cmd.Execute(1);
	Redraw();
      }  
  }

/**shift events snapclock clocks to left*/
void tPianoWin::OnShiftLeft(){
  if (EventsSelected())
    {
      long steps =  -SnapClocks();
      tCmdShift cmd(Filter, steps);
      cmd.Execute();
      Redraw();
    }
}

/**shift events snapclock clocks to right*/
void tPianoWin::OnShiftRight(){
  if (EventsSelected())
    {
      long steps = SnapClocks();
      tCmdShift cmd(Filter, steps);
      cmd.Execute();
      Redraw();
    }
}

void tPianoWin::OnShift(){
  MenShift(SnapClocks());  //see eventwin.cpp
  }

void tPianoWin::OnCut(){
  CutOrCopy(MEN_CUT);
  }

void tPianoWin::OnCopy(){
  CutOrCopy(MEN_COPY);
  }

/**helper for cut and copy events*/
void tPianoWin::CutOrCopy(int id){
      if (EventsSelected())
      {
	PasteBuffer.Clear();
	tCmdCopyToBuffer cmd(Filter, &PasteBuffer);
	Filter->OtherSelected = VisibleTempo;
	cmd.Execute(0);	// no UNDO
	if (id == MEN_CUT)
	{
	  tCmdErase cmd(Filter);
	  cmd.Execute(1);	// with UNDO
	  Redraw();
	}
	Filter->OtherSelected = 0;
	if (GuitarWin)
	  GuitarWin->Redraw();
      }  
  }


void tPianoWin::OnErase(){
  if (EventsSelected())
    {
      tCmdErase cmd(Filter);
      cmd.Execute(1);	// with UNDO
	Redraw();
    }  
}

/**togle display of events from all tracks, or just from the current track */
void tPianoWin::OnVisibleAllTracks(){
      VisibleAllTracks = !VisibleAllTracks;
      //VisibleAllTracks = tool_bar->GetToolState(MEN_VIS_ALL_TRK);
      Redraw();  
  }

/**send a midi reset*/
  void tPianoWin::OnReset(){
  Midi->AllNotesOff(1);
}

  //INSER_EVENT_HANDLER_HERE

/*
//all the code commented out will compile nicely


      //these wont compile nicely

  case MEN_VISIBLE:   VisibleDialog(); break;

    case MEN_METERCH:	MenMeterChange(); break;



case MEN_CLOSE:     Show(FALSE); break;

*/

/**show the guitar edit  window*/
  void tPianoWin::OnGuitar(){
  if (!GuitarWin)
    GuitarWin = new tGuitarWin(this);
  GuitarWin->Show(TRUE);
}


		void tPianoWin::OnMSelect(){   PressRadio(MEN_MSELECT  ); MouseEvnt.SetLeftAction(MA_SELECT); }
		void tPianoWin::OnMLength(){   PressRadio(MEN_MLENGTH  ); MouseEvnt.SetLeftAction(MA_LENGTH); }
		void tPianoWin::OnMDialog(){   PressRadio(MEN_MDIALOG  ); MouseEvnt.SetLeftAction(MA_DIALOG); }
		void tPianoWin::OnMCutPaste(){ PressRadio(MEN_MCUTPASTE); MouseEvnt.SetLeftAction(MA_CUTPASTE); }

  void tPianoWin::OnZoomIn(){
      if (ClocksPerPixel > 1) {
        ZoomIn();
        // TrackNr for Ctr-Editor
        NewPosition(TrackNr,FromClock);
      }
}

void tPianoWin::OnZoomOut(){
  if (ClocksPerPixel<32) {
    ZoomOut();
    NewPosition(TrackNr, FromClock);
  }
}


void tPianoWin::OnSnap8(){
  PasteBuffer.Clear(); SetSnapDenom(8);
}


void tPianoWin::OnSnap8D(){
  PasteBuffer.Clear(); SetSnapDenom(12);
}

void tPianoWin::OnSnap16(){
  PasteBuffer.Clear(); SetSnapDenom(16);
}

void tPianoWin::OnSnap16D(){
  PasteBuffer.Clear(); SetSnapDenom(24);
}








// ********************************************************************
// Visible
// ********************************************************************

int tPianoWin::IsVisible(tEvent *e)
{
  switch (e->Stat)
  {
    case StatKeyOn: return VisibleKeyOn;
    case StatPitch: return VisiblePitch;
    case StatControl: return VisibleController;
    case StatProgram: return VisibleProgram;
    case StatSetTempo: return VisibleTempo;
    case StatSysEx: return VisibleSysex;
    case StatPlayTrack: return VisiblePlayTrack;
    case StatEndOfTrack: return true;
    case StatText: return true;
    // SN++
    case StatChnPressure: return VisibleMono;
  }
  return 0;
}


int tPianoWin::IsVisible(tTrack *t)
{
  if (!VisibleAllTracks)
    return t == Track;

  return (Track->Channel == Config(C_DrumChannel)) == (t->Channel == Config(C_DrumChannel));
}

#ifndef __PORTING

class tVisibleDlg : public wxForm
{
  tPianoWin *win;
  public:

    tVisibleDlg(tPianoWin *p) : wxForm( USED_WXFORM_BUTTONS ), win(p) {}
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
  HelpInstance->ShowTopic("Events");
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


void tPianoWin::VisibleDialog()
{
  wxDialogBox *panel = new wxDialogBox(this, "Select Events", FALSE );
  tVisibleDlg * dlg = new tVisibleDlg(this);
  dlg->EditForm(panel);
  panel->Fit();
  panel->Show(TRUE);
}
#endif // __PORTING

// ********************************************************************
// Painting
// ********************************************************************

const int isBlack[12] = {0,1,0,1,0,0,1,0,1,0,1,0};
#define IsBlack(Key)  isBlack[(Key) % 12]


void tPianoWin::OnPaintSub(wxDC* dc, long x, long y)
{
  long OldFromClock = FromClock;
  tEventWin::OnPaintSub(dc, x, y);
// SN++ Da Jazz nun eine ReDo Funktion hat. Behebt gleichzeitig ein kleines
//		Update Problem beim mehrfachen ZoomOut.
//		Aktives Ctrl-Fenster neu zeichnen bzw. reinitialisieren.

//  if (CtrlEdit && OldFromClock != FromClock)
//    CtrlEdit->ReInit(Track, FromClock, ClocksPerPixel);

  if (CtrlEdit)
    CtrlEdit->ReInit(Track, FromClock, ClocksPerPixel);

//
  xPiano  = CanvasX;

  long StopClk;
  tBarInfo BarInfo(Song);
  char buf[20];

  dc->BeginDrawing();
  dc->DestroyClippingRegion();
  dc->SetBackground(*wxWHITE_BRUSH);
  DrawPlayPosition(dc);
  SnapSel->Draw(dc,xEvents, yEvents, wEvents, hEvents);
  dc->Clear();


  ///////////////////////////////////////////////////////////////
  // horizontal lines(ripped from drawpianoroll code)

//     for (y = Line2y(FromLine); y < yEvents + hEvents; y += hLine)
//      if (y > yEvents)	// cheaper than clipping
//        dc->DrawLine(xEvents+1, y, xEvents + wEvents, y);

  dc->SetPen(*wxGREY_PEN);
  wxBrush blackKeysBrush=wxBrush(wxColor(250,240,240),wxSOLID);
  int Pitch = 127 - FromLine;
  long wBlack = wPiano * 2 / 3;
  y = Line2y(FromLine);
    while (Pitch >= 0 && y < yEvents + hEvents)
    {
      if (IsBlack(Pitch))
      {
	dc->SetBrush(blackKeysBrush);//*wxLIGHT_GREY_PEN
	dc->DrawRectangle(CanvasX, y, 2000, hLine);
      }

      else if ((Pitch % 12) == 0)
      {
	dc->SetPen(*wxCYAN_PEN);
	dc->DrawLine(CanvasX, y + hLine, 2000, y + hLine);

      }
      else if (!IsBlack(Pitch - 1))
      {
	dc->SetPen(*wxGREEN_PEN);
	dc->DrawLine(CanvasX, y + hLine, 2000, y + hLine);
      }

      y += hLine;
      --Pitch;
    }


  ///////////////////////////////////////////////////////////////


  MouseLine = -1;

  #define VLine(x) DrawLine(x, CanvasY, x, yEvents+hEvents)
  #define HLine(y) DrawLine(CanvasX, y, CanvasX + CanvasW, y)

  dc->SetPen(*wxBLACK_PEN);

  // vertical lines

  dc->VLine(xPiano);
  dc->VLine(xEvents);
  dc->VLine(xEvents-1);
  dc->HLine(yEvents);
  dc->HLine(yEvents-1);
  dc->HLine(yEvents + hEvents);

  // draw vlines and bar numbers

  dc->SetFont(*FixedFont);
  BarInfo.SetClock(FromClock);
  StopClk = x2Clock(CanvasX + CanvasW);
  long clk = BarInfo.Clock;
  int intro = Song->GetIntroLength();
  while (clk < StopClk)
  {
    clk = BarInfo.Clock;
    x = Clock2x(clk);
    // vertical lines and bar numbers
    int i;
    dc->SetPen(*wxBLACK_PEN);
    sprintf(buf, "%d", BarInfo.BarNr + 1 - intro);
    if (x > xEvents)
    {
      dc->DrawText(buf, x + LittleBit, yEvents - hFixedFont - 2);
      dc->SetPen(*wxGREY_PEN);
      dc->DrawLine(x, yEvents - hFixedFont, x, yEvents+hEvents);
    }

    dc->SetPen(*wxLIGHT_GREY_PEN);
    for (i = 0; i < BarInfo.CountsPerBar; i++)
    {
      clk += BarInfo.TicksPerBar / BarInfo.CountsPerBar;
      x = Clock2x(clk);
      if (x > xEvents)
	dc->DrawLine(x, yEvents+1, x, yEvents+hEvents);
    }
    BarInfo.Next();
  }


  dc->SetPen(*wxBLACK_PEN);
  DrawPianoRoll(dc);
  LineText(dc,xPiano, CanvasY-1, wPiano, "", hTop);



  // draw chords from harmony-browser
  if (VisibleHBChord && the_harmony_browser && !Track->IsDrumTrack()) {
    HBAnalyzer *an = the_harmony_browser->getAnalyzer();
    if (an != 0)
    {
      wxBrush cbrush = *wxBLUE_BRUSH;
      wxBrush sbrush = *wxBLUE_BRUSH;
#ifdef wx_msw
      cbrush.SetColour(191,191,255);
      sbrush.SetColour(191,255,191);
#else
      cbrush.SetColour(220,220,255);
      sbrush.SetColour(230,255,230);
#endif

      //dc->SetClippingRegion(xEvents, yEvents, xEvents + wEvents, yEvents + hEvents);
      dc->SetLogicalFunction(wxXOR);
      dc->SetPen(*wxTRANSPARENT_PEN);

      int steps = an->Steps();
      for (int step = 0; step < steps; step ++) {
        long start = an->Step2Clock(step);
        long stop  = an->Step2Clock(step + 1);
        if (stop > FromClock && start < ToClock) {
          // this chord is visible
          HBContext *context = an->GetContext(step);
          HBChord chord = context->Chord();
          HBChord scale = context->Scale();

          long x = Clock2x(start);
          if (x < xEvents)	// clip to left border
            x = xEvents;
          long w = Clock2x(stop) - x;
          if (w <= 0)
            continue;

          long h = hLine;
	  for (int i = 0; i < 12; i++) {
	    int pitch = i;
	    wxBrush *brush = 0;
	    if (chord.Contains(i))
	      brush = &cbrush;
	    else if (scale.Contains(i))
	      brush = &sbrush;
	    if (brush) {
	      dc->SetBrush(*brush);
	      while (pitch < 127) {
		long y = Pitch2y(pitch);
		if (y >= yEvents && y <= yEvents + hEvents - h) // y-clipping
		  dc->DrawRectangle(x, y, w, h);
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
      tTrack *t = Song->GetTrack(i);
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
  dc->SetBackground(*wxWHITE_BRUSH);	// xor-bug

  SnapSel->Draw(dc, xEvents, yEvents, wEvents, hEvents);

  DrawPlayPosition(dc);
  dc->EndDrawing();
}




void tPianoWin::DrawPianoRoll(wxDC* dc)
{
  char buf[20];

  dc->SetBrush(*grey_brush);
  dc->DrawRectangle(xPiano, yEvents, wPiano, hEvents); //draw grey bg for keyboard
  dc->SetBrush(*wxBLACK_BRUSH);

  dc->SetTextBackground(*grey_color);

  long wBlack = wPiano * 2 / 3;
  int Pitch = 127 - FromLine;
  long y = Line2y(FromLine);

  if (VisibleKeyOn
#ifdef AUDIO
      && !Track->GetAudioMode()
#endif
      && (!Track->IsDrumTrack() || !VisibleDrumNames))
  {
    dc->SetFont(*FixedFont);
     while (Pitch >= 0 && y < yEvents + hEvents)
    {
      if (IsBlack(Pitch))
      {
	dc->DrawRectangle(CanvasX, y, wBlack, hLine);
	dc->DrawLine(CanvasX + wBlack, y + hLine/2, CanvasX + wPiano, y + hLine/2);
	dc->SetPen(*wxWHITE_PEN);
	dc->DrawLine(CanvasX + wBlack+1, y + hLine/2+1, CanvasX + wPiano, y + hLine/2+1);
	dc->DrawLine(CanvasX, y, CanvasX + wBlack, y);
	dc->SetPen(*wxBLACK_PEN);
      }

      else if ((Pitch % 12) == 0)
      {
	dc->DrawLine(CanvasX, y + hLine, CanvasX + wPiano, y + hLine);
	dc->SetPen(*wxWHITE_PEN);
	dc->DrawLine(CanvasX, y + hLine+1, CanvasX + wPiano, y + hLine+1);
	dc->SetPen(*wxBLACK_PEN);
	sprintf(buf, "%d", Pitch / 12);
	dc->DrawText(buf, CanvasX + wBlack + LittleBit, y + hLine / 2);
      }
      else if (!IsBlack(Pitch - 1))
      {
	dc->DrawLine(CanvasX, y + hLine, CanvasX + wPiano, y + hLine);
	dc->SetPen(*wxWHITE_PEN);
	dc->DrawLine(CanvasX, y + hLine+1, CanvasX + wPiano, y + hLine+1);
	dc->SetPen(*wxBLACK_PEN);
      }

      y += hLine;
      --Pitch;
    }
  }

#ifdef AUDIO
  else if (Track->GetAudioMode())
  {
    dc->SetFont(*DrumFont);
    while (Pitch >= 0 && y < yEvents + hEvents)
    {
      dc->DrawText(Midi->GetSampleName(Pitch), CanvasX + LittleBit, y);
      y += hLine;
      --Pitch;
    }
  }
#endif

  else
  {
    // Draw text?
    tNamedValue *names = 0;
    if (VisibleKeyOn && VisibleDrumNames)
      names = &Config.DrumName(0);
    else if (VisibleController)
      names = &Config.CtrlName(0);
    else if (VisibleProgram)
      names = &Config.VoiceName(0);
    else if (VisibleSysex)
      names = SysexGroupNames;
    if (names)
    {
      dc->SetFont(*DrumFont);
      while (Pitch >= 0 && y < yEvents + hEvents)
      {
        dc->DrawText(names[Pitch+1].Name, CanvasX + LittleBit, y);
	y += hLine;
	--Pitch;
      }
    }
    else if (VisiblePitch)
      ;
  }

  //dc->DestroyClippingRegion();
  dc->SetTextBackground(*wxWHITE);
  dc->SetFont(*Font);
}


//JAVE the previous definition of this function used xor, which is a reserved keyword in ISO C++ 98 
//(meaning it wouldnt compile on newer gcc:s)
void tPianoWin::DrawEvent(wxDC* dc, tEvent *e, wxBrush *Brush, int xoor, int force_color)
{
  // SN++ Aftertouch
  if (e->IsKeyPressure() || e->IsChnPressure()) return;

  int length = e->GetLength() / ClocksPerPixel;
  // Always draw at least two pixels to avoid invisible (behind a
  // vertical line) or zero-length events:
  if (length < 3)
    length = 3;
  dc->BeginDrawing();
  if (xoor)
    dc->SetLogicalFunction(wxXOR);
  long x = Clock2x(e->Clock);
  long y = Pitch2y(e->GetPitch());
  if (!xoor)	
  {
    dc->SetBrush(*wxWHITE_BRUSH);
    dc->DrawRectangle(x, y + LittleBit, length, hLine - 2 * LittleBit);
  }

// SN++
  // show velocity as colors
  if (force_color !=0 && UseColors && e->IsKeyOn()) {
    int vel = e->IsKeyOn()->Veloc;
    int i = vel * NUM_COLORS / 127;
    if (i > NUM_COLORS-1)
      i = NUM_COLORS-1;
    dc->SetBrush(*(color_brush[i]));
  }
  else
    dc->SetBrush(*Brush);
  // end velocity colors

   
  dc->DrawRectangle(x, y + LittleBit, length, hLine - 2 * LittleBit);



  if (xoor)
    dc->SetLogicalFunction(wxCOPY);
  dc->SetBrush(*wxBLACK_BRUSH);
  dc->EndDrawing();
}




void tPianoWin::DrawEvents(wxDC* dc, tTrack *t, int Stat, wxBrush *Brush, int force_color)
{
  //dc->SetClippingRegion(xEvents, yEvents, xEvents + wEvents, yEvents + hEvents);
  dc->SetBrush(*Brush);

  tEventIterator Iterator(t);
  tEvent *e = Iterator.First();
  int FromPitch = 127 - ToLine;
  int ToPitch   = 127 - FromLine;

  // Koordinate fuer Linien

  long x0 = Clock2x(0);
  long y0 = Line2y(64);
  char buf[20];

  while (e)
  {
    if (e->Stat == Stat)
    {
      int Pitch   = e->GetPitch();
      long Length = e->GetLength();
      long Clock  = e->Clock;

      long x1 = Clock2x(Clock);
      long y1 = Line2y(127 - Pitch);
//        if (e->IsPlayTrack()) {
//  	y1=Line2y(127-e->IsPlayTrack()->track); //JAVE so the y position of playtrack events tell which track they play (the drawing should rather be polymorpic in my opinion)
      //use pitch instead
      //      }
      // event partially visible?
      if (Clock + Length >= FromClock && FromPitch < Pitch && Pitch <= ToPitch)
      {
	long DrawLength = Length/ClocksPerPixel;
	// do clipping ourselves
	if (x1 < xEvents)
	{
	  DrawLength -= xEvents - x1;
	  x1 = xEvents;
	}
	// Always draw at least two pixels to avoid invisible (behind a
	// vertical line) or zero-length events:
	if (DrawLength < 3)
	  DrawLength = 3;

	// show velocity as colors
	if (!force_color && UseColors && e->IsKeyOn()) {
	  int vel = e->IsKeyOn()->Veloc;
	  int i = vel * NUM_COLORS / 127;
	  if (i > NUM_COLORS-1)
	    i = NUM_COLORS-1;
	  dc->SetBrush(*(color_brush[i]));
	}
	else
	  dc->SetBrush(*Brush);
	// end velocity colors

        dc->DrawRectangle(x1, y1 + LittleBit, DrawLength, hLine - 2 * LittleBit);
	//shouldnt it be in drawevent? odd. 

	//printf("isplaytrack:%x",e->IsPlayTrack());
	if (e->IsPlayTrack()) {
	  dc->SetPen(*wxBLACK_PEN);
	  sprintf(buf, "Track:%l", e->IsPlayTrack()->track); 
	  dc->DrawText(buf, x1, y1 + LittleBit);
	}
      }
      
      if(Clock + Length >= FromClock){
	//thesse events are always visible in vertical
	if (e->IsEndOfTrack()) {
	  dc->SetPen(*wxRED_PEN);
	  dc->VLine(x1); //draw a vertical bar
	  dc->SetPen(*wxBLACK_PEN);
	  sprintf(buf, "EOT"); 
	  dc->DrawText(buf, x1, y1 + LittleBit);
	}
	
	if (e->IsText()) {
	  //fprintf(stderr,"text:%s",e->IsText()->GetText());
	  dc->SetPen(*wxGREEN_PEN);
	  dc->VLine(x1); //draw a vertical bar
	  dc->SetPen(*wxBLACK_PEN);
	  sprintf(buf, (const char*)e->IsText()->GetText());
	  int textX;
	  int textY;
	  
	  dc->GetTextExtent((const char*)e->IsText()->GetText(), &textX, &textY); 
	  dc->SetBrush(*wxWHITE_BRUSH);
	  long textlabely=CanvasY+hTop;//text labels drawn at top
	  dc->DrawRectangle(x1-textX, textlabely + LittleBit, textX, textY);//hLine - 2 * LittleBit);
	  dc->DrawText(buf, x1-textX, textlabely + LittleBit);
	}
      }

      x0 = x1;
      y0 = y1;

      if (Clock > ToClock)
        break;
    }
    e = Iterator.Next();
  }
  dc->SetBrush(*wxBLACK_BRUSH);
  //dc->DestroyClippingRegion();
}



// ********************************************************************
// Utilities
// ********************************************************************

long tPianoWin::SnapClocks()
{
  long clk = Song->TicksPerQuarter * 4L / SnapDenomiator;
  if (clk < 1)
    return 1;
  return clk;
}

int tPianoWin::y2Pitch(long y)
{
  int pitch = 127 - y2Line(y);
  if (pitch < 0)
    return 0;
  if (pitch > 127)
    return 127;
  return pitch;
}


long tPianoWin::Pitch2y(int Pitch)
{
  return Line2y(127 - Pitch);
}


tEvent *tPianoWin::FindEvent(tTrack *Track, long Clock, int Pitch)
// Pitch == -1: search for any pitches
{
  tEventIterator Iterator(Track);
  tEvent *e = Iterator.First();
  while (e)
  {
    if (e->Clock <= Clock)
    {
      if ((e->Clock + e->GetLength() >= Clock)
           && (e->GetPitch() == Pitch || Pitch == -1)
	   && IsVisible(e))
      {
        return e;
      }
    }
    else
      return 0;
    e = Iterator.Next();
  }
  return 0;
}


// SN++
// Methoden fuer Key-Aftertouch
void tPianoWin::kill_keys_aftertouch(tTrack *t, tEvent *e)
{
  int key,channel;
  tEventIterator iter(t);
  tKeyPressure *a;
  tKeyOn *k = e->IsKeyOn();
  if (!k) return;
  if (k->Length < 2) return;
  key = k->Key;
  channel = k->Channel;
  e = iter.Range(k->Clock+1,k->Clock+k->Length);
  while (e) {
    a = e->IsKeyPressure();
    if (a)
      if (a->Key == key && a->Channel == channel) t->Kill(e);
    e = iter.Next();
  }
}

void tPianoWin::paste_keys_aftertouch(tTrack *t, tEvent *e)
{
  int key,channel;
  tEventIterator iter(t);
  tKeyPressure *a;
  tKeyOn *k = e->IsKeyOn();
  if (!k) return;
  channel = k->Channel;
  if (k->Length < 2) return;
  key = k->Key;
  e = iter.Range(k->Clock+1,k->Clock+k->Length);
  while (e) {
    a = e->IsKeyPressure();
    if (a)
      if (a->Key == key && a->Channel == channel) PasteBuffer.Put(e->Copy());
    e = iter.Next();
  }
}

int tPianoWin::nKeyOnEvents()
{
  int count = 0;

  tEventIterator Iterator(&PasteBuffer);
  tEvent *e = Iterator.First();
  while (e) {
    if (e->IsKeyOn()) count++;
    e = Iterator.Next();
  }
  return count;
}



void tPianoWin::Copy(tTrack *t, tEvent *e, int Kill)
{
  if (!e)
    return;

  Song->NewUndoBuffer();
  PasteBuffer.Clear();
  PasteBuffer.Put(e->Copy());
  // SN++
  if (e->IsKeyOn())
    paste_keys_aftertouch(t,e);
  //
  if (Kill)
  {
    tKeyOn *k = e->IsKeyOn();
    if (k)
    {
      kill_keys_aftertouch(t,e);
#ifdef AUDIO
      if (Track->GetAudioMode())
	Midi->ListenAudio(k->Key, 0);
      else
	Listen.KeyOn(Track, k->Key, k->Channel, k->Veloc, k->Length);
#else
      Listen.KeyOn(Track, k->Key, k->Channel, k->Veloc, k->Length);
#endif
    }

  wxDC* dc=new wxClientDC(this);
    DrawEvent(dc, e, wxWHITE_BRUSH, 0);
    t->Kill(e);
    t->Cleanup();
  }


  if (GuitarWin)
    GuitarWin->Redraw();

  // SN++ Veloc- oder Aftertouch-Editor updaten
  if (CtrlEdit)
    CtrlEdit->UpDate();
}


void tPianoWin::Paste(tTrack *t, long Clock, int Pitch)
{
  if (PasteBuffer.nEvents == 0)
  {
    int len = SnapClocks() - 4;
    if (len < 2)
      len = 2;          
    tKeyOn *e = new tKeyOn(0, 0, 64, 64, len);
    PasteBuffer.Put(e);
  }
  // SN++
  if (nKeyOnEvents() > 1)
    Pitch = -1;	// don't change Pitch

  Song->NewUndoBuffer();
  tEventIterator Iterator(&PasteBuffer);
  tEvent *e = Iterator.First();
  if (e)
  {
    // SN++
    tEvent *a = e;
    while (a) {
      if (e->IsChnPressure()) {
       a = Iterator.Next();
        e = a;
      } else
       a = NULL;
    }

    long DeltaClock = Clock - e->Clock;
    int  DeltaPitch = 0;
    if (Pitch >= 0)
      DeltaPitch = Pitch - e->GetPitch();
    while (e)
    {
      tEvent *c = e->Copy();
      c->SetPitch(c->GetPitch() + DeltaPitch);
      c->Clock += DeltaClock;
      if (t->ForceChannel && c->IsChannelEvent())
        c->IsChannelEvent()->Channel = t->Channel - 1;
      tKeyOn *k = c->IsKeyOn();
#ifdef AUDIO
      if (k)
      {
	if (Track->GetAudioMode())
	  Midi->ListenAudio(k->Key, 0);
	else
	  Listen.KeyOn(Track, k->Key, k->Channel, k->Veloc, k->Length);
      }
#else
      if (k)
	Listen.KeyOn(Track, k->Key, k->Channel, k->Veloc, k->Length);
#endif
      wxDC* dc=new wxClientDC(this);
      DrawEvent(dc, c, c->GetBrush(), 0, 1);
      t->Put(c);
      e = Iterator.Next();
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
 * left drag: Events markieren fuer Menu
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
    tPianoWin *Win;
  public:
    tMousePlay(tPianoWin *win, wxMouseEvent &e);
    int Event(wxMouseEvent &e);
};

tMousePlay::tMousePlay(tPianoWin *win, wxMouseEvent &e)
{
  Win = win;

  Pitch = 0;
  Channel = Win->Track->Channel ? Win->Track->Channel - 1 : 0;
  Event(e);
}


int tMousePlay::Event(wxMouseEvent &e)
{
  int x, y;

  int OldPitch = Pitch;
  e.GetPosition(&x, &y);

  if (e.LeftDown())
  {
    Pitch = Win->y2Pitch((long)y);
    Veloc = 64;
  }
  else if (e.MiddleDown())
  {
    Pitch = Win->y2Pitch((long)y);
    Veloc = 80;
  }
  else if (e.RightDown())
  {
    Pitch = Win->y2Pitch((long)y);
    Veloc = 110;
  }
  else if (e.ButtonUp())
  {
    Pitch = 0;
  }
  else if (e.Dragging())
    Pitch = Win->y2Pitch((long)y);
  else
    return 0;

#ifdef AUDIO
  if (Win->Track->GetAudioMode())
  {
    if (Pitch && Pitch != OldPitch)
      Midi->ListenAudio(Pitch, 0);
  }
  else
#endif

  {
    if (OldPitch && OldPitch != Pitch)
    {
      tKeyOff of(0, Channel, OldPitch);
      Midi->OutNow(Win->Track, &of);
      OldPitch = 0;
    }

    if (Pitch && Pitch != OldPitch)
    {
      tKeyOn on(0, Channel, Pitch, Veloc);
      Midi->OutNow(Win->Track, &on);
      OldPitch = 0;
    }
  }

  if (!Pitch)
  {
    Win->MouseAction = 0;
    delete this;
    return 1;	// done
  }
  return 0;
}


void tPianoWin::MousePiano(wxMouseEvent &e)
{
  if (e.ButtonDown())
    MouseAction = new tMousePlay(this, e);
}


int tPianoWin::Channel()
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
    tPianoWin *Win;
    tTrack    *Track;
    wxDC      *dc;

  public:
    tKeyLengthDragger(tKeyOn *k, tPianoWin *w);
    int Dragging(wxMouseEvent &e);
    int ButtonUp(wxMouseEvent &e);
    int Event(wxMouseEvent &e);
};


tKeyLengthDragger::tKeyLengthDragger(tKeyOn *k, tPianoWin *w)
{
  KeyOn = k;
  Copy  = k->Copy() -> IsKeyOn();
  Win   = w;

  // SN++ BUG FIX: undo/redo
  Win->Song->NewUndoBuffer();
  //
  wxDC* dc=new wxClientDC(w);
  Win->DrawEvent(dc, Copy, wxWHITE_BRUSH, 0);
  Win->DrawEvent(dc, Copy, Copy->GetBrush(), 1, 1);
}

int tKeyLengthDragger::Event(wxMouseEvent &e)
{
  if (e.Dragging())		return Dragging(e);
  else if (e.ButtonUp())	return ButtonUp(e);
  return 0;
}

int tKeyLengthDragger::Dragging(wxMouseEvent &e)
{
  int fx, fy;

  wxDC* dc=new wxClientDC(Win);
  Win->DrawEvent(dc, Copy, Copy->GetBrush(), 1, 1);
  e.GetPosition(&fx, &fy);
  long Clock = Win->x2Clock((long)fx);
  int  Length = Clock - Copy->Clock;
  if (Length <= 0)
    Length = 1;
  Copy->Length = Length;

  Win->DrawEvent(dc, Copy, Copy->GetBrush(), 1, 1);
  return 0;
}

int tKeyLengthDragger::ButtonUp(wxMouseEvent &e)
{
  wxDC* dc=new wxClientDC(Win);
  Win->DrawEvent(dc, Copy, Copy->GetBrush(), 1, 1);
  Win->DrawEvent(dc, Copy, Copy->GetBrush(), 0, 1);

  // SN++ Key_Aftertouch
  if (Copy->Length < KeyOn->Length) {
    int key,channel;
    tEventIterator iter(Win->Track);
    tKeyPressure *a;
    key = Copy->Key;
    channel = Copy->Channel;
    tEvent *e = iter.Range(Copy->Clock+Copy->Length,Copy->Clock+KeyOn->Length);
    while (e) {
      a = e->IsKeyPressure();
      if (a)
        if (a->Key == key && a->Channel == channel) Win->Track->Kill(e);
      e = iter.Next();
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
// tPlayTrackLengthDragger JAVE this is just copied from tKeyLengthDragger, the need to be inherited somehow
// -------------------------------------------------------------------------

class tPlayTrackLengthDragger : public tMouseAction
{
    tPlayTrack    *KeyOn;
    tPlayTrack    *Copy;
    tPianoWin *Win;
    tTrack    *Track;
    wxDC      *dc;

  public:
    tPlayTrackLengthDragger(tPlayTrack *k, tPianoWin *w);
    int Dragging(wxMouseEvent &e);
    int ButtonUp(wxMouseEvent &e);
    int Event(wxMouseEvent &e);
};


tPlayTrackLengthDragger::tPlayTrackLengthDragger(tPlayTrack *k, tPianoWin *w)
{
  KeyOn = k;
  Copy  = k->Copy() -> IsPlayTrack();
  Win   = w;

  // SN++ BUG FIX: undo/redo
  Win->Song->NewUndoBuffer();
  //
  wxDC* dc=new wxClientDC(Win);
  Win->DrawEvent(dc, Copy, wxWHITE_BRUSH, 0);
  Win->DrawEvent(dc, Copy, Copy->GetBrush(), 1, 1);
}

int tPlayTrackLengthDragger::Event(wxMouseEvent &e)
{
  if (e.Dragging())		return Dragging(e);
  else if (e.ButtonUp())	return ButtonUp(e);
  return 0;
}

int tPlayTrackLengthDragger::Dragging(wxMouseEvent &e)
{
  int fx, fy;
  wxDC* dc=new wxClientDC(Win);
  Win->DrawEvent(dc, Copy, Copy->GetBrush(), 1, 1);
  e.GetPosition(&fx, &fy);
  long Clock = Win->x2Clock((long)fx);
  int  Length = Clock - Copy->Clock;
  if (Length <= 0)
    Length = 1;
  Copy->eventlength = Length; 

  Win->DrawEvent(dc, Copy, Copy->GetBrush(), 1, 1);
  return 0;
}

int tPlayTrackLengthDragger::ButtonUp(wxMouseEvent &e)
{
  wxDC* dc=new wxClientDC(Win);
  Win->DrawEvent(dc, Copy, Copy->GetBrush(), 1, 1);
  Win->DrawEvent(dc, Copy, Copy->GetBrush(), 0, 1);

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
    tPianoWin *Win;
    tKeyOn    *KeyOn;
  public:
    int Event(wxMouseEvent &e);
    tVelocCounter(tPianoWin *w, tRect *r, tKeyOn *e)
      : tMouseCounter(w, r, e->Veloc, 1, 127)
    {
      Win = w;
      KeyOn = e;

      // SN++ BUG FIX: undo/redo
      Win->Song->NewUndoBuffer();
      //
      wxDC* dc=new wxClientDC(Win);
      dc->SetFont(*(Win->FixedFont));
    }
};



int tVelocCounter::Event(wxMouseEvent &e)
{
  if (tMouseCounter::Event(e))
  {
    tKeyOn *Copy = (tKeyOn *)KeyOn->Copy();
    Copy->Veloc = Value;
    Win->Track->Kill(KeyOn);
    Win->Track->Put(Copy);
    Win->Track->Cleanup();

// SN++ und event neu zeichnen (neue Farbe nach Mouserelease darstellen).
  wxDC* dc=new wxClientDC(Win);
  Win->DrawEvent(dc, Copy, Copy->GetBrush(), 0, 1);
// SN++ veloc editor updaten
  if (Win->CtrlEdit)
           Win->CtrlEdit->UpDate();
//
    Win->MouseAction = 0;

    dc->SetFont(*(Win->Font));
    delete this;
  }
  return 0;
}




// --------------------------------------------------------------------


void tPianoWin::MouseCutPaste(wxMouseEvent &e, Bool cut)
{

  //converts physical coords to logical(scrolled) coords
  wxClientDC* scrolledDC=new wxClientDC(Canvas);
  Canvas->PrepareDC(*scrolledDC);
  wxPoint point=e.GetLogicalPosition(*scrolledDC);
  delete scrolledDC;

  long   x=point.x;
  long   y=point.y;

  long Clock = x2Clock(x);
  int  Pitch = y2Pitch(y);
  tEvent *m = FindEvent(Track, Clock, Pitch);
  if (m)
    Copy(Track, m, cut);
  else
    Paste(Track, SnapClock(Clock), Pitch);

  // allways redraw
  Redraw();
}


void tPianoWin::MouseEvents(wxMouseEvent &e)
{
  int action = MouseEvnt.Action(e);
  cout << "Mouse event: " << action << endl;
  if (action)
  {
    int fx, fy;

    e.GetPosition(&fx, &fy);
    long x = (long)fx;
    long y = (long)fy;

    long Clock = x2Clock(x);
    int  Pitch = y2Pitch(y);
    tEvent *m = FindEvent(Track, Clock, Pitch);
    tKeyOn *k = 0;
    tPlayTrack *p = 0;
    if (m){ //both these events are drag length
      k = m->IsKeyOn();
      p = m->IsPlayTrack();
    }
    switch (action)
    {
      case MA_CUTPASTE:
        MouseCutPaste(e, 1);
        break;

      case MA_COPY:
        MouseCutPaste(e, 0);
        break;

      case MA_LENGTH	:
        if (k) {
#ifdef AUDIO
	  if (!Track->GetAudioMode())
#endif
	    MouseAction = new tKeyLengthDragger(k, this);
	}
	else 
	  if(p) {
	    MouseAction = new tPlayTrackLengthDragger(p, this);
	  }
	  else
	  // event not found, maybe change to another Track
	  if (VisibleAllTracks)
	    {
	      int i;
	      for (i = 0; i < Song->nTracks; i++)
		{
		  tTrack *t = Song->GetTrack(i);
		  if (IsVisible(t) && FindEvent(t, Clock, Pitch))
		    {
		      NewPosition(i, -1L);
		      break;
		    }
		}
	    }
	break;


      case MA_DIALOG	:
	EventDialog(m, this, Track, Clock, Track->Channel - 1, Pitch);
        break;


      case MA_LISTEN	:
	MousePiano(e);
	break;

      case MA_SELECT	:
      case MA_CONTSEL	:
	tEventWin::OnMouseEvent(e);
	break;

      case MA_VELOCITY:
        if (k)
        {
	  tRect r;
	  r.x = CanvasX + LittleBit;
	  r.y = CanvasY;
	  r.width = wPiano - 2 * LittleBit;
	  r.height = hTop;

	  tVelocCounter *VelocCounter = new tVelocCounter(this, &r, k);
	  VelocCounter->Event(e);
	  MouseAction = VelocCounter;
	}
	break;

    }
  }
}

// ------------------------------------------------------------------------
// dispatch Mouseevent
// ------------------------------------------------------------------------

/** indicate which key on the pianoroll that the mouse is hovering over by highlighting it

this function is bd because it draws directly in the dc, rather it should invalidate and let OnDraw do the actual painting

currently the code dont work becuase it doesnt care about scrolling(because i get the dc the wrong way)
*/
void tPianoWin::ShowPitch(int pitch)
{
  long line = y2Line(Pitch2y(pitch)); //this is the current position of the mouse, MouseLine is the last position
  if (line >= FromLine && line != MouseLine)
  {
    wxDC* dc=new wxClientDC(Canvas);
    Canvas->PrepareDC(*dc); //to translate scrolled coordinates (doesnt work?!?)
    dc->SetLogicalFunction(wxXOR);
    //dc->SetBrush(wxBLACK_BRUSH);
    dc->SetBrush(*wxBLUE_BRUSH);
    if (MouseLine >= 0) 
      dc->DrawRectangle(xPiano, Line2y(MouseLine) + LittleBit, wPiano, hLine - 2*LittleBit); //erase the previous highlight
    MouseLine = line;
    dc->DrawRectangle(xPiano, Line2y(MouseLine) + LittleBit, wPiano, hLine - 2*LittleBit); //draw the new position
    dc->SetLogicalFunction(wxCOPY);
    delete dc;
  }
}


int tPianoWin::OnMouseEvent(wxMouseEvent &e)
{

  // aktuelle Zeile am linken Rand als Balken zeichnen

  // SN++ BUG FIX
  if (e.Moving() && !e.Dragging() && !MouseAction)
  {

    int fx, fy;
    e.GetPosition(&fx, &fy);
    int pitch = y2Pitch(long(fy));
    ShowPitch(pitch);
#ifndef __PORTING
    if (GuitarWin)
      GuitarWin->ShowPitch(pitch);
#endif // __PORTING
  }

  // dispatch

  if (!MouseAction)
  {
    int fx, fy;
    e.GetPosition(&fx, &fy);
    long x = (long)fx;
    long y = (long)fy;

    cout << "Mouse Clicker: " << x << " " << y << " " << MouseEvnt.Action(e) << endl;
    cout << "Event Bounds: " << yEvents << " " << xEvents << " " << wEvents << endl;
    cout << "Piano Bounds: " << yEvents << " " << xPiano << " " << wPiano << endl;
    if (y > yEvents)	// click in event area?
    {
      if (xPiano < x && x < xPiano + wPiano)
        MousePiano(e);
      else if (xEvents < x && x < xEvents + wEvents)
        MouseEvents(e);
      else
        tEventWin::OnMouseEvent(e);
    }

    else 		// click in top line
    if (x > xEvents)
    {
      int action = MousePlay.Action(e);

      if (action)
      {
        if (!Midi->Playing)
        {
          long Clock, LoopClock;
	  if (action == MA_CYCLE)
	  {
	    if (SnapSel->Selected)
	    {

	      Clock = Filter->FromClock;
	      LoopClock = Filter->ToClock;

	    }
	    else
	     {
	      Clock = x2BarClock((long)x, 0);
	      LoopClock = x2BarClock((long)x, 4);
	    }
	  }
	  else {
	    Clock = SnapClock(x2Clock((long)x));
	    LoopClock = 0;
	  }
	  Midi->SetRecordInfo(0);
	  Midi->StartPlay(Clock, LoopClock);
	}
	else			// Stop Record/Play
	  Midi->StopPlay();
      }
    }

  }
  else
    tEventWin::OnMouseEvent(e);

  return 0;
}



int tPianoWin::OnKeyEvent(wxKeyEvent &e)
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
        OnMenuCommand(MEN_CUT);
        return 1;
      case 'C':
      case WXK_INSERT:
        OnMenuCommand(MEN_COPY);
        return 1;
    }
  }

  else if (e.ShiftDown())
  {
    switch (e.KeyCode())
    {
      case WXK_UP:
        if (TrackNr > 0)
        {
          TrackNr --;
          NewPosition(TrackNr, -1L);
	}
	return 1;
      case WXK_DOWN:
        if (TrackNr < Song->nTracks - 1)
        {
          TrackNr ++;
          NewPosition(TrackNr, -1L);
	}
	return 1;
    }
  }

  else {
    switch(e.KeyCode()) {
      case WXK_DELETE:
        OnMenuCommand(MEN_ERASE);
        return 1;
    }
  }

  return 0;
}

// ------------------------------------------------------------------------
// Snapper
// ------------------------------------------------------------------------


void tPianoWin::SnapSelStop(wxMouseEvent &e)
{
  if (SnapSel->Selected)
  {

    long fr = y2Pitch((long)(SnapSel->r.y + SnapSel->r.height - 1));
    long to = y2Pitch((long)SnapSel->r.y + 1);


    Filter->FltEvents[FltKeyOn].Selected = VisibleKeyOn;
    Filter->FltEvents[FltKeyOn].FromValue = fr;
    Filter->FltEvents[FltKeyOn].ToValue   = to;

    Filter->FltEvents[FltPitch].Selected = VisiblePitch;
    Filter->FltEvents[FltPitch].FromValue = (fr << 7) - 8192;
    Filter->FltEvents[FltPitch].ToValue   = ((to + 1) << 7) - 8192;

    Filter->FltEvents[FltControl].Selected = VisibleController;
    Filter->FltEvents[FltControl].FromValue = fr;
    Filter->FltEvents[FltControl].ToValue   = to;

    Filter->FltEvents[FltProgram].Selected = VisibleProgram;
    Filter->FltEvents[FltProgram].FromValue = fr;
    Filter->FltEvents[FltProgram].ToValue   = to;

    Filter->FltEvents[FltTempo].Selected = VisibleTempo;
    Filter->FltEvents[FltTempo].FromValue = fr;
    Filter->FltEvents[FltTempo].ToValue   = to;

    Filter->FltEvents[FltSysEx].Selected = VisibleSysex;
    Filter->FltEvents[FltSysEx].FromValue = fr;
    Filter->FltEvents[FltSysEx].ToValue   = to;

    // SN++ Aftertouch (gehoeren to KeyOn Events).
    Filter->FltEvents[FltKeyPressure].Selected  = VisibleKeyOn;
    Filter->FltEvents[FltKeyPressure].FromValue = fr;
    Filter->FltEvents[FltKeyPressure].ToValue   = to;

    // SN++ Channel Aftertouch
    Filter->FltEvents[FltChnPressure].Selected  = VisibleMono;
    Filter->FltEvents[FltChnPressure].FromValue = fr;
    Filter->FltEvents[FltChnPressure].ToValue   = to;


    Filter->FromTrack = TrackNr;
    Filter->ToTrack   = TrackNr;
    Filter->FromClock = SnapClock(x2Clock((long)SnapSel->r.x + 1));
    Filter->ToClock   = SnapClock(x2Clock((long)(SnapSel->r.x + SnapSel->r.width + 1)));


  }

  // SN++ Veloc- oder Aftertouch-Editor updaten
  if (CtrlEdit)
           CtrlEdit->UpDate();
}



void tPianoWin::SnapSelStart(wxMouseEvent &)
{
  nSnaps = 0;
  long clk = SnapClock(FromClock, 0);
  long qnt = SnapClocks();
  while (clk <= ToClock && nSnaps < MaxSnaps)
  {
    xSnaps[nSnaps++] = Clock2x(clk);
    clk += qnt;
  }
  if (nSnaps < MaxSnaps)
    SnapSel->SetXSnap(nSnaps, xSnaps);
  else
    SnapSel->SetXSnap(0,0,0);
  SnapSel->SetYSnap(FromLine * hLine + hTop, yEvents + hEvents, hLine);
}


long tPianoWin::SnapClock(long clk, int up)
{
  long qnt = SnapClocks();
  clk -= (clk % qnt);
  if (up)
    clk += qnt;
  return clk;
}


// **************************************************************************
// Snap
// **************************************************************************


void tPianoWin::SnapDlg()
{
  tSnapDlg * dlg = new tSnapDlg(this, &SnapDenomiator);
  dlg->Create();
}


void tPianoWin::PressRadio(int id)
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
    if (*pid != id && tool_bar->GetToolState(*pid))
      tool_bar->ToggleTool(*pid, FALSE);
#ifndef wx_xt
  if (id > 0 && !tool_bar->GetToolState(id))
    tool_bar->ToggleTool(id, TRUE);
#endif
}



void tPianoWin::SetSnapDenom(long value)
{
  const int N = 4;
  const struct { int id; long val; } tab[N] = {
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
    if (tab[i].id != id && tool_bar->GetToolState(tab[i].id))
      tool_bar->ToggleTool(tab[i].id, FALSE);
#ifndef wx_xt
  if (id > 0 && !tool_bar->GetToolState(id))
    tool_bar->ToggleTool(id, TRUE);
#endif

  SnapDenomiator = value;
  //MouseEvnt.SetLeftAction(MA_CUTPASTE);
}



void tPianoWin::SetVisibleAllTracks(Bool value)
{
  tool_bar->ToggleTool(MEN_VIS_ALL_TRK, value);
  VisibleAllTracks = value;
  Redraw();
}

