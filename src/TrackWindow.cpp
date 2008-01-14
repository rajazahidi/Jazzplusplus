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

#include "TrackWindow.h"
#include "TrackFrame.h"

#include <iostream>

using namespace std;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZTrackWindow::JZTrackWindow(
  JZTrackFrame* pParent,
  const wxPoint& Position,
  const wxSize& Size)
  : wxScrolledWindow(
      pParent,
      wxID_ANY,
      Position,
      Size,
      wxHSCROLL | wxVSCROLL | wxNO_FULL_REPAINT_ON_RESIZE),
    mpGreyColor(0),
    mpGreyBrush(0),
    hLine(0),
    hTop(40),
    wLeft(100),
    mClocksPerPixel(36),
    mLittleBit(0),
    mCanvasX(0),
    mCanvasY(0),
    mCanvasWidth(0),
    mCanvasHeight(0),
    mFromClock(0),
    mToClock(0),
    mFromLine(0),
    mToLine(0),
    xPatch(0),
    wPatch(0),
    nBars(0),
    mCounterMode(eCmProgram),
    mpFixedFont(0),
    mFixedFontHeight(0),
    mFontSize(12),
    mpFont(0)
{
#ifdef WX_MSW
  mpGreyColor = new wxColor(192, 192, 192);
#else
  mpGreyColor = new wxColor(220, 220, 220);
#endif

  mpGreyBrush = new wxBrush(*mpGreyColor, wxSOLID);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZTrackWindow::~JZTrackWindow()
{
  delete mpGreyColor;
  delete mpGreyBrush;
  delete mpFixedFont;
  delete mpFont;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZTrackWindow::Create()
{
  int x, y;

  wxDC* pDc = new wxClientDC(this);

  // dc is from Canvas
  pDc->SetFont(wxNullFont);

  delete mpFixedFont;
  mpFixedFont = new wxFont(12, wxSWISS, wxNORMAL, wxNORMAL);
  pDc->SetFont(*mpFixedFont);
  pDc->GetTextExtent("M", &x, &mFixedFontHeight);

  delete mpFont;
  mpFont = new wxFont(mFontSize, wxSWISS, wxNORMAL, wxNORMAL);
  pDc->SetFont(*mpFont);

  pDc->GetTextExtent("M", &x, &y);
  mLittleBit = (int)(x / 2);

  pDc->GetTextExtent("HXWjgi", &x, &y);
  hLine = y + mLittleBit;

  hTop = mFixedFontHeight + 2 * mLittleBit;

  pDc->GetTextExtent("99", &x, &y);
  wNumber = x + mLittleBit;

  pDc->GetTextExtent("Normal Trackname", &x, &y);
  wName = x + mLittleBit;

  pDc->GetTextExtent("m", &x, &y);
  wState = x + mLittleBit;

  pDc->GetTextExtent("999", &x, &y);
  wPatch = x + 2 * mLittleBit;

  wLeft = wNumber + wName + wState + wPatch + 1;

  cout
    << " " << wNumber
    << " " << wName
    << " " << wState
    << " " << wPatch
    << " " << wLeft
    << endl;

  UnMark();

  delete pDc;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZTrackWindow::Mark(int x, int y)
{
  Marked.SetX(x2xBar(x));
  Marked.SetY(y2yLine(y));
  Marked.SetWidth(x2wBar(x));
  Marked.SetHeight(hLine);

  wxDC* pDc = new wxClientDC(this);
  LineText(*pDc, Marked.GetX(), Marked.GetY(), Marked.GetWidth(), ">");
  delete pDc;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZTrackWindow::UnMark()
{
  Marked.SetX(-1);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZTrackWindow::OnDraw(wxDC& Dc)
{
  Dc.SetBackground(*wxWHITE_BRUSH);
  Dc.Clear();

  GetViewStart(&mCanvasX, &mCanvasY);
  GetClientSize(&mCanvasWidth, &mCanvasHeight);

  xEvents = mCanvasX + wLeft;
  yEvents = mCanvasY + hTop;
  wEvents = mCanvasWidth - wLeft;
  hEvents = mCanvasHeight - hTop;

  mFromLine = mCanvasY / hLine; 
  mToLine   = (mCanvasY + mCanvasHeight - hTop) / hLine;
  mFromClock = mCanvasX * mClocksPerPixel;
  mToClock = x2Clock(mCanvasX + mCanvasWidth);

  xNumber = mCanvasX;
  xName   = xNumber + wNumber;
  xState  = xName   + wName;
  xPatch  = xState  + wState;

//  int StopClk;

//  Dc.BeginDrawing();
//  Dc.DestroyClippingRegion();

  DrawPlayPosition(Dc);

  Dc.SetBackground(*wxWHITE_BRUSH);
  Dc.Clear();

  DrawCounters(Dc);
}

//-----------------------------------------------------------------------------
// Description:
//   Draw the "play position", by placing a vertical line where the
// "play clock" is.
//-----------------------------------------------------------------------------
void JZTrackWindow::DrawPlayPosition(wxDC& Dc)
{
#if 0
  if (!SnapSel->Active && PlayClock >= FromClock && PlayClock < ToClock)
  {
    Dc.SetBrush(*wxBLACK_BRUSH);
    Dc.SetPen(*wxBLACK_PEN);

//    Dc.SetLogicalFunction(wxXOR);

    int x = Clock2x(PlayClock);

    // Draw a line, 2 pixwels wide.
    Dc.DrawLine(x,     mCanvasY, x,     yEvents + hEvents);
    Dc.DrawLine(x + 1, mCanvasY, x + 1, yEvents + hEvents);

//    Dc.SetLogicalFunction(wxCOPY);
  }
//  if (mpNextWin)
//  {
//    mpNextWin->DrawPlayPosition(Dc);
//  }
#endif
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZTrackWindow::LineText(
  wxDC& Dc,
  int x,
  int y,
  int Width,
  const char* pString,
  int Height,
  bool Down)
{
  if (Height <= 0)
  {
    Height = hLine;
    y = y2yLine(y);
  }
  if (Width && Height)
  {
    Dc.SetBrush(*wxGREY_BRUSH);
    Dc.SetPen(*wxGREY_PEN);
#ifdef wx_msw
    Dc.DrawRectangle(x, y, Width + 1, Height + 1);
#else
    Dc.DrawRectangle(x, y, Width, Height);
#endif
    x += 1;
    y += 1;
    Width -= 2;
    Height -= 2;
    if (Down)
    {
      Dc.SetPen(*wxBLACK_PEN);
      Dc.DrawLine(x, y, x + Width, y);
      Dc.DrawLine(x, y, x,         y + Height);
      Dc.SetPen(*wxWHITE_PEN);
      Dc.DrawLine(x + Width, y,          x + Width, y + Height);
      Dc.DrawLine(x,         y + Height, x + Width, y + Height);
    }
    else
    {
      Dc.SetPen(*wxWHITE_PEN);
      Dc.DrawLine(x, y, x + Width, y);
      Dc.DrawLine(x, y, x,         y + Height);
      Dc.SetPen(*wxBLACK_PEN);
      Dc.DrawLine(x + Width, y,          x + Width, y + Height);
      Dc.DrawLine(x,         y + Height, x + Width, y + Height);
    }
    Dc.SetPen(*wxBLACK_PEN);
    x -= 2;
    y -= 2;
  }
  wxColor bg = Dc.GetTextBackground();
  Dc.SetTextBackground(*mpGreyColor);
  Dc.DrawText(pString, x + mLittleBit, y + mLittleBit);
  Dc.SetTextBackground(*wxWHITE);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZTrackWindow::DrawCounters(wxDC& Dc)
{
  int i;
  const char* pString = CounterStr();
  LineText(Dc, xPatch, mCanvasY - 1, wPatch, pString, hTop);

  Dc.SetClippingRegion(xPatch, yEvents, xPatch + wPatch, yEvents + hEvents);
  for (i = mFromLine; i < mToLine; i++)
  {
#if 0
    JZTrack* pTrack = gProject->GetTrack(i);
    if (pTrack)
    {
      char buf[20];
      int Value;
      switch (CounterMode)
      {
        case eCmProgram:
          Value = pTrack->GetPatch();
          break;
        case eCmBank:
          Value = pTrack->GetBank();
          break;
        case eCmVolume:
          Value = pTrack->GetVolume();
          break;
        case eCmPan:
          Value = pTrack->GetPan();
          break;
        case eCmReverb:
          Value = pTrack->GetReverb();
          break;
        case eCmChorus:
          Value = pTrack->GetChorus();
          break;
        default:
          Value = 0;
          break;
      }
      sprintf(buf, "%3d", Value);
      LineText(Dc, xPatch, Line2y(i), wPatch, buf);
    }
    else
#endif
    {
      LineText(Dc, xPatch, Line2y(i), wPatch, "?");
    }
  }
  Dc.DestroyClippingRegion();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
const char* JZTrackWindow::CounterStr()
{
  const char* pString;
  switch (mCounterMode)
  {
    case eCmProgram:
      pString = "Prg";
      break;
    case eCmBank:
      pString = "Bnk";
      break;
    case eCmVolume:
      pString = "Vol";
      break;
    case eCmPan:
      pString = "Pan";
      break;
    case eCmReverb:
      pString = "Rev";
      break;
    case eCmChorus:
      pString = "Cho";
      break;
    default:
      pString = "???";
      break;
  }
  return pString;
}

//-----------------------------------------------------------------------------
// Was the VLine macro
//-----------------------------------------------------------------------------
void JZTrackWindow::DrawVerticalLine(wxDC& Dc, int XPosition) const
{
  Dc.DrawLine(XPosition, mCanvasY, XPosition, yEvents + hEvents);
}

//-----------------------------------------------------------------------------
// Was the HLine macro
//-----------------------------------------------------------------------------
void JZTrackWindow::DrawHorizontalLine(wxDC& Dc, int YPosition) const
{
  Dc.DrawLine(mCanvasX, YPosition, mCanvasX + mCanvasWidth, YPosition);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZTrackWindow::x2xBar(int x)
{
  for (int i = 1; i < nBars; i++)
  {
    if (x < xBars[i])
    {
      return xBars[i - 1];
    }
  }
  return -1;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZTrackWindow::x2wBar(int x)
{
  for (int i = 1; i < nBars; i++)
  {
    if (x < xBars[i])
    {
      return xBars[i] - xBars[i - 1];
    }
  }
  return 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZTrackWindow::Line2y(int Line)
{
  return Line * hLine + hTop;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZTrackWindow::x2Clock(int x)
{
  return (x - xEvents) * mClocksPerPixel + mFromClock;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZTrackWindow::Clock2x(int Clock)
{
  return xEvents + (Clock - mFromClock) / mClocksPerPixel;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZTrackWindow::y2yLine(int y, int Up)
{
  if (Up)
  {
    y += hLine;
  }
  y -= hTop;
  y -= y % hLine;
  y += hTop;
  return y;
}
