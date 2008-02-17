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
// Changes
//
// 2000.03.18        Takashi Iwai <iwai@ww.uni-erlangen.de>
// - Modified for ALSA 0.5.x.
//   You'll need the latest CVS version of ALSA-lib, which is expected
//   to be released as ver.0.5.7.
// - The input/output devices are selected via a pop-up window
//   like OSS driver mode.
//*****************************************************************************

#include "WxWidgets.h"

#include "AlsaPlayer.h"
#include "TrackFrame.h"
#include "TrackWindow.h"
#include "Dialogs.h"
#include "Configuration.h"
#include "Globals.h"

#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>

#include <iostream>

using namespace std;

tAlsaPlayer::tAlsaPlayer(JZSong *song)
  : tPlayer(song)
{
  ithru = othru = 0;

  installed = 1;
  poll_millisec = 25;
  recd_clock = 0;
  echo_clock = 0;

  if (snd_seq_open(&handle, "hw", SND_SEQ_OPEN_DUPLEX, 0) < 0)
  {
    perror("open sequencer");
    installed = 0;
    return;
  }

  // set myself into non blocking mode
  if (set_blocking_mode(0) < 0)
  {
    installed = 0;
    return;
  }
  client = snd_seq_client_id(handle);
  struct pollfd pfds;
  snd_seq_poll_descriptors(handle, &pfds, 1, POLLIN|POLLOUT);

  //JAVE seqfd doesnt seem to be used for anything, not here nor in the base
  // class tPlayer(but heavily in tSeq2Player)
//  seqfd = pfds.fd;

  // create my input/output port
  memset(&self, 0, sizeof(self));
  self.client  = client;
  self.port    = create_port(handle, "Input/Output");

  cerr << "created client:port = " << self.client << ':' << self.port << endl;

  // allocate a queue
  queue = snd_seq_alloc_named_queue(handle, "Jazz++");

  // register my name
  set_client_info(handle, "The JAZZ++ Midi Sequencer");

  // scan input addressess
  scan_clients(iaddr, SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ);

  // scan output addresses
  scan_clients(oaddr, SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE);

  inp_dev = gpConfig->GetValue(C_AlsaInputDevice);
  if (inp_dev < 0)
  {
    cout<<"invalid input device, so selecting one"<<endl;
    inp_dev = select_list(iaddr, "Input Device", inp_dev);
    cout << "Input device is: " << inp_dev << endl;
    gpConfig->Put(C_AlsaInputDevice, inp_dev);
  }
  outp_dev = gpConfig->GetValue(C_AlsaOutputDevice);
  if (outp_dev < 0)
  {
    cout<<"invalid output device, so selecting one"<<endl;
    outp_dev = select_list(oaddr, "Output Device", outp_dev);
  }

  if (inp_dev >= 0)
    subscribe_inp(inp_dev);
  if (outp_dev >= 0)
    subscribe_out(outp_dev);

  set_pool_sizes();

  snd_seq_set_output_buffer_size(handle, 65536);

  if (installed)
  {
    thru = new tAlsaThru();
    SetSoftThru(
      gpConfig->GetValue(C_SoftThru),
      gpConfig->GetValue(C_ThruInput),
      gpConfig->GetValue(C_ThruOutput));
  }

}




void tAlsaPlayer::clear_input_queue()
{
  snd_seq_drop_input(handle);
}

void tAlsaPlayer::set_pool_sizes()
{
  if (snd_seq_set_client_pool_output(handle, 2000) < 0)
    perror("set pool output");
  if (snd_seq_set_client_pool_input(handle, 2000) < 0)
    perror("set pool input");
  if (snd_seq_set_client_pool_output_room(handle, 1000) < 0)
    perror("set pool output room");
}



void tAlsaPlayer::SetSoftThru(int on, int idev, int odev)
{
  if (idev != ithru || odev != othru)
  {
    ithru = idev;
    othru = odev;

    thru->Stop();

  }
  if (on && !thru->IsRunning())
  {
    thru->SetSource(iaddr[ithru].client, iaddr[ithru].port);
    thru->SetDestin(oaddr[othru].client, oaddr[othru].port);

    thru->Start();

  }
  else if (!on && thru->IsRunning())
  {

    thru->Stop();

  }
}

