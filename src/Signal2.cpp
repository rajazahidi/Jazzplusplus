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

#include "Signal2.h"
#include "SignalInterface.h"
#include "Random.h"
#include "FrequencyTable.h"

using namespace std;

void tSigSynth::Run(tSigOutput &osig, tSigInput &isig, float add_seconds)
{
  isig.Init();
  osig.Init();
  current = 0;
  // add 1 second to length
  const long N = isig.GetLength() + (long)(add_seconds * sampling_rate);
  osig.Resize(N);
  tSigValue val;
  for (long i = 0; i < N; i++) {
    isig.GetSample(val);
    osig.Out(val);
    current++;
  }
}

void tSigSynth::DeleteAllGenerators()
{
  for (unsigned i = 0; i < generators.size(); ++i)
  {
    delete generators[i];
  }

  //generators.clear();
  generators = vector<tSigInput*>();
}

const float tSigReverb::comb_times[COMBS] =
{
  0.0297f, 0.0371f, 0.0411f, 0.0437f
};

const float tSigReverb::alpas_times[ALPAS] =
{
  0.005f, 0.0017f // , 0.013f
};

// ----------------------------------------------------------------
//                          interface functions
// ----------------------------------------------------------------

void sig_reverb(
  tSample &spl,
  float rvbtime_val, // echo absorbtion
  float bright_val,  // lowpass filter freq
  float volume_val,  // effect volume
  float room_val)    // echo delay
{
  long sr = spl.GetSamplingRate();
  long ch = spl.GetChannels();
  tSigSynth synth(sr, ch);

  tShortIter isig(synth, spl);
  tFloatSample obuf(ch, sr);
  tFloatIter osig(synth, obuf);

  tSigReverb comb(synth, rvbtime_val, bright_val, volume_val, room_val);
  comb.AddInput(isig);
  synth.Run(osig, comb, rvbtime_val);

  obuf.Rescale();
  spl.Set(obuf);

}


/*
          isig
           |
          delay --  lfo_delay
           |
          pan ----- lfo_pan
           |
          mixer --- const(volume)
           |
          osig
*/

void sig_chorus(
  tSample &spl,
  float  pitch_freq,    // pitch modification freq
  float  pitch_range,   // variable delay in seconds
  float  pan_freq,      // pan freq in Hz
  float  pan_spread,    // 0..1
  float  volume)        // 0..1
{
  long sr = spl.GetSamplingRate();
  long ch = spl.GetChannels();
  tSigSynth synth(sr, ch);

  tShortIter isig(synth, spl);
  tFloatSample obuf(ch, sr);
  tFloatIter osig(synth, obuf);

  tSigDelay delay(synth, pitch_range);
  tSigSine  lfo_delay(synth, pitch_freq, 1, -PI/2);
  delay.AddInput(isig);
  delay.AddControl(lfo_delay);

  tSigPanpot pan(synth);
  tSigSine lfo_pan(synth, pan_freq, pan_spread, 0);
  pan.AddInput(delay);
  pan.AddControl(lfo_pan);

  tSigMix2 mixer(synth);
  tSigConst mix_balance(synth, volume);
  mixer.AddInput(pan);
  mixer.AddInput(isig);
  mixer.AddControl(mix_balance);

  // Run!
  synth.Run(osig, mixer, pitch_range);

  obuf.Rescale();
  spl.Set(obuf);
}

static void setup_wav_harmonics(tSigWaveOscil &wav, JZRndArray &arr)
{
  int k, f;
  const int N = wav.Size();
  for (k = 0; k < N; k++)
    wav[k] = 0;

  for (f = 0; f < arr.Size(); f++) {
    tLineMap<float>map(0, N, 0, 2*PI*(f+1));
    for (k = 0; k < N; k++)
      wav[k] += sin(map(k)) * (float)arr[f] * (float)arr[f];
  }
}


