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

#include "Command.h"
#include "Song.h"
#include "Track.h"
#include "Filter.h"
#include "Random.h"
#include "Globals.h"

#include <cstdlib>
#include <limits>

using namespace std;

// ************************************************************************
// tSelectedEvents
// ************************************************************************

class tSelectedKeys : public tCommand
{
  public:
    long Keys[128];
  tSelectedKeys(tFilter *f);
  void ExecuteEvent(tTrack *t, JZEvent *e);
};

tSelectedKeys::tSelectedKeys(tFilter *f)
  : tCommand(f)
{
  int i;
  for (i = 0; i <  128; i++)
    Keys[i] = 0;
}

void tSelectedKeys::ExecuteEvent(tTrack *t, JZEvent *e)
{
  tKeyOn *k = e->IsKeyOn();
  if (k)
    Keys[ k->Key ] += k->Length;
}

// ************************************************************************
// tScale
// ************************************************************************

                            //  c     d     e  f     g    a      b
static const int CMajor[12] = { 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1 };

void tScale::Init(int ScaleNr, tFilter *f)
{
  int i;

  for (i = 0; i < 12; i++)
    ScaleKeys[i] = 0;

  if (ScaleNr == gScaleChromatic)
  {
    for (i = 0; i < 12; i++)
      ScaleKeys[i] = 1;
  }

  else if (ScaleNr == gScaleSelected)
  {
    int found = 0;
    tSelectedKeys cmd(f);
    cmd.Execute(0);
    for (i = 0; i < 128; i++)
    {
      if (cmd.Keys[i])
      {
        ScaleKeys[ i % 12 ] = 1;
        found = 1;
      }
    }
    if (!found)
    {
      ScaleKeys[0] = 1; // avoid loop in Member()
    }
  }

  else
  {
    for (i = 0; i < 12; i++)
      ScaleKeys[ (i + ScaleNr) % 12 ] = CMajor[i];
  }
}


int tScale::Analyze(tFilter *f)
{
  long keys[12];
  for (int i = 0; i < 12; i++)
  {
    keys[i] = 0;
  }

  tSelectedKeys cmd(f);
  cmd.Execute(0);
  for (int i = 0; i < 128; i++)
  {
    keys[i % 12] += cmd.Keys[i];
  }

  long Min = std::numeric_limits<long>::max();
  int ScaleIndex = 0;
  for (int TestScaleIndex = 0; TestScaleIndex < 12; ++TestScaleIndex)
  {
    tScale Scale;
    Scale.Init(TestScaleIndex);
    long Error = 0;
    for (int i = 0; i < 12; ++i)
    {
      if (Scale.ScaleKeys[i] == 0)
      {
        Error += keys[i];
      }
    }
    if (Error < Min)
    {
      ScaleIndex = TestScaleIndex;
      Min = Error;
    }
  }

  return ScaleIndex;
}


int tScale::Next(int Key)
{
  do
    ++ Key;
  while (!Member(Key));
  return Key;
}

int tScale::Prev(int Key)
{
  do
    -- Key;
  while (!Member(Key));
  return Key;
}

int tScale::Transpose(int Key, int Steps)
{
  int Offset = 0;

  while (!Member(Key))
  {
    ++ Key;
    ++ Offset;
  }

  while (Steps > 0)
  {
    Key = Next(Key);
    -- Steps;
  }
  while (Steps < 0)
  {
    Key = Prev(Key);
    ++ Steps;
  }
  return Key - Offset;
}


int tScale::FitInto(int Key)
{
  int Offset = 0;

  while (!Member(Key))
  {
    ++ Offset;
    if (Offset & 1)
      Key += Offset;
    else
      Key -= Offset;
  }
  return Key;
}

// ************************************************************************
// tCommand
// ************************************************************************

tCommand::tCommand(tFilter *f)
{
  Filter = f;
  Song = f->Song;
  Reverse = 0;
}

void tCommand::Execute(int NewUndo)
{
  wxBeginBusyCursor();
  if (NewUndo)
    Song->NewUndoBuffer();
  tTrackIterator Tracks(Filter, Reverse);
  tTrack *t = Tracks.First();
  while (t)
  {
    ExecuteTrack(t);
    t = Tracks.Next();
  }
  wxEndBusyCursor();
}


