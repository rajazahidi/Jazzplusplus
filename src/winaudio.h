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

#ifndef winaudio_h
#define winaudio_h
#include "config.h"
#ifdef AUDIO

#include <mmsystem.h>
#include "winplay.h"
#include "audio.h"
#include "jazzdll.h"


class tSample;

class tWinAudioPlayer : public tWinIntPlayer
{
  friend class tAudioListener;
  public:
    tWinAudioPlayer(tSong *song);
    virtual ~tWinAudioPlayer();
    int LoadSamples(const char *filename);
    virtual void Notify();
    virtual void StartPlay(long Clock, long LoopClock = 0, int Continue = 0);
    virtual void StopPlay();
    virtual void StartAudio();   // called async by driver
    virtual int Installed() { return installed && tWinIntPlayer::Installed(); }
    virtual int GetAudioEnabled() const { return audio_enabled; }
    virtual void SetAudioEnabled(int x) { audio_enabled = x; }
    virtual void ListenAudio(int key, int start_stop_mode = 1);
    virtual void ListenAudio(tSample &spl, long fr_smpl, long to_smpl);
    virtual long GetListenerPlayPosition();

    virtual Bool IsListening() const {
      return listener != 0;
    }

    // for recording
    int RecordMode() const;
    int PlaybackMode() const {
      return !RecordMode() || can_duplex;
    }

    enum ErrorCode {
      NoError,
      ErrOutOpen, ErrOutPrepare, ErrOutUnprepare,
      ErrInpOpen, ErrInpPrepare, ErrInpUnprepare,
      ErrCapGet, ErrCapSync
    };
    ErrorCode GetError() {
      return error;
    }
    virtual void ShowError();

  private:
    ErrorCode error;

    int can_duplex;	// TRUE = can do full duplex record/play
    int can_sync;       // TRUE = can determine exact output play position

    int  OpenDsp();    // 0 = ok
    int  CloseDsp();   // 0 = ok

    int  installed;
    int  audio_enabled;   // 0 means midi only
    long blocks_played;   // # of blocks written to device
    int  play_buffers_needed;  // driver requests more output buffers

    long start_clock;     // when did play start
    long start_time;      // play start time (not altered by SetTempo)
    tAudioListener *listener;

    // ms specific
    friend void FAR PASCAL audioInterrupt(HWAVEOUT, UINT, DWORD, DWORD, DWORD);
    HWAVEOUT hout;
    HWAVEOUT hinp;
    void WriteBuffers();   // send samples.full_buffers to driver
    void AudioCallback(UINT msg);
    int  hout_open;        // true = playback device opended successful
    int  hinp_open;        // true = recording device opended successful

    tAudioRecordBuffer recbuffers;
    int  record_buffers_needed;  // driver needs more buffers

    // a semaphor for thread synchronization. Since Notify() and
    // the audio callback are not time critical, its safe to
    // let them wait for each other.
    CRITICAL_SECTION mutex;

};


#endif
#endif
