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

#include "PianoWindow.h"
#include "PianoFrame.h"

#define ScLine 50L
#define ScPage 8L

//*****************************************************************************
// Description:
//   This is the piano window definition.
//*****************************************************************************
BEGIN_EVENT_TABLE(JZPianoWindow, wxScrolledWindow)

  EVT_SIZE(JZPianoWindow::OnSize) 

  EVT_MOUSE_EVENTS(JZPianoWindow::OnMouseEvent)

END_EVENT_TABLE()

JZPianoWindow::JZPianoWindow(
  JZPianoFrame* pPianoFrame,
  const wxPoint& Position,
  const wxSize& Size,
  long WindowStyle)
  : wxScrolledWindow(pPianoFrame, wxID_ANY, Position, Size, WindowStyle),
    mpPianoFrame(pPianoFrame),
    mCanvasX(0),
    mCanvasY(0),
    mCanvasWidth(0),
    mCanvasHeight(0)
{
}

/**
JAVE seems to want to clip the paint area
calls the subclass paint routine

onpaint seems never to get called
*/

void JZPianoWindow::OnDraw(wxDC& Dc)
{
  // OnPaint never seems to get called, but OnDraw does get called.
  int x = 0, y = 0;
  GetViewStart(&x, &y);
  mpPianoFrame->OnPaintSub(&Dc, x * ScLine, y * ScLine);  
}


void JZPianoWindow::OnSize(wxSizeEvent& Event)
{
  GetClientSize(&mCanvasWidth, &mCanvasHeight);
  if (mCanvasWidth && mCanvasHeight)
  {
    Refresh();
  }
}

/**
   this mouse handler delegates to the subclased eventwin
 */
void JZPianoWindow::OnMouseEvent(wxMouseEvent &Event)
{
  mpPianoFrame->OnMouseEvent(Event);
}

bool JZPianoWindow::OnCharHook(wxKeyEvent& Event)
{
  return mpPianoFrame->OnKeyEvent(Event);
}

void JZPianoWindow::SetScrollRanges()
{
  int w, h;
  mpPianoFrame->GetVirtSize(&w, &h);
  SetScrollbars(ScLine, ScLine, w/ScLine, h/ScLine, ScPage, ScPage);
  EnableScrolling(FALSE, FALSE);
}

void JZPianoWindow::SetScrollPosition(int x, int y)
{
  x /= ScLine;
  y /= ScLine;
  Scroll(x, y);
}