// connect output addrs/queue with my client/oport
void tAlsaPlayer::subscribe_out(int outp)
{
  if (snd_seq_connect_to(handle, self.port, oaddr[outp].client, oaddr[outp].port) < 0)
  {
    perror("subscribe output");
  }
}

// connect input addrs/queue with my client/iport
void tAlsaPlayer::subscribe_inp(int inp)
{
  snd_seq_port_subscribe_t *subs;
  snd_seq_port_subscribe_alloca(&subs);
  snd_seq_port_subscribe_set_time_update(subs, 1);
  snd_seq_port_subscribe_set_queue(subs, queue);
  snd_seq_port_subscribe_set_sender(subs, &iaddr[inp]);
  snd_seq_port_subscribe_set_dest(subs, &self);
  if (snd_seq_subscribe_port(handle, subs) < 0)
  {
    perror("subscribe input");
  }
}

// disconnect output addrs/queue with my client/oport
void tAlsaPlayer::unsubscribe_out(int outp) {
  if (snd_seq_disconnect_to(handle, self.port, oaddr[outp].client, oaddr[outp].port) < 0) {
    perror("unsubscribe output");
  }
}

// connect input addrs/queue with my client/iport
void tAlsaPlayer::unsubscribe_inp(int inp) {
  snd_seq_port_subscribe_t *subs;
  snd_seq_port_subscribe_alloca(&subs);
  snd_seq_port_subscribe_set_time_update(subs, 1);
  snd_seq_port_subscribe_set_queue(subs, queue);
  snd_seq_port_subscribe_set_sender(subs, &iaddr[inp]);
  snd_seq_port_subscribe_set_dest(subs, &self);
  if (snd_seq_unsubscribe_port(handle, subs) < 0) {
    perror("unsubscribe input");
  }
}

// set the name of this client
void tAlsaPlayer::set_client_info(snd_seq_t *handle, const char *name) {
  if (snd_seq_set_client_name(handle, (char *)name) < 0) {
    perror("ioctl");
  }
}

// create a new port
int tAlsaPlayer::create_port(snd_seq_t *handle, const char *name)
{
  return snd_seq_create_simple_port(
    handle,
    (char *)name,
    SND_SEQ_PORT_CAP_READ |
      SND_SEQ_PORT_CAP_SUBS_READ |
      SND_SEQ_PORT_CAP_WRITE |
      SND_SEQ_PORT_CAP_SUBS_WRITE,
      SND_SEQ_PORT_TYPE_MIDI_GENERIC);
}


int tAlsaPlayer::Installed()
{
  return installed;
}

tAlsaPlayer::~tAlsaPlayer()
{
  if (thru)
    delete thru;
  snd_seq_close(handle);
}

