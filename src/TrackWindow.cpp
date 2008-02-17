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
#include "Project.h"
#include "Player.h"
#include "RecordingInfo.h"
#include "Globals.h"

#include <iostream>
#include <sstream>
#include <iomanip>

using namespace std;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(JZTrackWindow, wxScrolledWindow)
  EVT_SIZE(JZTrackWindow::OnSize)
END_EVENT_TABLE()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZTrackWindow::JZTrackWindow(
  JZTrackFrame* pParent,
  JZSong* pSong,
  const wxPoint& Position,
  const wxSize& Size)
  : wxScrolledWindow(
      pParent,
      wxID_ANY,
      Position,
      Size,
      wxHSCROLL | wxVSCROLL | wxNO_FULL_REPAINT_ON_RESIZE),
    mpSong(pSong),
    mpFilter(0),
    mpGreyColor(0),
    mpGreyBrush(0),
    hLine(0),
    hTop(40),
    wLeft(100),
    mClocksPerPixel(36),
    mPlayClock(-1),
    mUseColors(true),
    mLittleBit(0),
    mCanvasX(0),
    mCanvasY(0),
    mCanvasWidth(0),
    mCanvasHeight(0),
    mFromClock(0),
    mToClock(0),
    mFromLine(0),
    mToLine(0),
    mpSnapSel(0),
    xPatch(0),
    wPatch(0),
    nBars(0),
    mCounterMode(eCmProgram),
    mNumberMode(eNmMidiChannel),
    mpFixedFont(0),
    mFixedFontHeight(0),
    mFontSize(12),
    mpFont(0),
    mPreviouslyRecording(false),
    mPreviousClock(0)
{
#ifdef __WXMSW__
  mpGreyColor = new wxColor(192, 192, 192);
#else
  mpGreyColor = new wxColor(220, 220, 220);
#endif

  mpGreyBrush = new wxBrush(*mpGreyColor, wxSOLID);

  mpSnapSel = new tSnapSelection(this);

  mpFilter = new tFilter(mpSong);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZTrackWindow::~JZTrackWindow()
{
  delete mpGreyColor;
  delete mpGreyBrush;
  delete mpFixedFont;
  delete mpFont;
  delete mpSnapSel;
  delete mpFilter;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZTrackWindow::Create()
{
  int x, y;

  wxDC* pDc = new wxClientDC(this);

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

//   Update the play position to the clock argument, and trigger a redraw so
// the play bar will be drawn.
void JZTrackWindow::NewPlayPosition(long Clock)
{
  long scroll_clock = (mFromClock + 5 * mToClock) / 6L;

  if (
    !mpSnapSel->Active &&
    ((Clock > scroll_clock) || (Clock < mFromClock)) && (Clock >= 0L))
  {
    // Avoid permanent redraws when end of scroll range is reached.
    if (
      Clock > mFromClock &&
      mToClock >= mpSong->MaxQuarters * mpSong->TicksPerQuarter)
    {
      return;
    }
//    long x = Clock2x(Clock);
//    Canvas->SetScrollPosition(x - wLeft, CanvasY);
  }

  if (!mpSnapSel->Active)	// sets clipping
  {
    if (mPlayClock != Clock)
    {
      long OldPlayClock = mPlayClock;
      mPlayClock = Clock;
      wxRect invalidateRect;
      invalidateRect.x = Clock2x(OldPlayClock) - 1;
      invalidateRect.y = mCanvasY;
      invalidateRect.width = 3;
      invalidateRect.height= 100000000;
      //DrawPlayPosition();
      Refresh(true, &invalidateRect);

      invalidateRect.x = Clock2x(mPlayClock) - 1;
      Refresh(true, &invalidateRect);
      //DrawPlayPosition();

      Refresh();
    }
  }
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
void JZTrackWindow::OnSize(wxSizeEvent& Event)
{
  GetClientSize(&mCanvasWidth, &mCanvasHeight);
  if (mCanvasWidth && mCanvasHeight)
  {
    Refresh();
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZTrackWindow::OnDraw(wxDC& Dc)
{
  GetViewStart(&mCanvasX, &mCanvasY);
  GetClientSize(&mCanvasWidth, &mCanvasHeight);

  xEvents = mCanvasX + wLeft;
  yEvents = mCanvasY + hTop;
  wEvents = mCanvasWidth - wLeft;
  hEvents = mCanvasHeight - hTop;

  mFromLine = mCanvasY / hLine; 
  mToLine = 1 + (mCanvasY + mCanvasHeight - hTop) / hLine;
  mFromClock = mCanvasX * mClocksPerPixel;
  mToClock = x2Clock(mCanvasX + mCanvasWidth);

  xNumber = mCanvasX;
  xName   = xNumber + wNumber;
  xState  = xName   + wName;
  xPatch  = xState  + wState;

//  int StopClk;

  Dc.DestroyClippingRegion();

  Dc.SetBackground(*wxWHITE_BRUSH);
  Dc.Clear();

  Dc.SetPen(*wxBLACK_PEN);

  // Draw the vertical lines.
  DrawVerticalLine(Dc, xNumber);
  DrawVerticalLine(Dc, xName);
  DrawVerticalLine(Dc, xState);
  DrawVerticalLine(Dc, xPatch);

  // SN+ Dc.VLine(xEvents);
  DrawVerticalLine(Dc, xEvents - 1);
  DrawHorizontalLine(Dc, yEvents);
  DrawHorizontalLine(Dc, yEvents - 1);

  if (mpSong)
  {
    JZBarInfo BarInfo(mpSong);
    BarInfo.SetClock(mFromClock);
//    StopClk = x2Clock(mCanvasX + mCanvasWidth);
    nBars = 0;
    int intro = gpProject->GetIntroLength();
    Dc.SetPen(*wxGREY_PEN);
    while (1)
    {
      int x = Clock2x(BarInfo.Clock);
      if (x > mCanvasX + mCanvasWidth)
      {
        break;
      }
      if (x >= xEvents)   // so ne Art clipping
      {
        // SN+-      if ((BarInfo.BarNr % 4) == 0)
        int c;
        if (mClocksPerPixel > 48)
        {
          c = 8;
        }
        else
        {
          c = 4;
        }
        if (((BarInfo.BarNr - intro + 96) % c) == 0)
        {
          Dc.SetPen(*wxBLACK_PEN);
          ostringstream Oss;
          Oss << BarInfo.BarNr + 1 - intro;
          Dc.DrawText(Oss.str().c_str(), x + mLittleBit, yEvents - hLine);
          Dc.SetPen(*wxGREY_PEN);
          Dc.DrawLine(x, yEvents + 1 - hLine, x, yEvents + hEvents);
        }
        else
        {
          Dc.SetPen(*wxLIGHT_GREY_PEN);
          Dc.DrawLine(x, yEvents + 1, x, yEvents + hEvents);
        }

        // x-coordinate for MouseAction->Snap()
        if (nBars < eMaxBars)
        {
          xBars[nBars++] = x;
        }
      }
      BarInfo.Next();
    }
    Dc.SetPen(*wxBLACK_PEN);
  }

  // For each track show the num, name, state, prg.
  Dc.SetClippingRegion(
    mCanvasX,
    yEvents,
    mCanvasX + mCanvasWidth,
    yEvents + hEvents);
  int TrackNumber = mFromLine;
  for (int y = Line2y(TrackNumber); y < yEvents + hEvents; y += hLine)
  {
    // SN+    Dc.HLine(y);
    Dc.SetPen(*wxGREY_PEN);
    Dc.DrawLine(xEvents + 1, y, mCanvasX + mCanvasWidth, y);
    Dc.SetPen(*wxBLACK_PEN);
    Dc.DrawLine(mCanvasX, y, xEvents, y);

    tTrack* pTrack = gpProject->GetTrack(TrackNumber);
    if (pTrack)
    {
      // TrackName, show the button pressed when dialog is open
      //Dc.DrawText(pTrack->GetName(), xName + mLittleBit, y + mLittleBit);
      if (pTrack->DialogBox)
      {
        LineText(Dc, xName, y, wName, pTrack->GetName(), -1, true);
      }
      else
      {
        LineText(Dc, xName, y, wName, pTrack->GetName(), -1, false);
      }

      // TrackStatus
      //Dc.DrawText(pTrack->GetStateChar(), xState + mLittleBit, y + mLittleBit);
      LineText(Dc, xState, y, wState, pTrack->GetStateChar());
    }
    ++TrackNumber;
  }
  Dc.DestroyClippingRegion();

  DrawNumbers(Dc);
  DrawSpeed(Dc);
  DrawCounters(Dc);

  LineText(Dc, xState, mCanvasY - 1, wState, "", hTop);

  DrawEvents(Dc);

  if (Marked.x > 0)
  {
    LineText(Dc, (long)Marked.x, (long)Marked.y, (long)Marked.width, ">");
  }
  Dc.DestroyClippingRegion();
  DrawPlayPosition(Dc);

  // Draw the selection box.
  mpSnapSel->Draw(Dc, xEvents, yEvents, wEvents, hEvents);
}

//-----------------------------------------------------------------------------
// Description:
//   This function draws the "numbers" column (leftmost one), which either
// represents track numbers or midi channel depending on mode.
//-----------------------------------------------------------------------------
void JZTrackWindow::DrawNumbers(wxDC& Dc)
{
  const char* pString = NumberStr();
  LineText(Dc, xNumber, mCanvasY - 1, wNumber, pString, hTop);

  Dc.SetClippingRegion(xNumber, yEvents, xNumber + wNumber, yEvents + hEvents);
  for (int i = mFromLine; i < mToLine; ++i)
  {
    tTrack* pTrack = gpProject->GetTrack(i);
    if (pTrack != 0)
    {
      if (pTrack->GetAudioMode())
      {
        LineText(Dc, xNumber, Line2y(i), wNumber, "Au");
      }
      else
      {
        int Value;
        switch (mNumberMode)
        {
          case eNmTrackNr:
            Value = i;
            break;
          case eNmMidiChannel:
            Value = pTrack->Channel;
            break;
          default:
            Value = 0;
            break;
        }
        ostringstream Oss;
        Oss << setw(2) << Value;
        LineText(Dc, xNumber, Line2y(i), wNumber, Oss.str().c_str());
      }
    }
  }
  Dc.DestroyClippingRegion();
}

//-----------------------------------------------------------------------------
// Description:
//   This function draws the "speed" tempo indicator in the top left part of
// the canvas.
//-----------------------------------------------------------------------------
void JZTrackWindow::DrawSpeed(wxDC& Dc, int Value, bool Down)
{
  if (Value < 0)
  {
    Value = gpProject->GetTrack(0)->GetDefaultSpeed();
  }

  ostringstream Oss;
  Oss << "speed: " << setw(3) << Value;

  LineText(Dc, xName, mCanvasY - 1, wName, Oss.str().c_str(), hTop, Down);
}

//-----------------------------------------------------------------------------
// Description:
//   Draw the "play position", by placing a vertical line where the
// "play clock" is.
//-----------------------------------------------------------------------------
void JZTrackWindow::DrawPlayPosition(wxDC& Dc)
{
  if (!mpSnapSel->Active && mPlayClock >= mFromClock && mPlayClock < mToClock)
  {
    Dc.SetBrush(*wxBLACK_BRUSH);
    Dc.SetPen(*wxBLACK_PEN);

//    Dc.SetLogicalFunction(wxXOR);

    int x = Clock2x(mPlayClock);

    // Draw a line, 2 pixwels wide.
    Dc.DrawLine(x,     mCanvasY, x,     yEvents + hEvents);
    Dc.DrawLine(x + 1, mCanvasY, x + 1, yEvents + hEvents);

//    Dc.SetLogicalFunction(wxCOPY);
  }
//  if (mpNextWin)
//  {
//    mpNextWin->DrawPlayPosition(Dc);
//  }
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
    Dc.SetBrush(*mpGreyBrush);
    Dc.SetPen(*wxGREY_PEN);
#ifdef __WXMSW__
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
    tTrack* pTrack = gpProject->GetTrack(i);
    if (pTrack)
    {
      char buf[20];
      int Value;
      switch (mCounterMode)
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
    {
      LineText(Dc, xPatch, Line2y(i), wPatch, "?");
    }
  }
  Dc.DestroyClippingRegion();
}

//-----------------------------------------------------------------------------
// Description:
//   Draw the MIDI events.
//-----------------------------------------------------------------------------
void JZTrackWindow::DrawEvents(wxDC& Dc)
{
  if (!mpSong)
  {
    return;
  }

  JZBarInfo BarInfo(mpSong);

  Dc.SetClippingRegion(xEvents, yEvents, wEvents, hEvents);

  int TrackNumber = mFromLine;
  for (int y = Line2y(TrackNumber); y < yEvents + hEvents; y += hLine)
  {
    tTrack *Track = gpProject->GetTrack(TrackNumber);
    if (Track)
    {
      tEventIterator Iterator(Track);
      long StopClk = x2Clock(mCanvasX + mCanvasWidth);
      JZEvent *e = Iterator.Range(mFromClock, StopClk);
      int y0 = y + mLittleBit;
      int y1 = y + hLine - mLittleBit;

      if (mUseColors)
      {
#if 0
	while (e)       // slow!
	{
	  float x = Clock2x(e->GetClock());
	  Dc.SetPen(e->GetPen());
	  Dc.DrawLine(x, y0, x, y1);
	  e = Iterator.Next();
	}
#else
	int xdone = -1;
	int h = y1 - y0;
	while (e)       // very slow!
	{
	  int x1 = Clock2x(e->GetClock() + e->GetLength());
	  if (x1 > xdone)
          {
	    int x0 = Clock2x(e->GetClock());
	    if (x0 < xdone)
            {
	      x0 = xdone;
            }
	    int w = x1 - x0;
	    if (w < 2)
            {
	      w = 2;
            }
	    xdone = x0 + w;
	    Dc.SetPen(*e->GetPen());
	    Dc.SetBrush(*e->GetBrush());
	    Dc.DrawRectangle(x0, y0, w, h);
	  }
	  e = Iterator.Next();
	}
#endif
	Dc.SetPen(*wxBLACK_PEN);
      }
      else
      {
	float xblack = -1.0;
	while (e)
	{
	  int x = Clock2x(e->GetClock());

	  // Avoid painting events ON the bar
	  if ( !(e->GetClock() % BarInfo.TicksPerBar) ) x = x + 1;

	  if (x > xblack)
	  {
	    Dc.DrawLine(x, y0, x, y1);
#ifndef SLOW_MACHINE
	    xblack = x;
#else
	    xblack = x + 4;
#endif
	  }
	  e = Iterator.Next();
	}
      }
    }
    ++TrackNumber;
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
int JZTrackWindow::x2BarClock(int x, int Next)
{
  long Clock = x2Clock(x);
  JZBarInfo BarInfo(mpSong);
  BarInfo.SetClock(Clock);
  while (Next--)
  {
    BarInfo.Next();
  }
  return BarInfo.Clock;
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

//-----------------------------------------------------------------------------
//   Returns a string indicating the current use of the "numbers" column,
// which is the leftmost one.
// T means track number, M means midi channel
//-----------------------------------------------------------------------------
const char* JZTrackWindow::NumberStr() const
{
  const char* pString;
  switch (mNumberMode)
  {
    case eNmTrackNr:
      pString = "T";
      break;
    case eNmMidiChannel:
      pString = "M";
      break;
    default:
      pString = "?";
      break;
  }
  return pString;
}

int JZTrackWindow::EventsSelected(const wxString& Message)
{
  if (!mpSnapSel->Selected)
  {
    wxMessageBox(Message, "Error", wxOK);
    return 0;
  }
  return 1;
}

void JZTrackWindow::MousePlay(wxMouseEvent& Event, TEMousePlayMode Mode)
{
  cout << "JZTrackWindow::MousePlay" << endl;

  if (Mode == eMouse && !Event.ButtonDown())
  {
    return;
  }

  // This is a little hack to keep it working for now.
  // All this stuff needs to be moved.
  JZRecordingInfo* pRecInfo = gpProject->GetRecInfo();

  if (!gpProject->IsPlaying())
  {
    switch (Mode)
    {
      case eMouse:
        int x, y;
        Event.GetPosition(&x, &y);
        gpProject->SetPlayPosition(x2BarClock(x));
        gpProject->Mute((Event.RightDown() != 0));
        if (mpSnapSel->Selected && (Event.ShiftDown() || Event.MiddleDown()))
        {
          gpProject->SetLoop(true);
        }
        else
        {
          gpProject->SetLoop(false);
          mPreviouslyRecording = mpSnapSel->Selected;
        }
        break;

      case eSpaceBar:
        break;

      case ePlayButton:
        cout << "JZTrackFrame::PlayButton" << endl;
        gpProject->SetLoop(false);
        gpProject->SetRecord(false);
        break;

      case ePlayLoopButton:
        if (!EventsSelected("please select loop range first"))
        {
          return;
        }
        gpProject->SetLoop(true);
        gpProject->SetRecord(false);
        break;

      case eRecordButton:
        if (!EventsSelected("please select record track/bar first"))
        {
          return;
        }
        JZBarInfo bi(gpProject);

        bi.SetClock(mpFilter->FromClock);

        if (bi.BarNr > 0)
        {
          bi.SetBar(bi.BarNr - 1);
        }
        gpProject->SetPlayPosition(bi.Clock);
        gpProject->SetRecord(true);
        gpProject->SetLoop(false);
        break;
    }

    // todo: Figure out if we should have getters for these instead
    // and make them private jppProject members
    bool loop   = gpProject->mLoop;
    bool muted  = gpProject->mMuted;
    bool record = gpProject->mRecord;

    // Is it possible to record?
    if (record && mpSnapSel->Selected)
    {
      pRecInfo->mTrackIndex = mpFilter->FromTrack;

      pRecInfo->mpTrack = gpProject->GetTrack(pRecInfo->mTrackIndex);

      pRecInfo->mFromClock = mpFilter->FromClock;
      pRecInfo->mToClock   = mpFilter->ToClock;

      if (muted)
      {
        pRecInfo->mIsMuted = true;
        pRecInfo->mpTrack->SetState(tsMute);
#ifdef OBSOLETE
        LineText(
          *pDc,
          xState,
          Line2y(pRecInfo.mTrackIndex),
          wState,
          pRecInfo.Track->GetStateChar());
#endif
      }
      else
      {
        pRecInfo->mIsMuted = false;
      }
    }
    else
    {
      pRecInfo->mpTrack = 0;
    }

    // Is it possible to loop?
    int loop_clock = 0;
    if (loop && mpSnapSel->Selected)
    {
      mPreviousClock = mpFilter->FromClock;
      loop_clock = mpFilter->ToClock;
    }

    // GO!
    cout << "Go!" << endl;

    //if (pRecInfo->Track)  // recording?
      //gpProject->Midi->SetRecordInfo(pRecInfo);
    //else
      //gpProject->Midi->SetRecordInfo(0);

    cout << "Midi->StartPlay" << endl;

    gpProject->mStartTime = mPreviousClock;
    gpProject->mStopTime = loop_clock;
    gpProject->Play();

  } //if(!Midi->Playing)
  else
  {
    gpProject->Stop();
    if (pRecInfo->mpTrack)
    {
      if (pRecInfo->mIsMuted)
      {
//        wxDC* pDc = new wxClientDC(mpTrackWindow);

        pRecInfo->mpTrack->SetState(tsPlay);
//        LineText(
//          *pDc,
//          xState,
//          Line2y(pRecInfo->mTrackIndex),
//          wState,
//          pRecInfo->mpTrack->GetStateChar());

//        delete pDc;
      }
      if (
        !pRecInfo->mpTrack->GetAudioMode() &&
        !gpProject->GetPlayer()->RecdBuffer.IsEmpty())
      {
        //int choice = wxMessageBox("Keep recorded events?", "You played", wxOK | wxCANCEL);
        //if (choice == wxOK)
        {
          wxBeginBusyCursor();
          gpProject->NewUndoBuffer();
          pRecInfo->mpTrack->MergeRange(
            &gpProject->GetPlayer()->RecdBuffer,
            pRecInfo->mFromClock,
            pRecInfo->mToClock,
            pRecInfo->mIsMuted);
          wxEndBusyCursor();
          Refresh();
        }
      }
    }
  }
}
