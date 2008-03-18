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

#include "MouseAction.h"
#include "Rectangle.h"

class JZTrackFrame;
class JZSong;
class tFilter;
class tSnapSelection;
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

enum TENumberModes
{
  eNmTrackNr,
  eNmMidiChannel,
  eNmModes
};

class JZTrackWindow : public wxScrolledWindow
{
  public:

    tFilter* mpFilter;

    tSnapSelection* mpSnapSel;

    enum TELimits
    {
      eMaxBars = 200
    };

    JZTrackWindow(
      JZTrackFrame* pParent,
      JZSong* pSong,
      const wxPoint& Position,
      const wxSize& Size);

    virtual ~JZTrackWindow();

    void Create();

    void NewPlayPosition(int Clock);

    void MousePlay(wxMouseEvent& Event, TEMousePlayMode Mode);

    int EventsSelected(const wxString& Message);

    void ZoomIn();

    void ZoomOut();

    void SetScrollRanges(const int& x, const int& y);

    void SetScrollPosition(int x, int y);

  private:

    void GetVirtualEventSize(int& Width, int& Height) const;

    void OnSize(wxSizeEvent& Event);

    void OnEraseBackground(wxEraseEvent& Event);

    void OnLeftButtonUp(wxMouseEvent& Event);

    virtual void OnDraw(wxDC& Dc);

    void Draw(wxDC& Dc);

    void DrawPlayPosition(wxDC& Dc);

    void DrawNumbers(wxDC& Dc);

    void DrawSpeed(wxDC& Dc, int Value = -1, bool Down = false);

    void DrawCounters(wxDC& Dc);

    void DrawEvents(wxDC& Dc);

    void LineText(
      wxDC& Dc,
      int x,
      int y,
      int w,
      const char* pString,
      int h = -1,
      bool Down = false);

    void Mark(int x, int y);

    void UnMark();

    const char* GetCounterString();

    const char* GetNumberString() const;

    // Was the VLine macro
    void DrawVerticalLine(wxDC& Dc, int XPosition) const;

    // Was the HLine macro
    void DrawHorizontalLine(wxDC& Dc, int YPosition) const;

    int x2xBar(int x);

    int x2wBar(int x);

    int Track2y(int Track);

    int x2Clock(int x);

    int Clock2x(int Clock);

    int x2BarClock(int x, int Next = 0);

    int y2yLine(int y, int Up = 0);

  private:

    static const int mScrollLine;

    JZSong* mpSong;

    wxColor* mpGreyColor;
    wxBrush* mpGreyBrush;

    int mTrackHeight;
    int mTopInfoHeight;
    int mLeftInfoWidth;
    int mClocksPerPixel;
    int mPlayClock;
    bool mUseColors;
    int mLittleBit;
    int mEventsX, mEventsY, mEventsWidth, mEventsHeight;
    int mScrolledX, mScrolledY;
    int mCanvasWidth, mCanvasHeight;
    int mFromClock, mToClock;
    int mFromLine, mToLine;

    // The values indicate the staring postions and widths of the track fields
    // on the left hand side of the screen.  Note that the position of the
    // first field displayed is always 0, so it doesn't need to be recorded.
    int mNumberWidth;
    int mTrackNameX, mTrackNameWidth;
    int mStateX, mStateWidth;
    int mPatchX, mPatchWidth;

    int mBarCount;
    int mBarX[eMaxBars];

    TECounterModes mCounterMode;
    TENumberModes mNumberMode;

    wxFont* mpFixedFont;
    int mFixedFontHeight;

    int mFontSize;
    wxFont* mpFont;

    bool mPreviouslyRecording;
    int mPreviousClock;

    JZRectangle Marked;

    bool mDrawing;

    wxBitmap* mpFrameBuffer;

  DECLARE_EVENT_TABLE()
};

#endif // !defined(JZ_TRACKWINDOW_H)
