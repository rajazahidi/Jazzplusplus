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

//#include "config.h"
#include "MouseAction.h"
#include "EventWindow.h"
#include "DeprecatedStringUtils.h"

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

int tMouseMapper::Action(wxMouseEvent &e)
{
  if (!e.ButtonDown())
    return 0;

  if (left_action > 0 && e.LeftDown() &&!e.ShiftDown() && !e.ControlDown())
    return left_action;

  int i = 0;        // left down
  if (e.MiddleDown())
    i = 1;
  else
  if (e.RightDown())
    i = 2;

  if (e.ShiftDown())
    i += 3;
  if (e.ControlDown())
    i += 6;
  return actions[i];
}

//////////////////////////////////////////////////////////
//tSelection implementation

tSelection::tSelection(wxScrolledWindow* pWindow)
  : win(pWindow),
    mpBackgroundBrush(0)
{
  Active = 0;
  Selected = false;
  mpBackgroundBrush = new wxBrush(wxColor(192, 192, 192), wxSOLID);
}

tSelection::~tSelection()
{
  delete mpBackgroundBrush;
}

int tSelection::Event(wxMouseEvent &e)
{
  if (e.ButtonDown())
    return ButtonDown(e);
  else if (e.ButtonUp())
    return ButtonUp(e);
  else if (e.Dragging())
    return Dragging(e);
  return 0;
}

int tSelection::ButtonDown(wxMouseEvent &e)
{
  if (!Active)
  {
#ifdef __WXMSW__
//OBSOLETE    Canvas->CaptureMouse();
#endif
    Active = 1;
//     wxDC *dc = new wxPaintDC(win);//Canvas->GetDC();
//     Dc.SetBrush(*mpBackgroundBrush);
//     Dc.SetLogicalFunction(wxXOR);
    if (Selected && e.ShiftDown())
    {
      // Continue selection
      JZRectangle rr = r;
      rr.SetNormal();
//       if (rr.width && rr.height)
//         Dc.DrawRectangle(rr.x, rr.y, rr.width, rr.height);
      Dragging(e);
    }
    else
    {
      Selected = false;
      int x, y;
      //e.Position(&x, &y);
      wxDC *dc = new wxClientDC(win);//Canvas->GetDC();
      wxPoint point=e.GetLogicalPosition(*dc);
      delete dc;
      x=point.x;y=point.y;
      Snap(x, y, 0);
      r.x = x;
      r.y = y;
      r.width = 1;
      r.height = 1;
//       Dc.DrawRectangle(r.x, r.y, r.width, r.height);
      //Dragging(e);
    }
//     Dc.SetLogicalFunction(wxCOPY);
  }
  win->Refresh(); //invalidate
  return 0;
}


int tSelection::Dragging(wxMouseEvent &e)
{
  if (!Active)
    ButtonDown(e);

  if (Active)
  {
    int x, y;
    JZRectangle r1, r2; //r1=previous rect, r2=new rect

    wxDC *dc = new wxClientDC(win);
    wxPoint point=e.GetLogicalPosition(*dc);
    delete dc;

    x=point.x;
    y=point.y;
    if ((short)x < 0)
      x = 0;
    if ((short)y < 0)
      y = 0;
    Snap(x, y, 1);

    r1 = r;
    r1.SetNormal();

    r.width = x - r.x;
    r.height = y - r.y;

    r2 = r;
    r2.SetNormal();

    win->Refresh(TRUE, &r1); 
    win->Refresh(TRUE, &r2); 
  }
  //invalidate both old and new rect

  return 0;
}


