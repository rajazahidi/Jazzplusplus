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

#include "WxWidgets.h"

#include <windows.h>
#include <memory.h>

#include "WindowsAudioInterface.h"
#include "RecordingInfo.h"
#include "Globals.h"

// not sure if mutex may cause a dead lock when used to
// synchronize the wxTimer::Notify() interrupt and the
// low level midi interrupt, as these are not mswin threads.
// As far as I can see, the mutex is not needed anyway.

// IMPORTANT: enabling critical sections freezes NT!

#define InitializeCriticalSection(a)
#define EnterCriticalSection(a)
#define TryEnterCriticalSection(a)
#define LeaveCriticalSection(a)
#define DeleteCriticalSection(a)

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class tAudioListener : public wxTimer
{
  // play a sample from piano roll

  public:

    tAudioListener(JZWindowsAudioPlayer* pPlayer, int key)
      : wxTimer(),
        mpPlayer(pPlayer),
        mCount(0),
        mHardExit(true),
        mChannels(0)
    {
      mpPlayer->mpListener = this;

      // Indicate that we are not recording!
      mpPlayer->rec_info = 0;

      mChannels = mpPlayer->mSamples.GetChannels();

      mCount = mpPlayer->mSamples.PrepareListen(key);

      mpPlayer->OpenDsp();

      mpPlayer->StartAudio();

      Start(200);
    }

    tAudioListener(
      JZWindowsAudioPlayer* pPlayer,
      tSample& spl,
      long fr_smpl,
      long to_smpl)
      : wxTimer(),
        mpPlayer(pPlayer),
        mCount(0),
        mHardExit(true),
        mChannels(0)
    {
      mpPlayer->mpListener = this;

      // Indicate that we are not recording!
      mpPlayer->rec_info = 0;

      mChannels = mpPlayer->mSamples.GetChannels();

      mCount = mpPlayer->mSamples.PrepareListen(&spl, fr_smpl, to_smpl);

      mpPlayer->OpenDsp();

      mpPlayer->StartAudio();

      Start(200);
    }

    ~tAudioListener()
    {
      Stop();

      // todo: if !mHardExit flush outstanding buffers to device
      // before closing
      mpPlayer->CloseDsp();
      mpPlayer->mpListener = 0;
    }

    virtual void Notify()
    {
      EnterCriticalSection(&mpPlayer->mutex);
      mCount += mpPlayer->mSamples.ContinueListen();
      mpPlayer->WriteBuffers();
      LeaveCriticalSection(&mpPlayer->mutex);
      if (mpPlayer->blocks_played >= mCount)
      {
        mHardExit = false;
        delete this;
      }
    }

    long GetPlayPosition()
    {
      MMTIME mmtime;
      mmtime.wType = TIME_SAMPLES;
      waveOutGetPosition(mpPlayer->hout, &mmtime, sizeof(mmtime));
      return mmtime.u.sample * mChannels;
    }

  private:

    JZWindowsAudioPlayer* mpPlayer;

    long mCount;

    bool mHardExit;

    long mChannels;
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZWindowsAudioPlayer::JZWindowsAudioPlayer(JZSong* pSong)
  : JZWindowsIntPlayer(pSong),
    mErrorCode(NoError),
    mCanDuplex(false),
    mCanSynchronize(true),
    mpListener(0)
{
  state->audio_player = this;

  InitializeCriticalSection(&mutex);

  long dummy    = 0;
  AudioBuffer   = new tEventArray();
  installed     = 0;
  dummy = gpConfig->GetValue(C_EnableAudio);
  audio_enabled = dummy;
  hout_open     = 0;
  hinp_open     = 0;

  // check for device
  installed = 0;
  mCanDuplex = (gpConfig->GetValue(C_DuplexAudio) != 0);

  if (OpenDsp() == 0)
  {
    // check output device capabilities
    WAVEOUTCAPS ocaps;
    MMRESULT res = waveOutGetDevCaps((UINT)hout, &ocaps, sizeof(ocaps));
    if (res != MMSYSERR_NOERROR)
    {
      mErrorCode = ErrCapGet;
    }
    else if (!(ocaps.dwSupport & WAVECAPS_SAMPLEACCURATE))
    {
      // This is not an error; just a warning.
      wxMessageBox(
        "Your soundcard does not support audio/midi sync",
        "Warning",
        wxOK);
      mCanSynchronize = false;
    }

    if (!mErrorCode && CloseDsp() == 0)
    {
      installed = 1;
    }
  }
  recbuffers.Clear();
  audio_enabled = (audio_enabled && installed);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZWindowsAudioPlayer::~JZWindowsAudioPlayer()
{
  delete mpListener;
  delete AudioBuffer;

  // Close the device if it is open.
  CloseDsp();

  // Release the semaphor.
  DeleteCriticalSection(&mutex);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZWindowsAudioPlayer::ShowError()
{
  const char* pMessage = 0;
  switch (mErrorCode)
  {
    case ErrOutOpen:
      pMessage = "Cannot open audio output device";
      break;
    case ErrOutPrepare:
      pMessage = "Cannot prepare audio output headers";
      break;
    case ErrOutUnprepare:
      pMessage = "Cannot unprepare audio output headers";
      break;
    case ErrInpOpen:
      pMessage = "Cannot open audio input device";
      break;
    case ErrInpPrepare:
      pMessage = "Cannot prepare audio input headers";
      break;
    case ErrInpUnprepare:
      pMessage = "Cannot unprepare audio input headers";
      break;
    case ErrCapGet:
      pMessage = "Unable to get audio device capbabilities";
      break;
    case ErrCapSync:
      pMessage = "Your soundcard does not support audio/midi sync";
      break;
  }
  if (pMessage)
  {
    wxMessageBox(pMessage, "Error", wxOK);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZWindowsAudioPlayer::LoadSamples(const char *filename)
{
  return mSamples.Load(filename);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZWindowsAudioPlayer::OpenDsp()
{
  int i;
  MMRESULT res;

  mErrorCode = NoError;  // everything ok for now.

  if (!audio_enabled)
  {
    return 0;
  }

  mCanDuplex = (gpConfig->GetValue(C_DuplexAudio) != 0);

  // specify the data format
  WAVEFORMATEX fmt;
  memset(&fmt, 0, sizeof(fmt));
  fmt.wFormatTag      = WAVE_FORMAT_PCM;
  fmt.nChannels       = mSamples.GetChannels();
  fmt.nSamplesPerSec  = mSamples.GetSpeed();
  fmt.nBlockAlign     = mSamples.GetChannels() * sizeof(short);
  fmt.nAvgBytesPerSec = fmt.nBlockAlign * fmt.nSamplesPerSec;
  fmt.wBitsPerSample  = 16;
  fmt.cbSize          = 0;

  blocks_played   = 0;
  play_buffers_needed = 0;
  record_buffers_needed = 0;

  // open playback device
  if (!hout_open && PlaybackMode())
  {
    hout = 0;
    res = waveOutOpen(
      &hout,
      WAVE_MAPPER,
      &fmt,
      (DWORD)audioInterrupt,
      (DWORD)this,
      CALLBACK_FUNCTION);

    if (res != MMSYSERR_NOERROR)
    {
      mErrorCode = ErrOutOpen;
      return 1;
    }

    // prepare headers
    for (i = 0; i < BUFCOUNT; i++)
    {
      tAudioBuffer *buf = mSamples.GetBuffer(i);
      WAVEHDR *hdr = new WAVEHDR;
      memset(hdr, 0, sizeof(WAVEHDR));
      buf->hdr = hdr;

      hdr->lpData           = (char *)buf->Data();
      hdr->dwBufferLength   = BUFBYTES;          // length, in bytes, of the buffer

      res = waveOutPrepareHeader(hout, hdr, sizeof(WAVEHDR));
      if (res != MMSYSERR_NOERROR)
      {
        mErrorCode = ErrOutPrepare;
        return 1;
      }
    }
    hout_open = 1;
  }

  if (!hinp_open && RecordMode())
  {
    hinp = 0;
    recbuffers.Clear();

    res = waveInOpen(&hinp, WAVE_MAPPER, &fmt, (DWORD)audioInterrupt, (DWORD)this, CALLBACK_FUNCTION);
    if (res != MMSYSERR_NOERROR)
    {
      mErrorCode = ErrInpOpen;
      return 1;
    }

    // prepare headers and add them to recording device
    for (i = 0; i < BUFCOUNT; i++)
    {
      WAVEHDR *hdr = new WAVEHDR;
      memset(hdr, 0, sizeof(WAVEHDR));

      tAudioBuffer *buf = recbuffers.RequestBuffer();
      buf->hdr       = hdr;

      hdr->lpData           = (LPSTR)buf->data;
      hdr->dwBufferLength   = BUFBYTES;          // length, in bytes, of the buffer
      hdr->dwFlags          = 0;                 // see below

      res = waveInPrepareHeader(hinp, hdr, sizeof(WAVEHDR));
      if (res != MMSYSERR_NOERROR)
        return 1;

      res = waveInAddBuffer(hinp, hdr, sizeof(WAVEHDR));
      if (res != MMSYSERR_NOERROR)
      {
        mErrorCode = ErrInpPrepare;
        return 1;
      }
    }
    hinp_open = 1;

  }

  return 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZWindowsAudioPlayer::CloseDsp()
{
  // todo: close the device immediately if open

  int i;
  MMRESULT res;

  if (hout_open)
  {
    hout_open = 0;

    // shut up!
    waveOutReset(hout);

    // unprepare headers
    for (i = 0; i < BUFCOUNT; i++)
    {
      tAudioBuffer *buf = mSamples.GetBuffer(i);
      WAVEHDR *hdr = (WAVEHDR *)buf->hdr;

      res = waveOutUnprepareHeader(hout, hdr, sizeof(WAVEHDR));
      if (res != MMSYSERR_NOERROR)
      {
        mErrorCode = ErrOutUnprepare;
        return 1;
      }
      delete hdr;
      buf->hdr = 0;
    }

    // close the device
    waveOutClose(hout);
  }

  if (hinp_open)
  {
    hinp_open = 0;
    waveInReset(hinp);

    int n = recbuffers.buffers.GetSize();
    for (i = 0; i < n; i++)
    {
      tAudioBuffer *buf = recbuffers.buffers[i];
      if (buf == 0)
        break;
      res = waveInUnprepareHeader(hinp, (WAVEHDR *)buf->hdr, sizeof(WAVEHDR));
      if (res != MMSYSERR_NOERROR)
      {
        mErrorCode = ErrInpUnprepare;
        return 1;
      }
      delete buf->hdr;
      buf->hdr = 0;
    }

    waveInClose(hinp);
  }

  return 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void FAR PASCAL audioInterrupt(
  HWAVEOUT hout,
  UINT wMsg,
  DWORD dwUser,
  DWORD dw1,
  DWORD dw2)
{
  if (wMsg == MM_WOM_DONE || wMsg == MM_WIM_DATA)
  {
    ((JZWindowsAudioPlayer *)dwUser)->AudioCallback(wMsg);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZWindowsAudioPlayer::AudioCallback(UINT wMsg)
{
  // async called by driver when the driver has processed a buffer completely
  EnterCriticalSection(&mutex);

  if (hout_open && wMsg == MM_WOM_DONE)
  {
    blocks_played ++;
    play_buffers_needed ++;
    tAudioBuffer *buf = mSamples.driv_buffers.Get();
    mSamples.free_buffers.Put(buf);
  }
  if (hinp_open && wMsg == MM_WIM_DATA)
  {
    record_buffers_needed ++;
  }
  LeaveCriticalSection(&mutex);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZWindowsAudioPlayer::StartAudio()
{
  // async called by driver to start audio in sync with midi
  if (hout_open)
  {
    WriteBuffers();
    play_buffers_needed = 0;
  }

  if (hinp_open)
  {
    waveInStart(hinp);
  }
}

//-----------------------------------------------------------------------------
// Description:
//   Send the sample set to driver.
//-----------------------------------------------------------------------------
void JZWindowsAudioPlayer::WriteBuffers()
{
  if (audio_enabled && hout_open)
  {
    tAudioBuffer* pAudioBuffer;
    while ((pAudioBuffer = mSamples.full_buffers.Get()) != 0)
    {
      if (
        waveOutWrite(
          hout,
          pAudioBuffer->hdr,
          sizeof(WAVEHDR)) == MMSYSERR_NOERROR)
      {
        mSamples.driv_buffers.Put(pAudioBuffer);
        --play_buffers_needed;
      }
      else
      {
        mSamples.full_buffers.UnGet(pAudioBuffer);
        break;
      }
    }
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZWindowsAudioPlayer::Notify()
{
  if (audio_enabled)
  {
    EnterCriticalSection(&mutex);

    if (hout_open)
    {
      mSamples.FillBuffers(OutClock);
      if (play_buffers_needed > 0)  // dont trigger start play by accident
      {
        WriteBuffers();
      }

      // midi time correction
      if (mCanSynchronize && mSamples.softsync)
      {
        MMTIME mmtime;
        MMRESULT res;
        mmtime.wType = TIME_SAMPLES;
        res = waveOutGetPosition(hout, &mmtime, sizeof(mmtime));
        if (res == MMSYSERR_NOERROR && mmtime.wType == TIME_SAMPLES)
        {
          long time_now = (long)timeGetTime();
          long audio_now =
            (long)((double)start_time + 1000.0 * mmtime.u.sample / mSamples.speed);

          // low pass filter for time-correction (not really necessary)
          const long low = 50;
          state->time_correction =
            (low * state->time_correction + (100 - low) * (audio_now - time_now) ) / 100L;
        }
      }
    }


    if (hinp_open)
    {

      while (record_buffers_needed > 0)
      {
        // add a new record buffer
        WAVEHDR *hdr = new WAVEHDR;
        memset(hdr, 0, sizeof(WAVEHDR));

        tAudioBuffer *buf = recbuffers.RequestBuffer();
        buf->hdr = hdr;

        hdr->lpData         = (LPSTR)buf->data;
        hdr->dwBufferLength = BUFBYTES;          // length, in bytes, of the buffer
        hdr->dwFlags        = 0;

        if (waveInPrepareHeader(hinp, hdr, sizeof(WAVEHDR)) == MMSYSERR_NOERROR)
        {
          waveInAddBuffer(hinp, hdr, sizeof(WAVEHDR));
          record_buffers_needed --;
        }
        else
        {
          break;
        }
      }

      if (mCanSynchronize && mSamples.softsync && !hout_open)
      {
        // midi time correction
        MMTIME mmtime;
        MMRESULT res;
        mmtime.wType = TIME_SAMPLES;
        res = waveInGetPosition(hinp, &mmtime, sizeof(mmtime));
        if (res == MMSYSERR_NOERROR && mmtime.wType == TIME_SAMPLES)
        {
          long time_now  = (long)timeGetTime();
          long audio_now =
            (long)((double)state->start_time + 1000.0 * mmtime.u.sample / mSamples.speed);
          // low pass filter for time-correction (not really necessary)
          const long low = 50;
          state->time_correction = (low * state->time_correction + (100 - low) * (audio_now - time_now) ) / 100L;
        }
      }
    }

    LeaveCriticalSection(&mutex);
  }  // if (audio_enabled)

  JZWindowsIntPlayer::Notify();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZWindowsAudioPlayer::StartPlay(long Clock, long LoopClock, int Continue)
{
  mSamples.StartPlay(Clock);
  JZWindowsIntPlayer::StartPlay(Clock, LoopClock, Continue);

  if (!audio_enabled)
    return;

  delete mpListener;

  start_clock = Clock;
  start_time = state->start_time;

  mSamples.ResetBuffers(AudioBuffer, start_clock, state->ticks_per_minute);
  mSamples.FillBuffers(OutClock);

  OpenDsp();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZWindowsAudioPlayer::StopPlay()
{
  JZWindowsIntPlayer::StopPlay();
  CloseDsp();
  mSamples.StopPlay();
  if (RecordMode())
  {
    long frc = rec_info->mFromClock;
    if (frc < start_clock)
      frc = start_clock;
    long toc = rec_info->mToClock;
    long play_clock = Time2Clock(state->play_time);
    if (toc > play_clock)
      toc = play_clock;
    mSamples.SaveRecordingDlg(frc, toc, recbuffers);
  }
  recbuffers.Clear();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZWindowsAudioPlayer::ListenAudio(int key, int start_stop_mode)
{
  if (!audio_enabled)
  {
    return;
  }

  // Play the audio file from the piano roll.
  if (Playing)
  {
    return;
  }

  // If already listening then stop listening.
  if (mpListener)
  {
    delete mpListener;
    if (start_stop_mode)
    {
      return;
    }
  }
  if (key < 0)
  {
    return;
  }
  mpListener = new tAudioListener(this, key);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZWindowsAudioPlayer::ListenAudio(tSample &spl, long fr_smpl, long to_smpl)
{
  if (!audio_enabled)
    return;

  if (Playing)
    return;
  // when already listening then stop listening
  if (mpListener)
  {
    delete mpListener;
  }
  mpListener = new tAudioListener(this, spl, fr_smpl, to_smpl);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
long JZWindowsAudioPlayer::GetListenerPlayPosition()
{
  if (!mpListener)
  {
    return -1L;
  }
  return mpListener->GetPlayPosition();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZWindowsAudioPlayer::RecordMode() const
{
  return rec_info != 0 && rec_info->mpTrack->GetAudioMode();
}
