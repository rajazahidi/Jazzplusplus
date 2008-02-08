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
#include "PianoFrame.h"
#include "EventWindow.h"
#include "Song.h"
#include "Track.h"
//#include "jazz.h"

static const long wbar = 2;
static int bars_state = 2;  // from ArrayEdit

tCtrlEditBase::tCtrlEditBase(
  int min,
  int max,
  JZPianoFrame* p,
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
  JZPianoFrame* p,
  char const *label,
  int dx,
  int x,
  int y,
  int w,
  int h)
{
  x_off = dx;
  parent = p;
  track  = 0;
  from_clock = 0;
  to_clock = 1;
  i_max    = 1;
  clocks_per_pixel = 0;
  sticky = 1;

  panel = new tCtrlPanel(this, (wxWindow*)parent, x, y, dx, h, 0, "Controller Edit");
  //(void) new wxMessage(panel, (char *)label);
  //panel->NewLine();

  //__PORTING: changed the calls a bit so it would compile, need to remake the layout and do the event bindings

  ctrlmode = 0;  // edit seems stupid to me ...

  wxBoxSizer *topsizer = new wxBoxSizer( wxVERTICAL );

  if (!ctrlmode) {
    topsizer->Add(new wxButton(panel, -1,   "Apply")) ;
    //(wxFunction)Apply,
    topsizer->Add(new wxButton(panel, -1,  "Revert")) ;
    //(wxFunction)Revert,

    //(void)new wxButton(panel, (wxFunction)Bars,   "Bars");
  } else {
    topsizer->Add(new wxButton(panel, -1,  "Create"));  // create new events (wxFunction)Apply,
    topsizer->Add(new wxButton(panel, -1,    "Change"));  // change existing events (wxFunction)Edit,
    topsizer->Add(new wxButton(panel, -1, "Revert")); //(wxFunction)Revert,
    //(void)new wxButton(panel, (wxFunction)Bars,   "Bars");
  }
  ctrlmode = 0;
  // ab hier dient ctrlmode zur Unterscheidung zwischen
  // Apply und Edit.

  edit = new tArrayEdit((wxFrame *)parent, array, x+dx, y, w - dx, h, 0);
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
  if (!selectable) return;
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


void tCtrlEditBase::ReInit(tTrack *t, long fc, long cpp)
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
    /* PAT - The following ifdef was removed due to changes in gcc 3.x.  If it
       needs to be put back for compatibility purposes, it will need to return
       in an alternate form. */
    /*#ifdef FOR_MSW*/
    return array[(int)(i_max-1)];
  return array[(int)i];
  /*#else
    return array[i_max-1];
  return array[i];
  #endif*/
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
    JZEvent *e = iter.Range(0, from_clock);
    while (e)
    {
      if (IsCtrlEdit(e))
      {
        val = GetValue(e);
      }
      e = iter.Next();
    }
  }

  JZEvent *e = iter.Range(from_clock, to_clock);

  for (i = 0; i < array.Size(); i++)
    array[i] = val;

  i = 0;
  while (e)
  {
    if (IsCtrlEdit(e))
    {
      int k = Clock2i(e->Clock);
      if (sticky)
	while (i < k)
	  array[i++] = val;
      val = GetValue(e);
      array[k] = val;
    }
    e = iter.Next();
  }
  if (sticky && !selectable)
    while (i < array.Size())
      array[i++] = val;

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
  parent->Song->NewUndoBuffer();
  // delete old events, but skip clock 0 to preserve track defaults:
  // (dirty but might work...)
  tEventIterator iter(track);
  JZEvent *e = iter.Range(std::max(1L, from_clock), to_clock);
  int old_val = Missing();

