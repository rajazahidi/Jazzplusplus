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

#include "EventWindow.h"

#include "Command.h"
#include "Dialogs/ShiftDialog.h"
#include "EventFrame.h"
#include "Filter.h"
#include "MouseAction.h"
#include "Song.h"
#include "Help.h"
#include "ProjectManager.h"
#include "PropertyListDialog.h"

#include <wx/dc.h>
#include <wx/msgdlg.h>

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
  mpSnapSel = new JZSnapSelection(this);

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
bool JZEventWindow::AreEventsSelected()
{
  return mpSnapSel->IsSelected();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool JZEventWindow::EventsSelected(const wxString& Message) const
{
  if (!mpSnapSel->IsSelected())
  {
    wxMessageBox(Message, "Error", wxOK);
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
// Description:
//   Display the "shift events" dialog.
//-----------------------------------------------------------------------------
void JZEventWindow::Shift(int Units)
{
  if (AreEventsSelected())
  {
    int Unit = 30;
    int Shift = 0;
    JZShiftDialog ShiftDialog(*this, *mpFilter, Units, Shift, this);

    if (ShiftDialog.ShowModal() == wxID_OK && Shift != 0)
    {
      tCmdShift ShiftCommand(mpFilter, Shift * Unit);
      ShiftCommand.Execute();

      JZProjectManager::Instance()->UpdateAllViews();
    }
  }
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
  int TotalClockTics =
    mpSong->GetMaxQuarters() * mpSong->GetTicksPerQuarter();
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
  JZBarInfo BarInfo(*mpSong);
  BarInfo.SetClock(Clock);
  while (Next--)
  {
    BarInfo.Next();
  }
  return BarInfo.GetClock();
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
// MeterChange Dialog
//*****************************************************************************
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
