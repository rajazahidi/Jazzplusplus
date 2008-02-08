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

#ifndef JZ_RHYTHM_H
#define JZ_RHYTHM_H

#include "Random.h"

#include "ToolBar.h"

#include <iostream>

class tTrack;
class JZEventFrame;
class JZSong;
class JZBarInfo;

#define MAX_GROUPS  5
#define MAX_KEYS   20

#define MOTIF_Y_OFFSET 16

struct tRhyGroup {
  int contrib;
  int listen;

  tRhyGroup() {
    listen = 0;
    contrib = 0;
  }
  void write(std::ostream& os);
  void read(std::istream& is, int version);
};


struct tRhyGroups {
  tRhyGroup g[MAX_GROUPS];
  tRhyGroup& operator [] (int i) { return g[i]; }
  void write(std::ostream& os);
  void read(std::istream &is, int version);
};


class tRhythm
{
    friend class tRhythmWin;

    char * label;

  protected:

    JZRndArray rhythm;
    JZRndArray length;
    JZRndArray veloc;

    int steps_per_count;
    int count_per_bar;
    int n_bars;
    int keys[MAX_KEYS];
    int n_keys;
    int mode;
    int parm;

    bool randomize;
    tRhyGroups groups;
    JZRndArray history;

    // set by GenInit()
    long start_clock;
    long next_clock;

    void GenGroup(JZRndArray &out, int grp, JZBarInfo &bi, tRhythm *rhy[], int n_rhy);
    int Clock2i(long clock, JZBarInfo &bi) const;
    int ClocksPerStep(JZBarInfo &bi) const;

  public:
    tRhythm(int key);
    tRhythm(const tRhythm &o);
    tRhythm & operator= (const tRhythm &o);
    virtual ~tRhythm();

    char const * GetLabel() { return label; }
    void SetLabel(char const *);

    void Generate(tTrack *track, long fr_clock, long to_clock, long ticks_per_bar);
    void Generate(tTrack *track, JZBarInfo &bi, tRhythm *rhy[], int n_rhy);
    void GenInit(long start_clock);
    void GenerateEvent(tTrack *track, long clock, short vel, short len);

    void write(std::ostream& os);
    void read(std::istream& is, int version);
};


class tRhythmWin : public wxFrame
{
    friend std::ostream& operator << (std::ostream& os, tRhythmWin const &a);
    friend std::istream& operator >> (std::istream& is, tRhythmWin &a);

    wxPanel    *inst_panel;
#ifdef OBSOLETE
    wxText     *label;
#endif
    wxSlider   *steps_per_count;
    wxSlider   *count_per_bar;
    wxSlider   *n_bars;
    wxListBox  *instrument_list;
    wxCheckBox *rand_checkbox;

    wxPanel    *group_panel;
    wxListBox  *group_list;
    wxSlider   *group_contrib;
    wxSlider   *group_listen;
    int        act_group;

    tArrayEdit    *length_edit;
    tArrayEdit    *veloc_edit;
    tRhyArrayEdit *rhythm_edit;

    enum       { MAX_INSTRUMENTS = 20 };
    tRhythm    *instruments[MAX_INSTRUMENTS];
    int        n_instruments;
    int        act_instrument;	// -1 if none

    // this one is edited and copied from/to instruments[i]
    tRhythm    edit;

    // ignore Updates while creating the window (motif)
    bool in_create;

    // callbacks
#ifdef OBSOLETE
    static void ItemCallback(wxItem& item, wxCommandEvent& event);
#endif
    static void SelectInstr(wxListBox& list, wxCommandEvent& event);
    static void SelectGroup(wxListBox& list, wxCommandEvent& event);
    static void Add(wxButton &but, wxCommandEvent& event);
    static void Del(wxButton &but, wxCommandEvent& event);
    static void Generate(wxButton &but, wxCommandEvent& event);
    static void Help();

    void Instrument2Win(int i = -1);	// instrument[act_instrument] -> win
    void Win2Instrument(int i = -1);	// win -> instrument[act_instrument]
    void AddInstrumentDlg();
    void AddInstrument(tRhythm *r);
    void DelInstrument();

    JZEventFrame *event_win;
    JZSong     *song;

    void RndEnable();

    char *default_filename;
    bool has_changed;
    wxToolBar* mpToolBar;
    float tb_width, tb_height;

    void UpInstrument();
    void DownInstrument();
    void InitInstrumentList();

  public:

    virtual void OnMenuCommand(int id);
    virtual void OnSize(int w, int h);
    tRhythmWin(JZEventFrame *parent, JZSong *song);
    virtual ~tRhythmWin();
    void OnPaint();
    void GenRhythm();
    bool OnClose();
};

extern tRhythmWin *rhythm_win;

#endif // !defined(JZ_RHYTHM_H)
