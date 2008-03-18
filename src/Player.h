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

#ifndef JZ_PLAYER_H
#define JZ_PLAYER_H

#include <wx/timer.h>

#include "config.h"
#include "Events.h"
#include "Track.h"
#include "Song.h"
//#include "midinet.h"
#include "Audio.h"
#include "DeprecatedStringUtils.h"

#include <sys/types.h>
#include <time.h>


// audio-menu
#define MEN_AUDIO_LOAD     200
#define MEN_AUDIO_SAVE     201
#define MEN_AUDIO_GLOBAL   202
#define MEN_AUDIO_SAMPLES  203
#define MEN_AUDIO_NEW      204
#define MEN_AUDIO_SAVE_AS  205


class tPlayLoop
{
  public:

    tPlayLoop();

    void Set(long Start, long Stop);

    void Reset();

    // external clock -> internal clock where
    //   external clock == physical clock
    //   internal clock == song position
    long Ext2IntClock(long Clock);

    // the other way round
    long Int2ExtClock(long Clock);

    void PrepareOutput(
      tEventArray *buf,
      JZSong* pSong,
      long ExtFr,
      long ExtTo,
      int mode = 0);

  private:

    long mStartClock;
    long mStopClock;
};

enum tClockSource { CsInt = 0, CsFsk, CsMidi, CsMtc };

class JZRecordingInfo;


class tDeviceList
{
  public:

    enum
    {
      eMaximumDeviceCount = 10
    };

    tDeviceList();

    tDeviceList(const char* pName);

    virtual ~tDeviceList();

    unsigned GetCount() const
    {
      return mDeviceNames.size();
    }

    const std::string& GetName(unsigned i) const
    {
      if (i < mDeviceNames.size())
      {
        return mDeviceNames[i];
      }
      static std::string Unknown("Unknown");
      return Unknown;
    }

    unsigned Add(const char* pName)
    {
      mDeviceNames.push_back(pName);
      return mDeviceNames.size();
//      if (count < eMaximumDeviceCount)
//      {
//        names[count] = copystring(name);
//        return count++;
//      }
//      return eMaximumDeviceCount;
    }

    void Clear()
    {
      mDeviceNames.clear();
//      count = 0;
    }

//    tNamedValue* AsNamedValue();

  protected:

//    int count;
//    char *names[eMaximumDeviceCount];
    std::vector<std::string> mDeviceNames;

  private:

    // Prevent accidental copy or assignment.
    tDeviceList(const tDeviceList &);
    tDeviceList& operator = (const tDeviceList &);
};

class tPlayer : public wxTimer
{
  protected:

    long OutClock;
    tPlayLoop* PlayLoop;
    // timer value for polling the record queue
    int poll_millisec;
    JZRecordingInfo* rec_info;   // 0 == not recording


  public:

    bool Playing;        // successful StartPlay

    virtual int Installed() = 0;        // Hardware found
    // if unable to install, pop up a messagebox explaining why.
    virtual void ShowError();

    JZSong *Song;
    tEventArray mPlayBuffer;
    tEventArray RecdBuffer;
    void SetRecordInfo(JZRecordingInfo* inf)
    {
      rec_info = inf;
    }

    bool IsPlaying() const
    {
      return Playing;
    }

    virtual int FindMidiDevice()
    {
      return -1;
    }

    virtual int SupportsMultipleDevices() { return 0; }
    virtual tDeviceList & GetOutputDevices() { return DummyDeviceList; }
    virtual tDeviceList & GetInputDevices() { return DummyDeviceList; }
    virtual int GetThruInputDevice() { return 0; }
    virtual int GetThruOutputDevice() { return 0; }

    // Audio stuff
    virtual void StartAudio() {}
    tEventArray *AudioBuffer;
    virtual int GetAudioEnabled() const { return 0; }
    virtual void SetAudioEnabled(int) { }
    virtual void ListenAudio(int key, int start_stop_mode = 1) {}
    virtual void ListenAudio(tSample &spl, long fr_smpl, long to_smpl) {}
    virtual bool IsListening() const { return 0; }

