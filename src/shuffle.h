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


#ifndef shuffle_h
#define shuffle_h

#ifndef eventwin_h
#include "eventwin.h"
#endif

// ***************************************************************************
// Menu-Funktionen
// ***************************************************************************

#include "proplistdlg.h"
class tShuffleDlg : public tPropertyListDlg
{
    static int  track_mode;        // 0 = all in sync, 1 = all random, 2 = one exclusive
    static bool skip_silence;      // 1
    static bool random_order;	   // 0
    static long clocks_per_segm;   // 96 * 1/16
    tNamedChoice StepList;

    tFilter *filter;
    tEventWin *win;

    wxList  modelist;
    const char * modestr;
    static const char *modetxt[];

  public:

    tShuffleDlg(tEventWin *w, tFilter *f);
    void AddProperties();
    bool OnClose();
    void OnHelp();
};

#endif

