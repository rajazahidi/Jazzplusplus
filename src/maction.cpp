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

#include "config.h"
#include "wx/wx.h"
#pragma hdrstop

#include "maction.h"
#include "eventwin.h"

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

void tMouseMapper::SetAction(int code, Button but, Bool shift, Bool ctrl)
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

  int i = 0;	// left down
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

tSelection::tSelection(wxScrolledWindow* win)//wxCanvas *canvas)
{
  //  Canvas = canvas;
  this->win = win;//canvas->GetDC();
  Active = 0;
  Selected = 0;
  //back = wxGREY_BRUSH;
  back = new wxBrush(*new wxColor(192, 192, 192), wxSOLID);
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
#ifdef wx_msw
    Canvas->CaptureMouse();
#endif
    Active = 1;
//     wxDC *dc = new wxPaintDC(win);//Canvas->GetDC();
//     dc->SetBrush(*back);
//     dc->SetLogicalFunction(wxXOR);
    if (Selected && e.ShiftDown())
    {
      // Continue selection
      tRect rr = r;
      rr.SetNormal();
//       if (rr.width && rr.height)
// 	dc->DrawRectangle(rr.x, rr.y, rr.width, rr.height);
      Dragging(e);
    }
    else
    {
      Selected = 0;
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
//       dc->DrawRectangle(r.x, r.y, r.width, r.height);
      //Dragging(e);
    }
//     dc->SetLogicalFunction(wxCOPY);
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
    tRect r1, r2; //r1=previous rect, r2=new rect

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
#ifdef wx_msw
    Canvas->ReleaseMouse();
#endif
    Active = 0;
    r.SetNormal();
//     wxDC *dc = new wxPaintDC(win);//Canvas->GetDC();

//     dc->SetLogicalFunction(wxXOR);
//     if (r.width && r.height)
//       dc->DrawRectangle(r.x, r.y, r.width, r.height);
//     dc->SetLogicalFunction(wxCOPY);
    Selected =  (r.width > 3 && r.height > 3); //its selected only if larger than 3x3 pixels
    return 1;
  }
  win->Refresh();
  return 0;
}

/** draw the selected rectangle, normally called from OnDraw in the parent window*/
void tSelection::Draw(wxDC* dc)
{
  cout << "tSelection::Draw -----------------------------------------------------------------------------"<<endl;
  //    dc->DrawRectangle(100,100,100,100);
//   if (Selected) //we cant check for "selected" here, because...
//   {

    tRect rr = r;

    dc->DestroyClippingRegion();

      dc->SetLogicalFunction(wxXOR);
      dc->SetBrush(*back);
    
    rr.SetNormal();
    if (rr.width && rr.height)
      dc->DrawRectangle(rr.x, rr.y, rr.width, rr.height);
    dc->SetLogicalFunction(wxCOPY);
    //  }
}

/** draw, but use clipping to redruce drawing*/
void tSelection::Draw(wxDC* dc, long x, long y, long w, long h)
{
//   if (Selected)
//   {
    dc->SetClippingRegion(x, y, x+w, y+h);
    Draw(dc);
    dc->DestroyClippingRegion();
    //  }
}

/**i think this one is meant to select a rectangle and repaint it.
it did this by drawing directly in the dc. this is bad, so i tried changing it to
invalidation instead*/
void tSelection::Select(tRect &rr, long x, long y, long w, long h)
{
  // clear old rectangle
  //  Draw(x, y, w, h);
  // make new one
  r = rr;
  Selected = 1;
  //  Draw(x, y, w, h);
  win->Refresh(); //inefficient because should invvalidate only the rectangle
}

void tSelection::Select(tRect &rr)
{
  Select(rr, 0,0,3000,3000);
}

// ***********************************************************************
// tSnapSelection
// ***********************************************************************




static void SnapVec(long &x, long *Coords, int nCoords, int up)
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


static void SnapMod(long &x, long Min, long Max, long Step, int up)
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
  long x = (long)fx;
  long y = (long)fy;
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

void tSnapSelection::SetXSnap(long nx, long *cx)
{
  xCoords = cx;
  nxCoords = nx;
  xStep = 0;
}

void tSnapSelection::SetYSnap(long ny, long *cy)
{
  yCoords = cy;
  nyCoords = ny;
  yStep = 0;
}

void tSnapSelection::SetXSnap(long xmin, long xmax, long xstep)
{
  xMin = xmin;
  xMax = xmax;
  xStep = xstep;
  xCoords = 0;
}

void tSnapSelection::SetYSnap(long ymin, long ymax, long ystep)
{
  yMin = ymin;
  yMax = ymax;
  yStep = ystep;
  yCoords = 0;
}


// *************************************************************************
// tMouseCounter
// *************************************************************************

tMouseCounter::tMouseCounter(tEventWin *wwin, tRect *rr, int val, int min, int max, int wait)
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


void tMouseCounter::ShowValue(Bool down)
{
#if 0
  char buf[20];
  dc->SetBrush(wxWHITE_BRUSH);
  dc->SetPen(wxTRANSPARENT_PEN);
  if (r.w && r.h)
    dc->DrawRectangle((int)r.x, (int)r.y, (int)r.w, (int)r.h);
  dc->SetPen(wxBLACK_PEN);
  sprintf(buf, "%3d", Value);
  dc->DrawText(buf, (int)r.x, (int)r.y);
#else
  char buf[20];
  sprintf(buf, "%3d", Value);
  wxDC* dc=new wxClientDC(win);
  win->LineText(dc, (long)r.x, (long)r.y, (long)r.width, buf, (long)r.height, down);
#endif
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

tMouseButton::tMouseButton(tEventWin *win, tRect *r, const char *down, const char *up)
{
  this->win = win;
  this->r   = *r;
  if (up == 0)
    up = down;
  this->down = copystring(down);
  this->up   = copystring(up);
  wxDC* dc=new wxClientDC(win);
  win->LineText(dc, (long)r->x, (long)r->y, (long)r->width, (char *)down, (long)r->height, TRUE);
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
    win->LineText(dc, (long)r.x, (long)r.y, (long)r.width, (char *)up, (long)r.height, FALSE);
    delete this;
    return 1;
  }
  return 0;
}

