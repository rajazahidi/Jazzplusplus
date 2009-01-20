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

class JZFilter;
class JZEvent;
class JZTrack;
class JZSong;
class tEventArray;
class JZRndArray;
class JZBarInfo;
class tKeyOn;

//*****************************************************************************
//*****************************************************************************
class tScale
{
  public:
    void Init(int ScaleNr, JZFilter* pFilter = 0);
    int ScaleKeys[12];

    int Member(int Key)
    {
      return ScaleKeys[Key % 12];
    }

    int Next(int Key);
    int Prev(int Key);
    int Transpose(int Key, int Steps);
    int FitInto(int Key);
    static int Analyze(JZFilter* pFilter);        // returns ScaleNr
};

//*****************************************************************************
//*****************************************************************************
class tCommand
{
  public:

    tCommand(JZFilter* pFilter);

    virtual ~tCommand();

    virtual void Execute(int NewUndo = 1);

    virtual void ExecuteTrack(JZTrack* pTrack);

    virtual void ExecuteEvent(JZTrack* pTrack, JZEvent* pEvent);

    int Interpolate(int Clock, int vmin, int vmax);

  public:

    JZFilter* mpFilter;
    JZSong* mpSong;
    bool mReverse;
};

//*****************************************************************************
//*****************************************************************************
class tCmdShift : public tCommand
{
  public:

    tCmdShift(JZFilter* pFilter, long DeltaClock);
    virtual void ExecuteEvent(JZTrack* pTrack, JZEvent* pEvent);

  private:

    long mDeltaClock;
};

//*****************************************************************************
//*****************************************************************************
class tCmdErase : public tCommand
{
  public:
    int LeaveSpace;
    tCmdErase(JZFilter* pFilter, int LeaveSpace = 1);
    virtual void Execute(int NewUndo = 1);
    virtual void ExecuteEvent(JZTrack* pTrack, JZEvent* pEvent);
};

//*****************************************************************************
//*****************************************************************************
class tCmdVelocity : public tCommand
{
  public:
    int FromValue, ToValue, Mode;
    tCmdVelocity(JZFilter* pFilter, int From, int To, int Mode);
    virtual void ExecuteEvent(JZTrack* pTrack, JZEvent* pEvent);
};

//*****************************************************************************
//*****************************************************************************
class tCmdLength : public tCommand
{
  public:
    int FromValue, ToValue, Mode;
    tCmdLength(JZFilter* pFilter, int From, int To, int Mode);
    virtual void ExecuteEvent(JZTrack* pTrack, JZEvent* pEvent);
};

//*****************************************************************************
//*****************************************************************************
class tCmdSeqLength : public tCommand
{
  public:
  double scale;
  long startClock;
    tCmdSeqLength(JZFilter* pFilter, double scale);
    virtual void ExecuteEvent(JZTrack* pTrack, JZEvent* pEvent);
};

//*****************************************************************************
//*****************************************************************************
class tCmdMidiDelay : public tCommand
{
  public:
  double scale;
  long clockDelay;
  int repeat;

    tCmdMidiDelay(
      JZFilter* pFilter,
      double scale,
      long clockDelay,
      int repeat);
    virtual void ExecuteEvent(JZTrack* pTrack, JZEvent* pEvent);
};

//*****************************************************************************
//*****************************************************************************
class tCmdConvertToModulation : public tCommand
{
  public:

  tCmdConvertToModulation(JZFilter* pFilter);
  virtual void ExecuteTrack(JZTrack* pTrack);
};

//*****************************************************************************
//*****************************************************************************
class tCmdCleanup : public tCommand
{
    long lengthLimit;
    int  shortenOverlaps;
    tKeyOn *prev_note[16][128];
  public:
    tCmdCleanup(JZFilter* pFilter, long limitClocks, int shortenOverlaps);
    virtual void ExecuteTrack(JZTrack* pTrack);
    virtual void ExecuteEvent(JZTrack* pTrack, JZEvent* pEvent);
};

//*****************************************************************************
//*****************************************************************************
class tCmdSearchReplace : public tCommand
{
  short fr, to;
  public:
    tCmdSearchReplace(JZFilter* pFilter, short fr, short to);
    virtual void ExecuteEvent(JZTrack* pTrack, JZEvent* pEvent);
};

//*****************************************************************************
//*****************************************************************************
class tCmdQuantize : public tCommand
{
    long Quantize(long Clock, int islen);
  public:
    long QntClocks;
    int NoteStart;     // yes
    int NoteLength;    // no
    int Delay;         // zero
    int Groove;        // zero
    tCmdQuantize(JZFilter* pFilter, long QntClocks, int groove, int delay);
    virtual void ExecuteEvent(JZTrack* pTrack, JZEvent* pEvent);
};

//*****************************************************************************
//*****************************************************************************
class tCmdTranspose : public tCommand
{
  public:
    int Notes;
    int FitIntoScale;
    tScale Scale;
    tCmdTranspose(
      JZFilter* pFilter,
      int Notes,
      int ScaleNr = 0,
      int FitIntoScale = 0);
    virtual void ExecuteEvent(JZTrack* pTrack, JZEvent* pEvent);
};

//*****************************************************************************
//*****************************************************************************
class tCmdSetChannel : public tCommand
{
  public:
    int NewChannel;        // 0
    tCmdSetChannel(JZFilter* pFilter, int NewChannel);
    virtual void ExecuteEvent(JZTrack* pTrack, JZEvent* pEvent);
};

//*****************************************************************************
//*****************************************************************************
class tCmdCopyToBuffer : public tCommand
{
  public:

    tCmdCopyToBuffer(JZFilter* pFilter, tEventArray *Buffer);

    virtual void ExecuteEvent(JZTrack* pTrack, JZEvent* pEvent);

  private:

    tEventArray* mpBuffer;
};

//*****************************************************************************
//*****************************************************************************
class tCmdCopy : public tCommand
{
  public:
    int  DestTrack;
    long DestClock;

    int EraseSource;        // no
    int EraseDestin;        // yes
    int InsertSpace;        // no
    long RepeatClock;        // -1L

    tCmdCopy(JZFilter* pFilter, long DestTrack, long DestClock);
    virtual void ExecuteTrack(JZTrack* pTrack);
};

//*****************************************************************************
//*****************************************************************************
class tCmdExchLeftRight : public tCommand
{
  public:
    tCmdExchLeftRight(JZFilter* pFilter);
    virtual void ExecuteEvent(JZTrack* pTrack, JZEvent* pEvent);
};

//*****************************************************************************
//*****************************************************************************
class tCmdExchUpDown : public tCommand
{
  public:
    tCmdExchUpDown(JZFilter* pFilter);
    virtual void ExecuteTrack(JZTrack* pTrack);
};

//*****************************************************************************
//*****************************************************************************
class tCmdMapper : public tCommand
{
  public:

    enum prop
    {
      veloc,
      length,
      key,
      rhythm,
      random,
      pan,
      modul,
      cc1,
      cc2,
      pitch,
      clock
    };

    tCmdMapper(
      JZFilter* pFilter,
      prop Destination,
      prop dst,
      JZRndArray& RandomArray,
      int BarCount,
      bool Add);

    virtual ~tCmdMapper();

    virtual void ExecuteEvent(JZTrack* pTrack, JZEvent* pEvent);

  private:
    int mBarCount;
    int mStartBar;
    bool mAdd;
    prop mSource;
    prop mDestination;
    JZBarInfo* mpBarInfo;
    JZRndArray& mRandomArray;
};

#endif // !defined(JZ_COMMAND_H)
