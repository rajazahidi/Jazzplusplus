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


#ifndef toolbar_h
#define toolbar_h


#ifndef wx_wxh
#include "wx/wx.h"
#endif

#ifndef wx_bbarh
//#include "wx_bbar.h"
#endif

/**
this struct is used to initialize the tToolBar class, which is a wxToolBar wrapper
*/
struct tToolDef
{
  int  id;
  bool sticky;
  int  sep;
  const void *resid;
  const char *tooltip;
};


//porting note:  wxToolBar should be sufficient in wxwin 2
//also: i no longer subclass, because to test if it works better with letting the frame construct the bar

#ifdef wx_msw
class tToolBar : public wxButtonBar
#else
class tToolBar //: public wxToolBar
#endif
{
  public:
    tToolBar(wxFrame *frame, tToolDef *tdef, int ndefs);
    bool OnLeftClick(int toolIndex, bool toggled);
    void OnMouseEnter(int toolIndex);
    wxToolBar* GetToolBar();
  private:
    wxFrame *win;
    wxToolBar* toolbar;
};

#endif

