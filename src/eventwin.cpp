/*
**  The JAZZ++ Midi Sequencer
**
** Copyright (C) 1994-2000 Andreas Voss and Per Sigmond, all rights reserved.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
*/                                                                              

#include "wx/wx.h"

#include "proplistdlg.h"

#include "eventwin.h"
#include "util.h"

#include "song.h"
#include "command.h"

#include "dialogs.h"


#include "jazz.h"

#include "toolbar.h"



// ************************************************************************
// tCanvas
// ************************************************************************

#ifdef wx_xt
#define ScLine 1L
#define ScPage 200L
#else
#define ScLine 50L
#define ScPage 8L
#endif


//notice, there is a conflict between this and the subclass event tables that i dont know how to resolv
BEGIN_EVENT_TABLE(tEventWin, wxFrame)
  EVT_SIZE    (           tEventWin::OnSize)
END_EVENT_TABLE()






tCanvas::tCanvas(tEventWin *frame, int x, int y, int w, int h, int style)
  : wxScrolledWindow(frame, -1, wxPoint(x, y), wxSize(w, h), style)
{
  EventWin = frame;
}

/**
JAVE seems to want to clip the paint area
calls the subclass paint routine

onpaint seems never to get called
*/

 void tCanvas::OnDraw(wxDC& dc)
{
  //onpaint never seems to get called, but ondraw does get called
  int x=0, y=0;
  GetViewStart(&x, &y);
  EventWin->OnPaintSub(&dc, (long)x * ScLine, (long)y * ScLine);  
  cout << "tCanvas::OnDraw\n";
}

//the canvas sends events to the subclassed window, i (might)filter the events a bit so as not get all mouse move events
BEGIN_EVENT_TABLE(tCanvas, wxScrolledWindow)
   EVT_MOUSE_EVENTS(tCanvas::OnMouseEvent)
//   EVT_LEFT_DOWN(tCanvas::OnMouseEvent)
//   EVT_LEFT_UP(tCanvas::OnMouseEvent)
//   EVT_RIGHT_DOWN(tCanvas::OnMouseEvent)
//   EVT_RIGHT_UP(tCanvas::OnMouseEvent)
END_EVENT_TABLE()

  /**
     this mouse handler delegates to the subclased eventwin
   */
void tCanvas::OnMouseEvent(wxMouseEvent &e)
{
  EventWin->OnMouseEvent(e);
}

/**
probably never called in wx2
*/
// void tCanvas::OnEvent(wxMouseEvent &e)
// {
//   EventWin->OnMouseEvent(e);
// }

//JAVE the OnChar method seems to be gone in wxwin232, but its documented, so i dont know whats happened
//the OnCharHook should do the same thing basically(it was there from the start. OnChar seemd redundant)

// void tCanvas::OnChar(wxKeyEvent &e)
// {
//   if (!EventWin->OnKeyEvent(e))
//     wxWindow::OnChar(e);
// }

// void tEventWin::OnChar(wxKeyEvent& e)
// {
//   if (!OnKeyEvent(e))
//     wxFrame::OnChar(e);
// }



Bool tCanvas::OnCharHook(wxKeyEvent& e) {
  return EventWin->OnKeyEvent(e);
}


Bool tEventWin::OnCharHook(wxKeyEvent& e)
{
  return OnKeyEvent(e);
}


void tCanvas::SetScrollRanges()
{
  long w, h;
  EventWin->GetVirtSize(&w, &h);
  SetScrollbars(ScLine, ScLine, w/ScLine, h/ScLine, ScPage, ScPage);
#ifdef wx_xt
  EnableScrolling(TRUE, TRUE);
#else
  EnableScrolling(FALSE, FALSE);
#endif
}

void tCanvas::SetScrollPosition(long x, long y)
{
  x /= ScLine;
  y /= ScLine;
  Scroll(x, y);
}


// ************************************************************************
// tEventWin
// ************************************************************************

