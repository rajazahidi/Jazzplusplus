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

#ifndef JZ_SAMPLE_H
#define JZ_SAMPLE_H

class tSampleSet;

#include <iostream>
#include <cmath>


#include <assert.h>
#include <sys/stat.h>
#include <string.h>

#ifdef wx_x
#include <time.h>
#endif

// #include "jazzlib.h"
#include "SignalInterface.h"

// msvc 5.0 has a buggy optimizer!! it cannot compute the following:
// pow(2.0, 1.0 / 12.0)
#define FSEMI    1.059463094
// log ( pow(2.0, 1.0 / 12.0) )
#define LOGFSEMI 0.057762264

// --------------------------------------------------------
// --------------- stolen from vplay ----------------------
// --------------------------------------------------------

/* Definitions for Microsoft WAVE format */

#define RIFF		0x46464952
#define WAVE		0x45564157
#define FMT		0x20746D66
#define DATA		0x61746164
#define PCM_CODE	1
#define WAVE_MONO	1
#define WAVE_STEREO	2

/* it's in chunks like .voc and AMIGA iff, but my source say there
   are in only in this combination, so I combined them in one header;
   it works on all WAVE-file I have
*/
typedef unsigned short u_short;
typedef unsigned long u_long;

// 'old' format for writing .wav files
typedef struct _waveheader {
  u_long	main_chunk;	/* 'RIFF' */
  u_long	length;		/* filelen */
  u_long	chunk_type;	/* 'WAVE' */

  u_long	sub_chunk;	/* 'fmt ' */
  u_long	sc_len;		/* length of sub_chunk, =16 */
  u_short	format;		/* should be 1 for PCM-code */
  u_short	modus;		/* 1 Mono, 2 Stereo */
  u_long	sample_fq;	/* frequence of sample */
  u_long	byte_p_sec;
  u_short	byte_p_spl;	/* samplesize; 1 or 2 bytes */
  u_short	bit_p_spl;	/* 8, 12 or 16 bit */

  u_long	data_chunk;	/* 'data' */
  u_long	data_length;	/* # sample bytes */
} WaveHeader;






/* ----------------------------------------------------------

There are two different kinds of samples, short and float.
The class hierarchy is a little buggy, they should have been
derived from a common base. Most of the functionality is in
the short version. Some functionality is duplicated in
both classes.
---------------------------------------------------------- */

/**
 * floating point representation of a sample. This simplifies
 * algorithms because there is no need to take care of overruns.
 *
 * used for the 'big' CMIX interface too.
 */

class tFloatSample // : public tCMIX
{
  friend class tSample;
  public:
    tFloatSample(tSample &spl);
    tFloatSample(tSample &spl, long fr, long to);
    tFloatSample(int ch, long sr);
    virtual ~tFloatSample();
    float Peak(long fr = -1, long to = -1);
    void Rescale(float maxval = 32766.0, long fr = -1, long to = -1);
    void RescaleToShort(long fr = -1, long to = -1);
    float &operator[](long i) { return data[i]; }
    void Initialize(long size = 0);
    void PasteMix(tFloatSample &src, long offs = 0);
    void PasteMix(tSample &src, long offs = 0);
    void RemoveTrailingSilence(float peak = 50);

    // CMIX Interface functions

    virtual long SetNote(float offs, float dur);
    virtual void EndNote();
    virtual int AddOut(float *p);
    virtual int GetIn(float *p);
    int GetSample(float i, float *p);
    long Seconds2Samples(float time);
    float Samples2Seconds(long samples);
    void InsertSilence(long pos, long length);
    void Convert2Mono();
    void ClipToCurrent();

    // CMIX wavetables (gen routines)
    void Normalize();  // make values in 0..1
    void HanningWindow(long size);

    // Effects

    void Echo(int num_echos, long delay, float ampl);
    void RndEcho(int num_echos, long delay, float ampl);
    void RndEchoStereo(int num_echos, long delay, float ampl);

    /**
     * see args of tSplFilter::Setup() for this.
     */
    void Filter(long fr, long to, tSplFilter::Type type, int order, double freq, double bw);

    /**
     * signal template classes interface
     */

    void AssureLength(long new_length);

    int GetChannels() const {
      return channels;
    }

    long GetSamplingRate() const {
      return sampling_rate;
    }

    float *GetData() {
      return data;
    }
    long GetLength() const {
      return length;
    }

  protected:
    float *data;
    long  length;
    int   channels;
    long  sampling_rate;

    // CMIX-IO position
    long  current;
};


// ----------------------------------------------------------

