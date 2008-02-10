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

#ifndef JZ_WINDOWSMIDIINTERFACE_H
#define JZ_WINDOWSMIDIINTERFACE_H

#include "DynamicArray.h"

struct tWinPlayerState;

extern "C"
{
// in win-3.x this has to be in a dll!
void FAR PASCAL midiIntInputHandler(HMIDIIN, WORD, DWORD, DWORD, DWORD);
void FAR PASCAL midiMidiInputHandler(HMIDIIN, WORD, DWORD, DWORD, DWORD);
void FAR PASCAL midiMtcInputHandler(HMIDIIN, WORD, DWORD, DWORD, DWORD);
void FAR PASCAL midiIntTimerHandler(UINT, UINT, DWORD, DWORD, DWORD);
void FAR PASCAL midiMidiTimerHandler(UINT, UINT, DWORD, DWORD, DWORD);
void FAR PASCAL midiMtcTimerHandler(UINT, UINT, DWORD, DWORD, DWORD);
tWinPlayerState FAR * FAR PASCAL NewWinPlayerState();
void FAR PASCAL DeleteWinPlayerState(tWinPlayerState FAR * state);

void CALLBACK MidiOutProc(
  HMIDIOUT hmo,
  UINT wMsg,
  DWORD dwInstance,
  DWORD dwParam1,
  DWORD dwParam2
);

}

class tWinAudioPlayer;

// pseudo data word of struct midi_event, values must be less
// than 128 to be distinguished from a midi status
#define START_AUDIO 1
#define SYSEX_EVENT 2



class tWinSysexBuffer
{
    friend class tWinSysexBufferArray;

  public:

    tWinSysexBuffer(tWinSysexBufferArray *array)
      : parent(array)
    {
      maxsize = 0;
      size = 0;
      prepared = 0;
      next_free = 0;
      data = 0;
      memset(&hdr, 0, sizeof(hdr));
    }

    MIDIHDR *MidiHdr()
    {
      return &hdr;
    }

    int IsPrepared() const
    {
      return prepared;
    }

    // sysex and length NOT including 0xf0 and 0xf7
    void PrepareOut(HMIDIOUT hmo, const unsigned char *sysex, int length)
    {
      if (prepared)
      {
        UnprepareOut(hmo);
      }

      if (length + 2 >= maxsize)
      {
        maxsize = length + 20;
        delete [] data;
        data = new unsigned char[maxsize];
      }

      data[0] = (char)0xf0;
      memcpy(data+1, sysex, length);
      data[length+1] = (char)0xf7;
      size = length + 2;

      memset(&hdr, 0, sizeof(hdr));
      hdr.dwUser = (DWORD)this;
      hdr.lpData = (char *)data;
      hdr.dwBufferLength = size;
      OutputDebugString("prepare\n");
      midiOutPrepareHeader(hmo, &hdr, sizeof(hdr));
      prepared = 1;
    }

    void UnprepareOut(HMIDIOUT hmo)
    {
      if (prepared)
      {
        OutputDebugString("unprepare\n");
        midiOutUnprepareHeader(hmo, &hdr, sizeof(hdr));
        size = 0;
        prepared = 0;
      }
    }

    void Release();

/**
    void PrepareInp(HMIDIIN hmi)
    {
      memset(&hdr, 0, sizeof(hdr));
      hdr.dwUser = (DWORD)this;
      hdr.lpData = (char *)data;
      hdr.dwBufferLength = maxsize;
      midiInPrepareHeader(hmi, &hdr, sizeof(hdr));
      prepared = 1;
    }
    void UnprepareInp(HMIDIIN hmi)
    {
      midiInUnprepareHeader(hmi, &hdr, sizeof(hdr));
      prepared = 0;
    }
**/

  protected:
    MIDIHDR hdr;
    unsigned char *data;
    int size;
    int maxsize;
    int prepared;
    tWinSysexBuffer *next_free;
    tWinSysexBufferArray *parent;
};



class tWinSysexBufferArray
{
  public:

    tWinSysexBufferArray()
    {
      size = 0;
      next_free = 0;
    }

