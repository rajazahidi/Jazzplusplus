//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Copyright (C) 2009-2013 Peter J. Stieber, all rights reserved.
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

#include "EventFrame.h"

#include "EventWindow.h"
#include "Resources.h"
#include "ToolBar.h"

#include <iostream>

using namespace std;

//*****************************************************************************
// Description:
//   This is the event frame class definition.
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(JZEventFrame, wxFrame)

  EVT_MENU(ID_SHIFT, JZEventFrame::OnShift)

  EVT_MENU(ID_CLEANUP, JZEventFrame::OnCleanup)

  EVT_MENU(ID_SEARCH_AND_REPLACE, JZEventFrame::OnSearchReplace)

  EVT_MENU(ID_QUANTIZE, JZEventFrame::OnQuantize)

  EVT_MENU(ID_SET_CHANNEL, JZEventFrame::OnSetChannel)

  EVT_MENU(ID_TRANSPOSE, JZEventFrame::OnTranspose)

  EVT_MENU(wxID_DELETE, JZEventFrame::OnDelete)

  EVT_MENU(ID_VELOCITY, JZEventFrame::OnVelocity)

  EVT_MENU(ID_LENGTH, JZEventFrame::OnLength)

  EVT_MENU(ID_MISC_METER_CHANGE, JZEventFrame::OnMeterChange)

END_EVENT_TABLE()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZEventFrame::JZEventFrame(
  wxWindow* pParent,
  const wxString& Title,
  JZSong* pSong,
  const wxPoint& Position,
  const wxSize& Size,
  long WindowStyle)
  : wxFrame(pParent, wxID_ANY, Title, Position, Size, WindowStyle),
    mpToolBar(0),
    mpEventWindow(0)
{
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZEventFrame::~JZEventFrame()
{
  delete mpToolBar;
}

//-----------------------------------------------------------------------------
// Description:
//   Register the event window with the frame.
//-----------------------------------------------------------------------------
void JZEventFrame::SetEventWindow(JZEventWindow* pEventWindow)
{
  mpEventWindow = pEventWindow;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool JZEventFrame::OnClose()
{
  return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::OnUpdateEventsSelected(wxUpdateUIEvent& Event)
{
  if (mpEventWindow)
  {
    Event.Enable(mpEventWindow->AreEventsSelected());
  }
  else
  {
    Event.Enable(false);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::OnShift(wxCommandEvent& Event)
{
  if (mpEventWindow && mpEventWindow->EventsSelected("Please select a range of events or track first."))
  {
    mpEventWindow->Shift(16);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::OnQuantize(wxCommandEvent& Event)
{
  if (mpEventWindow && mpEventWindow->EventsSelected("Please select a range of events or track first."))
  {
    mpEventWindow->Quantize();
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::OnSetChannel(wxCommandEvent& Event)
{
  if (mpEventWindow && mpEventWindow->EventsSelected("Please select a range of events or track first."))
  {
    mpEventWindow->SetChannel();
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::OnTranspose(wxCommandEvent& Event)
{
  if (mpEventWindow && mpEventWindow->EventsSelected("Please select a range of events or track first."))
  {
    mpEventWindow->Transpose();
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::OnDelete(wxCommandEvent& Event)
{
  if (mpEventWindow && mpEventWindow->EventsSelected("Please select a range of events or track first."))
  {
    mpEventWindow->Delete();
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::OnVelocity(wxCommandEvent& Event)
{
  if (mpEventWindow && mpEventWindow->EventsSelected("Please select a range of events or track first."))
  {
    mpEventWindow->Velocity();
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::OnLength(wxCommandEvent& Event)
{
  if (mpEventWindow && mpEventWindow->EventsSelected("Please select a range of events or track first."))
  {
    mpEventWindow->Length();
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::OnConvertToModulation(wxCommandEvent& Event)
{
  if (mpEventWindow && mpEventWindow->EventsSelected("Please select a range of events or track first."))
  {
    mpEventWindow->ConvertToModulation();
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::OnCleanup(wxCommandEvent& Event)
{
  if (mpEventWindow && mpEventWindow->EventsSelected("Please select a range of events or track first."))
  {
    mpEventWindow->Cleanup();
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::OnSearchReplace(wxCommandEvent& Event)
{
  if (mpEventWindow && mpEventWindow->EventsSelected("Please select a range of events or track first."))
  {
    mpEventWindow->SearchReplace();
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::OnMeterChange(wxCommandEvent& Event)
{
  if (mpEventWindow)
  {
    mpEventWindow->EditMeter();
  }
}
