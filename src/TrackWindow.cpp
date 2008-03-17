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

//*****************************************************************************
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(JZTrackWindow, wxScrolledWindow)
  EVT_SIZE(JZTrackWindow::OnSize)
  EVT_ERASE_BACKGROUND(JZTrackWindow::OnEraseBackground)
END_EVENT_TABLE()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
const int JZTrackWindow::mScrollLine = 50;

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
    mpFilter(0),
    mpSnapSel(0),
    mpSong(pSong),
    mpGreyColor(0),
    mpGreyBrush(0),
    mTrackHeight(10),
    mTopInfoHeight(40),
    mLeftInfoWidth(100),
    mClocksPerPixel(36),
    mPlayClock(-1),
    mUseColors(true),
    mLittleBit(2),
    mEventsX(),
    mEventsY(),
    mEventsWidth(),
    mEventsHeight(),
    mScrolledX(0),
    mScrolledY(0),
    mCanvasWidth(0),
    mCanvasHeight(0),
    mFromClock(0),
    mToClock(0),
    mFromLine(0),
    mToLine(0),
    mNumberWidth(),
    mTrackNameX(),
    mTrackNameWidth(),
    mStateX(),
    mStateWidth(),
    mPatchX(0),
    mPatchWidth(0),
    mBarCount(0),
    mCounterMode(eCmProgram),