// 0 = event successfully sent to driver
// 1 = try again later
int tAlsaPlayer::OutEvent(JZEvent *e, int now)
{
  int rc = 0;
  snd_seq_event_t ev;
  memset(&ev, 0, sizeof(ev));
  switch (e->Stat)
  {
    case StatKeyOn:
      {
        tKeyOn *k = e->IsKeyOn();
        set_event_header(&ev, e->GetClock(), SND_SEQ_EVENT_NOTEON);
        ev.data.note.channel = k->Channel;
        ev.data.note.note = k->Key;
        ev.data.note.velocity = k->Veloc;
        rc = write(&ev, now);
      }
      break;

    case StatKeyOff:
      {
        tKeyOff *k = e->IsKeyOff();
        set_event_header(&ev, e->GetClock(), SND_SEQ_EVENT_NOTEOFF);
        ev.data.note.channel = k->Channel;
        ev.data.note.note = k->Key;
        ev.data.note.velocity = k->OffVeloc;
        rc = write(&ev, now);
      }
      break;

    case StatProgram:
      {
        tProgram *k = e->IsProgram();
        set_event_header(&ev, e->GetClock(), SND_SEQ_EVENT_PGMCHANGE);
        ev.data.control.channel = k->Channel;
        ev.data.control.value = k->Program;
        rc = write(&ev, now);
      }
      break;

    case StatKeyPressure:
      {
        tKeyPressure *k = e->IsKeyPressure();
        set_event_header(&ev, e->GetClock(), SND_SEQ_EVENT_KEYPRESS);
        ev.data.note.channel = k->Channel;
        ev.data.note.note = k->Key;
        ev.data.note.velocity = k->Value;
        rc = write(&ev, now);
      }
      break;

    case StatChnPressure:
      {
        tChnPressure *k = e->IsChnPressure();
        set_event_header(&ev, e->GetClock(), SND_SEQ_EVENT_CHANPRESS);
        ev.data.control.channel = k->Channel;
        ev.data.control.value = k->Value;
        rc = write(&ev, now);
      }
      break;

    case StatControl:
      {
        tControl *k = e->IsControl();
        set_event_header(&ev, e->GetClock(), SND_SEQ_EVENT_CONTROLLER);
        ev.data.control.channel = k->Channel;
        ev.data.control.param = k->Control;
        ev.data.control.value = k->Value;
        rc = write(&ev, now);
      }
      break;

    case StatPitch:
      {
        tPitch *k = e->IsPitch();
        set_event_header(&ev, e->GetClock(), SND_SEQ_EVENT_PITCHBEND);
        ev.data.control.channel = k->Channel;
        ev.data.control.value = k->Value;
        rc = write(&ev, now);
      }
      break;

    case StatSetTempo:
      {
        int bpm = e->IsSetTempo()->GetBPM();
        int us  = (int)( 60.0E6 / (double)bpm );
        set_event_header(&ev, e->GetClock(), SND_SEQ_EVENT_TEMPO);
        snd_seq_ev_set_queue_tempo(&ev, queue, us);
        rc = write(&ev, now);
      }
      break;

    case StatSysEx:
      {
        tSysEx *s = e->IsSysEx();
        // prepend 0xf0
        char *buf = new char[s->Length + 1];
        buf[0] = 0xF0;
        memcpy(buf + 1, s->Data, s->Length);
        set_event_header(&ev, e->GetClock(), s->Length + 1, buf);
        rc = write(&ev, now);
        delete [] buf;
      }
      break;

    default:
      break;
  }
  return rc < 0 ? 1 : 0;
}


void tAlsaPlayer::OutBreak()
{
  OutBreak(OutClock);
}

/** "echos" are used to synchronize.
they are supposed to be read later by the Notify call chain
 */
int tAlsaPlayer::compose_echo(int clock, unsigned int arg)
{
  snd_seq_event_t ev;
  memset(&ev, 0, sizeof(ev));
  ev.source = self;
  ev.dest = self;
  snd_seq_ev_schedule_tick(&ev, queue, 0, clock);
  cout<<"scheduling echo for "<<clock<<endl;
  snd_seq_ev_set_fixed(&ev);
  ev.type = SND_SEQ_EVENT_ECHO;
  ev.data.raw32.d[0] = arg;
  return write(&ev);
}

void tAlsaPlayer::OutBreak(long clock)
{

  while (echo_clock + 48 < clock) {
    echo_clock += 48;
    if (compose_echo(echo_clock, 0) < 0)
      break;
  }

  flush_output();
}

void tAlsaPlayer::StartPlay(long clock, long loopClock, int cont)
{
  cout<<"tAlsaPlayer::StartPlay"<<endl;
  recd_clock = clock;
  echo_clock = clock;
  play_clock = clock;
  flush_output();
  start_timer(clock);
  tPlayer::StartPlay(clock, loopClock, cont);
  Notify();
  //flush_output();
}

void tAlsaPlayer::ResetPlay(long clock)
{
  /* purge queues */
  snd_seq_drop_output_buffer(handle);
  snd_seq_drop_output(handle);

  AllNotesOff();
}

#define FIRST_DELTACLOCK 720
#define DELTACLOCK 960
#define ADVANCE_PLAY 480

/** notify is periodically called by the timer.

it will output events to the output buffer, and update play_clock and recd_clock
 */

