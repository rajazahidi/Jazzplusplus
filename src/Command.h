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

#ifndef JZ_COMMAND_H
#define JZ_COMMAND_H

class tFilter;
class JZEvent;
class tTrack;
class JZSong;
class tEventArray;
class JZRndArray;
class JZBarInfo;
class tKeyOn;

class tScale
{
  public:
    void Init(int ScaleNr, tFilter *f = 0);
    int ScaleKeys[12];

    int Member(int Key)
    {
      return ScaleKeys[Key % 12];
    }

    int Next(int Key);
    int Prev(int Key);
    int Transpose(int Key, int Steps);
    int FitInto(int Key);
    static int Analyze(tFilter *f);        // returns ScaleNr
};


class tCommand
{
  public:
    tFilter *Filter;
    JZSong   *Song;
    int      Reverse;

    tCommand(tFilter *f);
    virtual void Execute(int NewUndo = 1);
    virtual void ExecuteTrack(tTrack *t);
    virtual void ExecuteEvent(tTrack *t, JZEvent *e);
    long Interpolate(long clk, long vmin, long vmax);
    virtual ~tCommand() {}
};


class tCmdShift : public tCommand
{
  long DeltaClock;
  public:
    tCmdShift(tFilter *f, long DeltaClock);
    virtual void ExecuteEvent(tTrack *t, JZEvent *e);
};


class tCmdErase : public tCommand
{
  public:
    int LeaveSpace;
    tCmdErase(tFilter *f, int LeaveSpace = 1);
    virtual void Execute(int NewUndo = 1);
    virtual void ExecuteEvent(tTrack *t, JZEvent *e);
};

class tCmdVelocity : public tCommand
{
  public:
    int FromValue, ToValue, Mode;
    tCmdVelocity(tFilter *f, int From, int To, int Mode);
    virtual void ExecuteEvent(tTrack *t, JZEvent *e);
};


class tCmdLength : public tCommand
{
  public:
    int FromValue, ToValue, Mode;
    tCmdLength(tFilter *f, int From, int To, int Mode);
    virtual void ExecuteEvent(tTrack *t, JZEvent *e);
};


class tCmdSeqLength : public tCommand
{
  public:
  double scale;
  long startClock;
    tCmdSeqLength(tFilter *f, double scale);
    virtual void ExecuteEvent(tTrack *t, JZEvent *e);
};



class tCmdMidiDelay : public tCommand
{
  public:
  double scale;
  long clockDelay;
  int repeat;

    tCmdMidiDelay(tFilter *f, double scale, long clockDelay, int repeat);
    virtual void ExecuteEvent(tTrack *t, JZEvent *e);
};

class tCmdConvertToModulation : public tCommand
{
  public:

  tCmdConvertToModulation(tFilter *f);
  virtual void ExecuteTrack(tTrack *t);
};





class tCmdCleanup : public tCommand
{
    long lengthLimit;
    int  shortenOverlaps;
    tKeyOn *prev_note[16][128];
  public:
    tCmdCleanup(tFilter *f, long limitClocks, int shortenOverlaps);
    virtual void ExecuteTrack(tTrack *t);
    virtual void ExecuteEvent(tTrack *t, JZEvent *e);
};


class tCmdSearchReplace : public tCommand
{
  short fr, to;
  public:
    tCmdSearchReplace(tFilter *f, short fr, short to);
    virtual void ExecuteEvent(tTrack *t, JZEvent *e);
};


class tCmdQuantize : public tCommand
{
    long Quantize(long Clock, int islen);
  public:
    long QntClocks;
    int NoteStart;        // yes
    int NoteLength;        // no
    int Delay;         // zero
    int Groove;         // zero
    tCmdQuantize(tFilter *f, long QntClocks, int groove, int delay);
    virtual void ExecuteEvent(tTrack *t, JZEvent *e);
};


class tCmdTranspose : public tCommand
{
  public:
    int Notes;
    int FitIntoScale;
    tScale Scale;
    tCmdTranspose(tFilter *f, int Notes, int ScaleNr = 0, int FitIntoScale = 0);
    virtual void ExecuteEvent(tTrack *t, JZEvent *e);
};


class tCmdSetChannel : public tCommand
{
  public:
    int NewChannel;        // 0
    tCmdSetChannel(tFilter *f, int NewChannel);
    virtual void ExecuteEvent(tTrack *t, JZEvent *e);
};


class tCmdCopyToBuffer : public tCommand
{
  tEventArray *Buffer;
  public:
    tCmdCopyToBuffer(tFilter *f, tEventArray *Buffer);
    virtual void ExecuteEvent(tTrack *t, JZEvent *e);
};



class tCmdCopy : public tCommand
{
  public:
    int  DestTrack;
    long DestClock;

    int EraseSource;        // no
    int EraseDestin;        // yes
    int InsertSpace;        // no
    long RepeatClock;        // -1L

    tCmdCopy(tFilter *f, long DestTrack, long DestClock);
    virtual void ExecuteTrack(tTrack *t);
};



class tCmdExchLeftRight : public tCommand
{
  public:
    tCmdExchLeftRight(tFilter *f);
    virtual void ExecuteEvent(tTrack *t, JZEvent *e);
};

class tCmdExchUpDown : public tCommand
{
  public:
    tCmdExchUpDown(tFilter *f);
    virtual void ExecuteTrack(tTrack *t);
};

class tCmdMapper : public tCommand
{
  public:
    enum prop { veloc, length, key, rhythm, random, pan, modul, cc1, cc2, pitch, clock };
    tCmdMapper(tFilter *f, prop src, prop dst, JZRndArray &array, int nbars, int add);
    ~tCmdMapper();
    virtual void ExecuteEvent(tTrack *t, JZEvent *e);
  private:
    int n_bars;
    int start_bar;
    int add;
    prop source;
    prop destin;
    JZBarInfo *binfo;
    JZRndArray &array;
};

#endif // !defined(JZ_COMMAND_H)
