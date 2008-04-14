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

#ifndef JZ_AUDIO_H
#define JZ_AUDIO_H

#include "DynamicArray.h"
#include "Project.h"

class tEventArray;
class JZTrack;
struct tAudioBuffer;
class tAudioBufferQueue;
class tAudioRecordBuffer;
class tSampleWin;
class tSample;
class tSampleVoice;

// these should be variables and queried from the driver!
//
// there is still a bug somewhere:
//   FRAGBITS 13
//   BUFCOUNT 64
//   midi-speed 114 (trackwin)
// does not work, sounds like it skips a buffer after 18 bars
//
// 1MB of buffer data seems to be reasonable.


#ifdef __WXMSW__

// mswin has big buffers, good!

#define FRAGBITS   14
#define FRAGBYTES  (1 << FRAGBITS)  // # bytes
#define FRAGSHORTS (FRAGBYTES/2)    // # shorts
#define BUFSHORTS  FRAGSHORTS
#define BUFBYTES   FRAGBYTES
#define BUFCOUNT   64               // # buffers

#else

// linux only has 64K buffers and wastes one fragment. So keep
// fragments small

#define FRAGBITS   13
#define FRAGBYTES  (1 << FRAGBITS)  // # bytes
#define FRAGSHORTS (FRAGBYTES/2)    // # shorts
#define BUFSHORTS  FRAGSHORTS
#define BUFBYTES   FRAGBYTES
#define BUFCOUNT   128               // # buffers

#endif


// =============================================================
//                            AudioBuffers
// =============================================================

#ifndef __WXMSW__
#define WAVEHDR char
#endif

struct tAudioBuffer
{
  // This is a Microsoft Windows for mswin wavehdr
  WAVEHDR* hdr;
  short* data;

  tAudioBuffer(int dummy)
    : hdr(0),
      data(0)
  {
    data = new short [BUFSHORTS];
    // in case recording stops inside a buffer
    memset(data, 0, BUFBYTES);
  }

  ~tAudioBuffer()
  {
    delete hdr;
    delete [] data;
  }

  void Clear()
  {
    memset(data, 0, BUFBYTES);
  }

  short* Data()
  {
    return data;
  }
};


DECLARE_ARRAY(tAudioBufferArray, tAudioBuffer*)


class tAudioBufferQueue
{
  public:

    tAudioBufferQueue()
    {
      Clear();
    }

    ~tAudioBufferQueue()
    {
    }

    void Clear()
    {
      written = read = 0;
      for (int i = 0; i < BUFCOUNT; i++)
      {
        array[i] = 0;
      }
    }

    int Count() const
    {
      return written - read;
    }

    int Empty() const
    {
      return written == read;
    }

    void Put(tAudioBuffer *buf)
    {
      array[written++ % BUFCOUNT] = buf;
    }

    tAudioBuffer* Get()
    {
      if (written == read)
      {
        return 0;
      }
      return(array[read++ % BUFCOUNT]);
    }

    void UnGet(tAudioBuffer* buf)
    {
      array[ --read % BUFCOUNT ] = buf;
    }

  private:

    tAudioBuffer* array[BUFCOUNT];

    int read, written;
};



class tAudioRecordBuffer
{
  friend class tSampleSet;
  friend class JZWindowsAudioPlayer;

  public:

    tAudioRecordBuffer()
    {
      num_buffers = 0;
    }

    ~tAudioRecordBuffer()
    {
      Clear();
    }

    void Clear();
    tAudioBuffer * RequestBuffer();
    void UndoRequest()
    {
      --num_buffers;
    }
    void ResetBufferSize(int size)
    {
      bufbytes = size;
    }

  private:

    tAudioBufferArray buffers;
    int num_buffers;
    int bufbytes;
};


// =============================================================
//                            tSampleSet
// =============================================================

class tSamplesDlg;

class tSampleSet
{
  private:

    friend class tSampleVoice;
    friend class tSample;
    friend class tAudioGloblForm;
    friend class tSamplesDlg;
    friend class JZWindowsAudioPlayer;
    friend class tAudioPlayer;
    friend class tAlsaAudioPlayer;

  public:

    tSampleSet(long ticks_per_minute);

    virtual ~tSampleSet();

    int Load(const wxString& FileName);

    // load jazz.spl
    void LoadDefaultSettings();

    int Save(const wxString& FileName);

