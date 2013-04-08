//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Copyright (C) 2013 Peter J. Stieber
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

#include "ArrayControl.h"

#include "Mapper.h"
#include "Random.h"

#include <wx/dcclient.h>

#include <sstream>

using namespace std;

#define TICK_LINE 0

//*****************************************************************************
//*****************************************************************************
static string GetText(int YValue)
{
  ostringstream Oss;
  Oss << YValue;
  return Oss.str();
}

//*****************************************************************************
// Description:
//   This is the array control class definition.
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(JZArrayControl, wxControl)
  EVT_SIZE(JZArrayControl::OnSize)
  EVT_PAINT(JZArrayControl::OnPaint)
  EVT_MOUSE_EVENTS(JZArrayControl::OnMouseEvent)
  EVT_MOUSE_CAPTURE_LOST(JZArrayControl::OnMouseCaptureLost)
END_EVENT_TABLE()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZArrayControl::JZArrayControl(
  wxWindow* pParent,
  wxWindowID Id,
  const JZRndArray& RandomArray,
  const wxPoint& Position,
  const wxSize& Size,
  long WindowStyle)
  : wxControl(pParent, Id, Position, Size, wxNO_BORDER),
    mpRandomArray(0),
    mStyleBits(ARED_GAP | ARED_XTICKS),
    mEnabled(true),
    mLabel(),
    mX(0),
    mY(0),
    mYNull(0),
    mWidth(0),
    mHeight(0),
    mDragging(false),
    mIndex(-1),
    mXMin(0),
    mXMax(RandomArray.Size())
{
  mpRandomArray = new JZRndArray(RandomArray);

  SetInitialSize(Size);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZArrayControl::~JZArrayControl()
{
  delete mpRandomArray;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZArrayControl::SetLabel(const string& Label)
{
  mLabel = Label;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZArrayControl::SetXMinMax(int XMin, int XMax)
{
  mXMin = XMin;
  mXMax = XMax;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZArrayControl::OnSize(wxSizeEvent& SizeEvent)
{
  Refresh();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZArrayControl::OnPaint(wxPaintEvent& Event)
{
  wxSize Size = GetClientSize();
  mWidth = Size.GetWidth();
  mHeight = Size.GetHeight();

  wxPaintDC Dc(this);

  int TextWidth, TextHeight;
  Dc.GetTextExtent("123", &TextWidth, &TextHeight);

  if (mStyleBits & ARED_XTICKS)
  {
    // Leave space for the bottom line.
    mHeight -= TextHeight;
  }

  if (mStyleBits & (ARED_MINMAX | ARED_YTICKS))
  {
    // Leave space to display the minimum and maximum
    mX = TextWidth + TICK_LINE;
    mWidth -= TextWidth + TICK_LINE;
  }

  mYNull =
    mY + mHeight -
    mHeight * (mpRandomArray->GetNull() - mpRandomArray->GetMin()) /
    (mpRandomArray->GetMax() - mpRandomArray->GetMin());

  int i;

  // surrounding rectangle
  Dc.Clear();
  if (mEnabled)
  {
    Dc.SetBrush(*wxWHITE_BRUSH);
  }
  else
  {
    Dc.SetBrush(*wxGREY_BRUSH);
  }

  Dc.SetPen(*wxBLACK_PEN);

  if (mWidth && mHeight)
  {
    Dc.DrawRectangle(0, 0, mWidth, mHeight);
  }

  // sliders
  Dc.SetBrush(*wxBLACK_BRUSH);
  for (i = 0; i < mpRandomArray->Size(); ++i)
  {
    DrawBar(Dc, i, true);
  }

  DrawXTicks(Dc);
  DrawLabel(Dc);
  DrawYTicks(Dc);
  DrawNull(Dc);

//  if (mpDrawBars)
  {
//    mpDrawBars->DrawBars(Dc);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZArrayControl::OnMouseEvent(wxMouseEvent& MouseEvent)
{
  if (!mEnabled)
  {
    return;
  }
  if (MouseEvent.ButtonDown())
  {
    ButtonDown(MouseEvent);
  }
  else if (MouseEvent.Dragging())
  {
    Dragging(MouseEvent);
  }
  else if (MouseEvent.ButtonUp())
  {
    ButtonUp(MouseEvent);
  }
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZArrayControl::OnMouseCaptureLost(wxMouseCaptureLostEvent&)
{
  if (HasCapture())
  {
    ReleaseMouse();
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZArrayControl::ButtonDown(wxMouseEvent& MouseEvent)
{
  CaptureMouse();
  mDragging = true;
  mIndex = GetIndex(MouseEvent);
  Dragging(MouseEvent);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZArrayControl::Dragging(wxMouseEvent& MouseEvent)
{
  if (!mDragging)
  {
    return;
  }

  if (mIndex < 0)
  {
    mIndex = GetIndex(MouseEvent);
  }

  wxClientDC Dc(this); // PORTING this is evil and shoud go

  int Value = mpRandomArray->GetNull();
  if (MouseEvent.LeftIsDown())
  {
    int EventX, EventY;
    MouseEvent.GetPosition(&EventX, &EventY);

    Value = (int)((double)(mY + mHeight - EventY) *
      (mpRandomArray->GetMax() - mpRandomArray->GetMin()) / mHeight +
      mpRandomArray->GetMin() + 0.5);

    if (Value < mpRandomArray->GetMin())
    {
      Value = mpRandomArray->GetMin();
    }
    if (Value > mpRandomArray->GetMax())
    {
      Value = mpRandomArray->GetMax();
    }
  }

  if (MouseEvent.ShiftDown())
  {
    for (int k = 0; k < mpRandomArray->Size(); ++k)
    {
      DrawBar(Dc, k, 0);
      (*mpRandomArray)[k] = Value;
      DrawBar(Dc, k, 1);
    }
  }
  else if (MouseEvent.ControlDown())
  {
    DrawBar(Dc, mIndex, 0);
    (*mpRandomArray)[mIndex] = Value;
    DrawBar(Dc, mIndex, 1);
  }
  else
  {
    int i = GetIndex(MouseEvent);
    int k = i;
    if (i < mIndex)
    {
      for (; i <= mIndex; ++i)
      {
        DrawBar(Dc, i, 0);
        (*mpRandomArray)[i] = Value;
        DrawBar(Dc, i, 1);
      }
    }
    else
    {
      for (; i >= mIndex; --i)
      {
        DrawBar(Dc, i, 0);
        (*mpRandomArray)[i] = Value;
        DrawBar(Dc, i, 1);
      }
    }
    mIndex = k;
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZArrayControl::ButtonUp(wxMouseEvent& MouseEvent)
{
  if (HasCapture())
  {
    ReleaseMouse();
  }
  mDragging = false;
  mIndex    = -1;
  Refresh();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZArrayControl::GetIndex(wxMouseEvent& MouseEvent)
{
  int EventX, EventY;
  MouseEvent.GetPosition(&EventX, &EventY);
  int Index = (int)((EventX - mX) * mpRandomArray->Size() / mWidth);
  if (Index < 0)
  {
    Index = 0;
  }
  if (Index >= mpRandomArray->Size())
  {
    Index = mpRandomArray->Size() - 1;
  }
  return Index;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZArrayControl::DrawBar(wxDC& Dc, int i, bool black)
{
  if (mStyleBits & ARED_LINES)
  {
    if (!black)
    {
      Dc.SetPen(*wxWHITE_PEN);
    }

    JZMapper XMap(0, mpRandomArray->Size(), 0, mWidth);
    JZMapper
      YMap(mpRandomArray->GetMin(), mpRandomArray->GetMax(), mHeight, 0);

    int x1 = (int)XMap.XToY(i + 0.5);
    int y1 = (int)YMap.XToY((*mpRandomArray)[i]);
    if (i > 0)
    {
      // draw line to prev position
      int x0 = (int)XMap.XToY(i - 0.5);
      int y0 = (int)YMap.XToY((*mpRandomArray)[i - 1]);
      Dc.DrawLine(x0, y0, x1, y1);
    }
    if (i < mpRandomArray->Size() - 1)
    {
      // draw line to next position
      int x2 = (int)XMap.XToY(i + 1.5);
      int y2 = (int)YMap.XToY((*mpRandomArray)[i + 1]);
      Dc.DrawLine(x1, y1, x2, y2);
    }

    if (!black)
    {
      Dc.SetPen(*wxBLACK_PEN);
    }
    return;
  }

  int Gap = 0;
  if (mStyleBits & ARED_GAP)
  {
    Gap = mWidth / mpRandomArray->Size() / 6;
    if (!Gap && mWidth / mpRandomArray->Size() > 3)
    {
      Gap = 1;
    }
  }

  int wbar = mWidth / mpRandomArray->Size() - 2 * Gap;
  int xbar = mX + i * mWidth / mpRandomArray->Size() + Gap;
  int hbar = mHeight * ((*mpRandomArray)[i] - mpRandomArray->GetNull()) /
    (mpRandomArray->GetMax() - mpRandomArray->GetMin());
  int ybar;

  if (mStyleBits & ARED_BLOCKS)
  {
    /*
    ybar = mYNull - hbar;
    if (hbar < 0)
      hbar = -hbar;
    hbar = (hbar < 2) ? hbar : 2;
    */
    int hblk = 12;

    ybar = mYNull - hbar - hblk/2;
    hbar = hblk;
    if (ybar < mY)
    {
      int d = mY - ybar;
      ybar += d;
      hbar -= d;
    }
    if (ybar + hbar > mHeight)
    {
      int d = (ybar + hbar) - mHeight;
      hbar -= d;
    }
    if (hbar < 2)
      hbar = 2;
  }
  else

  if (hbar < 0)
  {
    ybar = mYNull;
    hbar = -hbar;
  }
  else
    ybar = mYNull - hbar;

  if (ybar == mY)
  {
    ++ybar, --hbar;
  }

  if (!black)
  {
    Dc.SetBrush(*wxWHITE_BRUSH);
    Dc.SetPen(*wxWHITE_PEN);
  }
  if (wbar && hbar)
  {
    Dc.DrawRectangle(xbar, ybar, wbar, hbar);
  }
  if (!black)
  {
    Dc.SetBrush(*wxBLACK_BRUSH);
    Dc.SetPen(*wxBLACK_PEN);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZArrayControl::DrawLabel(wxDC& Dc)
{
  Dc.SetFont(*wxSMALL_FONT);
  if (!mLabel.empty())
  {
    Dc.DrawText(mLabel, 5, 2);
  }
  Dc.SetFont(*wxNORMAL_FONT);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZArrayControl::DrawXTicks(wxDC& Dc)
{
  if (!(mStyleBits & ARED_XTICKS))
  {
    return;
  }

  Dc.SetFont(*wxSMALL_FONT);

  // Compute tickmark x-distance.
  int TextWidth, TextHeight;
  Dc.GetTextExtent("-123", &TextWidth, &TextHeight);
  int MaxLabels = (int)(mWidth / (TextWidth + TextWidth / 2));
  if (MaxLabels > 0)
  {
    int Step = (mXMax - mXMin + 1) / MaxLabels;
    if (Step <= 0)
    {
      Step = 1;
    }
    for (int Value = mXMin; Value <= mXMax; Value += Step)
    {
      string String = GetText(Value);
      Dc.GetTextExtent(String, &TextWidth, &TextHeight);
      int YPosition = mY + mHeight;
      float XPosition = mX + mWidth * (Value - mXMin) / (mXMax - mXMin + 1);

      // Center text.
      XPosition -= TextWidth / 2.0f;

      // Middle of bar.
      XPosition += 0.5f * mWidth / mpRandomArray->Size();

      Dc.DrawText(String, (int)XPosition, YPosition);
    }
  }

  Dc.SetFont(*wxNORMAL_FONT);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZArrayControl::DrawYTicks(wxDC& Dc)
{
  Dc.SetFont(*wxSMALL_FONT);

  if (mStyleBits & ARED_YTICKS)
  {
    // Compute tickmark y-distance.
    int TextWidth, TextHeight;
    Dc.GetTextExtent("-123", &TextWidth, &TextHeight);
    int MaxLabels = (int)(mHeight / (TextHeight + TextHeight / 2));
    if (MaxLabels > 0)
    {
      int Step =
        (mpRandomArray->GetMax() - mpRandomArray->GetMin()) / MaxLabels;
      if (Step <= 0)
      {
        Step = 1;
      }
      for (
        int Value = mpRandomArray->GetMin();
        Value < mpRandomArray->GetMax();
        Value += Step)
      {
        string String = GetText(Value);
        Dc.GetTextExtent(String, &TextWidth, &TextHeight);
        int YPosition =
          mY + mHeight -
          mHeight * (Value - mpRandomArray->GetMin()) /
          (mpRandomArray->GetMax() - mpRandomArray->GetMin()) -
          TextHeight / 2;
        Dc.DrawText(String, mX - TextWidth - TICK_LINE, YPosition);
      }
    }
  }
  else if (mStyleBits & ARED_MINMAX)
  {
    // mMin/mMax
    int TextWidth, TextHeight;
    ostringstream Oss;

    Oss << mpRandomArray->GetMax();
    Dc.GetTextExtent(Oss.str(), &TextWidth, &TextHeight);
    Dc.DrawText(Oss.str(), mX - TextWidth, mY);

    Oss.str("");

    Oss << mpRandomArray->GetMin();
    Dc.GetTextExtent(Oss.str(), &TextWidth, &TextHeight);
    Dc.DrawText(Oss.str(), mX - TextWidth, mY + mHeight - TextHeight);
  }

  Dc.SetFont(*wxNORMAL_FONT);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZArrayControl::DrawNull(wxDC& Dc)
{
  Dc.SetPen(*wxCYAN_PEN);

  // Draw y-null line.
  if (
    mpRandomArray->GetMin() < mpRandomArray->GetNull() &&
    mpRandomArray->GetNull() < mpRandomArray->GetMax())
  {
    Dc.DrawLine(
      mX,
      mpRandomArray->GetNull(),
      mX + mWidth,
      mpRandomArray->GetNull());
  }

  // Draw x-null line.
  if (mXMin < 0 && 0 < mXMax)
  {
    int x0 = mWidth * (0 - mXMin) / (mXMax - mXMin);
    Dc.DrawLine(x0, mY, x0, mY + mHeight);
  }

  Dc.SetPen(*wxBLACK_PEN);
}
