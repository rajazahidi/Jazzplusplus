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

#include "wx/wx.h"

#include "config.h"
#include "audiodrv.h"
#include "jazz.h"	// MIN and MAX
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <errno.h>

#include <fcntl.h>

#include "trackwin.h"


#define db(a) cout << #a << " = " << a << endl

#define AUDIO_DEVICE "/dev/dsp"

class tAudioListener : public wxTimer
{
  public:
    tAudioListener(tAudioPlayer *p, int key) {
      hard_exit = TRUE;
      player = p;
      player->listener = this;
      player->rec_info = 0;  // not recording!

      // SYNC seems not to work?? so add 8 more silent buffers
      // to hear the end of the sample too.
      count = 8 + player->samples.PrepareListen(key);
      player->OpenDsp();
      Start(20);
    }

    tAudioListener(tAudioPlayer *p, tSample &spl, long fr_smpl, long to_smpl) {
      hard_exit = TRUE;
      player = p;
      player->listener = this;
      player->rec_info = 0;  // not recording!

      count = 8 + player->samples.PrepareListen(&spl, fr_smpl, to_smpl);
      player->OpenDsp();
      Start(20);
    }

    ~tAudioListener() {
      Stop();
      player->CloseDsp(hard_exit);
      player->listener = 0;
    }

    virtual void Notify() {
      count -= player->WriteSamples();
      count += player->samples.ContinueListen();
      if (count <= 0)
      {
        hard_exit = FALSE;
        delete this;
      }
    }

    long GetPlayPosition() {
      count_info cinfo;
      if (ioctl(player->dev, SNDCTL_DSP_GETOPTR, &cinfo) == -1)
	perror("SNDCTL_DSP_GETOPTR");
      return (cinfo.bytes - cinfo.ptr) / sizeof(short);
    }

  private:
    tAudioPlayer *player;
    int count;
    int hard_exit;
};



tAudioPlayer::tAudioPlayer(tSong *song)
  : tSeq2Player(song)
{
  long dummy = 0;
  AudioBuffer   = new tEventArray();
  installed     = 0;
  dummy = Config(C_EnableAudio);
  audio_enabled = dummy;
  listener      = 0;
  can_duplex    = 0;    // no duplex yet.
  dev           = -1;

  // check for device
  dev = open(AUDIO_DEVICE, O_WRONLY, 0);
  if (dev >= 0) {
    // check device caps
    int caps;
    ioctl(dev, SNDCTL_DSP_GETCAPS, &caps);
    if (caps & DSP_CAP_REALTIME)
      ; // fprintf(stderr, AUDIO " supports REALTIME, good!\n");
    if (caps & DSP_CAP_DUPLEX)
      can_duplex = 1;	// good soundcard!
    if (!(caps & DSP_CAP_TRIGGER))
      fprintf(stderr, "no CAP_TRIGGER support!\n");
    else
      installed = 1;
    close(dev);
  }
  else
    perror(AUDIO_DEVICE);
  dev = -1;  // closed
  audio_enabled = audio_enabled && installed;

}


tAudioPlayer::~tAudioPlayer()
{
  delete listener;
  delete AudioBuffer;
  if (dev >= 0)
    close(dev);
}


int tAudioPlayer::LoadSamples(const char *filename)
{
  return samples.Load(filename);
}

int tAudioPlayer::RecordMode() const {
  return rec_info != 0 && rec_info->Track->GetAudioMode();
}

void tAudioPlayer::StartAudio()
{
  if (!audio_enabled)
    return;

  long ticks_per_minute = Song->TicksPerQuarter * Song->Speed();
  samples.ResetBuffers(AudioBuffer, start_clock, ticks_per_minute);
  if (PlaybackMode())
    samples.FillBuffers(OutClock);

  audio_bytes = 0;
  midi_clock  = 0;
  midi_speed  = Song->Speed();
  curr_speed  = midi_speed;

  OpenDsp();

  if (Config(C_OssBug1))
    WriteSamples();
  else
  {
    // ok, suspend the device until midi starts
    ioctl(dev, SNDCTL_DSP_SETSYNCRO, 0);
    if (PlaybackMode())
      WriteSamples();
    SEQ_PLAYAUDIO(0xffff);  // start all available devices
  }

  force_read = 1;
}