tEventWin::tEventWin(wxFrame *parent, char *title, tSong *song, int x, int y, int width, int height )
  : wxFrame(parent, -1, title, wxPoint(x, y), wxSize(width, height)) // default is 640x442
{
  tool_bar  = 0;
#ifdef wx_msw
  grey_color = new wxColor(192, 192, 192);
#else
  grey_color = new wxColor(220, 220, 220);
#endif
  grey_brush = new wxBrush(*grey_color, wxSOLID);


  Song      = song;


  ParentWin = parent;
  NextWin   = 0;


  Filter    = new tFilter(Song);


  Canvas = 0;
  MouseAction = 0;
  SnapSel = 0;
  DialogBox = 0;
  MixerForm = 0;
  // dc = 0;

  hTop   = 40;
  wLeft  = 100;
  xEvents = wLeft;
  yEvents = hTop;
  wEvents = hEvents = 0;
  hLine = 0;
  LittleBit = 1;

  FontSize = 12;
  ClocksPerPixel = 36;
  UseColors = 1;

  PlayClock = -1;

  Font = 0;
  FixedFont = 0;

  hFixedFont = 0;
  CanvasX = CanvasY = CanvasW = CanvasH = 0;      // canvas coords
  FromClock = ToClock = 0;
  FromLine = ToLine = 0;

    m_textWindow = new wxTextCtrl(this, -1, "", wxPoint(0, 0), wxSize(-1, -1), wxTE_MULTILINE);

}


tEventWin::~tEventWin()
{
  delete Canvas;
  delete SnapSel;

#ifndef __PORTING
  delete Filter;
#endif // __PORTING

  delete tool_bar;

  if (MixerForm)
    delete MixerForm;
}



void tEventWin::CreateMenu()
{
#if 0
  wxMenu *menu = new wxMenu;
  menu->Append(999, "&MenuItem");
  wxMenuBar *menu_bar = new wxMenuBar;
  menu_bar->Append(menu, "&Debug");
  SetMenuBar(menu_bar);
#endif
}


/**

create the canvas component(used for differently dependingon the subclass)
size it to the client area of the frame(frame size minus toolbar and menus )
*/
void tEventWin::CreateCanvas()
{
  cout << "createcanvas\n"; 
 int w, h;
  GetClientSize(&w, &h);
  Canvas = new tCanvas(this, 0, 0, w, h);
  //dc = Canvas->GetDC();
  //dc = new wxClientDC(Canvas);
}

/**
second phase of creation. make menus, the canvas, and so on
*/
void tEventWin::Create()
{
  cout <<"tEventWin::Create\n";
  CreateMenu();

  CreateCanvas();
  SnapSel = new tSnapSelection(Canvas);


  Setup();
  Canvas->SetScrollRanges();
  Canvas->SetScrollPosition(0,0);//this wasnt here before wx2, why?
}


/**initialize the constants used in drawing*/
void tEventWin::Setup()
{
  //float x, y;
  long x,y;

  wxDC* dc=new wxClientDC(Canvas);
  //dc is from Canvas
  dc->SetFont(wxNullFont);
  delete FixedFont;
  FixedFont = new wxFont(12, wxSWISS, wxNORMAL, wxNORMAL);
  dc->SetFont(*FixedFont);
  dc->GetTextExtent("M", &x, &y);
  hFixedFont = (int)y;

  delete Font;
  Font = new wxFont(FontSize, wxSWISS, wxNORMAL, wxNORMAL);
  dc->SetFont(*Font);

  dc->GetTextExtent("M", &x, &y);
  LittleBit = (int)(x/2);

  dc->GetTextExtent("HXWjgi", &x, &y);
  hLine = (int)y + LittleBit;
  delete dc;
}


