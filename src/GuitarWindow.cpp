//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Copyright (C) 1994-2000 Andreas Voss and Per Sigmond, all rights reserved.
// Modifications Copyright (C) 2004 Patrick Earl
// Modifications Copyright (C) 2008-2013 Peter J. Stieber
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

#include "GuitarWindow.h"

#include "GuitarFrame.h"
#include "Events.h"
#include "Globals.h"
#include "Player.h"
#include "Project.h"
#include "Track.h"

#include <wx/dcclient.h>

#include <string>
#include <algorithm>

using namespace std;

//*****************************************************************************
// Description:
//   This is the guitar window class definition.
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool JZGuitarWindow::mChordMode = false;

//-----------------------------------------------------------------------------
// Start with a regular 6-string guitar: not a bass.
//-----------------------------------------------------------------------------
bool JZGuitarWindow::mBassGuitar = false;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool JZGuitarWindow::mShowOctaves = true;

//-----------------------------------------------------------------------------
// 21 frets standard full-board view
//-----------------------------------------------------------------------------
int JZGuitarWindow::mFretCount = 21;

//-----------------------------------------------------------------------------
// 4-string Bass Guitar Pitches (G2, D2, A1, E1)
//-----------------------------------------------------------------------------
const int JZGuitarWindow::mBassPitches[4] =
{
  28 + 15, // 43 = G2
  28 + 10, // 38 = D2
  28 +  5, // 33 = A1
  28       // 28 = E1
};

//-----------------------------------------------------------------------------
// 6-string Guitar Standard Tuning Pitches (E4, B3, G3, D3, A2, E2)
//-----------------------------------------------------------------------------
const int JZGuitarWindow::mGuitarPitches[6] =
{
  40 + 24, // 64 = E4
  40 + 19, // 59 = B3
  40 + 15, // 55 = G3
  40 + 10, // 50 = D3
  40 +  5, // 45 = A2
  40       // 40 = E2
};

void JZGuitarWindow::SetBassGuitar(bool b)
{
  mBassGuitar = b;
}

void JZGuitarWindow::SetFretCount(int count)
{
  if (count < 12) count = 12;
  if (count > 24) count = 24;
  mFretCount = count;
}

void JZGuitarWindow::UpdateSettings()
{
  if (mBassGuitar)
  {
    mpPitches = mBassPitches;
    mStringCount = 4;
  }
  else
  {
    mpPitches = mGuitarPitches;
    mStringCount = 6;
  }
  Refresh();
}

//-----------------------------------------------------------------------------
// Description:
//   This is the event table for the guitar window.  The event tables connect
// the wxWidgets events with the functions (event handlers) which process
// them.
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(JZGuitarWindow, wxScrolledWindow)

  EVT_SIZE(JZGuitarWindow::OnSize)
  EVT_PAINT(JZGuitarWindow::OnPaint)
  EVT_MOTION(JZGuitarWindow::OnMouseMove)
  EVT_LEFT_DOWN(JZGuitarWindow::OnMouseDown)
  EVT_LEFT_UP(JZGuitarWindow::OnMouseUp)
  EVT_LEAVE_WINDOW(JZGuitarWindow::OnMouseLeave)
  EVT_MOUSE_CAPTURE_LOST(JZGuitarWindow::OnMouseCaptureLost)