static void setup_wav_control(tSigWaveCtrl &wav, JZRndArray &arr)
{
  int i;

  const int N = wav.Size();

  tLineMap<float>xmap(0, N, 0, arr.Size() - 1);
  tLineMap<float>ymap(arr.Min(), arr.Max(), -1, 1);
  for (i = 0; i < N; i++)
    /* PAT - Original line:  wav[i] = ymap(arr[xmap(i)]); */
    wav[i] = ymap(arr[xmap(i)]);

#if 0
{
  int i, k;
  //cout << arr.GetLabel() << endl;
  for (i = 0; i < N; i++) {
    cout << i << ' ';
    for (k = 0; k < tSigValue::MAXCHN; k++)
      cout << wav[i][k] << ' ';
    cout << endl;
  }
}
#endif

}

void sig_wavsynth(
  tSample &spl,                // destin
  double duration,        // length in seconds
  int midi_key,                // base frequency
  double fshift,        // frequeny modulation factor, 0 = off
  int ntables,                // number of wavetables to be mixed
  JZRndArray *arr[][4],        // (fft, vol, pitch, pan) * N
  int noisegen          // first array is noise filter
)
{
  long sr = spl.GetSamplingRate();
  long ch = spl.GetChannels();
  tSigSynth synth(sr, ch);
  FreqTab ft;
  double freq = ft.freq(midi_key);

  tFloatSample obuf(ch, sr);
  tFloatIter *osig = new tFloatIter(synth, obuf);

  int i;
  tSigMixer *mix = new tSigMixer(synth);
  for (i = 0; i < ntables; i++)
  {
    tSigInput *inp;
    if (!noisegen || i > 0)
    {
      // create wavetable synth
      tSigWaveOscil *wav = new tSigWaveOscil(synth, 1000, freq);
      setup_wav_harmonics(*wav, *arr[i][0]);

      // create a pitch control signal
      tSigWaveCtrl *frq = new tSigWaveCtrl(synth, 200, duration);
      setup_wav_control(*frq, *arr[i][2]);

      // connect freq control to wavetable
      wav->AddControl(*frq);

      inp = wav;
    }
    else
    {
      // create a noise generator
      tSigNoise *noise = new tSigNoise(synth);

      // create a filter control signal from harmonics
      tSigWaveCtrl *ctl = new tSigWaveCtrl(synth, 200, duration);
      setup_wav_control(*ctl, *arr[i][0]);

      // create the filter modifier
      tSigFilter<tOpBandpass> *flt = new tSigFilter<tOpBandpass>(synth, 1000, 100, 10);
      flt->AddInput(*noise);
      flt->AddControl(*ctl);

      inp = flt;
    }

    // create volume control signal
    tSigWaveCtrl *vol = new tSigWaveCtrl(synth, 200, duration);
    setup_wav_control(*vol, *arr[i][1]);

    // make a panpot object
    tSigPanpot *pan = new tSigPanpot(synth);
    tSigWaveCtrl *ppan = new tSigWaveCtrl(synth, 200, duration);
    setup_wav_control(*ppan, *arr[i][3]);
    pan->AddControl(*ppan);

    // put it all together
    pan->AddInput(*inp);
    mix->AddInput(*pan);
    mix->AddControl(*vol);
  }

  // Run!
  synth.Run(*osig, *mix, duration);

  obuf.Rescale();
  spl.Set(obuf);

  // delete the generators
  synth.DeleteAllGenerators();
}

// ---------------------------------------------------------------
//                     old Filter Interface
// ---------------------------------------------------------------

tSplFilter::tSplFilter() {
  filter = 0;
}

tSplFilter::~tSplFilter() {
  delete filter;
}

void tSplFilter::Init(Type t, float xsr, double f0, double bw)
{
  sr = xsr;
  delete filter;
  filter = 0;
  switch(t)
  {
    case LOWPASS:
      filter = new tOpLowpass();
      break;

    case HIGHPASS:
      filter = new tOpHighpass();
      break;

    case BANDPASS:
      filter = new tOpBandpass();
      break;

    case BANDSTOP:
      filter = new tOpBandstop();
      break;
  }
  filter->Setup(sr, f0, f0 * bw);
}

void tSplFilter::ReInit(double f0, double bw)
{
  filter->Setup(sr, f0, f0 * bw);
}

float tSplFilter::Loop(float sig) {
  return filter->Loop(sig);
}

