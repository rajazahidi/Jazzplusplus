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

#include "Synth.h"
#include "Configuration.h"
#include "Song.h"
#include "Globals.h"
//#include "Command.h"
//#include "Audio.h"
#include "Metronome.h"


JZSong::~JZSong()
{
  int i;
  Clear();
  for (i = 0; i < MaxTracks; i++)
  {
    delete Tracks[i];
  }
}

JZSong::JZSong()
{
  int i;
  nTracks = MaxTracks;
  TicksPerQuarter = 120;
  intro_length = 0;
  for (i = 0; i < MaxTracks; i++)
    Tracks[i] = new tTrack();
  MaxQuarters = 100 * 4;  // start with 100 bars ...
}

void JZSong::Clear()
{
  for (int i = 0; i < nTracks; i++)
    Tracks[i]->Clear();
  nTracks = MaxTracks;
}

int JZSong::Speed()
{
  return Tracks[0]->GetDefaultSpeed();
}

void JZSong::Read(tReadBase &io, const char *fname)
{
  int i;
  wxBeginBusyCursor();
  for (i = 0; i < MaxTracks; i++) {
    //    cout << Tracks<<"\n";
    Tracks[i]->Clear();
  }
  int n = io.Open(fname);
  for (i = 0; i < n && i < MaxTracks; i++) {
    Tracks[i]->Read(io);
  }
  io.Close();
  TicksPerQuarter = io.TicksPerQuarter;

  if (TicksPerQuarter < 48)
  {
    SetTicksPerQuarter(48);
  }
  else if (TicksPerQuarter > 192)
  {
    SetTicksPerQuarter(192);
  }

  // adjust song length = midi length + 16 bars
  long newlen = GetLastClock() / TicksPerQuarter + 16 * 4;
  if (newlen > MaxQuarters)
    MaxQuarters = newlen;

  wxEndBusyCursor();
}


void JZSong::Write(tWriteBase &io, const char *fname)
{
  // Make sure track 0 has a synth reset
  if (!Tracks[0]->Reset)
  {
    Tracks[0]->Reset = gpSynth->Reset()->IsSysEx();
  }

  int n = NumUsedTracks();
  if (!io.Open(fname, n, TicksPerQuarter))
  {
    return;
  }

  wxBeginBusyCursor();
  for (int i = 0; i < n; i++)
    Tracks[i]->Write(io);
  io.Close();
  wxEndBusyCursor();
}


tTrack *JZSong::GetTrack(int Nr)
{
  if (Nr >= 0 && Nr < nTracks)
    return Tracks[Nr];
  return 0;
}


long JZSong::GetLastClock()
{
  long max = 0;
  for (int i = 0; i < nTracks; i++)
  {
    long clk = Tracks[i]->GetLastClock();
    if (clk > max)
      max = clk;
  }
  return max;
}


void JZSong::Clock2String(long clk, char *buf)
{
  JZBarInfo b(this);
  b.SetClock(clk);
  clk -= b.Clock;
  long TicksPerCount = b.TicksPerBar / b.CountsPerBar;
  int Count = clk / TicksPerCount;
  sprintf(buf, "%3d:%d:%03ld", b.BarNr + 1 - intro_length, Count + 1, clk % TicksPerCount);
}

long JZSong::String2Clock(const char *buf)
{
  int bar = 1;
  long clk = 0;
  int  cnt = 1;
  sscanf(buf, "%d:%d:%ld", &bar, &cnt, &clk);
  -- bar;
  -- cnt;
  bar += intro_length;  // buf is from user input!
  JZBarInfo b(this);
  b.SetBar(bar);
  long TicksPerCount = b.TicksPerBar / b.CountsPerBar;
  return b.Clock + cnt * TicksPerCount + clk;
}


void JZSong::MergeTracks(
  long FrClock,
  long ToClock,
  tEventArray *Destin,
  tMetronomeInfo *MetronomeInfo,
  long delta,
  int mode)
{
  int i;

  // Make metronome
  if (MetronomeInfo->IsOn)
    MakeMetronome( FrClock, ToClock, Destin, MetronomeInfo, delta );

  // Find Solo-Tracks
  int solo = 0;
  for (i = 0; i < nTracks; i++)
  {
    if (Tracks[i]->State == tsSolo)
    {
      solo = 1;
      break;
    }
  }

  for (i = 0; i < nTracks; i++)
  {
    tTrack *t = Tracks[i];
    if (t->State == tsSolo || (!solo && t->State == tsPlay))
    {
      if (t->GetAudioMode() != mode)
        continue;

      tEventIterator Iterator(Tracks[i]);
      JZEvent *e = Iterator.Range(FrClock, ToClock);
      while (e)
      {
        JZEvent *c = e->Copy();
        c->SetClock(c->GetClock() + delta);
        c->SetDevice(t->GetDevice());
        Destin->Put(c);

        if(c->IsPlayTrack())
        {
          MergePlayTrackEvent(c->IsPlayTrack(), Destin, 0);
        }

        e = Iterator.Next();
      }
    }
  }
}

