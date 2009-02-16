//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Copyright (C) 1994-2000 Andreas Voss and Per Sigmond, all rights reserved.
// Modifications Copyright (C) 2004 Patrick Earl
// Modifications Copyright (C) 2008-2009 Peter J. Stieber
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

#ifndef JZ_MOUSEACTION_H
#define JZ_MOUSEACTION_H

#ifndef wx_timerh
#include <wx/timer.h>
#endif

#include "Rectangle.h"

#include <vector>

class JZEventWindow;

enum TEMousePlayMode
{
  eMouse,
  eSpaceBar,
  ePlayButton,
  ePlayLoopButton,
  eRecordButton
};

//*****************************************************************************
//*****************************************************************************
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

    enum Button
    {
      Left,
      Middle,
      Right
    };

    void SetAction(
      int code,
      Button but = Left,
      bool shift = false,
      bool ctrl = false);

    int Action(wxMouseEvent& MouseEvent);

    void SetLeftAction(int id = 0)
    {
      left_action = id;
    }

  private:

    int actions[12];

    int left_action;
};

//*****************************************************************************
// Description:
//   This is a base class for mouse actions.  The classes are instantiated in
// the mouse handler of the event window, for example, to keep state during
// mouse operations, like drag and drop and so on.
//   The ProcessMouseEvent() function is used to determine what to do with an
// incoming event.  Normally, if the event is a drag event, call the drag
// function of the class, and so on.
//*****************************************************************************
class tMouseAction
{
  public:

    virtual ~tMouseAction()                          {}
    virtual int Dragging(wxMouseEvent& MouseEvent)   { return 0; }
    virtual int LeftDown(wxMouseEvent& MouseEvent)   { return 0; }
    virtual int LeftUp(wxMouseEvent& MouseEvent)     { return 0; }
    virtual int RightDown(wxMouseEvent& MouseEvent)  { return 0; }
    virtual int RightUp(wxMouseEvent& MouseEvent)    { return 0; }
    virtual int MiddleDown(wxMouseEvent& MouseEvent) { return 0; }
    virtual int MiddleUp(wxMouseEvent& MouseEvent)   { return 0; }
    virtual int ProcessMouseEvent(wxMouseEvent& MouseEvent)
    {
      if (MouseEvent.Dragging())
      {
        return Dragging(MouseEvent);
      }
      else if (MouseEvent.LeftDown())
      {
        return LeftDown(MouseEvent);
      }
      else if (MouseEvent.LeftUp())
      {
        return LeftUp(MouseEvent);
      }
      else if (MouseEvent.MiddleDown())
      {
        return MiddleDown(MouseEvent);
      }
      else if (MouseEvent.MiddleUp())
      {
        return MiddleUp(MouseEvent);
      }
      else if (MouseEvent.RightDown())
      {
        return RightDown(MouseEvent);
      }
      else if (MouseEvent.RightUp())
      {
        return RightUp(MouseEvent);
      }
      return 0;
    }
};


//*****************************************************************************
// Description:
//   This is the selection class declaration.  This class selects events using
// the mouse and draws indicating the selected events.
//*****************************************************************************
class JZSelection : public tMouseAction
{
  public:

    JZSelection(wxWindow* pWindow);

    virtual ~JZSelection();

    virtual bool IsActive() const
    {
      return mActive;
    }

    virtual bool IsSelected() const
    {
      return mSelected;
    }

    virtual void SetSelected(bool Selected)
    {
      mSelected = Selected;
    }

    virtual const JZRectangle& GetRectangle() const
    {
      return mRectangle;
    }

    virtual void SetRectangle(const JZRectangle& Rectangle)
    {
      mRectangle = Rectangle;
    }

    virtual void Snap(int& x, int& y, bool drag)
    {
    }

    virtual int Dragging(wxMouseEvent& MouseEvent);

    virtual int ProcessMouseEvent(wxMouseEvent& MouseEvent);

    virtual int ButtonDown(wxMouseEvent& MouseEvent);

    virtual int ButtonUp(wxMouseEvent& MouseEvent);

    virtual void Draw(wxDC& Dc, int ScrolledX, int ScrolledY);

