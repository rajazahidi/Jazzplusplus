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

class tStdChunk
{
  long Size;		// Groesse von Base
  long nRead;		// soviele Bytes aus Datei gelesen
  unsigned char *Base;  // Puffer for Daten
  unsigned char *cp;	// Aktueller Schreib/Lese Pointer
  long Clock;		// Absolute Clock
  int  EofSeen;  	// endoftrack meta-event gelesen
  int  RunningStatus;	// letzter Status

  void Resize(int SizeNeeded);
  void PutVar(unsigned long val);
  unsigned long GetVar();

public:

  int  IsEof();		// nur nach Load, Save hat nie Eof
  void Load(FILE *fd);
  void Save(FILE *fd);	// haengt EndOfTrack an
  tStdChunk();
  ~tStdChunk();
  void Put(JZEvent *e, unsigned char *Data, int Length);
  JZEvent *Get();	// NULL bei Trackende
  void Rewind();	// Schreib/Lesezeiger zurÅcksetzen
};


class tStdRead : public tReadBase
{
    tStdChunk *Tracks;
    int TrackNr;

  public:

    virtual int  Open(const char *fname);
    virtual void Close();

    virtual JZEvent *Read();
    virtual int NextTrack();
};



class tStdWrite : public tWriteBase
{
    tStdChunk *Tracks;
    int TrackNr;
    int nTracks;
    int TicksPerQuarter;

  public:
    virtual int  Open(char *fname, int nTracks, int TicksPerQuarter);
    virtual void Close();
    virtual int Write(JZEvent *e, unsigned char *s, int len);
    virtual void NextTrack();
};

#endif // !defined(JZ_STANDARDFILE_H)