int tSelection::ButtonUp(wxMouseEvent &e)
{
  if (Active)
  {
#ifdef __WXMSW__
//OBSOLETE    Canvas->ReleaseMouse();
#endif
    Active = 0;
    r.SetNormal();
//     wxDC *dc = new wxPaintDC(win);//Canvas->GetDC();

//     Dc.SetLogicalFunction(wxXOR);
//     if (r.width && r.height)
//       Dc.DrawRectangle(r.x, r.y, r.width, r.height);
//     Dc.SetLogicalFunction(wxCOPY);
    Selected = (r.width > 3 && r.height > 3); //its selected only if larger than 3x3 pixels
    return 1;
  }
  win->Refresh();
  return 0;
}

// Description:
//   Draw the selected rectangle, normally called from OnDraw
// in the parent window.
void tSelection::Draw(wxDC& Dc)
{
  cout
    << "tSelection::Draw ---------------------------------------------------"
    << endl;
  //    Dc.DrawRectangle(100,100,100,100);
// if (Selected) //we cant check for "selected" here, because...
// {

    JZRectangle rr = r;

    Dc.DestroyClippingRegion();

    Dc.SetLogicalFunction(wxXOR);
    Dc.SetBrush(*mpBackgroundBrush);
    
    rr.SetNormal();
    if (rr.width && rr.height)
    {
      Dc.DrawRectangle(rr.x, rr.y, rr.width, rr.height);
    }
    Dc.SetLogicalFunction(wxCOPY);
//  }
}

// Draw, but use clipping to redruce drawing
void tSelection::Draw(wxDC& Dc, int x, int y, int w, int h)
{
//   if (Selected)
//   {
    Dc.SetClippingRegion(x, y, x+w, y+h);
    Draw(Dc);
    Dc.DestroyClippingRegion();
    //  }
}

//   I think this one is meant to select a rectangle and repaint it.
// It did this by drawing directly in the device context.  This is bad, so I
// tried changing it to invalidation instead.
void tSelection::Select(JZRectangle &rr, int x, int y, int w, int h)
{
  // clear old rectangle
  //  Draw(x, y, w, h);
  // make new one
  r = rr;
  Selected = true;
  //  Draw(x, y, w, h);
  win->Refresh(); //inefficient because should invvalidate only the rectangle
}

void tSelection::Select(JZRectangle &rr)
{
  Select(rr, 0,0,3000,3000);
}

// ***********************************************************************
// tSnapSelection
// ***********************************************************************




static void SnapVec(int &x, int *Coords, int nCoords, int up)
{
  int i;
  for (i = 0; i < nCoords; i++)
  {
    if (Coords[i] > x)
    {
      if (up || i == 0)
        x = Coords[i];
      else
        x = Coords[i-1];
      return;
    }
  }
  x = Coords[nCoords - 1];
}


static void SnapMod(int &x, int Min, int Max, int Step, int up)
{
  if (x <= Min)
  {
    x = Min;
    return;
  }
  if (x >= Max)
  {
    x = Max;
    return;
  }
  x -= (x - Min) % Step;
  if (up)
    x += Step;
}



void tSnapSelection::Snap(float &fx, float &fy, int drag)
{
  int x = (int)fx;
  int y = (int)fy;
  if (xCoords)
    SnapVec(x, xCoords, nxCoords, drag);
  else if (xStep)
    SnapMod(x, xMin, xMax, xStep, drag);

  if (yCoords)
    SnapVec(y, yCoords, nyCoords, drag);
  else if (yStep)
    SnapMod(y, yMin, yMax, yStep, drag);
  fx = x;
  fy = y;
}


tSnapSelection::tSnapSelection(wxScrolledWindow *c)
  : tSelection(c)
{
  xCoords = 0;
  yCoords = 0;
  xStep = yStep = 0;
}

void tSnapSelection::SetXSnap(int nx, int *cx)
{
  xCoords = cx;
  nxCoords = nx;
  xStep = 0;
}

void tSnapSelection::SetYSnap(int ny, int *cy)
{
  yCoords = cy;
  nyCoords = ny;
  yStep = 0;
}

