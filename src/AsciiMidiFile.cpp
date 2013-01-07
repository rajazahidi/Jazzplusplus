//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Copyright (C) 1994-2000 Andreas Voss and Per Sigmond, all rights reserved.
// Modifications Copyright (C) 2008-2013 Peter J. Stieber
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

#include "AsciiMidiFile.h"

//*****************************************************************************
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZAsciiRead::Open(const char* pFileName)
{
  int TrackCount, TicksPerQuarter;
  if (
    fscanf(
      mpFd,
      "Tracks %d, TicksPerQuarter %d\n",
      &TrackCount,
      &TicksPerQuarter) != 2)
  {
    return 0;
  }
  return TrackCount;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZEvent* JZAsciiRead::Read()
{
  JZEvent* pEvent = 0;

  long Clock;
  int StatusByte, Channel, Length;
  if (
    fscanf(
      mpFd,
      "%6lu %02x %2d %d ",
      &Clock,
      &StatusByte,
      &Channel,
      &Length) != 4)
  {
    return pEvent;
  }

  unsigned char* pBuffer = new unsigned char[Length];
  for (int i = 0; i < Length; ++i)
  {
    int d;
    fscanf(mpFd, "%02x ", &d);
    pBuffer[i] = (unsigned char)d;
  }

  switch (StatusByte)
  {
    case StatUnknown:
      break;

    case StatKeyOff:
      pEvent = new JZKeyOffEvent(Clock, Channel, pBuffer[0]);
      break;

    case StatKeyOn:
      pEvent = new JZKeyOnEvent(Clock, Channel, pBuffer[0], pBuffer[1]);
      break;

    case StatControl:
      pEvent = new JZControlEvent(Clock, Channel, pBuffer[0], pBuffer[1]);
      break;

    case StatPitch:
      pEvent = new JZPitchEvent(Clock, Channel, pBuffer[0], pBuffer[1]);
      break;

    case StatProgram:
      pEvent = new JZProgramEvent(Clock, Channel, pBuffer[0]);
      break;

    case StatText:
      pEvent = new JZTextEvent(Clock, pBuffer, Length);
      break;

    case StatTrackName:
      pEvent = new JZTrackNameEvent(Clock, pBuffer, Length);
      break;

    case StatMarker:
      pEvent = new JZMarkerEvent(Clock, pBuffer, Length);
      break;

    case StatEndOfTrack:
      break;

    case StatSetTempo:
      pEvent = new JZSetTempoEvent(Clock, pBuffer[0], pBuffer[1], pBuffer[2]);
      break;

    case StatTimeSignat:
      pEvent = new JZTimeSignatEvent(
        Clock,
        pBuffer[0],
        pBuffer[1],
        pBuffer[2],
        pBuffer[3]);
      break;

    case StatSysEx:
      pEvent = new JZSysExEvent(Clock, pBuffer, Length);
      break;
  }

  delete [] pBuffer;

  return pEvent;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZAsciiRead::NextTrack()
{
  return fscanf(mpFd, "NextTrack\n") == 0;
}

//*****************************************************************************
// Description:
//   Ascii-Output (debug)
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZAsciiWrite::Open(
  const char* pFileName,
  int TrackCount,
  int TicksPerQuarter)
{
  if (!JZWriteBase::Open(pFileName, TrackCount, TicksPerQuarter))
  {
    return 0;
  }

  fprintf(
    mpFd,
    "Tracks %d, TicksPerQuarter %d\n",
    TrackCount,
    TicksPerQuarter);

  return TrackCount;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZAsciiWrite::Write(JZEvent* pEvent, unsigned char* pData, int Length)
{
  JZChannelEvent* pChannelEvent;

  fprintf(mpFd, "%6d %02x ",  pEvent->GetClock(), pEvent->GetStat());
  if ((pChannelEvent = pEvent->IsChannelEvent()) != 0)
  {
    fprintf(mpFd, "%2d ",  pChannelEvent->GetChannel());
  }
  else
  {
    fprintf(mpFd, "-1 ");
  }

  fprintf(mpFd, "%d ", Length);
  for (int i = 0; i < Length; ++i)
  {
    fprintf(mpFd, "%02x ", pData[i]);
  }
  fprintf(mpFd, "\n");

  return 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZAsciiWrite::NextTrack()
{
  fprintf(mpFd, "NextTrack\n");
}