/**
this onsize handler is supposed to take care of handling of the resizing the two subwindows sizes to 
they dont overlap
*/
 void tEventWin::OnSize(wxSizeEvent& event)//int w, int h)
 {
   //   wxFrame::OnSize(event);



   //the below code is from the toolbar sample, the layoutchidlren function
    wxSize size = GetClientSize();

    int offset;
//     if ( tool_bar )
//     {
//         tool_bar->SetSize(-1, size.y);
//         tool_bar->Move(0, 0);

//         offset = tool_bar->GetSize().x;
//     }
//     else
//     {
//         offset = 0;
//     }

    //the step below should set the offset of the Canvas
    //m_textWindow->SetSize(offset, 0, size.x - offset, size.y);

    

//     float maxToolBarWidth  = 0.0;
//     float maxToolBarHeight = 0.0;
//     if (tool_bar)
//       tool_bar->GetMaxSize(&maxToolBarWidth, &maxToolBarHeight);

    offset = tool_bar->GetSize().y; //get the height of the toolbar

    int frameWidth, frameHeight;
    GetClientSize(&frameWidth, &frameHeight);
    
     if (Canvas)
       //       Canvas->SetSize(0, (int)offset, (int)frameWidth, (int)(frameHeight - offset));
       Canvas->SetSize(0, (int)0, (int)frameWidth, (int)(frameHeight));
 //   if (tool_bar)
 //     tool_bar->SetSize(0, 0, (int)frameWidth, (int)maxToolBarHeight);

    cout<<"tEventWin::OnSize "<<frameWidth<<" "<<frameHeight<<"\n";


 }

// *******************************************************************
// Coord-Functions
// *******************************************************************


long tEventWin::x2Clock(long x)
{
  return (x - xEvents) * ClocksPerPixel + FromClock;
}


long tEventWin::Clock2x(long clk)
{
  return xEvents + (clk - FromClock) / ClocksPerPixel;
}




long tEventWin::x2BarClock(long x, int next)
{
  long clk = x2Clock(x);
  tBarInfo b(Song);
  b.SetClock(clk);
  while (next--)
    b.Next();
  return b.Clock;
}




long tEventWin::y2yLine(long y, int up)
{
  if (up)
    y += hLine;
  y -= hTop;
  y -= y % hLine;
  y += hTop;
  return y;
}

long tEventWin::y2Line(long y, int up)
{
  if (up)
    y += hLine;
  y -= hTop;
  return y / hLine;
}


long tEventWin::Line2y(long Line)
{
  return Line * hLine + hTop;
}

