//*****************************************************************************
// The JAZZ++ Midi Sequencer
//
// Copyright (C) 1994-2000 Andreas Voss and Per Sigmond, all rights reserved.
// Modifications Copyright (C) 2004 Patrick Earl
// Modifications Copyright (C) 2008-2010 Peter J. Stieber
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

#ifndef JZ_SIGNAL2_H
#define JZ_SIGNAL2_H

#include "Sample.h"
#include "Random.h"
#include "SignalInterface.h"

#define PI 3.14159265358979323846

#include <cmath>
#include <vector>
#include <algorithm>

const float log001 = -6.9078f;       // log(.001)

// ---------------------------------------------------------------------
//                                 utils
// ---------------------------------------------------------------------
template <class T>
class tLineMap
{
  public:
    /**
     * construct a mapper, that maps the range [x0,x1] to [y0,y1].
     */
    tLineMap(T x0, T x1, T y0, T y1)
    {
      Initialize(x0, x1, y0, y1);
    }
    tLineMap()
    {
      x0 = 0;
      y0 = 0;
      a = 0;
    }
    void Initialize (T xx0, T xx1, T yy0, T yy1)
    {
      x0 = xx0;
      y0 = yy0;
      a  = (yy1 - yy0) / (xx1 - xx0);
    }

    T operator()(T x) const
    {
      return y0 + (x - x0) * a;
    }

  protected:

    T x0, y0, a;
};


/**
 * maps the range -x .. +x to the range 1/y ... y using the exp()
 * function (i.e. map(0) == 1).
 */

template <class T>
class tExpoMap
{
  public:
    tExpoMap(T x, T y) : map(-x, x, -log(y), log(y))
    {
    }
    T operator()(T x)
    {
      return exp(map(x));
    }
  private:
    tLineMap<T> map;
};


// -----------------------------------------------------------------------
//                            sample
// -----------------------------------------------------------------------

/**
 * describes a sample
 */

class tSigValue
{
  public:

    enum
    {
      MAXCHN = 2
    };

    const float & operator[](int i) const
    {
      return val[i];
    }
    float & operator[](int i)
    {
      return val[i];
    }
    tSigValue()
    {
      for (int i = 0; i < MAXCHN; i++)
        val[i] = 0;
    }
    void operator += (float f)
    {
      for (int i = 0; i < MAXCHN; i++)
        val[i] += f;
    }
    void operator -= (float f)
    {
      for (int i = 0; i < MAXCHN; i++)
        val[i] -= f;
    }
    void operator *= (float f)
    {
      for (int i = 0; i < MAXCHN; i++)
        val[i] *= f;
    }
    void operator += (const tSigValue &f)
    {
      for (int i = 0; i < MAXCHN; i++)
        val[i] += f[i];
    }
    void operator -= (const tSigValue &f)
    {
      for (int i = 0; i < MAXCHN; i++)
        val[i] -= f[i];
    }
    void operator *= (const tSigValue &f)
    {
      for (int i = 0; i < MAXCHN; i++)
        val[i] *= f[i];
    }
    tSigValue operator +(float f) const
    {
      tSigValue tmp(*this);
      tmp += f;
      return tmp;
    }
    tSigValue operator -(float f) const
    {
      tSigValue tmp(*this);
      tmp -= f;
      return tmp;
    }
    tSigValue operator *(float f) const
    {
      tSigValue tmp(*this);
      tmp *= f;
      return tmp;
    }
    tSigValue operator +(const tSigValue &f) const
    {
      tSigValue tmp(*this);
      tmp += f;
      return tmp;
    }
    tSigValue operator -(const tSigValue &f) const
    {
      tSigValue tmp(*this);
      tmp -= f;
      return tmp;
    }
    tSigValue operator *(const tSigValue &f) const
    {
      tSigValue tmp(*this);
      tmp *= f;
      return tmp;
    }
    tSigValue &operator=(float x)
    {
      for (int i = 0; i < MAXCHN; i++)
        val[i] = x;
      return *this;
    }