//should call recursively, so playtrack events will resolve playtrack events
void JZSong::MergePlayTrackEvent(
  tPlayTrack* c, //the playtrack event
  tEventArray *Destin,
  int recursionDepth)
{
  //recursion might be simple, but we have the infinite loop problem, if a playtrack point to itself, either directly or indirectly
  //we probaly need to keep a list of seen tracks, simpler is to limit the recursion level to 100 or something.
  //the actual recursion is done later in the loop
  recursionDepth++;
  if(recursionDepth>100) //yes yes, you should use symbolics...
    return;
  fprintf(stderr, "playtrack %d\n",c->track);
  tTrack *t=Tracks[c->track];//the track we want to play
  tEventIterator IteratorPL(t); //get an iterator of all events the playtrack is pointing to 
  JZEvent *f;

  //FIXME this is just to test the idea, it would be good to modify getlastclock instead i think
  //find an EOT event, otherwise default to the last clock(should be + length of the last event as well)
  long loopLength=0;
  tEventIterator IteratorEOT(t); //get an iterator of all events the playtrack is pointing to 
  f = IteratorEOT.Range(0, t->GetLastClock());
  loopLength=t->GetLastClock(); 
  while(f)
  {
    if (f->IsEndOfTrack())
    {
      loopLength = t->GetLastClock();
    }
    f = IteratorEOT.Next();
  }

  // looplength will be used to loop the track, for the duration of
  // the playtrack event.
  long loopOffset=0;

  //   The loop below is supposed to repeat the referenced track for the
  // duration of the playtrack event.  Also, we should look out for if we
  // start playing in the middle of an playtrack event, curently it wont
  // play anything.
  //   To fix this we could search all unmuted tracks from the beginning to
  // the start of the loop-point, and see if there are any playtrack events
  // which start + length end up after the startloop.  Then we could move the
  // start of those to the beginning of the loop, and shorten the length.
  // There would still be a problem with playtracks not even bar length.
  while(loopOffset < c->eventlength)
  { 
    f = IteratorPL.Range(0, (c->eventlength-loopOffset));  //no more events then the length of the playtrack! and ensure last iteration is no longer than what is left 
    while (f)
    {
      JZEvent *d = f->Copy();
      d->SetClock(d->GetClock() + c->GetClock() + loopOffset);
      if(d->IsKeyOn())
      {
        d->IsKeyOn()->Key += c->transpose;
      }
      if(d->IsPlayTrack())
      {
        MergePlayTrackEvent(d->IsPlayTrack(), Destin, recursionDepth);
      }
      d->SetDevice(t->GetDevice());
      Destin->Put(d);
      f = IteratorPL.Next();
    }
    loopOffset+=loopLength;
  }
}


void JZSong::MakeMetronome(
  long FrClock,
  long ToClock,
  tEventArray *Destin,
  tMetronomeInfo *MetronomeInfo,
  long delta)
{
  JZBarInfo BarInfo( this );
  BarInfo.SetClock(FrClock);
  long clk = BarInfo.Clock;
  int count = 1;

  while (clk < FrClock)
  {
    clk += BarInfo.TicksPerBar / BarInfo.CountsPerBar;
    count++;
  }

  while (clk < ToClock)
  {
    if (count > BarInfo.CountsPerBar)
    {
      BarInfo.Next();
      clk = BarInfo.Clock;
      count = 1;
    }

    // Insert normal click always
    Destin->Put( MetronomeInfo->Normal(clk + delta) );

    //  On a bar?
    if ( (count == 1) && (MetronomeInfo->IsAccented) )
    {
      // Insert accented click also
      Destin->Put( MetronomeInfo->Accented(clk + delta) );
    }

    clk += BarInfo.TicksPerBar / BarInfo.CountsPerBar;
    count++;
  }
}


// ********************************************************************
// BarInfo
// *******************************************************************


JZBarInfo::JZBarInfo(JZSong *Song)
  : Iterator(Song->Tracks[0])
{
  BarNr = 0;
  Clock = 0;
  TicksPerQuarter = Song->TicksPerQuarter;
  CountsPerBar = 4;
  TicksPerBar = TicksPerQuarter * 4;
}


