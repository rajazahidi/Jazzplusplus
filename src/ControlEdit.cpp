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

#include "ControlEdit.h"
#include "PianoWindow.h"
#include "EventWindow.h"
#include "Song.h"
#include "Track.h"

static const long wbar = 2;
static int bars_state = 2;  // from ArrayEdit

tCtrlEditBase::tCtrlEditBase(
  int min,
  int max,
  JZPianoWindow* p,
  char const *label,
  int dx,
  int x,
  int y,
  int w,
  int h,
  int ctrledit)
  : array((w-dx)/wbar, min, max)
{
  ctrlmode = ctrledit;
  selectable = 0;
  Create(p, label, dx, x, y, w, h);
}

void tCtrlEditBase::Create(
  JZPianoWindow* pPianoWindow,
  char const *label,
  int dx,
  int x,
  int y,
  int w,
  int h)
{
  x_off = dx;
  mpPianoWindow = pPianoWindow;
  track  = 0;
  from_clock = 0;
  to_clock = 1;
  i_max    = 1;
  clocks_per_pixel = 0;
  sticky = 1;

  panel = new tCtrlPanel(this, mpPianoWindow, x, y, dx, h, 0, "Controller Edit");
  //(void) new wxMessage(panel, (char *)label);
  //panel->NewLine();

  // PORTING: changed the calls a bit so it would compile, need to remake the layout and do the event bindings

  ctrlmode = 0;  // edit seems stupid to me ...

  wxBoxSizer *topsizer = new wxBoxSizer( wxVERTICAL );

  if (!ctrlmode)
  {
    topsizer->Add(new wxButton(panel, wxID_ANY, "Apply")) ;
    //(wxFunction)Apply,
    topsizer->Add(new wxButton(panel, wxID_ANY, "Revert")) ;
    //(wxFunction)Revert,

    //(void)new wxButton(panel, (wxFunction)Bars,   "Bars");
  }
  else
  {
    topsizer->Add(new wxButton(panel, wxID_ANY, "Create"));  // create new events (wxFunction)Apply,
    topsizer->Add(new wxButton(panel, wxID_ANY,"Change"));  // change existing events (wxFunction)Edit,
    topsizer->Add(new wxButton(panel, wxID_ANY, "Revert")); //(wxFunction)Revert,
    //(void)new wxButton(panel, (wxFunction)Bars,   "Bars");
  }
  ctrlmode = 0;
  // ab hier dient ctrlmode zur Unterscheidung zwischen
  // Apply und Edit.

  edit = new tArrayEdit((wxFrame *)mpPianoWindow, array, x+dx, y, w - dx, h, 0);
  edit->SetLabel(label);
  edit->SetDrawBars(this);

  panel->SetAutoLayout( TRUE );     // tell dialog to use sizer
  panel->SetSizer( topsizer );      // actually set the sizer

  topsizer->Fit( panel );            // set size to minimum size as calculated by the sizer
  topsizer->SetSizeHints( panel );   // set size hints to honour mininum size
}


tCtrlEditBase::~tCtrlEditBase()
{
  delete panel;
  delete edit;
}

// SN++
void tCtrlEditBase::UpDate()
{
  if (!selectable)
  {
    return;
  }
  OnRevert();
}
//

void tCtrlEditBase::SetSize(int dx, int x, int y, int w, int h)
{
  array.Resize((long)(w-dx) / wbar);
  // av-- edit->array_val.Resize((long)(w-dx) / wbar);
  panel->SetSize(x, y, dx, h);
  edit->SetSize(x+dx, y, w - dx, h);
  ReInit(track, from_clock, clocks_per_pixel);
}


void tCtrlEditBase::ReInit(JZTrack *t, long fc, long cpp)
{
  int w, h;
  edit->GetSize(&w, &h);
  track = t;
  from_clock = fc;
  to_clock = from_clock + (long)w * clocks_per_pixel;
  clocks_per_pixel = cpp;
  i_max = Clock2i(to_clock);
  OnRevert();

}

