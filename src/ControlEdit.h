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

#ifndef JZ_CONTROLEDIT_H
#define JZ_CONTROLEDIT_H

#include "Random.h"

class JZPianoFrame;
class tTrack;
class JZEvent;
class tCtrlEditBase;

// to access tCtrlEditBase from Buttons etc

class tCtrlPanel : public wxPanel
{
  public:
  friend class tCtrlEditBase;
    tCtrlPanel(tCtrlEditBase *e, wxWindow *parent,
              int x=-1, int y=-1, int width=-1, int height=-1, long style=0,
              char *name = "panel")
      : wxPanel(parent, x, y, width, height, style, name)
    {
      edit = e;
    }
    tCtrlEditBase *edit;
};




class tCtrlEditBase : public tArrayEditDrawBars
{
  public:
    tCtrlEditBase(int min, int max, JZPianoFrame *parent, char const *label, int xoff, int x, int y, int w, int h, int mode=0);
    virtual ~tCtrlEditBase();
    void SetSize(int xoff, int x, int y, int w, int h);
    void ReInit(tTrack *track, long FromClock, long ClocksPerPixel);

// SN++ Default = 0, 1 bedeutet der Editor arbeitet auch auf Selektionen.
//      Dieser Patch zusammen mit dem "selectable Patch" im PianoWin
//      ist fuer VelocEdit und AftertouchEdit Updates.
    int selectable;
    virtual void UpDate();
//
  protected:
    virtual int Missing()  = 0;
    virtual int IsCtrlEdit(JZEvent *e)  = 0;
    virtual int GetValue(JZEvent *e)  = 0;
    virtual JZEvent * NewEvent(long clock, int val) { return 0; }

    virtual void OnApply();
    virtual void OnRevert();
    virtual void OnEdit();
    virtual void OnBars();

    long Clock2i(long clock);
    long i2Clock(long i);
    int  Clock2Val(long clock);
    int  sticky;

// SN++
	int x_off;
    int ctrlmode;
    tTrack *track;
    long   from_clock;
    long   to_clock;
    long   i_max;
    long   clocks_per_pixel;
    JZRndArray  array;

    tArrayEdit* edit;
    JZPianoFrame* parent;
    tCtrlPanel* panel;

  private:

    void Create(JZPianoFrame* p, char const *label, int dx, int x, int y, int w, int h);

    static void Apply(wxButton &but, wxCommandEvent& event);
    static void Revert(wxButton &but, wxCommandEvent& event);
// SN++
    static void Edit(wxButton &but, wxCommandEvent& event);
    static void Bars(wxButton &but, wxCommandEvent& event);
    void DrawBars(wxDC* dc);
};

class tPitchEdit : public tCtrlEditBase
{
  public:
    tPitchEdit(JZPianoFrame* parent, char const *label, int xoff, int x, int y, int w, int h);
  protected:
    virtual int Missing();
    virtual int IsCtrlEdit(JZEvent *e);
    virtual int GetValue(JZEvent *e);
    virtual JZEvent * NewEvent(long clock, int val);
};

// SN++ Key Aftertouch
class tPolyAfterEdit : public tCtrlEditBase
{
  public:
    tPolyAfterEdit(JZPianoFrame* parent, char const *label, int xoff, int x, int y, int w, int h);
  protected:
    virtual int Missing();
    virtual int IsCtrlEdit(JZEvent *e);
    virtual int GetValue(JZEvent *e);
    virtual void OnApply();
};

// SN++ Channel Aftertouch
class tChannelAfterEdit : public tCtrlEditBase
{
  public:
    tChannelAfterEdit(JZPianoFrame* parent, char const *label, int xoff, int x, int y, int w, int h);
  protected:
    virtual int Missing();
    virtual int IsCtrlEdit(JZEvent *e);
    virtual int GetValue(JZEvent *e);
    virtual JZEvent * NewEvent(long clock, int val);
    virtual void OnApply();
    virtual void UpDate();
};

class tCtrlEdit : public tCtrlEditBase
{
  public:
    tCtrlEdit(int CtrlNum, JZPianoFrame* parent, char const *label, int xoff, int x, int y, int w, int h);
  protected:
    virtual int Missing();
    virtual int IsCtrlEdit(JZEvent *e);
    virtual int GetValue(JZEvent *e);
    virtual JZEvent * NewEvent(long clock, int val);
  private:
    int ctrl_num;
};

class tVelocEdit : public tCtrlEditBase
{
  public:

    tVelocEdit(
      JZPianoFrame* parent,
      char const* label,
      int xoff,
      int x,
      int y,
      int w,
      int h);

  protected:

    virtual int Missing();
    virtual int IsCtrlEdit(JZEvent *e);
    virtual int GetValue(JZEvent *e);
    virtual void OnApply();
};

class tTempoEdit : public tCtrlEditBase
{
  public:

    tTempoEdit(
      int min,
      int max,
      JZPianoFrame* parent,
      char const *label,
      int xoff,
      int x,
      int y,
      int w,
      int h);

  protected:

    virtual int Missing();
    virtual int IsCtrlEdit(JZEvent *e);
    virtual int GetValue(JZEvent *e);
    virtual JZEvent * NewEvent(long clock, int val);
};

#endif // !defined(JZ_CONTROLEDIT_H)
