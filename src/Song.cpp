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


JZSong::JZSong()
  : mTracks()
{
  nTracks = eMaxTrackCount;
  TicksPerQuarter = 120;
  intro_length = 0;

  // Start with 100 measures of 4:4 time.
  MaxQuarters = 100 * 4;
}

JZSong::~JZSong()
{
  Clear();
}

void JZSong::Clear()
{
  for (int i = 0; i < nTracks; i++)
  {
    mTracks[i].Clear();
  }
  nTracks = eMaxTrackCount;
}

int JZSong::Speed()
{
  return mTracks[0].GetDefaultSpeed();
}

void JZSong::Read(JZReadBase& Io, const char* pFileName)
{
  int i;
  wxBeginBusyCursor();
  for (i = 0; i < eMaxTrackCount; ++i)
  {
    mTracks[i].Clear();
  }
  int n = Io.Open(pFileName);
  for (i = 0; i < n && i < eMaxTrackCount; ++i)
  {
    mTracks[i].Read(Io);
  }
  Io.Close();
  TicksPerQuarter = Io.GetTicksPerQuarter();

  if (TicksPerQuarter < 48)
  {
    SetTicksPerQuarter(48);
  }
  else if (TicksPerQuarter > 192)
  {
    SetTicksPerQuarter(192);
  }

  // Adjust the song length to equal the MIDI file length plus 16 bars.
  int NewLength = GetLastClock() / TicksPerQuarter + 16 * 4;
  if (NewLength > MaxQuarters)
  {
    MaxQuarters = NewLength;
  }

  wxEndBusyCursor();
}


void JZSong::Write(JZWriteBase& Io, const char* pFileName)
{
  // Make sure track 0 has a synth reset
  if (!mTracks[0].Reset)
  {
    mTracks[0].Reset = gpSynth->Reset()->IsSysEx();
  }

  int n = NumUsedTracks();
  if (!Io.Open(pFileName, n, TicksPerQuarter))
  {
    return;
  }

  wxBeginBusyCursor();
  for (int i = 0; i < n; ++i)
  {
    mTracks[i].Write(Io);
  }
  Io.Close();
  wxEndBusyCursor();
}


JZTrack *JZSong::GetTrack(int Nr)
{
  if (Nr >= 0 && Nr < nTracks)
  {
    return &mTracks[Nr];
  }
  return 0;
}


int JZSong::GetLastClock()
{
  int max = 0;
  for (int i = 0; i < nTracks; i++)
  {
    int clk = mTracks[i].GetLastClock();
    if (clk > max)
    {
      max = clk;
    }
  }
  return max;
}


void JZSong::Clock2String(int clk, char *buf)
{
  JZBarInfo b(this);
  b.SetClock(clk);
  clk -= b.Clock;
  int TicksPerCount = b.TicksPerBar / b.CountsPerBar;
  int Count = clk / TicksPerCount;
  sprintf(
    buf,
    "%3d:%d:%03d",
    b.BarNr + 1 - intro_length,
    Count + 1,
    clk % TicksPerCount);
}

int JZSong::String2Clock(const char* buf)
{
  int Bar = 1;
  int Clock = 0;
  int Count = 1;
  sscanf(buf, "%d:%d:%d", &Bar, &Count, &Clock);
  --Bar;
  --Count;
  Bar += intro_length;  // buf is from user input!
  JZBarInfo BarInfo(this);
  BarInfo.SetBar(Bar);
  int TicksPerCount = BarInfo.TicksPerBar / BarInfo.CountsPerBar;
  return BarInfo.Clock + Count * TicksPerCount + Clock;
}


