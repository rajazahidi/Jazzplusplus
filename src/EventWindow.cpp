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
const int JZEventWindow::mScrollSize = 50;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZEventWindow::JZEventWindow(
  wxFrame* pParent,
  JZSong* pSong,
  const wxPoint& Position,
  const wxSize& Size)
  : wxScrolledWindow(
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
    mTopInfoHeight(40),
    mTrackHeight(10),
    mLittleBit(2)
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
//-----------------------------------------------------------------------------
//void JZEventWindow::SetScrollRanges()
//{
//  int Width, Height;
//  GetVirtualEventSize(Width, Height);
//  SetScrollbars(
//    mScrollSize,
//    mScrollSize,
//    Width / mScrollSize,
//    Height / mScrollSize);
//  EnableScrolling(false, false);
//}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventWindow::SetScrollPosition(int x, int y)
{
  x /= mScrollSize;
  y /= mScrollSize;
  Scroll(x, y);
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
// JAVE seems to want to clip the paint area
// calls the subclass paint routine
//
// OnPaint seems never to get called
//-----------------------------------------------------------------------------
//void JZEventWindow::OnDraw(wxDC& Dc)
//{
//  //onpaint never seems to get called, but ondraw does get called
//  int x = 0, y = 0;
//  GetViewStart(&x, &y);
//  EventWin->OnPaintSub(Dc, x * mScrollSize, y * mScrollSize);
//  cout << "JZEventWindow::OnDraw << endl;
//}

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
    NextWin(0),
//    mpEventWindow(0),
    mpFont(0),
    mpFixedFont(0),
    hFixedFont(0),
    LittleBit(1),
    mTrackHeight(0),
    mTopInfoHeight(40),
    mLeftInfoWidth(100),
    FontSize(12),
    ClocksPerPixel(36),
    UseColors(true),
    mEventsX(mLeftInfoWidth),
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

  delete mpFont;
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
  cout <<"JZEventFrame::Create\n";
  CreateMenu();

//  CreateCanvas();
//  SnapSel = new tSnapSelection(mpEventWindow);


  Setup();
//  mpEventWindow->SetScrollRanges();
//  mpEventWindow->SetScrollPosition(0, 0); //this wasnt here before wx2, why?
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
  LittleBit = (int)(x/2);

  Dc.GetTextExtent("HXWjgi", &x, &y);
  mTrackHeight = (int)y + LittleBit;
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


int JZEventFrame::x2Clock(int x)
{
  return (x - mEventsX) * ClocksPerPixel + FromClock;
}


int JZEventFrame::Clock2x(int clk)
{
  return mEventsX + (clk - FromClock) / ClocksPerPixel;
}

int JZEventFrame::x2BarClock(int x, int next)
{
  int clk = x2Clock(x);
  JZBarInfo b(Song);
  b.SetClock(clk);
  while (next--)
  {
    b.Next();
  }
  return b.Clock;
}


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
  dc->DrawText((char *)str, x + LittleBit, y + LittleBit);
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

/**
   JAVE this was originally called OnPaint(x,y), but i renamed it because i confused it with the OnPaint() framework routine
   the call graph feels odd: the canvas is a member of the eventwin, with a pointer to the parent. the child calss the parent to redraw itself

   it doesnt do any real drawing, instead it sets up some member vars, to be used by other parts of the class

   it is now normally called from OnDraw in the mpEventWindow class,and also overridden in the subclass.
   so this one here just sets up  constants


   dc is the device context to draw in, normally generated from the framework from ondraw
   x and y is the coordinates of the start of the view

*/
void JZEventFrame::OnPaintSub(wxDC *dc, int x, int y)
{
  //printf("EventWin::OnPaintSub: x %ld, y %ld, w %ld, h %ld\n", x, y, w, h);
  CanvasX = x;
  CanvasY = y;
// wxCanvas::GetClientSize returns huge values, at least in wx_xt
  int xc, yc;
  GetClientSize(&xc, &yc);
  CanvasW = xc;
  CanvasH = yc;

  mEventsX = CanvasX + mLeftInfoWidth;
  mEventsY = CanvasY + mTopInfoHeight;
  mEventsWidth = CanvasW - mLeftInfoWidth;
  mEventsHeight = CanvasH - mTopInfoHeight;

  FromLine = CanvasY / mTrackHeight;
  ToLine   = (CanvasY + CanvasH - mTopInfoHeight) / mTrackHeight;
  FromClock = CanvasX * ClocksPerPixel;
  ToClock = x2Clock(CanvasX + CanvasW);
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
//-----------------------------------------------------------------------------
void JZEventFrame::GetVirtualEventSize(int& Width, int& Height)
{
  int TotalClockTics = Song->MaxQuarters * Song->TicksPerQuarter;
  Width = TotalClockTics / ClocksPerPixel + mLeftInfoWidth;
  Height = 127 * mTrackHeight + mTopInfoHeight;
}

//-----------------------------------------------------------------------------
// PlayPosition
//-----------------------------------------------------------------------------

//   Update the play position to the clock argument, and trigger a redraw so
// the play bar will be drawn.
void JZEventFrame::NewPlayPosition(int Clock)
{
  int scroll_clock = (FromClock + 5 * ToClock) / 6;

  if (!SnapSel->Active && ((Clock > scroll_clock) || (Clock < FromClock)) && (Clock >= 0))
  {
    // avoid permenent redraws when end of scroll range is reached
    if (Clock > FromClock && ToClock >= Song->MaxQuarters * Song->TicksPerQuarter)
      return;
//    int x = Clock2x(Clock);
//    mpEventWindow->SetScrollPosition(x - mLeftInfoWidth, CanvasY);
  }

  if (!SnapSel->Active)        // sets clipping
  {
    if (PlayClock != Clock)
    {
//      int oldplayclock=PlayClock;
//      PlayClock = Clock;
//      wxRect invalidateRect;
//      invalidateRect.x=Clock2x(oldplayclock)-1;
//      invalidateRect.y=CanvasY;
//      invalidateRect.width=3;
//      invalidateRect.height= 100000000;
//      //DrawPlayPosition();
//      mpEventWindow->Refresh(TRUE,&invalidateRect);

//      invalidateRect.x=Clock2x(PlayClock)-1;
//      mpEventWindow->Refresh(TRUE,&invalidateRect);
        //DrawPlayPosition();

//      mpEventWindow->Refresh();
    }
  }
  if (NextWin)
  {
    NextWin->NewPlayPosition(Clock);
  }
}

/** draw the "play position", by placing a vertical line where the "play clock" is */
void JZEventFrame::DrawPlayPosition(wxDC* dc)
{
  if (!SnapSel->Active && PlayClock >= FromClock && PlayClock < ToClock)
  {
//    wxDC* dc = new wxClientDC(this);
//    dc->SetLogicalFunction(wxXOR);
    dc->SetBrush(*wxBLACK_BRUSH);
    dc->SetPen(*wxBLACK_PEN);
    int x = Clock2x(PlayClock);

    //cout<<"JZEventFrame::DrawPlayPosition play pos x "<<x<<" "<<FromClock<<" "<<ToClock<<endl;
    //dc->DrawRectangle(x, CanvasY, 2*LittleBit, mTopInfoHeight);
    dc->DrawLine(x,     CanvasY, x,     mEventsY + mEventsHeight); //draw a line, 2 pixwels wide
    dc->DrawLine(x + 1, CanvasY, x + 1, mEventsY + mEventsHeight);
    dc->SetLogicalFunction(wxCOPY);
  }
//OLD  if (NextWin)
//OLD  {
//OLD    NextWin->DrawPlayPosition(dc);
//OLD  }
}

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
  if (NextWin)
          NextWin->Redraw();


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


void JZEventFrame::ZoomIn()
{

//  if (ClocksPerPixel >= 2)
//  {
//    ClocksPerPixel /= 2;
//    int x = CanvasX * 2;
//    int y = CanvasY;

//    wxDC* dc=new wxClientDC(mpEventWindow);
//    JZEventFrame::OnPaintSub(dc, x, y);
//    mpEventWindow->SetScrollRanges();
//    mpEventWindow->SetScrollPosition(x, y);
//    if (x == 0)
//      Redraw();

//  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZEventFrame::ZoomOut()
{
//  if (ClocksPerPixel <= 120)
//  {
//    ClocksPerPixel *= 2;
//    int x = CanvasX / 2;
//    int y = CanvasY;

    //wxClientDC Dc(mpEventWindow);
    //JZEventFrame::OnPaintSub(Dc, x, y);
//    mpEventWindow->SetScrollRanges();
//    mpEventWindow->SetScrollPosition(x, y);
    //if (x == 0)
    //  Redraw();
//  }
}