void tCommand::ExecuteTrack(tTrack *t)
{
  tEventIterator Iterator(t);
  JZEvent *e = Iterator.Range(Filter->FromClock, Filter->ToClock);
  while (e)
  {
    if (Filter->IsSelected(e))
      ExecuteEvent(t, e);
    e = Iterator.Next();
  }
  t->Cleanup();
}


void tCommand::ExecuteEvent(tTrack *t, JZEvent *e)
{
}

long tCommand::Interpolate(long clk, long vmin, long vmax)
{
  long cmin = Filter->FromClock;
  long cmax = Filter->ToClock;
  return (clk - cmin) * (vmax - vmin) / (cmax - cmin) + vmin;
}

// ***********************************************************************
// tCmdShift
// ***********************************************************************

tCmdShift::tCmdShift(tFilter *f, long dclk)
  : tCommand(f)
{
  DeltaClock = dclk;
}

void tCmdShift::ExecuteEvent(tTrack *t, JZEvent *e)
{
  JZEvent *c = e->Copy();
  t->Kill(e);
  c->SetClock(c->GetClock() + DeltaClock);
  t->Put(c);
}

// ************************************************************************
// tCmdErase
// ************************************************************************

tCmdErase::tCmdErase(tFilter *f, int lvsp)
  : tCommand(f)
{
  LeaveSpace = lvsp;
}

void tCmdErase::Execute(int NewUndo)
{
  tCommand::Execute(NewUndo);
  if (!LeaveSpace)
  {
    tFilter f(Filter);
    f.FromClock = Filter->ToClock;
    f.ToClock   = Song->GetLastClock() + 1;
    long DeltaClock = Filter->FromClock - Filter->ToClock;
    tCmdShift shift(&f, DeltaClock);
    shift.Execute(0);
  }
}

void tCmdErase::ExecuteEvent(tTrack *t, JZEvent *e)
{
  t->Kill(e);
}

// ************************************************************************
// tCmdQuantize
// ************************************************************************

tCmdQuantize::tCmdQuantize(tFilter *f, long clks, int grov, int dly)
  : tCommand(f)
{
  QntClocks = clks;
  Groove    = grov;
  Delay = dly;
  NoteStart = 1;
  NoteLength = 0;
}

long tCmdQuantize::Quantize(long Clock, int islen)
{
  Clock += QntClocks / 2;
  Clock -= Clock % QntClocks;
  if (!islen && (Clock % (2 * QntClocks) != 0))
    Clock += Groove;
  Clock += Delay;
  long minclk = islen ? 2 : 0;
  return Clock > minclk ? Clock : minclk;
}

void tCmdQuantize::ExecuteEvent(tTrack *t, JZEvent *e)
{
  tKeyOn *k;
  if ((k = e->IsKeyOn()) != 0)
  {
    k = (tKeyOn *)e->Copy();
    if (NoteStart)
    {
      k->SetClock(Quantize(k->GetClock(), 0));
    }
    if (NoteLength)
    {
      k->Length = Quantize(k->Length, 2);
    }
    t->Kill(e);
    t->Put(k);
  }
}

// ************************************************************************
// tCmdTranspose
// ************************************************************************

tCmdTranspose::tCmdTranspose(tFilter *f, int notes, int ScaleNr, int fit)
  : tCommand(f)
{
  Scale.Init(ScaleNr, Filter);
  Notes = notes;
  FitIntoScale = fit;
}

void tCmdTranspose::ExecuteEvent(tTrack *t, JZEvent *e)
{
  tKeyOn *k;
  if (e->IsKeyOn())
  {
    k = (tKeyOn *)e->Copy();
    if (FitIntoScale)
    {
      k->Key += Notes;
      k->Key = Scale.FitInto(k->Key);
    }
    else if (Notes)
      k->Key = Scale.Transpose(k->Key, Notes);
    t->Kill(e);
    t->Put(k);
  }
// SN++ Aftertouch
  tKeyPressure *a;
  if (e->IsKeyPressure())
  {
    a = (tKeyPressure *)e->Copy();
    if (FitIntoScale)
    {
      a->Key += Notes;
      a->Key = Scale.FitInto(a->Key);
    }
    else if (Notes)
      a->Key = Scale.Transpose(a->Key, Notes);
    t->Kill(e);
    t->Put(a);
  }
//
}

// ************************************************************************
// tCmdSetChannel
// ************************************************************************

