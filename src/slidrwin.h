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

#ifndef slidrwin_h
#define slidrwin_h

#include "toolbar.h"

#include "wx/wx.h"

class wxToolBar;
struct tToolDef;

/**
 * a window containing a panel on the top and some ArrayEdits below
 */

class tRhyArrayEdit;

class tSliderWin : public wxFrame
{
  public:
    DECLARE_EVENT_TABLE()
    tSliderWin(wxFrame *parent, wxFrame **ref, const char *title, int geo[4], tToolDef *tdefs = NULL);
    ~tSliderWin();
    void Initialize();
    virtual void OnSize(wxSizeEvent& event);
    virtual bool OnClose();

    virtual void AddItems();
    virtual void AddEdits();
#ifndef __PORTING
    virtual void OnItem(wxItem& item, wxCommandEvent& event);
#endif // __PORTING
    virtual void ForceRepaint();
  protected:
#ifndef __PORTING
    static void ItemCallback(wxItem& item, wxCommandEvent& event);
#endif // __PORTING

    int         in_constructor;
    wxFrame    **ref;

    wxPanel       *panel;
    //tRhyArrayEdit *edits[100];
    wxWindow *sliders[100];
    int           n_sliders;
    int           sliders_per_row;
    int           *geo;

    tToolBar *tool_bar;

};

#endif

