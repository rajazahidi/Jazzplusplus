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
#include "ToolBar.h"

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

//*****************************************************************************
// Description:
//   This is the track piano class definition.
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(JZPianoFrame, wxFrame)

//  EVT_MENU(wxID_EXIT, JZPianoFrame::OnFileExit)

//  EVT_MENU(wxID_HELP_CONTENTS, JZPianoFrame::OnHelpContents)

//  EVT_MENU(wxID_ABOUT, JZPianoFrame::OnHelpAbout)

END_EVENT_TABLE()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZPianoFrame::JZPianoFrame(
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
    mpToolBar(0)//,
//    mpFileMenu(0),
//    mpEditMenu(0)
{
  CreateToolBar();

//  CreateMenu();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZPianoFrame::~JZPianoFrame()
{
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