void tAlsaPlayer::Notify()
{
  // called by timer
  long Now = GetRealTimeClock();

  cout << "tAlsaPlayer::Notify " << Now << ' ' << play_clock << endl;

  if (Now < 0)
  {
    return;
  }

  if (Now < play_clock)
  {
    // rewind..
    // clear and rebuild
    cout << "tAlsaPlayer::Notify rewind" << endl;
    ResetPlay(Now);
    mPlayBuffer.Clear();
    OutClock = Now + FIRST_DELTACLOCK;
    PlayLoop->PrepareOutput(&mPlayBuffer, Song, Now, OutClock, 0);
    if (AudioBuffer)
    {
      AudioBuffer->Clear();
      PlayLoop->PrepareOutput(AudioBuffer, Song, Now, OutClock, 1);
    }
    mPlayBuffer.Length2Keyoff();
  } else {
    // time to put more events
    if ( Now >= (OutClock - ADVANCE_PLAY) )
    {
      PlayLoop->PrepareOutput(&mPlayBuffer, Song, OutClock, Now + DELTACLOCK, 0);
      if (AudioBuffer)
        PlayLoop->PrepareOutput(AudioBuffer, Song, OutClock, Now + DELTACLOCK, 1);
      OutClock = Now + DELTACLOCK;
      mPlayBuffer.Length2Keyoff();
    }
  }

  play_clock = Now;
  if (mPlayBuffer.nEvents && mPlayBuffer.Events[0]->GetClock() < OutClock)
  {
    FlushToDevice();
  }
  else
  {
    OutBreak();        // does nothing unless OutClock has changed
  }
}

void tAlsaPlayer::set_event_header(snd_seq_event_t *ev, long clock,int type)
{
  memset(ev, 0, sizeof(*ev));
  snd_seq_ev_set_source(ev, self.port);
  snd_seq_ev_set_subs(ev);
  snd_seq_ev_schedule_tick(ev, queue, 0, clock);
  snd_seq_ev_set_fixed(ev);
  ev->type = type;
}

void tAlsaPlayer::set_event_header(snd_seq_event_t *ev, long clock, int len, void *ptr)
{
  memset(ev, 0, sizeof(*ev));
  snd_seq_ev_set_source(ev, self.port);
  snd_seq_ev_set_subs(ev);
  snd_seq_ev_schedule_tick(ev, queue, 0, clock);
  snd_seq_ev_set_variable(ev, len, ptr);
}

/** init the alsa timer  */
int tAlsaPlayer::start_timer(long clock)
{
  int time_base = Song->TicksPerQuarter;
  int cur_speed = Song->GetTrack(0)->GetCurrentSpeed(clock);
  init_queue_tempo(time_base, cur_speed);
  start_queue_timer(clock);
  return 0;
}

/** set initial tempo */
void tAlsaPlayer::init_queue_tempo(int time_base, int bpm)
{
  snd_seq_queue_tempo_t *qtempo;
  snd_seq_queue_tempo_alloca(&qtempo);
  snd_seq_queue_tempo_set_ppq(qtempo, time_base);
  snd_seq_queue_tempo_set_tempo(qtempo, 60*1000000/bpm);
  if (snd_seq_set_queue_tempo(handle, queue, qtempo) < 0)
    perror("set_queue_tempo");
}


/** immediately start the alsa queue timer.
do this by sending an "start" event to the queue
*/
void tAlsaPlayer::start_queue_timer(long clock)
{
  stop_queue_timer(); // to be sure

  snd_seq_event_t ev;
  memset(&ev, 0, sizeof(ev));
  snd_seq_ev_set_source(&ev, self.port);
  snd_seq_ev_set_direct(&ev);
  snd_seq_ev_set_queue_pos_tick(&ev, queue, clock);
  int rv=0;
  rv=write(&ev, 1);
  if(rv<0)
    cout<<"tAlsaPlayer::start_queue_timer write failed"<<endl;
  snd_seq_ev_set_queue_continue(&ev, queue);
  rv=write(&ev, 1);
  if(rv<0)
    cout<<"tAlsaPlayer::start_queue_timer write failed"<<endl;

  cout<<"tAlsaPlayer::start_queue_timer added trial-and-terror start_queue"<<endl;
  snd_seq_start_queue(handle, queue, NULL);
}