  private:
    float val[MAXCHN];
};


/**
 * an array of samples
 */

class tSigValArray
{
  public:
    tSigValArray(long length, int channels)
    {
      this->size     = length;
      this->channels = channels;
      array = new tSigValue [length];
    }
    virtual ~tSigValArray()
    {
      delete [] array;
    }
    tSigValArray(const tSigValArray &o)
    {
      size = o.size;
      channels = o.channels;
      array = new tSigValue[size];
      for (int i = 0; i < size; i++)
        array[i] = o.array[i];
    }
    tSigValArray & operator = (tSigValArray &o)
    {
      if (&o == this)
        return *this;
      delete [] array;
      size = o.size;
      channels = o.channels;
      array = new tSigValue[size];
      for (int i = 0; i < size; i++)
        array[i] = o.array[i];
      return *this;
    }

    tSigValue & operator[](int i)
    {
      return array[i % size];
    }
    const tSigValue & operator[](int i) const
    {
      return array[i % size];
    }
    void Interpolate(tSigValue &val, float x) const
    {
      long  ofs = (long)x;
      if (ofs >= size-1)
      {
        val = array[size-1];
        return;
      }
      float rem = x - ofs;
      tSigValue &v1 = array[ofs];
      tSigValue &v2 = array[ofs+1];
      for (int i = 0; i < channels; i++)
      {
        tLineMap<float> map(0, 1, v1[i], v2[i]);
        val[i] = map(rem);
      }
    }
    void CyclicInterpolate(tSigValue &val, float x) const
    {
      long  ofs = (long)x;
      float rem = x - ofs;
      tSigValue &v1 = array[ofs % size];
      tSigValue &v2 = array[(ofs+1) % size];
      for (int i = 0; i < channels; i++)
      {
        tLineMap<float> map(0, 1, v1[i], v2[i]);
        val[i] = map(rem);
      }
    }
    long Size() const
    {
      return size;
    }

  private:
    tSigValue *array;
    int channels;
    long size;
};


// -----------------------------------------------------------------------
//                                parent
// -----------------------------------------------------------------------

class tSigInput;
class tSigOutput;

class tSigSynth
{
  friend class tSigInput;
  public:

    tSigSynth(long sr, int ch)
      : generators(),
        channels(ch),
        sampling_rate(sr),
        current(0)
    {
    }

    void AddGenerator(tSigInput &gen)
    {
      generators.push_back(&gen);
    }

    int GetChannelCount() const
    {
      return channels;
    }

    long GetSamplingRate() const
    {
      return sampling_rate;
    }

    long GetCurrent() const
    {
      return current;
    }
    void Run(tSigOutput &osig, tSigInput &isig, float add_seconds);

    void DeleteAllGenerators();

  protected:

    std::vector<tSigInput *> generators;
    int channels;
    long sampling_rate;
    long current;
};


class tSigInput
{

  public:

    tSigInput(tSigSynth &parent) : synth(parent)
    {
      synth.AddGenerator(*this);
      channels = synth.GetChannelCount();
      sampling_rate = synth.GetSamplingRate();
      current = -1;
    }

    virtual ~tSigInput()
    {
    }

    void GetSample(tSigValue &ret)
    {
      if (synth.current >= current)
      {
        current = synth.current + 1;
        NextValue();
      }
      ret = val;
    }

    float GetControl()
    {
      if (synth.current >= current)
      {
        current = synth.current + 100;
        NextValue();
        ctl = 0;
        for (int i = 0; i < channels; i++)
          ctl += val[i];
        ctl = ctl / channels;
      }
      return ctl;
    }

    int HasChanged() const
    {
      return synth.current >= current;
    }

    virtual void Init()
    {
      // called once before performance starts
      current = -1;
    }

    virtual long GetLength()
    {
      return 0;
    }
    virtual void NextValue() = 0;

  protected:
    tSigSynth &synth;
    tSigValue val;
    float     ctl;
    tSigValue nul;  // empty signal value
    long current;
    long channels;
    long sampling_rate;
};



