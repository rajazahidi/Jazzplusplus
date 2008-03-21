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

#include "Resources.h"

#include "PianoWindow.h"
#include "PianoFrame.h"
#include "ControlEdit.h"
#include "Song.h"
#include "Filter.h"
#include "HarmonyP.h"
#include "HarmonyBrowserAnalyzer.h"
#include "Harmony.h"
#include "Player.h"
#include "Synth.h"
#include "Command.h"
#include "GuitarFrame.h"
#include "Dialogs.h"
#include "SelectControllerDialog.h"
#include "ResourceDialog.h"
#include "Help.h"

#include <sstream>

using namespace std;





//*****************************************************************************
//*****************************************************************************

JZListen::JZListen()
  : mActive(false),
    mPitch(-1),
    mChannel(-1),
    mpTrack(0)
{
}

void JZListen::KeyOn(
  JZTrack* pTrack,
  int Pitch,
  int Channel,
  int Velocity,
  int MilliSeconds)
{
  if (!mActive)
  {
    mPitch = Pitch;
    mChannel = Channel;
    tKeyOn KeyOn(0, mChannel, Pitch, Velocity);
    gpMidiPlayer->OutNow(pTrack, &KeyOn);
    mActive = true;
    Start(MilliSeconds);
    mpTrack = pTrack;
  }
}

void JZListen::Notify()
{
  Stop();
  tKeyOff KeyOff(0, mChannel, mPitch);
  gpMidiPlayer->OutNow(mpTrack, &KeyOff);
  mActive = false;
}

//*****************************************************************************
// Description:
//   tMousePlay - Click in pianoroll
//*****************************************************************************
class tMousePlay : public tMouseAction
{
  public:

    tMousePlay(JZPianoWindow* pPianoWindow, wxMouseEvent& Event);
    int ProcessEvent(wxMouseEvent& Event);

  private:

    int mPitch, mVeloc, mChannel;
    JZPianoWindow* mpPianoWindow;
};

tMousePlay::tMousePlay(JZPianoWindow* pPianoWindow, wxMouseEvent& Event)
  : mPitch(0),
    mVeloc(-1),
    mChannel(-1),
    mpPianoWindow(pPianoWindow)
{
  mChannel =
    mpPianoWindow->GetTrack()->Channel ? mpPianoWindow->GetTrack()->Channel - 1 : 0;

  ProcessEvent(Event);
}


int tMousePlay::ProcessEvent(wxMouseEvent& Event)
{
  int x, y;

  int OldPitch = mPitch;
  mpPianoWindow->LogicalMousePosition(Event, x, y);

  if (Event.LeftDown())
  {
    mPitch = mpPianoWindow->y2Pitch(y);
    mVeloc = 64;
  }
  else if (Event.MiddleDown())
  {
    mPitch = mpPianoWindow->y2Pitch(y);
    mVeloc = 80;
  }
  else if (Event.RightDown())
  {
    mPitch = mpPianoWindow->y2Pitch(y);
    mVeloc = 110;
  }
  else if (Event.ButtonUp())
  {
    mPitch = 0;
  }
  else if (Event.Dragging())
  {
    mPitch = mpPianoWindow->y2Pitch(y);
  }
  else
  {
    return 0;
  }

  if (mpPianoWindow->GetTrack()->GetAudioMode())
  {
    if (mPitch && mPitch != OldPitch)
    {
      gpMidiPlayer->ListenAudio(mPitch, 0);
    }
  }
  else
  {
    if (OldPitch && OldPitch != mPitch)
    {
      tKeyOff of(0, mChannel, OldPitch);
      gpMidiPlayer->OutNow(mpPianoWindow->GetTrack(), &of);
      OldPitch = 0;
    }

    if (mPitch && mPitch != OldPitch)
    {
      tKeyOn on(0, mChannel, mPitch, mVeloc);
      gpMidiPlayer->OutNow(mpPianoWindow->GetTrack(), &on);
      OldPitch = 0;
    }
  }

  if (!mPitch)
  {
    mpPianoWindow->mpMouseAction = 0;
    delete this;
    return 1;        // done
  }
  return 0;
}







//*****************************************************************************
// Description:
//   tKeyLengthDragger
//*****************************************************************************
class tKeyLengthDragger : public tMouseAction
{
  public:

    tKeyLengthDragger(tKeyOn *k, JZPianoWindow *w);

    int Dragging(wxMouseEvent& Event);

    int ButtonUp(wxMouseEvent& Event);

    int Event(wxMouseEvent& Event);

  private:

    tKeyOn* mpKeyOn;
    tKeyOn    *Copy;
    JZPianoWindow* Win;
    JZTrack* mpTrack;
};


tKeyLengthDragger::tKeyLengthDragger(tKeyOn *k, JZPianoWindow *w)
{
  mpKeyOn = k;
  Copy  = k->Copy() -> IsKeyOn();
  Win   = w;

  // SN++ BUG FIX: undo/redo
  Win->mpSong->NewUndoBuffer();

  wxClientDC Dc(Win);

  // to translate scrolled coordinates
  Win->PrepareDC(Dc);

  Win->DrawEvent(Dc, Copy, wxWHITE_BRUSH, 0);
  Win->DrawEvent(Dc, Copy, Copy->GetBrush(), 1, 1);
}

int tKeyLengthDragger::Event(wxMouseEvent& Event)
{
  if (Event.Dragging())
  {
    return Dragging(Event);
  }
  else if (Event.ButtonUp())
  {
    return ButtonUp(Event);
  }
  return 0;
}

int tKeyLengthDragger::Dragging(wxMouseEvent& Event)
{
  wxClientDC Dc(Win);
  Win->PrepareDC(Dc); //to translate scrolled coordinates
  Win->DrawEvent(Dc, Copy, Copy->GetBrush(), 1, 1);
  int fx, fy;
  Win->LogicalMousePosition(Event, fx, fy);
  int Clock = Win->x2Clock(fx);
  int  Length = Clock - Copy->GetClock();
  if (Length <= 0)
  {
    Length = 1;
  }
  Copy->Length = Length;

  Win->DrawEvent(Dc, Copy, Copy->GetBrush(), 1, 1);
  return 0;
}

int tKeyLengthDragger::ButtonUp(wxMouseEvent& Event)
{
  // SN++ Key_Aftertouch
  if (Copy->Length < mpKeyOn->Length)
  {
    int key, channel;
    tEventIterator iter(Win->GetTrack());
    tKeyPressure *a;
    key = Copy->Key;
    channel = Copy->Channel;
    JZEvent* pEvent = iter.Range(
      Copy->GetClock() + Copy->Length,
      Copy->GetClock() + mpKeyOn->Length);
    while (pEvent)
    {
      a = pEvent->IsKeyPressure();
      if (a)
      {
        if (a->Key == key && a->Channel == channel)
        {
          Win->KillTrackEvent(pEvent);
        }
      }
      pEvent = iter.Next();
    }
  }
  //

  Win->ApplyToTrack(mpKeyOn, Copy);

  Win->mpMouseAction = 0;

  // SN++ Veloc- oder Aftertouch-Editor updaten
  Win->UpdateControl();

  // allways repaint
  Win->Refresh();

  delete this;
  return 0;
}


//*****************************************************************************
// Description:
//   tPlayTrackLengthDragger JAVE this is just copied from tKeyLengthDragger,
// the need to be inherited somehow
//*****************************************************************************
class tPlayTrackLengthDragger : public tMouseAction
{
    tPlayTrack* mpKeyOn;
    tPlayTrack    *Copy;
    JZPianoWindow *Win;
    JZTrack* mpTrack;

  public:
    tPlayTrackLengthDragger(tPlayTrack *k, JZPianoWindow *w);
    int Dragging(wxMouseEvent& Event);
    int ButtonUp(wxMouseEvent& Event);
    int Event(wxMouseEvent& Event);
};


tPlayTrackLengthDragger::tPlayTrackLengthDragger(tPlayTrack *k, JZPianoWindow *w)
{
  mpKeyOn = k;
  Copy  = k->Copy() -> IsPlayTrack();
  Win   = w;

  // SN++ BUG FIX: undo/redo
  Win->mpSong->NewUndoBuffer();
  //
  wxClientDC Dc(Win);
  Win->PrepareDC(Dc);
  Win->DrawEvent(Dc, Copy, wxWHITE_BRUSH, 0);
  Win->DrawEvent(Dc, Copy, Copy->GetBrush(), 1, 1);
}

int tPlayTrackLengthDragger::Event(wxMouseEvent& Event)
{
  if (Event.Dragging())
  {
    return Dragging(Event);
  }
  else if (Event.ButtonUp())
  {
    return ButtonUp(Event);
  }
  return 0;
}

int tPlayTrackLengthDragger::Dragging(wxMouseEvent& Event)
{
  wxClientDC Dc(Win);
  Win->PrepareDC(Dc);
  Win->DrawEvent(Dc, Copy, Copy->GetBrush(), 1, 1);
  int fx, fy;
  Win->LogicalMousePosition(Event, fx, fy);
  int Clock = Win->x2Clock(fx);
  int  Length = Clock - Copy->GetClock();
  if (Length <= 0)
    Length = 1;
  Copy->eventlength = Length; 

  Win->DrawEvent(Dc, Copy, Copy->GetBrush(), 1, 1);
  return 0;
}

int tPlayTrackLengthDragger::ButtonUp(wxMouseEvent& Event)
{
  wxClientDC Dc(Win);
  Win->PrepareDC(Dc);
  Win->DrawEvent(Dc, Copy, Copy->GetBrush(), 1, 1);
  Win->DrawEvent(Dc, Copy, Copy->GetBrush(), 0, 1);

  Win->ApplyToTrack(mpKeyOn, Copy);

  Win->mpMouseAction = 0;

  // SN++ Veloc- oder Aftertouch-Editor updaten
  Win->UpdateControl();

  // allways repaint
  Win->Refresh();

  delete this;
  return 0;
}



// --------------------------------------------------------------------
// VelocCounter
// --------------------------------------------------------------------