// SN++ events nur im apply-mode loeschen!
  if (!ctrlmode)
  {
    while (e)
    {
      if (IsCtrlEdit(e))
        track->Kill(e);
      e = iter.Next();
    }

    // find any previous events
    if (sticky)
    {
      e = iter.Range(0, from_clock - 1);
      while (e)
      {
        if (IsCtrlEdit(e))
        {
          old_val = GetValue(e);
        }
        e = iter.Next();
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
        e = NewEvent(clock, new_val);
        track->Put(e);
        old_val = new_val;
      }
    }
  }
  else
  {
    // edit mode: Erzeugt keine neuen Events sondern aendert den Wert
    // bestehender Events.
    // SN++
    tControl *cpy;
    while (e)
    {
      if(IsCtrlEdit(e))
      {
        if (Clock2Val(e->Clock) != e->IsControl()->Value)
        {
          cpy = e->Copy()->IsControl();
          cpy->Value = Clock2Val(e->Clock);
          track->Kill(e);
          track->Put(cpy);
        }
      }
      e = iter.Next();
    }
  }

  // done
  track->Cleanup();
  wxEndBusyCursor();
  OnRevert();

  // SN+ Bug Fix Controller in Piano Fenster updaten.
  parent->Redraw();
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
  JZBarInfo BarInfo(parent->Song);
  BarInfo.SetClock(from_clock);
  long gclk,x;
  int  ii;
  if (bars_state > 0) {
    gclk = BarInfo.Clock;
    while (gclk < to_clock) {
      gclk = BarInfo.Clock;
      x = parent->Clock2x(gclk-from_clock);
      edit->DrawBarLine(dc, x - x_off);
      if (bars_state == 2)
        for (ii = 0; ii < BarInfo.CountsPerBar; ii++) {
          gclk += BarInfo.TicksPerBar / BarInfo.CountsPerBar;
          x = parent->Clock2x(gclk-from_clock);
          edit->DrawBarLine(dc, x - x_off);
        }
      BarInfo.Next();
    }
  }
}


// ------------------------------------------------------------------

tPitchEdit::tPitchEdit(
  JZPianoFrame* parent,
  char const *label,
  int xoff,
  int x,
  int y,
  int w,
  int h)
  : tCtrlEditBase(-8191, 8191, parent, label, xoff, x, y, w, h)
{
}

int tPitchEdit::Missing()
{
  return 0;
}

int tPitchEdit::IsCtrlEdit(JZEvent *e)
{
  return e->IsPitch() != 0;
}

int tPitchEdit::GetValue(JZEvent *e)
{
  return e->IsPitch()->Value;
}

JZEvent * tPitchEdit::NewEvent(long clock, int val)
{
  return new tPitch(clock, track->Channel - 1, val);
}

// ------------------------------------------------------------------

tCtrlEdit::tCtrlEdit(
  int CtrlNum,
  JZPianoFrame* parent,
  char const *label,
  int xoff,
  int x,
  int y,
  int w,
  int h)
  : tCtrlEditBase(0, 127, parent, label, xoff, x, y, w, h, 1)
{
  ctrl_num = CtrlNum;
  if (ctrl_num == 10)	// panpot
    array.SetNull(64);
}

int tCtrlEdit::Missing()
{

  if (ctrl_num == 10)
    return 64;
  return 0;
}

int tCtrlEdit::IsCtrlEdit(JZEvent *e)
{
  tControl *c = e->IsControl();
  return (c && c->Control == ctrl_num);
}

int tCtrlEdit::GetValue(JZEvent *e)
{
  return e->IsControl()->Value;
}

JZEvent * tCtrlEdit::NewEvent(long clock, int val)
{
  return new tControl(clock, track->Channel - 1, ctrl_num, val);
}

// ------------------------------------------------------------------

tVelocEdit::tVelocEdit(
  JZPianoFrame* parent,
  char const *label,
  int xoff,
  int x,
  int y,
  int w,
  int h)
  : tCtrlEditBase(1, 127, parent, label, xoff, x, y, w, h)
{
  sticky = 0;
  selectable = 1;
}

int tVelocEdit::Missing()
{
  return 1;
}

int tVelocEdit::IsCtrlEdit(JZEvent *e)
{
  // SN++ Falls im PianoWin Events selektiert sind, werden nur diese
  //      Events geaendert
  if (!parent->SnapSel->Selected)
  {
      return (e->IsKeyOn() != 0);
  }
  else
  {
    if (e->IsKeyOn())
    {
      return (
        parent->mpFilter->IsSelected(e) &&
        (e->Clock >= parent->mpFilter->FromClock &&
          e->Clock <= parent->mpFilter->ToClock));
    }
  }
  return 0;
}