/** immediately stop the timer, by sending a stop event to the alsa queue*/
void tAlsaPlayer::stop_queue_timer()
{
  snd_seq_event_t ev;
  memset(&ev, 0, sizeof(ev));
  snd_seq_ev_set_source(&ev, self.port);
  snd_seq_ev_set_direct(&ev);
  snd_seq_ev_set_queue_stop(&ev, queue);
  write(&ev, 1);
}

/** write an event to the queue, return < 0 on failure */
int tAlsaPlayer::write(snd_seq_event_t *ev, int now) {
  if (now) {
    snd_seq_ev_set_direct(ev);
    return snd_seq_event_output_direct(handle, ev);
  }
  int rc = snd_seq_event_output(handle, ev) ;
  if (rc < 0 && rc != -EAGAIN) {
    snd_seq_extract_output(handle, NULL); // remove the error event
  }
  return rc;
}

void tAlsaPlayer::flush_output() {
  snd_seq_drain_output(handle);
}

int tAlsaPlayer::set_blocking_mode(int enable) {
  int rc;

  if ((rc = snd_seq_nonblock(handle, !enable)) < 0)
    perror("blocking mode");

  return rc;
}

void tAlsaPlayer::StopPlay()
{
  tPlayer::StopPlay();
  ResetPlay(0);
  flush_output();
  stop_queue_timer();
  clear_input_queue();
  gpTrackWindow->NewPlayPosition(-1L);
  RecdBuffer.Keyoff2Length();
}

void tAlsaPlayer::StartAudio()
{
}

/** called from GetRealTimeClock.
parses events in the queu.

   sets  recd_clock, from event timestamps
 */
void tAlsaPlayer::recd_event(snd_seq_event_t *ev)
{
  JZEvent *e = 0;
  cout << "tAlsaPlayer::recd_event got "<<(int)ev->type<<" (echo is "<<SND_SEQ_EVENT_ECHO<<") "<<endl;
  switch (ev->type)
  {

    case SND_SEQ_EVENT_NOTEON:
      if (ev->data.note.velocity > 0)
        e = new tKeyOn(0, ev->data.note.channel, ev->data.note.note, ev->data.note.velocity);
      else
        e = new tKeyOff(0, ev->data.note.channel, ev->data.note.note, 0);
      break;

    case SND_SEQ_EVENT_NOTEOFF:
      e = new tKeyOff(0, ev->data.note.channel, ev->data.note.note, ev->data.note.velocity);
      break;

    case SND_SEQ_EVENT_PGMCHANGE:
      e = new tProgram(0, ev->data.control.channel, ev->data.control.value);
      break;

    case SND_SEQ_EVENT_KEYPRESS:
      e = new tKeyPressure(0, ev->data.note.channel, ev->data.note.note, ev->data.note.velocity);
      break;

    case SND_SEQ_EVENT_CHANPRESS:
      e = new tChnPressure(0, ev->data.control.channel, ev->data.control.value);
      break;

    case SND_SEQ_EVENT_CONTROLLER:
      e = new tControl(0, ev->data.control.channel, ev->data.control.param, ev->data.control.value);
      break;

    case SND_SEQ_EVENT_PITCHBEND:
      e = new tPitch(0, ev->data.control.channel, ev->data.control.value);
      break;

    case SND_SEQ_EVENT_SYSEX:
      e = new tSysEx(0, ((unsigned char *)ev->data.ext.ptr) + 1, ev->data.ext.len - 1);
      break;

    case SND_SEQ_EVENT_ECHO:
      if (ev->data.raw32.d[0])
      {
        StartAudio();
      }
      else
      {
        recd_clock = ev->time.tick;
        cout<<"recd_clock now:"<<recd_clock<<endl;
      }
      break;

  }
  if (e)
  {
    // Not all events are to be recorded.  Only those filtered out and put into
    // the event.
    e->SetClock(PlayLoop->Ext2IntClock(ev->time.tick));
    RecdBuffer.Put(e);
  }
}

