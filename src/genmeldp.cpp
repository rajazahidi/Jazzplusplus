/*
**  The JAZZ++ Midi Sequencer
**
** Copyright (C) 1994-2000 Andreas Voss and Per Sigmond, all rights reserved.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
*/                                                                              

#include "random.h"

#include "genmeldp.h"


RMGRhythm::RMGRhythm(tRndArray &array) : arr(array)
{
  index = -1;
}


int RMGRhythm::get()
{
  int N = arr.Size();
  for (int i = 0; i < N; i++) {
    index = (index + 1) % N;
    if (arr.Random(index))
      return i + 1;
  }
  return N;
}



RMGRandom::RMGRandom(tRndArray &array, int minv, int maxv)
  : arr(array), minval(minv), maxval(maxv)
{
}


RMGRandom::RMGRandom(tRndArray &array)
  : arr(array), minval(0), maxval(array.Size())
{
}


RMGRandom::RMGRandom(tRndArray &array, int minv)
  : arr(array), minval(minv), maxval(minv + array.Size())
{
}

int RMGRandom::get()
{
  double i = arr.Random();
  double N = arr.Size();
  return minval + (int)(i / N * (maxval - minval));
}

int RMGRandom::get(int i)
{
  return arr.Random(i);
}

int RMGRandom::get(double d)
{
  return arr.Random(d);
}



// ----------------------- RMGPhrase ------------------------


RMGPhrase::RMGPhrase() {
  len = 0;
}


void RMGPhrase::append(const RMGPhrase &o) {
  int N = o.size();
  for (int i = 0; i < N; i++) {
    RMGEvent e = o[i];
    e.clock += len;
    push_back(e);
  }
  len += o.len;
}


RMGPhrase RMGPhrase::clone() {
  RMGPhrase phrase;
  phrase.append(*this);
  return phrase;
}


void RMGPhrase::transpose(int steps)
{
  iterator it = begin();
  if (it != end())
    (*it).interv += steps;
}

int RMGPhrase::sumintv() const
{
  int sumkey = 0;
  const_iterator it = begin();
  while (it != end()) {
    const RMGEvent &e = *it++;
    sumkey += e.interv;
  }
  return sumkey;
}

RMGPhrase RMGPhrase::operator+(const RMGPhrase &o) const
{
  RMGPhrase tmp = *this;
  tmp.append(o);
  return tmp;
}

RMGPhrase& RMGPhrase::operator +=(const RMGPhrase &o)
{
  append(o);
  return *this;
}


void RMGPhrase::clear()
{
  erase(begin(), end());
  len = 0;
}


// ----------------------- RMGInventor ------------------------


void RMGInventor::create(RMGPhrase &phrase) {
  param.reset();
  phrase.clear();


  phrase.len = param(SEEDLEN);

  // rhythm param returns intervals 1..n
  long clk = param(RHYTHM) - 1;

  int sign = (rnd.asDouble() < 0.5) ? -1 : 1;

  while (clk < phrase.len) {

    int len = param(LENGTH);
    if (clk + len > phrase.len)
      len = phrase.len - clk;

    int vel = param(VELOC);

    int key = param(INTERV);
#if 0
    if (rnd.asDouble() < -0.3)
      sign = -sign;
    key *= sign;
#else
    if (rnd.asDouble() < 0.5)
      key = -key;
#endif

    int chn = param(CHANNEL);
    int sig = param(SIGNIF);

    RMGEvent event(clk, chn, key, vel, len, sig);
    phrase.push_back(event);

    // avoid overlapping notes
    long dclk = param(RHYTHM);
    if (dclk < len)
      dclk = len;
    clk += dclk;
  }

}



// ----------------------- RMGVariations -----------------------


RMGVariations::RMGVariations(RMGInventor &inv)
: inventor(inv)
{
  recursion_level = 0;
}


