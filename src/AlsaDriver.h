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

#ifndef JZ_ALSADRIVER_H
#define JZ_ALSADRIVER_H

#include "Events.h"
#include "Player.h"
#include "Audio.h"
#include "AlsaPlayer.h"

#include <sys/time.h>

class tSample;
class tAlsaAudioListener;

class tAlsaAudioPlayer : public tAlsaPlayer
{
  friend class tAlsaAudioListener;
  public:
    tAlsaAudioPlayer(JZSong *song);
    virtual ~tAlsaAudioPlayer();
    int LoadSamples(const char *filename);
    virtual void Notify();
    virtual void StartPlay(long Clock, long LoopClock = 0, int Continue = 0);
    virtual void StopPlay();
    virtual int Installed() { return installed && tAlsaPlayer::Installed(); }
    virtual int GetAudioEnabled() const { return audio_enabled; }
    virtual void SetAudioEnabled(int x) { audio_enabled = x; }
    virtual void ListenAudio(int key, int start_stop_mode = 1);
    virtual void ListenAudio(tSample &spl, long fr_smpl, long to_smpl);
    virtual bool IsListening() const {
      return listener != 0;
    }
    virtual long GetListenerPlayPosition();
    virtual void StartAudio();
    virtual void ResetPlay(long clock);

    enum { PLAYBACK = 0, CAPTURE };

    // for recording
    int RecordMode() const;
    int PlayBackMode() const;

  private:
    int can_duplex;	// TRUE = can do full duplex record/play

    int WriteSamples();
    void ReadSamples();
    void MidiSync();
    void OpenDsp(int mode, int sync_mode);
    void CloseDsp(int reset);

    long GetCurrentPosition(int mode);
    int GetFreeSpace(int mode);

    snd_pcm_t *pcm[2];
    int installed;

    long audio_clock_offset;
    long cur_pos;
    long last_scount;
    long cur_scount;
    int running_mode;
    int  midi_speed;  // start speed in bpm
    int  curr_speed;  // actual speed in bpm
    int  audio_enabled; // 0 means midi only

    int card; // card number in config
    const char *dev[2]; // device names
    long frag_size[2];
    long frag_byte_size[2];
    int frame_shift[2];
    long frame_boundary[2];

    tAlsaAudioListener *listener;
    tAudioRecordBuffer recbuffers;
};

#endif // !defined(JZ_ALSADRIVER_H)
