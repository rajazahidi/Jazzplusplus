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

#ifndef JZ_SONG_H
#define JZ_SONG_H

#include "Track.h"
#include "Configuration.h"

class tMetronomeInfo;
class JZSong;

class JZBarInfo
{
  public:

    int   BarNr;
    long  Clock;
    int   TicksPerBar;
    int   CountsPerBar;

    JZBarInfo(JZSong *Song);

    void SetBar(int Bar = 0);

    void SetClock(long Clock = 0);

    void Next();

  private:

    tEventIterator Iterator;

    JZEvent *e;

    int TicksPerQuarter;
};

// same as keys in pianowin
#define MaxTracks 127

class JZSong
{
  friend class JZBarInfo;

  void MakeMetronome(
    long FrClock,
    long ToClock,
    tEventArray *Destin,
    tMetronomeInfo* MetronomeInfo,
    long delta = 0);

  public:

    int MaxQuarters;    // end of song in quarters for scrollbar settings
    int nTracks;
    int TicksPerQuarter;

    tTrack *Tracks[MaxTracks];
    void NewUndoBuffer();
    void Undo();
    void Redo();

    JZSong();
    virtual ~JZSong();

    void Clear();
    void Read(tReadBase &io, const char *fname = 0);
    void Write(tWriteBase &io, const char *fname = 0);

    tTrack *GetTrack(int Nr);
    long GetLastClock();
    int NumUsedTracks();        // number of used tracks
    int Speed();

// SN++
    void moveTrack(int from,int to);
//

    void Clock2String(long Clock, char *buf);
    long String2Clock(const char *buf);

    // merge Events from all Tracks into Destin
    void MergeTracks(
      long FrClock,
      long ToClock,
      tEventArray *Destin,
      tMetronomeInfo* MetronomeInfo,
      long DeltaClock = 0,
      int mode = 0);

    void MergePlayTrackEvent(
      tPlayTrack* c,
      tEventArray *Destin,
      int recursionDepth);

    void SetTicksPerQuarter(int NewTicks);

    int SetMeterChange(
      int BarNr,
      int Numerator,
      int Denomiator); //  0 = ok

    int GetIntroLength() const
    {
      return intro_length;
    }

    void SetIntroLength(int x)
    {
      intro_length = x;
    }

  private:

    int intro_length;
};

#endif // !defined(JZ_SONG_H)
