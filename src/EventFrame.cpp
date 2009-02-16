#include "EventFrame.h"

#include "Command.h"
#include "Dialogs.h"
#include "EventWindow.h"
#include "Filter.h"
#include "Resources.h"
#include "ToolBar.h"

#include <wx/dc.h>
#include <wx/msgdlg.h>

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
    mpFilter(0),
    MixerForm(0),
    mpToolBar(0),
    mpEventWindow(0)
{
  mpFilter = new JZFilter(pSong);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZEventFrame::~JZEventFrame()
{
  delete mpFilter;

  delete mpToolBar;

  if (MixerForm)
  {
    delete MixerForm;
  }
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
bool JZEventFrame::OnKeyEvent(wxKeyEvent& KeyEvent)
{
  return false;
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
bool JZEventFrame::OnCharHook(wxKeyEvent& KeyEvent)
{
  return OnKeyEvent(KeyEvent);
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
   return;
  }

  tSetChannelDlg * dlg = new tSetChannelDlg(mpFilter);
  dlg->Create();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::OnTranspose(wxCommandEvent& Event)
{
  if (!mpEventWindow || !mpEventWindow->AreEventsSelected())
  {
   return;
  }

  tTransposeDlg * dlg = new tTransposeDlg(this, mpFilter);
  dlg->Create();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::OnDelete(wxCommandEvent& Event)
{
  if (!mpEventWindow || !mpEventWindow->AreEventsSelected())
  {
   return;
  }

  tDeleteDlg * dlg = new tDeleteDlg(this, mpFilter);
  dlg->Create();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::OnVelocity(wxCommandEvent& Event)
{
  if (!mpEventWindow || !mpEventWindow->AreEventsSelected())
  {
   return;
  }

  tVelocityDlg * dlg = new tVelocityDlg(mpFilter);
  dlg->Create();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::OnLength(wxCommandEvent& Event)
{
  if (!mpEventWindow || !mpEventWindow->AreEventsSelected())
  {
   return;
  }

  tLengthDlg * dlg = new tLengthDlg(this, mpFilter);
  dlg->Create();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::OnConvertToModulation(wxCommandEvent& Event)
{
  if (!mpEventWindow || !mpEventWindow->AreEventsSelected())
  {
   return;
  }

  tCmdConvertToModulation cmd(mpFilter);
  cmd.Execute();
  Redraw();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::OnCleanup(wxCommandEvent& Event)
{
  if (!mpEventWindow || !mpEventWindow->AreEventsSelected())
  {
   return;
  }

  tCleanupDlg * dlg = new tCleanupDlg(this, mpFilter);
  dlg->Create();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::OnSearchReplace(wxCommandEvent& Event)
{
  if (!mpEventWindow || !mpEventWindow->AreEventsSelected())
  {
   return;
  }

  tSearchReplaceDlg * dlg = new tSearchReplaceDlg(this, mpFilter);
  dlg->Create();
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
