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

#ifndef JZ_TRACKWINDOW_H
#define JZ_TRACKWINDOW_H

class JZTrackFrame;
class wxFont;

enum TECounterModes
{
  eCmProgram,
  eCmBank,
  eCmVolume,
  eCmPan,
  eCmReverb,
  eCmChorus,
  eCmModes
};

class JZTrackWindow : public wxScrolledWindow
{
  public:

    JZTrackWindow(
      JZTrackFrame* pParent,
      const wxPoint& Position,
      const wxSize& Size);

    virtual ~JZTrackWindow();

    void Create();

  private:

    virtual void OnDraw(wxDC& Dc);

    void DrawCounters(wxDC& Dc);

    void LineText(
      wxDC& Dc,
      int x,
      int y,
      int w,
      const char* pString,
      int h = -1,
      bool Down = false);

    const char* CounterStr();

    int Line2y(int Line);

    int x2Clock(int x);

    int Clock2x(int Clock);

    int y2yLine(int y, int Up = 0);

  private:

    wxColor* mpGreyColor;
    wxBrush* mpGreyBrush;

    int hLine;
    int hTop;
    int wLeft;
    int mClocksPerPixel;
    int mLittleBit;
    int xEvents, yEvents, wEvents, hEvents;
    int mCanvasX, mCanvasY, mCanvasWidth, mCanvasHeight;
    int mFromClock, mToClock;
    int mFromLine, mToLine;

    int xPatch, wPatch;

    TECounterModes mCounterMode;

    wxFont* mpFixedFont;
    int mFixedFontHeight;

    int mFontSize;
    wxFont* mpFont;
};

#endif // !defined(JZ_TRACKWINDOW_H)
