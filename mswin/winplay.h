
/*
** Copyright (C) 1994-1997 Andreas Voss and Per Sigmond, all rights reserved.
**
** License is granted to copy and distribute this software for any purpose,
** provided that the copyright notice and this license notice is included in
** all copies and in all related documentation.
** License is granted to use this software for non-commercial purposes only.
** The copyright holders grant no other licenses expressed or implied and
** the licensee acknowleges that the copyright holders have no liability for
** licensee's use.
**
** This software is provided AS IS.
**
** THE COPYRIGHT HOLDERS DISCLAIM AND LICENSEE AGREES THAT ALL WARRANTIES,
** EXPRESSED OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. NOTWITHSTANDING
** ANY OTHER PROVISION CONTAINED HEREIN, ANY LIABILITY FOR DAMAGES RESULTING
** FROM THE SOFTWARE OR ITS USE IS EXPRESSLY DISCLAIMED, INCLUDING
** CONSEQUENTIAL OR ANY OTHER INDIRECT DAMAGES, WHETHER ARISING IN CONTRACT,
** TORT (INCLUDING NEGLIGENCE) OR STRICT LIABILITY, EVEN IF THE COPYRIGHT
** HOLDERS ARE ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
*/

#ifndef winplay_h
#define winplay_h

#include "player.h"
extern "C" {
#include <mmsystem.h>
}
#include "jazzdll.h"



class tWinPlayer : public tPlayer
{
  public:
    tWinPlayer(tSong *song);
    int Installed();
    virtual ~tWinPlayer();
    virtual int OutEvent(tEvent *e);
    virtual int OutSysex(tEvent *e, DWORD time);
    void OutNow(tEvent *e);
    void OutNow(tParam *r);
    void OutBreak();
    virtual void OutBreak(long BreakOver);
    void StartPlay(long Clock, long LoopClock = 0, int Continue = 0);
    void StopPlay();
    virtual long GetRealTimeClock() = 0;
    virtual void FlushToDevice();
    void SetSoftThru(int on, int idev, int odev);
    virtual void InitMtcRec() { }
    virtual tMtcTime* FreezeMtcRec() { return(0); }

    static void SettingsDlg(long &idev, long &odev);

    enum { MAX_MIDI_DEVS = 10 };

  protected:

    tWinPlayerState *state;
    DWORD Event2Dword(tEvent *e);
    tEvent *Dword2Event(DWORD dw);
    long  Clock2Time(long clock);
    long  Time2Clock(long time);
    void  SetTempo(long bpm, long clock);
    BOOL  timer_installed;
    long  midiClockOut;
    long  lastValidMtcClock;
    void  FillMidiClocks( long to );
    void  FlushToDevice( long clock );

    tEventArray OutOfBandEvents;
    long  RealTimeClock2Time(long clock);
    long  Time2RealTimeClock(long time);
    void  SetRealTimeTempo(long bpm, long clock);
    long  real_start_time;
    long  real_ticks_per_minute;

    // buffer for sysexdata
    HANDLE hSysHdr;
    MIDIHDR *pSysHdr;
    HANDLE hSysBuf;
    unsigned char *pSysBuf;
    unsigned  int maxSysLen;
};

class tWinIntPlayer : public tWinPlayer
{
  public:
    tWinIntPlayer(tSong *song) : tWinPlayer( song ) { }
    virtual long GetRealTimeClock();
};

class tWinMidiPlayer : public tWinPlayer
{
  public:
    tWinMidiPlayer(tSong *song) : tWinPlayer( song ) { }
    virtual long GetRealTimeClock();
    virtual int OutEvent(tEvent *e);
    virtual void OutBreak(long clock);
};

class tWinMtcPlayer : public tWinPlayer
{
  public:
    tWinMtcPlayer(tSong *song) : tWinPlayer( song ) { }
    virtual long GetRealTimeClock();
    virtual void InitMtcRec();
    virtual tMtcTime* FreezeMtcRec();
};

#endif