long tCtrlEditBase::Clock2i(long clock)
{
  return (clock - from_clock) / clocks_per_pixel / wbar;
}

long tCtrlEditBase::i2Clock(long i)
{
  return i * clocks_per_pixel * wbar + from_clock;
}

int tCtrlEditBase::Clock2Val(long clock)
{
  long i = Clock2i(clock);
  if (i >= i_max-1)
  {
    //* PAT - The following ifdef was removed due to changes in gcc 3.x.  If
    // it needs to be put back for compatibility purposes, it will need to
    // return in an alternate form.
//    #ifdef FOR_MSW
    return array[(int)(i_max - 1)];
  }
  return array[(int)i];
//#else
//  {
//    return array[i_max-1];
//  }
//  return array[i];
//#endif

#if 0
  long v1 = array[i];
  long v2 = array[i+1];
  long c1 = i2Clock(i);
  long c2 = i2Clock(i+1);
  int  val = (v2 - v1) * (clock - c1) / (c2 - c1) + v1;
  return val;
#endif
}

void tCtrlEditBase::OnRevert()
{
  int i;

  tEventIterator iter(track);
  int val = Missing();

  if (sticky && !selectable)
  {
    JZEvent* pEvent = iter.Range(0, from_clock);
    while (pEvent)
    {
      if (IsCtrlEdit(pEvent))
      {
        val = GetValue(pEvent);
      }
      pEvent = iter.Next();
    }
  }

  JZEvent* pEvent = iter.Range(from_clock, to_clock);

  for (i = 0; i < array.Size(); i++)
  {
    array[i] = val;
  }

  i = 0;
  while (pEvent)
  {
    if (IsCtrlEdit(pEvent))
    {
      int k = Clock2i(pEvent->GetClock());
      if (sticky)
      {
        while (i < k)
        {
          array[i++] = val;
        }
      }
      val = GetValue(pEvent);
      array[k] = val;
    }
    pEvent = iter.Next();
  }
  if (sticky && !selectable)
  {
    while (i < array.Size())
    {
      array[i++] = val;
    }
  }

  edit->Refresh();
}


/*void tCtrlEditBase::Revert(wxButton &but, wxCommandEvent& event)
{
  tCtrlPanel *panel = (tCtrlPanel *)but.GetParent();
  panel->edit->OnRevert();
}
*/


void tCtrlEditBase::OnApply()
{
  wxBeginBusyCursor();
  mpPianoWindow->GetSong()->NewUndoBuffer();
  // delete old events, but skip clock 0 to preserve track defaults:
  // (dirty but might work...)
  tEventIterator iter(track);
  JZEvent* pEvent = iter.Range(std::max(1L, from_clock), to_clock);
  int old_val = Missing();

// SN++ events nur im apply-mode loeschen!
  if (!ctrlmode)
  {
    while (pEvent)
    {
      if (IsCtrlEdit(pEvent))
      {
        track->Kill(pEvent);
      }
      pEvent = iter.Next();
    }

    // find any previous events
    if (sticky)
    {
      pEvent = iter.Range(0, from_clock - 1);
      while (pEvent)
      {
        if (IsCtrlEdit(pEvent))
        {
          old_val = GetValue(pEvent);
        }
        pEvent = iter.Next();
      }
    }

    // SN++ set-Mode
    // create new events
    long clock;
    for (clock = from_clock; clock < to_clock; clock++)
    {
      int new_val = Clock2Val(clock);

      if (old_val != new_val)
      {
        pEvent = NewEvent(clock, new_val);
        track->Put(pEvent);
        old_val = new_val;
      }
    }
  }
  else
  {
    // edit mode: Erzeugt keine neuen Events sondern aendert den Wert
    // bestehender Events.
    // SN++
    tControl* pControlCopy;
    while (pEvent)
    {
      if (IsCtrlEdit(pEvent))
      {
        if (Clock2Val(pEvent->GetClock()) != pEvent->IsControl()->mValue)
        {
          pControlCopy = pEvent->Copy()->IsControl();
          pControlCopy->mValue = Clock2Val(pEvent->GetClock());
          track->Kill(pEvent);
          track->Put(pControlCopy);
        }
      }
      pEvent = iter.Next();
    }
  }

  // done
  track->Cleanup();
  wxEndBusyCursor();
  OnRevert();

  // SN+ Bug Fix Controller in Piano Fenster updaten.
  mpPianoWindow->Refresh();
}

