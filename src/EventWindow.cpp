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

#include "EventWindow.h"
#include "PianoFrame.h"
#include "Song.h"
#include "Command.h"
#include "Dialogs.h"
#include "Help.h"
#include "ToolBar.h"
#include "PropertyListDialog.h"

using namespace std;


//*****************************************************************************
// Description:
//   This is the event window class definition.
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//BEGIN_EVENT_TABLE(JZEventWindow, wxScrolledWindow)
//END_EVENT_TABLE()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZEventWindow::JZEventWindow(
  wxFrame* pParent,
  JZSong* pSong,
  const wxPoint& Position,
  const wxSize& Size)
  : wxWindow(
      pParent,
      wxID_ANY,
      Position,
      Size,
      wxHSCROLL | wxVSCROLL | wxNO_FULL_REPAINT_ON_RESIZE),
    mpSnapSel(0),
    mpFilter(0),
    mpSong(pSong),
    mpGreyColor(0),
    mpGreyBrush(0),
    mClockTicsPerPixel(36),
    mTopInfoHeight(40),
    mLeftInfoWidth(100),
    mTrackHeight(10),
    mLittleBit(2),
    mEventsX(0),
    mEventsY(mTopInfoHeight),
    mEventsWidth(),
    mEventsHeight(),
    mCanvasWidth(0),
    mCanvasHeight(0),
    mFromClock(0),
    mToClock(0),
    mFromLine(0),
    mToLine(0),
    mScrolledX(0),
    mScrolledY(0)
{
  mpSnapSel = new tSnapSelection(this);

  mpFilter = new JZFilter(mpSong);

#ifdef __WXMSW__
  mpGreyColor = new wxColor(192, 192, 192);
#else
  mpGreyColor = new wxColor(220, 220, 220);
#endif

  mpGreyBrush = new wxBrush(*mpGreyColor, wxSOLID);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZEventWindow::~JZEventWindow()
{
  delete mpSnapSel;
  delete mpFilter;
  delete mpGreyColor;
  delete mpGreyBrush;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZEventWindow::EventsSelected(const wxString& Message) const
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
void JZEventWindow::GetVirtualEventSize(
  int& EventWidth,
  int& EventHeight) const
{
  int TotalClockTics = mpSong->MaxQuarters * mpSong->TicksPerQuarter;
  EventWidth = TotalClockTics / mClockTicsPerPixel;
  EventHeight = 127 * mTrackHeight;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventWindow::SetXScrollPosition(int x)
{
  // The following line converts an x position in window coordinates to an
  // x position in scrolled coordinates.
  int ScrolledX = x - mEventsX + mScrolledX;

  if (mScrolledX != ScrolledX)
  {
    mScrolledX = ScrolledX;

    // Set the new from clock and to clock positions based on the new scroll
    // position.
    mFromClock = mScrolledX * mClockTicsPerPixel;
    mToClock = x2Clock(mCanvasWidth);

    SetScrollPos(wxHORIZONTAL, mScrolledX);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventWindow::SetYScrollPosition(int y)
{
  // The following line converts a y position in window coordinates to a
  // y position in scrolled coordinates.
  int ScrolledY = y - mEventsY + mScrolledY;

  if (mScrolledY != y)
  {
    mScrolledY = ScrolledY;

    // Set the new from line and to line positions based on the new scroll
    // position.
    mFromLine = mScrolledY / mTrackHeight;
    mToLine = 1 + (mScrolledY + mCanvasHeight - mTopInfoHeight) / mTrackHeight;

    SetScrollPos(wxVERTICAL, mScrolledY);
  }
}

//-----------------------------------------------------------------------------
// Description:
//   This function takes an x-pixel value in window coordinates and converts
// it to clock tics.
//-----------------------------------------------------------------------------
int JZEventWindow::x2Clock(int x)
{
  return (x - mEventsX) * mClockTicsPerPixel + mFromClock;
}

//-----------------------------------------------------------------------------
// Description:
//   This function takes clock tics and converts the value into an x-pixel
// location on the screen in window coordinates.
//-----------------------------------------------------------------------------
int JZEventWindow::Clock2x(int Clock)
{
  return mEventsX + (Clock - mFromClock) / mClockTicsPerPixel;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZEventWindow::x2BarClock(int x, int Next)
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
int JZEventWindow::y2yLine(int y, int Up)
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
// Was the VLine macro
//-----------------------------------------------------------------------------
void JZEventWindow::DrawVerticalLine(wxDC& Dc, int XPosition) const
{
  Dc.DrawLine(XPosition, 0, XPosition, mEventsY + mEventsHeight);
}

//-----------------------------------------------------------------------------
// Was the HLine macro
//-----------------------------------------------------------------------------
void JZEventWindow::DrawHorizontalLine(wxDC& Dc, int YPosition) const
{
  Dc.DrawLine(0, YPosition, mCanvasWidth, YPosition);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventWindow::LineText(
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

  if (pString && strlen(pString) > 0)
  {
    wxColor TextBackgroundColor = Dc.GetTextBackground();
    Dc.SetTextBackground(*mpGreyColor);
    int TextWidth, TextHeight;
    Dc.GetTextExtent(pString, &TextWidth, &TextHeight);
    int Margin = (Width - TextWidth) / 2;
    if (Margin < mLittleBit)
    {
      Margin = mLittleBit;
    }
    Dc.DrawText(pString, x + Margin, y + mLittleBit);
    Dc.SetTextBackground(TextBackgroundColor);
  }
}

//-----------------------------------------------------------------------------
//   This mouse handler delegates to the subclased event window.
//-----------------------------------------------------------------------------
//void JZEventWindow::OnMouseEvent(wxMouseEvent& MouseEvent)
//{
//  EventWin->OnMouseEvent(MouseEvent);
//}

// JAVE the OnChar method seems to be gone in wxwin232, but its documented, so
// I don't know what happened.  The OnCharHook should do the same thing
// basically.  It was there from the start.  OnChar seemd redundant.

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// void JZEventWindow::OnChar(wxKeyEvent& KeyEvent)
// {
//   if (!EventWin->OnKeyEvent(KeyEvent))
//   {
//     wxWindow::OnChar(KeyEvent);
//   }
// }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// void JZEventFrame::OnChar(wxKeyEvent& KeyEvent)
// {
//   if (!OnKeyEvent(KeyEvent))
//   {
//     wxFrame::OnChar(KeyEvent);
//   }
// }

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//bool JZEventWindow::OnCharHook(wxKeyEvent& KeyEvent)
//{
//  return EventWin->OnKeyEvent(KeyEvent);
//}

//*****************************************************************************
// Description:
//   This is the event frame class definition.
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(JZEventFrame, wxFrame)
  EVT_SIZE(JZEventFrame::OnSize)
END_EVENT_TABLE()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZEventFrame::JZEventFrame(
  wxWindow* pParent,
  const wxString& Title,
  JZSong* pSong,
  const wxPoint& Position,
  const wxSize& Size)
  : wxFrame(pParent, wxID_ANY, Title, Position, Size),
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
    mpGreyBrush(0)
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



void JZEventFrame::CreateMenu()
{
}


/**

create the canvas component(used for differently dependingon the subclass)
size it to the client area of the frame(frame size minus toolbar and menus )
*/
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


// Initialize the constants used in drawing.
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


/**
this onsize handler is supposed to take care of handling of the resizing the two subwindows sizes to
they dont overlap
*/
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

// *******************************************************************
// Coord-Functions
// *******************************************************************


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

int JZEventFrame::y2Line(int y, int up)
{
  if (up)
  {
    y += mTrackHeight;
  }
  y -= mTopInfoHeight;
  return y / mTrackHeight;
}


int JZEventFrame::Line2y(int Line)
{
  return Line * mTrackHeight + mTopInfoHeight;
}

/*
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

// *******************************************************************
// Painting behavior
// *******************************************************************

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

// ******************************************************************
// Mouse
// ******************************************************************

bool JZEventFrame::OnKeyEvent(wxKeyEvent &e)
{
  return false;
}

/** seems to handle the "selection" rectangle. normally called from the base class onmouseevent handler */
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

          if (SnapSel->Selected){
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

// ******************************************************************
// dummies
// ******************************************************************

bool JZEventFrame::OnClose()
{
  return FALSE;
}

void JZEventFrame::OnMenuCommand(int)
{
}

void JZEventFrame::SnapSelStart(wxMouseEvent& MouseEvent)
{
}

void JZEventFrame::SnapSelStop(wxMouseEvent& MouseEvent)
{
}

//-----------------------------------------------------------------------------
// PlayPosition
//-----------------------------------------------------------------------------

// **************************************************************************
// EventsSelected
// **************************************************************************

int JZEventFrame::EventsSelected(const char *msg)
{
  if (!SnapSel->Selected)
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

// **************************************************************************
// Quantize
// **************************************************************************

void JZEventFrame::MenQuantize()
{


  if (!EventsSelected())
    return;
  //  wxDialogBox *panel = new wxDialogBox(this, "Quantize", FALSE );
  tQuantizeDlg * dlg = new tQuantizeDlg(this, mpFilter);
  dlg->Create();


}

// **************************************************************************
// Cleanup
// **************************************************************************

void JZEventFrame::MenCleanup()
{


  if (!EventsSelected())
    return;
  tCleanupDlg * dlg = new tCleanupDlg(this, mpFilter);
  dlg->Create();

}


// **************************************************************************
// SearchReplace
// **************************************************************************

void JZEventFrame::MenSearchReplace()
{


  if (!EventsSelected())
    return;
  tSearchReplaceDlg * dlg = new tSearchReplaceDlg(this, mpFilter);
  dlg->Create();


}


// **************************************************************************
// SetChannel
// **************************************************************************

void JZEventFrame::MenSetChannel()
{


  if (!EventsSelected())
    return;
  tSetChannelDlg * dlg = new tSetChannelDlg(mpFilter);
  dlg->Create();

}


// **************************************************************************
// Transpose
// **************************************************************************

void JZEventFrame::MenTranspose()
{


  if (!EventsSelected())
    return;
  tTransposeDlg * dlg = new tTransposeDlg(this, mpFilter);
  dlg->Create();


}

/**show the "shift events" dialog */

void JZEventFrame::MenShift(int Unit)
{
  if (EventsSelected())
  {

    tShiftDlg * dlg = new tShiftDlg(this, mpFilter, Unit);
    dlg->Create();
  }


}


// ********************************************************************************
// Delete
// ********************************************************************************


void JZEventFrame::MenDelete()
{


  if (!EventsSelected())
    return;
  tDeleteDlg * dlg = new tDeleteDlg(this, mpFilter);
  dlg->Create();
}

// ********************************************************************************
// Velocity
// ********************************************************************************


void JZEventFrame::MenVelocity()
{

  if (!EventsSelected())
    return;
  tVelocityDlg * dlg = new tVelocityDlg(mpFilter);
  dlg->Create();
}

// ********************************************************************************
// Length
// ********************************************************************************


void JZEventFrame::MenLength()
{


  if (!EventsSelected())
    return;
  tLengthDlg * dlg = new tLengthDlg(this, mpFilter);
  dlg->Create();

}


// ********************************************************************************
// convert to modulation
// ********************************************************************************


void JZEventFrame::MenConvertToModulation()
{


  if (!EventsSelected())
    return;
  tCmdConvertToModulation cmd(mpFilter);
  cmd.Execute();
  Redraw();
}





// ******************************************************************
// MeterChange Dialog
// ******************************************************************


class tMeterChangeDlg : public tPropertyListDlg
{
public:
  JZEventFrame *EventWin;
  static int Numerator;
  static int Denomiator;
  static int BarNr;
  tMeterChangeDlg(JZEventFrame *w);
  void AddProperties();
  virtual bool OnClose();
  virtual void OnCancel();
  virtual void OnHelp();
};


int tMeterChangeDlg::Numerator = 4;
int tMeterChangeDlg::Denomiator = 4;
int tMeterChangeDlg::BarNr = 1;

tMeterChangeDlg::tMeterChangeDlg(JZEventFrame *w)
  : tPropertyListDlg("Meter Change")
{
  EventWin = w;
}


void tMeterChangeDlg::OnCancel()
{
  EventWin->mpSettingsDialog = 0;
  //wxForm::OnCancel();
}


bool tMeterChangeDlg::OnClose()
{
  BarNr += EventWin->Song->GetIntroLength();
  EventWin->Song->SetMeterChange(BarNr, Numerator, Denomiator);
  EventWin->Redraw();
  EventWin->mpSettingsDialog = 0;
  //wxForm::OnOk();
  return false;
}

void tMeterChangeDlg::OnHelp()
{
  gpHelpInstance->ShowTopic("Meterchange");
}


void tMeterChangeDlg::AddProperties()
{
 //  Add(wxMakeFormShort("BarNr:",     &BarNr,      wxFORM_DEFAULT, 0,0,0,100));
//   Add(wxMakeFormNewLine());
//   Add(wxMakeFormShort("Numerator",  &Numerator,  wxFORM_DEFAULT, 0,0,0,100));
//   Add(wxMakeFormNewLine());
//   Add(wxMakeFormShort("Denomiator", &Denomiator, wxFORM_DEFAULT, 0,0,0,100));
//   Add(wxMakeFormNewLine());
//   Add(wxMakeFormMessage("Supported Denomiators: 2,4,8,16,32"));
//   AssociatePanel(panel);
  sheet->AddProperty(new wxProperty(
    "BarNr",
    wxPropertyValue(&BarNr),
    "integer"));//JAVE validators here? problem is i dont know which ranges are valid FIXME
  sheet->AddProperty(new wxProperty(
    "Numerator",
    wxPropertyValue(&Numerator),
    "integer"));
  sheet->AddProperty(new wxProperty(
    "Denomiator(2,4,8,16,32)",
    wxPropertyValue(&Denomiator),
     "integer"));//JAVE should be a integer list instead FIXME
}



void JZEventFrame::MenMeterChange()
{
  tMeterChangeDlg *dlg;
  if (mpSettingsDialog)
  {
    mpSettingsDialog->Show(TRUE);
    return;
  }
  //  mpSettingsDialog = new wxDialogBox(this, "MeterChange", FALSE );
  dlg = new tMeterChangeDlg(this);
  dlg->Create();
}