void tSnapSelection::SetXSnap(int xmin, int xmax, int xstep)
{
  xMin = xmin;
  xMax = xmax;
  xStep = xstep;
  xCoords = 0;
}

void tSnapSelection::SetYSnap(int ymin, int ymax, int ystep)
{
  yMin = ymin;
  yMax = ymax;
  yStep = ystep;
  yCoords = 0;
}


// *************************************************************************
// tMouseCounter
// *************************************************************************

tMouseCounter::tMouseCounter(
  tButtonLabelInterface* wwin,
  JZRectangle* rr,
  int val,
  int min,
  int max,
  int wait)
{
  win = wwin;
  r  = *rr;
  Value = val;
  Min = min;
  Max = max;
  Timeout = 500;
  Wait = wait;
}


int tMouseCounter::LeftDown(wxMouseEvent &e)
{
  Delta = e.ShiftDown() ? 10 : 1;
  Start(Timeout);
  if (Wait)
    ShowValue(TRUE);
  else
    Notify();
  return 0;
}

int tMouseCounter::LeftUp(wxMouseEvent &e)
{
  Stop();
  ShowValue(FALSE);
  return 1;
}

int tMouseCounter::RightDown(wxMouseEvent &e)
{
  Delta = e.ShiftDown() ? -10 :  -1;
  Start(Timeout);
  if (Wait)
    ShowValue(TRUE);
  else
    Notify();
  return 0;
}


int tMouseCounter::RightUp(wxMouseEvent &e)
{
  Stop();
  ShowValue(FALSE);
  return 1;
}


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


void tMouseCounter::ShowValue(bool down)
{
  char buf[20];
  sprintf(buf, "%3d", Value);
  win->ButtonLabelDisplay(buf, down);
}

// -------------------------------------------------------------------------
// tMarkDestin
// -------------------------------------------------------------------------


tMarkDestin::tMarkDestin(wxScrolledWindow* canvas, wxFrame *frame, int left)
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

int tMarkDestin::ButtonDown(wxMouseEvent &e)
{
  wxCursor c =  wxCursor(wxCURSOR_ARROW);
  Canvas->SetCursor(c);

  //converts physical coords to logical(scrolled) coords
  wxClientDC* scrolledDC=new wxClientDC(Canvas);
  Canvas->PrepareDC(*scrolledDC);
  wxPoint point=e.GetLogicalPosition(*scrolledDC);
  delete scrolledDC;

  x=point.x;
  y=point.y;
  //  cout<<"tMarkDestin::ButtonDown "<<x<<" "<<y<<endl;
  return 1;
}

int tMarkDestin::RightDown(wxMouseEvent &e)
{
  ButtonDown(e);
  Aborted = 1;
  //Frame->SetStatusText("Operation aborted");
  return 1;
}

int tMarkDestin::LeftDown(wxMouseEvent &e)
{
  ButtonDown(e);
  Aborted = 0;
  //Frame->SetStatusText("");
  return 1;
}

// -------------------------------------------------------------------------
// tMouseButton - simulate a 3D button
// -------------------------------------------------------------------------

tMouseButton::tMouseButton(
  JZEventFrame *win,
  JZRectangle* r,
  const char *down,
  const char *up)
{
  this->win = win;
  this->r   = *r;
  if (up == 0)
    up = down;
  this->down = copystring(down);
  this->up   = copystring(up);
  wxDC* dc=new wxClientDC(win);
  win->LineText(dc, r->x, r->y, r->width, (char *)down, r->height, TRUE);
}

tMouseButton::~tMouseButton()
{
  delete [] (char *)up;  // msvc is buggy!
  delete [] (char *)down;
}

int tMouseButton::Event(wxMouseEvent &e)
{
  if (e.ButtonUp())
  {
    Action();
    wxDC* dc=new wxClientDC(win);
    win->LineText(dc, r.x, r.y, r.width, (char *)up, r.height, false);
    delete this;
    return 1;
  }
  return 0;
}

