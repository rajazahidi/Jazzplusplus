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

/**
JAVE i dont get the meaning of this class
it seems to just fork out the method calls to its delegate, EventWin
*/
/*
class tCanvas: public wxScrolledWindow
{
  public:
    JZEventFrame *EventWin;
    tCanvas(JZEventFrame *frame, int x, int y, int w, int h, int style = 0);
    void OnPaint(wxPaintEvent& event);
    //    void OnEvent(wxMouseEvent& event);
    void OnMouseEvent(wxMouseEvent& event);
    void OnChar(wxKeyEvent& event);
    bool OnCharHook(wxKeyEvent& event);
    void SetScrollRanges();
    void SetScrollPosition(long x, long y);
    void OnDraw(wxDC& dc);
    DECLARE_EVENT_TABLE()
};
*/


/**
 * JZEventFrame
 *
 * A window with Panel, Canvas, Scrollbars, Menus,
 * common baseclass for TrackWin and PianoWin.
 *
 *Panel, menu is administered by derived class
 * Funktionen
 *   - Settings dialog
 *   - Selection via Snapsel
 */


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

    tFilter* mpFilter;

    JZPianoFrame* NextWin;


    // 2) Create():
    virtual void Create();
    virtual void CreateMenu();
    void CreateCanvas();
//    tCanvas* Canvas;

    // Setup()
    wxFont* mpFont;
    wxFont* mpFixedFont; // remains with 12pt
    int hFixedFont;   // Height of letters

    long   LittleBit;
    long   hLine;

    long   hTop;
    long   wLeft;
    long   FontSize;
    long   ClocksPerPixel;
    bool   UseColors;

    // Parameters changed, e.g. Song loaded
    virtual void Setup();

    // filled by OnPaint()
    //wxDC *dc;
    long xEvents, yEvents, wEvents, hEvents;
    long CanvasX, CanvasY, CanvasW, CanvasH;	// canvas coords
    long FromClock, ToClock;
    long FromLine, ToLine;

    // Mousehandling
    tSnapSelection *SnapSel;
    tMouseAction *MouseAction;
    virtual void SnapSelStart(wxMouseEvent &e);
    virtual void SnapSelStop(wxMouseEvent &e);

    // methods
    long y2Line(long y, int up = 0);
    long y2yLine(long y, int up = 0);
    long Line2y(long line);
    void LineText(wxDC *dc, long x, long y, long w, const char *str, int h = -1, bool down = FALSE);
    long x2Clock(long x);
    long Clock2x(long clk);
    long x2BarClock(long x, int Next = 0);

    long    PlayClock;
    virtual void NewPlayPosition(long Clock);
    virtual void DrawPlayPosition(wxDC* dc);

    // sent by trackwin: scroll to Position
    virtual void NewPosition(int TrackNr, long Clock){}

    // Events
    virtual void OnPaintSub(wxDC *dc, long x, long y);
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

    virtual void GetVirtSize(long *w, long *h);

    // Edit-Menu

    // if selection active: TRUE, else: Errormessage + FALSE
    int EventsSelected(const char* msg = 0);

    void MenQuantize();
    void MenSetChannel();
    void MenTranspose();
    void MenShift(long Unit);
    void MenDelete();
    void MenVelocity();
    void MenLength();
    void MenSeqLength();
    void MenMidiDelay();
    void MenConvertToModulation();
    void MenCleanup();
    void MenSearchReplace();
    void MenMeterChange();

    void ZoomIn();
    void ZoomOut();
    
  protected:

    JZToolBar* mpToolBar;
    wxColor* mpGreyColor;
    wxBrush* mpGreyBrush;

  DECLARE_EVENT_TABLE()
};

#endif // !defined(JZ_EVENTFRAME_H)
