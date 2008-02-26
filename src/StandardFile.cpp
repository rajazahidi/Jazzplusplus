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

#include "StandardFile.h"
#include "ErrorMessage.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>

// ----------------------------------------------------------------------
// StdFile-Util
// ----------------------------------------------------------------------

#ifdef sparc

static void SwapW(void *p)
{
}

static void SwapL(void *p)
{
}

#else

static void SwapW(void* p)
{
  char *cp = (char *)p;
  char tmp = cp[0];
  cp[0] = cp[1];
  cp[1] = tmp;
}


static void SwapL(void* p)
{
  short tmp, *sp = (short *)p;
  SwapW(&sp[0]);
  SwapW(&sp[1]);
  tmp = sp[0];
  sp[0] = sp[1];
  sp[1] = tmp;
}

#endif

// --------------------------------------------------------------
// StdChunk
// --------------------------------------------------------------

tStdChunk::tStdChunk()
  : mpBase(0)
{
  Size = 128;
  mpBase = new unsigned char [Size];
  nRead = 0;
  Rewind();
}

tStdChunk::~tStdChunk()
{
  delete [] mpBase;
}

void tStdChunk::Rewind()
{
  RunningStatus = 0;
  cp = mpBase;
  EofSeen = 0;
  Clock = 0;
}


inline void tStdChunk::Resize(int Needed)
{
  long Used = cp - mpBase;
  long i, n = Size;
  if (Size - Used < Needed)
  {
    do {
      Size *= 2;
      //mpBase = (unsigned char *)realloc(mpBase, Size);
    } while (Size - Used < Needed);
    unsigned char *tmp = new unsigned char [Size];
    for (i = 0; i < n; i++)
    {
      tmp[i] = mpBase[i];
    }
    delete [] mpBase;
    mpBase = tmp;
    cp = mpBase + Used;
  }
}


inline int tStdChunk::IsEof()
{
  return EofSeen || ((cp - mpBase) >= nRead);
}


void tStdChunk::PutVar(unsigned long val)
{
  unsigned long buf;
  buf = val & 0x7f;
  while ((val >>= 7) > 0)
  {
    buf <<= 8;
    buf |= 0x80;
    buf += (val & 0x7f);
  }

  while (1)
  {
    *cp ++ = buf;
    if (buf & 0x80)
      buf >>= 8;
    else
      break;
  }
}


unsigned long tStdChunk::GetVar()
{
  unsigned long val;
  char c;
  if ((val = *cp++) & 0x80)
  {
    val &= 0x7f;
    do
      val = (val << 7) + ((c = *cp++) & 0x7f);
    while (c & 0x80);
  }
  return val;
}



void tStdChunk::Put(JZEvent *e, unsigned char* Data, int Length)
{
  unsigned char Stat;
  long  dif;

  Resize(Length + 20);
  dif = e->GetClock() - Clock;
  PutVar(dif);
  Clock += dif;

#if 0
printfxxo("%02X %02X   ", e->Clock, dif);
if (e->Stat != 0x90)
{
   int i;
   printf("%02X ", e->Stat);
   for (i = 0; i < Length; i++)
     printf("%02X ", Data[i]);
   putchar('\n');
}
#endif

  switch (e->Stat)
  {
    // KeyOff -> KeyOn mit Vel=0. Gives better Runningstatus!
    case StatKeyOff:
      // SN-- only if KeyOff veloc is zero
      if (!e->IsKeyOff()->OffVeloc)
      {
        Stat = StatKeyOn | e->IsChannelEvent()->Channel;
        if (Stat != RunningStatus)
        {
          RunningStatus = Stat;
          *cp++ = Stat;
        }
        *cp++ = Data[0];
        *cp++ = 0;
      }
      else
      {
        Stat = StatKeyOff | e->IsChannelEvent()->Channel;
        if (Stat != RunningStatus)
        {
          RunningStatus = Stat;
          *cp++ = Stat;
        }
        while (Length--)
        {
          *cp++ = *Data++;
        }

      }
      break;
    case StatKeyOn:
    case StatControl:
    case StatPitch:
    case StatProgram:
    case StatKeyPressure:
    // SN++
    case StatChnPressure:

      Stat = e->Stat | e->IsChannelEvent()->Channel;
      if (Stat != RunningStatus)
      {
        RunningStatus = Stat;
        *cp++ = Stat;
      }
      while (Length--)
      {
        *cp++ = *Data++;
      }
      break;

    case StatSysEx:
      Stat = StatSysEx;
      RunningStatus = 0;
      *cp++ = Stat;
      PutVar(Length);
      while (Length--)
      {
        *cp++ = *Data++;
      }
      break;

    /*
     * Meta-Events
     */

    case StatText:
    case StatTrackName:
    case StatMarker:
    case StatEndOfTrack:
    case StatSetTempo:
    case StatTimeSignat:
    case StatKeySignat:
    case StatMtcOffset:

    default:        /* hopefully */

#if 0
if (1)
{
   int i;
   printf("%02X ", e->Stat);
   for (i = 0; i < Length; i++)
     printf("%02X ", Data[i]);
   putchar('\n');
}
#endif

      Stat = e->Stat;
      RunningStatus = 0;
      *cp++ = 0xff;
      *cp++ = Stat;
      PutVar(Length);
      while (Length--)
      {
        *cp++ = *Data++;
      }
      break;
  }
}