void RMGVariations::exch_updn(RMGPhrase &phrase)
{
  RMGPhrase::iterator it = phrase.begin();
  while (it != phrase.end()) {
    RMGEvent &e = *it++;
    e.interv = -e.interv;
  }
}


void RMGVariations::resize(RMGPhrase &phrase, bool incr)
{
  RMGPhrase::iterator it = phrase.begin();
  while (it != phrase.end()) {
    RMGEvent &e = *it++;
    if (incr) {
      if (e.interv > 0)
        ++ e.interv;
      else
        -- e.interv;
    }
    else {
      if (e.interv > 0)
        -- e.interv;
      else
        ++ e.interv;
    }
  }
}



void RMGVariations::create(RMGPhrase &orig)
{
  if (recursion_level > 3)
    return;
  ++ recursion_level;

  RMGPhrase out = orig;
  RMGPhrase seed = orig;

  int count = param(REPEAT);
  Variations action = (Variations)param(VARIAT);

  for (int i = 1; i < count; i++) {
    switch (action) {
    case EXCHUPDN:
      exch_updn(seed);
      break;
    case RESIZE:
      resize(seed, (count % 2) == 0);
      break;
    }
    out.append(seed);
  }

  action = (Variations)param(VARIAT);
  switch (action) {
  case RECURSE:
    {
      RMGPhrase temp;
      inventor.create(temp);
      //create(temp);
      out = temp + out + temp;
    }
  }

  orig = out;
  -- recursion_level;
}


// ----------------------- RMGGenerator -----------------------


void RMGGenerator::run(long size) {
  RMGInventor inventor;

  inventor.param[RMGInventor::RHYTHM]  = param[RHYTHM];
  inventor.param[RMGInventor::LENGTH]  = param[LENGTH];
  inventor.param[RMGInventor::VELOC]   = param[VELOC];
  inventor.param[RMGInventor::INTERV]  = param[INTERV];
  inventor.param[RMGInventor::CHANNEL] = param[CHANNEL];
  inventor.param[RMGInventor::SEEDLEN] = param[SEEDLEN];
  inventor.param[RMGInventor::RANGE]   = param[RANGE];

  RMGVariations variations(inventor);
  variations.param[RMGVariations::VARIAT] = param[VARIAT];
  variations.param[RMGVariations::REPEAT] = param[REPEAT];

  int pitch_center = param(CENTER);
  int pitch_range  = param(RANGE);
  int note_count   = param(CHORD);

  long start = 0;
  int pitch = scale.next_up(param(CENTER), start, 0);
  while (size > 0) {
    inventor.create(phrase);
    variations.create(phrase);


    RMGPhrase::iterator it = phrase.begin();
    while (it != phrase.end()) {
      RMGEvent e = *it++;
      if (e.clock >= size)
        break;

      long clock = start + e.clock;
      int interv = e.interv;

      // limit to range
      if (pitch > pitch_center + pitch_range && interv > 0)
        interv = -interv;
      if (pitch < pitch_center - pitch_range && interv < 0)
        interv = -interv;

      pitch = scale_transpose(pitch, interv);
      out.Note(clock, e.channel, pitch, e.veloc, e.length);

      int tmp_pitch = pitch;
      for (int i = 1; i < note_count; i++) {
        tmp_pitch = scale_transpose(tmp_pitch, -1);
        if (rnd.asDouble() < 0.5)
          tmp_pitch = scale_transpose(tmp_pitch, -1);
        out.Note(clock, e.channel, tmp_pitch, e.veloc, e.length);
      }

    }

    start += phrase.length();
    size  -= phrase.length();
  }

}


int RMGGenerator::scale_transpose(int pitch, int interv) {
  while (interv > 0) {
    pitch = scale.next_up(pitch + 1, 0, 0);
    interv--;
  }
  while (interv < 0) {
    pitch = scale.next_dn(pitch - 1, 0, 0);
    interv++;
  }
  return pitch;
}

