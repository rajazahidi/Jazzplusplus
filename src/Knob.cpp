//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Copyright (C) 2008 Peter J. Stieber
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

#include <wx/dcbuffer.h>

#include "Knob.h"
#include "Globals.h"

#include <cmath>

//*****************************************************************************
// Description:
//   This is the knob class definition.
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(JZKnob,wxControl)
  EVT_SIZE(JZKnob::OnSize)
  EVT_ERASE_BACKGROUND(JZKnob::OnEraseBackground)
  EVT_PAINT(JZKnob::OnPaint)
  EVT_LEFT_DOWN(JZKnob::OnMouse)
  EVT_LEFT_UP(JZKnob::OnMouse)
  EVT_MOTION(JZKnob::OnMouse)
  EVT_MOUSEWHEEL(JZKnob::OnMouse)
END_EVENT_TABLE()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZKnob::JZKnob()
  : wxControl()
{
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZKnob::JZKnob(
  wxWindow* pParent,
  wxWindowID Id,
  int Value,
  int MinValue,
  int MaxValue,
  unsigned int MinAngle,
  unsigned int Range,
  const wxPoint& Position,
  const wxSize& Size,
  long WindowStyle,
  const wxValidator& Validator,
  const wxString& Name)
  : wxControl()
{
  Create(
    pParent,
    Id,
    Value,
    MinValue,
    MaxValue,
    MinAngle,
    Range,
    Position,
    Size,
    WindowStyle,
    Validator,
    Name);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZKnob::Create(
  wxWindow* pParent,
  wxWindowID Id,
  int Value,
  int MinValue,
  int MaxValue,
  unsigned int MinAngle,
  unsigned int Range,
  const wxPoint& Position,
  const wxSize& Size,
  long WindowStyle,
  const wxValidator& Validator,
  const wxString& Name)
{
  wxControl::Create(
    pParent,
    Id,
    Position,
    Size,
    WindowStyle | wxNO_BORDER,
    Validator,
    Name);

  SetInitialSize(Size);

  mMin = MinValue;
  mMax = MaxValue;
  Range %= 360;
  MinAngle %= 360;
  mMaxAngle = (MinAngle + 360 - Range) % 360;

  mRange = Range;
  SetValue(Value);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZKnob::SetRange(int MinValue, int MaxValue)
{
  if (MinValue < MaxValue)
  {
    mMin = MinValue;
    mMax = MaxValue;
    SetValue(mSetting);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZKnob::SetValue(int Value)
{
  if (Value < mMin)
  {
    Value = mMin;
  }
  if (Value > mMax)
  {
    Value = mMax;
  }

  if (Value != mSetting)
  {
    mSetting = Value;
    Refresh(false);
    Update();
  }
  return mSetting;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZKnob::OnSize(wxSizeEvent& Event)
{
  int Width, Height;
  GetClientSize(&Width, &Height);
  if (Width > 0 && Height > 0)
  {
    mBuffer.Create(Width, Height);
  }
}

//-----------------------------------------------------------------------------
// Description:
//   This code always erasew when painting so we override this function to
// avoid flicker.
//-----------------------------------------------------------------------------
void JZKnob::OnEraseBackground(wxEraseEvent& Event)
{
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZKnob::OnPaint(wxPaintEvent& Event)
{
  wxSize Size = GetSize();

  double Theta = gDegreesToRadians *
    (mMaxAngle + (((double)mMax - mSetting) / (mMax - mMin)) * mRange);

  double DeltaX = cos(Theta);

  // Negate because of the upside down coordinates
  double DeltaY = -sin(Theta);

  wxPaintDC PaintDc(this);

  wxBufferedDC Dc(&PaintDc, mBuffer);

  Dc.SetBackground(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE)));

  Dc.Clear();

  int XCenter, YCenter;
  GetCenter(XCenter, YCenter);

  int OuterRadius = static_cast<int>(
    (((Size.x < Size.y) ? Size.x : Size.y) * .48) + 0.5);
  int InnerRadius = static_cast<int>(OuterRadius * 0.6 + 0.5);

  wxColour Color(120, 100, 100);
  wxBrush Brush(Color, wxSOLID);
  wxPen Pen(Color);
  int KnobRadius = OuterRadius;
  for (unsigned char Red = 120; KnobRadius > 0 && Red < 250; Red += 5)
  {
    Color.Set(Red, 100, 100);
    Brush.SetColour(Color);
    Pen.SetColour(Color);
    Dc.SetBrush(Brush);
    Dc.SetPen(Pen);
    Dc.DrawCircle(XCenter, YCenter, KnobRadius);
    --KnobRadius;
  }

  wxPen WhitePen(*wxWHITE, 3);
  Dc.SetPen(WhitePen);
  Dc.DrawLine(
    XCenter + static_cast<int>(OuterRadius * DeltaX + 0.5),
    YCenter + static_cast<int>(OuterRadius * DeltaY + 0.5),
    XCenter + static_cast<int>(InnerRadius * DeltaX + 0.5),
    YCenter + static_cast<int>(InnerRadius * DeltaY + 0.5));
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZKnob::OnMouse(wxMouseEvent& Event)
{
  wxEventType ScrollEvent = wxEVT_NULL;

  if (Event.Moving())
  {
    Event.Skip();
    return;
  }

  if (Event.GetWheelRotation() < 0)
  {
    SetValue(GetValue() - 1);
    Event.Skip();
    return;
  }

  if (Event.GetWheelRotation() > 0)
  {
    SetValue(GetValue() + 1);
    Event.Skip();
    return;
  }

  int XCenter, YCenter;
  GetCenter(XCenter, YCenter);

  double DeltaX = Event.m_x - XCenter;
  double DeltaY = YCenter - Event.m_y;
  if (DeltaX == 0.0 && DeltaY == 0.0)
  {
    return;
  }

  double Theta = atan2(DeltaY, DeltaX) * gRadiansToDegrees;
  if (Theta < 0.0)
  {
    Theta += 360.0;
  }

  double DeltaTheta = Theta - mMaxAngle;
  if (DeltaTheta < 0.0)
  {
    DeltaTheta += 360;
  }
  if (DeltaTheta > mRange)
  {
    return;
  }
  int NewValue = int(mMax - (DeltaTheta / mRange) * (mMax - mMin));

  SetValue(NewValue);
  if (Event.Dragging() || Event.ButtonUp())
  {
    if (Event.ButtonUp())
    {
      ScrollEvent = wxEVT_SCROLL_THUMBRELEASE;
    }
    else
    {
      ScrollEvent = wxEVT_SCROLL_THUMBTRACK;
    }

    wxScrollEvent ScrollEvent(ScrollEvent, m_windowId);
    ScrollEvent.SetPosition(NewValue);
    ScrollEvent.SetEventObject(this);
    GetEventHandler()->ProcessEvent(ScrollEvent);

    wxCommandEvent CommandEvent(wxEVT_COMMAND_SLIDER_UPDATED, m_windowId);
    CommandEvent.SetInt(NewValue);
    CommandEvent.SetEventObject(this);
    GetEventHandler()->ProcessEvent(CommandEvent);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZKnob::GetCenter(int& x, int& y) const
{
  wxSize Size = GetSize();
  x = Size.x / 2;
  y = Size.y / 2;
}