class tSigOutput : public tSigInput
{
  public:
    tSigOutput(tSigSynth &synth) : tSigInput(synth)
    {
    }
    virtual void Out(const tSigValue &v) = 0;
    virtual void Resize(long)            = 0;
};



/**
 * adaptor class for tSample and tFloatSample
 */

template <class T, class SPL>
class tSampleIterator : public tSigOutput
{
  public:
    tSampleIterator(tSigSynth &synth, SPL &s)
      : tSigOutput(synth),
        spl(s)
    {
      length = spl.GetLength() / channels;
      data   = spl.GetData();
    }

    virtual void Init()
    {
      tSigOutput::Init();
      length = spl.GetLength() / channels;
      data   = spl.GetData();
    }

    virtual void NextValue()
    {
      if (current >= length)
      {
        val = nul;
        return;
      }
      long idata = channels * current;
      for (int i = 0; i < channels; i++)
      {
        val[i] = data[idata++];
      }
    }

    virtual void Out(const tSigValue &val)
    {
      long idata = channels * synth.GetCurrent();
      for (int i = 0; i < channels; i++)
      {
        data[idata++] = (T)val[i];
      }
    };

    virtual void Resize(long len)
    {
      spl.AssureLength(len * channels);
      data   = spl.GetData();
      length = spl.GetLength() / channels;
    }

    virtual long GetLength()
    {
      return length;
    }

  protected:
    SPL &spl;
    T*  data;
    long length;
};

typedef tSampleIterator<short, tSample>      tShortIter;
typedef tSampleIterator<float, tFloatSample> tFloatIter;


template <class T, class SPL>
class tSampleResizingIterator : public tSampleIterator<T, SPL>
{
  public:
    tSampleResizingIterator(tSigSynth &synth, SPL &s)
      : tSampleIterator<T, SPL>(synth, s)
    {
    }

    virtual void Out(const tSigValue &v)
    {
      if (this->GetCurrent() >= this->GetLength())
      {
        Resize(this->GetLength() * 2);
      }
      long idata = this->GetCurrent() * this->GetChannelCount();
      for (int i = 0; i < this->GetChannelCount(); i++)
      {
        this->data[idata++] = (T)v[i];
      }
    }
};

typedef tSampleResizingIterator<short, tSample>      tResizingShortIter;
typedef tSampleResizingIterator<float, tFloatSample> tResizingFloatIter;



class tSignalModifier : public tSigInput
{
  public:
    tSignalModifier(tSigSynth &synth)
      : tSigInput(synth),
        recurse_init(false),
        recurse_length(0)
    {
    }

    void AddInput(tSigInput &sig)
    {
      inputs.push_back(&sig);
    }

    void AddControl(tSigInput &sig)
    {
      controls.push_back(&sig);
    }

    virtual long GetLength()
    {
      if (recurse_length)
      {
        return 0;
      }
      recurse_length = 1;
      long lmax = 0;
      for (unsigned i = 0; i < inputs.size(); ++i)
      {
        lmax = std::max(lmax, inputs[i]->GetLength());
      }
      recurse_length = 0;
      return lmax;
    }

    virtual void Init()
    {
      tSigInput::Init();

      if (recurse_init)
      {
        return;
      }
      recurse_init = true;

      unsigned i;

      for (i = 0; i < inputs.size(); ++i)
      {
        inputs[i]->Init();
      }

      for (i = 0; i < controls.size(); ++i)
      {
        controls[i]->Init();
      }

      recurse_init = false;
    }

  protected:

    std::vector<tSigInput *>inputs;
    std::vector<tSigInput *>controls;
    bool recurse_init;
    int recurse_length;
};


// -----------------------------------------------------------------------
//                        Wavetable Synth
// -----------------------------------------------------------------------

