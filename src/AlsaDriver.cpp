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
//
// ALSA audio player for ALSA 1.0.15 driver
// Copyright (c) 2000 Takashi Iwai <tiwai@suse.de>
//
// Description:
//   Mostly based on audiodrv.c.
//
// Aug. 11, 2000
//   Initial version: only playback is tested.
//*****************************************************************************

#include "AlsaDriver.h"

#include "TrackFrame.h"
#include "RecordingInfo.h"
#include "Configuration.h"
#include "Globals.h"

#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <sys/ioctl.h>
#include <unistd.h>

using namespace std;

#define MAX_FRAGS  16  // enough large?

class tAlsaAudioListener : public wxTimer
{
  public:

    tAlsaAudioListener(tAlsaAudioPlayer* pPlayer, int key)
      : wxTimer(),
        mpPlayer(pPlayer),
        mHardExit(true)
    {
      mpPlayer->mpListener = this;
      mpPlayer->mpRecordingInfo = 0;  // not recording!
      mpPlayer->running_mode = 0;

      // SYNC seems not to work?? so add 8 more silent buffers
      // to hear the end of the sample too.
      mpPlayer->OpenDsp(tAlsaAudioPlayer::PLAYBACK, 0);
      mCount = 8 + mpPlayer->mSamples.PrepareListen(key);
      Start(20);
    }

    tAlsaAudioListener(
      tAlsaAudioPlayer* pPlayer,
      tSample& spl,
      long fr_smpl,
      long to_smpl)
      : wxTimer(),
        mpPlayer(pPlayer),
        mHardExit(true)
    {
      mpPlayer->mpListener = this;
      mpPlayer->mpRecordingInfo = 0;  // not recording!
      mpPlayer->running_mode = 0;

      mpPlayer->OpenDsp(tAlsaAudioPlayer::PLAYBACK, 0);

      mpPlayer->mSamples.ResetBufferSize(
        mpPlayer->frag_byte_size[tAlsaAudioPlayer::PLAYBACK]);

      mCount = 8 + mpPlayer->mSamples.PrepareListen(&spl, fr_smpl, to_smpl);

      Start(20);
    }

    ~tAlsaAudioListener()
    {
      Stop();
      mpPlayer->CloseDsp(mHardExit);
      mpPlayer->mpListener = 0;
    }

    virtual void Notify()
    {
      mCount -= mpPlayer->WriteSamples();
      mCount += mpPlayer->mSamples.ContinueListen();
      if (mCount <= 0)
      {
        mHardExit = false;
        delete this;
      }
    }

    long GetPlayPosition()
    {
      return mpPlayer->GetCurrentPosition(tAlsaAudioPlayer::PLAYBACK);
    }

  private:

    tAlsaAudioPlayer* mpPlayer;

    int mCount;

    bool mHardExit;
};


tAlsaAudioPlayer::tAlsaAudioPlayer(JZSong* pSong)
  : tAlsaPlayer(pSong)
{
  mpAudioBuffer = new tEventArray();
  mInstalled    = false;
  mAudioEnabled = false;
  mpListener    = 0;
  mCanDuplex    = 0;    // no duplex yet.
  pcm[PLAYBACK] = NULL;
  pcm[CAPTURE] = NULL;

  mDeviceNames[PLAYBACK] = gpConfig->GetStrValue(C_AlsaAudioOutputDevice);
  mDeviceNames[CAPTURE] = gpConfig->GetStrValue(C_AlsaAudioInputDevice);

  // FIXME
  mCanDuplex = 1;
  mInstalled = true;
  mAudioEnabled = true;
}


tAlsaAudioPlayer::~tAlsaAudioPlayer()
{
  delete mpListener;
  delete mpAudioBuffer;
  if (pcm[PLAYBACK])
  {
    snd_pcm_close(pcm[PLAYBACK]);
    pcm[PLAYBACK] = NULL;
  }
  if (pcm[CAPTURE])
  {
    snd_pcm_close(pcm[CAPTURE]);
    pcm[CAPTURE] = NULL;
  }
}


int tAlsaAudioPlayer::LoadSamples(const char *filename)
{
  return mSamples.Load(filename);
}

int tAlsaAudioPlayer::RecordMode() const
{
  return running_mode & (1 << CAPTURE);
}

int tAlsaAudioPlayer::PlayBackMode() const
{
  return running_mode & (1 << PLAYBACK);
}