    // Drawing with clipping.
    virtual void Draw(
      wxDC& Dc,
      int ScrolledX,
      int ScrolledY,
      int ClipX,
      int ClipY,
      int ClipWidth,
      int ClipHeight);

    // May not be called while dragging.
    void Select(JZRectangle& rr, int x, int y, int w, int h);

    void Select(JZRectangle& Rectangle);

  private:

    bool mActive;

    // The following indicates if the rectangle is valid.
    bool mSelected;
    JZRectangle mRectangle;

    wxWindow* mpWindow;

    wxBrush* mpBackgroundBrush;
};

//*****************************************************************************
// Description:
//   This is the snap selection class declaration.
//*****************************************************************************
class JZSnapSelection : public JZSelection
{
  public:

    JZSnapSelection(wxWindow* pWindow);

    virtual void Snap(int& x, int& y, bool Up);

    void SetXSnap(int XCount, int* pXVector);

    void SetYSnap(int YCount, int* pYVector);

    void SetXSnap(int XMin, int XMax, int XStep);

    void SetYSnap(int YMin, int YMax, int YStep);

  private:

    static void SnapToVector(
      int& Coordinate,
      std::vector<int> Vector,
      bool Up);

    static void SnapMod(
      int& Coordinate,
      int Min,
      int Max,
      int Step,
      bool Up);

  protected:

    std::vector<int> mXCoordinates;

    std::vector<int> mYCoordinates;

    int mXMin, mXMax, mXStep, mYMin, mYMax, mYStep;
};


//*****************************************************************************
//  tButtonLabelInterface
//
//  Specifies an interface for displaying a text string within another widget.
//  The other widget would inherit from this interface and implement the Display
//  method to print the string somewhere appropriate.  The down argument
//  indicates if the text should be displayed in a depressed button or a normal
//  button.
//*****************************************************************************
class tButtonLabelInterface
{
  public:

    virtual ~tButtonLabelInterface()
    {
    }

    virtual void ButtonLabelDisplay(
      const wxString& Text,
      bool IsButtonDown) = 0;
};


//*****************************************************************************
//  MouseCounter - let you enter numbers with left/right mouse button
//*****************************************************************************
class tMouseCounter : public wxTimer, public tMouseAction
{
  public:

    JZRectangle r;

    int Value;

    tMouseCounter(
      tButtonLabelInterface *win,
      JZRectangle *rec,
      int val,
      int min,
      int max,
      int wait = 0);

  private:

    int Min, Max, Delta;
    int Timeout;
    int Wait;        // don't inc/dec at Init
    tButtonLabelInterface *win;

    virtual int LeftDown(wxMouseEvent& MouseEvent);
    virtual int LeftUp(wxMouseEvent& MouseEvent);
    virtual int RightDown(wxMouseEvent& MouseEvent);
    virtual int RightUp(wxMouseEvent& MouseEvent);
    virtual void Notify();
    virtual void ShowValue(bool down);
};


//*****************************************************************************
// tMarkDestin - mark destination of some operation
//*****************************************************************************
class tMarkDestin : public tMouseAction
{
  public:
    int Aborted;
    float x, y;

    virtual int LeftDown(wxMouseEvent& MouseEvent);
    virtual int RightDown(wxMouseEvent& MouseEvent);
    tMarkDestin(wxWindow* canvas, wxFrame* frame, int left);

  private:
    wxWindow *Canvas;
    wxFrame  *Frame;
    int ButtonDown(wxMouseEvent& MouseEvent);
};

//*****************************************************************************
// tMouseButton - simulate a 3D button
//*****************************************************************************
class tMouseButton : public tMouseAction
{
  public:

    tMouseButton(
      JZEventWindow* pEventWindow,
      JZRectangle* pRectangle,
      const char* pDownString,
      const char* upUpString = 0);

    virtual ~tMouseButton();

    virtual int ProcessMouseEvent(wxMouseEvent& MouseEvent);

  protected:

    virtual void Action()
    {
    }

  private:

    JZEventWindow* mpEventWindow;

    JZRectangle mRectangle;

    wxString mDownString;

    wxString mUpString;
};

#endif // !defined(JZ_MOUSEACTION_H)
