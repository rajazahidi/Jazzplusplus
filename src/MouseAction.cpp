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

#include "WxWidgets.h"

#include "MouseAction.h"
#include "EventWindow.h"

using namespace std;

// -----------------------------------------------------------------
// tMouseMapper - map mouse button to Command-ID
// -----------------------------------------------------------------

tMouseMapper::tMouseMapper(const int a[12])
{
  for (int i = 0; i < 12; i++)
    actions[i] = a[i];
  left_action = 0;
}

tMouseMapper::tMouseMapper()
{
  for (int i = 0; i < 12; i++)
    actions[i] = 0;
  left_action = 0;
}

void tMouseMapper::SetAction(int code, Button but, bool shift, bool ctrl)
{
  int i = 0;
  switch (but)
  {
    case Left:
      i = 0;
      break;
    case Middle:
      i = 1;
      break;
    case Right:
      i = 2;
      break;
  }
  if (shift)
    i += 3;
  if (ctrl)
    i += 6;
  actions[i] = code;
}

int tMouseMapper::Action(wxMouseEvent& Event)
{
  if (!Event.ButtonDown())
  {
    return 0;
  }

  if (
    left_action > 0 &&
    Event.LeftDown() &&
    !Event.ShiftDown() &&
    !Event.ControlDown())
  {
    return left_action;
  }

  int i = 0;        // left down
  if (Event.MiddleDown())
  {
    i = 1;
  }
  else if (Event.RightDown())
  {
    i = 2;
  }

  if (Event.ShiftDown())
  {
    i += 3;
  }
  if (Event.ControlDown())
  {
    i += 6;
  }
  return actions[i];
}

