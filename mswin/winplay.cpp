
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

#include "config.h"
#include "winplay.h"
#include "jazzdll.h"
#include "trackwin.h"
#include "jazz.h"
#include "dialogs.h"

#include <dos.h>

// for msvc uncomment these
#ifdef MSVC
#define enable()
#define disable()
#endif

#if 0
FILE* logfile;
#define OPENLOG logfile=fopen("logfile.txt","wt")
#define CLOSELOG fclose(logfile)
#define LOG(a,b,c) fprintf(a,b,c)
#endif

#if 0
/* got these unresolved externals with wx166F and VC++ 5.0 */
extern "C" {
int  PROIO_yywrap(void)
{
  return 1;
}
void *alloca(size_t size) { return malloc(size); }
}
#endif
char wxDummyChar = 0;


tWinPlayer::tWinPlayer(tSong *song)
  : tPlayer(song)
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
  state->soft_thru = Config(C_SoftThru);
  state->doing_mtc_rec = FALSE;
  state->audio_player = 0;

  long ilong = -1, olong = -1;
  if (!Config.Get(C_WinInputDevice, ilong) || !Config.Get(C_WinOutputDevice, olong))
    SettingsDlg(ilong, olong);
  //else if (ilong < 0 || olong < 0)
  // only output device MUST be there
  else if (olong < 0)
    SettingsDlg(ilong, olong);

  // select input device
  if (ilong >= 0)
  {
    UINT dev = (UINT)ilong;
    UINT rc;
    switch (Config(C_ClockSource))
    {
      case CsMidi:
        rc = midiInOpen(&state->hinp, dev, (DWORD)midiMidiInputHandler, (DWORD)state, CALLBACK_FUNCTION);
        break;
      case CsMtc:
        rc = midiInOpen(&state->hinp, dev, (DWORD)midiMtcInputHandler, (DWORD)state, CALLBACK_FUNCTION);
        break;
      case CsInt:
      case CsFsk:
      default:
        rc = midiInOpen(&state->hinp, dev, (DWORD)midiIntInputHandler, (DWORD)state, CALLBACK_FUNCTION);
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


int tWinPlayer::Installed()
{
  return timer_installed && state->hout;
}


tWinPlayer::~tWinPlayer()
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




void tWinPlayer::SettingsDlg(long &idev, long &odev)
{
  tNamedValue Devs[MAX_MIDI_DEVS];
  UINT i;

  // select input device
  UINT ninp = midiInGetNumDevs();
  for (i = 0; i < ninp; i++)
  {
    MIDIINCAPS caps;
    midiInGetDevCaps(i, &caps, sizeof(caps));
    Devs[i].Name = copystring(caps.szPname);
    Devs[i].Value = i;
  }
  Devs[i].Name = 0;
  Devs[i].Value = 0;

  wxDialogBox *panel = new wxDialogBox(TrackWin, "Input MIDI device", TRUE);
  tMidiDeviceDlg *dlg = new tMidiDeviceDlg(TrackWin, Devs, &idev );
  dlg->EditForm(panel);
  panel->Fit();
  panel->Show(TRUE);
  for (i = 0; i < ninp; i++)
    delete [] Devs[i].Name;

  // select output device
  UINT nout = midiOutGetNumDevs();
  for (i = 0; i < nout; i++)
  {
    MIDIOUTCAPS caps;
    midiOutGetDevCaps(i, &caps, sizeof(caps));
    Devs[i].Name = copystring(caps.szPname);
    Devs[i].Value = i;
  }
  Devs[i].Name = copystring("Midi Mapper");
  Devs[i].Value = MAX_MIDI_DEVS;
  i++;
  Devs[i].Name = 0;
  Devs[i].Value = 0;

  panel = new wxDialogBox(TrackWin, "Output MIDI device", TRUE);
  dlg = new tMidiDeviceDlg(TrackWin, Devs, &odev );
  dlg->EditForm(panel);
  panel->Fit();
  panel->Show(TRUE);
  for (i = 0; i < nout+1; i++)
    delete [] Devs[i].Name;

  if (idev >= 0)
    Config.Put(C_WinInputDevice, idev);
  else
    Config.Get(C_WinInputDevice, idev);
  if (odev >= 0)
    Config.Put(C_WinOutputDevice, odev);
  else
    Config.Get(C_WinOutputDevice, odev);
}

void tWinPlayer::SetSoftThru(int on, int idev, int odev)
{
  state->soft_thru = on;
}

tEvent *tWinPlayer::Dword2Event(DWORD dw)
{
  union {
    DWORD w;
    uchar c[4];
  } u;
  u.w = dw;

  tEvent *e = 0;

  switch(u.c[0] & 0xf0)
  {
    case 0x80:
      e = new tKeyOff(0, u.c[0] & 0x0f, u.c[1]);
      break;

    case 0x90:
      if (u.c[2])
        e = new tKeyOn(0, u.c[0] & 0x0f, u.c[1], u.c[2], 0);
      else
        e = new tKeyOff(0, u.c[0] & 0x0f, u.c[1]);
      break;

    case 0xA0:
      e = new tKeyPressure(0, u.c[0] & 0x0f, u.c[1], u.c[2]);
      break;

    case 0xB0:
	if (u.c[1] != 0x7b)
          e = new tControl(0, u.c[0] & 0x0f, u.c[1], u.c[2]);
      break;

    case 0xC0:
      e = new tProgram(0, u.c[0] & 0x0f, u.c[1]);
      break;

    case 0xD0:
      e = new tChnPressure(0, u.c[0] & 0x0f, u.c[1]);
      break;

    case 0xE0:
      e = new tPitch(0, u.c[0] & 0x0f, u.c[1], u.c[2]);
      break;
  }
  return e;
}


DWORD tWinPlayer::Event2Dword(tEvent *e)
{
  union {
    DWORD w;
    uchar c[4];
  } u;
  u.w = 0;

  int Stat = e->Stat;
  switch (Stat)
  {
    case StatKeyOn:
      {
	tKeyOn *k = e->IsKeyOn();
	u.c[0] = 0x90 | k->Channel;
	u.c[1] = k->Key;
	u.c[2] = k->Veloc;
      }
      break;

    case StatKeyOff:
      {
	tKeyOff *k = e->IsKeyOff();
	u.c[0] = 0x80 | k->Channel;
	u.c[1] = k->Key;
	u.c[2] = 0;
      }
      break;

    case StatProgram:
      {
	tProgram *k = e->IsProgram();
	u.c[0] = 0xC0 | k->Channel;
	u.c[1] = k->Program;
      }
      break;

    case StatChnPressure:
      {
	tChnPressure *k = e->IsChnPressure();
	u.c[0] = 0xC0 | k->Channel;
	u.c[1] = k->Value;
      }
      break;

    case StatControl:
      {
	tControl *k = e->IsControl();
	u.c[0] = 0xB0 | k->Channel;
	u.c[1] = k->Control;
	u.c[2] = k->Value;
      }
      break;

    case StatKeyPressure:
      {
	tKeyPressure *k = e->IsKeyPressure();
	u.c[0] = 0xA0 | k->Channel;
	u.c[1] = k->Key;
	u.c[2] = k->Value;
      }
      break;

    case StatPitch:
      {
	tPitch *k = e->IsPitch();
	int     v = k->Value + 8192;
	u.c[0] = 0xE0 | k->Channel;
	u.c[1] = (uchar)(v & 0x7F);
	u.c[2] = (uchar)(v >> 7);
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
        tSetTempo *t = e->IsSetTempo();
        if (t && t->Clock > 0)
        {
          SetTempo( t->GetBPM(), t->Clock );
          OutOfBandEvents.Put( e->Copy() );
        }
      }
      break;

    default:
      break;
  }
  return u.w;
}


long tWinPlayer::Clock2Time(long clock)
{
  if (clock < state->start_clock)
    return state->start_time;
  return (long)( (double)(clock - state->start_clock) * 60000.0 / (double)state->ticks_per_minute + state->start_time);
}

long tWinPlayer::Time2Clock(long time)
{
  if (time < state->start_time)
    return state->start_clock;
  return (long)((double)(time - state->start_time) * (double)state->ticks_per_minute / 60000.0 + state->start_clock);
}

void tWinPlayer::SetTempo(long bpm, long clock)
{
  long t1 = Clock2Time(clock);
  state->ticks_per_minute = (long)bpm * (long)Song->TicksPerQuarter;
  long t2 = Clock2Time(clock);
  state->start_time += (t1 - t2);
}

long tWinPlayer::RealTimeClock2Time(long clock)
{
  if (clock < state->start_clock)
    return real_start_time;
  return (long)( (double)(clock - state->start_clock) * 60000.0 / (double)real_ticks_per_minute + real_start_time);
}

long tWinPlayer::Time2RealTimeClock(long time)
{
  if (time < real_start_time)
    return state->start_clock;
  return (long)((double)(time - real_start_time) * (double)real_ticks_per_minute / 60000.0 + state->start_clock);
}

void tWinPlayer::SetRealTimeTempo(long bpm, long clock)
{
  long t1 = RealTimeClock2Time(clock);
  real_ticks_per_minute = (long)bpm * (long)Song->TicksPerQuarter;
  long t2 = RealTimeClock2Time(clock);
  real_start_time += (t1 - t2);
}


int tWinPlayer::OutSysex(tEvent *e, DWORD time)
{
  tSysEx *sx = e->IsSysEx();
  if (sx == 0)
    return 1;

  if (state->play_buffer.nfree() < 2)
    return 1;

  state->sysex_found = TRUE;
  tWinSysexBuffer *buf = state->osx_buffers->AllocBuffer();
  buf->PrepareOut(state->hout, sx->Data, sx->Length - 1);
  state->play_buffer.put(SYSEX_EVENT, time);
  state->play_buffer.put((DWORD)buf, time);
  return 0;
}


int tWinPlayer::OutEvent(tEvent *e)
{
  DWORD d = Event2Dword(e);
  if (d)
    state->play_buffer.put(d, Clock2Time(e->Clock));
  else if (e->IsSysEx() && (e->Clock > 0))
    OutSysex(e, Clock2Time(e->Clock));
  return 0;
}

int tWinMidiPlayer::OutEvent(tEvent *e)
{
  DWORD d = Event2Dword(e);
  if (d)
    state->play_buffer.put(d, e->Clock);
  else if (e->IsSysEx() && (e->Clock > 0))
    OutSysex(e, e->Clock);
  return 0;
}


void tWinPlayer::OutNow(tEvent *e)
{
  DWORD d = Event2Dword(e);
  if (d)
    midiOutShortMsg(state->hout, d);
  else if (e->Stat == StatSetTempo)
  {
    if (state->playing)
      SetTempo(e->IsSetTempo()->GetBPM(), OutClock);
  }
  else if (e->Stat == StatSysEx)
  {
    tSysEx *s = e->IsSysEx();
    if (s->Length + 1 < maxSysLen)
    {
      pSysBuf[0] = 0xf0;
      memcpy(pSysBuf + 1, s->Data, s->Length);

      pSysHdr->lpData = (LPSTR)pSysBuf;
      pSysHdr->dwBufferLength = s->Length + 1;
      pSysHdr->dwUser = 0;

      if (midiOutPrepareHeader(state->hout, pSysHdr, sizeof(MIDIHDR)) == 0)
        midiOutLongMsg(state->hout, pSysHdr, sizeof(MIDIHDR));
      // here we should wait, until the data are physically sent.
      // but there is no API call for this?!
      midiOutUnprepareHeader(state->hout, pSysHdr, sizeof(MIDIHDR));
    }
  }
}


void tWinPlayer::OutNow(tParam *r)
{
  OutNow(&r->Msb);
  OutNow(&r->Lsb);
  OutNow(&r->DataMsb);
  OutNow(&r->ResetMsb);
  OutNow(&r->ResetLsb);
}

void tWinPlayer::FillMidiClocks( long to )
{
  while (midiClockOut <= to)
  {
    tMidiClock *e = new tMidiClock( midiClockOut );
    PlayBuffer.Put( e );
    midiClockOut = midiClockOut + state->ticks_per_signal;
  }
  PlayBuffer.Sort();
}

void tWinPlayer::OutBreak(long clock)
{
  if (Config(C_RealTimeOut))
  {
    FillMidiClocks( OutClock );
    FlushToDevice( OutClock );
  }
  else
  {
    state->play_buffer.put(0, Clock2Time(clock));
  }
}

void tWinMidiPlayer::OutBreak(long clock)
{
  if (Config(C_RealTimeOut))
  {
    FillMidiClocks( OutClock );
    FlushToDevice( OutClock );
  }
  else
  {
    state->play_buffer.put(0, clock);
  }
}

void tWinPlayer::OutBreak()
{
  OutBreak(OutClock);
}

static DWORD GetMtcTime( tWinPlayerState *state )
{
  DWORD frames = state->mtc_frames;
  switch (state->mtc_start.type)
  {
    case 0:
      return( ((frames / 24) * 1000) + (((frames % 24) * state->time_per_frame) / 1000) );
    case 1:
      return( ((frames / 25) * 1000) + (((frames % 25) * state->time_per_frame) / 1000) );
    case 2:
    case 3:
      return( ((frames / 30) * 1000) + (((frames % 30) * state->time_per_frame) / 1000) );
    default:
      return( 0 );
  }
}

void tWinPlayer::StartPlay(long Clock, long LoopClock, int Continue)
{
  state->play_buffer.clear();
  state->recd_buffer.clear();
  state->sysex_found = FALSE;

  state->ticks_per_minute  = Song->TicksPerQuarter * Song->Speed();
  real_ticks_per_minute    = state->ticks_per_minute;
  state->ticks_per_signal  = Song->TicksPerQuarter / 24;
  state->time_per_tick = 60000000L / state->ticks_per_minute;
  state->time_correction   = 0;

  if (Config(C_ClockSource) == CsMtc)
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
  TrackWin->NewPlayPosition(PlayLoop->Ext2IntClock(Clock));
  state->playing = TRUE;  // allow for SetTempo in OutNow()
  tPlayer::StartPlay(Clock, LoopClock, Continue);

  if (Config(C_RealTimeOut))
  {
    tMetaEvent *e;
    if (!Continue)
      e = new tStartPlay( 0 );
    else
      e = new tContPlay( 0 );
    OutNow( e );
    FillMidiClocks( PlayBuffer.GetLastClock() ); // also does a sort
  }


  switch (Config(C_ClockSource)) {
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
      timeSetEvent(state->min_timer_period, state->min_timer_period, midiIntTimerHandler, (DWORD)state, TIME_ONESHOT);
      break;
  }

}


void tWinPlayer::StopPlay()
{
  wxBeginBusyCursor();
  state->playing = FALSE;
  tPlayer::StopPlay();
  if (Config(C_RealTimeOut))
  {
    tStopPlay *e = new tStopPlay(0);
    OutNow( e );
    delete e;
  }
  AllNotesOff();
  RecdBuffer.Keyoff2Length();

  if (state->hout)
  {
    if (state->sysex_found)
      midiOutReset(state->hout);
    int n = state->osx_buffers->Size();
    for (int i = 0; i < n; i++) {
      tWinSysexBuffer *buf = state->osx_buffers->At(i);
      if (buf->IsPrepared())
        buf->UnprepareOut(state->hout);
    }
    state->osx_buffers->ReleaseAllBuffers();
  }


  /**
  // sysex recording not finished yet.
  if (state->hinp)
  {
    midiInReset(state->hinp);
    int n = state->isx_buffers->Size();
    for (int i = 0; i < n; i++) {
      tWinSysexBuffer *buf = state->isx_buffers->At(i);
      if (buf->IsPrepared())
        buf->UnprepareIn(state->hinp);
      state->isx_buffers->ReleaseAllBuffers();
    }
  }
  **/

  wxEndBusyCursor();
}


void tWinPlayer::FlushToDevice()
// try to send all events up to OutClock to device
{
  if (Config(C_RealTimeOut))
  {
    FillMidiClocks( OutClock );
  }
  FlushToDevice( OutClock );
  OutBreak(OutClock);
}

void tWinPlayer::FlushToDevice( long clock )
{
  tEventIterator Iterator(&PlayBuffer);
  tEvent *e = Iterator.Range(0, clock);
  if (e) {
    do {
      OutEvent(e);
      e->Kill();
      e = Iterator.Next();
    } while (e);

    PlayBuffer.Cleanup(0);
  }
}

long tWinIntPlayer::GetRealTimeClock()
{
  while (!state->recd_buffer.empty())
  {
    midi_event *m = state->recd_buffer.get();

    // Event?
    tEvent     *e = Dword2Event(m->data);
    if (e)
    {
      e->Clock = PlayLoop->Ext2IntClock(Time2RealTimeClock(m->ref));
      RecdBuffer.Put(e);
    }
  }

  long clock = Time2RealTimeClock( (long)timeGetTime() + state->time_correction );

  TrackWin->NewPlayPosition(PlayLoop->Ext2IntClock(clock/48 * 48));

  if ( !OutOfBandEvents.IsEmpty() )
  {
    tEventIterator Iterator(&OutOfBandEvents);
    tEvent *e = Iterator.Range(0, clock);
    while (e)
    {
      switch (e->Stat)
      {
        case StatSetTempo:
          SetRealTimeTempo( ((tSetTempo *)e)->GetBPM(), clock );
          break;
        default:
          break;
      }
      e->Kill();
      e = Iterator.Next();
    }
    OutOfBandEvents.Cleanup(0);
  }

  return clock;
}

long tWinMidiPlayer::GetRealTimeClock()
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
      union {
        DWORD w;
        uchar c[4];
      } u;
      Midi->StopPlay();
      u.w = m->data;
      clock = ((long)u.c[1] + (128L * (long)u.c[2])) * (Song->TicksPerQuarter / 4);
      Midi->StartPlay( clock, 0, 1 );
      return -1;
    }

    // Event?
    tEvent     *e = Dword2Event(m->data);
    if (e)
    {
      e->Clock = PlayLoop->Ext2IntClock( m->ref );
      RecdBuffer.Put(e);
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

  TrackWin->NewPlayPosition(PlayLoop->Ext2IntClock(clock/48 * 48));
  return clock;
}

long tWinMtcPlayer::GetRealTimeClock()
{

  long clock;

  while (!state->recd_buffer.empty())
  {
    midi_event *m = state->recd_buffer.get();
    if (m->data == 0xf1)
    {
      // MTC starting (from midi input handler)
      Midi->StopPlay();
      clock = PlayLoop->Ext2IntClock( Time2Clock( m->ref ) );
      lastValidMtcClock = clock;
      Midi->StartPlay( clock, 0, 1 );
      return -1;
    }

    // Event?
    tEvent     *e = Dword2Event(m->data);
    if (e)
    {
      e->Clock = PlayLoop->Ext2IntClock(Time2Clock(m->ref));
      RecdBuffer.Put(e);
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

  TrackWin->NewPlayPosition(PlayLoop->Ext2IntClock(clock/48 * 48));

  if ( !OutOfBandEvents.IsEmpty() )
  {
    tEventIterator Iterator(&OutOfBandEvents);
    tEvent *e = Iterator.Range(0, clock);
    while (e)
    {
      switch (e->Stat)
      {
        case StatSetTempo:
          SetRealTimeTempo( ((tSetTempo *)e)->GetBPM(), clock );
          break;
        default:
          break;
      }
      e->Kill();
      e = Iterator.Next();
    }
    OutOfBandEvents.Cleanup(0);
  }

  return clock;
}

void tWinMtcPlayer::InitMtcRec()
{
  state->doing_mtc_rec = TRUE;
  StartPlay( 0, 0, 0 );
}

tMtcTime* tWinMtcPlayer::FreezeMtcRec()
{
  StopPlay();
  state->doing_mtc_rec = FALSE;
  return( new tMtcTime( (long) GetMtcTime( state ), (tMtcType) state->mtc_start.type ) );
}

