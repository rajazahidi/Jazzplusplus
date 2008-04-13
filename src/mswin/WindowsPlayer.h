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

#ifndef JZ_WINDOWSPLAYER_H
#define JZ_WINDOWSPLAYER_H

#include "Player.h"

extern "C"
{
#include <mmsystem.h>
}

#include "WindowsMidiInterface.h"

//*****************************************************************************
//*****************************************************************************
class JZWindowsPlayer : public JZPlayer
{
  public:

    JZWindowsPlayer(JZSong* pSong);

    int Installed();
    virtual ~JZWindowsPlayer();
    virtual int OutEvent(JZEvent* e);
    virtual int OutSysex(JZEvent* e, DWORD time);
    void OutNow(JZEvent *e);
    void OutNow(tParam *r);
    void OutBreak();
    virtual void OutBreak(long BreakOver);
    void StartPlay(long Clock, long LoopClock = 0, int Continue = 0);
    void StopPlay();
    virtual long GetRealTimeClock() = 0;
    virtual void FlushToDevice();
    void SetSoftThru(int on, int InputDevice, int OutputDevice);

    virtual void InitMtcRec()
    {
    }

    virtual tMtcTime* FreezeMtcRec()
    {
      return 0;
    }

    static void SettingsDlg(long& InputDevice, long& OutputDevice);

    enum { MAX_MIDI_DEVS = 10 };

  protected:

    tWinPlayerState* state;
    DWORD Event2Dword(JZEvent *e);
    JZEvent* Dword2Event(DWORD dw);
    long Clock2Time(long clock);
    long Time2Clock(long time);
    void SetTempo(long bpm, long clock);
    BOOL timer_installed;
    long midiClockOut;
    long lastValidMtcClock;
    void FillMidiClocks(long to);
    void FlushToDevice(long clock);

    tEventArray OutOfBandEvents;
    long RealTimeClock2Time(long clock);
    long Time2RealTimeClock(long time);
    void SetRealTimeTempo(long bpm, long clock);
    long real_start_time;
    long real_ticks_per_minute;

    // buffer for sysexdata
    HANDLE hSysHdr;
    MIDIHDR *pSysHdr;
    HANDLE hSysBuf;
    unsigned char *pSysBuf;
    unsigned short maxSysLen;
};

//*****************************************************************************
//*****************************************************************************
class JZWindowsIntPlayer : public JZWindowsPlayer
{
  public:

    JZWindowsIntPlayer(JZSong* pSong)
      : JZWindowsPlayer(pSong)
    {
    }

    virtual long GetRealTimeClock();
};

//*****************************************************************************
//*****************************************************************************
class JZWindowsMidiPlayer : public JZWindowsPlayer
{
  public:

    JZWindowsMidiPlayer(JZSong* pSong)
      : JZWindowsPlayer(pSong)
    {
    }

    virtual long GetRealTimeClock();
    virtual int OutEvent(JZEvent* pEvent);
    virtual void OutBreak(long clock);
};

//*****************************************************************************
//*****************************************************************************
class JZWindowsMtcPlayer : public JZWindowsPlayer
{
  public:
    JZWindowsMtcPlayer(JZSong* pSong)
      : JZWindowsPlayer(pSong)
    {
    }

    virtual long GetRealTimeClock();
    virtual void InitMtcRec();
    virtual tMtcTime* FreezeMtcRec();
};

#endif // !defined(JZ_WINDOWSPLAYER_H)
