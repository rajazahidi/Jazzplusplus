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

#ifndef mapper_h
#define mapper_h

#include "wx/wx.h"
#include "random.h"
#include <iostream>
using namespace std;

class tSong;
class tTrack;
class tEventWin;

class tRhythmSliderPanel : public wxPanel
{
  public:
    tRhythmSliderPanel(wxWindow *parent, tRhyArrayEdit *edit, int x, int y, int w, int h);
    int StepsPerCount() { return steps_per_count; }
    int CountPerBar() { return count_per_bar; }
    int NBars() { return n_bars; }
    void OnPaint();
    bool Enable(bool enable);
    void AddEdit(tRhyArrayEdit *edit) {
      edits[n_edits++] = edit;
    }
  private:
    bool       enabled;
    bool       in_create;
    wxSlider   *steps_per_count_slider;
    wxSlider   *count_per_bar_slider;
    wxSlider   *n_bars_slider;
    int 	steps_per_count;
    int 	count_per_bar;
    int 	n_bars;
    tRhyArrayEdit *edits[20];
    int         n_edits;
#ifndef __PORTING 
    static void ItemCallback(wxItem& item, wxCommandEvent& event);
#endif // __PORTING
    void ItemChanged();
};


class tMapperWin : public wxFrame
{
    friend ostream & operator << (ostream &os, tMapperWin const &a);
    friend istream & operator >> (istream &is, tMapperWin &a);

    wxPanel    *panel;
    wxStaticText     *label;
    wxListBox  *source;
    wxListBox  *destin;
    wxCheckBox *mode;
    wxButton   *apply;

    tRndArray array;
    tRhyArrayEdit *edit;
    tRhythmSliderPanel *sliders;

    // ignore Updates while creating the window (motif)
    bool in_create;

    // callbacks
#ifndef __PORTING 
    static void ItemCallback(wxItem& item, wxCommandEvent& event);
#endif // __PORTING
    void ItemChanged();
    void ApplyPressed();

    tEventWin *event_win;
    tSong     *song;

  public:

    virtual void OnMenuCommand(int id);
    tMapperWin(tEventWin *parent, tSong *song);
    virtual ~tMapperWin();
    void Help();
    void OnPaint();
    bool OnClose() { return TRUE; }
};


#define MOTIF_Y_OFFSET 16

extern tMapperWin *mapper_win;

#endif