    virtual int OnMenuCommand(int id) {
      if (Playing)
      {
        return 0;
      }
      return samples.OnMenuCommand(id);
    }
    virtual const char *GetSampleName(int i) {
      return samples.GetSampleName(i);
    }
    virtual void AdjustAudioLength(JZTrack *t) {
      long ticks_per_minute = Song->TicksPerQuarter * Song->Speed();
      samples.AdjustAudioLength(t, ticks_per_minute);
    }
    void EditSample(int key) {
      samples.Edit(key);
    }

    virtual long GetListenerPlayPosition()
    {
      return -1L;
    }

    void LoadDefaultSettings()
    {
      samples.LoadDefaultSettings();
    }

  protected:
    tSampleSet samples;

  public:
    tPlayer(JZSong *song);
    virtual ~tPlayer();

    void Notify();

    virtual void FlushToDevice();

    // return 0 = ok, 1 = buffer full, try again later
    virtual int OutEvent(JZEvent* pEvent) = 0;

    virtual void OutBreak() = 0;

    // send event immediately ignoring clock
    void OutNow(JZTrack *t, JZEvent* pEvent)
    {
      pEvent->SetDevice(t->GetDevice());
      OutNow(pEvent);
    }

    void OutNow(int device, JZEvent* pEvent)
    {
      pEvent->SetDevice(device);
      OutNow(pEvent);
    }

    void OutNow(JZTrack *t, tParam *r);

    // what's played right now?
    virtual long GetRealTimeClock() = 0;

    virtual void StartPlay(long Clock, long LoopClock = 0, int Continue = 0);
    virtual void StopPlay();
    virtual void AllNotesOff(int Reset = 0);

    virtual void SetSoftThru(int on, int idev, int odev)
    {
    }

    virtual void SetHardThru(int on, int idev, int odev)
    {
    }

    virtual void InitMtcRec()
    {
    }

    virtual tMtcTime* FreezeMtcRec()
    {
      return 0;
    }

  protected:

    virtual void OutNow(JZEvent* pEvent) = 0;
    
  private:

    tDeviceList DummyDeviceList;
};

extern char *midinethost;

// --------------------------------------------------------
// Roland MPU 401
// --------------------------------------------------------

#ifdef DEV_MPU401

#include <unistd.h>
#include <fcntl.h>

class tBuffer : public tWriteBase
{

    char Buffer[2000];
    int  Written, Read;

  public:

    long Clock;
    int  RunningStatus;

    void Clear()
    {
      Read = Written = 0;
      Clock = 0;
      RunningStatus = 0;
    }

    tBuffer()
    {
      Clear();
    }

    int Put(char c)
    {
      if (Written < (int)sizeof(Buffer))
      {
        Buffer[Written++] = c;
        return 0;
      }
      return -1;
    }

    int Get(int dev)
    {
      if (Read == Written)
        ReadFile(dev);
      if (Read != Written)
        return (unsigned char)Buffer[Read++];
      return -1;
    }

    int Empty()
    {
      return Read == Written;
    }

    int FreeBytes()
    {
      return (int)sizeof(Buffer) - Written;
    }

    void PutVar(long val)
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
        Put((unsigned char)buf);
        if (buf & 0x80)
          buf >>= 8;
        else
          break;
      }
    }

    long GetVar(int dev)
    {
      unsigned long val;
      int c;
      val = Get(dev);
      if (val & 0x80)
      {
        val &= 0x7f;
        do
        {
          c = Get(dev);
          assert(c > 0);
          val = (val << 7) + (c & 0x7f);
        } while (c & 0x80);
      }
      return val;
    }

    int WriteFile(int dev)
    {
      int bytes;
      if (Read != Written)
      {
        bytes = write_noack_mpu(Buffer + Read, Written - Read);
        if (bytes > 0)
          Read += bytes;
      }
      if (Read != Written)
        return 0;
      Read = Written = 0;
      return 1;
    }

    int ReadFile(int dev)
    {
      int i, bytes;
      if (Read)        // move data to beginning of buffer
      {
        for (i = 0; i < Written - Read; i++)
          Buffer[i] = Buffer[i + Read];
        Written -= Read;
        Read = 0;
      }
      non_block_io( dev, 1 );
      bytes = read(dev, Buffer + Written, sizeof(Buffer) - Written);
      non_block_io( dev, 0 );
      if (bytes > 0)
      {
        Written += bytes;
        return bytes;
      }
      return 0;
    }

    int Write(JZEvent *e, unsigned char *data, int len)
    {
      int i;
      for (i = 0; i < len; i++)
        Put(data[i]);
      return 0;
    }
};


