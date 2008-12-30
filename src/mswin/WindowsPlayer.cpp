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

#include "WindowsPlayer.h"

#include "WindowsMidiInterface.h"
#include "JazzPlusPlusApplication.h"
#include "ProjectManager.h"
#include "TrackFrame.h"
#include "TrackWindow.h"
#include "Dialogs.h"
#include "MidiDeviceDialog.h"
#include "Globals.h"

//#include <dos.h>

#include <wx/msgdlg.h>

using namespace std;

// for msvc uncomment these
#ifdef _MSC_VER
#define enable()
#define disable()
#endif

//*****************************************************************************
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZWindowsPlayer::JZWindowsPlayer(JZSong* pSong)
  : JZPlayer(pSong)
{
  poll_millisec = 25;
  timer_installed = FALSE;
  state = NewWinPlayerState();

  state->hinp   = 0;
  state->hout   = 0;
  state->recd_buffer.clear();
  state->play_buffer.clear();
  state->thru_buffer.clear();
  state->playing = FALSE;
  state->soft_thru = gpConfig->GetValue(C_SoftThru);
  state->doing_mtc_rec = FALSE;
  state->audio_player = 0;

  int ilong = -1, olong = -1;
  if (
    !gpConfig->Get(C_WinInputDevice, ilong) ||
    !gpConfig->Get(C_WinOutputDevice, olong))
  {
    SettingsDlg(ilong, olong);
  }
  // only output device MUST be there
  else if (olong < 0)
  {
    SettingsDlg(ilong, olong);
  }

  // select input device
  if (ilong >= 0)
  {
    UINT dev = (UINT)ilong;
    UINT rc;
    switch (gpConfig->GetValue(C_ClockSource))
    {
      case CsMidi:
        rc = midiInOpen(
          &state->hinp,
          dev,
          (DWORD)midiMidiInputHandler,
          (DWORD)state,
          CALLBACK_FUNCTION);
        break;
      case CsMtc:
        rc = midiInOpen(
          &state->hinp,
          dev,
          (DWORD)midiMtcInputHandler,
          (DWORD)state,
          CALLBACK_FUNCTION);
        break;
      case CsInt:
      case CsFsk:
      default:
        rc = midiInOpen(
          &state->hinp,
          dev,
          (DWORD)midiIntInputHandler,
          (DWORD)state,
          CALLBACK_FUNCTION);
        break;
    }
    if (rc)
    {
      char errtxt[200];
      midiInGetErrorText(rc, (LPSTR)errtxt, sizeof(errtxt));
      wxMessageBox(errtxt, "open midi input", wxOK);
    }
  }

  // select output device
  if (olong >= 0)
  {
    UINT dev = (UINT)olong;
    if (dev == MAX_MIDI_DEVS)
      dev = MIDI_MAPPER;

    //UINT rc = midiOutOpen(&state->hout, dev, 0L, 0L, 0L);
    UINT rc = midiOutOpen(&state->hout, dev, (DWORD)MidiOutProc, (DWORD)state, CALLBACK_FUNCTION);
    if (rc)
    {
      char errtxt[200];
      midiOutGetErrorText(rc, (LPSTR)errtxt, sizeof(errtxt));
      wxMessageBox(errtxt, "open midi output", wxOK);
    }
  }

  // install timer
  {
    TIMECAPS caps;
    if (timeGetDevCaps(&caps, sizeof(caps)) == 0)
    {
      state->min_timer_period = caps.wPeriodMin;
      state->max_timer_period = caps.wPeriodMax;
      if (timeBeginPeriod(state->min_timer_period) == 0)
        timer_installed = TRUE;
    }
    if (!timer_installed)
      wxMessageBox("could not install timer", "midi timer", wxOK);
  }


  maxSysLen = 2000;
  hSysHdr = GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE | GMEM_ZEROINIT, (DWORD)sizeof(MIDIHDR));
  pSysHdr = (MIDIHDR *)GlobalLock(hSysHdr);
  hSysBuf = GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE | GMEM_ZEROINIT, (DWORD)maxSysLen);
  pSysBuf = (unsigned char *)GlobalLock(hSysBuf);

  if (state->hinp)
    midiInStart(state->hinp);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool JZWindowsPlayer::IsInstalled()
{
  return timer_installed && state->hout;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZWindowsPlayer::~JZWindowsPlayer()
{
  if (state->hinp)
  {
    midiInReset(state->hinp);
    midiInClose(state->hinp);
  }
  if (state->hout)
  {
    midiOutReset(state->hout);
    midiOutClose(state->hout);
  }
  if (timer_installed)
    timeEndPeriod(state->min_timer_period);

  GlobalUnlock(hSysHdr);
  GlobalFree(hSysHdr);
  GlobalUnlock(hSysBuf);
  GlobalFree(hSysBuf);

  DeleteWinPlayerState(state);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZWindowsPlayer::SetSoftThru(int on, int InputDevice, int OutputDevice)
{
  state->soft_thru = on;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
JZEvent *JZWindowsPlayer::Dword2Event(DWORD dw)
{
  union
  {
    DWORD w;
    unsigned char c[4];
  } u;
  u.w = dw;

  JZEvent* pEvent = 0;

  switch(u.c[0] & 0xf0)
  {
    case 0x80:
      pEvent = new tKeyOff(0, u.c[0] & 0x0f, u.c[1]);
      break;

    case 0x90:
      if (u.c[2])
        pEvent = new tKeyOn(0, u.c[0] & 0x0f, u.c[1], u.c[2], 0);
      else
        pEvent = new tKeyOff(0, u.c[0] & 0x0f, u.c[1]);
      break;

    case 0xA0:
      pEvent = new tKeyPressure(0, u.c[0] & 0x0f, u.c[1], u.c[2]);
      break;

    case 0xB0:
      if (u.c[1] != 0x7b)
      {
        pEvent = new tControl(0, u.c[0] & 0x0f, u.c[1], u.c[2]);
      }
      break;

    case 0xC0:
      pEvent = new tProgram(0, u.c[0] & 0x0f, u.c[1]);
      break;

    case 0xD0:
      pEvent = new tChnPressure(0, u.c[0] & 0x0f, u.c[1]);
      break;

    case 0xE0:
      pEvent = new tPitch(0, u.c[0] & 0x0f, u.c[1], u.c[2]);
      break;
  }
  return pEvent;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
DWORD JZWindowsPlayer::Event2Dword(JZEvent* pEvent)
{
  union
  {
    DWORD w;
    unsigned char c[4];
  } u;
  u.w = 0;

  int Stat = pEvent->GetStat();
  switch (Stat)
  {
    case StatKeyOn:
      {
        tKeyOn* pKeyOn = pEvent->IsKeyOn();
        u.c[0] = 0x90 | pKeyOn->GetChannel();
        u.c[1] = pKeyOn->GetKey();
        u.c[2] = pKeyOn->GetVelocity();
      }
      break;

    case StatKeyOff:
      {
        tKeyOff* pKeyOff = pEvent->IsKeyOff();
        u.c[0] = 0x80 | pKeyOff->GetChannel();
        u.c[1] = pKeyOff->GetKey();
        u.c[2] = 0;
      }
      break;

    case StatProgram:
      {
        tProgram* pProgram = pEvent->IsProgram();
        u.c[0] = 0xC0 | pProgram->GetChannel();
        u.c[1] = pProgram->GetProgram();
      }
      break;

    case StatChnPressure:
      {
        tChnPressure* k = pEvent->IsChnPressure();
        u.c[0] = 0xC0 | k->GetChannel();
        u.c[1] = k->Value;
      }
      break;

    case StatControl:
      {
        tControl* pControl = pEvent->IsControl();
        u.c[0] = 0xB0 | pControl->GetChannel();
        u.c[1] = pControl->GetControl();
        u.c[2] = pControl->GetControlValue();
      }
      break;

    case StatKeyPressure:
      {
        tKeyPressure* pKeyPressure = pEvent->IsKeyPressure();
        u.c[0] = 0xA0 | pKeyPressure->GetChannel();
        u.c[1] = pKeyPressure->GetKey();
        u.c[2] = pKeyPressure->GetPressureValue();
      }
      break;

    case StatPitch:
      {
        tPitch *k = pEvent->IsPitch();
        int     v = k->Value + 8192;
        u.c[0] = 0xE0 | k->GetChannel();
        u.c[1] = (unsigned char)(v & 0x7F);
        u.c[2] = (unsigned char)(v >> 7);
      }
      break;

    case StatMidiClock:
    case StatStartPlay:
    case StatContPlay:
    case StatStopPlay:
      {
        u.c[0] = Stat;
      }
      break;

    case StatSysEx:
      break;

    case StatSetTempo:
      {
        tSetTempo *t = pEvent->IsSetTempo();
        if (t && t->GetClock() > 0)
        {
          SetTempo( t->GetBPM(), t->GetClock() );
          OutOfBandEvents.Put(pEvent->Copy());
        }
      }
      break;

    default:
      break;
  }
  return u.w;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
long JZWindowsPlayer::Clock2Time(long clock)
{
  if (clock < state->start_clock)
    return state->start_time;
  return (long)( (double)(clock - state->start_clock) * 60000.0 /
    (double)state->ticks_per_minute + state->start_time);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
long JZWindowsPlayer::Time2Clock(long time)
{
  if (time < state->start_time)
    return state->start_clock;
  return (long)(
    (double)(time - state->start_time) * (double)state->ticks_per_minute / 60000.0 +
    state->start_clock);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZWindowsPlayer::SetTempo(long bpm, long clock)
{
  long t1 = Clock2Time(clock);
  state->ticks_per_minute = (long)bpm * (long)Song->GetTicksPerQuarter();
  long t2 = Clock2Time(clock);
  state->start_time += (t1 - t2);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
long JZWindowsPlayer::RealTimeClock2Time(long clock)
{
  if (clock < state->start_clock)
    return real_start_time;
  return (long)( (double)(clock - state->start_clock) * 60000.0 / (double)real_ticks_per_minute + real_start_time);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
long JZWindowsPlayer::Time2RealTimeClock(long time)
{
  if (time < real_start_time)
    return state->start_clock;
  return (long)((double)(time - real_start_time) * (double)real_ticks_per_minute / 60000.0 + state->start_clock);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZWindowsPlayer::SetRealTimeTempo(long bpm, long clock)
{
  long t1 = RealTimeClock2Time(clock);
  real_ticks_per_minute = (long)bpm * (long)Song->GetTicksPerQuarter();
  long t2 = RealTimeClock2Time(clock);
  real_start_time += (t1 - t2);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZWindowsPlayer::OutSysex(JZEvent* pEvent, DWORD time)
{
  tSysEx *sx = pEvent->IsSysEx();
  if (sx == 0)
    return 1;

  if (state->play_buffer.nfree() < 2)
    return 1;

  state->sysex_found = TRUE;
  tWinSysexBuffer *buf = state->osx_buffers->AllocBuffer();
  buf->PrepareOut(state->hout, sx->GetData(), sx->GetLength() - 1);
  state->play_buffer.put(SYSEX_EVENT, time);
  state->play_buffer.put((DWORD)buf, time);
  return 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZWindowsPlayer::OutEvent(JZEvent* pEvent)
{
  DWORD d = Event2Dword(pEvent);
  if (d)
  {
    state->play_buffer.put(d, Clock2Time(pEvent->GetClock()));
  }
  else if (pEvent->IsSysEx() && (pEvent->GetClock() > 0))
  {
    OutSysex(pEvent, Clock2Time(pEvent->GetClock()));
  }
  return 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int JZWindowsMidiPlayer::OutEvent(JZEvent* pEvent)
{
  DWORD d = Event2Dword(pEvent);
  if (d)
  {
    state->play_buffer.put(d, pEvent->GetClock());
  }
  else if (pEvent->IsSysEx() && (pEvent->GetClock() > 0))
  {
    OutSysex(pEvent, pEvent->GetClock());
  }
  return 0;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZWindowsPlayer::OutNow(JZEvent* pEvent)
{
  DWORD d = Event2Dword(pEvent);
  if (d)
  {
    midiOutShortMsg(state->hout, d);
  }
  else if (pEvent->GetStat() == StatSetTempo)
  {
    if (state->playing)
    {
      SetTempo(pEvent->IsSetTempo()->GetBPM(), OutClock);
    }
  }
  else if (pEvent->GetStat() == StatSysEx)
  {
    tSysEx *s = pEvent->IsSysEx();
    if (s->GetDataLength() + 1 < maxSysLen)
    {
      pSysBuf[0] = 0xf0;
      memcpy(pSysBuf + 1, s->GetData(), s->GetDataLength());

      pSysHdr->lpData = (LPSTR)pSysBuf;
      pSysHdr->dwBufferLength = s->GetDataLength() + 1;
      pSysHdr->dwUser = 0;

      if (midiOutPrepareHeader(state->hout, pSysHdr, sizeof(MIDIHDR)) == 0)
        midiOutLongMsg(state->hout, pSysHdr, sizeof(MIDIHDR));
      // here we should wait, until the data are physically sent.
      // but there is no API call for this?!
      midiOutUnprepareHeader(state->hout, pSysHdr, sizeof(MIDIHDR));
    }
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZWindowsPlayer::OutNow(tParam *r)
{
  OutNow(&r->mMsb);
  OutNow(&r->mLsb);
  OutNow(&r->mDataMsb);
  OutNow(&r->mResetMsb);
  OutNow(&r->mResetLsb);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZWindowsPlayer::FillMidiClocks(long to)
{
  while (midiClockOut <= to)
  {
    tMidiClock* pEvent = new tMidiClock(midiClockOut);
    mPlayBuffer.Put(pEvent);
    midiClockOut = midiClockOut + state->ticks_per_signal;
  }
  mPlayBuffer.Sort();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZWindowsPlayer::OutBreak(long clock)
{
  if (gpConfig->GetValue(C_RealTimeOut))
  {
    FillMidiClocks( OutClock );
    FlushToDevice( OutClock );
  }
  else
  {
    state->play_buffer.put(0, Clock2Time(clock));
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZWindowsMidiPlayer::OutBreak(long clock)
{
  if (gpConfig->GetValue(C_RealTimeOut))
  {
    FillMidiClocks( OutClock );
    FlushToDevice( OutClock );
  }
  else
  {
    state->play_buffer.put(0, clock);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZWindowsPlayer::OutBreak()
{
  OutBreak(OutClock);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static DWORD GetMtcTime(tWinPlayerState* pState)
{
  DWORD frames = pState->mtc_frames;
  switch (pState->mtc_start.type)
  {
    case 0:
      return (
        ((frames / 24) * 1000) +
        (((frames % 24) * pState->time_per_frame) / 1000));
    case 1:
      return (
        ((frames / 25) * 1000) +
        (((frames % 25) * pState->time_per_frame) / 1000));
    case 2:
    case 3:
      return (
        ((frames / 30) * 1000) +
        (((frames % 30) * pState->time_per_frame) / 1000));
    default:
      return 0;
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZWindowsPlayer::StartPlay(long Clock, long LoopClock, int Continue)
{
  state->play_buffer.clear();
  state->recd_buffer.clear();
  state->sysex_found = FALSE;

  state->ticks_per_minute  = Song->GetTicksPerQuarter() * Song->Speed();
  real_ticks_per_minute    = state->ticks_per_minute;
  state->ticks_per_signal  = Song->GetTicksPerQuarter() / 24;
  state->time_per_tick = 60000000L / state->ticks_per_minute;
  state->time_correction   = 0;

  if (gpConfig->GetValue(C_ClockSource) == CsMtc)
  {
    if (state->doing_mtc_rec)
    {
      Clock = 0;
    }
    if (!Continue)
    {
      tMtcTime *offs = Song->GetTrack(0)->GetMtcOffset();
      state->start_time = offs->ToMillisec();
      real_start_time = state->start_time;
      state->mtc_start.type = offs->type;
      delete offs;
      state->start_clock = 0;
    }
  }
  else
  {
    state->start_time  = (long)timeGetTime() + 500;
    real_start_time = state->start_time;
    state->start_clock = Clock;
  }

  state->play_time   = state->start_time;
  midiClockOut = Clock;

  if (GetAudioEnabled())
    state->play_buffer.put(START_AUDIO, state->start_time);

  OutOfBandEvents.Clear();

  JZProjectManager::Instance()->NewPlayPosition(
    PlayLoop->Ext2IntClock(Clock));

  state->playing = TRUE;  // allow for SetTempo in OutNow()
  JZPlayer::StartPlay(Clock, LoopClock, Continue);

  if (gpConfig->GetValue(C_RealTimeOut))
  {
    tMetaEvent* pEvent;
    if (!Continue)
    {
      pEvent = new tStartPlay(0);
    }
    else
    {
      pEvent = new tContPlay(0);
    }
    OutNow(pEvent);
    FillMidiClocks(mPlayBuffer.GetLastClock()); // also does a sort
  }


  switch (gpConfig->GetValue(C_ClockSource))
  {
    case CsMidi:
      state->virtual_clock = Clock - state->ticks_per_signal;
      state->signal_time = state->start_time - 5000L;
      // state->playing = TRUE;
      timeSetEvent(state->min_timer_period, state->min_timer_period, midiMidiTimerHandler, (DWORD)state, TIME_ONESHOT);
      break;
    case CsMtc:
      if (!Continue)
      {
        // In microseconds: 1 sec / frames_per_sec
        switch (state->mtc_start.type)
        {
          case 0:
            state->time_per_frame = 1000000 / 24;
            break;
          case 1:
            state->time_per_frame = 1000000 / 25;
            break;
          case 2:
          case 3:
            state->time_per_frame = 1000000 / 30;
            break;
        }
        state->signal_time = 0;
        state->mtc_valid = FALSE;
        state->last_qfm = 0;
        state->qfm_bits = 0;
        lastValidMtcClock = Clock;
      }
      // state->playing = TRUE;
      timeSetEvent(state->min_timer_period, state->min_timer_period, midiMtcTimerHandler, (DWORD)state, TIME_ONESHOT);
      break;
    case CsInt:
    case CsFsk:
    default:
      // state->playing = TRUE;
      timeSetEvent(
        state->min_timer_period,
        state->min_timer_period,
        midiIntTimerHandler,
        (DWORD)state,
        TIME_ONESHOT);
      break;
  }

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZWindowsPlayer::StopPlay()
{
  wxBeginBusyCursor();
  state->playing = FALSE;
  JZPlayer::StopPlay();
  if (gpConfig->GetValue(C_RealTimeOut))
  {
    tStopPlay* pEvent = new tStopPlay(0);
    OutNow(pEvent);
    delete pEvent;
  }
  AllNotesOff();
  RecdBuffer.Keyoff2Length();

  if (state->hout)
  {
    if (state->sysex_found)
      midiOutReset(state->hout);
    int n = state->osx_buffers->Size();
    for (int i = 0; i < n; ++i)
    {
      tWinSysexBuffer *buf = state->osx_buffers->At(i);
      if (buf->IsPrepared())
      {
        buf->UnprepareOut(state->hout);
      }
    }
    state->osx_buffers->ReleaseAllBuffers();
  }


/*
  // sysex recording not finished yet.
  if (state->hinp)
  {
    midiInReset(state->hinp);
    int n = state->isx_buffers->Size();
    for (int i = 0; i < n; ++i)
    {
      tWinSysexBuffer *buf = state->isx_buffers->At(i);
      if (buf->IsPrepared())
      {
        buf->UnprepareIn(state->hinp);
      }
      state->isx_buffers->ReleaseAllBuffers();
    }
  }
*/

  wxEndBusyCursor();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZWindowsPlayer::FlushToDevice()
// try to send all events up to OutClock to device
{
  if (gpConfig->GetValue(C_RealTimeOut))
  {
    FillMidiClocks( OutClock );
  }
  FlushToDevice( OutClock );
  OutBreak(OutClock);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZWindowsPlayer::FlushToDevice(long clock)
{
  tEventIterator Iterator(&mPlayBuffer);
  JZEvent* pEvent = Iterator.Range(0, clock);
  if (pEvent)
  {
    do
    {
      OutEvent(pEvent);
      pEvent->Kill();
      pEvent = Iterator.Next();
    } while (pEvent);

    mPlayBuffer.Cleanup(0);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
long JZWindowsIntPlayer::GetRealTimeClock()
{
  while (!state->recd_buffer.empty())
  {
    midi_event *m = state->recd_buffer.get();

    // Event?
    JZEvent* pEvent = Dword2Event(m->data);
    if (pEvent)
    {
      pEvent->SetClock(PlayLoop->Ext2IntClock(Time2RealTimeClock(m->ref)));
      RecdBuffer.Put(pEvent);
    }
  }

  long clock = Time2RealTimeClock( (long)timeGetTime() + state->time_correction );

  JZProjectManager::Instance()->NewPlayPosition(
    PlayLoop->Ext2IntClock(clock / 48 * 48));

  if ( !OutOfBandEvents.IsEmpty() )
  {
    tEventIterator Iterator(&OutOfBandEvents);
    JZEvent* pEvent = Iterator.Range(0, clock);
    while (pEvent)
    {
      switch (pEvent->GetStat())
      {
        case StatSetTempo:
          SetRealTimeTempo( ((tSetTempo *)pEvent)->GetBPM(), clock );
          break;
        default:
          break;
      }
      pEvent->Kill();
      pEvent = Iterator.Next();
    }
    OutOfBandEvents.Cleanup(0);
  }

  return clock;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
long JZWindowsMidiPlayer::GetRealTimeClock()
{
  long clock;

  while (!state->recd_buffer.empty())
  {
    midi_event *m = state->recd_buffer.get();
    if (m->data == 0xfc)
    {
      // Stop play received
      AllNotesOff();
      return -1;
    }
    else if ( (m->data & 0x000000ff) == 0xf2 )
    {
      // Song pointer received
      union
      {
        DWORD w;
        unsigned char c[4];
      } u;
      gpMidiPlayer->StopPlay();
      u.w = m->data;
      clock =
        ((long)u.c[1] + (128L * (long)u.c[2])) * (Song->GetTicksPerQuarter() / 4);
      gpMidiPlayer->StartPlay( clock, 0, 1 );
      return -1;
    }

    // Event?
    JZEvent* pEvent = Dword2Event(m->data);
    if (pEvent)
    {
      pEvent->SetClock(PlayLoop->Ext2IntClock(m->ref));
      RecdBuffer.Put(pEvent);
    }
  }

  long delta_clock = (((long)timeGetTime() - state->signal_time) * 1000L) / state->time_per_tick;

  if (delta_clock > (2 * state->ticks_per_signal))
  {
    clock = state->virtual_clock;
  }
  else
  {
    clock = state->virtual_clock + delta_clock;
  }

  JZProjectManager::Instance()->NewPlayPosition(
    PlayLoop->Ext2IntClock(clock / 48 * 48));

  return clock;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
long JZWindowsMtcPlayer::GetRealTimeClock()
{
  long clock;

  while (!state->recd_buffer.empty())
  {
    midi_event *m = state->recd_buffer.get();
    if (m->data == 0xf1)
    {
      // MTC starting (from midi input handler)
      gpMidiPlayer->StopPlay();
      clock = PlayLoop->Ext2IntClock( Time2Clock( m->ref ) );
      lastValidMtcClock = clock;
      gpMidiPlayer->StartPlay( clock, 0, 1 );
      return -1;
    }

    // Event?
    JZEvent* pEvent = Dword2Event(m->data);
    if (pEvent)
    {
      pEvent->SetClock(PlayLoop->Ext2IntClock(Time2Clock(m->ref)));
      RecdBuffer.Put(pEvent);
    }
  }

  if (state->mtc_valid)
  {
    if ( ((long)timeGetTime() - state->signal_time) > 500 )
    {
      /* Assume tape stopped */
      disable();
      state->mtc_valid = 0;
      state->qfm_bits = 0;
      enable();
      AllNotesOff();
      return( -1 );
    }
    if (state->doing_mtc_rec)
    {
      clock = 0;
    }
    else
    {
      clock = Time2Clock( GetMtcTime( state ) );
    }
    lastValidMtcClock = clock;
  }
  else
  {
    clock = lastValidMtcClock;
  }

  JZProjectManager::Instance()->NewPlayPosition(
    PlayLoop->Ext2IntClock(clock / 48 * 48));

  if ( !OutOfBandEvents.IsEmpty() )
  {
    tEventIterator Iterator(&OutOfBandEvents);
    JZEvent* pEvent = Iterator.Range(0, clock);
    while (pEvent)
    {
      switch (pEvent->GetStat())
      {
        case StatSetTempo:
          SetRealTimeTempo( ((tSetTempo *)pEvent)->GetBPM(), clock );
          break;
        default:
          break;
      }
      pEvent->Kill();
      pEvent = Iterator.Next();
    }
    OutOfBandEvents.Cleanup(0);
  }

  return clock;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZWindowsMtcPlayer::InitMtcRec()
{
  state->doing_mtc_rec = TRUE;
  StartPlay( 0, 0, 0 );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
tMtcTime* JZWindowsMtcPlayer::FreezeMtcRec()
{
  StopPlay();
  state->doing_mtc_rec = FALSE;
  return(new tMtcTime(
    (long) GetMtcTime(state),
    (tMtcType) state->mtc_start.type));
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void JZWindowsPlayer::SettingsDlg(int& InputDevice, int& OutputDevice)
{
  vector<pair<string, int> > MidiDevices;

  //=========================
  // Select the input device.
  //=========================

  // Get a list of the available input devices.
  UINT i;
  UINT InputMidiDeviceCount = midiInGetNumDevs();
  for (i = 0; i < InputMidiDeviceCount; ++i)
  {
    MIDIINCAPS caps;
    midiInGetDevCaps(i, &caps, sizeof(caps));
    MidiDevices.push_back(make_pair(caps.szPname, i));
  }

  if (InputMidiDeviceCount > 0)
  {
    JZMidiDeviceDialog MidiInputDeviceDialog(
      MidiDevices,
      InputDevice,
      ::wxGetApp().GetMainFrame(),
      "Input MIDI device");
    MidiInputDeviceDialog.ShowModal();
  }

  MidiDevices.clear();

  // select output device
  UINT OutputMidiDeviceCount = midiOutGetNumDevs();
  for (i = 0; i < OutputMidiDeviceCount; ++i)
  {
    MIDIOUTCAPS caps;
    midiOutGetDevCaps(i, &caps, sizeof(caps));
    MidiDevices.push_back(make_pair(caps.szPname, i));
  }
  MidiDevices.push_back(make_pair("Midi Mapper", MAX_MIDI_DEVS));

  JZMidiDeviceDialog MidiOutputDeviceDialog(
    MidiDevices,
    OutputDevice,
    gpTrackWindow,
    "Output MIDI device");
  MidiOutputDeviceDialog.ShowModal();

  if (InputDevice >= 0)
  {
    gpConfig->Put(C_WinInputDevice, InputDevice);
  }
  else
  {
    gpConfig->Get(C_WinInputDevice, InputDevice);
  }

  if (OutputDevice >= 0)
  {
    gpConfig->Put(C_WinOutputDevice, OutputDevice);
  }
  else
  {
    gpConfig->Get(C_WinOutputDevice, OutputDevice);
  }
}
