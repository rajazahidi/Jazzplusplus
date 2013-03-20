//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Copyright (C) 1994-2000 Andreas Voss and Per Sigmond, all rights reserved.
// Modifications Copyright (C) 2004 Patrick Earl
// Modifications Copyright (C) 2008-2013 Peter J. Stieber
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

#pragma once

#include <wx/scrolwin.h>

#include <iostream>

#include "DynamicArray.h"


class JZRandomGenerator
{
  public:

    double asDouble();
};

extern JZRandomGenerator rnd;

#undef min
#undef max


// array of probabilities

class JZRndArray
{
  friend class JZArrayEdit;

  protected:

    JZIntArray mArray;
    int n;        // number of elements in array
    int nul, min, max;

  public:

    int Null()
    {
      return nul;
    }
    void SetNull(int n)
    {
      nul = n;
    }
    JZRndArray(int n, int min, int max);
    JZRndArray & operator = (const JZRndArray &);
    JZRndArray(JZRndArray const &);

    virtual ~JZRndArray();
    int &operator[] (int i)
    {
      return mArray[i];
    }
    int  operator[] (int i) const
    {
      return mArray[i];
    }
    /* PAT - The following ifdef was removed due to changes in gcc 3.x.  If it
       needs to be put back for compatibility purposes, it will need to return
       in an alternate form. */
    /*#ifdef FOR_MSW*/
    double operator[](double f);
    float operator[](float f)
    {
      /*#else
    double operator[](double f) const;
    float operator[](float f) const
    {
    #endif*/
      return (float)operator[]((double)f);
    }
    int Size() const
    {
      return n;
    }
    int Min() const
    {
      return min;
    }
    int Max() const
    {
      return max;
    }
    void SetMinMax(int min, int max);
    void Resize(int nn)
    {
      n = nn;
    }

    friend std::ostream & operator << (std::ostream &, JZRndArray const &);
    friend std::istream & operator >> (std::istream &, JZRndArray &);

    int Random();        // returns index 0..n-1 (arrayvalues -> empiric distribution)
    int Random(double rndval);        // returns index 0..n-1 (arrayvalues -> empiric distribution)
    int Random(int i);  // return 0/1
    int Interval(int seed);

    void SetUnion(JZRndArray &o, int fuzz);
    void SetDifference(JZRndArray &o, int fuzz);
    void SetIntersection(JZRndArray &o, int fuzz);
    void SetInverse(int fuzz);
    int Fuzz(int fuzz, int v1, int v2) const;
    void Clear();
};


#define ARED_GAP            1
#define ARED_XTICKS         2
#define ARED_YTICKS         4
#define ARED_MINMAX         8
#define ARED_RHYTHM        16
#define ARED_BLOCKS        32
#define ARED_LINES         64


class JZArrayEditDrawBars
{
  public:

    virtual ~JZArrayEditDrawBars()
    {
    }

    virtual void DrawBars(wxDC& Dc) = 0;
};


class JZArrayEdit : public wxScrolledWindow
{
  protected:

    // paint position
    int x, y, w, h, ynul;
    void DrawBar(wxDC& Dc, int i, int black);

    int dragging;                // Dragging-Event valid
    int index;                // ctrl down: drag this one

    JZRndArray& mArray;
    int &n, &min, &max, &nul;        // shorthand for mArray.n, mArray.min, ...
    wxString mLabel;
    JZArrayEditDrawBars *draw_bars;

    // array size is mapped to this range for x-tick marks
    int xmin, xmax;

    virtual void DrawXTicks(wxDC& Dc);
    virtual void DrawYTicks(wxDC& Dc);
    virtual void DrawLabel(wxDC& Dc);
    virtual void DrawNull(wxDC& Dc);
    int Index(wxMouseEvent& MouseEvent);

    int enabled;
    int mStyleBits;

    virtual const char *GetXText(int xval);  // Text for x-tickmarks
    virtual const char *GetYText(int yval);  // Text for y-tickmarks

  public:

    JZArrayEdit(
      wxFrame* pParent,
      JZRndArray& Array,
      int xx,
      int yy,
      int ww,
      int hh,
      int StyleBits = (ARED_GAP | ARED_XTICKS));

    virtual ~JZArrayEdit();

    virtual void OnDraw(wxDC& Dc);
    virtual void OnSize(wxSizeEvent& event);
    virtual void OnMouseEvent(wxMouseEvent& MouseEvent);
    virtual int Dragging(wxMouseEvent& MouseEvent);
    virtual int ButtonDown(wxMouseEvent& MouseEvent);
    virtual int ButtonUp(wxMouseEvent& MouseEvent);

    virtual void SetLabel(char const *llabel);
    void Enable(int enable = 1);
    void SetStyle(int StyleBits)
    {
      mStyleBits = StyleBits;
    }
    // min and max value in array (both values inclusive)
    void SetYMinMax(int min, int max);
    // for display x-axis only, does not resize the array (both values inclusive)
    void SetXMinMax(int xmin, int xmax);
    void DrawBarLine (wxDC& Dc, int xx);
    void SetDrawBars(JZArrayEditDrawBars *x)
    {
      draw_bars = x;
    }
    void Init()
    {
    }

  DECLARE_EVENT_TABLE()
};



class JZRhyArrayEdit : public JZArrayEdit
{
  public:

    JZRhyArrayEdit(
      wxFrame *parent,
      JZRndArray& Array,
      int xx,
      int yy,
      int ww,
      int hh,
      int StyleBits = (ARED_GAP | ARED_XTICKS | ARED_RHYTHM));

    void SetMeter(int steps_per_count, int count_per_bar, int n_bars);

  protected:

    virtual void DrawXTicks(wxDC& Dc);

  private:

    int steps_per_count;
    int count_per_bar;
    int n_bars;
};