JZEvent *tStdChunk::Get()
{
  int len;
  unsigned char Stat;
  unsigned char Channel;
  JZEvent *e = 0;

  while (!IsEof())
  {

    Clock += GetVar();

    switch (Stat = *cp)        // Event-Typ
    {

      case StatSysEx:          // Sysex
        ++ cp;
        len = GetVar();
        e = new tSysEx(Clock, cp, len);
        cp += len;
        //RunningStatus = 0;
        return e;

      case 0xff:                // Meta-Event
        ++ cp;

#if 0
if (1)
{
   printf("%02X %02X", *cp, Clock);
   putchar('\n');
}
#endif

        switch (Stat = *cp++)        // Meta-Type
        {

          case StatText:        // Text-Event
            len = GetVar();
            e = new tText(Clock, cp, len);
            cp += len;
            return e;

          case StatTrackName:        // Track-Name
            len = GetVar();
            e = new tTrackName(Clock, cp, len);
            cp += len;
            return e;

          case StatPlayTrack:        // JAVE playtrack event
            len = GetVar();
            fprintf(stderr, "reading playtrack event\n");
            e = new tPlayTrack(Clock, cp, len);
            cp += len;
            return e;


          case StatJazzMeta:        // Jazz Meta Event
            len = GetVar();
            if (memcmp(cp, "JAZ2", 4) == 0)
              e = new tJazzMeta(Clock, cp, len);
            else
              e = new tMetaEvent(Clock, Stat, cp, len);
            cp += len;
            return e;

          case StatCopyright:        // Copyright notice
            len = GetVar();
            e = new tCopyright(Clock, cp, len);
            cp += len;
            return e;

          case StatMarker:
            len = GetVar();
            e = new tMarker(Clock, cp, len);
            cp += len;
            return e;

          case StatEndOfTrack:
            EofSeen = 1;
            cp += GetVar();
            e = new tEndOfTrack(Clock); //JAVE return an explicit event rather than 0
            return e;
            //return 0;                // EOF

          case StatSetTempo:
            len = GetVar();
            e = new tSetTempo(Clock, cp[0], cp[1], cp[2]);
            cp += len;
            return e;

          case StatTimeSignat:
            len = GetVar();
            e = new tTimeSignat(Clock, cp[0], cp[1], cp[2], cp[3]);
            cp += len;
            return e;

          case StatMtcOffset:                // MtcOffset
            len = GetVar();
            e = new tMtcOffset(Clock, cp, len);
            cp += len;
            RunningStatus = 0;
            return e;

          default:                // Text und andere ignorieren
            len = GetVar();
            e = new tMetaEvent(Clock, Stat, cp, len);
            cp += len;
            return e;
        }
        break;

      default:

        if (cp[0] & 0x80)        // neuer Running Status?
          RunningStatus = *cp++;
        Stat  = RunningStatus & 0xF0;
        Channel = RunningStatus & 0x0F;

        switch(Stat)
        {
          case StatKeyOff:  // SN++ added off veloc
            e = new tKeyOff(Clock, Channel, cp[0],cp[1]);
            cp += 2;
            return e;

          case StatKeyOn:
            if (cp[1])
              e = new tKeyOn(Clock, Channel, cp[0], cp[1]);
            else
              e = new tKeyOff(Clock, Channel, cp[0]);
            cp += 2;
            return e;

          case StatKeyPressure:
// SN++ Aftertouch
            e = new tKeyPressure(Clock, Channel, cp[0], cp[1]);
            cp += 2;
            return e;

          case StatControl:
            e = new tControl(Clock, Channel, cp[0], cp[1]);
            cp += 2;
            return e;

          case StatPitch:
            e = new tPitch(Clock, Channel, cp[0], cp[1]);
            cp += 2;
            return e;

          case StatProgram:
            e = new tProgram(Clock, Channel, cp[0]);
            cp += 1;
            return e;

          case StatChnPressure:
            e = new tChnPressure(Clock, Channel, cp[0]);
            cp += 1;
            return e;

          default:
            Error("GetEvent: unknown Status %d", Stat);
            return 0;
        }
    }
  }
  return 0; // eof
}


