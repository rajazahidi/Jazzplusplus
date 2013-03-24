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

#include "Random.h"

#include <wx/dcclient.h>

//*****************************************************************************
// Description:
//   This is the array control class definition.
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(JZArrayControl, wxControl)
  EVT_SIZE(JZArrayControl::OnSize)
  EVT_PAINT(JZArrayControl::OnPaint)
END_EVENT_TABLE()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZArrayControl::JZArrayControl(
  wxWindow* pParent,
  wxWindowID Id,
  const JZRndArray& RandomArray,
  const wxPoint& Position,
  const wxSize& Size,
  long WindowStyle,
  const wxValidator& Validator,
  const wxString& Name)
  : wxControl(),
    mpRandomArray(0)
{
  mpRandomArray = new JZRndArray(RandomArray);

  Create(
    pParent,
    Id,
    RandomArray,
    Position,
    Size,
    WindowStyle,
    Validator,
    Name);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZArrayControl::~JZArrayControl()
{
  delete mpRandomArray;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZArrayControl::Create(
  wxWindow* pParent,
  wxWindowID Id,
  const JZRndArray& RandomArray,
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

  *mpRandomArray = RandomArray;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZArrayControl::OnSize(wxSizeEvent& SizeEvent)
{
  int Width = SizeEvent.GetSize().GetWidth();
  int Height = SizeEvent.GetSize().GetHeight();

  SizeEvent.Skip();

  wxClientDC Dc(this);

  int TextWidth, TextHeight;
  Dc.GetTextExtent("123", &TextWidth, &TextHeight);

#if 0
  if (mStyleBits & ARED_XTICKS)
  {
    Height -= TextHeight;
  }
  if (mStyleBits & (ARED_MINMAX | ARED_YTICKS))
  {
    x = (int)(TextWidth + TICK_LINE);
    Width -= (int)(TextWidth + TICK_LINE);
  }
  ynul = y + height - Height * (nul - min) / (max - min);
#endif
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZArrayControl::OnPaint(wxPaintEvent& Event)
{
}