class tSigWaveOscil : public tSignalModifier
{
  // wave table oscillator. Cannot be used for Control Signals!!
  public:
    tSigWaveOscil(tSigSynth &synth, int N, double f, double ffact = FSEMI)
      : tSignalModifier(synth),
        array(N, synth.GetChannelCount()),
        freq(f),
        SR(synth.GetSamplingRate()),
        frqfact(ffact)
    {
      dx = N / SR * freq;
      x  = 0;
      fmap.Initialize(-1, 1, 1.0 / frqfact, frqfact);
      have_freq_control = 0;
    }

    virtual void Init()
    {
      tSignalModifier::Init();
      have_freq_control = (controls.size() != 0);
    }

    tSigValue & operator[](int i)
    {
      return array[i];
    }

    const tSigValue & operator[](int i) const
    {
      return array[i];
    }

    void NextValue()
    {
      if (have_freq_control && controls[0]->HasChanged())
      {
        dx = array.Size() / SR * freq * fmap(controls[0]->GetControl());
      }
      array.CyclicInterpolate(val, x);
      x += dx;
    }

    long Size() const
    {
      return array.Size();
    }

  protected:
    tSigValArray array;
    double freq;
    double SR;
    double x;
    double dx;
    double frqfact;
    int have_freq_control;
    tLineMap<double>fmap;
};


class tSigWaveCtrl : public tSigInput
{
  // control signal from wave table
  public:
    tSigWaveCtrl(tSigSynth &synth, int N, double durat)
      : tSigInput(synth),
        array(N, synth.GetChannelCount()),
        xmap(0, synth.GetSamplingRate() * durat, 0, N)
    {
    }

    tSigValue & operator[](int i)
    {
      return array[i];
    }

    const tSigValue & operator[](int i) const
    {
      return array[i];
    }

    void NextValue()
    {
      array.Interpolate(val, xmap(current));
    }

    long Size() const
    {
      return array.Size();
    }

  protected:
    tSigValArray array;
    tLineMap<float>xmap;
};


class tSigNoise : public tSigInput
{
  // control signal from wave table
  public:
    tSigNoise(tSigSynth &synth)
      : tSigInput(synth)
    {
    }

    void NextValue()
    {
      for (int i = 0; i < channels; i++)
        val[i] = ((rnd.asDouble() * 2.0) - 1.0) * 32000.0;
    }
};

// -----------------------------------------------------------------------
//                               LFO's
// -----------------------------------------------------------------------

class tSigConst : public tSigInput
{
  public:
    tSigConst(tSigSynth &synth, float x)
      : tSigInput(synth)
    {
      for (int i = 0; i < channels; i++)
        val[i] = x;
    }

    virtual void NextValue()
    {
    }
};


/**
 * a sine oscillator.
 */

class tSigSine : public tSigInput
{
  public:
    tSigSine(tSigSynth &synth, double freq, double amp = 1.0, double phi = 0)
      : tSigInput(synth)
    {
      double plen = synth.GetSamplingRate() / freq;
      map.Initialize(0, plen, phi, phi + 2 * PI);
      ampl = amp;
    }
    void NextValue()
    {
      float y = ampl * sin(map((double)current));
      for (int i = 0; i < channels; i++)
        val[i] = y;
    }
  protected:
    tLineMap<double> map;
    double ampl;
};



/**
 * changes volume controlled by a lfo. The lfo values -1..+1
 * will change the volume by factor 0..2
 */

class tSigVolume : public tSignalModifier
{
  public:
    tSigVolume(tSigSynth &synth) : tSignalModifier(synth)
    {
    }
    void NextValue()
    {
      float vol = controls[0]->GetControl();
      inputs[0]->GetSample(val);
      for (int i = 0; i < channels; i++)
        val[i] *= (1.0 + vol);
    }
};


/**
 * modify panpot controlled by a lfo
 */

class tSigPanpot : public tSignalModifier
{
  public:
    tSigPanpot(tSigSynth &synth) : tSignalModifier(synth)
    {
    }
    void NextValue()
    {
      float p = controls[0]->GetControl();
      inputs[0]->GetSample(val);
      if (p > 0)
        val[0] *= (1 - p);
      else
        val[1] *= (1 + p);
    }
};