// SN++
void tCtrlEditBase::Bars(wxButton &but, wxCommandEvent& event)
{
  ((tCtrlPanel *)but.GetParent())->edit->OnBars();
}

void tCtrlEditBase::OnBars()
{
  // Bars und Werte updaten
  if (bars_state < 2)
  {
    bars_state++;
  }
  else
  {
    bars_state=0;
  }
  edit->Refresh();
}

/*void tCtrlEditBase::Apply(wxButton &but, wxCommandEvent& event)
{
  ((tCtrlPanel *)but.GetParent())->edit->OnApply();
}


void tCtrlEditBase::Edit(wxButton &but, wxCommandEvent& event)
{
  ((tCtrlPanel *)but.GetParent())->edit->OnEdit();
}
*/

void tCtrlEditBase::OnEdit()
{
  ctrlmode = 1;  // edit current events
  OnApply();
  ctrlmode = 0;
}

// SN++ Has 3 Modes (bars_state)  0: no Bars, 1,2: draw Bars
// av: called by tArrayEdit::OnPaint
void tCtrlEditBase::DrawBars(wxDC* dc)
{
  JZBarInfo BarInfo(*mpPianoWindow->GetSong());
  BarInfo.SetClock(from_clock);
  long gclk,x;
  int  ii;
  if (bars_state > 0)
  {
    gclk = BarInfo.GetClock();
    while (gclk < to_clock)
    {
      gclk = BarInfo.GetClock();
      x = mpPianoWindow->Clock2x(gclk-from_clock);
      edit->DrawBarLine(dc, x - x_off);
      if (bars_state == 2)
      {
        for (ii = 0; ii < BarInfo.GetCountsPerBar(); ++ii)
        {
          gclk += BarInfo.GetTicksPerBar() / BarInfo.GetCountsPerBar();
          x = mpPianoWindow->Clock2x(gclk-from_clock);
          edit->DrawBarLine(dc, x - x_off);
        }
      }
      BarInfo.Next();
    }
  }
}


// ------------------------------------------------------------------

tPitchEdit::tPitchEdit(
  JZPianoWindow* pPianoWindow,
  char const *label,
  int xoff,
  int x,
  int y,
  int w,
  int h)
  : tCtrlEditBase(-8191, 8191, pPianoWindow, label, xoff, x, y, w, h)
{
}

int tPitchEdit::Missing()
{
  return 0;
}

int tPitchEdit::IsCtrlEdit(JZEvent* pEvent)
{
  return pEvent->IsPitch() != 0;
}

int tPitchEdit::GetValue(JZEvent* pEvent)
{
  return pEvent->IsPitch()->Value;
}

JZEvent * tPitchEdit::NewEvent(long clock, int val)
{
  return new tPitch(clock, track->Channel - 1, val);
}

// ------------------------------------------------------------------

tCtrlEdit::tCtrlEdit(
  int CtrlNum,
  JZPianoWindow* pPianoWindow,
  char const *label,
  int xoff,
  int x,
  int y,
  int w,
  int h)
  : tCtrlEditBase(0, 127, pPianoWindow, label, xoff, x, y, w, h, 1)
{
  ctrl_num = CtrlNum;
  if (ctrl_num == 10)  // panpot
  {
    array.SetNull(64);
  }
}

int tCtrlEdit::Missing()
{
  if (ctrl_num == 10)
  {
    return 64;
  }
  return 0;
}

int tCtrlEdit::IsCtrlEdit(JZEvent* pEvent)
{
  tControl* pControl = pEvent->IsControl();
  return (pControl && pControl->mControl == ctrl_num);
}