void tAudioPlayer::OpenDsp()
{
  int tmp;

  if (!audio_enabled)
    return;

  can_duplex = Config(C_DuplexAudio);

  // linux driver seems to need some real free memory, which sometimes
  // is not available when operating with big samples. So allocate
  // some memory here, touch it and free it. Hopefully libc returns it
  // to the system and the driver grabs it on open(). NOT TESTED!
  {
    const int size = 0x10000; // 64K
    char *mem = (char *)malloc(size);
    memset(mem, 0, size);
    free(mem);
  }

  int mode = 0;
  if (can_duplex)
    mode = O_RDWR;
  else if (RecordMode())
    mode = O_RDONLY;
  else
    mode = O_WRONLY;

  dev = open(AUDIO_DEVICE, mode, 0);
  if (dev < 0) {
    perror(AUDIO_DEVICE);
    audio_enabled = 0;
    return;
  }

  if (can_duplex)
    ioctl(dev, SNDCTL_DSP_SETDUPLEX, 0);

  tmp = 0xffff0000 | FRAGBITS;
  if (ioctl(dev, SNDCTL_DSP_SETFRAGMENT, &tmp)==-1)
    perror("ioctl DSP_SETFRAGMENT");

  tmp = samples.BitsPerSample();
  ioctl(dev, SNDCTL_DSP_SAMPLESIZE, &tmp);
  if (tmp != samples.BitsPerSample())
    fprintf(stderr, "Unable to set the sample size\n");

  tmp = (samples.GetChannels() == 1) ? 0 : 1;
  if (ioctl (dev, SNDCTL_DSP_STEREO, &tmp)==-1)
    fprintf (stderr, "Unable to set mono/stereo\n");

  tmp = samples.GetSpeed();
  if (ioctl (dev, SNDCTL_DSP_SPEED, &tmp) == -1)
    perror("ioctl DSP_SPEED");

  // check if fragsize was ok
  ioctl (dev, SNDCTL_DSP_GETBLKSIZE, &tmp);
  if (tmp < 1)
    perror ("GETBLKSIZE");
  else if (tmp != FRAGBYTES)
    fprintf(stderr, "Unable to verify FRAGMENT %d, fbytes = %d, fshorts = %d\n", tmp, FRAGBYTES, FRAGSHORTS);
}


void tAudioPlayer::CloseDsp(int reset)
{
  if (dev >= 0)
  {
    if (reset)
    {
      if (ioctl(dev,  SNDCTL_DSP_RESET, 0) == -1)
	perror("SNDCTL_DSP_RESET");
    }
    else {
      if (ioctl (dev, SNDCTL_DSP_SYNC, NULL) < 0)
        perror("SNDCTL_DSP_SYNC");
    }
    close(dev);
    dev = -1;
  }
}


void tAudioPlayer::Notify()
{
  if (audio_enabled) {
    if (PlaybackMode()) {
      WriteSamples();
      // here it may hang when swapping in pages
      samples.FillBuffers(OutClock);
      WriteSamples();
    }
    if (RecordMode())
      ReadSamples();

    if (samples.softsync)
      MidiSync();
  }
  tSeq2Player::Notify();
}


int tAudioPlayer::WriteSamples()
{
  if (!audio_enabled)
    return 0;

  int blocks_written = 0;

  // number of blocks to be written
  audio_buf_info info;
  if (ioctl(dev, SNDCTL_DSP_GETOSPACE, &info) == -1)
    perror("SNDCTL_DSP_GETOSPACE");

  // todo: this is a bug in the audiodriver in newer kernels (2.1.28)
  // and the oss/linux for 2.0.29 it should be
  // for (int i = 0; i < info.fragments; i++) {

  for (int i = 0; i < info.fragments - 1; i++) {
    tAudioBuffer *buf = samples.full_buffers.Get();
    if (buf == 0)
      break;
    if (write(dev, buf->Data(), BUFBYTES) != BUFBYTES)
      perror("write");
    blocks_written ++;
    samples.free_buffers.Put(buf);
  }

  return blocks_written;
}


