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

#ifndef JZ_STANDARDFILE_H
#define JZ_STANDARDFILE_H

#include "Events.h"

#include <stdio.h>

class JZEvent;
class tStdChunk
{
  public:

    tStdChunk();

    ~tStdChunk();

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


class tStdRead : public tReadBase
{
  public:

    tStdRead();

    virtual ~tStdRead();

    virtual int Open(const char* pFileName);
    virtual void Close();

    virtual JZEvent* Read();
    virtual int NextTrack();

  private:

    tStdChunk* mpTracks;
    int TrackNr;
};



class tStdWrite : public tWriteBase
{
  public:

    tStdWrite();

    virtual ~tStdWrite();

    virtual int Open(char* pFileName, int nTracks, int TicksPerQuarter);
    virtual void Close();
    virtual int Write(JZEvent* Event, unsigned char *s, int len);
    virtual void NextTrack();

  private:

    tStdChunk* mpTracks;
    int TrackNr;
    int nTracks;
    int TicksPerQuarter;
};

#endif // !defined(JZ_STANDARDFILE_H)