int tVelocEdit::GetValue(JZEvent *e)
{
  return e->IsKeyOn()->Veloc;
}

void tVelocEdit::OnApply()
{
  static long from_clk, to_clk;

  wxBeginBusyCursor();
  parent->Song->NewUndoBuffer();

  tEventIterator iter(track);

  if (parent->SnapSel->Selected) {
    from_clk = parent->mpFilter->FromClock;
    to_clk   = parent->mpFilter->ToClock;
  } else {
    from_clk = from_clock;
    to_clk   = to_clock;
  }

  JZEvent *e = iter.Range(from_clk, to_clk);

  while (e) {
    // SN++ Falls im PianoWin Events selektiert sind, werden nur diese
    //      Events geaendert
    if (!parent->SnapSel->Selected || parent->mpFilter->IsSelected(e) )
  {

    tKeyOn *k = e->IsKeyOn();
    if (k)
    {
      tKeyOn *cpy = k->Copy()->IsKeyOn();

      int i = Clock2i(cpy->Clock);
      cpy->Veloc = array[i];
      track->Kill(k);
      track->Put(cpy);
    }
    }
    e = iter.Next();
  }
  track->Cleanup();
  wxEndBusyCursor();
  OnRevert();
  // SN+ fuer Color Darstellung
  parent->Redraw();
}

// ------------------------------------------------------------------

tPolyAfterEdit::tPolyAfterEdit(JZPianoFrame* parent, char const *label, int xoff, int x, int y, int w, int h)
  : tCtrlEditBase(0, 127, parent, label, xoff, x, y, w, h, 1)
{
  sticky = 0;  // SN must be set for proper editing!
  selectable = 1;
}


int tPolyAfterEdit::Missing()
{
  return 0;
}

int tPolyAfterEdit::IsCtrlEdit(JZEvent *e)
{
  // SN++ Falls im PianoWin Events selektiert sind, werden nur diese
  //      Events geaendert

  if (!parent->SnapSel->Selected)
  return e->IsKeyPressure() != 0;
  else
      if (e->IsKeyPressure()) {
        return( parent->mpFilter->IsSelected(e)  &&
                (e->Clock >= parent->mpFilter->FromClock &&
                 e->Clock <= parent->mpFilter->ToClock) );
      }
  return 0;
}

int tPolyAfterEdit::GetValue(JZEvent *e)
{
  if (e->IsKeyPressure())
  return e->IsKeyPressure()->Value;
  return -1;
}


void tPolyAfterEdit::OnApply()
{
  static long from_clk, to_clk;
  JZEvent *e;

  // SN++ Apply works only if some events are selected !!
  if (!parent->SnapSel->Selected) {
    OnRevert();
    return;
  }

  wxBeginBusyCursor();
  parent->Song->NewUndoBuffer();

  tEventIterator iter(track);

  if (parent->SnapSel->Selected) {
    from_clk = parent->mpFilter->FromClock;
    to_clk   = parent->mpFilter->ToClock;
  } else {
    from_clk = from_clock;
    to_clk   = to_clock;
  }
  tKeyPressure *k;
  tKeyOn      *keyon;

  if (!ctrlmode) { // OnApply
  // SN++ Alle selektierten AfterTouch events loeschen
    e = iter.Range(from_clk, to_clk);
    while (e) {
      if (!parent->SnapSel->Selected || parent->mpFilter->IsSelected(e) )
      {
	k = e->IsKeyPressure();
	if (k) track->Kill(k);
      }
      e = iter.Next();
    }
    // SN++ Neue Aftertouch's von KeyOn bis KeyLength einfuehgen;
    long key_end,key_clk;
    int  key_val = -1;
    int  key_cha;
    JZEvent *after;
    e = iter.Range(from_clk, to_clk);
    while (e) {
      if (!parent->SnapSel->Selected || parent->mpFilter->IsSelected(e) )
      {
	keyon = e->IsKeyOn();
	if (keyon) {
	  key_clk = keyon->Clock+1;
	  key_end = keyon->Clock + keyon->Length;
	  key_val = keyon->Key;
	  key_cha = keyon->Channel;
	}
	if (key_val>0)
	{
	  int i,temp=0;
	  for (long iclk=key_clk;iclk<key_end && iclk<to_clk;iclk +=8) {
	    i = Clock2i(iclk);
	    // SN++ Ein neues Event wird nur erzeut wenn sich der Wert aendert
	    //      und der Wert groesser als 0 ist.
	    if (array[i] > 0 && array[i] != temp) {
	    after = new tKeyPressure(iclk,key_cha,key_val,array[i]);
	    track->Put(after);
	      temp = array[i];
	    }
	  }
	  key_val = -1;
	}
      }
      e = iter.Next();
    }
  } else { // OnEdit
    // edit mode: Erzeugt keine neuen Events sondern aendert den Wert
    // bestehender Events.
    // SN++
    e = iter.Range(from_clk, to_clk);
    tKeyPressure *cpy;
    while (e)
    {
      if (!parent->SnapSel->Selected || parent->mpFilter->IsSelected(e))
      {
        if(e->IsKeyPressure())
	  if (Clock2Val(e->Clock) != e->IsKeyPressure()->Value) {
	    cpy = e->Copy()->IsKeyPressure();
	    cpy->Value = Clock2Val(e->Clock);
	    track->Kill(e);
      track->Put(cpy);
    }
      }
    e = iter.Next();
  }
  }

  track->Cleanup();
  wxEndBusyCursor();
  OnRevert();
  // SN+ fuer Color Darstellung
  parent->Redraw();
}