int tCtrlEdit::GetValue(JZEvent* pEvent)
{
  return pEvent->IsControl()->mValue;
}

JZEvent * tCtrlEdit::NewEvent(long clock, int val)
{
  return new tControl(clock, track->Channel - 1, ctrl_num, val);
}

// ------------------------------------------------------------------

tVelocEdit::tVelocEdit(
  JZPianoWindow* pParent,
  char const *label,
  int xoff,
  int x,
  int y,
  int w,
  int h)
  : tCtrlEditBase(1, 127, pParent, label, xoff, x, y, w, h)
{
  sticky = 0;
  selectable = 1;
}

int tVelocEdit::Missing()
{
  return 1;
}

int tVelocEdit::IsCtrlEdit(JZEvent* pEvent)
{
  // SN++ Falls im PianoWin Events selektiert sind, werden nur diese
  //      Events geaendert
  if (!mpPianoWindow->mpSnapSel->IsSelected())
  {
    return (pEvent->IsKeyOn() != 0);
  }
  else
  {
    if (pEvent->IsKeyOn())
    {
      return (
        mpPianoWindow->GetFilter()->IsSelected(pEvent) &&
        (pEvent->GetClock() >= mpPianoWindow->GetFilter()->FromClock &&
          pEvent->GetClock() <= mpPianoWindow->GetFilter()->ToClock));
    }
  }
  return 0;
}

int tVelocEdit::GetValue(JZEvent* pEvent)
{
  return pEvent->IsKeyOn()->mVelocity;
}

void tVelocEdit::OnApply()
{
  static long from_clk, to_clk;

  wxBeginBusyCursor();
  mpPianoWindow->GetSong()->NewUndoBuffer();

  tEventIterator iter(track);

  if (mpPianoWindow->mpSnapSel->IsSelected())
  {
    from_clk = mpPianoWindow->GetFilter()->FromClock;
    to_clk   = mpPianoWindow->GetFilter()->ToClock;
  }
  else
  {
    from_clk = from_clock;
    to_clk   = to_clock;
  }

  JZEvent* pEvent = iter.Range(from_clk, to_clk);

  while (pEvent)
  {
    // SN++ Falls im PianoWin Events selektiert sind, werden nur diese
    //      Events geaendert
    if (
      !mpPianoWindow->mpSnapSel->IsSelected() ||
      mpPianoWindow->GetFilter()->IsSelected(pEvent))
    {

      tKeyOn* pKeyOn = pEvent->IsKeyOn();
      if (pKeyOn)
      {
        tKeyOn* pKeyOnCopy = pKeyOn->Copy()->IsKeyOn();

        int i = Clock2i(pKeyOnCopy->GetClock());
        pKeyOnCopy->mVelocity = array[i];
        track->Kill(pKeyOn);
        track->Put(pKeyOnCopy);
      }
    }
    pEvent = iter.Next();
  }
  track->Cleanup();
  wxEndBusyCursor();
  OnRevert();

  // SN+ for Color Darstellung
  mpPianoWindow->Refresh();
}

// ------------------------------------------------------------------

tPolyAfterEdit::tPolyAfterEdit(
  JZPianoWindow* pPianoWindow,
  char const *label,
  int xoff,
  int x,
  int y,
  int w,
  int h)
  : tCtrlEditBase(0, 127, pPianoWindow, label, xoff, x, y, w, h, 1)
{
  sticky = 0;  // SN must be set for proper editing!
  selectable = 1;
}


int tPolyAfterEdit::Missing()
{
  return 0;
}

int tPolyAfterEdit::IsCtrlEdit(JZEvent* pEvent)
{
  // SN++ Falls im PianoWin Events selektiert sind, werden nur diese
  //      Events geaendert

  if (!mpPianoWindow->mpSnapSel->IsSelected())
  {
    return pEvent->IsKeyPressure() != 0;
  }
  else
  {
    if (pEvent->IsKeyPressure())
    {
      return (
        mpPianoWindow->GetFilter()->IsSelected(pEvent) &&
        (pEvent->GetClock() >= mpPianoWindow->GetFilter()->FromClock &&
        pEvent->GetClock() <= mpPianoWindow->GetFilter()->ToClock));
    }
  }
  return 0;
}