/** called periodically from Notify
it calculates the rt clock by looking at timestamps on events in the queue, and also updates the display, so the name is
not well chosen.
 */
long tAlsaPlayer::GetRealTimeClock()
{
  // input recorded events (including my echo events)
  snd_seq_event_t *ie;
  long old_recd_clock = recd_clock;
  while (snd_seq_event_input(handle, &ie) >= 0 && ie != 0)
  {
    recd_event(ie);
    snd_seq_free_event(ie);
  }
  if (recd_clock != old_recd_clock)
  {
    gpTrackWindow->NewPlayPosition(PlayLoop->Ext2IntClock(recd_clock/48 * 48));
  }
  return recd_clock;
}


/** this function goes through each client, and each port on each client*/
void tAlsaPlayer::scan_clients(tAlsaDeviceList &list, int cap)
{
  snd_seq_client_info_t *cinfo;
  snd_seq_port_info_t *pinfo;

  snd_seq_client_info_alloca(&cinfo);
  snd_seq_port_info_alloca(&pinfo);

  list.Clear();

  snd_seq_client_info_set_client(cinfo, 0);
  while (snd_seq_query_next_client(handle, cinfo) >= 0)
  {
    int c = snd_seq_client_info_get_client(cinfo);
    if (c == self.client)
      continue;
    snd_seq_port_info_set_client(pinfo, c);
    snd_seq_port_info_set_port(pinfo, -1);
    while (snd_seq_query_next_port(handle, pinfo) >= 0)
    {
      if ((snd_seq_port_info_get_capability(pinfo) & cap) == cap)
      {
        snd_seq_addr_t a = *snd_seq_port_info_get_addr(pinfo);
        char buf[500];
        strcpy(buf, snd_seq_client_info_get_name(cinfo));
        strcat(buf, " ");
        strcat(buf, snd_seq_port_info_get_name(pinfo));
        list.add(buf, a);
      }
    }
  }
}

int tAlsaPlayer::FindMidiDevice()
{
  if (inp_dev != -1)
    unsubscribe_inp(inp_dev);
  inp_dev = select_list(iaddr, "Input MIDI device", inp_dev);
  gpConfig->Put(C_AlsaInputDevice, inp_dev);
  if (inp_dev != -1)
    subscribe_inp(inp_dev);

  if (outp_dev != -1)
    unsubscribe_out(outp_dev);
  outp_dev = select_list(oaddr, "Output MIDI device", outp_dev);
  gpConfig->Put(C_AlsaOutputDevice, outp_dev);
  if (outp_dev != -1)
    subscribe_out(outp_dev);
  return 0;
}

int tAlsaPlayer::select_list(tAlsaDeviceList &list, char *title, int def_device)
{

  if (list.GetCount() > 0)
  {
    int ndevs = list.GetCount();
    wxString devs[ndevs];

    for (int i = 0; i < ndevs; i++)
    {
      devs[i] = list.GetName(i);
    }

    wxSingleChoiceDialog dialog(gpTrackWindow, title, title, ndevs, devs);

    if (def_device != -1)
    {
      dialog.SetSelection(def_device);
    }

    int res = dialog.ShowModal();
    int k = dialog.GetSelection();

    if (res == wxCANCEL)
    {
      k = -1;
    }

    return k;

  }
  else
  {
    cerr << "no device found!" << endl;
    return -1;
  }
}


void tAlsaDeviceList::print(const char *msg)
{
  cout << msg << endl;
  int i = 0;
  for (
    vector<snd_seq_addr_t>::const_iterator iSound = addr.begin();
    iSound != addr.end();
    ++iSound)
  {
    const snd_seq_addr_t& a = *iSound;
    cout << GetName(i++) << " = " << (int)a.client << ":" << (int)a.port << endl;
  }
}

unsigned tAlsaDeviceList::add(const char* pName, const snd_seq_addr_t& a)
{
  mDeviceNames.push_back(pName);
  addr.push_back(a);
  return addr.size();
}

snd_seq_addr_t& tAlsaDeviceList::operator[](unsigned i)
{
  if (i >= addr.size())
  {
    return addr[0];
  }
  return addr[i];
}