    ~tWinSysexBufferArray()
    {
      int n = Size();
      for (int i = 0; i < n; i++)
      {
        delete (tWinSysexBuffer *)array[i];
      }
    }

    int Size() const
    {
      return size;
    }

    tWinSysexBuffer * At(int i) const
    {
      return (tWinSysexBuffer *)array[i];
    }

    tWinSysexBuffer * AllocBuffer()
    {
      if (next_free == 0)
      {
        tWinSysexBuffer *buf = new tWinSysexBuffer(this);
        array[size++] = (void *)buf;
        return buf;
      }
      tWinSysexBuffer *buf = next_free;
      next_free = buf->next_free;
      return buf;
    }

    void ReleaseBuffer(tWinSysexBuffer *buf)
    {
      buf->next_free = next_free;
      next_free = buf;
    }

    void ReleaseAllBuffers()
    {
      next_free = 0;
      int n = Size();
      for (int i = 0; i < n; i++)
      {
        tWinSysexBuffer *buf = At(i);
        buf->next_free = next_free;
        next_free = buf;
      }
    }

  private:

    JZVoidPtrArray array;
    int size;
    tWinSysexBuffer *next_free;
};


inline
void tWinSysexBuffer::Release()
{
  parent->ReleaseBuffer(this);
}

// # events in record/play queue
#define MIDI_BUFFER_SIZE 4096


struct midi_event
{
  DWORD ref;  // Means time or clock depending on sync mode.
  DWORD data; // midi event or pseudo data.
};


class tMidiQueue
{
  public:

    midi_event * get()
    {
      if (wr == rd)
        return 0;
      struct midi_event *e = &buffer[rd];
      rd = (rd + 1) % MIDI_BUFFER_SIZE;
      return e;
    }

    midi_event * peek()
    {
      if (wr == rd)
        return 0;
      return &buffer[rd];
    }

    void put(DWORD data, DWORD ref)
    {
      if (nfree() < 1)
        return;
      midi_event *e = &buffer[wr];
      e->data = data;
      e->ref = ref;
      wr = (wr + 1) % MIDI_BUFFER_SIZE;
    }

    int empty() const {
      return rd == wr;
    }

    int nfree() const {
      return (rd - wr - 1 + MIDI_BUFFER_SIZE) % MIDI_BUFFER_SIZE;
    }


    void clear()
    {
      rd = wr = 0;
    }

    tMidiQueue()
    {
      clear();
    }

  private:
    int rd, wr;
    midi_event buffer[MIDI_BUFFER_SIZE];
};

#define WIN_SYNC_INTERNAL 0
#define WIN_SYNC_SONGPTR  1
#define WIN_SYNC_MTC      2

#define WIN_MTC_TYPE_24    0
#define WIN_MTC_TYPE_25    1
#define WIN_MTC_TYPE_30DF  2
#define WIN_MTC_TYPE_30NDF 3


struct tWinPlayerMtcTime
{
  DWORD hour;
  DWORD min;
  DWORD sec;
  DWORD fm;
  DWORD type;
};

struct tWinPlayerState
{
  HANDLE hmem;
  HMIDIIN hinp;
  HMIDIOUT hout;

  long start_time;
  long play_time;
  long start_clock;
  long ticks_per_minute;

  long play_clock;
  long virtual_clock;
  long ticks_per_signal;
  long signal_time;
  long time_per_tick;

  tWinPlayerMtcTime mtc_start;
  DWORD mtc_frames;
  BOOL mtc_valid;
  DWORD last_qfm;
  DWORD qfm_bits;
  DWORD time_per_frame;

  UINT min_timer_period;
  UINT max_timer_period;
  BOOL playing;
  BOOL soft_thru;
  BOOL doing_mtc_rec;

  tMidiQueue recd_buffer;
  tMidiQueue play_buffer;
  tMidiQueue thru_buffer;

  tWinAudioPlayer* audio_player;
  long       time_correction;

  tWinSysexBufferArray* isx_buffers;
  tWinSysexBufferArray* osx_buffers;
  int sysex_found;
};


#endif // !defined(JZ_WINDOWSMIDIINTERFACE_H)
