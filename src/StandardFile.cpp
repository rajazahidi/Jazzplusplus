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

#include <sstream>

using namespace std;

#ifdef sparc

//*****************************************************************************
//*****************************************************************************
static void SwapW(void *p)
{
}

//*****************************************************************************
//*****************************************************************************
static void SwapL(void *p)
{
}

#else

//*****************************************************************************
//*****************************************************************************
static void SwapW(void* p)
{
  char *cp = (char *)p;
  char tmp = cp[0];
  cp[0] = cp[1];
  cp[1] = tmp;
}

//*****************************************************************************
//*****************************************************************************
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

//*****************************************************************************
//*****************************************************************************
class JZStandardChunk
{
  public:

    JZStandardChunk();

    ~JZStandardChunk();

    int IsEof();            // Only after Load, Save never has Eof.

    void Load(FILE* fd);

    void Save(FILE* fd);    // Depends on EndOfTrack

    void Put(JZEvent* pEvent, unsigned char* pData, int Length);

    // A return value of NULL indicates we are at the end of the track.
    JZEvent* Get();

    void Rewind();

  private:

    long Size;             // Size of base
    long nRead;            // Number of bytes read from the file
    unsigned char* mpBase; // Buffer for data.
    unsigned char* cp;     // Aktueller Schreib/Lese pointer
    long Clock;            // Absolute Clock
    int EofSeen;           // endoftrack meta-event read
    int RunningStatus;

    void Resize(int SizeNeeded);
    void PutVar(unsigned long val);
    unsigned long GetVar();
};

