#include "EventFrame.h"

#include "Command.h"
#include "Dialogs.h"
#include "EventWindow.h"
#include "Filter.h"
#include "MouseAction.h"
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

  EVT_UPDATE_UI(ID_SHIFT, JZEventFrame::OnUpdateEditShift)
  EVT_MENU(ID_SHIFT, JZEventFrame::OnEditShift)

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
    Song(pSong),
    mpFilter(0),
    mTrackHeight(0),
    mTopInfoHeight(40),
    FontSize(12),
    ClocksPerPixel(36),
    mEventsX(),
    mEventsY(mTopInfoHeight),
    mEventsWidth(0),
    mEventsHeight(0),
    CanvasX(0),
    CanvasY(0),
    CanvasW(0),
    CanvasH(0),
    FromClock(0),
    ToClock(0),
    FromLine(0),
    ToLine(0),
    SnapSel(0),
    MouseAction(0),
    PlayClock(-1),
    mpSettingsDialog(0),
    MixerForm(0),
    mpToolBar(0),
    mpGreyColor(0),
    mpGreyBrush(0),
    mpEventWindow(0)
{
#ifdef __WXMSW__
  mpGreyColor = new wxColor(192, 192, 192);
#else
  mpGreyColor = new wxColor(220, 220, 220);
#endif
  mpGreyBrush = new wxBrush(*mpGreyColor, wxSOLID);

  mpFilter = new JZFilter(Song);
}