//*****************************************************************************
// Description:
//  This is the selection class definition.
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZSelection::JZSelection(wxWindow* pWindow)
  : mActive(false),
    mSelected(false),
    mRectangle(),
    mpWindow(pWindow),
    mpBackgroundBrush(0)
{
//  mpBackgroundBrush = new wxBrush(wxColor(192, 192, 192), wxSOLID);
  mpBackgroundBrush = new wxBrush(wxColor(100, 100, 100), wxSOLID);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZSelection::~JZSelection()
{
  delete mpBackgroundBrush;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZSelection::Event(wxMouseEvent& Event)
{
  if (Event.ButtonDown())
  {
    return ButtonDown(Event);
  }
  else if (Event.ButtonUp())
  {
    return ButtonUp(Event);
  }
  else if (Event.Dragging())
  {
    return Dragging(Event);
  }
  return 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZSelection::ButtonDown(wxMouseEvent& Event)
{
  if (!mActive)
  {
    mActive = true;
    if (mSelected && Event.ShiftDown())
    {
      // Continue selection
      JZRectangle Rectangle = mRectangle;
      Rectangle.SetNormal();
      Dragging(Event);
    }
    else
    {
      mSelected = false;
      int x = Event.GetX();
      int y = Event.GetY();
      Snap(x, y, 0);
      mRectangle.x = x;
      mRectangle.y = y;
      mRectangle.width = 1;
      mRectangle.height = 1;
    }
  }
  return 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZSelection::Dragging(wxMouseEvent& Event)
{
  if (!mActive)
  {
    ButtonDown(Event);
  }

  if (mActive)
  {
    int x = Event.GetX();
    int y = Event.GetY();
    if (x < 0)
    {
      x = 0;
    }
    if (y < 0)
    {
      y = 0;
    }
    Snap(x, y, 1);

    mRectangle.width = x - mRectangle.x;
    mRectangle.height = y - mRectangle.y;
  }

  return 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZSelection::ButtonUp(wxMouseEvent& Event)
{
  if (mActive)
  {
    mActive = false;
    mRectangle.SetNormal();

    // Only select if the rectangle is larger than 3x3 pixels.
    mSelected = (mRectangle.width > 3 && mRectangle.height > 3);
    return 1;
  }

  mpWindow->Refresh();
  return 0;
}

//-----------------------------------------------------------------------------
// Description:
//   Draw the selected rectangle, normally called from OnDraw
// in the parent window.
//-----------------------------------------------------------------------------
void JZSelection::Draw(wxDC& Dc, int ScrolledX, int ScrolledY)
{
//  if (mSelected)
  {
    JZRectangle Rectangle = mRectangle;

    Dc.SetLogicalFunction(wxXOR);
    Dc.SetBrush(*mpBackgroundBrush);

    Rectangle.SetNormal();
    if (Rectangle.width && Rectangle.height)
    {
      Dc.DrawRectangle(
        Rectangle.x - ScrolledX,
        Rectangle.y - ScrolledY,
        Rectangle.width,
        Rectangle.height);
    }
    Dc.SetLogicalFunction(wxCOPY);
  }
}

//-----------------------------------------------------------------------------
// Description:
//   Draw, but use clipping to redruce drawing.
//-----------------------------------------------------------------------------
void JZSelection::Draw(
  wxDC& Dc,
  int ScrolledX,
  int ScrolledY,
  int ClipX,
  int ClipY,
  int ClipWidth,
  int ClipHeight)
{
//  if (mSelected)
  {
    Dc.SetClippingRegion(ClipX, ClipY, ClipWidth, ClipHeight);
    Draw(Dc, ScrolledX, ScrolledY);
    Dc.DestroyClippingRegion();
  }
}

//-----------------------------------------------------------------------------
//   I think this one is meant to select a rectangle and repaint it.
// It did this by drawing directly in the device context.  This is bad, so I
// tried changing it to invalidation instead.
//-----------------------------------------------------------------------------
void JZSelection::Select(JZRectangle& Rectangle, int x, int y, int w, int h)
{
  // clear old rectangle
  //  Draw(x, y, w, h);
  // make new one
  mRectangle = Rectangle;
  mSelected = true;
  //  Draw(x, y, w, h);

  // Inefficient because should invalidate only the rectangle.
  mpWindow->Refresh();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZSelection::Select(JZRectangle& Rectangle)
{
  Select(Rectangle, 0, 0, 3000, 3000);
}

//*****************************************************************************
// Description:
//   This is the snap selection class definition.
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZSnapSelection::JZSnapSelection(wxWindow* pWindow)
  : JZSelection(pWindow),
    mXCoordinates(),
    mYCoordinates(),
    mXMin(0),
    mXMax(0),
    mXStep(0), 
    mYMin(0),
    mYMax(0),
    mYStep(0)
{
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZSnapSelection::Snap(int& x, int& y, bool drag)
{
  if (!mXCoordinates.empty())
  {
    SnapToVector(x, mXCoordinates, drag);
  }
  else if (mXStep)
  {
    SnapMod(x, mXMin, mXMax, mXStep, drag);
  }

  if (!mYCoordinates.empty())
  {
    SnapToVector(y, mYCoordinates, drag);
  }
  else if (mYStep)
  {
    SnapMod(y, mYMin, mYMax, mYStep, drag);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZSnapSelection::SetXSnap(int XCount, int* pXVector)
{
  mXCoordinates.clear();
  for (int i = 0; i < XCount; ++i)
  {
    mXCoordinates.push_back(pXVector[i]);
  }
  mXStep = 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZSnapSelection::SetYSnap(int YCount, int* pYVector)
{
  mYCoordinates.clear();
  for (int i = 0; i < YCount; ++i)
  {
    mXCoordinates.push_back(pYVector[i]);
  }
  mYStep = 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZSnapSelection::SetXSnap(int XMin, int XMax, int XStep)
{
  mXMin = XMin;
  mXMax = XMax;
  mXStep = XStep;
  mXCoordinates.clear();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZSnapSelection::SetYSnap(int YMin, int YMax, int YStep)
{
  mYMin = YMin;
  mYMax = YMax;
  mYStep = YStep;
  mYCoordinates.clear();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZSnapSelection::SnapToVector(
  int& Coordinate,
  vector<int> Vector,
  bool Up)
{
  for (unsigned i = 0; i < Vector.size(); ++i)
  {
    if (Vector[i] > Coordinate)
    {
      if (Up || i == 0)
      {
        Coordinate = Vector[i];
      }
      else
      {
        Coordinate = Vector[i - 1];
      }
      return;
    }
  }
  Coordinate = Vector[Vector.size() - 1];
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZSnapSelection::SnapMod(
  int& Coordinate,
  int Min,
  int Max,
  int Step,
  bool Up)
{
  if (Coordinate <= Min)
  {
    Coordinate = Min;
    return;
  }
  if (Coordinate >= Max)
  {
    Coordinate = Max;
    return;
  }
  Coordinate -= (Coordinate - Min) % Step;
  if (Up)
  {
    Coordinate += Step;
  }
}


// *************************************************************************
// tMouseCounter
// *************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
tMouseCounter::tMouseCounter(
  tButtonLabelInterface* wwin,
  JZRectangle* Rectangle,
  int val,
  int min,
  int max,
  int wait)
{
  win = wwin;
  r = *Rectangle;
  Value = val;
  Min = min;
  Max = max;
  Timeout = 500;
  Wait = wait;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int tMouseCounter::LeftDown(wxMouseEvent& Event)
{
  Delta = Event.ShiftDown() ? 10 : 1;
  Start(Timeout);
  if (Wait)
  {
    ShowValue(TRUE);
  }
  else
  {
    Notify();
  }
  return 0;
}

int tMouseCounter::LeftUp(wxMouseEvent& Event)
{
  Stop();
  ShowValue(FALSE);
  return 1;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int tMouseCounter::RightDown(wxMouseEvent& Event)
{
  Delta = Event.ShiftDown() ? -10 :  -1;
  Start(Timeout);
  if (Wait)
  {
    ShowValue(TRUE);
  }
  else
  {
    Notify();
  }
  return 0;

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int tMouseCounter::RightUp(wxMouseEvent& Event)
{
  Stop();
  ShowValue(FALSE);
  return 1;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void tMouseCounter::Notify()
{
  Value += Delta;
  if (Value > Max)
    Value = Max;
  if (Value < Min)
    Value = Min;
  ShowValue(TRUE);
  if (Timeout > 50)
  {
    Stop();
    Timeout >>= 1;
    Start(Timeout);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void tMouseCounter::ShowValue(bool down)
{
  char buf[20];
  sprintf(buf, "%3d", Value);
  win->ButtonLabelDisplay(buf, down);
}

// -------------------------------------------------------------------------
// tMarkDestin
// -------------------------------------------------------------------------


tMarkDestin::tMarkDestin(wxWindow* canvas, wxFrame *frame, int left)
{
  wxCursor c;
  Canvas = canvas;
  Frame  = frame;
  if (left)
    c =  wxCursor(wxCURSOR_POINT_LEFT);
  else
    c =  wxCursor(wxCURSOR_POINT_RIGHT);
  Canvas->SetCursor(c);
  Aborted = 1;
  //Frame->SetStatusText("Click Destination point");
}

int tMarkDestin::ButtonDown(wxMouseEvent& Event)
{
  wxCursor c =  wxCursor(wxCURSOR_ARROW);
  Canvas->SetCursor(c);

  //converts physical coords to logical(scrolled) coords
  wxClientDC* scrolledDC=new wxClientDC(Canvas);
  Canvas->PrepareDC(*scrolledDC);
  wxPoint point = Event.GetLogicalPosition(*scrolledDC);
  delete scrolledDC;

  x=point.x;
  y=point.y;
  //  cout<<"tMarkDestin::ButtonDown "<<x<<" "<<y<<endl;
  return 1;
}

int tMarkDestin::RightDown(wxMouseEvent& Event)
{
  ButtonDown(Event);
  Aborted = 1;
  //Frame->SetStatusText("Operation aborted");
  return 1;
}

int tMarkDestin::LeftDown(wxMouseEvent& Event)
{
  ButtonDown(Event);
  Aborted = 0;
  //Frame->SetStatusText("");
  return 1;
}

//*****************************************************************************
// tMouseButton - simulate a 3D button
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
tMouseButton::tMouseButton(
  JZEventWindow* pEventWindow,
  JZRectangle* pRectangle,
  const char* pDownString,
  const char* pUpString)
  : mpEventWindow(pEventWindow),
    mRectangle(*pRectangle),
    mDownString(),
    mUpString()
{
  if (pDownString)
  {
    mDownString = pDownString;
  }

  if (pUpString)
  {
    mUpString = pUpString;
  }
  else
  {
    mUpString = mDownString;
  }

  wxClientDC Dc(mpEventWindow);

  mpEventWindow->LineText(
    Dc,
    mRectangle.x,
    mRectangle.y,
    mRectangle.GetWidth(),
    mDownString.c_str(),
    mRectangle.GetHeight(),
    true);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
tMouseButton::~tMouseButton()
{
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int tMouseButton::Event(wxMouseEvent& MouseEvent)
{
  if (MouseEvent.ButtonUp())
  {
    Action();

    wxClientDC Dc(mpEventWindow);

    mpEventWindow->LineText(
      Dc,
      mRectangle.x,
      mRectangle.y,
      mRectangle.GetWidth(),
      mUpString.c_str(),
      mRectangle.GetHeight(),
      false);

    delete this;

    return 1;
  }
  return 0;
}
