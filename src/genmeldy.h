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

#ifndef genmeldy_h
#define genmeldy_h

#include "wx/wx.h"

#include "random.h"
#include "slidrwin.h"

class tRhyArrayEdit;
class tRndArray;
class tTrackWin;
class tRhythmSliderPanel;


class tGenMelody : public tSliderWin
{
  public:
    tGenMelody(tTrackWin *win, wxFrame **ref);
    void OnMenuCommand(int id);

    void AddItems();
    void AddEdits();
#ifndef __PORTING 
    void OnItem(wxItem& item, wxCommandEvent& event);
    static void Callback(wxItem& item, wxCommandEvent& event);
#endif // __PORTING

  private:
    static int geo[4];
    enum {
      R_LENGTH,  // note length
      R_VELOC,   // veloc range
      R_INTERV,  // pitch intervals

      R_PATLEN,  // pattern length
      R_REPEAT,  // pattern repeat count
      R_VARIAT,  // pattern variations

      R_RHYTHM,  // rhythm

      N_EDITS
    };
    //tRhyArrayEdit *edits[N_EDITS];  // in tSliderWin
    tRndArray     *arrays[N_EDITS];
    char *default_filename;

    wxSlider *pitch_center_slider;
    wxSlider *pitch_range_slider;
    wxSlider *note_count_slider;
    wxCheckBox *rand_patlen_cbox;
    wxCheckBox *rhythm_intv_cbox;

    static int pitch_center;
    static int pitch_range;
    static int note_count;
    static int rand_patlen;
    static int rhythm_intv;

    void Generate();
    tTrackWin *tw;
    tRhythmSliderPanel *rhythm_panel;

    tRhyArrayEdit **edits;
};

#endif
