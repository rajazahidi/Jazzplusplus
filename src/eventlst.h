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

#ifndef eventlst_h
#define eventlst_h

#include "wx/wx.h"

#include "random.h"
#include "slidrwin.h"
#include "filter.h"

class tTrackWin;

class MyTextWindow : public wxFrame {
public:
  MyTextWindow(wxFrame *parent, int x, int y, int w, int h);
  void OnChar(wxKeyEvent& event);
  void Create();
 protected:
  wxTextCtrl* textCtrl;
};


/** show a list of midi events in ascii form*/
class tEventList : public tSliderWin
{
  public:
    tEventList(tTrackWin *win, wxFrame **ref);
    ~tEventList();
    void OnMenuCommand(int id);
    void OnChar(wxKeyEvent& event);

    void AddItems();
    void AddEdits();

    void Scan(tFilter *f);
    bool Accept();

  private:
    static int geo[4];
    char *default_filename;
    tTrackWin *tw;


      wxTextCtrl* textwin;


    tFilter filter;
};

#endif
