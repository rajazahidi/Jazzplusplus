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
#ifndef genmeldp_h
#define genmeldp_h

#include "events.h"

#include <vector>

class tRndArray;

class RMGNumber {
public:
  virtual int get() = 0;
  virtual int get(int i) {
    return get();
  }
  virtual int get(double d) {
    return get();
  }
  virtual void reset() {}
};


class RMGParam : public RMGNumber {
public:
  RMGParam() {
    impl = 0;
  }

  virtual int get() {
    if (impl)
      return impl->get();
    return 0;
  }

  virtual int get(int i) {
    if (impl)
      return impl->get(i);
    return 0;
  }
  virtual int get(double d) {
    if (impl)
      return impl->get(d);
    return 0;
  }
  virtual void reset() {
    if (impl)
      impl->reset();
  }

  void operator=(RMGNumber *ptr) {
    impl = ptr;
  }

private:
  RMGNumber *impl;
};



template <int SIZE>
class RMGParamArray {
public:
  RMGParam& operator[](int i) {
    return arr[i];
  }
  void reset() {
    for (int i = 0; i < SIZE; i++)
      arr[i].reset();
  }
  int operator()(int i) {
    return arr[i].get();
  }
  int operator()(int i, int k) {
    return arr[i].get(k);
  }
private:
  RMGParam arr[SIZE];
};



class RMGConst : public RMGNumber {
public:
  RMGConst(int val) : value(val) {}
  int get() {
    return value;
  }
private:
  int value;
};


class RMGRandom : public RMGNumber {
public:
  RMGRandom(tRndArray &arr);
  RMGRandom(tRndArray &arr, int minval);
  RMGRandom(tRndArray &arr, int minval, int maxval);
  int get();
  int get(int i);
  int get(double d);
private:
  tRndArray &arr;
  int minval, maxval;
};


class RMGRhythm : public RMGNumber {
public:
  RMGRhythm(tRndArray &arr);
  int get();
  void reset() {
    index = -1;
  }
private:
  tRndArray &arr;
  int index;
};


/*
class RMGInterval : public RMGNumber {
public:
  RMGInterval(RMGNumber &intervals, int center, int range);
  int get();
  void reset();
private:
  int value;
  int center;
  int range;
};
*/


class RMGOutput {
public:
  virtual void Note(long clock, int chn, int key, int vel, int len) = 0;
};


class RMGScale {
public:
  virtual int next_up(int key, long clock, int signif) = 0;
  virtual int next_dn(int key, long clock, int signif) = 0;
};


struct RMGEvent {
  long  clock;    // relative to start of phrase
  int   channel;  // midi channel
  int   interv;   // delta pitch
  int   length;   // note length
  int   veloc;    // note velocity
  int   signif;   // significance 0..100

public:

  RMGEvent(long clk, int chn, int key, int vel, int len, int sig) {
    clock   = clk;
    channel = chn;
    interv  = key;
    length  = len;
    veloc   = vel;
    signif  = sig;
  }
  RMGEvent() {}
  RMGEvent(const RMGEvent &o) {
    clock = o.clock;
    channel = o.channel;
    interv = o.interv;
    length = o.length;
    veloc = o.veloc;
    signif = o.signif;
  }
};




#ifdef wx_msw
typedef std::vector<RMGEvent> RMGEventArray;
#else
typedef vector<RMGEvent> RMGEventArray;
#endif

class RMGPhrase : public RMGEventArray {
  friend class RMGInventor;
public:
  RMGPhrase();
  RMGPhrase(const RMGPhrase &o) : RMGEventArray(o) { len = o.len; }
  RMGPhrase& operator=(RMGPhrase const &o) {
    erase(begin(), end());
    len = 0;
    append(o);
    return *this;
  }
  void clear();
  void append(const RMGPhrase &o);
  RMGPhrase operator+(const RMGPhrase &o) const;
  RMGPhrase& operator +=(const RMGPhrase &o);
  RMGPhrase clone();

  int length() const {
    return len;
  }
  int sumintv() const;
  void transpose(int steps);
protected:
  int len;
};



class RMGInventor {
public:
  enum {
    RHYTHM, LENGTH, VELOC, INTERV, CHANNEL, SEEDLEN, SIGNIF, RANGE, N_PARAM
  };
  RMGParamArray<N_PARAM> param;

  void create(RMGPhrase &phrase);
};




class RMGVariations {
public:
  RMGVariations(RMGInventor &inventor);
  enum {
    VARIAT, REPEAT, N_PARAM
  };
  RMGParamArray<N_PARAM> param;

  enum Variations {
    NOCHANGE, EXCHUPDN, RECURSE, RESIZE,
      N_VARIAT
  };

  void exch_updn(RMGPhrase &phrase);
  void resize(RMGPhrase &phrase, bool incr);
  void create(RMGPhrase &phrase);

private:
  RMGInventor &inventor;
  int recursion_level;
};


class RMGGenerator {
public:
  RMGGenerator(RMGOutput &o, RMGScale &s)
    : out(o), scale(s)
  {
  }

  void run(long size);

  enum {
    LENGTH, VELOC, INTERV, SEEDLEN, REPEAT,
    VARIAT, RHYTHM, CENTER, RANGE, CHORD, CHANNEL,
    N_PARAM
  };

  RMGParamArray<N_PARAM> param;

private:
  int scale_transpose(int pitch, int interv);
  RMGOutput &out;
  RMGScale  &scale;
  RMGPhrase phrase;
};

#endif
