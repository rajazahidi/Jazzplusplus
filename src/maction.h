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


#ifndef maction_h
#define maction_h


#ifndef wx_wxh
#include "wx/wx.h"
#endif

#ifndef wx_timerh
#include <wx/timer.h>
#endif

#ifndef util_h
#include "util.h"
#endif


class tMouseMapper
{
  public:
    // actions
    // 0..2 = left/middle/right down
    // 3..5 = left/middle/right down + shift
    // 6..8 = left/middle/right down + ctrl
    // 9..11 = left/middle/right down + ctrl + shift

    tMouseMapper(const int actions[12]);
    tMouseMapper();

    enum Button { Left, Middle, Right };
    void SetAction(int code, Button but = Left, bool shift = FALSE, bool ctrl = FALSE);
    int Action(wxMouseEvent &);
    void SetLeftAction(int id = 0) { left_action = id; }

  private:
    int actions[12];
    int left_action;
};

/**
base class for mouse actions. the classes are instantiated in the mousehandler of eventwin, for example, to keep state during mouse operations, like drag and drop and so on.

the event() finction is used to determine what to do with an incoming event(normally, if the event is a drag event call the drag function of the class, and so on)
*/
class tMouseAction
{
  public:
    virtual ~tMouseAction()                     {}
    virtual int Dragging(wxMouseEvent &) 	{ return 0; };
    virtual int LeftDown(wxMouseEvent &) 	{ return 0; };
    virtual int LeftUp(wxMouseEvent &) 	        { return 0; };
    virtual int RightDown(wxMouseEvent &) 	{ return 0; };
    virtual int RightUp(wxMouseEvent &) 	{ return 0; };
    virtual int MiddleDown(wxMouseEvent &) 	{ return 0; };
    virtual int MiddleUp(wxMouseEvent &) 	{ return 0; };
    virtual int Event(wxMouseEvent &e)
    {
      if (e.Dragging())		return Dragging(e);
      else if (e.LeftDown())	return LeftDown(e);
      else if (e.LeftUp())	return LeftUp(e);
      else if (e.MiddleDown())	return MiddleDown(e);
      else if (e.MiddleUp())	return MiddleUp(e);
      else if (e.RightDown())	return RightDown(e);
      else if (e.RightUp())	return RightUp(e);
      return 0;
    }
};


/**
 Selection -    draw a rectangle with the mouse, selecting events

 this class needs to draw in the window, thus it needs acess to the device context of the
window.  This was  by storing a wxCanvas pointer in wxwin168, but wxCanvas is gone in wxwin2.
on the other hand we can now draw in all windows.


*/


class tSelection : public tMouseAction
{
  //  wxWindow *win;
  wxScrolledWindow *win;
  //  wxCanvas *Canvas;
  wxBrush  *back;
public:
  int Active;
  virtual void Snap(int &x, int &y, int drag) {}
  tRect r;
  int Selected;		// r is valid
  virtual int Dragging(wxMouseEvent &);
  virtual int Event(wxMouseEvent &e);
  virtual int ButtonDown(wxMouseEvent &);
  virtual int ButtonUp(wxMouseEvent &);
  virtual void Draw(wxDC* dc);
  virtual void Draw(wxDC* dc, long x, long y, long w, long h); // clipping
  tSelection(wxScrolledWindow* w);//wxCanvas *canvas);
  // may not be called while dragging
  void Select(tRect &rr, long x, long y, long w, long h);
  void Select(tRect &rr);
};

class tSnapSelection : public tSelection
{
protected:
  long  *xCoords, nxCoords;
  long  *yCoords, nyCoords;
  long  xMin, xMax, xStep, yMin, yMax, yStep;

public:
  tSnapSelection(wxScrolledWindow *c);
  virtual void Snap(float &x, float &y, int up);
  void SetXSnap(long ny, long *cx);
  void SetYSnap(long ny, long *cy);
  void SetXSnap(long xMin, long xMax, long xStep);
  void SetYSnap(long yMin, long yMax, long yStep);
};


class tEventWin;


/**
  tButtonLabelInterface

  Specifies an interface for displaying a text string within another widget.
  The other widget would inherit from this interface and implement the Display
  method to print the string somewhere appropriate.  The down argument
  indicates if the text should be displayed in a depressed button or a normal
  button.
*/
class tButtonLabelInterface {
 public:
  virtual void ButtonLabelDisplay(wxString text, bool down) = 0;
};


/**
  MouseCounter - let you enter numbers with left/right mouse button

*/
class tMouseCounter : public wxTimer, public tMouseAction
{
    int Min, Max, Delta;
    int Timeout;
    int Wait;	// don't inc/dec at Init
    tButtonLabelInterface *win;

    virtual int LeftDown(wxMouseEvent &);
    virtual int LeftUp(wxMouseEvent &);
    virtual int RightDown(wxMouseEvent &);
    virtual int RightUp(wxMouseEvent &);
    virtual void Notify();
    virtual void ShowValue(bool down);
  public:
    tRect r;
    int Value;
    tMouseCounter(tButtonLabelInterface *win, tRect *rec, int val, int min, int max, int wait = 0);
};


// -------------------------------------------------------------------------
// tMarkDestin - mark destination of some operation
// -------------------------------------------------------------------------

class tMarkDestin : public tMouseAction
{
  wxScrolledWindow *Canvas;
  wxFrame  *Frame;
  int ButtonDown(wxMouseEvent &);

public:
  int Aborted;
  float x, y;

  virtual int LeftDown(wxMouseEvent &);
  virtual int RightDown(wxMouseEvent &);
  tMarkDestin(wxScrolledWindow *canvas, wxFrame *frame, int left);
};

// -------------------------------------------------------------------------
// tMouseButton - simulate a 3D button
// -------------------------------------------------------------------------

class tMouseButton : public tMouseAction
{
  public:
    tMouseButton(tEventWin *win, tRect *r, const char *down, const char *up = 0);
    virtual ~tMouseButton();
    virtual int Event(wxMouseEvent &e);
  protected:
    virtual void Action() {}
  private:
    tEventWin *win;
    tRect     r;
    const char *down;
    const char *up;
};

#endif

