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

  EVT_SIZE(JZEventFrame::OnSize)

END_EVENT_TABLE()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZEventFrame::JZEventFrame(
  JZEventWindow* pEventWindow,
  const wxString& Title,
  JZSong* pSong,
  const wxPoint& Position,
  const wxSize& Size)
  : wxFrame(pEventWindow, wxID_ANY, Title, Position, Size),
    Song(pSong),
    mpFilter(0),
    mpFixedFont(0),
    hFixedFont(0),
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
    mpEventWindow(pEventWindow)
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

  delete mpFixedFont;

  delete mpToolBar;

  if (MixerForm)
  {
    delete MixerForm;
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::CreateMenu()
{
}


//-----------------------------------------------------------------------------
// create the canvas component(used for differently dependingon the subclass)
// size it to the client area of the frame(frame size minus toolbar and menus )
//-----------------------------------------------------------------------------
//void JZEventFrame::CreateCanvas()
//{
//  cout << "CreateCanvas" << endl;
//  int Width, Height;
//  GetClientSize(&Width, &Height);
//  mpEventWindow = new JZEventWindow(this, 0, 0, Width, Height);
//}

/**
second phase of creation. make menus, the canvas, and so on
*/
void JZEventFrame::Create()
{
  CreateMenu();

  Setup();
}

//-----------------------------------------------------------------------------
// Initialize the constants used in drawing.
//-----------------------------------------------------------------------------
void JZEventFrame::Setup()
{
/*
  int x, y;

  wxClientDC Dc(mpEventWindow);
  Dc.SetFont(wxNullFont);
  delete mpFixedFont;
  mpFixedFont = new wxFont(12, wxSWISS, wxNORMAL, wxNORMAL);
  Dc.SetFont(*mpFixedFont);
  Dc.GetTextExtent("M", &x, &y);
  hFixedFont = (int)y;

  delete mpFont;
  mpFont = new wxFont(FontSize, wxSWISS, wxNORMAL, wxNORMAL);
  Dc.SetFont(*mpFont);

  Dc.GetTextExtent("M", &x, &y);
  mLittleBit = (int)(x/2);

  Dc.GetTextExtent("HXWjgi", &x, &y);
  mTrackHeight = (int)y + mLittleBit;
*/
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::OnUpdateEditShift(wxUpdateUIEvent& Event)
{
  Event.Enable(mpEventWindow->AreEventsSelected());
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::OnEditShift(wxCommandEvent& Event)
{
//  mpEventWindow->Shift();
}

//-----------------------------------------------------------------------------
// this onsize handler is supposed to take care of handling of the resizing
// the two subwindows sizes to they dont overlap
//-----------------------------------------------------------------------------
void JZEventFrame::OnSize(wxSizeEvent& Event)
{
//  wxFrame::OnSize(Event);

  // The code below is from the toolbar sample, the layoutchidlren function
  wxSize size = GetClientSize();

  int offset;
//  if (mpToolBar)
//  {
//    mpToolBar->SetSize(-1, size.y);
//    mpToolBar->Move(0, 0);
//
//    offset = mpToolBar->GetSize().x;
//  }
//  else
//  {
//    offset = 0;
//  }

    // The step below should set the offset of the mpEventWindow
    // m_textWindow->SetSize(offset, 0, size.x - offset, size.y);

//  float maxToolBarWidth  = 0.0;
//  float maxToolBarHeight = 0.0;
//  if (mpToolBar)
//  {
//    mpToolBar->GetMaxSize(&maxToolBarWidth, &maxToolBarHeight);
//  }

  offset = mpToolBar->GetSize().y; //get the height of the toolbar

  int frameWidth, frameHeight;
  GetClientSize(&frameWidth, &frameHeight);

//     if (mpEventWindow)
//       //       mpEventWindow->SetSize(0, (int)offset, (int)frameWidth, (int)(frameHeight - offset));
//       mpEventWindow->SetSize(0, (int)0, (int)frameWidth, (int)(frameHeight));
// //   if (mpToolBar)
// //     mpToolBar->SetSize(0, 0, (int)frameWidth, (int)maxToolBarHeight);

  cout
    << "JZEventFrame::OnSize " << frameWidth<< 'x' << frameHeight << endl;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool JZEventFrame::OnCharHook(wxKeyEvent& e)
{
  return OnKeyEvent(e);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZEventFrame::y2yLine(int y, int up)
{
  if (up)
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
//   //the problem is that onpaint no longer tkes no argument, and is supposed to be called from the framework only, so it should be split
//   delete dc;

//  mpEventWindow->Refresh();
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