END_EVENT_TABLE()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZGuitarWindow::JZGuitarWindow(
  JZGuitarFrame* pParent,
  const wxPoint& Position,
  const wxSize& Size)
  : wxScrolledWindow(
      pParent,
      wxID_ANY,
      Position,
      Size,
      wxHSCROLL | wxVSCROLL | wxNO_FULL_REPAINT_ON_RESIZE),
    mNutX(75),
    mMargin(2),
    mActivePitch(0),
    mPlayPitch(0),
    mpFont(0),
    mpBoldFont(0),
    mpSmallFont(0)
{
  mWidth = Size.GetWidth();
  mHeight = Size.GetHeight();

  if (mBassGuitar)
  {
    mpPitches = mBassPitches;
    mStringCount = 4;
  }
  else
  {
    mpPitches = mGuitarPitches;
    mStringCount = 6;
  }
  mStringHeight = 26;
  mFretWidth = 40;

  mpFont = new wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
  mpBoldFont = new wxFont(11, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
  mpSmallFont = new wxFont(8, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZGuitarWindow::~JZGuitarWindow()
{
  if (HasCapture())
  {
    ReleaseMouse();
  }
  StopNote();
  delete mpFont;
  delete mpBoldFont;
  delete mpSmallFont;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZGuitarWindow::ClearBuffer()
{
  mActivePitch = 0;
  StopNote();
  Refresh();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZGuitarWindow::OnSize(wxSizeEvent& Event)
{
  GetClientSize(&mWidth, &mHeight);
  Refresh();
  Event.Skip();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZGuitarWindow::ShowPitch(int Pitch)
{
  mActivePitch = Pitch;
  Refresh();
}

void JZGuitarWindow::ShowPitch(wxDC&, int Pitch)
{
  mActivePitch = Pitch;
  Refresh();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZGuitarWindow::OnPaint(wxPaintEvent&)
{
  wxPaintDC Dc(this);
  OnDraw(Dc);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZGuitarWindow::OnDraw(wxDC& Dc)
{
  GetClientSize(&mWidth, &mHeight);
  if (mWidth <= 0 || mHeight <= 0)
  {
    return;
  }

  DrawBoard(Dc);
  if (mActivePitch > 0)
  {
    DrawPitch(Dc, mActivePitch, true);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZGuitarWindow::DrawBoard(wxDC& Dc)
{
  mNutX = 75; // Width of Tab String Labels / Headstock area
  int topY = 28;
  int bottomY = mHeight - 28;
  int playableHeight = bottomY - topY;

  mStringHeight = (mStringCount > 1) ? playableHeight / (mStringCount - 1) : playableHeight;
  if (mStringHeight < 1) mStringHeight = 1;
  int fretboardWidth = mWidth - mNutX - 10;
  if (fretboardWidth < 10) fretboardWidth = 10;
  mFretWidth = fretboardWidth / mFretCount;
  if (mFretWidth < 1) mFretWidth = 1;

  // 1. Fretboard Wood Background (Warm Rosewood tone)
  Dc.SetPen(*wxTRANSPARENT_PEN);
  wxBrush woodBrush(wxColour(46, 38, 33));
  Dc.SetBrush(woodBrush);
  Dc.DrawRectangle(mNutX, topY - 12, fretboardWidth, playableHeight + 24);

  // 2. Headstock / TAB Header Area (Dark charcoal/slate)
  wxBrush headstockBrush(wxColour(30, 28, 26));
  Dc.SetBrush(headstockBrush);
  Dc.DrawRectangle(0, topY - 12, mNutX, playableHeight + 24);

  // Draw "TAB" header label on the headstock
  Dc.SetFont(*mpBoldFont);
  Dc.SetTextForeground(wxColour(220, 190, 140));
  Dc.DrawText("TAB", 8, 6);

  // 3. Fret Position Inlays (Pearl Dots)
  wxBrush dotBrush(wxColour(225, 225, 230));
  Dc.SetBrush(dotBrush);
  Dc.SetPen(wxPen(wxColour(110, 105, 100), 1));
  int centerY = topY + playableHeight / 2;
  int dotRadius = 5;

  for (int f = 1; f <= mFretCount; ++f)
  {
    int fretCenter = mNutX + (f - 1) * mFretWidth + mFretWidth / 2;

    // Single dots at standard frets: 3, 5, 7, 9, 15, 17, 19, 21
    if (f == 3 || f == 5 || f == 7 || f == 9 || f == 15 || f == 17 || f == 19 || f == 21)
    {
      Dc.DrawCircle(fretCenter, centerY, dotRadius);
    }
    // Double dots at octave: fret 12 (and 24)
    else if (f == 12 || f == 24)
    {
      int offset = mStringHeight;
      if (mStringCount >= 6) offset = static_cast<int>(mStringHeight * 1.3);
      Dc.DrawCircle(fretCenter, centerY - offset / 2, dotRadius);
      Dc.DrawCircle(fretCenter, centerY + offset / 2, dotRadius);
    }
  }

  // 4. Fret Wires (Metallic silver nickel)
  wxPen wirePen(wxColour(205, 210, 220), 2);
  Dc.SetPen(wirePen);
  for (int f = 1; f <= mFretCount; ++f)
  {
    int fx = mNutX + f * mFretWidth;
    Dc.DrawLine(fx, topY - 12, fx, bottomY + 12);
  }

  // 5. Nut (Bone/Ivory nut bar separating Tab open string header from fret 1)
  wxBrush nutBrush(wxColour(245, 240, 225));
  Dc.SetBrush(nutBrush);
  Dc.SetPen(wxPen(wxColour(170, 160, 140), 1));
  Dc.DrawRectangle(mNutX - 4, topY - 14, 6, playableHeight + 28);

  // 6. Strings with Realistic Gauges & Tab String Names
  static const char* guitarTuningNames[6] = { "e (1)", "B (2)", "G (3)", "D (4)", "A (5)", "E (6)" };
  static const char* bassTuningNames[4] = { "G (1)", "D (2)", "A (3)", "E (4)" };

  for (int s = 0; s < mStringCount; ++s)
  {
    int sy = topY + s * mStringHeight;

    int thickness = 1;
    if (mBassGuitar)
    {
      thickness = 2 + s;
    }
    else
    {
      if (s >= 4) thickness = 3;
      else if (s >= 2) thickness = 2;
      else thickness = 1;
    }

    // Draw string
    wxPen stringPen(wxColour(215, 220, 230), thickness);
    Dc.SetPen(stringPen);
    Dc.DrawLine(0, sy, mWidth, sy);

    // Draw Tab String Name in headstock area
    Dc.SetFont(*mpFont);
    Dc.SetTextForeground(wxColour(230, 230, 230));
    const char* label = mBassGuitar ? bassTuningNames[s] : guitarTuningNames[s];
    Dc.DrawText(label, 6, sy - 8);
  }

  // 7. Fret Numbers along the bottom
  Dc.SetFont(*mpSmallFont);
  for (int f = 0; f <= mFretCount; ++f)
  {
    int fx = 0;
    if (f == 0)
    {
      fx = mNutX / 2 + 10;
    }
    else
    {
      fx = mNutX + (f - 1) * mFretWidth + mFretWidth / 2;
    }

    bool isKeyFret = (f == 0 || f == 3 || f == 5 || f == 7 || f == 9 || f == 12 || f == 15 || f == 17 || f == 19 || f == 21 || f == 24);
    if (isKeyFret)
    {
      Dc.SetTextForeground(wxColour(255, 205, 80)); // Gold for key frets
    }
    else
    {
      Dc.SetTextForeground(wxColour(145, 145, 155));
    }

    wxString numStr = wxString::Format("%d", f);
    int tw, th;
    Dc.GetTextExtent(numStr, &tw, &th);
    Dc.DrawText(numStr, fx - tw / 2, bottomY + 12);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZGuitarWindow::DrawPitch(wxDC& Dc, int Pitch, int String, bool Show)
{
  static const string KeyNames[12] =
  {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
  };

  if (String < 0 || String >= mStringCount || Pitch < mpPitches[String])
  {
    return;
  }

  int fret = Pitch - mpPitches[String];
  if (fret < 0 || fret > mFretCount)
  {
    return;
  }

  int topY = 28;
  int sy = topY + String * mStringHeight;
  int cx = 0;

  if (fret == 0)
  {
    cx = mNutX / 2 + 14; // Open string / nut position
  }
  else
  {
    cx = mNutX + (fret - 1) * mFretWidth + mFretWidth / 2;
  }

  if (Show)
  {
    int radius = 12;
    if (mFretWidth < 28) radius = std::max(8, mFretWidth / 2 - 2);

    wxColour badgeColor = (Pitch == mPlayPitch) ? wxColour(255, 175, 20) : wxColour(35, 145, 255);
    Dc.SetBrush(wxBrush(badgeColor));
    Dc.SetPen(wxPen(*wxWHITE, 1));
    Dc.DrawCircle(cx, sy, radius);

    Dc.SetFont(*mpBoldFont);
    Dc.SetTextForeground(*wxWHITE);
    string name = KeyNames[Pitch % 12];
    int tw, th;
    Dc.GetTextExtent(name.c_str(), &tw, &th);
    Dc.DrawText(name.c_str(), cx - tw / 2, sy - th / 2);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZGuitarWindow::DrawPitch(wxDC& Dc, int Pitch, bool Show)
{
  if (Pitch <= 0)
  {
    return;
  }

  if (mShowOctaves)
  {
    for (int i = 0; i < mStringCount; ++i)
    {
      for (int p = Pitch % 12; p < 127; p += 12)
      {
        DrawPitch(Dc, p, i, Show);
      }
    }
  }
  else
  {
    for (int i = 0; i < mStringCount; ++i)
    {
      DrawPitch(Dc, Pitch, i, Show);
    }
  }
}

//-----------------------------------------------------------------------------
// Coordinate mapping: y -> string index
//-----------------------------------------------------------------------------
int JZGuitarWindow::y2String(int y)
{
  int topY = 28;
  if (mStringHeight <= 0) return 0;
  int s = (y - topY + mStringHeight / 2) / mStringHeight;
  if (s < 0) s = 0;
  if (s >= mStringCount) s = mStringCount - 1;
  return s;
}

//-----------------------------------------------------------------------------
// Coordinate mapping: x -> fret index (0 = open string, 1..mFretCount)
//-----------------------------------------------------------------------------
int JZGuitarWindow::x2Grid(int x)
{
  if (x < mNutX) return 0;
  if (mFretWidth <= 0) return 1;
  int f = (x - mNutX) / mFretWidth + 1;
  if (f > mFretCount) f = mFretCount;
  return f;
}

//-----------------------------------------------------------------------------
// Coordinate mapping: (x, y) -> MIDI pitch
//-----------------------------------------------------------------------------
int JZGuitarWindow::Xy2Pitch(int x, int y)
{
  if (mStringHeight <= 0 || mFretWidth <= 0) return 0;
  int s = y2String(y);
  if (s < 0 || s >= mStringCount) return 0;

  if (x < mNutX)
  {
    return mpPitches[s]; // Fret 0: open string
  }
  int f = (x - mNutX) / mFretWidth + 1;
  if (f >= 1 && f <= mFretCount)
  {
    return mpPitches[s] + f;
  }
  return 0;
}

//-----------------------------------------------------------------------------
// Sound synthesis: play note immediately via MIDI player
//-----------------------------------------------------------------------------
void JZGuitarWindow::PlayNote(int pitch)
{
  if (pitch <= 0 || pitch == mPlayPitch) return;

  StopNote();

  mPlayPitch = pitch;
  if (gpMidiPlayer)
  {
    JZTrack* pTrack = gpProject ? gpProject->GetTrack(0) : 0;
    int channel = pTrack ? pTrack->GetChannel() : 0;
    JZKeyOnEvent KeyOn(0, channel, pitch, 100);
    gpMidiPlayer->OutNow(pTrack, &KeyOn);
  }
}

void JZGuitarWindow::StopNote()
{
  if (mPlayPitch > 0)
  {
    if (gpMidiPlayer)
    {
      JZTrack* pTrack = gpProject ? gpProject->GetTrack(0) : 0;
      int channel = pTrack ? pTrack->GetChannel() : 0;
      JZKeyOnEvent KeyOff(0, channel, mPlayPitch, 0);
      gpMidiPlayer->OutNow(pTrack, &KeyOff);
    }
    mPlayPitch = 0;
  }
}

//-----------------------------------------------------------------------------
// Interactive mouse events: click & drag to play notes on fretboard
//-----------------------------------------------------------------------------
void JZGuitarWindow::OnMouseDown(wxMouseEvent& MouseEvent)
{
  if (!HasCapture())
  {
    CaptureMouse();
  }
  wxPoint pos = MouseEvent.GetPosition();
  int pitch = Xy2Pitch(pos.x, pos.y);
  if (pitch > 0)
  {
    PlayNote(pitch);
    mActivePitch = pitch;
    Refresh();
  }
}

void JZGuitarWindow::OnMouseMove(wxMouseEvent& MouseEvent)
{
  wxPoint pos = MouseEvent.GetPosition();
  int pitch = Xy2Pitch(pos.x, pos.y);

  if (MouseEvent.LeftIsDown())
  {
    if (pitch > 0 && pitch != mPlayPitch)
    {
      PlayNote(pitch);
      mActivePitch = pitch;
      Refresh();
    }
  }
  else
  {
    if (pitch != mActivePitch)
    {
      mActivePitch = pitch;
      Refresh();
    }
  }
}

void JZGuitarWindow::OnMouseUp(wxMouseEvent&)
{
  if (HasCapture())
  {
    ReleaseMouse();
  }
  StopNote();
  Refresh();
}

void JZGuitarWindow::OnMouseLeave(wxMouseEvent& MouseEvent)
{
  if (!MouseEvent.LeftIsDown())
  {
    StopNote();
    mActivePitch = 0;
    Refresh();
  }
}

void JZGuitarWindow::OnMouseCaptureLost(wxMouseCaptureLostEvent&)
{
  StopNote();
  mActivePitch = 0;
  Refresh();
}