/**
 * contains data for one sample. For fastest playback access
 * samples are stored as signed shorts.
 *   data[0]   == 1st value for left channel
 *   data[1]   == 1st value for right channel
 *   data[n]   == 1st value for n-th channel
 *   data[n+1] == 2nd value for left channel
 *   ...
 * all length values mean number of shorts and
 * should be multiples of of set.GetChannels().
 * Offsets should start on channel boundaries,
 * that is offs % set.GetChannels() == 0.
 */

class tSample {
  friend class tFloatSample;
  friend class tSplPan;
  friend class tSplPitch;
  public:
    friend class tSampleSet;
    friend class tSampleVoice;
    tSample(tSampleSet &s);
    virtual ~tSample();

    int Load(int force = 0);
    int LoadWav();
    int LoadRaw();

    int SaveWave();
    int Save();

    // Properties
    void SetLabel(const char *str);
    const char *GetLabel() const {
      return label;
    }

    void SetVolume(int vol) {
      dirty |= (vol != volume);
      volume = vol;
    }

    int GetVolume() const {
      return volume;
    }

    void SetPan(int p) {
      dirty |= (p != pan);
      pan = p;
    }

    int GetPan() const {
      return pan;
    }

    void SetPitch(int p) {
      dirty |= (p != pitch);
      pitch = p;
    }

    int GetPitch() const {
      return pitch;
    }

    void SetFilename(const char *fname);
    const char *GetFilename() const {
      return filename;
    }

    long GetLength() const {
      return length;
    }

    int IsEmpty() const {
      return length == 0;
    }
    void SetExternal(int ext) {
      external_flag = ext;
      //external_time = 0;
    }
    int GetExternal() const {
      return external_flag;
    }

    void Clear();

    void GotoRAM() {
      // try to swap this sample into memory ...
      volatile short dummy;
      for (long i = 0; i < length; i++)
        dummy = data[i];
    }

    short *GetData() {
      return data;
    }


    /**
     * access global adustments from tSampleSet
     */

    tSampleSet &SampleSet() {
      return set;
    }

    tSampleSet *operator->() {
      return &set;
    }

    int GetChannels() const;
    long GetSamplingRate() const;

    /**
     * align offset to channel boundary
     */
    long Align(long offs) const;

    /**
     * copy part of the data into another tSample o. If o
     * contains other data these will be erased.
     */

    void Copy(tSample &dst, long fr_smpl = -1, long to_smpl = -1);

    /**
     * like Copy but deletes the source selection afterwards.
     */

    void Cut(tSample &dst, long fr_smpl = -1, long to_smpl = -1);

    /**
     * delete part of this sample.
     */
    void Delete(long fr_smpl = -1, long to_smpl = -1);

    /**
     * paste some data into this sample, data are inserted
     */
    void PasteIns(tSample &src, long offs);

    /**
     * paste some data into this sample, data are mixed with
     * the current contents.
     */
    void PasteMix(tSample &src, long offs);
    void PasteOvr(tSample &src, long fr, long to);
    void ReplaceSilence(long offs, long len);
    void Rescale(short maxval = 32766);
    void Reverse(long fr, long to);
    long Peak();

    /**
     * flip phase of left/right channel
     */
     void Flip(int ch);


    void Transpose(float freq_factor);
    void TransposeSemis(float semis);

    /**
     * inserts some zero values at pos
     */

    void InsertSilence(long pos, long length);

    /**
     * initialize length and data from the float sample
     */

    void Set(tFloatSample &fs);

    /**
     * replace part of the data with the data in fs,
     * original data are overwritten.
     */

    void Set(tFloatSample &fs, long offs);

    /**
     * like Set() but try to make a smooth transition
     */
    void SetSmooth(tFloatSample &fs, long offs, long fade_len = -1);

    long Seconds2Samples(float time);
    float Samples2Seconds(long samples);

    void AssureLength(long len);

  protected:

    void FreeData();
    void MakeData(long length, int zero = 1);

    int Convert(std::istream& is, long byte_count, int channels, int bits, long speed);
    long length;  // number of shorts
    short *data;  // signed shorts
    tSampleSet &set;

    char  *label;   // msvc cannot delete 'const char *' ????
    char  *filename;
    int    volume;
    int    pan;
    int    pitch;  // delta pitch

#ifdef HAVE_IOS_OPENMODE
    std::_Ios_Openmode openread;
#else
    int openread;
#endif

    int    dirty;

    int    external_flag;  // reload on disk change?
    long external_time;  // last modified on disk

};

#endif // !defined(JZ_SAMPLE_H)
