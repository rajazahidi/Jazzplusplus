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

#ifndef JZ_ALSAPLAY_H
#define JZ_ALSAPLAY_H

#include "Player.h"
#include "AlsaThru.h"

#include <alsa/asoundlib.h>

class tAlsaDeviceList : public tDeviceList
{
  public:
    unsigned add(const char* pName, const snd_seq_addr_t& a);
    snd_seq_addr_t& operator[](unsigned i);
    void print(const char *msg);
  private:
    std::vector<snd_seq_addr_t> addr;
};



class tAlsaPlayer : public tPlayer
{
  friend class tAlsaThru;
  public:
    tAlsaPlayer(JZSong *song);
    virtual ~tAlsaPlayer();

    void Notify();
    int Installed();
    int  OutEvent(JZEvent *e, int now);
    int  OutEvent(JZEvent *e) { return OutEvent(e, 0); }
    void OutNow(JZEvent *e)   { OutEvent(e, 1); }
    void OutNow(tParam *r);
    void OutBreak();
    void OutBreak(long BreakOver);
    void StartPlay(long Clock, long LoopClock = 0, int Continue = 0);
    void StopPlay();
    long GetRealTimeClock();
    virtual void SetSoftThru(int on, int idev, int odev);
    virtual int SupportsMultipleDevices() { return 1; }
    virtual tDeviceList & GetOutputDevices() { return oaddr; }
    virtual tDeviceList & GetInputDevices() { return iaddr; }
    virtual int GetThruInputDevice() { return ithru; }
    virtual int GetThruOutputDevice() { return othru; }
    int FindMidiDevice();

  protected:
    snd_seq_t *handle;
    tAlsaDeviceList iaddr;        // addresses of input devices
    tAlsaDeviceList oaddr;        // addresses of output devices
    int client;            // me
    snd_seq_addr_t self;   // my address
    int queue;             // queue
    int inp_dev, outp_dev; // input/output device index
    int sync_in, sync_in_dev, sync_in_mtcType;
    int sync_out, sync_out_dev, sync_out_mtcType;
 
    int installed;

    static int create_port(snd_seq_t *handle, const char *name);
    static void set_client_info(snd_seq_t *handle, const char *name);
    void subscribe_inp(int dev);
    void unsubscribe_inp(int dev);
    void subscribe_out(int dev);
    void unsubscribe_out(int dev);
    void thru_connect();
    void thru_disconnect();
    void scan_clients(tAlsaDeviceList &list, int device_caps);
    int select_list(tAlsaDeviceList &list, char *title, int def_device);
    int  start_timer(long clock);
    int write(snd_seq_event_t *ev) { return write(ev, 0); } // 0 == ok
    int write(snd_seq_event_t *ev, int now); // 0 == ok
    void set_event_header(snd_seq_event_t *ev, long clock, int type);
    void set_event_header(snd_seq_event_t *ev, long clock, int len, void *ptr);
    void init_queue_tempo(int time_base, int bpm);
    void start_queue_timer(long clock);
    void stop_queue_timer();
    void recd_event(snd_seq_event_t *ev);
    void flush_output();
    int  set_blocking_mode(int enable);
    void clear_input_queue();
    void purge_queues();
    void set_pool_sizes();
    int compose_echo(int clock, unsigned int arg = 0);
    virtual void StartAudio();
    virtual void ResetPlay(long clock);
    int sync_master();
    void sync_master_remove();
    int sync_slave();
    void sync_slave_remove();

    long play_clock;   // current clock
    long recd_clock;  // clock received so far from recorded events or echo events
    long echo_clock;  // echo events have been sent up to this clock

    tAlsaThru *thru;
    int ithru, othru;  // index in iaddr, oaddr of source/target device
};


#endif // !defined(JZ_ALSAPLAY_H)
