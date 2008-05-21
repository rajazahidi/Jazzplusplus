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

//*****************************************************************************
// tCommand
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
tCommand::tCommand(JZFilter* pFilter)
  : mpFilter(pFilter),
    mpSong(pFilter->mpSong),
    mReverse(false)
{
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
tCommand::~tCommand()
{
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void tCommand::Execute(int NewUndo)
{
  wxBeginBusyCursor();
  if (NewUndo)
  {
    mpSong->NewUndoBuffer();
  }
  JZTrackIterator Tracks(mpFilter, mReverse);
  JZTrack* pTrack = Tracks.First();
  while (pTrack)
  {
    ExecuteTrack(pTrack);
    pTrack = Tracks.Next();
  }
  wxEndBusyCursor();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void tCommand::ExecuteTrack(JZTrack* pTrack)
{
  tEventIterator Iterator(pTrack);
  JZEvent* pEvent = Iterator.Range(mpFilter->FromClock, mpFilter->ToClock);
  while (pEvent)
  {
    if (mpFilter->IsSelected(pEvent))
    {
      ExecuteEvent(pTrack, pEvent);
    }
    pEvent = Iterator.Next();
  }
  pTrack->Cleanup();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void tCommand::ExecuteEvent(JZTrack* pTrack, JZEvent* pEvent)
{
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int tCommand::Interpolate(int Clock, int vmin, int vmax)
{
  int ClockMin = mpFilter->FromClock;
  int ClockMax = mpFilter->ToClock;
  return (Clock - ClockMin) * (vmax - vmin) / (ClockMax - ClockMin) + vmin;
}

//*****************************************************************************
// tSelectedEvents
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class tSelectedKeys : public tCommand
{
  public:

    tSelectedKeys(JZFilter* pFilter);

    void ExecuteEvent(JZTrack* pTrack, JZEvent* pEvent);

  public:

    long Keys[128];
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
tSelectedKeys::tSelectedKeys(JZFilter* pFilter)
  : tCommand(pFilter)
{
  int i;
  for (i = 0; i <  128; ++i)
  {
    Keys[i] = 0;
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void tSelectedKeys::ExecuteEvent(JZTrack* pTrack, JZEvent* pEvent)
{
  tKeyOn* pKeyOn = pEvent->IsKeyOn();
  if (pKeyOn)
  {
    Keys[pKeyOn->GetKey()] += pKeyOn->GetEventLength();
  }
}

//*****************************************************************************
// tScale
//*****************************************************************************
                            //  c     d     e  f     g    a      b
static const int CMajor[12] = { 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1 };

void tScale::Init(int ScaleNr, JZFilter* pFilter)
{
  int i;

  for (i = 0; i < 12; i++)
  {
    ScaleKeys[i] = 0;
  }

  if (ScaleNr == gScaleChromatic)
  {
    for (i = 0; i < 12; i++)
      ScaleKeys[i] = 1;
  }

  else if (ScaleNr == gScaleSelected)
  {
    int found = 0;
    tSelectedKeys cmd(pFilter);
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


int tScale::Analyze(JZFilter* pFilter)
{
  long keys[12];
  for (int i = 0; i < 12; i++)
  {
    keys[i] = 0;
  }

  tSelectedKeys cmd(pFilter);
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

// ***********************************************************************
// tCmdShift
// ***********************************************************************

tCmdShift::tCmdShift(JZFilter* pFilter, long dclk)
  : tCommand(pFilter)
{
  DeltaClock = dclk;
}

void tCmdShift::ExecuteEvent(JZTrack* pTrack, JZEvent* pEvent)
{
  JZEvent* c = pEvent->Copy();
  pTrack->Kill(pEvent);
  c->SetClock(c->GetClock() + DeltaClock);
  pTrack->Put(c);
}

// ************************************************************************
// tCmdErase
// ************************************************************************

tCmdErase::tCmdErase(JZFilter* pFilter, int lvsp)
  : tCommand(pFilter)
{
  LeaveSpace = lvsp;
}

void tCmdErase::Execute(int NewUndo)
{
  tCommand::Execute(NewUndo);
  if (!LeaveSpace)
  {
    JZFilter Filter(mpFilter);
    Filter.FromClock = mpFilter->ToClock;
    Filter.ToClock = mpSong->GetLastClock() + 1;
    long DeltaClock = mpFilter->FromClock - mpFilter->ToClock;
    tCmdShift shift(&Filter, DeltaClock);
    shift.Execute(0);
  }
}

void tCmdErase::ExecuteEvent(JZTrack* pTrack, JZEvent* pEvent)
{
  pTrack->Kill(pEvent);
}

// ************************************************************************
// tCmdQuantize
// ************************************************************************

tCmdQuantize::tCmdQuantize(JZFilter* pFilter, long clks, int grov, int dly)
  : tCommand(pFilter)
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

void tCmdQuantize::ExecuteEvent(JZTrack* pTrack, JZEvent* pEvent)
{
  tKeyOn* pKeyOn;
  if ((pKeyOn = pEvent->IsKeyOn()) != 0)
  {
    pKeyOn = (tKeyOn *)pEvent->Copy();
    if (NoteStart)
    {
      pKeyOn->SetClock(Quantize(pKeyOn->GetClock(), 0));
    }
    if (NoteLength)
    {
      pKeyOn->SetLength(Quantize(pKeyOn->GetEventLength(), 2));
    }
    pTrack->Kill(pEvent);
    pTrack->Put(pKeyOn);
  }
}

// ************************************************************************
// tCmdTranspose
// ************************************************************************

tCmdTranspose::tCmdTranspose(JZFilter* pFilter, int notes, int ScaleNr, int fit)
  : tCommand(pFilter)
{
  Scale.Init(ScaleNr, mpFilter);
  Notes = notes;
  FitIntoScale = fit;
}

void tCmdTranspose::ExecuteEvent(JZTrack* pTrack, JZEvent* pEvent)
{
  tKeyOn* pKeyOn;
  if (pEvent->IsKeyOn())
  {
    pKeyOn = (tKeyOn *)pEvent->Copy();
    if (FitIntoScale)
    {
      pKeyOn->SetKey(pKeyOn->GetKey() + Notes);
      pKeyOn->SetKey(Scale.FitInto(pKeyOn->GetKey()));
    }
    else if (Notes)
    {
      pKeyOn->SetKey(Scale.Transpose(pKeyOn->GetKey(), Notes));
    }
    pTrack->Kill(pEvent);
    pTrack->Put(pKeyOn);
  }
// SN++ Aftertouch
  tKeyPressure *a;
  if (pEvent->IsKeyPressure())
  {
    a = (tKeyPressure *)pEvent->Copy();
    if (FitIntoScale)
    {
      a->Key += Notes;
      a->Key = Scale.FitInto(a->Key);
    }
    else if (Notes)
      a->Key = Scale.Transpose(a->Key, Notes);
    pTrack->Kill(pEvent);
    pTrack->Put(a);
  }
//
}

// ************************************************************************
// tCmdSetChannel
// ************************************************************************

tCmdSetChannel::tCmdSetChannel(JZFilter* pFilter, int chan)
  : tCommand(pFilter)
{
  NewChannel = chan;
}

void tCmdSetChannel::ExecuteEvent(JZTrack* pTrack, JZEvent* pEvent)
{
  tChannelEvent* c;

  if ((c = pEvent->IsChannelEvent()) != 0)
  {
    c = (tChannelEvent *)pEvent->Copy();
    c->SetChannel(NewChannel);
    pTrack->Kill(pEvent);
    pTrack->Put(c);
  }
}

// ************************************************************************
// tCmdVelocity
// ************************************************************************

tCmdVelocity::tCmdVelocity(JZFilter* pFilter, int from, int to, int m)
  : tCommand(pFilter)
{
  FromValue = from;
  ToValue  = to;
  Mode = m;
}

void tCmdVelocity::ExecuteEvent(JZTrack* pTrack, JZEvent* pEvent)
{
  tKeyOn* pKeyOn;

  if (pEvent->IsKeyOn() != 0)
  {
    pKeyOn = (tKeyOn *)pEvent->Copy();
    long val = 0;
    if (ToValue <= 0)
      val = FromValue;
    else
      val = Interpolate(pKeyOn->GetClock(), FromValue, ToValue);
    switch (Mode)
    {
      case 0:
        break;
      case 1:
        val = pKeyOn->GetVelocity() + val;
        break;
      case 2:
        val = pKeyOn->GetVelocity() - val;
        break;
    }
    pKeyOn->SetVelocity(val < 1 ? 1 : (val > 127 ? 127 : val));
    pTrack->Kill(pEvent);
    pTrack->Put(pKeyOn);
  }
}

// ************************************************************************
// tCmdLength
// ************************************************************************

tCmdLength::tCmdLength(JZFilter* pFilter, int from, int to, int m)
  : tCommand(pFilter)
{
  FromValue = from;
  ToValue  = to;
  Mode = m;
}

void tCmdLength::ExecuteEvent(JZTrack* pTrack, JZEvent* pEvent)
{
  tKeyOn* pKeyOn;

  if (pEvent->IsKeyOn() != 0)
  {
    pKeyOn = (tKeyOn *)pEvent->Copy();
    long val = 0;
    if (ToValue <= 0)
      val = FromValue;
    else
      val = Interpolate(pKeyOn->GetClock(), FromValue, ToValue);
    switch (Mode)
    {
      case 0:
        break;
      case 1:
        val = pKeyOn->GetEventLength() + val;
        break;
      case 2:
        val = pKeyOn->GetEventLength() - val;
        break;
    }

    pKeyOn->SetLength(val < 1 ? 1 : val);
    pTrack->Kill(pEvent);
    pTrack->Put(pKeyOn);
  }
}



// ************************************************************************
// tCmdSeqLength
//    JAVE this command is supposed to stretch/contract a sequence of events in time
//   by factor "scale" from starting point "startClock"
// ************************************************************************

tCmdSeqLength::tCmdSeqLength(JZFilter* pFilter, double scale)
  : tCommand(pFilter)
{
  this->scale=scale;
  this->startClock=-1000;
}

/** move an event according to startclock and scale
 */
void tCmdSeqLength::ExecuteEvent(JZTrack* pTrack, JZEvent* pEvent)
{
  // Make a copy of the current event.
  JZEvent* k;
  k = (tKeyOn *)pEvent->Copy();

  //little hack, if clock is -1000 it means set startclock from the first event.
  if(startClock==-1000)
  {
    startClock = k->GetClock();
  }

  //calculate the new start clock, move the new event and kill the old one
  k->SetClock((long int)(((k->GetClock() - startClock) * scale) + startClock));
  pTrack->Kill(pEvent);
  pTrack->Put(k);
}



// ************************************************************************
// tCmdConvertToModulation
//    JAVE this command is supposed convert a midi note sequence
// to a pitch bend/volume control sequence instead
// ************************************************************************

tCmdConvertToModulation::tCmdConvertToModulation(JZFilter* pFilter)
  : tCommand(pFilter)
{
}

//need to override executetrack, since we have begin/end behaviour in this filter

void tCmdConvertToModulation::ExecuteTrack(JZTrack* pTrack)
{
  //JAVE:iterate over all events, make a long event from start until stop of the sequence,
  //convert all note-on messages to a pitch bend/volume controller pair, velocity -> volume
  //make a volume off controller at the end of the current event
  tEventIterator Iterator(pTrack);
  JZEvent* pEvent = Iterator.Range(mpFilter->FromClock, mpFilter->ToClock);
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
  while (pEvent)
  {
    if (mpFilter->IsSelected(pEvent) && pEvent->IsKeyOn())
    {
      if (startclock == -1)
      {
        startclock = pEvent->IsKeyOn()->GetClock();
        startvelocity = pEvent->IsKeyOn()->GetVelocity();
        channel = pEvent->IsKeyOn()->GetChannel();
        startkey = pEvent->IsKeyOn()->GetKey();
        previouspitch = pEvent->GetPitch();
      }
      pitchdiff = pEvent->GetPitch()-previouspitch;

      tPitch* pitchmodulation=0;
      pitchmodulation = new tPitch(
        pEvent->GetClock(),
        channel,
        pitchsteparray[pitchdiff + 4]);

      pTrack->Put(pitchmodulation);

      pTrack->Kill(pEvent); //remove the old event

      pTrack->Put(new tControl(
        pEvent->GetClock(),
        channel,
        0x07,
        pEvent->IsKeyOn()->GetVelocity()));
      pTrack->Put(new tControl(
        pEvent->GetClock() + pEvent->IsKeyOn()->GetEventLength(),
        channel,
        0x07,
        0));

      lastlength = pEvent->IsKeyOn()->GetEventLength();
      endclock = pEvent->GetClock();
      previouspitch = pEvent->GetPitch();
    }
    //ExecuteEvent(pTrack, pEvent);
    pEvent = Iterator.Next();
  }
  //now insert the new long event
  tKeyOn* longevent = new tKeyOn(
    startclock,
    channel,
    startkey,
    startvelocity,
    endclock - startclock + lastlength);
  pTrack->Put(longevent);
  pTrack->Cleanup();
}

// ************************************************************************
// tMidiDelayDlg
//    JAVE this is a simple midi delay line
// ************************************************************************

tCmdMidiDelay::tCmdMidiDelay(
  JZFilter* pFilter,
  double scale,
  long clockDelay,
  int repeat)
  : tCommand(pFilter)
{
  this->scale=scale;
  this->clockDelay=clockDelay;
  this->repeat=repeat;
}

void tCmdMidiDelay::ExecuteEvent(JZTrack* pTrack, JZEvent* pEvent)
{
  tKeyOn* pKeyOn;

  for (int i = 1; i < repeat; ++i)
  {
    if (pEvent->IsKeyOn())
    {
      // only echo note events
      pKeyOn = (tKeyOn *)pEvent->Copy();
      pKeyOn->SetClock(pKeyOn->GetClock()+ clockDelay * i);
      pKeyOn->SetVelocity(
        (unsigned char)(pow(scale, i) * pKeyOn->GetVelocity()));
      pTrack->Put(pKeyOn);
    }
  }
}



// ************************************************************************
// tCmdCleanup
// ************************************************************************

tCmdCleanup::tCmdCleanup(JZFilter* pFilter, long clks, int so)
  : tCommand(pFilter)
{
  lengthLimit = clks;
  shortenOverlaps = so;
}

void tCmdCleanup::ExecuteTrack(JZTrack* pTrack)
{
  memset(prev_note, 0, sizeof(prev_note));
  tCommand::ExecuteTrack(pTrack);
}

void tCmdCleanup::ExecuteEvent(JZTrack* pTrack, JZEvent* pEvent)
{
  tKeyOn* pKeyOn;
  if ((pKeyOn = pEvent->IsKeyOn()) != 0)
  {
    if (pKeyOn->GetEventLength() < lengthLimit)
    {
      // Remove short notes.
      pTrack->Kill(pEvent);
    }
    else if (shortenOverlaps)
    {
      // Shorten length of overlapping notes.
      tKeyOn* pPreviousKeyOn = prev_note[pKeyOn->GetChannel()][pKeyOn->GetKey()];
      if (
        pPreviousKeyOn &&
        pPreviousKeyOn->GetClock() + pPreviousKeyOn->GetEventLength() >=
          pKeyOn->GetClock())
      {
        pPreviousKeyOn->SetLength(
          pKeyOn->GetClock() - pPreviousKeyOn->GetClock() - 1);
        if (pPreviousKeyOn->GetEventLength() < lengthLimit)
        {
          pTrack->Kill(pPreviousKeyOn);
        }
      }
      prev_note[pKeyOn->GetChannel()][pKeyOn->GetKey()] = pKeyOn;
    }
  }
}

// ************************************************************************
// tCmdSearchReplace
// ************************************************************************

tCmdSearchReplace::tCmdSearchReplace(JZFilter* pFilter, short sf, short st)
  : tCommand(pFilter)
{
  fr = sf;
  to = st;
}

void tCmdSearchReplace::ExecuteEvent(JZTrack* pTrack, JZEvent* pEvent)
{
  tControl* pControl = pEvent->IsControl();
  if (pControl)
  {
    if (pControl->mControl == fr)
    {
      tControl* pControlCopy = (tControl *)pControl->Copy();
      pControlCopy->mControl = to;
      pTrack->Kill(pControl);
      pTrack->Put(pControlCopy);
    }
  }
}

// ************************************************************************
// tCmdCopyToBuffer
// ************************************************************************

tCmdCopyToBuffer::tCmdCopyToBuffer(
  JZFilter* pFilter,
  tEventArray* pBuffer)
  : tCommand(pFilter)
{
  mpBuffer = pBuffer;
}

void tCmdCopyToBuffer::ExecuteEvent(JZTrack* pTrack, JZEvent* pEvent)
{
  mpBuffer->Put(pEvent->Copy());
}

// **********************************************************************
// tCmdCopy
// **********************************************************************



tCmdCopy::tCmdCopy(JZFilter* pFilter, long dt, long dc)
  : tCommand(pFilter)
{
  DestTrack = dt;
  DestClock = dc;

  EraseSource = 0;        // no
  EraseDestin = 1;        // yes
  InsertSpace = 0;        // no
  RepeatClock = -1;        // -1L

  mReverse = DestTrack > mpFilter->FromTrack;
  if (mReverse)
  {
    DestTrack += mpFilter->ToTrack - mpFilter->FromTrack; // ToTrack inclusive
  }
}



void tCmdCopy::ExecuteTrack(JZTrack *s)
{
  long StartClock, StopClock;
  JZTrack *d;

  StartClock = DestClock;

  if (RepeatClock < 0)
    StopClock = StartClock + mpFilter->ToClock - mpFilter->FromClock;
  else
    StopClock = RepeatClock;

  d = mpSong->GetTrack(DestTrack);

  if (mReverse)
  {
    if (DestTrack)
    {
      --DestTrack;
    }
  }
  else
  {
    ++DestTrack;
  }

  if (s && d)
  {

    // Events nach tmp kopieren
    tEventArray tmp;
    {
      tEventIterator Iterator(s);
      long  DeltaClock = StartClock - mpFilter->FromClock;
      JZEvent* pEvent = Iterator.Range(mpFilter->FromClock, mpFilter->ToClock);
      while (pEvent)
      {
        long NewClock = pEvent->GetClock() + DeltaClock;
        if (NewClock >= StopClock)
          break;

        if (mpFilter->IsSelected(pEvent))
        {
          JZEvent* cpy = pEvent->Copy();
          cpy->SetClock(NewClock);
          tmp.Put(cpy);
        }

        pEvent = Iterator.Next();
        if (!pEvent)
        {
          pEvent = Iterator.First();
          DeltaClock += mpFilter->ToClock - mpFilter->FromClock;
        }
      }
    }

    // ggf Freien Platz einfuegen

    if (InsertSpace && d->GetLastClock() > StartClock)
    {
      tEventIterator Iterator(d);
      JZEvent* pEvent = Iterator.Range(StartClock, d->GetLastClock() + 1);
      long DeltaClock = StopClock - StartClock;
      while (pEvent)
      {
        if (mpFilter->IsSelected(pEvent))
        {
          JZEvent* c = pEvent->Copy();
          c->SetClock(c->GetClock() + DeltaClock);
          d->Kill(pEvent);
          d->Put(c);
        }
        pEvent = Iterator.Next();
      }
      d->Cleanup();
    }

    // ggf Quelle loeschen

    if (EraseSource)
    {
      tEventIterator Iterator(s);
      JZEvent* pEvent = Iterator.Range(mpFilter->FromClock, mpFilter->ToClock);
      while (pEvent)
      {
        if (mpFilter->IsSelected(pEvent))
        {
          s->Kill(pEvent);
        }
        pEvent = Iterator.Next();
      }
      s->Cleanup();
    }

    // ggf Ziel loeschen

    if (EraseDestin)
    {
      tEventIterator Iterator(d);
      JZEvent* pEvent = Iterator.Range(StartClock, StopClock);
      while (pEvent)
      {
        if (mpFilter->IsSelected(pEvent))
        {
          d->Kill(pEvent);
        }
        pEvent = Iterator.Next();
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

tCmdExchLeftRight::tCmdExchLeftRight(JZFilter* pFilter)
  : tCommand(pFilter)
{
}

void tCmdExchLeftRight::ExecuteEvent(JZTrack* pTrack, JZEvent* pEvent)
{
  if (pEvent->IsKeyOn())
  {
    tKeyOn* pKeyOn = (tKeyOn *)pEvent->Copy();
    pKeyOn->SetClock(
      mpFilter->FromClock + mpFilter->ToClock - pKeyOn->GetClock());
    pTrack->Kill(pEvent);
    pTrack->Put(pKeyOn);
  }
}


// ************************************************************************
// tCmdExchUpDown
// ************************************************************************

tCmdExchUpDown::tCmdExchUpDown(JZFilter* pFilter)
  : tCommand(pFilter)
{
}

void tCmdExchUpDown::ExecuteTrack(JZTrack* pTrack)
{
  int i;
  int Keys[128];
  JZEvent* pEvent;

  // find all Key's selected

  for (i = 0; i < 128; i++)
  {
    Keys[i] = 0;
  }

  tEventIterator Iterator(pTrack);
  pEvent = Iterator.Range(mpFilter->FromClock, mpFilter->ToClock);
  while (pEvent)
  {
    if (mpFilter->IsSelected(pEvent) && pEvent->IsKeyOn())
    {
      tKeyOn* pKeyOn = (tKeyOn *)pEvent;
      Keys[pKeyOn->GetKey()] = 1;
    }
    pEvent = Iterator.Next();
  }

  // reverse Key's

  pEvent = Iterator.Range(mpFilter->FromClock, mpFilter->ToClock);
  while (pEvent)
  {
    if (mpFilter->IsSelected(pEvent) && pEvent->IsKeyOn())
    {
      tKeyOn* pKeyOn = (tKeyOn *)pEvent->Copy();
      int n_th = 0;

      // the n'th key from bottom ..
      for (i = 0; i <= pKeyOn->GetKey(); i++)
      {
        n_th += Keys[i];
      }

      // .. becomes the n'th key from top
      for (i = 127; i > 0 && n_th; --i)
      {
        n_th -= Keys[i];
      }

      pKeyOn->SetKey(i + 1);

      pTrack->Kill(pEvent);
      pTrack->Put(pKeyOn);
    }
    pEvent = Iterator.Next();
  }
  pTrack->Cleanup();
}

//*****************************************************************************
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
tCmdMapper::tCmdMapper(
  JZFilter* pFilter,
  prop Source,
  prop Destination,
  JZRndArray& RandomArray,
  int BarCount,
  bool Add)
  : tCommand(pFilter),
    mBarCount(BarCount),
    mStartBar(0),
    mAdd(Add),
    mSource(Source),
    mDestination(Destination),
    mpBarInfo(0),
    mRandomArray(RandomArray)
{
  mpBarInfo = new JZBarInfo(*mpSong);
  mpBarInfo->SetClock(mpFilter->FromClock);
  mStartBar = mpBarInfo->GetBarIndex();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
tCmdMapper::~tCmdMapper()
{
  delete mpBarInfo;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void tCmdMapper::ExecuteEvent(JZTrack* pTrack, JZEvent* pEvent)
{
  tKeyOn* pKeyOn = pEvent->IsKeyOn();
  if (pKeyOn)
  {
    int sval = 0;
    switch (mSource)
    {
      case veloc:
        sval = mRandomArray[(int)pKeyOn->GetVelocity()];
        break;

      case length:
        sval = mRandomArray[(int)pKeyOn->GetEventLength()];
        break;

      case key:
        sval = mRandomArray[(int)pKeyOn->GetKey()];
        break;

      case rhythm:
      {
        mpBarInfo->SetClock(pKeyOn->GetClock());
        long arr_tpb = mRandomArray.Size() / mBarCount;
        long arr_bar = (mpBarInfo->GetBarIndex() - mStartBar) % mBarCount;
        long i = arr_tpb * arr_bar + arr_tpb *
          (pKeyOn->GetClock() - mpBarInfo->GetClock()) / mpBarInfo->GetTicksPerBar();
//        cout
//          << "mpBarInfo->GetTicksPerBar() " << mpBarInfo->GetTicksPerBar()
//          << ", arr_tpb " << arr_tpb
//          << ", pKeyOn->GetClock() " << pKeyOn->GetClock()
//          << ", mpBarInfo->GetClock() " << mpBarInfo->GetClock()
//          << '\n'
//          << "mRandomArray.Size() " << mRandomArray.Size()
//          << ", mBarCount " << mBarCount
//          << ", i " << i
//          << endl;
        sval = mRandomArray[(int)i];
      }
      break;

      case random:
        sval = mRandomArray.Random();
        if (mAdd)
        {
          sval -= mRandomArray.Size()/2;
        }
        break;

      default:
        break;
    }

    switch (mDestination)
    {
      case veloc:
      {
        if (mAdd)
        {
          sval = pKeyOn->GetVelocity() + sval;
        }
        if (sval > 127)
        {
          sval = 127;
        }
        if (sval < 1)
        {
          sval = 1;
        }
        tKeyOn* pKeyOnCopy = (tKeyOn *)pKeyOn->Copy();
        pTrack->Kill(pKeyOn);
        pKeyOnCopy->SetVelocity(sval);
        pTrack->Put(pKeyOnCopy);
      }
      break;

      case key:
      {
        if (mAdd)
        {
          sval = pKeyOn->GetKey() + sval;
        }
        if (sval > 127)
        {
          sval = 127;
        }
        if (sval < 1)
        {
          sval = 1;
        }
        tKeyOn* pKeyOnCopy = (tKeyOn *)pKeyOn->Copy();
        pTrack->Kill(pKeyOn);
        pKeyOnCopy->SetKey(sval);
        pTrack->Put(pKeyOnCopy);
      }
      break;

      case length:
      {
        if (mAdd)
        {
          sval = pKeyOn->GetEventLength() + sval;
        }
        if (sval < 1)
        {
          sval = 1;
        }
        tKeyOn* pKeyOnCopy = (tKeyOn *)pKeyOn->Copy();
        pTrack->Kill(pKeyOn);
        pKeyOnCopy->SetLength(sval);
        pTrack->Put(pKeyOnCopy);
      }
      break;

      case clock:
      {
        tKeyOn* pKeyOnCopy = (tKeyOn *)pKeyOn->Copy();
        pKeyOnCopy->SetClock(pKeyOnCopy->GetClock() + sval);
        if (pKeyOnCopy->GetClock() < 0)
        {
          pKeyOnCopy->SetClock(0);
        }
        pTrack->Kill(pKeyOn);
        pTrack->Put(pKeyOnCopy);
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