/**
 * delay a signal for a variable number of samples
 */

class tSigDelay : public tSignalModifier
{
  public:
    tSigDelay(tSigSynth &synth, float time)
      : tSignalModifier(synth),
        size((long)(time * sampling_rate) + 2),
        array(size, synth.GetChannelCount())
    {
      map.Initialize(-1, 1, 0, -size+1);
    }

    void NextValue()
    {
      inputs[0]->GetSample(array[current % size]);
      tSigValue ctl;
      controls[0]->GetSample(ctl);
      float x = size + map(ctl[0]) + current;
      array.CyclicInterpolate(val, x);
    }
  protected:
    long size;
    tSigValArray array;
    tLineMap<float> map;
};


// -----------------------------------------------------------------
//                           filters
// -----------------------------------------------------------------

class tOpFilter
{
  public:
    virtual ~tOpFilter()
    {
    }
    virtual void Setup(float sr, float hp, float dummy) = 0;
    virtual float Loop(float sig) = 0;
};

class tOpLowpass : public tOpFilter
{
  public:
    tOpLowpass()
    {
      y1 = 0;
    }
    virtual ~tOpLowpass()
    {
    }
    virtual void Setup(float sr, float hp, float dummy)
    {
      double b = 2.0 - cos(hp * 2.0 * PI / sr);
      c2 = b - sqrt(b * b - 1.0);
      c1 = 1.0 - c2;
    }
    virtual float Loop(float sig)
    {
      y1 = c1 * sig + c2 * y1;
      return y1;
    }
  protected:
    float c1, c2, y1;
};

class tOpHighpass : public tOpFilter
{
  public:
    tOpHighpass()
    {
      y1 = 0;
    }
    virtual ~tOpHighpass()
    {
    }
    virtual void Setup(float sr, float hp, float dummy)
    {
      double b = 2.0 - cos(hp * 2.0 * PI / sr);
      c2 = b - sqrt(b * b - 1.0);
      c1 = 1.0 - c2;
    }
    virtual float Loop(float sig)
    {
      float tmp = y1 = c2 * (y1 + sig);
      y1 -= sig;
      return tmp;
    }
  protected:
    float c1, c2, y1;
};

class tOpBandpass : public tOpFilter
{
  public:
    tOpBandpass()
    {
      y1 = y2 = 0;
    }
    virtual ~tOpBandpass()
    {
    }
    // bw = Hz = upper - lower half power point
    virtual void Setup(float sr, float cf, float bw)
    {
      double cosf = cos(cf * 2.0 * PI / sr);
      c3 = exp(bw * (-2.0 * PI / sr));
      c2 = (c3 * 4) * cosf / (c3 + 1.);
      c1 = (1.0 - c3) * sqrt(1.0 - c2 * c2 / (c3 * 4));
    }
    virtual float Loop(float sig)
    {
      float out = c1 * sig + c2 * y1 - c3 * y2;
      y2 = y1;
      y1 = out;
      return out;
    }
  protected:
    float c1, c2, c3, y1, y2;
};

class tOpBandstop : public tOpFilter
{
  public:
    tOpBandstop()
    {
      y1 = y2 = 0;
    }
    virtual ~tOpBandstop()
    {
    }
    virtual void Setup(float sr, float cf, float bw)
    {
      double cosf = cos(cf * 2.0 * PI / sr);
      c3 = exp(bw * (-2.0 * PI / sr));
      c2 = (c3 * 4) * cosf / (c3 + 1.);
      c1 = 1.0 - (1.0 - c3) * sqrt(1.0 - c2 * c2 / (c3 * 4));
    }
    virtual float Loop(float sig)
    {
      float out = c1 * sig + c2 * y1 - c3 * y2;
      y2 = y1;
      y1 = out - sig;
      return out;
    }
  protected:
    float c1, c2, c3, y1, y2;
};