tCmdSetChannel::tCmdSetChannel(tFilter *f, int chan)
  : tCommand(f)
{
  NewChannel = chan;
}

void tCmdSetChannel::ExecuteEvent(tTrack *t, JZEvent *e)
{
  tChannelEvent *c;

  if ((c = e->IsChannelEvent()) != 0)
  {
    c = (tChannelEvent *)e->Copy();
    c->Channel = NewChannel;
    t->Kill(e);
    t->Put(c);
  }
}

// ************************************************************************
// tCmdVelocity
// ************************************************************************

tCmdVelocity::tCmdVelocity(tFilter *f, int from, int to, int m)
  : tCommand(f)
{
  FromValue = from;
  ToValue  = to;
  Mode = m;
}

void tCmdVelocity::ExecuteEvent(tTrack *t, JZEvent *e)
{
  tKeyOn *k;

  if (e->IsKeyOn() != 0)
  {
    k = (tKeyOn *)e->Copy();
    long val = 0;
    if (ToValue <= 0)
      val = FromValue;
    else
      val = Interpolate(k->GetClock(), FromValue, ToValue);
    switch (Mode) {
      case 0: break;
      case 1: val = k->Veloc + val; break;
      case 2: val = k->Veloc - val; break;
    }
    k->Veloc = val < 1 ? 1 : (val > 127 ? 127 : val);
    t->Kill(e);
    t->Put(k);
  }
}

// ************************************************************************
// tCmdLength
// ************************************************************************

tCmdLength::tCmdLength(tFilter *f, int from, int to, int m)
  : tCommand(f)
{
  FromValue = from;
  ToValue  = to;
  Mode = m;
}

void tCmdLength::ExecuteEvent(tTrack *t, JZEvent *e)
{
  tKeyOn *k;

  if (e->IsKeyOn() != 0)
  {
    k = (tKeyOn *)e->Copy();
    long val = 0;
    if (ToValue <= 0)
      val = FromValue;
    else
      val = Interpolate(k->GetClock(), FromValue, ToValue);
    switch (Mode) {
      case 0: break;
      case 1: val = k->Length + val; break;
      case 2: val = k->Length - val; break;
    }

    k->Length = val < 1 ? 1 : val;
    t->Kill(e);
    t->Put(k);
  }
}



// ************************************************************************
// tCmdSeqLength
//    JAVE this command is supposed to stretch/contract a sequence of events in time
//   by factor "scale" from starting point "startClock"
// ************************************************************************

tCmdSeqLength::tCmdSeqLength(tFilter *f, double scale)
  : tCommand(f)
{
  this->scale=scale;
  this->startClock=-1000;
}

/** move an event according to startclock and scale
 */
void tCmdSeqLength::ExecuteEvent(tTrack *t, JZEvent *e)
{
  //make a copy of the current event 
  JZEvent *k;
  k = (tKeyOn *)e->Copy();

  //little hack, if clock is -1000 it means set startclock from the first event.
  if(startClock==-1000){
    startClock=k->GetClock();
  }

  //calculate the new start clock, move the new event and kill the old one
  k->SetClock((long int)(((k->GetClock()-startClock)*scale) + startClock ) );
  t->Kill(e);
  t->Put(k);
  
}



// ************************************************************************
// tCmdConvertToModulation
//    JAVE this command is supposed convert a midi note sequence
// to a pitch bend/volume control sequence instead
// ************************************************************************

tCmdConvertToModulation::tCmdConvertToModulation(tFilter *f)
  : tCommand(f)
{
}

//need to override executetrack, since we have begin/end behaviour in this filter