void tEventWin::LineText(wxDC *dc, long x, long y, long w, const char *str, int h, Bool down)
{
  if (h <= 0)
  {
    h = hLine;
    y = y2yLine(y);
  }
  if (w && h)
  {
    //dc->SetBrush(wxGREY_BRUSH);
    dc->SetBrush(*grey_brush);
    dc->SetPen(*wxGREY_PEN);
    #ifdef wx_msw
    dc->DrawRectangle(x, y, w+1, h+1);
    #else
    dc->DrawRectangle(x, y, w, h);
    #endif
    x += 1;
    y += 1;
    w -= 2;
    h -= 2;
    if (down) {
      dc->SetPen(*wxBLACK_PEN);
      dc->DrawLine(x, y, x+w, y);
      dc->DrawLine(x, y, x, y+h);
      dc->SetPen(*wxWHITE_PEN);
      dc->DrawLine(x+w, y, x+w, y+h);
      dc->DrawLine(x, y+h, x+w, y+h);
    }
    else {
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
  wxColor &bg = dc->GetTextBackground();
  dc->SetTextBackground(*grey_color);
  dc->DrawText((char *)str, x + LittleBit, y + LittleBit);
  dc->SetTextBackground(*wxWHITE);
}


// *******************************************************************
// Painting behavior
// *******************************************************************

void tEventWin::Redraw()
{
//   wxDC* dc=new wxClientDC(this);
//   wxPaintEvent e;
//   cout<<"FIXME tEventWin::Redraw"<<endl;
//   Canvas->OnDraw(*dc); //this will in turn call the eventwin onpaintsub
//   //the problem is that onpaint no longer tkes no argument, and is supposed to be called from the framework only, so it should be split
//   delete dc;
  Canvas->Refresh();

}

/**
   JAVE this was originally called OnPaint(x,y), but i renamed it because i confused it with the OnPaint() framework routine
   the call graph feels odd: the canvas is a member of the eventwin, with a pointer to the parent. the child calss the parent to redraw itself

   it doesnt do any real drawing, instead it sets up some member vars, to be used by other parts of the class

   it is now normally called from OnDraw in the Canvas class,and also overridden in the subclass.
   so this one here just sets up  constants


   dc is the device context to draw in, normally generated from the framework from ondraw
   x and y is the coordinates of the start of the view
   
*/
void tEventWin::OnPaintSub(wxDC *dc, long x, long y)
{
  //printf("EventWin::OnPaintSub: x %ld, y %ld, w %ld, h %ld\n", x, y, w, h);
  CanvasX = x;
  CanvasY = y;
// wxCanvas::GetClientSize returns huge values, at least in wx_xt
  int xc, yc;
  GetClientSize(&xc, &yc);
  CanvasW = xc;
  CanvasH = yc;

  xEvents = CanvasX + wLeft;
  yEvents = CanvasY + hTop;
  wEvents = CanvasW - wLeft;
  hEvents = CanvasH - hTop;
  //printf("EventWin::OnPaint: xe %ld, ye %ld, we %ld, he %ld\n", xEvents, yEvents, wEvents, hEvents);

  FromLine = CanvasY / hLine; 
  ToLine   = (CanvasY + CanvasH - hTop) / hLine;
  FromClock = CanvasX * ClocksPerPixel;
  ToClock = x2Clock(CanvasX + CanvasW);
}

// ******************************************************************
// Mouse
// ******************************************************************

int tEventWin::OnKeyEvent(wxKeyEvent &e)
{
  return 0;
}

/** seems to handle the "selection" rectangle. normally called from the base class onmouseevent handler */
int tEventWin::OnMouseEvent(wxMouseEvent &e)
{
  //  cout <<"tEventWin::OnMouseEvent"<<endl;
  if (!MouseAction)
  {
    // create SnapSel?

    long x;
    long y;
    e.GetPosition(&x, &y);
    if (xEvents < x && x < xEvents + wEvents && yEvents < y && y < yEvents + hEvents)
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

bool tEventWin::OnClose()
{
  return FALSE;
}

void tEventWin::OnMenuCommand(int) {}
void tEventWin::SnapSelStart(wxMouseEvent &e){}
void tEventWin::SnapSelStop(wxMouseEvent &e) {}
void tEventWin::GetVirtSize(long *w, long *h)
{


  long clk = Song->MaxQuarters * Song->TicksPerQuarter;
  *w = clk / ClocksPerPixel + wLeft;
  *h = 127 * hLine + hTop;


  *w = 5000L;
}

// ------------------------------------------------------------------------------------
// PlayPosition
// -----------------------------------------------------------------------------------

/** update the play position to the clock argument, and trigger a redraw so the play bar will be drawn*/
void tEventWin::NewPlayPosition(long Clock)
{


  long scroll_clock = (FromClock + 5 * ToClock) / 6L;

  if (!SnapSel->Active && ((Clock > scroll_clock) || (Clock < FromClock)) && (Clock >= 0L) )
  {
    // avoid permenent redraws when end of scroll range is reached
    if (Clock > FromClock && ToClock >= Song->MaxQuarters * Song->TicksPerQuarter)
      return;
    long x = Clock2x(Clock);
    Canvas->SetScrollPosition(x - wLeft, CanvasY);
  }

  if (!SnapSel->Active)	// sets clipping
  {
    if (PlayClock != Clock) {
      long oldplayclock=PlayClock;
      PlayClock = Clock;
//        wxRect invalidateRect;
//        invalidateRect.x=Clock2x(oldplayclock)-1;
//        invalidateRect.y=CanvasY;
//        invalidateRect.width=3;
//        invalidateRect.height= 100000000;
//       //       DrawPlayPosition();
//        Canvas->Refresh(TRUE,&invalidateRect);

//              invalidateRect.x=Clock2x(PlayClock)-1;
//       Canvas->Refresh(TRUE,&invalidateRect);
      //       DrawPlayPosition();

      Canvas->Refresh();


    }
  }
  if (NextWin)
    NextWin->NewPlayPosition(Clock);


}

/** draw the "play position", by placing a vertical line where the "play clock" is */
void tEventWin::DrawPlayPosition(wxDC* dc)
{
   if (!SnapSel->Active && PlayClock >= FromClock && PlayClock < ToClock)
   {
    //    wxDC* dc=new wxClientDC(this);
  //    dc->SetLogicalFunction(wxXOR);
    dc->SetBrush(*wxBLACK_BRUSH);
    dc->SetPen(*wxBLACK_PEN);
    long x = Clock2x(PlayClock);
    //cout<<"tEventWin::DrawPlayPosition play pos x "<<x<<" "<<FromClock<<" "<<ToClock<<endl;
    //dc->DrawRectangle(x, CanvasY, 2*LittleBit, hTop);
    dc->DrawLine(x,  CanvasY,x,  yEvents+hEvents); //draw a line, 2 pixwels wide
    dc->DrawLine(x+1,CanvasY,x+1,yEvents+hEvents);
    dc->SetLogicalFunction(wxCOPY);
      }
  if (NextWin)
    NextWin->DrawPlayPosition(dc);
}

// **************************************************************************
// EventsSelected
// **************************************************************************

int tEventWin::EventsSelected(const char *msg)
{
  if (!SnapSel->Selected)
  {
    if (msg == 0)
      msg = "please select some events first";
    wxMessageBox((char *)msg, "Error", wxOK);
    return 0;
  }
  return 1;
}

// **************************************************************************
// Quantize
// **************************************************************************

void tEventWin::MenQuantize()
{


  if (!EventsSelected())
    return;
  //  wxDialogBox *panel = new wxDialogBox(this, "Quantize", FALSE );
  tQuantizeDlg * dlg = new tQuantizeDlg(this, Filter);
  dlg->Create();


}

// **************************************************************************
// Cleanup
// **************************************************************************

void tEventWin::MenCleanup()
{


  if (!EventsSelected())
    return;
  tCleanupDlg * dlg = new tCleanupDlg(this, Filter);
  dlg->Create();

}


// **************************************************************************
// SearchReplace
// **************************************************************************

void tEventWin::MenSearchReplace()
{


  if (!EventsSelected())
    return;
  tSearchReplaceDlg * dlg = new tSearchReplaceDlg(this, Filter);
  dlg->Create();


}


// **************************************************************************
// SetChannel
// **************************************************************************

void tEventWin::MenSetChannel()
{


  if (!EventsSelected())
    return;
  tSetChannelDlg * dlg = new tSetChannelDlg(Filter);
  dlg->Create();

}


// **************************************************************************
// Transpose
// **************************************************************************

void tEventWin::MenTranspose()
{


  if (!EventsSelected())
    return;
  tTransposeDlg * dlg = new tTransposeDlg(this, Filter);
  dlg->Create();


}

/**show the "shift events" dialog */

void tEventWin::MenShift(long Unit)
{
  if (EventsSelected())
  {

    tShiftDlg * dlg = new tShiftDlg(this, Filter, Unit);
    dlg->Create();
  }


}


// ********************************************************************************
// Delete
// ********************************************************************************


void tEventWin::MenDelete()
{


  if (!EventsSelected())
    return;
  tDeleteDlg * dlg = new tDeleteDlg(this, Filter);
  dlg->Create();
}

// ********************************************************************************
// Velocity
// ********************************************************************************


void tEventWin::MenVelocity()
{

  if (!EventsSelected())
    return;
  tVelocityDlg * dlg = new tVelocityDlg(Filter);
  dlg->Create();
}

// ********************************************************************************
// Length
// ********************************************************************************


void tEventWin::MenLength()
{


  if (!EventsSelected())
    return;
  tLengthDlg * dlg = new tLengthDlg(this, Filter);
  dlg->Create();

}

// ********************************************************************************
// Sequence Length
// ********************************************************************************


void tEventWin::MenSeqLength()
{

  if (!EventsSelected())
    return;
  tSeqLengthDlg* dlg = new tSeqLengthDlg(this, Filter);
  dlg->Create();


}

// ********************************************************************************
// Midi delay
// ********************************************************************************


void tEventWin::MenMidiDelay()
{

  if (!EventsSelected())
    return;
  tMidiDelayDlg* dlg = new tMidiDelayDlg(this, Filter);
  dlg->Create();
}


// ********************************************************************************
// convert to modulation
// ********************************************************************************


void tEventWin::MenConvertToModulation()
{


  if (!EventsSelected())
    return;
  tCmdConvertToModulation cmd(Filter);
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
  tEventWin *EventWin;
  static int Numerator;
  static int Denomiator;
  static int BarNr;
  tMeterChangeDlg(tEventWin *w);
  void AddProperties();
  virtual bool OnClose();
  virtual void OnCancel();
  virtual void OnHelp();
};


int tMeterChangeDlg::Numerator = 4;
int tMeterChangeDlg::Denomiator = 4;
int tMeterChangeDlg::BarNr = 1;

tMeterChangeDlg::tMeterChangeDlg(tEventWin *w)
  : tPropertyListDlg("Meter Change")
{
  EventWin = w;
}


void tMeterChangeDlg::OnCancel()
{
  EventWin->DialogBox = 0;
  //wxForm::OnCancel();
}


bool tMeterChangeDlg::OnClose()
{
  BarNr += EventWin->Song->GetIntroLength();
  EventWin->Song->SetMeterChange(BarNr, Numerator, Denomiator);
  EventWin->Redraw();
  EventWin->DialogBox = 0;
  //wxForm::OnOk();
  return false;
}

void tMeterChangeDlg::OnHelp()
{
        HelpInstance->ShowTopic("Meterchange");
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
  sheet->AddProperty(new wxProperty("BarNr",wxPropertyValue((long*) &BarNr), "integer"));//JAVE validators here? problem is i dont know which ranges are valid FIXME
  sheet->AddProperty(new wxProperty("Numerator",wxPropertyValue((long*) &Numerator), "integer"));
  sheet->AddProperty(new wxProperty("Denomiator(2,4,8,16,32)",wxPropertyValue((long*) &Denomiator), "integer"));//JAVE should be a integer list instead FIXME


}



void tEventWin::MenMeterChange()
{
  tMeterChangeDlg *dlg;
  if (DialogBox)
  {
    DialogBox->Show(TRUE);
    return;
  }
  //  DialogBox = new wxDialogBox(this, "MeterChange", FALSE );
  dlg = new tMeterChangeDlg(this);
  dlg->Create();

}





void tEventWin::ZoomIn()
{

  if (ClocksPerPixel >= 2) {
    ClocksPerPixel /= 2;
    long x = CanvasX * 2;
    long y = CanvasY;

    wxDC* dc=new wxClientDC(Canvas);
    tEventWin::OnPaintSub(dc, x, y);
    Canvas->SetScrollRanges();
    Canvas->SetScrollPosition(x, y);
    if (x == 0)
      Redraw();

  }
}

void tEventWin::ZoomOut()
{
  if (ClocksPerPixel <= 120) {
    ClocksPerPixel *= 2;
    long x = CanvasX / 2;
    long y = CanvasY;

    wxDC* dc=new wxClientDC(Canvas);
    tEventWin::OnPaintSub(dc, x, y);
    Canvas->SetScrollRanges();
    Canvas->SetScrollPosition(x, y);
    if (x == 0)
      Redraw();
  }
}