template <class FILTER>
class tSigFilter : public tSignalModifier
{
  public:
    // freq_factor : control will change freq from freq/factor ... freq*factor
    tSigFilter(tSigSynth &synth, float freq, float bandw = 0, float freq_factor = 2) :
      tSignalModifier(synth),
      fmap(1, freq_factor)
    {
      sr = (float)sampling_rate;
      for (int i = 0; i < channels; i++)
        filter[i].Setup(sr, freq, bandw);
      have_control = false;
      this->freq = freq;
      this->bandw = bandw;
    }
    virtual void Init()
    {
      tSignalModifier::Init();
      have_control = (controls.size() == 1);
    }
    void NextValue()
    {
      if (have_control && controls[0]->HasChanged())
      {
        float f = freq * fmap(controls[0]->GetControl());
        for (int i = 0; i < channels; i++)
          filter[i].Setup(sr, f, bandw);
      }
      inputs[0]->GetSample(val);
      for (int i = 0; i < channels; i++)
        val[i] = filter[i].FILTER::Loop(val[i]);
    }
    tSigValue operator()(const tSigValue &sig)
    {
      for (int i = 0; i < channels; i++)
        val[i] = filter[i].FILTER::Loop(sig[i]);
      return val;
    }

  protected:
    FILTER filter[tSigValue::MAXCHN];
    bool have_control;
    float freq;
    float sr;
    float bandw;
    tExpoMap<float> fmap;
};



#if 0
class tSigLowpass : public tSignalModifier
// old and probably buggy
{
  public:

    tSigLowpass(tSigSynth &synth, float fg) : tSignalModifier(synth)
    {
      fg = fg / sampling_rate;
      a0 = 2 * PI * fg;
      // b1 = a0 - 1.0; // approx
      b1 = -exp(-2*PI*fg); // exact
    }

    void NextValue()
    {
      tSigValue sig;
      inputs[0]->GetSample(sig);
      val = sig * a0 - val * b1;
    }

    tSigValue operator()(const tSigValue &sig)
    {
      val = sig * a0 - val * b1;
      return val;
    }
  protected:
    float a0, b1;
};
#endif



/**
 * comb reverb ("colored"), csound echo algorithm
 * echo densitiy is controlled by loop_time, attenuation rate is controlled
 * by reverb_time.
 */

class tSigComb : public tSignalModifier
{
  public:
    tSigComb(tSigSynth &synth, float loop_time, float reverb_time)
      : tSignalModifier(synth),
        size((long)(loop_time * synth.GetSamplingRate())),
        array(size+2, synth.GetChannelCount())
    {
      coeff = exp((double)(log001 * loop_time / reverb_time));
    }

    void NextValue()
    {
      val = array[current];
      array[current] *= coeff;
      tSigValue tmp;
      inputs[0]->GetSample(tmp);
      array[current] += tmp;
    }

    tSigValue operator()(const tSigValue &inp)
    {
      current++;
      val = array[current];
      array[current] *= coeff;
      array[current] += inp;
      return val;
    }

  protected:
    long size;
    tSigValArray array;

    float coeff;
};

/**
 * alpass reverb ("flat"), csound echo algorithm
 * echo densitiy is controlled by loop_time, attenuation rate is controlled
 * by reverb_time.
 */

class tSigAlpass : public tSigComb
{
  public:
    tSigAlpass(tSigSynth &synth, float loop_time, float reverb_time)
      : tSigComb(synth, loop_time, reverb_time)
    {
    }

    void NextValue()
    {
      tSigValue y, z, sig;
      inputs[0]->GetSample(sig);
      y = array[current];
      array[current] = z = y * coeff + sig;
      val = y - z * coeff;
    }

    tSigValue operator()(const tSigValue &sig)
    {
      tSigValue y, z;
      current++;
      y = array[current];
      array[current] = z = y * coeff + sig;
      val = y - z * coeff;
      return val;
    }

  protected:
};

/**
 * reverb ("room"), csound reverb algorithm with lowpass filter added
 */

