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



#ifndef guitar_h
#define guitar_h

#ifndef wx_wxh
#include "wx/wx.h"
#endif

class tPianoWin;
class tGuitarCanvas;

class tGuitarWin : public wxFrame
{
  public:

    tGuitarWin(tPianoWin *parent);
    virtual ~tGuitarWin();
    virtual void OnSize(int w, int h);
    void Redraw();
    void ShowPitch(int pitch);
    virtual void OnMenuCommand(int Id);
    void OnHelp();
#ifndef __PORTING
    virtual bool OnClose();
#endif // __PORTING
  private:
    tGuitarCanvas *canvas;
    tPianoWin     *piano;
};

#endif

