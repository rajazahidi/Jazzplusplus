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

#ifndef arpeggio_h
#define arpeggio_h

#include "wx/wx.h"

#include "slidrwin.h"

class tRhyArrayEdit;
class tRndArray;
class tTrackWin;
class tMeasureChoice;


class tArpeggioWin : public tSliderWin
{
  public:
    tArpeggioWin(tTrackWin *win, wxFrame **ref);
    void OnMenuCommand(int id);

    void AddItems();
    void AddEdits();
#ifndef __PORTING 
    void OnItem(wxItem& item, wxCommandEvent& event);
    static void Callback(wxItem& item, wxCommandEvent& event);
#endif // __PORTING
    void Random();

  private:
    static int geo[4];
    enum {
      PITCH,
      //LENGTH,

      N_EDITS
    };
    tRndArray *arrays[N_EDITS];
    char *default_filename;

    wxSlider *step_count_slider;
    wxSlider *note_count_slider;
    wxCheckBox *is_controller_cb;
    tMeasureChoice *measure_cb;
    static int step_count;
    static int note_count;
    static bool is_controller;
    static int controller;
    static int measure;


    void Generate();
    tTrackWin *tw;

    tRhyArrayEdit **edits;
};

#endif