void tAudioPlayer::ReadSamples()
{
  audio_buf_info info;
  if (ioctl(dev, SNDCTL_DSP_GETISPACE, &info) == -1)
    perror("SNDCTL_DSP_GETISPACE");

  // a oss bug: if read is not called, there will be
  // no recording. probably recording does NOT start
  // exactly in sync with midi - but who knows.
  if (force_read && !info.fragments)
    info.fragments = 1;
  force_read = 0;

  for (int i = 0; i < info.fragments; i++) {
    short *b = recbuffers.RequestBuffer()->data;
    if (read(dev, b, BUFBYTES) != BUFBYTES) {
      // oss bug? It send EINTR?? on first read..
      if (errno != EINTR && errno != EAGAIN)
	perror("read");
      recbuffers.UndoRequest();
      break;
    }
  }

}


void tAudioPlayer::MidiSync()
{
  // OSS is buggy! In Win32 SDK you read the docs, hack away and
  // everything works. In OSS, there are no docs and if it works
  // with kernel x it wont with kernel y.

  if (!audio_enabled)
    return;

  int command = SNDCTL_DSP_GETOPTR;
  if (!PlaybackMode())
    command = SNDCTL_DSP_GETIPTR;

  // get realtime info for audio/midi sync
  count_info cinfo;
  if (ioctl(dev, command, &cinfo) == -1)
    perror("SNDCTL_DSP_GETOPTR");

  // search for SNDCTL_DSP_GETOPTR in linux/drivers/sound/dmabuf
  // before trying to understand the next line
  long new_bytes = cinfo.bytes - cinfo.ptr;  // info.ptr is garbage!!
  if (new_bytes != audio_bytes) {
    // driver has processed some bytes or whole fragment
    if (ioctl(seqfd, SNDCTL_SEQ_GETTIME, &midi_clock) < 0)
      perror("ioctl SNDCTL_SEQ_GETTIME failed - please get a newer kernel (2.1.28 or up)");
    audio_bytes = new_bytes;

    // OSS bug?: mpu401 does not like speed changes too often
    long audio_clock = (long)samples.Samples2Ticks(audio_bytes/2);
    int  delta_clock = audio_clock - midi_clock;
    int new_speed = midi_speed + delta_clock;
    // limit speed changes to some reasonable values
    const int limit = 1;
    new_speed = MIN(new_speed, midi_speed + limit);
    new_speed = MAX(new_speed, midi_speed - limit);

    if (new_speed != curr_speed) {
      if (ioctl(seqfd, SNDCTL_TMR_TEMPO, &new_speed) < 0)
        // this sometimes happens with mpu-401 timer
	; // perror("SNDCTL_TMR_TEMPO");
      else
	curr_speed = new_speed;
      // xview has reentrancy problems!!
      // TrackWin->DrawSpeed(curr_speed);
    }

  }
}

void tAudioPlayer::StartPlay(long Clock, long LoopClock, int Continue)
{
  delete listener;
  samples.StartPlay(Clock);
  tSeq2Player::StartPlay(Clock, LoopClock, Continue);
}

void tAudioPlayer::StopPlay()
{
  samples.StopPlay();
  tSeq2Player::StopPlay();
  if (!audio_enabled)
    return;
  CloseDsp(TRUE);
  if (RecordMode())
  {
    long frc = rec_info->FromClock;
    if (frc < start_clock)
      frc = start_clock;
    long toc = rec_info->ToClock;
    if (toc > recd_clock)
      toc = recd_clock;
    samples.SaveRecordingDlg(frc, toc, recbuffers);
  }
  recbuffers.Clear();
  // xview has reentrancy problems!!
  // TrackWin->DrawSpeed(midi_speed);
}



void tAudioPlayer::ListenAudio(int key, int start_stop_mode)
{
  if (!audio_enabled)
    return;

  // when already listening then stop listening
  if (listener)
  {
    delete listener;
    listener = 0;
    if (start_stop_mode)
      return;
  }
  if (key < 0)
    return;

  if (dev >= 0)  // device busy (playing)
    return;
  listener = new tAudioListener(this, key);
}

void tAudioPlayer::ListenAudio(tSample &spl, long fr_smpl, long to_smpl)
{
  if (!audio_enabled)
    return;

  // when already listening then stop listening
  if (listener)
    delete listener;
  if (dev >= 0)  // device busy (playing)
    return;
  listener = new tAudioListener(this, spl, fr_smpl, to_smpl);
}

long tAudioPlayer::GetListenerPlayPosition() {
  if (!listener)
    return -1L;
  return listener->GetPlayPosition();
}