void tCmdConvertToModulation::ExecuteTrack(tTrack *t)
{
  //JAVE:iterate over all events, make a long event from start until stop of the sequence,
  //convert all note-on messages to a pitch bend/volume controller pair, velocity -> volume
  //make a volume off controller at the end of the current event
  tEventIterator Iterator(t);
  JZEvent *e = Iterator.Range(Filter->FromClock, Filter->ToClock);
  long startclock=-1;
  long endclock=-1;
  unsigned char channel=0;
  unsigned short lastlength=0;
  long previouspitch=0;
  long pitchdiff=0;

  //general midi recommends these values for half-note bends
  long pitchsteparray[] =
  {
    0x0000,
    0x07ff,
    0x1000,
    0x17ff,
    0x2000,
    0x27ff,
    0x3000,
    0x37ff,
    0x3fff
  };
  
  long startvelocity=0;
  long startkey=0;
  while (e)
  {
    if (Filter->IsSelected(e) && e->IsKeyOn())
      {
        if (startclock == -1)
        {
          startclock=e->IsKeyOn()->GetClock();
          startvelocity=e->IsKeyOn()->Veloc;
          channel=e->IsKeyOn()->Channel;
          startkey=e->IsKeyOn()->Key;
          previouspitch=e->GetPitch();
        }
        pitchdiff=e->GetPitch()-previouspitch;

        tPitch* pitchmodulation=0;
        pitchmodulation = new tPitch(e->GetClock(), channel, pitchsteparray[pitchdiff+4]);

        t->Put(pitchmodulation);


        t->Kill(e); //remove the old event

        t->Put(new tControl(e->GetClock(), channel, 0x07, e->IsKeyOn()->Veloc));
        t->Put(new tControl(e->GetClock()+e->IsKeyOn()->Length, channel, 0x07,0));

        lastlength=e->IsKeyOn()->Length;
        endclock=e->GetClock();
        previouspitch=e->GetPitch();
      }
      //ExecuteEvent(t, e);
    e = Iterator.Next();
  }
  //now insert the new long event
  tKeyOn* longevent = new tKeyOn(startclock, channel, startkey, startvelocity, endclock-startclock+lastlength );
  t->Put(longevent);
  t->Cleanup();
}

// ************************************************************************
// tMidiDelayDlg
//    JAVE this is a simple midi delay line
// ************************************************************************

tCmdMidiDelay::tCmdMidiDelay(tFilter *f, double scale,  long clockDelay, int repeat)
  : tCommand(f)
{
  this->scale=scale;
  this->clockDelay=clockDelay;
  this->repeat=repeat;
}

void tCmdMidiDelay::ExecuteEvent(tTrack *t, JZEvent *e)
{
  tKeyOn *k;

  for(int i=1; i< repeat; i++)
  {
    if(e->IsKeyOn()){ //only echo note events
      k = (tKeyOn *)e->Copy();
      k->SetClock(k->GetClock()+ clockDelay*i  );
        k->Veloc=(unsigned char)(pow(scale,i)*k->Veloc);
        t->Put(k);
      }
    }
  
}



// ************************************************************************
// tCmdCleanup
// ************************************************************************

tCmdCleanup::tCmdCleanup(tFilter *f, long clks, int so)
  : tCommand(f)
{
  lengthLimit = clks;
  shortenOverlaps = so;
}

void tCmdCleanup::ExecuteTrack(tTrack *t)
{
  memset(prev_note, 0, sizeof(prev_note));
  tCommand::ExecuteTrack(t);
}

void tCmdCleanup::ExecuteEvent(tTrack *t, JZEvent *e)
{
  tKeyOn *k;
  if ((k = e->IsKeyOn()) != 0)
  {
    if (k->Length < lengthLimit) {
      // remove short notes
      t->Kill(e);
    }
    else if (shortenOverlaps) {
      // shorten length of overlapping notes
      tKeyOn *p = prev_note[k->Channel][k->Key];
      if (p && p->GetClock() + p->Length >= k->GetClock())
      {
        p->Length = k->GetClock() - p->GetClock() - 1;
        if (p->Length < lengthLimit)
        {
          t->Kill(p);
        }
      }
      prev_note[k->Channel][k->Key] = k;
    }
  }
}

// ************************************************************************
// tCmdSearchReplace
// ************************************************************************

tCmdSearchReplace::tCmdSearchReplace(tFilter *f, short sf, short st)
  : tCommand(f)
{
  fr = sf;
  to = st;
}

void tCmdSearchReplace::ExecuteEvent(tTrack *t, JZEvent *e)
{
  tControl *ctrl;
  if ((ctrl = e->IsControl()) != 0)
  {
    if (ctrl->Control == fr)
    {
      tControl *copy = (tControl *)ctrl->Copy();
      copy->Control = to;
      t->Kill(ctrl);
      t->Put(copy);
    }
  }
}

// ************************************************************************
// tCmdCopyToBuffer
// ************************************************************************

tCmdCopyToBuffer::tCmdCopyToBuffer(tFilter *f, tEventArray *buf)
  : tCommand(f)
{
  Buffer = buf;
}