//*****************************************************************************
// StdChunk
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZStandardChunk::JZStandardChunk()
  : mpBase(0)
{
  Size = 128;
  mpBase = new unsigned char [Size];
  nRead = 0;
  Rewind();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZStandardChunk::~JZStandardChunk()
{
  delete [] mpBase;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZStandardChunk::Rewind()
{
  RunningStatus = 0;
  cp = mpBase;
  EofSeen = 0;
  Clock = 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline
void JZStandardChunk::Resize(int Needed)
{
  long Used = cp - mpBase;
  long i, n = Size;
  if (Size - Used < Needed)
  {
    do
    {
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

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline
int JZStandardChunk::IsEof()
{
  return EofSeen || ((cp - mpBase) >= nRead);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZStandardChunk::PutVar(unsigned long val)
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

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
unsigned long JZStandardChunk::GetVar()
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

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZStandardChunk::Put(JZEvent* pEvent, unsigned char* Data, int Length)
{
  unsigned char Stat;
  long  dif;

  Resize(Length + 20);
  dif = pEvent->GetClock() - Clock;
  PutVar(dif);
  Clock += dif;

#if 0
  printfxxo("%02X %02X   ", pEvent->Clock, dif);
  if (pEvent->GetStat() != 0x90)
  {
    int i;
    printf("%02X ", pEvent->GetStat());
    for (i = 0; i < Length; i++)
    {
      printf("%02X ", Data[i]);
    }
    putchar('\n');
  }
#endif

  switch (pEvent->GetStat())
  {
    // KeyOff -> KeyOn mit Vel=0. Gives better Runningstatus!
    case StatKeyOff:
      // SN-- only if KeyOff veloc is zero
      if (!pEvent->IsKeyOff()->OffVeloc)
      {
        Stat = StatKeyOn | pEvent->IsChannelEvent()->GetChannel();
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
        Stat = StatKeyOff | pEvent->IsChannelEvent()->GetChannel();
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

      Stat = pEvent->GetStat() | pEvent->IsChannelEvent()->GetChannel();
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
   printf("%02X ", pEvent->GetStat());
   for (i = 0; i < Length; i++)
     printf("%02X ", Data[i]);
   putchar('\n');
}
#endif

      Stat = pEvent->GetStat();
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

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZEvent* JZStandardChunk::Get()
{
  int len;
  unsigned char Stat;
  unsigned char Channel;
  JZEvent* pEvent = 0;

  while (!IsEof())
  {
    Clock += GetVar();

    switch (Stat = *cp)        // Event-Typ
    {
      case StatSysEx:          // Sysex
        ++ cp;
        len = GetVar();
        pEvent = new tSysEx(Clock, cp, len);
        cp += len;
        //RunningStatus = 0;
        return pEvent;

      case 0xff:                // Meta-Event
        ++cp;

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
            pEvent = new tText(Clock, cp, len);
            cp += len;
            return pEvent;

          case StatTrackName:        // Track-Name
            len = GetVar();
            pEvent = new tTrackName(Clock, cp, len);
            cp += len;
            return pEvent;

          case StatPlayTrack:        // JAVE playtrack event
            len = GetVar();
            fprintf(stderr, "reading playtrack event\n");
            pEvent = new tPlayTrack(Clock, cp, len);
            cp += len;
            return pEvent;

          case StatJazzMeta:        // Jazz Meta Event
            len = GetVar();
            if (memcmp(cp, "JAZ2", 4) == 0)
              pEvent = new tJazzMeta(Clock, cp, len);
            else
              pEvent = new tMetaEvent(Clock, Stat, cp, len);
            cp += len;
            return pEvent;

          case StatCopyright:        // Copyright notice
            len = GetVar();
            pEvent = new tCopyright(Clock, cp, len);
            cp += len;
            return pEvent;

          case StatMarker:
            len = GetVar();
            pEvent = new tMarker(Clock, cp, len);
            cp += len;
            return pEvent;

          case StatEndOfTrack:
            EofSeen = 1;
            cp += GetVar();
            pEvent = new tEndOfTrack(Clock); //JAVE return an explicit event rather than 0
            return pEvent;
            //return 0;                // EOF

          case StatSetTempo:
            len = GetVar();
            pEvent = new tSetTempo(Clock, cp[0], cp[1], cp[2]);
            cp += len;
            return pEvent;

          case StatTimeSignat:
            len = GetVar();
            pEvent = new tTimeSignat(Clock, cp[0], cp[1], cp[2], cp[3]);
            cp += len;
            return pEvent;

          case StatMtcOffset:                // MtcOffset
            len = GetVar();
            pEvent = new tMtcOffset(Clock, cp, len);
            cp += len;
            RunningStatus = 0;
            return pEvent;

          default:                // Text und andere ignorieren
            len = GetVar();
            pEvent = new tMetaEvent(Clock, Stat, cp, len);
            cp += len;
            return pEvent;
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
          pEvent = new tKeyOff(Clock, Channel, cp[0],cp[1]);
          cp += 2;
          return pEvent;

        case StatKeyOn:
          if (cp[1])
            pEvent = new tKeyOn(Clock, Channel, cp[0], cp[1]);
          else
            pEvent = new tKeyOff(Clock, Channel, cp[0]);
          cp += 2;
          return pEvent;

        case StatKeyPressure:
// SN++ Aftertouch
          pEvent = new tKeyPressure(Clock, Channel, cp[0], cp[1]);
          cp += 2;
          return pEvent;

        case StatControl:
          pEvent = new tControl(Clock, Channel, cp[0], cp[1]);
          cp += 2;
          return pEvent;

        case StatPitch:
          pEvent = new tPitch(Clock, Channel, cp[0], cp[1]);
          cp += 2;
          return pEvent;

        case StatProgram:
          pEvent = new tProgram(Clock, Channel, cp[0]);
          cp += 1;
          return pEvent;

        case StatChnPressure:
          pEvent = new tChnPressure(Clock, Channel, cp[0]);
          cp += 1;
          return pEvent;

        default:
        {
          ostringstream Oss;
          Oss << "GetEvent: unknown Status " << Stat;
          Error(Oss.str());
          return 0;
        }
      }
    }
  }
  return 0; // eof
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZStandardChunk::Load(FILE* pFd)
{
  char Type[4];
  int Size;

  fread(Type, 4, 1, pFd);
  fread(&Size, 4, 1, pFd);
  SwapL(&Size);
  Resize(Size);
  fread(mpBase, Size, 1, pFd);
  nRead = Size;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZStandardChunk::Save(FILE* pFd)
{
  int Size, hSize;

  Resize(4);
  *cp++ = 0x00;
  *cp++ = 0xff;
  *cp++ = 0x2f;
  *cp++ = 0x00;
  fwrite("MTrk", 4, 1, pFd);
  Size = hSize = cp - mpBase;
  SwapL(&hSize);
  fwrite(&hSize, 4, 1, pFd);
  fwrite(mpBase, Size, 1, pFd);
}

//*****************************************************************************
//*****************************************************************************
struct JZFileHeader
{
  short Format;
  short mTrackCount;
  short Unit;

  void Swap();
};

//*****************************************************************************
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZFileHeader::Swap()
{
  SwapW(&Format);
  SwapW(&mTrackCount);
  SwapW(&Unit);
}

//*****************************************************************************
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZStandardRead::JZStandardRead()
  : JZReadBase(),
    mpTracks(0),
    mTrackIndex(0)
{
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZStandardRead::~JZStandardRead()
{
  delete [] mpTracks;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZStandardRead::Open(const char* pFileName)
{
  JZFileHeader FileHeader;
  int hSize;
  int i;
  char Type[4];

  if (!JZReadBase::Open(pFileName))
  {
    ostringstream Oss;
    Oss << "Can't open " << pFileName;
    Error(Oss.str());
    return 0;
  }

  fread(Type, 4, 1, mpFd);

  if (strncmp("MThd", Type, 4) != 0)
  {
    Error("wrong Fileheader");
    return 0;
  }

  fread(&hSize, 4, 1, mpFd);
  SwapL(&hSize);
  assert (hSize == sizeof(FileHeader));

  fread(&FileHeader, 6, 1, mpFd);
  FileHeader.Swap();
  mTrackCount = FileHeader.mTrackCount;
  mTicksPerQuarter = FileHeader.Unit;

  mpTracks = new JZStandardChunk [mTrackCount];
  for (i = 0; i < mTrackCount; i++)
  {
    mpTracks[i].Load(mpFd);
  }

  mTrackIndex = -1;

  return mTrackCount;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZStandardRead::Close()
{
  JZReadBase::Close();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZEvent* JZStandardRead::Read()
{
  assert(mTrackIndex >= 0 && mTrackIndex < mTrackCount);
  return mpTracks[mTrackIndex].Get();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZStandardRead::NextTrack()
{
  ++mTrackIndex;
  return mTrackIndex < mTrackCount;
}

//*****************************************************************************
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZStandardWrite::JZStandardWrite()
  : JZWriteBase(),
    mpTracks(0),
    mTrackIndex(0),
    mTrackCount(0),
    mTicksPerQuarter(0)
{
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZStandardWrite::~JZStandardWrite()
{
  delete [] mpTracks;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZStandardWrite::Open(
  const char* pFileName,
  int TrackCount,
  int TicksPerQuarter)
{
  if (!JZWriteBase::Open(pFileName, TrackCount, TicksPerQuarter))
  {
    return 0;
  }

  mTrackCount = TrackCount;
  mTicksPerQuarter = TicksPerQuarter;
  mpTracks = new JZStandardChunk [TrackCount];
  mTrackIndex = -1;
  return mTrackCount;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZStandardWrite::Close()
{
  long Size;
  JZFileHeader FileHeader;
  int i;

  fwrite("MThd", 4, 1, mpFd);
  Size = 6;
  SwapL(&Size);
  fwrite(&Size, 4, 1, mpFd);
  FileHeader.Unit = mTicksPerQuarter;
  FileHeader.Format = 1;
  FileHeader.mTrackCount = mTrackCount;
  FileHeader.Swap();
  fwrite(&FileHeader, 6, 1, mpFd);

  for (i = 0; i < mTrackCount; i++)
  {
    mpTracks[i].Save(mpFd);
  }

  JZWriteBase::Close();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZStandardWrite::NextTrack()
{
  ++mTrackIndex;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZStandardWrite::Write(JZEvent* pEvent, unsigned char* pString, int Length)
{
  assert(mTrackIndex >= 0 && mTrackIndex < mTrackCount);
  mpTracks[mTrackIndex].Put(pEvent, pString, Length);
  return 0;
}