class tVelocCounter : public tMouseCounter
{
  public:
    int Event(wxMouseEvent& Event);
    tVelocCounter(JZPianoWindow *w, JZRectangle* r, tKeyOn* pEvent)
      : tMouseCounter(w, r, pEvent->Veloc, 1, 127)
    {
      Win = w;
      mpKeyOn = pEvent;

      // SN++ BUG FIX: undo/redo
      Win->mpSong->NewUndoBuffer();
      //
      wxClientDC Dc(Win);
      Dc.SetFont(*(Win->GetFixedFont()));
    }

  private:

    JZPianoWindow* Win;

    tKeyOn* mpKeyOn;
};



int tVelocCounter::Event(wxMouseEvent& Event)
{
  if (tMouseCounter::Event(Event))
  {
    tKeyOn *Copy = (tKeyOn *)mpKeyOn->Copy();
    Copy->Veloc = Value;

    Win->ApplyToTrack(mpKeyOn, Copy);

    wxClientDC Dc(Win);
    Win->PrepareDC(Dc);
    Win->DrawEvent(Dc, Copy, Copy->GetBrush(), 0, 1);

    Win->UpdateControl();

    Win->mpMouseAction = 0;

    Dc.SetFont(*(Win->GetFont()));
    delete this;
  }
  return 0;
}











static int mPianoFontSizes[] =
{
  6,  // Tiny
  7,  // Small
  8,  // Medium
  10, // Large
  12, // Huge
  -1, // End of list
};

const int isBlack[12] =
{
  0,
  1,
  0,
  1,
  0,
  0,
  1,
  0,
  1,
  0,
  1,
  0
};

#define IsBlack(Key)  isBlack[(Key) % 12]

// Mouse Actions Mapping
enum
{
  MA_PLAY = 1, // 0 represents no action.
  MA_CYCLE,
  MA_SELECT,
  MA_CONTSEL,
  MA_CUTPASTE,
  MA_LENGTH,
  MA_DIALOG,
  MA_LISTEN,
  MA_COPY,
  MA_VELOCITY
};

const int play_actions[12] =
{
  // left        middle           right
  MA_PLAY,       MA_CYCLE,        0,            // plain
  MA_CYCLE,      0,               0,            // shift
  0,             0,               0,            // ctrl
  0,             0,               0             // shift+ctrl
};

const int evnt_actions[12] =
{
  // left        middle          right
  MA_SELECT,     MA_CUTPASTE,    MA_LENGTH,      // plain
  MA_CONTSEL,    MA_COPY,        MA_LISTEN,      // shift
  MA_VELOCITY,   MA_DIALOG,      MA_VELOCITY,    // ctrl
  MA_CUTPASTE,   0,              MA_COPY         // shift+ctrl
};


//*****************************************************************************
// Description:
//   This is the piano window definition.
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(JZPianoWindow, wxScrolledWindow)

  EVT_SIZE(JZPianoWindow::OnSize) 

  EVT_MOUSE_EVENTS(JZPianoWindow::OnMouseEvent)