//    mNumberMode(eNmMidiChannel),
    mNumberMode(eNmTrackNr),
    mpFixedFont(0),
    mFixedFontHeight(0),
    mFontSize(12),
    mpFont(0),
    mPreviouslyRecording(false),
    mPreviousClock(0),
    mDrawing(false),
    mpFrameBuffer(0)
{
#ifdef __WXMSW__
  mpGreyColor = new wxColor(192, 192, 192);
#else
  mpGreyColor = new wxColor(220, 220, 220);
#endif

  mpGreyBrush = new wxBrush(*mpGreyColor, wxSOLID);

  mpSnapSel = new tSnapSelection(this);

  mpFilter = new tFilter(mpSong);

  SetBackgroundColour(*wxWHITE);

  mpFrameBuffer = new wxBitmap;
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
  delete mpFrameBuffer;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZTrackWindow::Create()
{
  wxClientDC Dc(this);

  Dc.SetFont(wxNullFont);

  delete mpFixedFont;
  mpFixedFont = new wxFont(12, wxSWISS, wxNORMAL, wxNORMAL);
  Dc.SetFont(*mpFixedFont);

  int Width, Height;
  Dc.GetTextExtent("M", &Width, &mFixedFontHeight);

  delete mpFont;
  mpFont = new wxFont(mFontSize, wxSWISS, wxNORMAL, wxNORMAL);
  Dc.SetFont(*mpFont);

  Dc.GetTextExtent("M", &Width, &Height);
  mLittleBit = Width / 2;

  Dc.GetTextExtent("HXWjgi", &Width, &Height);
  mTrackHeight = Height + 2 * mLittleBit;

  mTopInfoHeight = mFixedFontHeight + 2 * mLittleBit;

  Dc.GetTextExtent("999", &Width, &Height);
  mNumberWidth = Width + 2 * mLittleBit;

  Dc.GetTextExtent("Normal Track Name", &Width, &Height);
  mTrackNameWidth = Width + 2 * mLittleBit;

  Dc.GetTextExtent("m", &Width, &Height);
  mStateWidth = Width + 2 * mLittleBit;

  Dc.GetTextExtent("999", &Width, &Height);
  mPatchWidth = Width + 2 * mLittleBit;

  mLeftInfoWidth = mNumberWidth + mTrackNameWidth + mStateWidth + mPatchWidth + 1;

//DEBUG  cout
//DEBUG    << ' ' << mNumberWidth
//DEBUG    << ' ' << mTrackNameWidth
//DEBUG    << ' ' << mStateWidth
//DEBUG    << ' ' << mPatchWidth
//DEBUG    << ' ' << mLeftInfoWidth
//DEBUG    << endl;

  UnMark();

//  delete pDc;
}

//-----------------------------------------------------------------------------
// Description:
//   Update the play position to the clock argument, and trigger a redraw so
// the play bar will be drawn.
//-----------------------------------------------------------------------------
void JZTrackWindow::NewPlayPosition(int Clock)
{
  int scroll_clock = (mFromClock + 5 * mToClock) / 6L;

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
    int x = Clock2x(Clock);
    SetScrollPosition(x - mLeftInfoWidth, mScrolledY);
  }

  if (!mpSnapSel->Active)  // sets clipping
  {
    if (mPlayClock != Clock)
    {
      int OldPlayClock = mPlayClock;
      mPlayClock = Clock;
      wxRect invalidateRect;
      invalidateRect.x = Clock2x(OldPlayClock) - 1;
      invalidateRect.y = mScrolledY;
      invalidateRect.width = 3;
      invalidateRect.height= 100000000;
      //DrawPlayPosition();

      Refresh(true, &invalidateRect);

      invalidateRect.x = Clock2x(mPlayClock) - 1;

      Refresh(true, &invalidateRect);
      //DrawPlayPosition();

      Refresh(false);
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
  Marked.SetHeight(mTrackHeight);

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
  if (mCanvasWidth > 0 && mCanvasHeight > 0)
  {
    mpFrameBuffer->Create(mCanvasWidth, mCanvasHeight);
    SetScrollRanges(mScrolledX, mScrolledY);
//    SetScrollPosition(0, 0);
  }
}

//-----------------------------------------------------------------------------
// Description:
//   Do nothing, to avoid flickering.
//-----------------------------------------------------------------------------
void JZTrackWindow::OnEraseBackground(wxEraseEvent& Event)
{
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZTrackWindow::ZoomIn()
{
  if (mClocksPerPixel >= 2)
  {
    mClocksPerPixel /= 2;
    int x = mScrolledX * 2;
    int y = mScrolledY;

    SetScrollRanges(x, y);
//    SetScrollPosition(x, y);

    Refresh(false);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZTrackWindow::ZoomOut()
{
  if (mClocksPerPixel <= 120)
  {
    mClocksPerPixel *= 2;
    int x = mScrolledX / 2;
    int y = mScrolledY;

    SetScrollRanges(x, y);
//    SetScrollPosition(x, y);

    Refresh(false);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZTrackWindow::OnDraw(wxDC& Dc)
{
  Draw(Dc);
}

//-----------------------------------------------------------------------------
// Description:
//   Draw the static portion of the scenario in the static bitmap.
//-----------------------------------------------------------------------------
void JZTrackWindow::Draw(wxDC& Dc)
{
  if (!mpFrameBuffer->Ok() || mDrawing)
  {
    return;
  }

  mDrawing = true;

  // Create a memory device context and select the frame bitmap into it.
  wxMemoryDC LocalDc;
  LocalDc.SelectObject(*mpFrameBuffer);

  LocalDc.SetFont(*mpFont);

  // Setup the brush that is used to clear the background.
  LocalDc.SetBackground(*wxWHITE_BRUSH);

  // Clear the background using the brush that was just setup,
  // in case the following drawing calls fail.
  LocalDc.Clear();

  // Get the location, in scrolling units, of the upper left hand corner of
  // viewable portion of the virtual window that makes up the scrolled window.
  // Note that the bitmap we are drawing to always has upper left coordinates
  // of (0, 0).  The y value is used to draw the proper tracks and the x
  // value is used to draw the proper measures or bars.
  GetViewStart(&mScrolledX, &mScrolledY);

  // Convert scrolling units into pixels.
  mScrolledX *= mScrollLine;
  mScrolledY *= mScrollLine;

  GetClientSize(&mCanvasWidth, &mCanvasHeight);
  cout
    << "mCanvasWidth: " << mCanvasWidth
    << "   mCanvasHeight: " << mCanvasHeight
    << endl;

  mEventsX = mLeftInfoWidth;
  mEventsY = mTopInfoHeight;

  mEventsWidth = mCanvasWidth - mLeftInfoWidth;
  mEventsHeight = mCanvasHeight - mTopInfoHeight;

  mFromLine = mScrolledY / mTrackHeight;

  mToLine = 1 + (mScrolledY + mCanvasHeight - mTopInfoHeight) / mTrackHeight;
  mFromClock = mScrolledX * mClocksPerPixel;
//OLD  mToClock = x2Clock(mScrolledX + mCanvasWidth);
  mToClock = x2Clock(mScrolledX + mCanvasWidth - mLeftInfoWidth);
  mTrackNameX = mNumberWidth;
  mStateX = mTrackNameX + mTrackNameWidth;
  mPatchX = mStateX  + mStateWidth;

  LocalDc.DestroyClippingRegion();

  LocalDc.SetPen(*wxBLACK_PEN);

  // Draw the vertical lines.
  DrawVerticalLine(LocalDc, 0);
  DrawVerticalLine(LocalDc, mTrackNameX);
  DrawVerticalLine(LocalDc, mStateX);
  DrawVerticalLine(LocalDc, mPatchX);

  DrawVerticalLine(LocalDc, mEventsX - 1);
  DrawHorizontalLine(LocalDc, mEventsY);
  DrawHorizontalLine(LocalDc, mEventsY - 1);

  if (mpSong)
  {
    JZBarInfo BarInfo(mpSong);

    cout
      << "mCanvasWidth - mLeftInfoWidth: " << mCanvasWidth - mLeftInfoWidth << '\n'
      << "BarInfo.TicksPerBar            " << BarInfo.TicksPerBar << '\n'
      << "From Clock:                    " << mFromClock << '\n'
      << "To Clock:                      " << mToClock << '\n'
      << "Clocks/Pixel:                  " << mClocksPerPixel << '\n'
      << "From Measure:                  " << mFromClock / BarInfo.TicksPerBar << '\n'
      << "To Measure:                    " << mToClock / BarInfo.TicksPerBar
//      << "From X:                        " << mFromClock << '\n'
//      << "To X:                          " << mToClock << '\n'
      << endl;


    BarInfo.SetClock(mFromClock);

    mBarCount = 0;
    int Intro = gpProject->GetIntroLength();
    LocalDc.SetPen(*wxGREY_PEN);
    while (1)
    {
      int x = Clock2x(BarInfo.Clock);
      if (x > mScrolledX + mCanvasWidth)
      {
        break;
      }

      if (x >= mEventsX)
      {
        int c;
        if (mClocksPerPixel > 48)
        {
          c = 8;
        }
        else
        {
          c = 4;
        }
        if (((BarInfo.BarNr - Intro + 96) % c) == 0)
        {
          LocalDc.SetPen(*wxBLACK_PEN);
          ostringstream Oss;
          Oss << BarInfo.BarNr + 1 - Intro;
          LocalDc.DrawText(Oss.str().c_str(), x + mLittleBit, mEventsY - mTrackHeight);
          LocalDc.SetPen(*wxGREY_PEN);
          LocalDc.DrawLine(x, mEventsY + 1 - mTrackHeight, x, mEventsY + mEventsHeight);
        }
        else
        {
          LocalDc.SetPen(*wxLIGHT_GREY_PEN);
          LocalDc.DrawLine(x, mEventsY + 1, x, mEventsY + mEventsHeight);
        }

        // x-coordinate for MouseAction->Snap()
        if (mBarCount < eMaxBars)
        {
          mBarX[mBarCount++] = x;
        }
      }
      BarInfo.Next();
    }
    LocalDc.SetPen(*wxBLACK_PEN);
  }

  // For each track show the MIDI channel, name, state, prg.
  int TrackNumber = mFromLine;

  for (int y = Track2y(TrackNumber); y < mEventsY + mEventsHeight; y += mTrackHeight)
  {
    LocalDc.SetClippingRegion(
      0,
      mEventsY,
      mCanvasWidth,
      mEventsHeight);

    LocalDc.SetPen(*wxGREY_PEN);
    LocalDc.DrawLine(mEventsX + 1, y, mCanvasWidth, y);
    LocalDc.SetPen(*wxBLACK_PEN);
    LocalDc.DrawLine(0, y, mEventsX, y);

    LocalDc.DestroyClippingRegion();

    tTrack* pTrack = gpProject->GetTrack(TrackNumber);
    if (pTrack)
    {
      LocalDc.SetClippingRegion(
        mTrackNameX,
        mEventsY,
        mTrackNameWidth + mStateWidth,
        mEventsHeight);

      // Draw the track name.
      if (pTrack->DialogBox)
      {
        // Show the button pressed when the dialog box is open.
        LineText(LocalDc, mTrackNameX, y, mTrackNameWidth, pTrack->GetName(), -1, true);
      }
      else
      {
        LineText(LocalDc, mTrackNameX, y, mTrackNameWidth, pTrack->GetName(), -1, false);
      }

      // Draw the track status.
      LineText(LocalDc, mStateX, y, mStateWidth, pTrack->GetStateChar());

      LocalDc.DestroyClippingRegion();
    }
    else
    {
      LineText(LocalDc, mTrackNameX, y, mTrackNameWidth, "", -1, false);
      LineText(LocalDc, mStateX, y, mStateWidth, "");
    }

    ++TrackNumber;
  }

  DrawNumbers(LocalDc);
  DrawSpeed(LocalDc);
  DrawCounters(LocalDc);

  LineText(LocalDc, mStateX, -1, mStateWidth, "", mTopInfoHeight);

  DrawEvents(LocalDc);

  if (Marked.x > 0)
  {
    LineText(LocalDc, Marked.x, Marked.y, Marked.width, ">");
  }

  LocalDc.DestroyClippingRegion();

  DrawPlayPosition(LocalDc);

  // Draw the selection box.
  mpSnapSel->Draw(LocalDc, mEventsX, mEventsY, mEventsWidth, mEventsHeight);
  
//  LocalDc.SetClippingRegion(0, 0, mCanvasWidth, mCanvasHeight);
  Dc.Blit(
    mScrolledX,
    mScrolledY,
    mCanvasWidth,
    mCanvasHeight,
    &LocalDc,
    0,
    0,
    wxCOPY);
//  LocalDc.DestroyClippingRegion();

  LocalDc.SetFont(wxNullFont);
  LocalDc.SelectObject(wxNullBitmap);

  mDrawing = false;
}

//-----------------------------------------------------------------------------
// Description:
//   This function draws the "numbers" column (leftmost one), which either
// represents track numbers or midi channel depending on mode.
//-----------------------------------------------------------------------------
void JZTrackWindow::DrawNumbers(wxDC& Dc)
{
  const char* pString = GetNumberString();
  LineText(Dc, 0, -1, mNumberWidth, pString, mTopInfoHeight);

  Dc.SetClippingRegion(0, mEventsY, mNumberWidth, mEventsHeight);
  for (int i = mFromLine; i < mToLine; ++i)
  {
    tTrack* pTrack = gpProject->GetTrack(i);
    if (pTrack != 0)
    {
      if (pTrack->GetAudioMode())
      {
        LineText(Dc, 0, Track2y(i), mNumberWidth, "Au");
      }
      else
      {
        int Value;
        switch (mNumberMode)
        {
          case eNmTrackNr:
            Value = i + 1;
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
        LineText(Dc, 0, Track2y(i), mNumberWidth, Oss.str().c_str());
      }
    }
    else
    {
      LineText(Dc, 0, Track2y(i), mNumberWidth, "");
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

  LineText(Dc, mTrackNameX, -1, mTrackNameWidth, Oss.str().c_str(), mTopInfoHeight, Down);
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

    int x = Clock2x(mPlayClock);

    // Draw a line, 2 pixwels wide.
    Dc.DrawLine(x,     0, x,     mEventsY + mEventsHeight);
    Dc.DrawLine(x + 1, 0, x + 1, mEventsY + mEventsHeight);
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
    Height = mTrackHeight;
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
  const char* pString = GetCounterString();
  LineText(Dc, mPatchX, -1, mPatchWidth, pString, mTopInfoHeight);

  Dc.SetClippingRegion(mPatchX, mEventsY, mPatchWidth, mEventsHeight);
  for (i = mFromLine; i < mToLine; i++)
  {
    tTrack* pTrack = gpProject->GetTrack(i);
    if (pTrack)
    {
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
      ostringstream Oss;
      Oss << setw(3) << Value;
      LineText(Dc, mPatchX, Track2y(i), mPatchWidth, Oss.str().c_str());
    }
    else
    {
      LineText(Dc, mPatchX, Track2y(i), mPatchWidth, "?");
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

  Dc.SetClippingRegion(mEventsX, mEventsY, mEventsWidth, mEventsHeight);

  int TrackNumber = mFromLine;
  for (int y = Track2y(TrackNumber); y < mEventsY + mEventsHeight; y += mTrackHeight)
  {
    tTrack *Track = gpProject->GetTrack(TrackNumber);
    if (Track)
    {
      tEventIterator Iterator(Track);
      int StopClk = x2Clock(mScrolledX + mCanvasWidth);
      JZEvent* pEvent = Iterator.Range(mFromClock, StopClk);
      int y0 = y + mLittleBit;
      int y1 = y + mTrackHeight - mLittleBit;

      if (mUseColors)
      {
#if 0
        while (pEvent)       // slow!
        {
          float x = Clock2x(pEvent->GetClock());
          Dc.SetPen(pEvent->GetPen());
          Dc.DrawLine(x, y0, x, y1);
          pEvent = Iterator.Next();
        }
#else
        int xdone = -1;
        int h = y1 - y0;
        while (pEvent)       // very slow!
        {
          int x1 = Clock2x(pEvent->GetClock() + pEvent->GetLength());
          if (x1 > xdone)
          {
            int x0 = Clock2x(pEvent->GetClock());
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
            Dc.SetPen(*pEvent->GetPen());
            Dc.SetBrush(*pEvent->GetBrush());
            Dc.DrawRectangle(x0, y0, w, h);
          }
          pEvent = Iterator.Next();
        }
#endif
        Dc.SetPen(*wxBLACK_PEN);
      }
      else
      {
        float xblack = -1.0;
        while (pEvent)
        {
          int x = Clock2x(pEvent->GetClock());

          // Avoid painting events ON the bar
          if ( !(pEvent->GetClock() % BarInfo.TicksPerBar) ) x = x + 1;

          if (x > xblack)
          {
            Dc.DrawLine(x, y0, x, y1);
#ifndef SLOW_MACHINE
            xblack = x;
#else
            xblack = x + 4;
#endif
          }
          pEvent = Iterator.Next();
        }
      }
    }
    ++TrackNumber;
  }

  Dc.DestroyClippingRegion();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
const char* JZTrackWindow::GetCounterString()
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
//   Returns a string indicating the current use of the "numbers" column,
// which is the leftmost one.
// T means track number, M means midi channel
//-----------------------------------------------------------------------------
const char* JZTrackWindow::GetNumberString() const
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

//-----------------------------------------------------------------------------
// Was the VLine macro
//-----------------------------------------------------------------------------
void JZTrackWindow::DrawVerticalLine(wxDC& Dc, int XPosition) const
{
  Dc.DrawLine(XPosition, 0, XPosition, mEventsY + mEventsHeight);
}

//-----------------------------------------------------------------------------
// Was the HLine macro
//-----------------------------------------------------------------------------
void JZTrackWindow::DrawHorizontalLine(wxDC& Dc, int YPosition) const
{
  Dc.DrawLine(0, YPosition, mCanvasWidth, YPosition);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZTrackWindow::x2xBar(int x)
{
  for (int i = 1; i < mBarCount; ++i)
  {
    if (x < mBarX[i])
    {
      return mBarX[i - 1];
    }
  }
  return -1;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZTrackWindow::x2wBar(int x)
{
  for (int i = 1; i < mBarCount; ++i)
  {
    if (x < mBarX[i])
    {
      return mBarX[i] - mBarX[i - 1];
    }
  }
  return 0;
}

//-----------------------------------------------------------------------------
// Description:
//   Convert a track index into a y-pixel location in the visible window.
//-----------------------------------------------------------------------------
int JZTrackWindow::Track2y(int Track)
{
  return Track * mTrackHeight + mTopInfoHeight - mScrolledY;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZTrackWindow::x2Clock(int x)
{
  return (x - mEventsX) * mClocksPerPixel + mFromClock;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZTrackWindow::Clock2x(int Clock)
{
  return mEventsX + (Clock - mFromClock) / mClocksPerPixel;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZTrackWindow::x2BarClock(int x, int Next)
{
  int Clock = x2Clock(x);
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
    y += mTrackHeight;
  }
  y -= mTopInfoHeight;
  y -= y % mTrackHeight;
  y += mTopInfoHeight;
  return y;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZTrackWindow::EventsSelected(const wxString& Message)
{
  if (!mpSnapSel->Selected)
  {
    wxMessageBox(Message, "Error", wxOK);
    return 0;
  }
  return 1;
}

//-----------------------------------------------------------------------------
// Description:
//   Only consider the event portion of the window when computing the virtual
// size.  Do not consider the static information of the left or top portion of
// the screen.
//-----------------------------------------------------------------------------
void JZTrackWindow::GetVirtualEventSize(int& Width, int& Height) const
{
  int TotalClockTics = mpSong->MaxQuarters * mpSong->TicksPerQuarter;
  Width = TotalClockTics / mClocksPerPixel + mLeftInfoWidth;
  Height = 127 * mTrackHeight + mTopInfoHeight;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZTrackWindow::SetScrollRanges(const int& x, const int& y)
{
  int Width, Height;
  GetVirtualEventSize(Width, Height);
  SetScrollbars(
    mScrollLine,
    mScrollLine,
    (Width + mScrollLine) / mScrollLine,
    (Height + mScrollLine) / mScrollLine,
    x,
    y);
  EnableScrolling(false, false);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZTrackWindow::SetScrollPosition(int x, int y)
{
  x /= mScrollLine;
  y /= mScrollLine;
  Scroll(x, y);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZTrackWindow::MousePlay(wxMouseEvent& Event, TEMousePlayMode Mode)
{
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
          mStateX,
          Track2y(pRecInfo.mTrackIndex),
          mStateWidth,
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

    //if (pRecInfo->Track)  // recording?
      //gpProject->Midi->SetRecordInfo(pRecInfo);
    //else
      //gpProject->Midi->SetRecordInfo(0);

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
//          mStateX,
//          Track2y(pRecInfo->mTrackIndex),
//          mStateWidth,
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

          Refresh(false);
        }
      }
    }
  }
}