int tPolyAfterEdit::GetValue(JZEvent* pEvent)
{
  if (pEvent->IsKeyPressure())
  {
    return pEvent->IsKeyPressure()->Value;
  }
  return -1;
}


void tPolyAfterEdit::OnApply()
{
  static long from_clk, to_clk;
  JZEvent* pEvent;

  // SN++ Apply works only if some events are selected !!
  if (!mpPianoWindow->mpSnapSel->IsSelected())
  {
    OnRevert();
    return;
  }

  wxBeginBusyCursor();
  mpPianoWindow->GetSong()->NewUndoBuffer();

  tEventIterator iter(track);

  if (mpPianoWindow->mpSnapSel->IsSelected())
  {
    from_clk = mpPianoWindow->GetFilter()->FromClock;
    to_clk   = mpPianoWindow->GetFilter()->ToClock;
  }
  else
  {
    from_clk = from_clock;
    to_clk   = to_clock;
  }
  tKeyPressure *k;
  tKeyOn      *keyon;

  if (!ctrlmode)
  {
    // OnApply

    // SN++ Alle selektierten AfterTouch events loeschen
    pEvent = iter.Range(from_clk, to_clk);
    while (pEvent)
    {
      if (
        !mpPianoWindow->mpSnapSel->IsSelected() ||
        mpPianoWindow->GetFilter()->IsSelected(pEvent))
      {
        k = pEvent->IsKeyPressure();
        if (k)
        {
          track->Kill(k);
        }
      }
      pEvent = iter.Next();
    }
    // SN++ Neue Aftertouch's von KeyOn bis KeyLength einfuehgen;
    long key_end(-1), key_clk(-1);
    int  key_val = -1;
    int  key_cha(-1);
    JZEvent *after;
    pEvent = iter.Range(from_clk, to_clk);
    while (pEvent)
    {
      if (
        !mpPianoWindow->mpSnapSel->IsSelected() ||
        mpPianoWindow->GetFilter()->IsSelected(pEvent))
      {
        keyon = pEvent->IsKeyOn();
        if (keyon)
        {
          key_clk = keyon->GetClock() + 1;
          key_end = keyon->GetClock() + keyon->mLength;
          key_val = keyon->mKey;
          key_cha = keyon->Channel;
        }
        if (key_val>0)
        {
          int i,temp=0;
          for (long iclk=key_clk;iclk<key_end && iclk<to_clk;iclk +=8)
          {
            i = Clock2i(iclk);

            // SN++ Ein neues Event wird nur erzeut wenn sich der Wert aendert
            //      und der Wert groesser als 0 ist.
            if (array[i] > 0 && array[i] != temp)
            {
              after = new tKeyPressure(iclk, key_cha, key_val, array[i]);
              track->Put(after);
              temp = array[i];
            }
          }
          key_val = -1;
        }
      }
      pEvent = iter.Next();
    }
  }
  else
  {
    // OnEdit
    // edit mode: Erzeugt keine neuen Events sondern aendert den Wert
    // bestehender Events.
    // SN++
    pEvent = iter.Range(from_clk, to_clk);
    tKeyPressure* pKeyPressureCopy;
    while (pEvent)
    {
      if (
        !mpPianoWindow->mpSnapSel->IsSelected() ||
        mpPianoWindow->GetFilter()->IsSelected(pEvent))
      {
        if (pEvent->IsKeyPressure())
        {
          if (Clock2Val(pEvent->GetClock()) != pEvent->IsKeyPressure()->Value)
          {
            pKeyPressureCopy = pEvent->Copy()->IsKeyPressure();
            pKeyPressureCopy->Value = Clock2Val(pEvent->GetClock());
            track->Kill(pEvent);
            track->Put(pKeyPressureCopy);
          }
        }
      }
      pEvent = iter.Next();
    }
  }

  track->Cleanup();
  wxEndBusyCursor();
  OnRevert();
  // SN+ for Color Darstellung
  mpPianoWindow->Refresh();
}