    void ReloadSamples();

    void Edit(int key);

    int GetSpeed() const
    {
      return speed;
    }

    void SetSpeed(int x)
    {
      dirty |= (speed != x);
      speed = x;
    }

    int GetChannels() const
    {
      return channels;
    }

    void SetChannels(int x)
    {
      dirty |= (channels != x);
      channels = x;
    }

    int BitsPerSample() const
    {
      return bits;
    }

    bool GetSoftSync() const
    {
      return softsync;
    }

    void SetSoftSync(bool x)
    {
      softsync = x;
    }

    int ResetBuffers(tEventArray *, long start_clock, long ticks_per_minute);

    int ResetBufferSize(unsigned int bytes);

    int FillBuffers(long last_clock);

    tAudioBuffer *GetBuffer(int i) const
    {
      // 0 < i < BUFCOUNT
      return buffers[i];
    }

    void AdjustAudioLength(JZTrack *t, long ticks_per_minute);

    long Ticks2Samples(long ticks) const
    {
      long spl =
        (long)(60.0 * ticks * speed * channels / (double)ticks_per_minute);

      // Align to the first channel.
      return spl & -channels;
    }

    double Samples2Ticks(long samples) const
    {
      return (double)samples * ticks_per_minute / 60.0 / speed / channels;
    }

    // time in millisec
    long Ticks2Time(long ticks) const
    {
      return (long)(60000.0 * ticks / ticks_per_minute);
    }

    long Time2Ticks(long time) const
    {
      return (long)((double)time * ticks_per_minute / 60000.0);
    }

    long Samples2Time(long samples) const
    {
      return (long)(1000.0 * samples / speed / channels);
    }

    long Time2Samples(long time) const
    {
      return (long)(0.001 * time * speed * channels);
    }

    virtual const char *GetSampleName(int i);

    int OnMenuCommand(int id);

    void StartPlay(long clock);

    void StopPlay();

    // returns number of buffers prepared. Output starts at offs.
    int PrepareListen(int key, long fr_smpl = -1, long to_smpl = -1);

    int PrepareListen(tSample *spl, long fr_smpl = -1, long to_smpl = -1);

    int ContinueListen(); // return number of buffers

    void SaveRecordingDlg(long frc, long toc, tAudioRecordBuffer &buf);

    void SaveWave(
      const char *fname,
      long frc,
      long toc,
      tAudioRecordBuffer &buf);

    void AddNote(const char *fname, long frc, long toc);

    void RefreshDialogs();

    tSample &operator[](int i)
    {
      return *samples[i];
    }

  protected:

    long SampleSize(long num_samples)
    {
      return channels * (bits == 8 ? 1L : 2L) * num_samples;
    }

    long speed;    // samples / second
    int  channels; // mono = 1, stereo = 2
    int  bits;     // must be 16!
    bool softsync;      // enable software midi/audio sync
    enum { MAXSMPL = 128 };
    tSample *samples[MAXSMPL];
    tSampleWin *samplewin[MAXSMPL];

    long    ticks_per_minute;  // midi speed for audio/midi sync
    double  clocks_per_buffer;
    long    start_clock;       // when did play start

    int   event_index;


    unsigned int bufbytes;           // buffer size in byte
    unsigned int bufshorts;          // buffer size in short
    tAudioBuffer *buffers[BUFCOUNT]; // all the audio buffers
    tAudioBufferQueue free_buffers;  // to be filled with data
    tAudioBufferQueue full_buffers;  // to be played by driver
    tAudioBufferQueue driv_buffers;  // actually played by driver

    // return the start clock for i-th free buffer
    long buffers_written;            // for computing buffers clock
    long BufferClock(int i) const
    {
      return (long)(start_clock + i * clocks_per_buffer);
    }

    void GlobalSettingsDlg();
    void SamplesDlg();
    wxDialog *glb_dialog;
    tSamplesDlg *spl_dialog;

    tEventArray *events;

    enum { MAXPOLY = 100 };
    tSampleVoice * voices[MAXPOLY];
    int num_voices;
    int adjust_audio_length;

    wxString mDefaultFileName;
    wxString mRecordFileName;
    bool has_changed;
    int  is_playing;

    int dirty;  // needs reloading

    // to communicate between PrepareListen and ContinueListen
    tSample* listen_sample;
};

#endif // !defined(JZ_AUDIO_H)