// ---------------------------- jazz-driver -----------------------------

// Define 0xfa to mean start-play-command (filtered by midinetd)
#define START_PLAY_COMMAND 0xfa

// Define 0xfb to mean stop-play-command (filtered by midinetd)
#define STOP_PLAY_COMMAND 0xfb

// 0xfb also sent by midinetd to mark start of recorded data
#define START_OF_RECORD_BUFFER 0xfb

#define ACTIVE_TRACKS 7
#define ACTIVE_TRACKS_MASK 0x7f

class tMpuPlayer : public tPlayer
{
    int  dev;
    tBuffer PlyBytes;
    tBuffer RecBytes;
    long playclock;
    int clock_to_host_counter;

    int ActiveTrack;
    long TrackClock[ACTIVE_TRACKS];
    int TrackRunningStatus[ACTIVE_TRACKS];

    tEventArray OutOfBandEvents;

  public:

    tMpuPlayer(JZSong *song);
    virtual ~tMpuPlayer();
    int  OutEvent(JZEvent *e);
    void OutNow(JZEvent *e);
    void OutBreak();
    void OutBreak(long BreakOver);
    void StartPlay(long Clock, long LoopClock = 0, int Continue = 0);
    void StopPlay();
    long GetRealTimeClock();
    int  Installed();
    long GetRecordedData();
    void SetHardThru(int on, int idev, int odev);

    void FlushOutOfBand( long Clock );
};

#define TRK (0<<6)
#define DAT (1<<6)
#define CMD (2<<6)
#define RES (3<<6)

#define MPUDEVICE "/dev/mpu401"
#define MIDINETSERVICE "midinet"

#endif // DEV_MPU401

// ------------------------------ null-driver -------------------------------

class tNullPlayer : public tPlayer
{
  public:

    tNullPlayer(JZSong* pSong)
      : tPlayer(pSong)
    {
    }

    int Installed()
    {
      return 1;
    }

    virtual ~tNullPlayer()
    {
    }

    int OutEvent(JZEvent* pEvent)
    {
      return 0;
    }

    void OutNow(JZEvent* pEvent)
    {
    }

    void OutBreak()
    {
    }

    void StartPlay(long Clock, long LoopClock = 0, int Continue = 0)
    {
    }

    void StopPlay()
    {
    }

    long GetRealTimeClock()
    {
      return 0;
    }
};

// --------------------------- voxware driver -------------------------------

#ifdef DEV_SEQUENCER2

#include <sys/soundcard.h>

SEQ_USE_EXTBUF();
extern int seqfd;
extern int mididev;
void seqbuf_dump(void);
#define seqbuf_empty() (_seqbufptr == 0)
#define seqbuf_clear() (_seqbufptr = 0)
void seqbuf_flush_last_event();


class tOSSThru : public wxTimer
{
  public:
    virtual void Notify();
    tOSSThru();
    ~tOSSThru();
};



class tSeq2Player : public tPlayer
{
  public:
    friend class tOSSThru;
    tSeq2Player(JZSong *song);
    int Installed();
    virtual ~tSeq2Player();
    int  OutEvent(JZEvent *e, int now);
    int  OutEvent(JZEvent *e) { OutEvent(e, 0); return 0; }
    void OutNow(JZEvent *e)   { OutEvent(e, 1); }
    void OutBreak();
    void OutBreak(long BreakOver);
    void StartPlay(long Clock, long LoopClock = 0, int Continue = 0);
    void StopPlay();
    long GetRealTimeClock();
    virtual void FlushToDevice();
    void SetSoftThru(int on, int idev, int odev);
    int     FindMidiDevice();


  protected:

    long    play_clock;
    long    recd_clock;
    long    start_clock;
    long    echo_clock;

    tOSSThru *through;
    int     card_id;
};

#endif // DEV_SEQUENCER2

#endif // !defined(JZ_PLAYER_H)