void JZBarInfo::SetBar(int barnr)
{
  if (barnr < 0)  // avoid infinite loop
    barnr = 0;

  BarNr = barnr;
  Clock = 0;
  TicksPerBar = TicksPerQuarter * 4;
  e = Iterator.First();
  while (1)
  {
    // Events bis Taktanfang nach MeterChange durchsuchen
    // Meter-Event vor oder genau auf Taktanfang stehen
    while (e && e->GetClock() <= Clock)
    {
      e->BarInfo(TicksPerBar, CountsPerBar, TicksPerQuarter);
      e = Iterator.Next();
    }

    if (!barnr)
      return;

    // Clock + Bar auf Anfang naechster Takt
    -- barnr;
    Clock += TicksPerBar;
  }
}


void JZBarInfo::SetClock(long clock)
{
  BarNr = 0;
  Clock = 0;
  TicksPerBar = TicksPerQuarter * 4;
  e = Iterator.First();
  while (1)
  {
    while (e && e->GetClock() <= Clock)
    {
      e->BarInfo(TicksPerBar, CountsPerBar, TicksPerQuarter);
      e = Iterator.Next();
    }

    if (Clock + TicksPerBar > clock)
      return;

    // Clock + Bar auf Anfang naechster Takt
    Clock += TicksPerBar;
    ++ BarNr;
  }
}


void JZBarInfo::Next()
{
  ++ BarNr;
  Clock += TicksPerBar;

  while (e && e->GetClock() <= Clock)
  {
    e->BarInfo(TicksPerBar, CountsPerBar, TicksPerQuarter);
    e = Iterator.Next();
  }
}


// ***********************************************************************
// Undo
// ***********************************************************************

void JZSong::NewUndoBuffer()
{
  for (int i = 0; i < nTracks; i++)
    Tracks[i]->NewUndoBuffer();
}

void JZSong::Undo()
{
  wxBeginBusyCursor();
  for (int i = 0; i < nTracks; i++)
    Tracks[i]->Undo();
  wxEndBusyCursor();
}

void JZSong::Redo()
{
  wxBeginBusyCursor();
  for (int i = 0; i < nTracks; i++)
    Tracks[i]->Redo();
  wxEndBusyCursor();
}

// ***********************************************************************
// SetTicksPerQuarter
// ***********************************************************************

void JZSong::SetTicksPerQuarter(int NewTicks)
{
  int tt, ee;

  double f = (double)NewTicks / (double)TicksPerQuarter;
  for (tt = 0; tt < nTracks; tt++)
  {
    tTrack *t = Tracks[tt];
    for (ee = 0; ee < t->nEvents; ee++)
    {
      JZEvent *e = t->Events[ee];
      e->SetClock((long)(f * e->GetClock() + 0.5));
      tKeyOn *k = e->IsKeyOn();
      if (k)
      {
        k->Length = (long)(f * k->Length + 0.5);
      }
    }
  }
  TicksPerQuarter = NewTicks;
}

// ***********************************************************************
// SetMeterChange
// ***********************************************************************

int JZSong::SetMeterChange(int BarNr, int Numerator, int Denomiator)
{

  NewUndoBuffer();

  // Clock Taktanfang und -ende

  JZBarInfo BarInfo(this);
  BarInfo.SetBar(BarNr - 1);
  long FrClock = BarInfo.Clock;
  BarInfo.Next();
  long ToClock = BarInfo.Clock;

  // evtl vorhandene TimeSignatures loeschen

  tTrack *t = Tracks[0];
  tEventIterator Iterator(t);
  JZEvent *e = Iterator.Range(FrClock, ToClock);
  while (e)
  {
    if (e->IsTimeSignat())
      t->Kill(e);
    e = Iterator.Next();
  }

  // neues TimeSignature Event ablegen

  int Shift = 2;
  switch (Denomiator)
  {
    case 1:  Shift = 0; break;
    case 2:  Shift = 1; break;
    case 4:  Shift = 2; break;
    case 8:  Shift = 3; break;
    case 16: Shift = 4; break;
    case 32: Shift = 5; break;
  }

  e = new tTimeSignat(FrClock, Numerator, Shift);
  t->Put(e);
  t->Cleanup();
  return 0;
}


int JZSong::NumUsedTracks()
{
  int n;
  for (n = nTracks; n > 1; n--)
    if (!Tracks[n-1]->IsEmpty())
      break;
  return n;
}


// SN++
void JZSong::moveTrack(int from, int to)
{
  tTrack *Track;
  int i;

  if (from == to) return;

  Track = Tracks[from];
  if (from > to) {
    for (i=from;i>=to;i--)
       Tracks[i] = Tracks[i-1];
  } else {
    for (i=from;i<=to;i++)
       Tracks[i] = Tracks[i+1];
  }
  Tracks[to] = Track;
}