void tCmdCopyToBuffer::ExecuteEvent(tTrack *t, JZEvent *e)
{
  Buffer->Put(e->Copy());
}

// **********************************************************************
// tCmdCopy
// **********************************************************************



tCmdCopy::tCmdCopy(tFilter *f, long dt, long dc)
  : tCommand(f)
{
  DestTrack = dt;
  DestClock = dc;

  EraseSource = 0;        // no
  EraseDestin = 1;        // yes
  InsertSpace = 0;        // no
  RepeatClock = -1;        // -1L

  Reverse = DestTrack > Filter->FromTrack;
  if (Reverse)
    DestTrack += Filter->ToTrack - Filter->FromTrack; // ToTrack inclusive
}



void tCmdCopy::ExecuteTrack(tTrack *s)
{
  long StartClock, StopClock;
  tTrack *d;

  StartClock = DestClock;

  if (RepeatClock < 0)
    StopClock = StartClock + Filter->ToClock - Filter->FromClock;
  else
    StopClock = RepeatClock;

  d = Song->GetTrack(DestTrack);

  if (Reverse)
  {
    if (DestTrack)
      -- DestTrack;
  }
  else
    ++ DestTrack;

  if (s && d)
  {

    // Events nach tmp kopieren
    tEventArray tmp;
    {
      tEventIterator Iterator(s);
      long  DeltaClock = StartClock - Filter->FromClock;
      JZEvent *e = Iterator.Range(Filter->FromClock, Filter->ToClock);
      while (e)
      {
        long NewClock = e->GetClock() + DeltaClock;
        if (NewClock >= StopClock)
          break;

        if (Filter->IsSelected(e))
        {
          JZEvent* cpy = e->Copy();
          cpy->SetClock(NewClock);
          tmp.Put(cpy);
        }

        e = Iterator.Next();
        if (!e)
        {
          e = Iterator.First();
          DeltaClock += Filter->ToClock - Filter->FromClock;
        }
      }
    }

    // ggf Freien Platz einfuegen

    if (InsertSpace && d->GetLastClock() > StartClock)
    {
      tEventIterator Iterator(d);
      JZEvent *e = Iterator.Range(StartClock, d->GetLastClock() + 1);
      long DeltaClock = StopClock - StartClock;
      while (e)
      {
        if (Filter->IsSelected(e))
        {
          JZEvent *c = e->Copy();
          c->SetClock(c->GetClock() + DeltaClock);
          d->Kill(e);
          d->Put(c);
        }
        e = Iterator.Next();
      }
      d->Cleanup();
    }

    // ggf Quelle loeschen

    if (EraseSource)
    {
      tEventIterator Iterator(s);
      JZEvent *e = Iterator.Range(Filter->FromClock, Filter->ToClock);
      while (e)
      {
        if (Filter->IsSelected(e))
          s->Kill(e);
        e = Iterator.Next();
      }
      s->Cleanup();
    }

    // ggf Ziel loeschen

    if (EraseDestin)
    {
      tEventIterator Iterator(d);
      JZEvent *e = Iterator.Range(StartClock, StopClock);
      while (e)
      {
        if (Filter->IsSelected(e))
          d->Kill(e);
        e = Iterator.Next();
      }
      d->Cleanup();
    }

    // tmp und Zieltrack zusammenmischen, aufraeumen

    d->Merge(&tmp);
    d->Cleanup();
  }
}

// ************************************************************************
// tCmdExchLeftRight
// ************************************************************************

tCmdExchLeftRight::tCmdExchLeftRight(tFilter *f)
  : tCommand(f)
{
}

void tCmdExchLeftRight::ExecuteEvent(tTrack *t, JZEvent *e)
{
  if (e->IsKeyOn())
  {
    tKeyOn *k = (tKeyOn *)e->Copy();
    k->SetClock(Filter->FromClock + Filter->ToClock - k->GetClock());
    t->Kill(e);
    t->Put(k);
  }
}


// ************************************************************************
// tCmdExchUpDown
// ************************************************************************

tCmdExchUpDown::tCmdExchUpDown(tFilter *f)
  : tCommand(f)
{
}