// -------------------------------------------------------------------


void tStdChunk::Load(FILE *fd)
{
  char Type[4];
  int Size;

  fread(Type, 4, 1, fd);
  fread(&Size, 4, 1, fd);
  SwapL(&Size);
  Resize(Size);
  fread(mpBase, Size, 1, fd);
  nRead = Size;
}


void tStdChunk::Save(FILE *fd)
{
  int Size, hSize;

  Resize(4);
  *cp++ = 0x00;
  *cp++ = 0xff;
  *cp++ = 0x2f;
  *cp++ = 0x00;
  fwrite("MTrk", 4, 1, fd);
  Size = hSize = cp - mpBase;
  SwapL(&hSize);
  fwrite(&hSize, 4, 1, fd);
  fwrite(mpBase, Size, 1, fd);
}

// ----------------------------------------------------------------------

struct tFileHeader
{
  short Format;
  short nTracks;
  short Unit;

  void Swap();
};

void tFileHeader::Swap()
{
  SwapW(&Format);
  SwapW(&nTracks);
  SwapW(&Unit);
}

// ---------------------------- ReadStd -----------------------------


tStdRead::tStdRead()
  : tReadBase(),
    mpTracks(0),
    TrackNr(0)
{
}

tStdRead::~tStdRead()
{
  delete [] mpTracks;
}

int tStdRead::Open(const char* pFileName)
{
  tFileHeader h;
  int hSize;
  int i;
  char Type[4];

  if (!tReadBase::Open(pFileName))
  {
    Error("cant open %s", pFileName);
    return 0;
  }

  fread(Type, 4, 1, fd);

  if (strncmp("MThd", Type, 4) != 0)
  {
    Error("wrong Fileheader");
    return 0;
  }

  fread(&hSize, 4, 1, fd);
  SwapL(&hSize);
  assert (hSize == sizeof(h));

  fread(&h, 6, 1, fd);
  h.Swap();
  nTracks = h.nTracks;
  TicksPerQuarter = h.Unit;

  mpTracks = new tStdChunk [nTracks];
  for (i = 0; i < nTracks; i++)
  {
    mpTracks[i].Load(fd);
  }

  TrackNr = -1;

  return nTracks;
}

void tStdRead::Close()
{
  tReadBase::Close();
}

JZEvent *tStdRead::Read()
{
  assert(TrackNr >= 0 && TrackNr < nTracks);
  return mpTracks[TrackNr].Get();
}

int tStdRead::NextTrack()
{
  ++TrackNr;
  return TrackNr < nTracks;
}


// ------------------------------ tWriteStd ---------------------------------

tStdWrite::tStdWrite()
  : tWriteBase(),
    mpTracks(0),
    TrackNr(0),
    nTracks(0),
    TicksPerQuarter(0)
{
}

tStdWrite::~tStdWrite()
{
  delete [] mpTracks;
}

int tStdWrite::Open(char* pFileName, int ntracks, int timebase)
{
  if (!tWriteBase::Open(pFileName, ntracks, timebase))
  {
    return 0;
  }
  nTracks = ntracks;
  TicksPerQuarter = timebase;
  mpTracks = new tStdChunk [ntracks];
  TrackNr = -1;
  return nTracks;
}

void tStdWrite::Close()
{
  long Size;
  tFileHeader h;
  int i;

  fwrite("MThd", 4, 1, fd);
  Size = 6;
  SwapL(&Size);
  fwrite(&Size, 4, 1, fd);
  h.Unit = TicksPerQuarter;
  h.Format = 1;
  h.nTracks = nTracks;
  h.Swap();
  fwrite(&h, 6, 1, fd);

  for (i = 0; i < nTracks; i++)
  {
    mpTracks[i].Save(fd);
  }

  tWriteBase::Close();
}

void tStdWrite::NextTrack()
{
  ++TrackNr;
}

int tStdWrite::Write(JZEvent *e, unsigned char* data, int len)
{
  assert(TrackNr >= 0 && TrackNr < nTracks);
  mpTracks[TrackNr].Put(e, data, len);
  return 0;
}
