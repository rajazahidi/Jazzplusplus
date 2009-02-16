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

  EVT_UPDATE_UI(ID_SHIFT, JZEventFrame::OnUpdateEventsSelected)
  EVT_MENU(ID_SHIFT, JZEventFrame::OnShift)

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
void JZEventFrame::Redraw()
{
//   wxDC* dc=new wxClientDC(this);
//   wxPaintEvent PaintEvent;
//   cout<<"FIXME JZEventFrame::Redraw"<<endl;
//   mpEventWindow->OnDraw(*dc); //this will in turn call the eventwin onpaintsub
//   // the problem is that onpaint no longer takes arguments, and is supposed
//   // to be called from the framework only, so it should be split.
//   delete dc;

//  if (mpEventWindow)
//  {
//    mpEventWindow->Refresh();
//  }
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
  if (mpEventWindow)
  {
    mpEventWindow->Shift(16);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::OnQuantize(wxCommandEvent& Event)
{
  if (mpEventWindow)
  {
    mpEventWindow->Quantize();
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::OnSetChannel(wxCommandEvent& Event)
{
  if (!mpEventWindow || !mpEventWindow->AreEventsSelected())
  {
    mpEventWindow->SetChannel();
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::OnTranspose(wxCommandEvent& Event)
{
  if (!mpEventWindow || !mpEventWindow->AreEventsSelected())
  {
    mpEventWindow->Transpose();
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::OnDelete(wxCommandEvent& Event)
{
  if (!mpEventWindow || !mpEventWindow->AreEventsSelected())
  {
    mpEventWindow->Delete();
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::OnVelocity(wxCommandEvent& Event)
{
  if (!mpEventWindow || !mpEventWindow->AreEventsSelected())
  {
    mpEventWindow->Velocity();
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::OnLength(wxCommandEvent& Event)
{
  if (!mpEventWindow || !mpEventWindow->AreEventsSelected())
  {
    mpEventWindow->Length();
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::OnConvertToModulation(wxCommandEvent& Event)
{
  if (!mpEventWindow || !mpEventWindow->AreEventsSelected())
  {
    mpEventWindow->ConvertToModulation();
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::OnCleanup(wxCommandEvent& Event)
{
  if (!mpEventWindow || !mpEventWindow->AreEventsSelected())
  {
    mpEventWindow->Cleanup();
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::OnSearchReplace(wxCommandEvent& Event)
{
  if (!mpEventWindow || !mpEventWindow->AreEventsSelected())
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