// ----------------------------------------------------------------------

tChannelAfterEdit::tChannelAfterEdit(
  JZPianoFrame* parent,
  char const *label,
  int xoff,
  int x,
  int y,
  int w,
  int h)
  : tCtrlEditBase(0, 127, parent, label, xoff, x, y, w, h, 1)
{
}


int tChannelAfterEdit::Missing()
{
  return 0;
}

int tChannelAfterEdit::IsCtrlEdit(JZEvent *e)
{
  return e->IsChnPressure() != 0;
}

int tChannelAfterEdit::GetValue(JZEvent *e)
{
  return e->IsChnPressure()->Value;
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
  parent->Song->NewUndoBuffer();
  // delete old events, but skip clock 0 to preserve track defaults:
  // (dirty but might work...)
  tEventIterator iter(track);
  JZEvent *e = iter.Range(std::max(1L, from_clock), to_clock);
  int old_val = Missing();

  // SN++ events nur im apply-mode loeschen!
  if (!ctrlmode)
  {
    while (e)
    {
      if (IsCtrlEdit(e))
        track->Kill(e);
      e = iter.Next();
    }

    // find any previous events
    if (sticky)
    {
      e = iter.Range(0, from_clock - 1);
      while (e)
	{
	  if (IsCtrlEdit(e))
	    {
	      old_val = GetValue(e);
	    }
	  e = iter.Next();
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
	    e = NewEvent(clock, new_val);
	    track->Put(e);
	    old_val = new_val;
	  }
      }
  } else {
    // edit mode: Erzeugt keine neuen Events sondern aendert den Wert
    // bestehender Events.
    // SN++
    tChnPressure *cpy;
    while (e) {
      if(IsCtrlEdit(e))
	if (Clock2Val(e->Clock) != GetValue(e)) {
           cpy = e->Copy()->IsChnPressure();
	   cpy->Value = Clock2Val(e->Clock);
           track->Kill(e);
           track->Put(cpy);
	}
      e=iter.Next();
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
  JZPianoFrame* parent,
  char const *label,
  int xoff,
  int x,
  int y,
  int w,
  int h)
  : tCtrlEditBase(min, max, parent, label, xoff, x, y, w, h)
{
}

int tTempoEdit::Missing()
{
  return track->GetDefaultSpeed();
}

int tTempoEdit::IsCtrlEdit(JZEvent *e)
{
  return e->IsSetTempo() != 0;
}

int tTempoEdit::GetValue(JZEvent *e)
{
  return e->IsSetTempo()->GetBPM();
}

JZEvent * tTempoEdit::NewEvent(long clock, int val)
{
  return new tSetTempo(clock, val);
}