void tAlsaAudioPlayer::StartPlay(long clock, long loopClock, int cont)
{
  delete mpListener;
  mSamples.StartPlay(clock);

  tAlsaPlayer::StartPlay(clock, loopClock, cont);
  if (!mAudioEnabled)
  {
    return;
  }

  long ticks_per_minute = mpSong->GetTicksPerQuarter() * mpSong->Speed();
  mSamples.ResetBuffers(mpAudioBuffer, clock, ticks_per_minute);
  last_scount = 0;
  cur_pos = 0;
  audio_clock_offset = clock;
  midi_speed  = mpSong->Speed();
  curr_speed  = midi_speed;

  running_mode = 0;
  if (mpRecordingInfo && mpRecordingInfo->mpTrack->GetAudioMode())
  {
    OpenDsp(CAPTURE, 1);
    recbuffers.ResetBufferSize(frag_byte_size[CAPTURE]);
  }

  if (
    mDeviceNames[CAPTURE] != mDeviceNames[PLAYBACK] ||
    mCanDuplex ||
    running_mode == 0)
  {
    OpenDsp(PLAYBACK, 1);
    mSamples.ResetBufferSize(frag_byte_size[PLAYBACK]);
    mSamples.FillBuffers(mOutClock);
  }

  if (running_mode == 0)
  {
    mAudioEnabled = false;
    return;
  }

  // ok, suspend the device until midi starts
  if (PlayBackMode())
  {
    WriteSamples();
  }
  compose_echo(clock, 1); // trigger echo
}


void tAlsaAudioPlayer::StartAudio()
{
  if (pcm[PLAYBACK])
  {
    snd_pcm_start(pcm[PLAYBACK]);
  }
  if (pcm[CAPTURE])
  {
    snd_pcm_start(pcm[CAPTURE]);
  }
}