void tCmdExchUpDown::ExecuteTrack(tTrack *t)
{
  int i;
  int Keys[128];
  JZEvent *e;

  // find all Key's selected

  for (i = 0; i < 128; i++)
    Keys[i] = 0;

  tEventIterator Iterator(t);
  e = Iterator.Range(Filter->FromClock, Filter->ToClock);
  while (e)
  {
    if (Filter->IsSelected(e) && e->IsKeyOn())
    {
      tKeyOn *k = (tKeyOn *)e;
      Keys[k->Key] = 1;
    }
    e = Iterator.Next();
  }

  // reverse Key's

  e = Iterator.Range(Filter->FromClock, Filter->ToClock);
  while(e)
  {
    if (Filter->IsSelected(e) && e->IsKeyOn())
    {
      tKeyOn *k = (tKeyOn *)e->Copy();
      int n_th = 0;
      // the n'th key from bottom ..
      for (i = 0; i <= k->Key; i++)
        n_th += Keys[i];
      // .. becomes the n'th key from top
      for (i = 127; i > 0 && n_th; --i)
        n_th -= Keys[i];
      k->Key = i + 1;

      t->Kill(e);
      t->Put(k);
    }
    e = Iterator.Next();
  }
  t->Cleanup();
}

// ************************************************************************
// tCmdMapper
// ************************************************************************

//enum prop { veloc, length, key, rhythm, random, pan, modul, cc1, cc2, pitch, clock };
tCmdMapper::tCmdMapper(tFilter *f, prop src, prop dst, JZRndArray &arr, int nb, int ad)
  : tCommand(f),
    array(arr)
{
  n_bars          = nb;
  source          = src;
  destin          = dst;
  add             = ad;
  binfo           = new JZBarInfo(Song);
  binfo->SetClock(Filter->FromClock);
  start_bar       = binfo->BarNr;
}

tCmdMapper::~tCmdMapper()
{
  delete binfo;
}

void tCmdMapper::ExecuteEvent(tTrack *t, JZEvent *e)
{
  tKeyOn *k = e->IsKeyOn();
  if (k)
  {
    int sval = 0;
    switch (source)
    {
      case veloc:
        sval = array[(int)k->Veloc];
        break;

      case length:
        sval = array[(int)k->Length];
        break;

      case key:
        sval = array[(int)k->Key];
        break;

      case rhythm: {
        binfo->SetClock(k->GetClock());
//        long sng_tpb = binfo->TicksPerBar;
        long arr_tpb = array.Size() / n_bars;
        long arr_bar = (binfo->BarNr - start_bar) % n_bars;
        long i = arr_tpb * arr_bar + arr_tpb * (k->GetClock() - binfo->Clock) / binfo->TicksPerBar;
// printf("sng_tpb %ld, arr_tpb %ld, k->GetClock() %ld, binfo->Clock %ld,\n arr.Size() %ld, n_bars %ld, i %ld\n",
//  sng_tpb, arr_tpb, k->GetClock(), binfo->Clock, array.Size(), n_bars, i);
// fflush(stdout);
        sval = array[(int)i];
      }
      break;

      case random:
        sval = array.Random();
        if (add)
          sval -= array.Size()/2;
        break;

      default:
        break;
    }

    switch (destin)
    {
      case veloc: {
        if (add)
          sval = k->Veloc + sval;
        if (sval > 127)
          sval = 127;
        if (sval < 1)
          sval = 1;
        tKeyOn *c = (tKeyOn *)k->Copy();
        t->Kill(k);
        c->Veloc = sval;
        t->Put(c);
      }
      break;

      case key: {
        if (add)
          sval = k->Key + sval;
        if (sval > 127)
          sval = 127;
        if (sval < 1)
          sval = 1;
        tKeyOn *c = (tKeyOn *)k->Copy();
        t->Kill(k);
        c->Key = sval;
        t->Put(c);
      }
      break;

      case length: {
        if (add)
          sval = k->Length + sval;
        if (sval < 1)
          sval = 1;
        tKeyOn *c = (tKeyOn *)k->Copy();
        t->Kill(k);
        c->Length = sval;
        t->Put(c);
      }
      break;

      case clock: {
        tKeyOn *c = (tKeyOn *)k->Copy();
        c->SetClock(c->GetClock() + sval);
        if (c->GetClock() < 0)
        {
          c->SetClock(0);
        }
        t->Kill(k);
        t->Put(c);
      }
      break;

      case pan:
      case modul:
      case cc1:
      case cc2:
      case pitch:
      default:
        break;
    }
  }
}