class tSigReverb : public tSignalModifier
{
  public:
    enum
    {
      COMBS = 4,
      ALPAS = 2
    };
    tSigReverb(
      tSigSynth &synth,
      float reverb_time = 0.7,
      float lowpass_freq = 5000,
      float effect_volume = 0.5,
      float loop_fact = 1.0)
      : tSignalModifier(synth),
        lowp(synth, lowpass_freq),
        balance(effect_volume)
    {
      int i;
      for (i = 0; i < COMBS; i++)
        combs[i] = new tSigComb(synth, comb_times[i] * loop_fact, reverb_time);
      for (i = 0; i < ALPAS; i++)
        alpas[i] = new tSigAlpass(synth, alpas_times[i] * loop_fact, reverb_time);
    }
    ~tSigReverb()
    {
      int i;
      for (i = 0; i < COMBS; i++)
        delete combs[i];
      for (i = 0; i < ALPAS; i++)
        delete alpas[i];
    }

    void NextValue()
    {
      int i;
      tSigValue inp;
      inputs[0]->GetSample(inp);
      val = nul;
      for (i = 0; i < COMBS; i++)
        val += (*combs[i])(inp);
      val = lowp(val);
      for (i = 0; i < ALPAS; i++)
        val = (*alpas[i])(val);
      val = val * balance + inp * (1.0 - balance);
    }

  protected:
    tSigComb   *combs[COMBS];
    tSigAlpass *alpas[ALPAS];
    tSigFilter<tOpLowpass> lowp;
    static const float comb_times[COMBS];
    static const float alpas_times[ALPAS];
    float balance;
};

/**
 * spread stereo by mixing the inverse of the other channel
 */
#if 0
class tSigStereoSpread : public tSignalModifier
{
  public:
    tSigStereoSpread(tSigInput &sig, tLFO &val)
      : tSignalModifier(sig), lfo(val)
    {
      lfo.Init(*this);
    }
    virtual int operator()(tSigValue &val)
    {
      if (!sig(val))
        return 0;
      float a = (lfo() + 1)/2; // map to 0..1
      if (channels > 1)
      {
        float tmp = val[0];
        val[0] -= a * val[1];
        val[1] -= a * tmp;
      }
      return 1;
    }
  protected:
    tLFO &lfo;
};

#endif

/**
 * mix 2 signals controlled by a balance lfo
 */

class tSigMix2 : public tSignalModifier
{
  public:
    tSigMix2(tSigSynth &synth) : tSignalModifier(synth)
    {
    }

    void Init()
    {
      tSignalModifier::Init();  // initialize sources
      len1 = inputs[0]->GetLength();
      len2 = inputs[1]->GetLength();
    }

    void NextValue()
    {
      tSigValue v1;
      tSigValue v2;
      inputs[0]->GetSample(v1);
      inputs[1]->GetSample(v2);
      float p = controls[0]->GetControl();
      if (p > 0)
        for (int i = 0; i < channels; i++)
          val[i] = v1[i] + (1 - p) * v2[i];
      else
        for (int i = 0; i < channels; i++)
          val[i] = v1[i] * (1 + p) + v2[i];
    }
  protected:
    long len1;
    long len2;
};


class tSigMixer : public tSignalModifier
{
  public:

    tSigMixer(tSigSynth &synth, float minctl = -1, float maxctl = 1)
      : tSignalModifier(synth),
        map(minctl, maxctl, 0, 1)
    {
    }

    void NextValue()
    {
      val = nul;
      unsigned ControlCount = controls.size();
      for (unsigned i = 0; i < inputs.size(); i++)
      {
        tSigValue v;
        inputs[i]->GetSample(v);
        if (i < ControlCount)
        {
          float vol = controls[i]->GetControl();
          vol = map(vol);
          v *= vol * vol;
        }
        val += v;
      }
    }

  private:

    tLineMap<float> map;
};

#endif // !defined(JZ_SIGNAL2_H)
