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
#include <mmsystem.h>
#include <memory.h>

#include "WindowsMidiInterface.h"

#include "WindowsAudioInterface.h"

extern "C"
{

/*
 *  LibMain - Generic for a DLL.  Just initializes a little local memory.
 */

int FAR PASCAL LibMain (
  HANDLE hInstance,
  WORD   wDataSeg, WORD wHeapSize,
  LPSTR  lpCmdLine)

{
    // Nothing to do - SDK Libentry does the LocalInit

    return TRUE;
}

/*
 *  WEP - Generic for a DLL.  Doesn't do a whole lot.
 */
void FAR PASCAL _WEP(WORD wParam)
{
}


// allocate memory for the player vars that are accessed
// in interrupt.
tWinPlayerState FAR * FAR PASCAL NewWinPlayerState()
{
  tWinPlayerState FAR *state = 0;
  // Allocate Fixed Memory for interrupt handler
  HANDLE hMem = GlobalAlloc(GMEM_SHARE | GMEM_FIXED | GMEM_ZEROINIT, (DWORD)sizeof(tWinPlayerState));
  state = (tWinPlayerState FAR *)GlobalLock(hMem);
#ifdef WIN32
  VirtualLock(state, sizeof(tWinPlayerState));
#else
  GlobalPageLock((HGLOBAL)HIWORD(state));
#endif
  memset(state, 0, sizeof(tWinPlayerState));
  state->hmem = hMem;

  state->isx_buffers = new tWinSysexBufferArray();
  state->osx_buffers = new tWinSysexBufferArray();

  return state;
}

// free interrupt data
void FAR PASCAL DeleteWinPlayerState(tWinPlayerState FAR * state)
{
  delete state->isx_buffers;
  state->isx_buffers = 0;
  delete state->osx_buffers;
  state->osx_buffers = 0;


  HANDLE hMem = state->hmem;
#ifdef WIN32
  VirtualUnlock(state, sizeof(tWinPlayerState));
#else
  GlobalPageUnlock((HGLOBAL)HIWORD(state));
#endif
  GlobalUnlock(hMem);
  GlobalFree(hMem);
}

#define Mtc2Frames( state ) \
{ \
  switch (state->mtc_start.type) \
  { \
    case 0: \
      state->mtc_frames = (((((state->mtc_start.hour * 60) + state->mtc_start.min) * 60) + state->mtc_start.sec) * 24) + state->mtc_start.fm; \
      break; \
    case 1: \
      state->mtc_frames = (((((state->mtc_start.hour * 60) + state->mtc_start.min) * 60) + state->mtc_start.sec) * 25) + state->mtc_start.fm; \
      break; \
    case 2: \
    case 3: \
      state->mtc_frames = (((((state->mtc_start.hour * 60) + state->mtc_start.min) * 60) + state->mtc_start.sec) * 30) + state->mtc_start.fm; \
      break; \
  } \
}

#define GetMtcTime( state, msec ) \
{ \
  switch (state->mtc_start.type) \
  { \
    case 0: \
      msec = ((state->mtc_frames / 24) * 1000) + (((state->mtc_frames % 24) * state->time_per_frame) / 1000); \
      break; \
    case 1: \
      msec = ((state->mtc_frames / 25) * 1000) + (((state->mtc_frames % 25) * state->time_per_frame) / 1000); \
      break; \
    case 2: \
    case 3: \
      msec = ((state->mtc_frames / 30) * 1000) + (((state->mtc_frames % 30) * state->time_per_frame) / 1000); \
      break; \
    default: \
      msec = 0; \
  } \
}



static inline void outsysex(tWinPlayerState *state)
{
  // take away the SYSEX_EVENT meta event
  (void) state->play_buffer.get();
  // next entry is the actual data
  midi_event *m = state->play_buffer.peek();
  tWinSysexBuffer *buf = (tWinSysexBuffer *)m->data;
  MIDIHDR *hdr = buf->MidiHdr();
  midiOutLongMsg(state->hout, hdr, sizeof(MIDIHDR));
  // dont care about returncodes because the SYSEX_EVENT was already
  // taken from the queue
}


// handle incoming midi data (internal clock)
void FAR PASCAL midiIntInputHandler(
  HMIDIIN hMidiIn,
  WORD wMsg,
  DWORD dwInstance,
  DWORD dwParam1,
  DWORD dwParam2)
{
    tWinPlayerState *state = (tWinPlayerState *)dwInstance;
    long now;

    now = (long)timeGetTime();

    switch(wMsg)
    {
        case MIM_DATA:
          // ignore active sensing and real time messages except midi stop
          if ( (dwParam1 & 0x000000ff) < 0xf8)
          {
              state->recd_buffer.put(dwParam1, now );

            /* Midi thru */
            if ( state->soft_thru )
            {
              if (!state->thru_buffer.empty() || midiOutShortMsg(state->hout, dwParam1) == MIDIERR_NOTREADY)
              {
                // device busy, output during normal play
                state->thru_buffer.put(dwParam1, 0);
              }
            }
          }
          break;
        case MIM_OPEN:
        case MIM_ERROR:
        default:
            break;
    }
}





// play output (internal clock)
void FAR PASCAL midiIntTimerHandler(UINT wTimerId, UINT wMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
  tWinPlayerState *state = (tWinPlayerState *)dwUser;
  if ( !state->playing )
    return;

  // output what was left from midi thru
  while (!state->thru_buffer.empty())
  {
    midi_event *m = state->thru_buffer.peek();
    if (midiOutShortMsg(state->hout, m->data) == MIDIERR_NOTREADY)
    {
      timeSetEvent(state->min_timer_period, state->min_timer_period * 5, midiIntTimerHandler, (DWORD)state, TIME_ONESHOT);
      return;
    }
    (void)state->thru_buffer.get();
  }

  state->play_time = (long)timeGetTime() + state->time_correction;

  midi_event *m = state->play_buffer.peek();
  while (m)
  {
    if (m->ref > state->play_time)
    {
      break;
    }

    if (m->data)
    {

      if (m->data == START_AUDIO)
        state->audio_player->StartAudio();
      else

      if (m->data == SYSEX_EVENT)
        outsysex(state);
      else

      if (midiOutShortMsg(state->hout, m->data) == MIDIERR_NOTREADY)
      {
        // try again later
        timeSetEvent(state->min_timer_period, state->min_timer_period * 5, midiIntTimerHandler, (DWORD)state, TIME_ONESHOT);
        return;
      }
    }
    (void) state->play_buffer.get();
    m = state->play_buffer.peek();
  }

  // compute delta time for next interrupt
  long delay = 100; // default in millisec
  if (m)
  {
    delay = (long)m->ref - (long)state->play_time;
  }
  if (delay < (long)state->min_timer_period)
    delay = (long)state->min_timer_period;
  else if (delay > (long)state->max_timer_period)
    delay = (long)state->max_timer_period;
  timeSetEvent((UINT)delay, state->min_timer_period, midiIntTimerHandler, (DWORD)state, TIME_ONESHOT);
}





// handle incoming midi data (midi clock source) (songpointer)
void FAR PASCAL midiMidiInputHandler(
  HMIDIIN hMidiIn,
  WORD wMsg,
  DWORD dwInstance,
  DWORD dwParam1,
  DWORD dwParam2)
{
    tWinPlayerState *state = (tWinPlayerState *)dwInstance;
    long now;

    now = (long)timeGetTime();

    switch(wMsg)
    {
        case MIM_DATA:
          if ( dwParam1 == 0xf8 )
          {
            state->signal_time = now;
            state->virtual_clock += state->ticks_per_signal;
            return;
          }

          // ignore active sensing and real time messages except midi stop
          if ( (dwParam1 != 0xf8) &&
               (dwParam1 != 0xfa) &&
               (dwParam1 != 0xfb) &&
               (dwParam1 != 0xFE) )
          {
            state->recd_buffer.put(dwParam1, state->virtual_clock + ( ((now - state->signal_time) * 1000L) / state->time_per_tick) );

            /* Midi thru, do not put stop-play thru */
            if ( state->soft_thru && (dwParam1 != 0xfc) )
            {
              if (!state->thru_buffer.empty() || midiOutShortMsg(state->hout, dwParam1) == MIDIERR_NOTREADY)
              {
                // device busy, output during normal play
                state->thru_buffer.put(dwParam1, 0);
              }
            }
          }
          break;
        case MIM_OPEN:
        case MIM_ERROR:
        default:
            break;
    }
}





// play output (midi clock source) (songpointer)
void FAR PASCAL midiMidiTimerHandler(UINT wTimerId, UINT wMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
  tWinPlayerState *state = (tWinPlayerState *)dwUser;
  if ( !state->playing )
    return;

  // output what was left from midi thru
  while (!state->thru_buffer.empty())
  {
    midi_event *m = state->thru_buffer.peek();
    if (midiOutShortMsg(state->hout, m->data) == MIDIERR_NOTREADY)
    {
      timeSetEvent(state->min_timer_period, state->min_timer_period * 5, midiMidiTimerHandler, (DWORD)state, TIME_ONESHOT);
      return;
    }
    (void)state->thru_buffer.get();
  }

  state->play_time = (long)timeGetTime();
  /* How many ticks since last signal? */
  long delta_clock = ((state->play_time - state->signal_time) * 1000L) / state->time_per_tick;

  if (delta_clock > (2 * state->ticks_per_signal)) /* Too many? */
  {
    state->play_clock = state->virtual_clock; /* Yes, means tape stopped */
  }
  else
  {
    state->play_clock = state->virtual_clock + delta_clock;
  }

  midi_event *m = state->play_buffer.peek();
  while (m)
  {
    if ((long)m->ref > state->play_clock)
      break;

    if (m->data)
    {

      if (m->data == SYSEX_EVENT)
        outsysex(state);
      else

      if (midiOutShortMsg(state->hout, m->data) == MIDIERR_NOTREADY)
      {
        // try again later
        timeSetEvent(state->min_timer_period, state->min_timer_period * 5, midiMidiTimerHandler, (DWORD)state, TIME_ONESHOT);
        return;
      }
    }
    (void) state->play_buffer.get();
    m = state->play_buffer.peek();
  }

  // compute delta time for next interrupt
  long delay = 100; // default in millisec

  if (m)
  {
    delay = (((long)m->ref - state->play_clock) * state->time_per_tick) / 1000L;
  }
  if (delay < (long)state->min_timer_period)
    delay = (long)state->min_timer_period;
  else if (delay > (long)state->max_timer_period)
    delay = (long)state->max_timer_period;

  timeSetEvent((UINT)delay, state->min_timer_period, midiMidiTimerHandler, (DWORD)state, TIME_ONESHOT);
}





// handle incoming midi data (MTC clock source)
void FAR PASCAL midiMtcInputHandler(
  HMIDIIN hMidiIn,
  WORD wMsg,
  DWORD dwInstance,
  DWORD dwParam1,
  DWORD dwParam2)
{
    tWinPlayerState *state = (tWinPlayerState *)dwInstance;
    long now;

    now = (long)timeGetTime();

    switch(wMsg)
    {
        case MIM_DATA:

          if ( (dwParam1 & 0x000000ff) == 0xf1 )
          {
            state->last_qfm = (dwParam1 & 0x00007000) >> 12;
            if (state->mtc_valid)
            {
              if ( (state->last_qfm % 4) == 0 )
              {
                state->signal_time = now;
                state->mtc_frames++;
              }
            }
            else
            {
              union
              {
                DWORD w;
                unsigned char c[4];
              } u;
              u.w = dwParam1;
              state->qfm_bits |= (0x0001 << state->last_qfm);
              switch (state->last_qfm)
              {
                case 0:
                  state->signal_time = now;
                  state->mtc_start.fm = u.c[1] & 0x0f;
                  break;
                case 1:
                  state->mtc_start.fm |= ((u.c[1] & 0x0f) << 4);
                  break;
                case 2:
                  state->mtc_start.sec = u.c[1] & 0x0f;
                  break;
                case 3:
                  state->mtc_start.sec |= ((u.c[1] & 0x0f) << 4);
                  break;
                case 4:
                  state->mtc_start.min = u.c[1] & 0x0f;
                  break;
                case 5:
                  state->mtc_start.min |= ((u.c[1] & 0x0f) << 4);
                  break;
                case 6:
                  state->mtc_start.hour = u.c[1] & 0x0f;
                  break;
                case 7:
                  state->mtc_start.hour |= ((u.c[1] & 0x01) << 4);
                  state->mtc_start.type = ((u.c[1] & 0x06) >> 1);
                  if (state->qfm_bits == 0xff)
                  {
                    long mtc_time;
                    state->signal_time = now;
                    Mtc2Frames( state );
                    GetMtcTime( state, mtc_time );
                    state->recd_buffer.put( 0xf1, mtc_time );
                    state->mtc_valid = TRUE;
                  }
                  state->qfm_bits = 0;
                  break;
              } /* switch last_qfm */
            } /* mtc_valid */
            return;
          } /* 0xf1 */

          // ignore active sensing and real time messages except midi stop
          if ( (dwParam1 & 0x000000ff) < 0xf8)
          {
            if (state->mtc_valid)
            {
              long mtc_time;
              GetMtcTime( state, mtc_time );
              state->recd_buffer.put(dwParam1, mtc_time + (now - state->signal_time) );
            }

            /* Midi thru */
            if ( state->soft_thru )
            {
              if (!state->thru_buffer.empty() || midiOutShortMsg(state->hout, dwParam1) == MIDIERR_NOTREADY)
              {
                // device busy, output during normal play
                state->thru_buffer.put(dwParam1, 0);
              }
            }
          }
          break;
        case MIM_OPEN:
        case MIM_ERROR:
        default:
            break;
    }
}




// play output (MTC clock source)
void FAR PASCAL midiMtcTimerHandler(UINT wTimerId, UINT wMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
  tWinPlayerState *state = (tWinPlayerState *)dwUser;
  if ( !state->playing )
    return;
  if ( state->doing_mtc_rec )
    return;

  // output what was left from midi thru
  while (!state->thru_buffer.empty())
  {
    midi_event *m = state->thru_buffer.peek();
    if (midiOutShortMsg(state->hout, m->data) == MIDIERR_NOTREADY)
    {
      timeSetEvent(state->min_timer_period, state->min_timer_period * 5, midiMtcTimerHandler, (DWORD)state, TIME_ONESHOT);
      return;
    }
    (void)state->thru_buffer.get();
  }

  long now = (long)timeGetTime();
  if ( state->mtc_valid )
  {
    GetMtcTime( state, state->play_time );
    state->play_time += now - state->signal_time;
  }
  else
  {
    /* Tape not running */
    return;
  }

  midi_event *m = state->play_buffer.peek();
  while (m)
  {
    if (m->ref > state->play_time)
    {
      break;
    }

    if (m->data)
    {
      if (m->data == SYSEX_EVENT)
        outsysex(state);
      else

      if (midiOutShortMsg(state->hout, m->data) == MIDIERR_NOTREADY)
      {
        // try again later
        timeSetEvent(state->min_timer_period, state->min_timer_period * 5, midiMtcTimerHandler, (DWORD)state, TIME_ONESHOT);
        return;
      }
    }
    (void) state->play_buffer.get();
    m = state->play_buffer.peek();
  }

  // compute delta time for next interrupt
  long delay = 100; // default in millisec
  if (m)
  {
    delay = (long)m->ref - (long)state->play_time;
  }
  if (delay < (long)state->min_timer_period)
    delay = (long)state->min_timer_period;
  else if (delay > (long)state->max_timer_period)
    delay = (long)state->max_timer_period;
  timeSetEvent((UINT)delay, state->min_timer_period, midiMtcTimerHandler, (DWORD)state, TIME_ONESHOT);
}



void CALLBACK MidiOutProc(
  HMIDIOUT hmo,
  UINT wMsg,
  DWORD dwInstance,
  DWORD dwParam1,
  DWORD dwParam2
)
{
  if (wMsg == MOM_DONE) {
    MIDIHDR *hdr = (MIDIHDR *)dwParam1;
    tWinSysexBuffer *buf = (tWinSysexBuffer *)hdr->dwUser;
    if (buf != 0) {  // ignore OutNow() buffers
      buf->Release();
      OutputDebugString("release\n");
    }
  }
}


} // extern "C"

