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

#ifndef JZ_EVENTFRAME_H
#define JZ_EVENTFRAME_H

#include "Filter.h"
#include "MouseAction.h"

class JZEventFrame;
class JZSong;
class JZToolBar;
class JZPianoFrame;

//*****************************************************************************
// Description:
//   This class is derived from a wxWidgets scrolled window, and acts as the
// common base class for JZTrackWindow and JSPianoWindow.
//*****************************************************************************
class JZEventWindow : public wxWindow
{
  public:

    JZSnapSelection* mpSnapSel;

    JZFilter* mpFilter;

    JZEventWindow(
      wxFrame* pParent,
      JZSong* pSong,
      const wxPoint& Position = wxDefaultPosition,
      const wxSize& Size = wxDefaultSize);

    virtual ~JZEventWindow();

    // WARNING: non-constant access.
    JZSong* GetSong() const;

    int EventsSelected(const wxString& Message) const;

    void LineText(
      wxDC& Dc,
      int x,
      int y,
      int Width,
      const char* pString,
      int Height = -1,
      bool Down = false);

    //========================================
    // Coordinate conversion member functions.
    //========================================

    int x2Clock(int x);

    int Clock2x(int Clock);

    int x2BarClock(int x, int Next = 0);

  protected:

    virtual void SnapSelStop(wxMouseEvent& Event)
    {
    }

    void DrawVerticalLine(wxDC& Dc, int XPosition) const;

    void DrawHorizontalLine(wxDC& Dc, int YPosition) const;

    virtual void GetVirtualEventSize(int& EventWidth, int& EventHeight) const;

    virtual void SetXScrollPosition(int x);

    virtual void SetYScrollPosition(int y);

    int y2yLine(int y, int Up = 0);

  protected:

    JZSong* mpSong;

    wxColor* mpGreyColor;
    wxBrush* mpGreyBrush;

    int mClockTicsPerPixel;
    int mTopInfoHeight;
    int mLeftInfoWidth;
    int mTrackHeight;
    int mLittleBit;

    int mEventsX, mEventsY, mEventsWidth, mEventsHeight;
    int mCanvasWidth, mCanvasHeight;
    int mFromClock, mToClock;
    int mFromLine, mToLine;

    int mScrolledX, mScrolledY;

//  DECLARE_EVENT_TABLE()
};

//*****************************************************************************
// Description:
//   These are the event window inline member functions.
//*****************************************************************************
//-----------------------------------------------------------------------------
// WARNING: non-constant access.
//-----------------------------------------------------------------------------
inline
JZSong* JZEventWindow::GetSong() const
{
  return mpSong;
}

//*****************************************************************************
// Description:
//   A frame window that containes a scrolled event window.  Acts as the
// common base class for JZTrackFrame and JSPianoFrame.
//
// The panel and menu are administered by derived classes.
// Functionality:
//   - Settings dialog
//   - Selection via Snapsel
//*****************************************************************************
class JZEventFrame : public wxFrame
{
  public:

    bool OnCharHook(wxKeyEvent& event);
    void OnChar(wxKeyEvent& event);

    // 2-step initialization: 1) constructor
    JZEventFrame(
      wxWindow* pParent,
      const wxString& Title,
      JZSong* pSong,
      const wxPoint& Position = wxDefaultPosition,
      const wxSize& Size = wxDefaultSize);

    virtual ~JZEventFrame();

    JZSong* Song;

    JZFilter* mpFilter;

    // 2) Create():
    virtual void Create();
    virtual void CreateMenu();
    void CreateCanvas();
//    JZEventWindow* mpEventWindow;

    // Setup()
    wxFont* mpFixedFont; // remains with 12pt
    int hFixedFont;   // Height of letters

    int mTrackHeight;

    int mTopInfoHeight;
    int FontSize;
    int ClocksPerPixel;

    // Parameters changed, e.g. Song loaded
    virtual void Setup();

    int mEventsX, mEventsY, mEventsWidth, mEventsHeight;
    int CanvasX, CanvasY, CanvasW, CanvasH;        // canvas coords
    int FromClock, ToClock;
    int FromLine, ToLine;

    // Mousehandling
    JZSnapSelection *SnapSel;
    tMouseAction *MouseAction;
    virtual void SnapSelStart(wxMouseEvent &e);
    virtual void SnapSelStop(wxMouseEvent &e);

    // methods
    int y2Line(int y, int up = 0);
    int y2yLine(int y, int up = 0);
    int Line2y(int line);
//    void LineText(wxDC *dc, int x, int y, int w, const char *str, int h = -1, bool down = false);

    int PlayClock;

    // Events
    virtual int  OnMouseEvent(wxMouseEvent& Event);
    virtual bool OnKeyEvent(wxKeyEvent& Event); // true = processed by eventwin
    virtual void OnSize(wxSizeEvent& Event);
    virtual void OnMenuCommand(int id);
    virtual bool OnClose();

    // Redraw - nach Aenderungen von Parametern, kein GUI-Event
    virtual void Redraw();

    // Settings-Dialog
    wxDialog* mpSettingsDialog;
    void SettingsDialog(int piano);

    // Mixer-Dialog
    wxDialog* MixerForm;

    // Edit-Menu

    // if selection active: TRUE, else: Errormessage + FALSE
    int EventsSelected(const char* msg = 0);

    void MenQuantize();
    void MenSetChannel();
    void MenTranspose();
    void MenShift(int Unit);
    void MenDelete();
    void MenVelocity();
    void MenLength();
    void MenSeqLength();
    void MenMidiDelay();
    void MenConvertToModulation();
    void MenCleanup();
    void MenSearchReplace();
    void MenMeterChange();

  protected:

    JZToolBar* mpToolBar;
    wxColor* mpGreyColor;
    wxBrush* mpGreyBrush;

  DECLARE_EVENT_TABLE()
};

#endif // !defined(JZ_EVENTFRAME_H)