// ----------------------------------------------------------------------

tChannelAfterEdit::tChannelAfterEdit(
  JZPianoWindow* pPianoWindow,
  char const *label,
  int xoff,
  int x,
  int y,
  int w,
  int h)
  : tCtrlEditBase(0, 127, pPianoWindow, label, xoff, x, y, w, h, 1)
{
}


int tChannelAfterEdit::Missing()
{
  return 0;
}

int tChannelAfterEdit::IsCtrlEdit(JZEvent* pEvent)
{
  return pEvent->IsChnPressure() != 0;
}

int tChannelAfterEdit::GetValue(JZEvent* pEvent)
{
  return pEvent->IsChnPressure()->Value;
}


JZEvent *tChannelAfterEdit::NewEvent(long clock, int val)
{
  return new tChnPressure(clock, track->Channel - 1, val);
}

void tChannelAfterEdit::UpDate()
{
  OnRevert();
}


void tChannelAfterEdit::OnApply()
{
  wxBeginBusyCursor();
  mpPianoWindow->GetSong()->NewUndoBuffer();

  // delete old events, but skip clock 0 to preserve track defaults:
  // (dirty but might work...)
  tEventIterator iter(track);
  JZEvent* pEvent = iter.Range(std::max(1L, from_clock), to_clock);
  int old_val = Missing();

  // SN++ events nur im apply-mode loeschen!
  if (!ctrlmode)
  {
    while (pEvent)
    {
      if (IsCtrlEdit(pEvent))
      {
        track->Kill(pEvent);
      }
      pEvent = iter.Next();
    }

    // find any previous events
    if (sticky)
    {
      pEvent = iter.Range(0, from_clock - 1);
      while (pEvent)
      {
        if (IsCtrlEdit(pEvent))
        {
          old_val = GetValue(pEvent);
        }
        pEvent = iter.Next();
      }
    }

    // SN++ set-Mode
    // create new events
    long clock;
    for (clock = from_clock; clock < to_clock; clock += 8)
    {
      int new_val = Clock2Val(clock);

      if (old_val != new_val)
      {
        pEvent = NewEvent(clock, new_val);
        track->Put(pEvent);
        old_val = new_val;
      }
    }
  }
  else
  {
    // edit mode: Erzeugt keine neuen Events sondern aendert den Wert
    // bestehender Events.
    // SN++
    tChnPressure* pChnPressureCopy;
    while (pEvent)
    {
      if (IsCtrlEdit(pEvent))
      {
        if (Clock2Val(pEvent->GetClock()) != GetValue(pEvent))
        {
          pChnPressureCopy = pEvent->Copy()->IsChnPressure();
          pChnPressureCopy->Value = Clock2Val(pEvent->GetClock());
          track->Kill(pEvent);
          track->Put(pChnPressureCopy);
        }
      }
      pEvent = iter.Next();
    }
  }

  // done
  track->Cleanup();
  wxEndBusyCursor();
  OnRevert();
}



// ------------------------------------------------------------------

tTempoEdit::tTempoEdit(
  int min,
  int max,
  JZPianoWindow* pPianoWindow,
  char const *label,
  int xoff,
  int x,
  int y,
  int w,
  int h)
  : tCtrlEditBase(min, max, pPianoWindow, label, xoff, x, y, w, h)
{
}

int tTempoEdit::Missing()
{
  return track->GetDefaultSpeed();
}

int tTempoEdit::IsCtrlEdit(JZEvent* pEvent)
{
  return pEvent->IsSetTempo() != 0;
}

int tTempoEdit::GetValue(JZEvent* pEvent)
{
  return pEvent->IsSetTempo()->GetBPM();
}

JZEvent * tTempoEdit::NewEvent(long clock, int val)
{
  return new tSetTempo(clock, val);
}