JZEventFrame::~JZEventFrame()
{
  delete SnapSel;

  delete mpGreyColor;
  delete mpGreyBrush;

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
void JZEventFrame::OnUpdateEditShift(wxUpdateUIEvent& Event)
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
void JZEventFrame::OnEditShift(wxCommandEvent& Event)
{
  if (mpEventWindow)
  {
    mpEventWindow->Shift(16);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool JZEventFrame::OnCharHook(wxKeyEvent& e)
{
  return OnKeyEvent(e);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZEventFrame::y2Line(int y, int up)
{
  if (up)
  {
    y += mTrackHeight;
  }
  y -= mTopInfoHeight;
  return y / mTrackHeight;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZEventFrame::Line2y(int Line)
{
  return Line * mTrackHeight + mTopInfoHeight;
}

/*
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::LineText(wxDC *dc, int x, int y, int w, const char *str, int h, bool down)
{
  if (h <= 0)
  {
    h = mTrackHeight;
    y = y2yLine(y);
  }
  if (w && h)
  {
    //dc->SetBrush(wxGREY_BRUSH);
    dc->SetBrush(*mpGreyBrush);
    dc->SetPen(*wxGREY_PEN);
    #ifdef __WXMSW__
    dc->DrawRectangle(x, y, w+1, h+1);
    #else
    dc->DrawRectangle(x, y, w, h);
    #endif
    x += 1;
    y += 1;
    w -= 2;
    h -= 2;
    if (down)
    {
      dc->SetPen(*wxBLACK_PEN);
      dc->DrawLine(x, y, x+w, y);
      dc->DrawLine(x, y, x, y+h);
      dc->SetPen(*wxWHITE_PEN);
      dc->DrawLine(x+w, y, x+w, y+h);
      dc->DrawLine(x, y+h, x+w, y+h);
    }
    else
    {
      dc->SetPen(*wxWHITE_PEN);
      dc->DrawLine(x, y, x+w, y);
      dc->DrawLine(x, y, x, y+h);
      dc->SetPen(*wxBLACK_PEN);
      dc->DrawLine(x+w, y, x+w, y+h);
      dc->DrawLine(x, y+h, x+w, y+h);
    }
    dc->SetPen(*wxBLACK_PEN);
    x -= 2;
    y -= 2;
  }
  dc->SetTextBackground(*mpGreyColor);
  dc->DrawText((char *)str, x + mLittleBit, y + mLittleBit);
  dc->SetTextBackground(*wxWHITE);
}
*/

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::Redraw()
{
//   wxDC* dc=new wxClientDC(this);
//   wxPaintEvent e;
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
bool JZEventFrame::OnKeyEvent(wxKeyEvent &e)
{
  return false;
}

//-----------------------------------------------------------------------------
// seems to handle the "selection" rectangle. normally called from the base
// class onmouseevent handler
//-----------------------------------------------------------------------------
int JZEventFrame::OnMouseEvent(wxMouseEvent &e)
{
  //  cout <<"JZEventFrame::OnMouseEvent"<<endl;
  if (!MouseAction)
  {
    // create SnapSel?
    int x;
    int y;
    e.GetPosition(&x, &y);
    if (mEventsX < x && x < mEventsX + mEventsWidth && mEventsY < y && y < mEventsY + mEventsHeight)
    {
      if (e.LeftDown())
      {
        {
          SnapSelStart(e);

          if (SnapSel->IsSelected())
          {
            Refresh(); //redraw the whole window instead(inefficient, we should rather invalidate a rect)
          }
          SnapSel->Event(e);
          MouseAction = SnapSel;
        }
      }
    }
  }
  else
  {
    // MouseAction active

    if (MouseAction->Event(e))
    {
      // MouseAction finished

      if (MouseAction == SnapSel)
      {
        SnapSelStop(e);
        Redraw(); //ineficcient, invalidate rect first instead
        MouseAction = 0;
        return 1;
      }

      MouseAction = 0;
    }
  }
  return 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool JZEventFrame::OnClose()
{
  return FALSE;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::OnMenuCommand(int)
{
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::SnapSelStart(wxMouseEvent& MouseEvent)
{
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::SnapSelStop(wxMouseEvent& MouseEvent)
{
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZEventFrame::EventsSelected(const char* msg)
{
  if (!SnapSel->IsSelected())
  {
    if (msg == 0)
    {
      msg = "please select some events first";
    }
    wxMessageBox((char *)msg, "Error", wxOK);
    return 0;
  }
  return 1;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::MenQuantize()
{
  if (!EventsSelected())
    return;
  //  wxDialogBox *panel = new wxDialogBox(this, "Quantize", FALSE );
  tQuantizeDlg * dlg = new tQuantizeDlg(this, mpFilter);
  dlg->Create();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::MenCleanup()
{
  if (!EventsSelected())
    return;
  tCleanupDlg * dlg = new tCleanupDlg(this, mpFilter);
  dlg->Create();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::MenSearchReplace()
{
  if (!EventsSelected())
    return;
  tSearchReplaceDlg * dlg = new tSearchReplaceDlg(this, mpFilter);
  dlg->Create();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::MenSetChannel()
{
  if (!EventsSelected())
    return;
  tSetChannelDlg * dlg = new tSetChannelDlg(mpFilter);
  dlg->Create();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::MenTranspose()
{
  if (!EventsSelected())
    return;
  tTransposeDlg * dlg = new tTransposeDlg(this, mpFilter);
  dlg->Create();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::MenDelete()
{
  if (!EventsSelected())
    return;
  tDeleteDlg * dlg = new tDeleteDlg(this, mpFilter);
  dlg->Create();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::MenVelocity()
{
  if (!EventsSelected())
    return;
  tVelocityDlg * dlg = new tVelocityDlg(mpFilter);
  dlg->Create();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::MenLength()
{
  if (!EventsSelected())
    return;
  tLengthDlg * dlg = new tLengthDlg(this, mpFilter);
  dlg->Create();
}

//-----------------------------------------------------------------------------
// convert to modulation
//-----------------------------------------------------------------------------
void JZEventFrame::MenConvertToModulation()
{
  if (!EventsSelected())
    return;
  tCmdConvertToModulation cmd(mpFilter);
  cmd.Execute();
  Redraw();
}