void JZSong::MergeTracks(
  int FrClock,
  int ToClock,
  tEventArray *Destin,
  const JZMetronomeInfo& MetronomeInfo,
  int delta,
  int mode)
{
  int i;

  // Make metronome
  if (MetronomeInfo.IsOn())
  {
    MakeMetronome(FrClock, ToClock, Destin, MetronomeInfo, delta);
  }

  // Find Solo-Tracks
  int solo = 0;
  for (i = 0; i < nTracks; i++)
  {
    if (mTracks[i].State == tsSolo)
    {
      solo = 1;
      break;
    }
  }

  for (i = 0; i < nTracks; ++i)
  {
    JZTrack* pTrack = &mTracks[i];
    if (pTrack->State == tsSolo || (!solo && pTrack->State == tsPlay))
    {
      if (pTrack->GetAudioMode() != mode)
      {
        continue;
      }

      tEventIterator Iterator(&mTracks[i]);
      JZEvent *e = Iterator.Range(FrClock, ToClock);
      while (e)
      {
        JZEvent *c = e->Copy();
        c->SetClock(c->GetClock() + delta);
        c->SetDevice(pTrack->GetDevice());
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
  JZTrack* pTrack = &mTracks[c->track];//the track we want to play
  tEventIterator IteratorPL(pTrack); //get an iterator of all events the playtrack is pointing to
  JZEvent *f;

  //FIXME this is just to test the idea, it would be good to modify getlastclock instead i think
  //find an EOT event, otherwise default to the last clock(should be + length of the last event as well)
  int loopLength = 0;
  tEventIterator IteratorEOT(pTrack); //get an iterator of all events the playtrack is pointing to
  f = IteratorEOT.Range(0, pTrack->GetLastClock());
  loopLength = pTrack->GetLastClock();
  while(f)
  {
    if (f->IsEndOfTrack())
    {
      loopLength = pTrack->GetLastClock();
    }
    f = IteratorEOT.Next();
  }

  // looplength will be used to loop the track, for the duration of
  // the playtrack event.
  int loopOffset=0;

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
      d->SetDevice(pTrack->GetDevice());
      Destin->Put(d);
      f = IteratorPL.Next();
    }
    loopOffset+=loopLength;
  }
}


void JZSong::MakeMetronome(
  int FrClock,
  int ToClock,
  tEventArray *Destin,
  const JZMetronomeInfo& MetronomeInfo,
  int delta)
{
  JZBarInfo BarInfo( this );
  BarInfo.SetClock(FrClock);
  int clk = BarInfo.Clock;
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
    Destin->Put(MetronomeInfo.CreateNormalEvent(clk + delta));

    //  On a bar?
    if (count == 1 && MetronomeInfo.IsAccented())
    {
      // Insert accented click also
      Destin->Put(MetronomeInfo.CreateAccentedEvent(clk + delta));
    }

    clk += BarInfo.TicksPerBar / BarInfo.CountsPerBar;
    count++;
  }
}


// ********************************************************************
// BarInfo
// *******************************************************************


JZBarInfo::JZBarInfo(JZSong *Song)
  : Iterator(&Song->mTracks[0])
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


void JZBarInfo::SetClock(int clock)
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
  for (int i = 0; i < nTracks; ++i)
  {
    mTracks[i].NewUndoBuffer();
  }
}

void JZSong::Undo()
{
  wxBeginBusyCursor();
  for (int i = 0; i < nTracks; ++i)
  {
    mTracks[i].Undo();
  }
  wxEndBusyCursor();
}

void JZSong::Redo()
{
  wxBeginBusyCursor();
  for (int i = 0; i < nTracks; ++i)
  {
    mTracks[i].Redo();
  }
  wxEndBusyCursor();
}

// ***********************************************************************
// SetTicksPerQuarter
// ***********************************************************************

void JZSong::SetTicksPerQuarter(int NewTicks)
{
  int tt, ee;

  double f = (double)NewTicks / (double)TicksPerQuarter;
  for (tt = 0; tt < nTracks; ++tt)
  {
    JZTrack* pTrack = &mTracks[tt];
    for (ee = 0; ee < pTrack->nEvents; ee++)
    {
      JZEvent *e = pTrack->Events[ee];
      e->SetClock((int)(f * e->GetClock() + 0.5));
      tKeyOn *k = e->IsKeyOn();
      if (k)
      {
        k->Length = (int)(f * k->Length + 0.5);
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
  int FrClock = BarInfo.Clock;
  BarInfo.Next();
  int ToClock = BarInfo.Clock;

  // evtl vorhandene TimeSignatures loeschen

  JZTrack* pTrack = &mTracks[0];
  tEventIterator Iterator(pTrack);
  JZEvent *e = Iterator.Range(FrClock, ToClock);
  while (e)
  {
    if (e->IsTimeSignat())
    {
      pTrack->Kill(e);
    }
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
  pTrack->Put(e);
  pTrack->Cleanup();
  return 0;
}


int JZSong::NumUsedTracks()
{
  int n;
  for (n = nTracks; n > 1; n--)
  {
    if (!mTracks[n - 1].IsEmpty())
    {
      break;
    }
  }
  return n;
}


// SN++
void JZSong::moveTrack(int from, int to)
{
  JZTrack* pTrack;
  int i;

  if (from == to) return;

  pTrack = &mTracks[from];
  if (from > to)
  {
    for (i = from; i >= to; i--)
    {
       mTracks[i] = mTracks[i - 1];
    }
  }
  else
  {
    for (i = from; i <= to; i++)
    {
       mTracks[i] = mTracks[i + 1];
    }
  }
  mTracks[to] = *pTrack;
}