END_EVENT_TABLE()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
const int JZPianoWindow::mScrollSize = 50;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZListen JZPianoWindow::mListen;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZPianoWindow::JZPianoWindow(
  JZPianoFrame* pPianoFrame,
  JZSong* pSong,
  const wxPoint& Position,
  const wxSize& Size,
  long WindowStyle)
  : wxScrolledWindow(pPianoFrame, wxID_ANY, Position, Size, WindowStyle),
    mpPianoFrame(pPianoFrame),
    mPlayClock(-1),
    mSnapCount(0),
    mpSong(pSong),
    mpTrack(0),
    mTrackIndex(0),
    mpFilter(0),
    mpCtrlEdit(0),
    mMousePlay(play_actions),
    mMouseEvent(evnt_actions),
    mLittleBit(1),
    mClockTicsPerPixel(4),
    mTopInfoHeight(40),
    mLeftInfoWidth(100),
    mFromClock(0),
    mToClock(0),
    mFromLine(0),
    mToLine(0),
    mCanvasX(0),
    mCanvasY(0),
    mCanvasWidth(0),
    mCanvasHeight(0),
    mTrackHeight(0),
    mUseColors(true),
    mMouseLine(-1),
    mFontSize(12),
    mpFont(0),
    mpFixedFont(0),
    mFixedFontHeight(0),
    mpDrumFont(0),
    mpSnapSel(0),
    mSnapDenomiator(16),
    mpMouseAction(0),
    mVisibleKeyOn(true),
    mVisiblePitch(false),
    mVisibleController(false),
    mVisibleProgram(false),
    mVisibleTempo(false),
    mVisibleSysex(false),
    mVisiblePlayTrack(false),
    mVisibleDrumNames(true),
    mVisibleAllTracks(false),
    mVisibleHBChord(true),
    mVisibleMono(false),
    mpGuitarFrame(0)
{
  InitColors();

  mpFilter = new tFilter(mpSong);

  mpTrack = mpSong->GetTrack(mTrackIndex);

  mFontSize = mPianoFontSizes[1]; // Must be an entry in the array.

  mpSnapSel = new tSnapSelection(this);

  for (int i = 0; i < eMaxTrackCount; i++)
  {
    mFromLines[i] = 64;
  }

  mMouseEvent.SetLeftAction(MA_SELECT);

  Setup();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZPianoWindow::~JZPianoWindow()
{
  delete mpCtrlEdit;
  delete mpFont;
  delete mpFixedFont;
  delete mpDrumFont;
  delete mpSnapSel;
  delete mpGuitarFrame;
}

//-----------------------------------------------------------------------------
// Description:
//   Generate some colors to represent note velocity.  The current settings use
// dark blue for quiet and bright red for loud.
//-----------------------------------------------------------------------------
void JZPianoWindow::InitColors()
{
  int c;
  for (int i = 0; i < NUM_COLORS; ++i)
  {
    c = 256 * i / NUM_COLORS; 
    mpColorBrush[i].SetColour(c, 0, 127 - c / 2);
    mpColorBrush[i].SetStyle(wxSOLID);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZPianoWindow::Setup()
{
  // This section is from JZEventFrame::Setup()

  int Width, Height;

  wxClientDC Dc(this);

  Dc.SetFont(wxNullFont);

  delete mpFixedFont;
  mpFixedFont = new wxFont(12, wxSWISS, wxNORMAL, wxNORMAL);

  Dc.SetFont(*mpFixedFont);
  Dc.GetTextExtent("M", &Width, &Height);
  mFixedFontHeight = Height;
  mTopInfoHeight = mFixedFontHeight + 2 * mLittleBit;

  delete mpFont;
  mpFont = new wxFont(mFontSize, wxSWISS, wxNORMAL, wxNORMAL);
  Dc.SetFont(*mpFont);

  Dc.GetTextExtent("M", &Width, &Height);
  mLittleBit = Width / 2;

  Dc.GetTextExtent("HXWjgi", &Width, &Height);
  mTrackHeight = Height + mLittleBit;

  delete mpDrumFont;
  mpDrumFont = new wxFont(mFontSize + 3, wxSWISS, wxNORMAL, wxNORMAL);

  Dc.SetFont(*mpDrumFont);

  Dc.GetTextExtent("Low Conga mid 2 or so", &Width, &Height);
  mPianoWidth = Width + mLittleBit;

  mLeftInfoWidth = mPianoWidth;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZPianoWindow::OnDraw(wxDC& Dc)
{
  // OnPaint never seems to get called, but OnDraw does get called.
  int x = 0, y = 0;
  GetViewStart(&x, &y);
  OnPaintSub(Dc, x * mScrollSize, y * mScrollSize);  
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZPianoWindow::OnPaintSub(wxDC& Dc, int x, int y)
{
//  int OldFromClock = mFromClock;

  OnEventWinPaintSub(x, y);

// SN++ Da Jazz nun eine ReDo Funktion hat. Behebt gleichzeitig ein kleines
//                Update Problem beim mehrfachen ZoomOut.
//                Aktives Ctrl-Fenster neu zeichnen bzw. reinitialisieren.

//  if (mpCtrlEdit && OldFromClock != mFromClock)
//    mpCtrlEdit->ReInit(mpTrack, mFromClock, mClockTicsPerPixel);

  if (mpCtrlEdit)
  {
    mpCtrlEdit->ReInit(mpTrack, mFromClock, mClockTicsPerPixel);
  }

  mPianoX = mCanvasX;

  int StopClk;
  JZBarInfo BarInfo(mpSong);
  char buf[20];

  Dc.DestroyClippingRegion();
  Dc.SetBackground(*wxWHITE_BRUSH);
  DrawPlayPosition(Dc);
  mpSnapSel->Draw(Dc, mEventsX, mEventsY, mEventsWidth, mEventsHeight);
  Dc.Clear();


  ///////////////////////////////////////////////////////////////
  // horizontal lines(ripped from drawpianoroll code)

//     for (y = Line2y(mFromLine); y < mEventsY + mEventsHeight; y += mTrackHeight)
//      if (y > mEventsY)        // cheaper than clipping
//        Dc.DrawLine(mEventsX+1, y, mEventsX + mEventsWidth, y);

  Dc.SetPen(*wxGREY_PEN);
  wxBrush blackKeysBrush=wxBrush(wxColor(250,240,240),wxSOLID);
  int Pitch = 127 - mFromLine;
  y = Line2y(mFromLine);
  while (Pitch >= 0 && y < mEventsY + mEventsHeight)
  {
    if (IsBlack(Pitch))
    {
      Dc.SetBrush(blackKeysBrush);//*wxLIGHT_GREY_PEN
      Dc.DrawRectangle(mCanvasX, y, 2000, mTrackHeight);
    }
    else if ((Pitch % 12) == 0)
    {
      Dc.SetPen(*wxCYAN_PEN);
      Dc.DrawLine(mCanvasX, y + mTrackHeight, 2000, y + mTrackHeight);
    }
    else if (!IsBlack(Pitch - 1))
    {
      Dc.SetPen(*wxGREEN_PEN);
      Dc.DrawLine(mCanvasX, y + mTrackHeight, 2000, y + mTrackHeight);
    }

    y += mTrackHeight;
    --Pitch;
  }


  ///////////////////////////////////////////////////////////////


  mMouseLine = -1;

  #define VLine(x) DrawLine(x, mCanvasY, x, mEventsY + mEventsHeight)
  #define HLine(y) DrawLine(mCanvasX, y, mCanvasX + mCanvasWidth, y)

  Dc.SetPen(*wxBLACK_PEN);

  // vertical lines

  Dc.VLine(mPianoX);
  Dc.VLine(mEventsX);
  Dc.VLine(mEventsX - 1);
  Dc.HLine(mEventsY);
  Dc.HLine(mEventsY - 1);
  Dc.HLine(mEventsY + mEventsHeight);

  // draw vlines and bar numbers

  Dc.SetFont(*mpFixedFont);
  BarInfo.SetClock(mFromClock);
  StopClk = x2Clock(mCanvasX + mCanvasWidth);
  int clk = BarInfo.Clock;
  int intro = mpSong->GetIntroLength();
  while (clk < StopClk)
  {
    clk = BarInfo.Clock;
    x = Clock2x(clk);
    // vertical lines and bar numbers
    int i;
    Dc.SetPen(*wxBLACK_PEN);
    sprintf(buf, "%d", BarInfo.BarNr + 1 - intro);
    if (x > mEventsX)
    {
      Dc.DrawText(buf, x + mLittleBit, mEventsY - mFixedFontHeight - 2);
      Dc.SetPen(*wxGREY_PEN);
      Dc.DrawLine(x, mEventsY - mFixedFontHeight, x, mEventsY + mEventsHeight);
    }

    Dc.SetPen(*wxLIGHT_GREY_PEN);
    for (i = 0; i < BarInfo.CountsPerBar; i++)
    {
      clk += BarInfo.TicksPerBar / BarInfo.CountsPerBar;
      x = Clock2x(clk);
      if (x > mEventsX)
      {
        Dc.DrawLine(x, mEventsY + 1, x, mEventsY + mEventsHeight);
      }
    }
    BarInfo.Next();
  }

  LineText(Dc, mCanvasX, mCanvasY, mPianoWidth, mTopInfoHeight);

  Dc.SetPen(*wxBLACK_PEN);
  DrawPianoRoll(Dc);

  // Draw chords from harmony-browser.
  if (mVisibleHBChord && gpHarmonyBrowser && !mpTrack->IsDrumTrack())
  {
    HBAnalyzer *an = gpHarmonyBrowser->getAnalyzer();
    if (an != 0)
    {
      wxBrush cbrush = *wxBLUE_BRUSH;
      wxBrush sbrush = *wxBLUE_BRUSH;
#ifdef __WXMSW__
      cbrush.SetColour(191,191,255);
      sbrush.SetColour(191,255,191);
#else
      cbrush.SetColour(220,220,255);
      sbrush.SetColour(230,255,230);
#endif

      //Dc.SetClippingRegion(mEventsX, mEventsY, mEventsWidth, mEventsHeight);
      Dc.SetLogicalFunction(wxXOR);
      Dc.SetPen(*wxTRANSPARENT_PEN);

      int steps = an->Steps();
      for (int step = 0; step < steps; step ++)
      {
        int start = an->Step2Clock(step);
        int stop  = an->Step2Clock(step + 1);
        if (stop > mFromClock && start < mToClock)
        {
          // this chord is visible
          HBContext *context = an->GetContext(step);
          HBChord chord = context->Chord();
          HBChord scale = context->Scale();

          int x = Clock2x(start);
          if (x < mEventsX)        // clip to left border
            x = mEventsX;
          int w = Clock2x(stop) - x;
          if (w <= 0)
            continue;

          int h = mTrackHeight;
          for (int i = 0; i < 12; i++)
          {
            int pitch = i;
            wxBrush *brush = 0;
            if (chord.Contains(i))
            {
              brush = &cbrush;
            }
            else if (scale.Contains(i))
            {
              brush = &sbrush;
            }
            if (brush)
            {
              Dc.SetBrush(*brush);
              while (pitch < 127)
              {
                int y = Pitch2y(pitch);
                if (y >= mEventsY && y <= mEventsY + mEventsHeight - h) // y-clipping
                {
                  Dc.DrawRectangle(x, y, w, h);
                }
                pitch += 12;
              }
            }
          }
        }
      }

      //Dc.DestroyClippingRegion();
      Dc.SetLogicalFunction(wxCOPY);
      Dc.SetPen(*wxBLACK_PEN);
      Dc.SetBrush(*wxBLACK_BRUSH);

      //delete an; PORTING
    }
  }
  /////////end draw choords

  if (mVisibleAllTracks)
  {
    int i;
    for (i = 0; i < mpSong->nTracks; i++)
    {
      JZTrack *t = mpSong->GetTrack(i);
      if (t != mpTrack && IsVisible(t))
      {
        DrawEvents(Dc, t, StatKeyOn, wxLIGHT_GREY_BRUSH, TRUE);
      }
    }
  }

  if (mVisibleKeyOn)
    DrawEvents(Dc, mpTrack, StatKeyOn, wxRED_BRUSH, FALSE);
  if (mVisiblePitch)
    DrawEvents(Dc, mpTrack, StatPitch, wxBLUE_BRUSH, FALSE);
  if (mVisibleController)
    DrawEvents(Dc, mpTrack, StatControl, wxCYAN_BRUSH, FALSE);
  if (mVisibleProgram)
    DrawEvents(Dc, mpTrack, StatProgram, wxGREEN_BRUSH, FALSE);
  if (mVisibleTempo)
    DrawEvents(Dc, mpTrack, StatSetTempo, wxGREEN_BRUSH, FALSE);
  if (mVisibleSysex)
    DrawEvents(Dc, mpTrack, StatSysEx, wxGREEN_BRUSH, FALSE);
  if (mVisiblePlayTrack)
    DrawEvents(Dc, mpTrack, StatPlayTrack, wxLIGHT_GREY_BRUSH, FALSE);
  
  DrawEvents(Dc, mpTrack, StatEndOfTrack, wxRED_BRUSH, FALSE);
  DrawEvents(Dc, mpTrack, StatText, wxBLACK_BRUSH, FALSE);

  Dc.SetPen(*wxBLACK_PEN);
  Dc.SetBrush(*wxBLACK_BRUSH);
  Dc.SetBackground(*wxWHITE_BRUSH);        // xor-bug

  mpSnapSel->Draw(Dc, mEventsX, mEventsY, mEventsWidth, mEventsHeight);

  DrawPlayPosition(Dc);
}

// Decription:
//   Draw the "play position", by placing a vertical line where the
// "play clock" is.
void JZPianoWindow::DrawPlayPosition(wxDC& Dc)
{
  if (!mpSnapSel->Active && mPlayClock >= mFromClock && mPlayClock < mToClock)
  {
    Dc.SetBrush(*wxBLACK_BRUSH);
    Dc.SetPen(*wxBLACK_PEN);
    int x = Clock2x(mPlayClock);

//    Dc.SetLogicalFunction(wxXOR);

    // Draw a line, 2 pixels wide.
    Dc.DrawLine(x,  mCanvasY,x,  mEventsY + mEventsHeight);
    Dc.DrawLine(x+1,mCanvasY,x+1,mEventsY + mEventsHeight);

//    Dc.SetLogicalFunction(wxCOPY);
  }
}

void JZPianoWindow::OnEventWinPaintSub(int x, int y)
{
  mCanvasX = x;
  mCanvasY = y;
  int xc, yc;
  GetClientSize(&xc, &yc);
  mCanvasWidth = xc;
  mCanvasHeight = yc;

  mEventsX = mCanvasX + mLeftInfoWidth;
  mEventsY = mCanvasY + mTopInfoHeight;
  mEventsWidth = mCanvasWidth - mLeftInfoWidth;
  mEventsHeight = mCanvasHeight - mTopInfoHeight;

  mFromLine = mCanvasY / mTrackHeight; 
  mToLine = (mCanvasY + mCanvasHeight - mTopInfoHeight) / mTrackHeight;
  mFromClock = mCanvasX * mClockTicsPerPixel;
  mToClock = x2Clock(mCanvasX + mCanvasWidth);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZPianoWindow::OnSize(wxSizeEvent& Event)
{
  GetClientSize(&mCanvasWidth, &mCanvasHeight);
  if (mCanvasWidth > 0 && mCanvasHeight > 0)
  {
    SetScrollRanges(mCanvasX, mCanvasY);
    Refresh(false);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool JZPianoWindow::OnCharHook(wxKeyEvent& Event)
{
  return OnKeyEvent(Event);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZPianoWindow::OnMenuCommand(int Id)
{
  switch (Id)
  {
    case ID_HELP_PIANO_WINDOW:
      gpHelpInstance->ShowTopic("Piano Window");
      break;
  }
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool JZPianoWindow::OnKeyEvent(wxKeyEvent& Event)
{
  if (Event.ControlDown())
  {
    switch (Event.GetKeyCode())
    {
      case 'Z':
        OnMenuCommand(wxID_UNDO);
        return true;
      case 'Y':
        OnMenuCommand(wxID_REDO);
        return true;
      case 'X':
        OnMenuCommand(wxID_CUT);
        return true;
      case 'C':
      case WXK_INSERT:
        OnMenuCommand(wxID_COPY);
        return true;
    }
  }
  else if (Event.ShiftDown())
  {
    switch (Event.GetKeyCode())
    {
      case WXK_UP:
        if (mTrackIndex > 0)
        {
          --mTrackIndex;
          NewPosition(mTrackIndex, -1L);
        }
        return true;
      case WXK_DOWN:
        if (mTrackIndex < mpSong->nTracks - 1)
        {
          ++mTrackIndex;
          NewPosition(mTrackIndex, -1L);
        }
        return true;
    }
  }
  else
  {
    switch(Event.GetKeyCode())
    {
      case WXK_DELETE:
        OnMenuCommand(wxID_DELETE);
        return true;
    }
  }

  return false;
}

void JZPianoWindow::NewPosition(int TrackIndex, int Clock)
{
  mFromLines[mTrackIndex] = mFromLine;

  // change track
  if (TrackIndex >= 0)
  {
    mTrackIndex = TrackIndex;
    mpTrack = mpSong->GetTrack(mTrackIndex);
    mpPianoFrame->SetTitle(mpTrack->GetName());
  }

  // change position
  if (Clock >= 0)
  {
    int x = Clock2x(Clock);
    SetScrollPosition(x - mLeftInfoWidth, Line2y(mFromLines[mTrackIndex]));
  }

// SN++ Ist geaendert. OnPaint zeichnet immer neu -> Bug Fix bei ZoomOut!
/*
  // OnPaint() redraws only if clock has changed
  if (mpCtrlEdit && TrackIndex >= 0)
    mpCtrlEdit->ReInit(mpTrack, mFromClock, mClockTicsPerPixel);
*/
  Refresh();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZPianoWindow::SetScrollRanges(const int& x, const int& y)
{
  int Width, Height;
  GetVirtualEventSize(Width, Height);
  SetScrollbars(
    mScrollSize,
    mScrollSize,
    Width / mScrollSize,
    Height / mScrollSize,
    x,
    y);
  EnableScrolling(false, false);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZPianoWindow::SetScrollPosition(int x, int y)
{
  x /= mScrollSize;
  y /= mScrollSize;
  Scroll(x, y);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZPianoWindow::Line2y(int Line)
{
  return Line * mTrackHeight + mTopInfoHeight;
}

// ********************************************************************
// Painting
// ********************************************************************


void JZPianoWindow::DrawPianoRoll(wxDC& Dc)
{
  // Draw the grey background for the keyboard.
  Dc.SetBrush(*wxLIGHT_GREY_BRUSH);
  Dc.DrawRectangle(
    mPianoX,
    mEventsY,
    mPianoWidth,
    mEventsHeight);

  Dc.SetBrush(*wxBLACK_BRUSH);

//  Dc.SetTextBackground(*wxLIGHT_GREY);

  int wBlack = mPianoWidth * 2 / 3;
  int Pitch = 127 - mFromLine;
  int y = Line2y(mFromLine);

  if (
    mVisibleKeyOn &&
    !mpTrack->GetAudioMode() &&
    (!mpTrack->IsDrumTrack() || !mVisibleDrumNames))
  {
    Dc.SetFont(*mpFixedFont);

    while (Pitch >= 0 && y < mEventsY + mEventsHeight)
    {
      if (IsBlack(Pitch))
      {
        Dc.DrawRectangle(mCanvasX, y, wBlack, mTrackHeight);
        Dc.DrawLine(mCanvasX + wBlack, y + mTrackHeight/2, mCanvasX + mPianoWidth, y + mTrackHeight/2);
        Dc.SetPen(*wxWHITE_PEN);
        Dc.DrawLine(mCanvasX + wBlack+1, y + mTrackHeight/2+1, mCanvasX + mPianoWidth, y + mTrackHeight/2+1);
        Dc.DrawLine(mCanvasX, y, mCanvasX + wBlack, y);
        Dc.SetPen(*wxBLACK_PEN);
      }
      else if ((Pitch % 12) == 0)
      {
        Dc.DrawLine(mCanvasX, y + mTrackHeight, mCanvasX + mPianoWidth, y + mTrackHeight);
        Dc.SetPen(*wxWHITE_PEN);
        Dc.DrawLine(mCanvasX, y + mTrackHeight + 1, mCanvasX + mPianoWidth, y + mTrackHeight + 1);
        Dc.SetPen(*wxBLACK_PEN);
        ostringstream Oss;
        Oss << Pitch / 12;
        Dc.DrawText(Oss.str().c_str(), mCanvasX + wBlack + mLittleBit, y + mTrackHeight / 2);
      }
      else if (!IsBlack(Pitch - 1))
      {
        Dc.DrawLine(mCanvasX, y + mTrackHeight, mCanvasX + mPianoWidth, y + mTrackHeight);
        Dc.SetPen(*wxWHITE_PEN);
        Dc.DrawLine(mCanvasX, y + mTrackHeight + 1, mCanvasX + mPianoWidth, y + mTrackHeight + 1);
        Dc.SetPen(*wxBLACK_PEN);
      }

      y += mTrackHeight;
      --Pitch;
    }
  }
  else if (mpTrack->GetAudioMode())
  {
    Dc.SetFont(*mpDrumFont);
    while (Pitch >= 0 && y < mEventsY + mEventsHeight)
    {
      Dc.DrawText(gpMidiPlayer->GetSampleName(Pitch), mCanvasX + mLittleBit, y);
      y += mTrackHeight;
      --Pitch;
    }
  }
  else
  {
    // Draw text?
    if (mVisibleKeyOn && mVisibleDrumNames)
    {
      Dc.SetFont(*mpDrumFont);
      while (Pitch >= 0 && y < mEventsY + mEventsHeight)
      {
        Dc.DrawText(
          gpConfig->DrumName(Pitch + 1).first.c_str(),
          mCanvasX + mLittleBit,
          y);

        y += mTrackHeight;

        --Pitch;
      }
    }
    else if (mVisibleController)
    {
      Dc.SetFont(*mpDrumFont);
      while (Pitch >= 0 && y < mEventsY + mEventsHeight)
      {
        Dc.DrawText(
          gpConfig->CtrlName(Pitch + 1).first.c_str(),
          mCanvasX + mLittleBit,
          y);

        y += mTrackHeight;
        --Pitch;
      }
    }
    else if (mVisibleProgram)
    {
      Dc.SetFont(*mpDrumFont);
      while (Pitch >= 0 && y < mEventsY + mEventsHeight)
      {
        Dc.DrawText(
          gpConfig->VoiceName(Pitch + 1).first.c_str(),
          mCanvasX + mLittleBit,
          y);

        y += mTrackHeight;
        --Pitch;
      }
    }
    else if (mVisibleSysex)
    {
      Dc.SetFont(*mpDrumFont);
      while (Pitch >= 0 && y < mEventsY + mEventsHeight)
      {
        Dc.DrawText(
          tSynthSysex::GetSysexGroupName(Pitch + 1),
          mCanvasX + mLittleBit,
          y);
        y += mTrackHeight;
        --Pitch;
      }
    }
    else if (mVisiblePitch)
    {
    }
  }

  //Dc.DestroyClippingRegion();
  //Dc.SetTextBackground(*wxWHITE);
  Dc.SetFont(*mpFont);
}

void JZPianoWindow::DrawEvent(
  wxDC& Dc,
  JZEvent* pEvent,
  const wxBrush* Brush,
  int xoor,
  int force_color)
{
  if (pEvent->IsKeyPressure() || pEvent->IsChnPressure())
  {
    return;
  }

  int length = pEvent->GetLength() / mClockTicsPerPixel;
  // Always draw at least two pixels to avoid invisible (behind a
  // vertical line) or zero-length events:
  if (length < 3)
  {
    length = 3;
  }

  if (xoor)
  {
    Dc.SetLogicalFunction(wxXOR);
  }
  int x = Clock2x(pEvent->GetClock());
  int y = Pitch2y(pEvent->GetPitch());
  if (!xoor)        
  {
    Dc.SetBrush(*wxWHITE_BRUSH);
    Dc.DrawRectangle(x, y + mLittleBit, length, mTrackHeight - 2 * mLittleBit);
  }

  // show velocity as colors
  if (force_color != 0 && mUseColors && pEvent->IsKeyOn())
  {
    int vel = pEvent->IsKeyOn()->Veloc;

    // Next line is "Patrick Approved."
    Dc.SetBrush(mpColorBrush[ vel * NUM_COLORS / 128 ]);
  }
  else
  {
    Dc.SetBrush(*Brush);
  }
  // end velocity colors

  Dc.DrawRectangle(x, y + mLittleBit, length, mTrackHeight - 2 * mLittleBit);

  if (xoor)
  {
    Dc.SetLogicalFunction(wxCOPY);
  }
  Dc.SetBrush(*wxBLACK_BRUSH);
}

void JZPianoWindow::DrawEvents(
  wxDC& Dc,
  JZTrack *t,
  int Stat,
  const wxBrush* Brush,
  int force_color)
{
//  Dc.SetClippingRegion(mEventsX, mEventsY, mEventsWidth, mEventsHeight);
  Dc.SetBrush(*Brush);

  tEventIterator Iterator(t);
  JZEvent* pEvent = Iterator.First();
  int FromPitch = 127 - mToLine;
  int ToPitch   = 127 - mFromLine;

  // Coordinate for Linien

  int x0 = Clock2x(0);
  int y0 = Line2y(64);
  char buf[20];

  while (pEvent)
  {
    if (pEvent->Stat == Stat)
    {
      int Pitch   = pEvent->GetPitch();
      int Length = pEvent->GetLength();
      int Clock  = pEvent->GetClock();

      int x1 = Clock2x(Clock);
      int y1 = Line2y(127 - Pitch);
//        if (pEvent->IsPlayTrack()) {
//          y1=Line2y(127-pEvent->IsPlayTrack()->track); //JAVE so the y position of playtrack events tell which track they play (the drawing should rather be polymorpic in my opinion)
      //use pitch instead
      //      }
      // event partially visible?
      if (Clock + Length >= mFromClock && FromPitch < Pitch && Pitch <= ToPitch)
      {
        int DrawLength = Length/mClockTicsPerPixel;
        // do clipping ourselves
        if (x1 < mEventsX)
        {
          DrawLength -= mEventsX - x1;
          x1 = mEventsX;
        }
        // Always draw at least two pixels to avoid invisible (behind a
        // vertical line) or zero-length events:
        if (DrawLength < 3)
        {
          DrawLength = 3;
        }

        // show velocity as colors
        if (!force_color && mUseColors && pEvent->IsKeyOn())
        {
          int vel = pEvent->IsKeyOn()->Veloc;
          Dc.SetBrush(mpColorBrush[ vel * NUM_COLORS / 128 ]);
        }
        else
        {
          Dc.SetBrush(*Brush);
        }
        // end velocity colors

        Dc.DrawRectangle(x1, y1 + mLittleBit, DrawLength, mTrackHeight - 2 * mLittleBit);
        //shouldnt it be in drawevent? odd. 

        if (pEvent->IsPlayTrack())
        {
          Dc.SetPen(*wxBLACK_PEN);
          ostringstream Oss;
          Oss << "Track:" << pEvent->IsPlayTrack()->track; 
          Dc.DrawText(Oss.str().c_str(), x1, y1 + mLittleBit);
        }
      }
      
      if (Clock + Length >= mFromClock)
      {
        //thesse events are always visible in vertical
        if (pEvent->IsEndOfTrack())
        {
          Dc.SetPen(*wxRED_PEN);
          Dc.VLine(x1); //draw a vertical bar
          Dc.SetPen(*wxBLACK_PEN);
          sprintf(buf, "EOT"); 
          Dc.DrawText(buf, x1, y1 + mLittleBit);
        }
        
        if (pEvent->IsText())
        {
          Dc.SetPen(*wxGREEN_PEN);
          Dc.VLine(x1); //draw a vertical bar
          Dc.SetPen(*wxBLACK_PEN);
          sprintf(buf, (const char*)pEvent->IsText()->GetText());
          int textX;
          int textY;
          
          Dc.GetTextExtent((const char*)pEvent->IsText()->GetText(), &textX, &textY); 
          Dc.SetBrush(*wxWHITE_BRUSH);
          int textlabely = mCanvasY + mTopInfoHeight;//text labels drawn at top
          Dc.DrawRectangle(x1-textX, textlabely + mLittleBit, textX, textY);//mTrackHeight - 2 * mLittleBit);
          Dc.DrawText(buf, x1-textX, textlabely + mLittleBit);
        }
      }

      x0 = x1;
      y0 = y1;

      if (Clock > mToClock)
      {
        break;
      }
    }
    pEvent = Iterator.Next();
  }
  Dc.SetBrush(*wxBLACK_BRUSH);
//  Dc.DestroyClippingRegion();
}

// Draws the a 3D button with text in it.  Used to draw the little area in the
// top left of the window.
void JZPianoWindow::LineText(
  wxDC& Dc,
  int x,
  int y,
  int w,
  int h,
  wxString str,
  bool down)
{
  Dc.SetBrush(*wxLIGHT_GREY_BRUSH); // Fill
  Dc.SetPen(*wxLIGHT_GREY_PEN);     // Outline
  Dc.DrawRectangle(x, y, w, h);

  x += 1;
  y += 1;
  w -= 2;
  h -= 2;

  // Draw the top and left lines of the 3D button.
  if (down)
  {
    Dc.SetPen(*wxBLACK_PEN);
  }
  else
  {
    Dc.SetPen(*wxWHITE_PEN);
  }

  Dc.DrawLine(x, y, x+w, y);
  Dc.DrawLine(x, y, x, y+h);

  // Draw the bottom and right lines of the 3D button.
  if (down)
  {
    Dc.SetPen(*wxWHITE_PEN);
  }
  else
  {
    Dc.SetPen(*wxBLACK_PEN);
  }

  Dc.DrawLine(x+w, y, x+w, y+h);
  Dc.DrawLine(x, y+h, x+w, y+h);

  // Print the message in the button.
  Dc.DrawText(str, x + mLittleBit, y + mLittleBit);
}

//-----------------------------------------------------------------------------
// Descriptions:
//   This mouse handler delegates to the subclassed event window.
//-----------------------------------------------------------------------------
void JZPianoWindow::OnMouseEvent(wxMouseEvent& Event)
{
  if (Event.Moving() && !Event.Dragging() && !mpMouseAction)
  {
    int fx, fy;
    LogicalMousePosition(Event, fx, fy);
    int pitch = y2Pitch(fy);
    ShowPitch(pitch);
    if (mpGuitarFrame)
    {
      mpGuitarFrame->ShowPitch(pitch);
    }
  }

  // dispatch

  if (!mpMouseAction)
  {
    int x, y;
    LogicalMousePosition(Event, x, y);

    if (y > mEventsY)        // click in event area?
    {
      if (mPianoX < x && x < mPianoX + mPianoWidth)
      {
        MousePiano(Event);
      }
      else if (mEventsX < x && x < mEventsX + mEventsWidth)
      {
        MouseEvents(Event);
      }
      else
      {
        OnEventWinMouseEvent(Event);
      }
    }
    else if (x > mEventsX)
    {
      // click in top line
      int action = mMousePlay.Action(Event);

      if (action)
      {
        if (!gpMidiPlayer->Playing)
        {
          int Clock, LoopClock;
          if (action == MA_CYCLE)
          {
            if (mpSnapSel->Selected)
            {
              Clock = mpFilter->FromClock;
              LoopClock = mpFilter->ToClock;
            }
            else
            {
              Clock = x2BarClock(x, 0);
              LoopClock = x2BarClock(x, 4);
            }
          }
          else
          {
            Clock = SnapClock(x2Clock(x));
            LoopClock = 0;
          }
          gpMidiPlayer->SetRecordInfo(0);
          gpMidiPlayer->StartPlay(Clock, LoopClock);
        }
        else
        {
          // Stop Record/Play
          gpMidiPlayer->StopPlay();
        }
      }
    }
  }
  else
  {
    OnEventWinMouseEvent(Event);
  }
}


// ------------------------------------------------------------------------
// Snapper
// ------------------------------------------------------------------------

void JZPianoWindow::SnapSelStop(wxMouseEvent& Event)
{
  if (mpSnapSel->Selected)
  {
    int fr = y2Pitch((mpSnapSel->r.y + mpSnapSel->r.height - 1));
    int to = y2Pitch(mpSnapSel->r.y + 1);

    mpFilter->FltEvents[FltKeyOn].Selected = mVisibleKeyOn;
    mpFilter->FltEvents[FltKeyOn].FromValue = fr;
    mpFilter->FltEvents[FltKeyOn].ToValue   = to;

    mpFilter->FltEvents[FltPitch].Selected = mVisiblePitch;
    mpFilter->FltEvents[FltPitch].FromValue = (fr << 7) - 8192;
    mpFilter->FltEvents[FltPitch].ToValue   = ((to + 1) << 7) - 8192;

    mpFilter->FltEvents[FltControl].Selected = mVisibleController;
    mpFilter->FltEvents[FltControl].FromValue = fr;
    mpFilter->FltEvents[FltControl].ToValue   = to;

    mpFilter->FltEvents[FltProgram].Selected = mVisibleProgram;
    mpFilter->FltEvents[FltProgram].FromValue = fr;
    mpFilter->FltEvents[FltProgram].ToValue   = to;

    mpFilter->FltEvents[FltTempo].Selected = mVisibleTempo;
    mpFilter->FltEvents[FltTempo].FromValue = fr;
    mpFilter->FltEvents[FltTempo].ToValue   = to;

    mpFilter->FltEvents[FltSysEx].Selected = mVisibleSysex;
    mpFilter->FltEvents[FltSysEx].FromValue = fr;
    mpFilter->FltEvents[FltSysEx].ToValue   = to;

    // SN++ Aftertouch (gehoeren to KeyOn Events).
    mpFilter->FltEvents[FltKeyPressure].Selected  = mVisibleKeyOn;
    mpFilter->FltEvents[FltKeyPressure].FromValue = fr;
    mpFilter->FltEvents[FltKeyPressure].ToValue   = to;

    // SN++ Channel Aftertouch
    mpFilter->FltEvents[FltChnPressure].Selected  = mVisibleMono;
    mpFilter->FltEvents[FltChnPressure].FromValue = fr;
    mpFilter->FltEvents[FltChnPressure].ToValue   = to;


    mpFilter->FromTrack = mTrackIndex;
    mpFilter->ToTrack   = mTrackIndex;
    mpFilter->FromClock = SnapClock(x2Clock(mpSnapSel->r.x + 1));
    mpFilter->ToClock   = SnapClock(x2Clock((mpSnapSel->r.x + mpSnapSel->r.width + 1)));
  }

  // SN++ Veloc- oder Aftertouch-Editor updaten
  if (mpCtrlEdit)
    mpCtrlEdit->UpDate();
}

void JZPianoWindow::SnapSelStart(wxMouseEvent &)
{
  mSnapCount = 0;
  int clk = SnapClock(mFromClock, 0);
  int qnt = SnapClocks();
  while (clk <= mToClock && mSnapCount < eMaxSnaps)
  {
    mSnapsX[mSnapCount++] = Clock2x(clk);
    clk += qnt;
  }
  if (mSnapCount < eMaxSnaps)
  {
    mpSnapSel->SetXSnap(mSnapCount, mSnapsX);
  }
  else
  {
    mpSnapSel->SetXSnap(0,0,0);
  }
  mpSnapSel->SetYSnap(
    mFromLine * mTrackHeight + mTopInfoHeight,
    mEventsY + mEventsHeight,
    mTrackHeight);
}

int JZPianoWindow::SnapClock(int Clock, int up)
{
  int qnt = SnapClocks();
  Clock -= (Clock % qnt);
  if (up)
  {
    Clock += qnt;
  }
  return Clock;
}

void JZPianoWindow::NewPlayPosition(int Clock)
{
  int scroll_clock = (mFromClock + 5 * mToClock) / 6L;

  if (
    !mpSnapSel->Active &&
    ((Clock > scroll_clock) || (Clock < mFromClock)) && (Clock >= 0L))
  {
    // Avoid permenent redraws when end of scroll range is reached
    if (Clock > mFromClock && mToClock >= mpSong->MaxQuarters * mpSong->TicksPerQuarter)
    {
      return;
    }

    int x = Clock2x(Clock);
    SetScrollPosition(x - mLeftInfoWidth, mCanvasY);
  }

  if (!mpSnapSel->Active)        // sets clipping
  {
    if (mPlayClock != Clock)
    {
 //     int oldplayclock = mPlayClock;
//      mPlayClock = Clock;
//        wxRect invalidateRect;
//        invalidateRect.x=Clock2x(oldplayclock)-1;
//        invalidateRect.y=mCanvasY;
//        invalidateRect.width=3;
//        invalidateRect.height= 100000000;
//       //       DrawPlayPosition();
//        Refresh(true, &invalidateRect);

//              invalidateRect.x=Clock2x(mPlayClock)-1;
//       Refresh(true, &invalidateRect);
//       DrawPlayPosition();

      Refresh();
    }
  }
}

int JZPianoWindow::EventsSelected(const char *msg)
{
  if (!mpSnapSel->Selected)
  {
    if (msg == 0)
      msg = "please select some events first";
    wxMessageBox((char *)msg, "Error", wxOK);
    return 0;
  }
  return 1;
}

void JZPianoWindow::ZoomIn()
{
  if (mClockTicsPerPixel >= 2)
  {
    mClockTicsPerPixel /= 2;
    int x = mCanvasX * 2;
    int y = mCanvasY;

    OnEventWinPaintSub(x, y);
    SetScrollRanges(x, y);
//    SetScrollPosition(x, y);

    NewPosition(mTrackIndex, mFromClock);

    if (x == 0)
    {
      Refresh();
    }
  }
}

void JZPianoWindow::ZoomOut()
{
  if (mClockTicsPerPixel <= 120)
  {
    mClockTicsPerPixel *= 2;
    int x = mCanvasX / 2;
    int y = mCanvasY;

    OnEventWinPaintSub(x, y);
    SetScrollRanges(x, y);
//    SetScrollPosition(x, y);

    NewPosition(mTrackIndex, mFromClock);

    if (x == 0)
    {
      Refresh();
    }
  }
}


int JZPianoWindow::OnEventWinMouseEvent(wxMouseEvent& Event)
{
  if (!mpMouseAction)
  {
    // create mpSnapSel?

    int x;
    int y;
    LogicalMousePosition(Event, x, y);
    if (mEventsX < x && x < mEventsX + mEventsWidth && mEventsY < y && y < mEventsY + mEventsHeight)
    {
      if (Event.LeftDown())
      {
        {
          SnapSelStart(Event);

          if (mpSnapSel->Selected)
          {
            Refresh(); //redraw the whole window instead(inefficient, we should rather invalidate a rect)
          }
          mpSnapSel->Event(Event);
          mpMouseAction = mpSnapSel;
        }
      }
    }
  }
  else
  {
    // mpMouseAction active

    if (mpMouseAction->Event(Event))
    {
      // mpMouseAction finished

      if (mpMouseAction == mpSnapSel)
      {
        SnapSelStop(Event);
        Refresh(); //ineficcient, invalidate rect first instead
        mpMouseAction = 0;
        return 1;
      }

      mpMouseAction = 0;
    }
  }
  return 0;
}

//   Indicate which key on the pianoroll that the mouse is hovering over by
// highlighting it.  This function is bad because it draws directly in the dc,
// rather it should invalidate and let OnDraw do the actual painting.
// Currently the code doesn't work because it doesn't care about scrolling
// (because I get the dc the wrong way).
void JZPianoWindow::ShowPitch(int Pitch)
{
  // This is the current position of the mouse.  mMouseLine is the last
  // position.
  int Line = y2Line(Pitch2y(Pitch));
  if (Line >= mFromLine && Line != mMouseLine)
  {
    wxClientDC Dc(this);

    // Translate scrolled coordinates.
    PrepareDC(Dc);

    Dc.SetLogicalFunction(wxXOR);

    Dc.SetBrush(*wxBLUE_BRUSH);
    if (mMouseLine >= 0) 
    {
      // Erase the previous highlight.
      Dc.DrawRectangle(
        mPianoX,
        Line2y(mMouseLine) + mLittleBit,
        mPianoWidth,
        mTrackHeight - 2 * mLittleBit);
    }
    mMouseLine = Line;

    // Draw the new position.
    Dc.DrawRectangle(
      mPianoX,
      Line2y(mMouseLine) + mLittleBit,
      mPianoWidth,
      mTrackHeight - 2 * mLittleBit);

    Dc.SetLogicalFunction(wxCOPY);
  }
}

int JZPianoWindow::x2Clock(int x)
{
  return (x - mEventsX) * mClockTicsPerPixel + mFromClock;
}

int JZPianoWindow::y2Line(int y, int up)
{
  if (up)
  {
    y += mTrackHeight;
  }
  y -= mTopInfoHeight;
  return y / mTrackHeight;
}

int JZPianoWindow::x2BarClock(int x, int next)
{
  int clk = x2Clock(x);
  JZBarInfo b(mpSong);
  b.SetClock(clk);
  while (next--)
    b.Next();
  return b.Clock;
}

int JZPianoWindow::y2yLine(int y, int up)
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

void JZPianoWindow::MouseCutPaste(wxMouseEvent& Event, bool Cut)
{
  wxClientDC Dc(this);

  PrepareDC(Dc);

  // Convert physical coordinates to logical (scrolled) coordinates.
  wxPoint Point = Event.GetLogicalPosition(Dc);

  int x = Point.x;
  int y = Point.y;

  int Clock = x2Clock(x);
  int Pitch = y2Pitch(y);
  JZEvent* pEvent = FindEvent(mpTrack, Clock, Pitch);
  if (pEvent)
  {
    Copy(mpTrack, pEvent, Cut);
  }
  else
  {
    Paste(mpTrack, SnapClock(Clock), Pitch);
  }

  // allways redraw
  Refresh();
}

void JZPianoWindow::MouseEvents(wxMouseEvent& Event)
{
  int action = mMouseEvent.Action(Event);

  if (action)
  {
    int x, y;
    LogicalMousePosition(Event, x, y);

    int Clock = x2Clock(x);
    int Pitch = y2Pitch(y);
    JZEvent *m = FindEvent(mpTrack, Clock, Pitch);
    tKeyOn *k = 0;
    tPlayTrack *p = 0;
    if (m)
    {
      // both these events are drag length
      k = m->IsKeyOn();
      p = m->IsPlayTrack();
    }
    switch (action)
    {
      case MA_CUTPASTE:
        MouseCutPaste(Event, 1);
        break;

      case MA_COPY:
        MouseCutPaste(Event, 0);
        break;

      case MA_LENGTH:
        if (k)
        {
          if (!mpTrack->GetAudioMode())
          {
            mpMouseAction = new tKeyLengthDragger(k, this);
          }
        }
        else
        {
          if(p)
          {
            mpMouseAction = new tPlayTrackLengthDragger(p, this);
          }
          else if (mVisibleAllTracks)
          {
            // event not found, maybe change to another Track
            int i;
            for (i = 0; i < mpSong->nTracks; i++)
            {
              JZTrack* pTrack = mpSong->GetTrack(i);
              if (IsVisible(pTrack) && FindEvent(pTrack, Clock, Pitch))
              {
                NewPosition(i, -1L);
                break;
              }
            }
          }
        }
        break;


      case MA_DIALOG:
        EventDialog(m, this, mpTrack, Clock, mpTrack->Channel - 1, Pitch);
        break;


      case MA_LISTEN:
        MousePiano(Event);
        break;

      case MA_SELECT:
      case MA_CONTSEL:
        OnEventWinMouseEvent(Event);
        break;

      case MA_VELOCITY:
        if (k)
        {
          JZRectangle r;
          r.x = mCanvasX + mLittleBit;
          r.y = mCanvasY;
          r.SetWidth(mPianoWidth - 2 * mLittleBit);
          r.SetHeight(mTopInfoHeight);

          tVelocCounter *VelocCounter = new tVelocCounter(this, &r, k);
          VelocCounter->Event(Event);
          mpMouseAction = VelocCounter;
        }
        break;

    }
  }
}

void JZPianoWindow::MousePiano(wxMouseEvent& Event)
{
  if (Event.ButtonDown())
  {
    mpMouseAction = new tMousePlay(this, Event);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZPianoWindow::GetVirtualEventSize(int& Width, int& Height)
{
  int TotalClockTics = mpSong->MaxQuarters * mpSong->TicksPerQuarter;
  Width = TotalClockTics / mClockTicsPerPixel + mLeftInfoWidth;
  Height = 127 * mTrackHeight + mTopInfoHeight;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZPianoWindow::LogicalMousePosition(
  wxMouseEvent& MouseEvent,
  int& x,
  int& y)
{
  MouseEvent.GetPosition(&x, &y);
  x += mCanvasX;
  y += mCanvasY;
}

//-----------------------------------------------------------------------------
// This is an an event handler for tMouseCounter.
//-----------------------------------------------------------------------------
void JZPianoWindow::ButtonLabelDisplay(const wxString& Text, bool IsButtonDown)
{
  wxClientDC Dc(this);
  LineText(Dc, 0, 0, mPianoWidth, mTopInfoHeight, Text, IsButtonDown);
}

//*****************************************************************************
// Visible
//*****************************************************************************

int JZPianoWindow::IsVisible(JZEvent* pEvent)
{
  switch (pEvent->Stat)
  {
    case StatKeyOn:
      return mVisibleKeyOn;
    case StatPitch:
      return mVisiblePitch;
    case StatControl:
      return mVisibleController;
    case StatProgram:
      return mVisibleProgram;
    case StatSetTempo:
      return mVisibleTempo;
    case StatSysEx:
      return mVisibleSysex;
    case StatPlayTrack:
      return mVisiblePlayTrack;
    case StatEndOfTrack:
      return true;
    case StatText:
      return true;
    case StatChnPressure:
      return mVisibleMono;
  }
  return 0;
}

int JZPianoWindow::IsVisible(JZTrack* pTrack)
{
  if (!mVisibleAllTracks)
  {
    return pTrack == mpTrack;
  }

  return (
    mpTrack->Channel == gpConfig->GetValue(C_DrumChannel)) ==
    (pTrack->Channel == gpConfig->GetValue(C_DrumChannel));
}

// ********************************************************************
// Utilities
// ********************************************************************

int JZPianoWindow::SnapClocks()
{
  int clk = mpSong->TicksPerQuarter * 4L / mSnapDenomiator;
  if (clk < 1)
    return 1;
  return clk;
}

void JZPianoWindow::SetSnapDenom(int Value)
{
  const int Size = 4;
  const struct
  {
    int mId;
    int mValue;
  } Table[Size] =
  {
    { ID_SNAP_8,    8 },
    { ID_SNAP_8D,  12 },
    { ID_SNAP_16,  16 },
    { ID_SNAP_16D, 24 },
  };

  int Id = 0;

  // find the button
  for (int i = 0; i < Size; ++i)
  {
    if (Table[i].mValue == Value)
    {
      Id = Table[i].mId;
    }
  }

  mpPianoFrame->SetToolbarButtonState(Id);

  mSnapDenomiator = Value;
//  mMouseEvent.SetLeftAction(MA_CUTPASTE);
}

int JZPianoWindow::y2Pitch(int y)
{
  int pitch = 127 - y2Line(y);
  if (pitch < 0)
    return 0;
  if (pitch > 127)
    return 127;
  return pitch;
}


int JZPianoWindow::Pitch2y(int Pitch)
{
  return Line2y(127 - Pitch);
}


JZEvent *JZPianoWindow::FindEvent(JZTrack* pTrack, int Clock, int Pitch)
// Pitch == -1: search for any pitches
{
  tEventIterator Iterator(pTrack);
  JZEvent* pEvent = Iterator.First();
  while (pEvent)
  {
    if (pEvent->GetClock() <= Clock)
    {
      if ((pEvent->GetClock() + pEvent->GetLength() >= Clock)
           && (pEvent->GetPitch() == Pitch || Pitch == -1)
           && IsVisible(pEvent))
      {
        return pEvent;
      }
    }
    else
    {
      return 0;
    }
    pEvent = Iterator.Next();
  }
  return 0;
}


void JZPianoWindow::kill_keys_aftertouch(JZTrack *t, JZEvent* pEvent)
{
  int key,channel;
  tEventIterator iter(t);
  tKeyPressure *a;
  tKeyOn *k = pEvent->IsKeyOn();
  if (!k)
  {
    return;
  }
  if (k->Length < 2)
  {
    return;
  }
  key = k->Key;
  channel = k->Channel;
  pEvent = iter.Range(k->GetClock() + 1, k->GetClock() + k->Length);
  while (pEvent)
  {
    a = pEvent->IsKeyPressure();
    if (a)
    {
      if (a->Key == key && a->Channel == channel)
      {
        t->Kill(pEvent);
      }
    }
    pEvent = iter.Next();
  }
}

void JZPianoWindow::paste_keys_aftertouch(JZTrack *t, JZEvent* pEvent)
{
  int key,channel;
  tEventIterator iter(t);
  tKeyPressure *a;
  tKeyOn *k = pEvent->IsKeyOn();
  if (!k) return;
  channel = k->Channel;
  if (k->Length < 2) return;
  key = k->Key;
  pEvent = iter.Range(k->GetClock() + 1, k->GetClock() + k->Length);
  while (pEvent)
  {
    a = pEvent->IsKeyPressure();
    if (a)
    {
      if (a->Key == key && a->Channel == channel)
      {
        mPasteBuffer.Put(pEvent->Copy());
      }
    }
    pEvent = iter.Next();
  }
}

int JZPianoWindow::Clock2x(int Clock)
{
  return mEventsX + (Clock - mFromClock) / mClockTicsPerPixel;
}

// show the guitar edit window.
void JZPianoWindow::CreateGuitarWindow()
{
  if (!mpGuitarFrame)
  {
    mpGuitarFrame = new JZGuitarFrame(this);
  }
  mpGuitarFrame->Show(true);
}

void JZPianoWindow::UpdateControl()
{
  if (mpCtrlEdit)
  {
    mpCtrlEdit->UpDate();
  }
}

void JZPianoWindow::ApplyToTrack(JZEvent* pEvent1, JZEvent* pEvent2)
{
  mpTrack->Kill(pEvent1);
  mpTrack->Put(pEvent2);
  mpTrack->Cleanup();
}

void JZPianoWindow::KillTrackEvent(JZEvent* pEvent)
{
  mpTrack->Kill(pEvent);
}

// positions for controller editor
#define CtrlH(h)        ((h)/4)
#define CtrlY(h)        (h - CtrlH(h))

// Activate velocity edit.
void JZPianoWindow::CtrlVelocity()
{
  int Width, Height;
  GetClientSize(&Width, &Height);

  delete mpCtrlEdit;

  mpCtrlEdit = new tVelocEdit(
    this,
    "Velocity",
    mPianoWidth,
    0,
    CtrlY(Height),
    mCanvasWidth,
    CtrlH(Height));

  mpCtrlEdit->ReInit(mpTrack, mFromClock, mClockTicsPerPixel);

  Refresh();
}

void JZPianoWindow::CtrlChannelAftertouchEdit()
{
  int Width, Height;
  GetClientSize(&Width, &Height);

  delete mpCtrlEdit;

  mpCtrlEdit = new tChannelAfterEdit(
    this,
    "Channel Aftertouch",
    mPianoWidth,
    0,
    CtrlY(Height),
    mCanvasWidth,
    CtrlH(Height));

  mpCtrlEdit->ReInit(mpTrack, mFromClock, mClockTicsPerPixel);
  Refresh();  
}

void JZPianoWindow::CtrlPolyAftertouchEdit()
{
  int Width, Height;
  GetClientSize(&Width, &Height);

  delete mpCtrlEdit;

  mpCtrlEdit = new tPolyAfterEdit(
    this,
    "Key Aftertouch",
    mPianoWidth,
    0,
    CtrlY(Height),
    mCanvasWidth,
    CtrlH(Height));

  mpCtrlEdit->ReInit(mpTrack, mFromClock, mClockTicsPerPixel);
  Refresh();  
}

void JZPianoWindow::CtrlNone()
{
  delete mpCtrlEdit;
  mpCtrlEdit = 0;
  Refresh();  
}

void JZPianoWindow::CtrlTempo()
{
  tEventIterator Iterator(mpTrack);

  mpTrack->Sort();

  JZEvent* pEvent = Iterator.Range(0, (unsigned) mpTrack->GetLastClock() + 1);
  tSetTempo* pSetTempo;
  int Min = 240;
  int Max = 20;
  while (pEvent)
  {
    if ((pSetTempo = pEvent->IsSetTempo()) != 0)
    {
      if (pSetTempo->GetBPM() < Min)
      {
        Min = pSetTempo->GetBPM();
      }
      if (pSetTempo->GetBPM() > Max)
      {
        Max = pSetTempo->GetBPM();
      }
    }
    pEvent = Iterator.Next();
  }
  if (Min - 50 > 20)
  {
    Min -= 50;
  }
  else
  {
    Min = 20;
  }
  if (Max + 50 < 240)
  {
    Max += 50;
  }
  else
  {
    Max = 240;
  }

  int Width, Height;
  GetClientSize(&Width, &Height);

  delete mpCtrlEdit;

  mpCtrlEdit = new tTempoEdit(
    Min,
    Max,
    this,
    "Tempo",
    mPianoWidth,
    0,
    CtrlY(Height),
    mCanvasWidth,
    CtrlH(Height));

  mpCtrlEdit->ReInit(mpTrack, mFromClock, mClockTicsPerPixel);
  Refresh();  
}

void JZPianoWindow::EditFilter()
{
  mpFilter->Dialog(0);
}

void JZPianoWindow::SelectController()
{
  int i = SelectControllerDlg();
  if (i > 0)
  {
    int Width, Height;
    GetClientSize(&Width, &Height);

    delete mpCtrlEdit;

    mpCtrlEdit = new tCtrlEdit(
      i - 1,
      this,
      gpConfig->CtrlName(i).first.c_str(),
      mPianoWidth,
      0,
      CtrlY(Height),
      mCanvasWidth,
      CtrlH(Height));

    mpCtrlEdit->ReInit(mpTrack, mFromClock, mClockTicsPerPixel);
    Refresh();
  }  
}

void JZPianoWindow::CtrlModulation()
{
  int Width, Height;
  GetClientSize(&Width, &Height);

  delete mpCtrlEdit;

  mpCtrlEdit = new tCtrlEdit(
    1,
    this,
    "Modulation",
    mPianoWidth,
    0,
    CtrlY(Height),
    mCanvasWidth,
    CtrlH(Height));

  mpCtrlEdit->ReInit(mpTrack, mFromClock, mClockTicsPerPixel);
  Refresh();  
}

void JZPianoWindow::CtrlPitch()
{
  int Width, Height;
  GetClientSize(&Width, &Height);

  delete mpCtrlEdit;

  mpCtrlEdit = new tPitchEdit(
    this,
    "Pitch",
    mPianoWidth,
    0,
    CtrlY(Height),
    mCanvasWidth,
    CtrlH(Height));
  
  mpCtrlEdit->ReInit(mpTrack, mFromClock, mClockTicsPerPixel);
  Refresh();  
}

void JZPianoWindow::Redo()
{
  mpSong->Redo();
  Refresh();
  if (mpCtrlEdit && mpTrack >= 0)
  {
    mpCtrlEdit->ReInit(mpTrack, mFromClock, mClockTicsPerPixel);  
  }
}

// Undo actions 
void JZPianoWindow::Undo()
{
  mpSong->Undo();
  Refresh();
  if (mpCtrlEdit && mpTrack >= 0)
  {
    mpCtrlEdit->ReInit(mpTrack, mFromClock, mClockTicsPerPixel);  
  }
}

// Quantize selected events.
void JZPianoWindow::Quantize()
{
  if (EventsSelected())
  {
    tCmdQuantize cmd(mpFilter, SnapClocks(), 0, 0);
    cmd.Execute(1);
    Refresh();
  }
}

// Flip events up and down.
void JZPianoWindow::ExchangeUpDown()
{
  if (EventsSelected())
  {
    tCmdExchUpDown cmd(mpFilter);
    cmd.Execute(1);
    Refresh();
  }  
}

// Flip events left to right.
void JZPianoWindow::ExchangeLeftRight()
{
  if (EventsSelected())
  {
    tCmdExchLeftRight cmd(mpFilter);
    cmd.Execute(1);
    Refresh();
  }  
}

// Shift events snapclock clocks to left.
void JZPianoWindow::ShiftLeft()
{
  if (EventsSelected())
  {
    int steps = -SnapClocks();
    tCmdShift cmd(mpFilter, steps);
    cmd.Execute();
    Refresh();
  }
}

// Shift events snapclock clocks to right.
void JZPianoWindow::ShiftRight()
{
  if (EventsSelected())
  {
    int steps = SnapClocks();
    tCmdShift cmd(mpFilter, steps);
    cmd.Execute();
    Refresh();
  }
}

// helper for cut and copy events
void JZPianoWindow::CutOrCopy(int Id)
{
  if (EventsSelected())
  {
    mPasteBuffer.Clear();
    tCmdCopyToBuffer cmd(mpFilter, &mPasteBuffer);
    mpFilter->OtherSelected = mVisibleTempo;
    cmd.Execute(0);        // no UNDO
    if (Id == wxID_CUT)
    {
      tCmdErase cmd(mpFilter);
      cmd.Execute(1);        // with UNDO
      Refresh();
    }
    mpFilter->OtherSelected = 0;
    if (mpGuitarFrame)
    {
      mpGuitarFrame->Update();
//      mpGuitarFrame->Redraw();
    }
  }  
}

void JZPianoWindow::Erase()
{
  if (EventsSelected())
  {
    tCmdErase cmd(mpFilter);
    cmd.Execute(1);        // with UNDO
    Refresh();
  }  
}

void JZPianoWindow::ToggleVisibleAllTracks()
{
  mVisibleAllTracks = !mVisibleAllTracks;

  Refresh();  
}

void JZPianoWindow::MSelect()
{
  mpPianoFrame->PressRadio(ID_SELECT);
  mMouseEvent.SetLeftAction(MA_SELECT);
}

void JZPianoWindow::MLength()
{
  mpPianoFrame->PressRadio(ID_CHANGE_LENGTH);
  mMouseEvent.SetLeftAction(MA_LENGTH);
}

void JZPianoWindow::MDialog()
{
  mpPianoFrame->PressRadio(ID_EVENT_DIALOG);
  mMouseEvent.SetLeftAction(MA_DIALOG);
}

void JZPianoWindow::MCutPaste()
{
  mpPianoFrame->PressRadio(ID_CUT_PASTE_EVENTS);
  mMouseEvent.SetLeftAction(MA_CUTPASTE);
}

void JZPianoWindow::Snap8()
{
  mPasteBuffer.Clear();
  SetSnapDenom(8);
}

void JZPianoWindow::Snap8D()
{
  mPasteBuffer.Clear();
  SetSnapDenom(12);
}

void JZPianoWindow::Snap16()
{
  mPasteBuffer.Clear();
  SetSnapDenom(16);
}

void JZPianoWindow::Snap16D()
{
  mPasteBuffer.Clear();
  SetSnapDenom(24);
}

void JZPianoWindow::Copy(JZTrack* pTrack, JZEvent* pEvent, int Kill)
{
  if (!pEvent)
  {
    return;
  }

  mpSong->NewUndoBuffer();
  mPasteBuffer.Clear();
  mPasteBuffer.Put(pEvent->Copy());

  if (pEvent->IsKeyOn())
  {
    paste_keys_aftertouch(pTrack, pEvent);
  }

  if (Kill)
  {
    tKeyOn *k = pEvent->IsKeyOn();
    if (k)
    {
      kill_keys_aftertouch(pTrack, pEvent);
      if (pTrack->GetAudioMode())
      {
        gpMidiPlayer->ListenAudio(k->Key, 0);
      }
      else
      {
        mListen.KeyOn(pTrack, k->Key, k->Channel, k->Veloc, k->Length);
      }
    }

    wxClientDC Dc(this);
    PrepareDC(Dc);
    DrawEvent(Dc, pEvent, wxWHITE_BRUSH, 0);
    pTrack->Kill(pEvent);
    pTrack->Cleanup();
  }

  if (mpGuitarFrame)
  {
    mpGuitarFrame->Update();
//    mpGuitarFrame->Redraw();
  }

  // SN++ Veloc- oder Aftertouch-Editor updaten
  if (mpCtrlEdit)
  {
    mpCtrlEdit->UpDate();
  }
}


void JZPianoWindow::Paste(JZTrack* pTrack, int Clock, int Pitch)
{
  if (mPasteBuffer.nEvents == 0)
  {
    int len = SnapClocks() - 4;
    if (len < 2)
    {
      len = 2;
    }
    tKeyOn* pEvent = new tKeyOn(0, 0, 64, 64, len);
    mPasteBuffer.Put(pEvent);
  }
  // SN++
  if (GetKeyOnEventCount() > 1)
  {
    // don't change Pitch
    Pitch = -1;
  }

  mpSong->NewUndoBuffer();
  tEventIterator Iterator(&mPasteBuffer);
  JZEvent* pEvent = Iterator.First();
  if (pEvent)
  {
    // SN++
    JZEvent *a = pEvent;
    while (a)
    {
      if (pEvent->IsChnPressure())
      {
        a = Iterator.Next();
        pEvent = a;
      }
      else
      {
        a = 0;
      }
    }

    int DeltaClock = Clock - pEvent->GetClock();
    int DeltaPitch = 0;
    if (Pitch >= 0)
    {
      DeltaPitch = Pitch - pEvent->GetPitch();
    }
    while (pEvent)
    {
      JZEvent *c = pEvent->Copy();
      c->SetPitch(c->GetPitch() + DeltaPitch);
      c->SetClock(c->GetClock() + DeltaClock);
      if (pTrack->ForceChannel && c->IsChannelEvent())
        c->IsChannelEvent()->Channel = pTrack->Channel - 1;
      tKeyOn *k = c->IsKeyOn();
      if (k)
      {
        if (pTrack->GetAudioMode())
        {
          gpMidiPlayer->ListenAudio(k->Key, 0);
        }
        else
        {
          mListen.KeyOn(pTrack, k->Key, k->Channel, k->Veloc, k->Length);
        }
      }
      wxClientDC Dc(this);
      PrepareDC(Dc);
      DrawEvent(Dc, c, c->GetBrush(), 0, 1);
      pTrack->Put(c);
      pEvent = Iterator.Next();
    }
    pTrack->Cleanup();
    // SN++ Veloc- oder Aftertouch-Editor updaten
    if (mpCtrlEdit)
    {
      mpCtrlEdit->UpDate();
    }
  }
}

int JZPianoWindow::GetKeyOnEventCount()
{
  int Count = 0;

  tEventIterator Iterator(&mPasteBuffer);
  JZEvent* pEvent = Iterator.First();
  while (pEvent)
  {
    if (pEvent->IsKeyOn())
    {
      ++Count;
    }
    pEvent = Iterator.Next();
  }
  return Count;
}

int JZPianoWindow::Channel()
{
  return mpTrack->Channel ? mpTrack->Channel - 1 : 0;
}

void JZPianoWindow::SnapDialog()
{
  tSnapDlg* pSnapDialog = new tSnapDlg(this, &mSnapDenomiator);
  pSnapDialog->Create();
}

void JZPianoWindow::SetVisibleAllTracks(bool Value)
{
  mVisibleAllTracks = Value;
  Refresh();
}

void JZPianoWindow::ActivateSettingsDialog()
{
  jppResourceDialog Dialog(this, "windowSettings");
  
  Dialog.Attach("use_colours", &mUseColors);
  Dialog.Attach("font_size", &mFontSize, mPianoFontSizes);

  if (Dialog.ShowModal() == wxID_OK)
  {
    Setup();
    SetScrollRanges(mCanvasX, mCanvasY);
    Refresh();
  }
}

// This is a test to see how to implement a dialog with Patrick's system.
// It replaces tMidiDelayDlg, which isnt necesarily a good idea.
void JZPianoWindow::ActivateMidiDelayDialog()
{
  if (!EventsSelected())
  {
    return;
  }

  int scale = 50; //in percent
  int clockDelay = 10;
  int repeat = 6;

  jppResourceDialog dialog(this, "midiDelay");
  
  dialog.Attach("scale", &scale);
  dialog.Attach("clockDelay", &clockDelay);
  dialog.Attach("repeat", &repeat);

  if (dialog.ShowModal() == wxID_OK)
  {
    //execute the command
    tCmdMidiDelay cmd(mpFilter, scale/100.0,clockDelay,repeat);
    cmd.Execute();
    SetScrollRanges(mCanvasX, mCanvasY);
    Refresh();
  }
}

void JZPianoWindow::ActivateSequenceLengthDialog()
{
  if (!EventsSelected())
  {
    return;
  }

  int scale = 100; //in percent

  jppResourceDialog dialog(this, "sequenceLength");
  
  dialog.Attach("scale", &scale);
  
  if (dialog.ShowModal() == wxID_OK)
  {
    //execute the command
    tCmdSeqLength cmd(mpFilter, (1.0*scale)/100.0);
    cmd.Execute();
    SetScrollRanges(mCanvasX, mCanvasY);
    Refresh();
  }
}

void JZPianoWindow::ActivateVelocityDialog()
{
  int FromValue = 64;
  int ToValue = 0;
  int modes[] =
  {
    8,  // set
    12,  // add
    16,  // subtract
    -1, // End of list
  };

  int Mode = modes[0];

  if (!EventsSelected())
  {
    return;
  }

  jppResourceDialog dialog(this, "velocity");
   dialog.Attach("start",&FromValue);
   dialog.Attach("stop",&ToValue);
   dialog.Attach("mode",&Mode,modes);

  if (dialog.ShowModal() == wxID_OK)
  {
    //execute the command
    tCmdVelocity cmd(mpFilter, FromValue, ToValue, Mode);
    cmd.Execute();
    SetScrollRanges(mCanvasX, mCanvasY);
    Refresh();
  }
}