void tAlsaAudioPlayer::OpenDsp(int mode, int sync_mode)
{
  if (!mAudioEnabled)
  {
    return;
  }

  unsigned int channels;
  snd_pcm_format_t format;
  snd_pcm_uframes_t buffer_size, period_size;

  frame_shift[mode] = 0;
  if (mSamples.GetBitsPerSample() == 8)
  {
    format = SND_PCM_FORMAT_U8;
  }
  else
  {
    format = SND_PCM_FORMAT_S16_LE;
    frame_shift[mode]++;
  }
  channels =  mSamples.GetChannelCount();
  if (channels > 1)
  {
    frame_shift[mode]++;
  }

  snd_pcm_stream_t stream = (mode == PLAYBACK) ?
    SND_PCM_STREAM_PLAYBACK : SND_PCM_STREAM_CAPTURE;
  if (
    snd_pcm_open(
      &pcm[mode],
      mDeviceNames[mode].c_str(),
      stream,
      SND_PCM_NONBLOCK) < 0)
  {
    perror("snd_pcm_open");
    mAudioEnabled = false;
    return;
  }

  snd_pcm_hw_params_t *hw;
  snd_pcm_hw_params_alloca(&hw);
  snd_pcm_hw_params_any(pcm[mode], hw);
  if (snd_pcm_hw_params_set_access(pcm[mode], hw, SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
  {
    perror("cannot set interleaved access");
    goto __error;
  }
  if (snd_pcm_hw_params_set_format(pcm[mode], hw, format) < 0)
  {
    perror("cannot set audio format");
    goto __error;
  }
  if (snd_pcm_hw_params_set_channels(pcm[mode], hw, channels) < 0)
  {
    perror("cannot set audio channels");
    goto __error;
  }
  if (
    snd_pcm_hw_params_set_rate(pcm[mode], hw, mSamples.GetSamplingRate(), 0) <
    0)
  {
    cerr  << "cannot set audio rate: " << mSamples.GetSamplingRate() << endl;
    goto __error;
  }

  period_size = FRAGBYTES >> frame_shift[mode];
  if (
    (period_size = snd_pcm_hw_params_set_period_size_near(pcm[mode], hw, &period_size, 0)) < 0)
  {
    perror("cannot set audio period");
    goto __error;
  }
  buffer_size = period_size * MAX_FRAGS;
  if (
    (buffer_size = snd_pcm_hw_params_set_buffer_size_near(pcm[mode], hw, &buffer_size)) < 0)
  {
    perror("cannot set audio buffer");
    goto __error;
  }
  if (snd_pcm_hw_params(pcm[mode], hw) < 0)
  {
    perror("snd_pcm_hw_params");
    goto __error;
  }

  frag_size[mode] = period_size; /* in frames */
  frag_byte_size[mode] = period_size << frame_shift[mode];
  frame_boundary[mode] = 0x7fffffff;

  snd_pcm_sw_params_t *sw;

  snd_pcm_sw_params_alloca(&sw);
  snd_pcm_sw_params_current(pcm[mode], sw);
  if (sync_mode)
  {
    // FIXME
    snd_pcm_sw_params_set_start_threshold(pcm[mode], sw, 0x7fffffff);
  }
  else
  {
    snd_pcm_sw_params_set_start_threshold(pcm[mode], sw, 1);
  }
  if (snd_pcm_sw_params(pcm[mode], sw) < 0)
  {
    perror("snd_pcm_sw_params");
    goto __error;
  }

  running_mode |= (1 << mode);

  return;

__error:
  snd_pcm_close(pcm[mode]);
  pcm[mode] = NULL;
  mAudioEnabled = false;
  return;
}


void tAlsaAudioPlayer::CloseDsp(bool Reset)
{
  if (pcm)
  {
    if (Reset)
    {
      if (pcm[PLAYBACK])
      {
        if (snd_pcm_drop(pcm[PLAYBACK]) < 0)
        {
          perror("playback drop");
        }
      }
    }
    else
    {
      if (pcm[PLAYBACK])
      {
        if (snd_pcm_drain(pcm[PLAYBACK]) < 0)
        {
          perror("playback drain");
        }
      }
      if (pcm[CAPTURE])
      {
        if (snd_pcm_drain(pcm[CAPTURE]) < 0)
        {
          perror("capture drain");
        }
      }
    }
    if (pcm[PLAYBACK])
    {
      snd_pcm_close(pcm[PLAYBACK]);
      pcm[PLAYBACK] = NULL;
    }
    if (pcm[CAPTURE])
    {
      snd_pcm_close(pcm[CAPTURE]);
      pcm[CAPTURE] = NULL;
    }
  }
}

void tAlsaAudioPlayer::Notify()
{
  if (mAudioEnabled)
  {
    if (pcm[PLAYBACK])
    {
      WriteSamples();

      // The code may hang here when swapping in pages.
      mSamples.FillBuffers(mOutClock);

      WriteSamples();
    }

    if (pcm[CAPTURE])
    {
      ReadSamples();
    }

    if (pcm[PLAYBACK] && mSamples.softsync)
    {
      MidiSync();
    }
  }
  tAlsaPlayer::Notify();
}

// number of frames (or bytes) free
int tAlsaAudioPlayer::GetFreeSpace(int mode)
{
  snd_pcm_status_t *info;
  snd_pcm_status_alloca(&info);
  if (snd_pcm_status(pcm[mode], info) < 0)
  {
    perror("snd_pcm_status");
    return 0;
  }
  return snd_pcm_status_get_avail(info); /* in frames */
}


int tAlsaAudioPlayer::WriteSamples()
{
  if (!mAudioEnabled || pcm[PLAYBACK] == NULL)
  {
    return 0;
  }

  int blocks_written = 0;
  int room;

  room = GetFreeSpace(PLAYBACK);

  for (; room > frag_size[PLAYBACK]; room -= frag_size[PLAYBACK])
  {
    tAudioBuffer *buf = mSamples.full_buffers.Get();
    if (buf == 0)
    {
      break;
    }

    ssize_t written = snd_pcm_writei(
      pcm[PLAYBACK],
      buf->Data(),
      frag_size[PLAYBACK]);

    if (written < 0)
    {
      if (written == -EPIPE)
      {
        cerr << "xrun!!" << endl;
        snd_pcm_prepare(pcm[PLAYBACK]);
      }
      else
      {
        perror("audio write");
      }
    }
    if (written > 0)
    {
      cur_scount += written;
    }
    blocks_written++;
    mSamples.free_buffers.Put(buf);
  }

  return blocks_written;
}


void tAlsaAudioPlayer::ReadSamples()
{
  if (!mAudioEnabled || pcm[CAPTURE] == NULL)
  {
    return;
  }

  int room = GetFreeSpace(CAPTURE);

  for (; room > frag_size[CAPTURE]; room -= frag_size[CAPTURE])
  {
    short *b = recbuffers.RequestBuffer()->data;
    if (snd_pcm_readi(pcm[CAPTURE], b, frag_size[CAPTURE]) !=
        frag_size[CAPTURE])
    {
      recbuffers.UndoRequest();
      break;
    }
  }
}


void tAlsaAudioPlayer::ResetPlay(long clock)
{
  tAlsaPlayer::ResetPlay(clock);
  if (pcm[PLAYBACK])
  {
    snd_pcm_drop(pcm[PLAYBACK]);
//    long ticks_per_minute = mpSong->GetTicksPerQuarter() * mpSong->Speed();
//    mSamples.ResetBuffers(mpAudioBuffer, clock, ticks_per_minute);
  }
  audio_clock_offset = clock;
  cur_pos = 0;
}

long tAlsaAudioPlayer::GetCurrentPosition(int mode)
{
  return cur_scount;
}

void tAlsaAudioPlayer::MidiSync()
{
  if (!mAudioEnabled)
  {
    return;
  }

  int mode;
  if (pcm[PLAYBACK])
  {
    mode = PLAYBACK;
  }
  else if (pcm[CAPTURE])
  {
    mode = CAPTURE;
  }
  else
  {
    return; // disabled
  }

  long scount = GetCurrentPosition(mode);

  // get realtime info for audio/midi sync
  if (scount != last_scount)
  {
    unsigned int qtick;
    snd_seq_queue_status_t *status;
    snd_seq_queue_status_alloca(&status);
    if (snd_seq_get_queue_status(handle, queue, status) < 0)
    {
      perror("snd_seq_get_queue_status");
      return;
    }
    qtick = snd_seq_queue_status_get_tick_time(status);
    int samplediff;
    if (scount < last_scount)
    {
      samplediff = frame_boundary[mode] - (last_scount - scount);
    }
    else
    {
      samplediff = scount - last_scount;
    }
    last_scount = scount;
    cur_pos += samplediff;
    long audio_clock =
      (long)mSamples.Samples2Ticks(cur_pos) + audio_clock_offset;
    int delta_clock = audio_clock - qtick;
    int new_speed = midi_speed + delta_clock;

    // limit speed changes to some reasonable values
    const int limit = 1;
    if (new_speed > midi_speed + limit)
    {
      new_speed = midi_speed + limit;
    }
    if (midi_speed - limit > new_speed)
    {
      new_speed = midi_speed - limit;
    }

    if (new_speed != curr_speed)
    {
      snd_seq_event_t ev;
      memset(&ev, 0, sizeof(ev));
      snd_seq_ev_set_source(&ev, self.port);
      snd_seq_ev_set_subs(&ev);
      snd_seq_ev_set_direct(&ev);
      snd_seq_ev_set_fixed(&ev);
      int us  = (int)(60.0E6 / new_speed);
      snd_seq_ev_set_queue_tempo(&ev, queue, us);
      write(&ev, 1);
      curr_speed = new_speed;
    }
  }
}

void tAlsaAudioPlayer::StopPlay()
{
  mSamples.StopPlay();
  tAlsaPlayer::StopPlay();
  if (!mAudioEnabled)
  {
    return;
  }

  CloseDsp(true);
  if (RecordMode())
  {
    long frc = mpRecordingInfo->mFromClock;
    if (frc < audio_clock_offset)
    {
      frc = audio_clock_offset;
    }
    long toc = mpRecordingInfo->mToClock;
    if (toc > recd_clock)
    {
      toc = recd_clock;
    }
    mSamples.SaveRecordingDlg(frc, toc, recbuffers);
  }
  recbuffers.Clear();
}

void tAlsaAudioPlayer::ListenAudio(int key, int start_stop_mode)
{
  if (!mAudioEnabled)
  {
    return;
  }

  // If already listening then stop listening.
  if (mpListener)
  {
    delete mpListener;
    mpListener = 0;
    if (start_stop_mode)
    {
      return;
    }
  }
  if (key < 0)
  {
    return;
  }

  if (pcm[PLAYBACK])  // device busy (playing)
  {
    return;
  }

  mpListener = new tAlsaAudioListener(this, key);
}

void tAlsaAudioPlayer::ListenAudio(tSample& spl, long fr_smpl, long to_smpl)
{
  if (!mAudioEnabled)
  {
    return;
  }

  // If already listening then stop listening.
  if (mpListener)
  {
    delete mpListener;
    mpListener = 0;
  }

  if (pcm[PLAYBACK])  // device busy (playing)
  {
    return;
  }
  mpListener = new tAlsaAudioListener(this, spl, fr_smpl, to_smpl);
}

long tAlsaAudioPlayer::GetListenerPlayPosition()
{
  if (!mpListener)
  {
    return -1L;
  }
  return mpListener->GetPlayPosition();
}
