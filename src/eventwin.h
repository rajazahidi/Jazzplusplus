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


#ifndef eventwin_h
#define eventwin_h


#include "wx/wx.h"

#include "filter.h"
#include "maction.h"


class tEventWin;
class tSong;
class tToolBar;
class tPianoWin;

/**
JAVE i dont get the meaning of this class
it seems to just fork out the method calls to its delegate, EventWin
*/
class tCanvas: public wxScrolledWindow //this was wxCanvas
{
  public:
    tEventWin *EventWin;
    tCanvas(tEventWin *frame, int x, int y, int w, int h, int style = 0);
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


#include "pianowin.h"


/**
 * tEventWin
 *
 * A window with Panel, Canvas, Scrollbars, Menus,
 * common baseclass for TrackWin and PianoWin.
 *
 *Panel, menu is administered by derived class
 * Funktionen
 *   - Settings dialog
 *   - Selection via Snapsel
 */


class tEventWin : public wxFrame
{

  public:
    bool OnCharHook(wxKeyEvent& event);
    void OnChar(wxKeyEvent& event);

    // 2-step initialization: 1) constructor
    tEventWin(wxFrame *frame, char *title, tSong *song, int x, int y, int width, int height );
    virtual ~tEventWin();



    tSong   *Song;


    tFilter *Filter;



    wxFrame *ParentWin;
    tPianoWin *NextWin;


    // 2) Create():
    virtual void Create();
    virtual void CreateMenu();
    void CreateCanvas();
    tCanvas *Canvas;

    // Setup()
    wxFont *Font;
    wxFont *FixedFont;	//remains with 12pt/ bleibt bei 12pt
    int    hFixedFont;	//Height letters/ Hoehe eines Buchstaben

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
    virtual int  OnMouseEvent(wxMouseEvent &e);
    virtual int  OnKeyEvent(wxKeyEvent &e); // true = processed by eventwin
        virtual void OnSize(wxSizeEvent& event);//int w, int h);
    virtual void OnMenuCommand(int id);
    virtual bool OnClose();

    // Redraw - nach Aenderungen von Parametern, kein GUI-Event
    virtual void Redraw();

    // Settings-Dialog
    wxDialog *DialogBox;
    void SettingsDialog(int piano);

    // Mixer-Dialog
    wxDialog *MixerForm;

    virtual void GetVirtSize(long *w, long *h);

    // Edit-Menu

    int EventsSelected(const char *msg = 0);	// if selection active: TRUE, else: Errormessage + FALSE
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


    
    //the event system is completely redone in wxwin2. now i wonder how subclasses behave if i define event tables in them
    //if i dont use the DECLARE_EVEN... and so on, the OnSize wont be called at all!
    DECLARE_EVENT_TABLE()
    wxTextCtrl         *m_textWindow;
  protected:

    tToolBar *tool_bar;
    wxColor *grey_color;
    wxBrush *grey_brush;

};


#endif
