/*
**  The JAZZ++ Midi Sequencer
**
** Copyright (C) 1994-2000 Andreas Voss and Per Sigmond, all rights reserved.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
*/                                                                              


#ifndef song_h
#define song_h

#ifndef track_h
#include "track.h"
#endif

#ifndef jazz_h
#include "jazz.h"
#endif

// ************************************************************************
// Song
// ************************************************************************

class tSong;

class tBarInfo
{
    tEventIterator Iterator;
    tEvent *e;
    int TicksPerQuarter;

  public:
    int   BarNr;
    long  Clock;
    int   TicksPerBar;
    int   CountsPerBar;

    tBarInfo(tSong *Song);
    void SetBar(int Bar = 0);
    void SetClock(long Clock = 0);
    void Next();
};

class tMetronomeInfo
{
  public:
    uchar KeyAcc;
    uchar KeyNorm;
    uchar Veloc;
    int IsOn;
    int IsAccented;

    tMetronomeInfo()
      : KeyAcc(36),KeyNorm(37),Veloc(127),IsAccented(1),IsOn(0)
    {}

    tKeyOn *Normal(long clock)
    {
      return new tKeyOn( clock, Config(C_DrumChannel)-1, KeyNorm, Veloc, 15 );
    }

    tKeyOn *Accented(long clock)
    {
      return new tKeyOn( clock, Config(C_DrumChannel)-1, KeyAcc, Veloc, 15 );
    }
};


// same as keys in pianowin
#define MaxTracks 127


class tSong
{
  friend class tBarInfo;
  void MakeMetronome(long FrClock,long ToClock,tEventArray *Destin, tMetronomeInfo *MetronomeInfo, long delta = 0);

  public:

    int MaxQuarters;    // end of song in quarters for scrollbar settings
    int nTracks;
    int TicksPerQuarter;

    tTrack *Tracks[MaxTracks];
    void NewUndoBuffer();
    void Undo();
    void Redo();

    tSong();
    virtual ~tSong();

    void Clear();
    void Read(tReadBase &io, const char *fname = 0);
    void Write(tWriteBase &io, const char *fname = 0);

    tTrack *GetTrack(int Nr);
    long GetLastClock();
    int NumUsedTracks();	// number of used tracks
    int Speed();

// SN++
    void moveTrack(int from,int to);
//

    void Clock2String(long Clock, char *buf);
    long String2Clock(const char *buf);

    // merge Events from all Tracks into Destin
    void MergeTracks(long FrClock, long ToClock, tEventArray *Destin, tMetronomeInfo *MetronomeInfo, long DeltaClock = 0, int mode = 0);
    void MergePlayTrackEvent(tPlayTrack* c,	tEventArray *Destin, int recursionDepth);
    void SetTicksPerQuarter(int NewTicks);
    int  SetMeterChange(int BarNr, int Numerator, int Denomiator); //  0 = ok

    int GetIntroLength() const {
      return intro_length;
    }
    void SetIntroLength(int x) {
      intro_length = x;
    }
private:
    int intro_length;
};


#endif
