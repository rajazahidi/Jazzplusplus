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

#include "wx/wx.h"

#include "hbanalyz.h"
#include "eventwin.h"
#include "command.h"
#include "harmonyp.h"
#include "song.h"
#include <math.h>


HBAnalyzer::HBAnalyzer(HBContext **s, int n)
{
  seq     = s;
  max_seq = n;

  count = 0;
  delta = 0;
  steps = 0;
}


HBAnalyzer::~HBAnalyzer()
{
  Exit();
}


void HBAnalyzer::Init(tFilter *f, int epc)
{
  Exit();	// cleanup from previous run

  filter = f;
  start_clock = f->FromClock;
  stop_clock = f->ToClock;
  eighths_per_chord = epc;

  if (eighths_per_chord == 0)
    steps = max_seq;	// map number of chords to selection
  else
  {
    tBarInfo BarInfo(filter->Song);
    BarInfo.SetClock(start_clock);
    int start_bar = BarInfo.BarNr;
    BarInfo.SetClock(stop_clock);
    int stop_bar = BarInfo.BarNr;
    steps = (stop_bar - start_bar) * 8L / eighths_per_chord;
  }

  count = new long * [steps];
  delta = new long * [steps];
  for (int i = 0; i < steps; i++)
  {
    count[i] = new long [12];
    delta[i] = new long [12];
    for (int j = 0; j < 12; j++)
    {
      count[i][j] = 0;
      delta[i][j] = 0;
    }
  }
}


void HBAnalyzer::Exit()
{
  for (int i = 0; i < steps; i++)
  {
    delete [] count[i];
    delete [] delta[i];
  }
  delete [] count;
  delete [] delta;
  count = 0;
  delta = 0;
  steps = 0;
}


int HBAnalyzer::Analyze(tFilter *f, int qbc)
{
  Init(f, qbc);
  if (steps < max_seq)
  {
    IterateEvents(&HBAnalyzer::CountEvent);
    CreateChords();
    return steps;
  }
  return 0;
}

int HBAnalyzer::Transpose(tFilter * f, int qbc)
{
  f->Song->NewUndoBuffer();
  Init(f, qbc);
  IterateEvents(&HBAnalyzer::CountEvent);
  GenerateMapping();
  IterateEvents(&HBAnalyzer::TransposeEvent);
  return 0;
}


void HBAnalyzer::IterateEvents(void (HBAnalyzer::*Action)(tKeyOn *on, tTrack *t))
{
  tTrackIterator Tracks(filter);
  tTrack *t = Tracks.First();
  while (t)
  {
    if (!t->IsDrumTrack())
    {
      tEventIterator Events(t);
      tEvent *e = Events.Range(filter->FromClock, filter->ToClock);
      while (e)
      {
	tKeyOn *on = e->IsKeyOn();
	if (on)
	  (this->*Action)(on, t);
	e = Events.Next();
      }
      t->Cleanup();
    }
    t = Tracks.Next();
  }
}


long HBAnalyzer::Step2Clock(int step)
{
  long fr = filter->FromClock;
  long to = filter->ToClock;
  return (step * (to - fr)) / steps + fr;
}

void HBAnalyzer::CountEvent(tKeyOn *on, tTrack *t)
{
  for (int i = 0; i < steps; i++)
  {
    long start = Step2Clock(i);
    long stop  = Step2Clock(i+1);
    if (on->Clock + on->Length >= start && on->Clock < stop)
    {
      if (on->Clock > start)
	start = on->Clock;
      if (on->Clock + on->Length < stop)
	stop = on->Clock + on->Length;
      count[i][on->Key % 12] += stop - start;
    }
  }
}


void HBAnalyzer::TransposeEvent(tKeyOn *on, tTrack *track)
{
  for (int i = 0; i < steps; i++)
  {
    long start = Step2Clock(i);
    long stop  = Step2Clock(i+1);
    if (on->Clock + on->Length >= start && on->Clock < stop)
    {
      // key matches this step
      long fr = start;
      long to = stop;
      if (on->Clock > fr)
	fr = on->Clock;
      if (on->Clock + on->Length < to)
	to = on->Clock + on->Length;

      // transpose if most of key length belongs to this step
      // OR: it covers the whole step
      if (to - fr >= on->Length/2 || (fr == start && to == stop))
      {
	tKeyOn *cp = (tKeyOn *)on->Copy();
	cp->Key += delta[i][on->Key % 12];
	track->Kill(on);
	track->Put(cp);

	// do not transpose again
        break;
      }
    }
  }
}


int HBAnalyzer::NumCount(int i)
{
  // count the notes of step i
  int  n = 0;
  for (int k = 0; k < 12; k++)
  {
    if (count[i][k] > 0)
      n++;
  }
  return n;
}


int HBAnalyzer::MaxCount(int i, const HBChord &done)
{
  // find the most used note in step i
  int  imax = 0;
  long nmax = 0;
  for (int k = 0; k < 12; k++)
  {
    if (count[i][k] > nmax && !done.Contains(k))
    {
      nmax = count[i][k];
      imax = k;
    }
  }
  return imax;
}


#if 1
void HBAnalyzer::GenerateMapping()
{
  int step;
  for (step = 0; step < steps; step++)
  {
    int j;

    int iseq = step % max_seq;
    HBChord chord = seq[iseq]->Chord();
    HBChord scale = seq[iseq]->Scale();
    int notes_remaining = NumCount(step);

    HBChord c;
    HBChord done;
    int     n;

    done.Clear();

    // map chord notes to the most used notes

    c = chord;
    n = c.Count();
    for (j = 0; notes_remaining && j < n; j++)
    {
      int old_key = MaxCount(step, done);
      int new_key = c.Fit(old_key);
      delta[step][old_key] = new_key - old_key;
      c -= new_key;
      notes_remaining --;
      done += old_key;
    }

    // map scale notes to the remaining notes

    c = scale - chord;
    n = c.Count();
    for (j = 0; notes_remaining && j < n; j++)
    {
      int old_key = MaxCount(step, done);
      int new_key = c.Fit(old_key);
      delta[step][old_key] = new_key - old_key;
      c -= new_key;
      notes_remaining --;
      done += old_key;
    }

    // map remaining notes cromatic

    c = chromatic_scale - scale - chord;
    n = c.Count();
    for (j = 0; notes_remaining && j < n; j++)
    {
      int old_key = MaxCount(step, done);
      int new_key = c.Fit(old_key);
      delta[step][old_key] = new_key - old_key;
      c -= new_key;
      notes_remaining --;
      done += old_key;
    }

  }
}

#else

class tChordMatrix {
  public:
    tChordMatrix() {
      for (int i = 0; i < 12; i++)
        for (int j = 0; j < 12; j++)
	  mat[i][j] = 0;
    }
    double* operator[](int i) { return mat[i]; }
  private:
    double mat[12][12];
};


void HBAnalyzer::GenerateMapping()
{
  int step;
  for (step = 0; step < steps; step++)
  {
    int i, j;

    tChordMatrix mat;

    int iseq = step % max_seq;
    HBChord chord = seq[iseq]->Chord();
    HBChord scale = seq[iseq]->Scale();

    int not_in_chord_costs = 50;
    int not_in_scale_costs = 100;
    int costs_per_semitone = 10;

    // create matrix:
    // mat[i][j] = costs for transposing old key i to new key j
    for (i = 0; i < 12; i++)
    {
      for (j = 0; j < 12; j++)
      {
        double cost = 0;
	if (!chord.Contains(j))
	  cost += not_in_chord_costs;
	if (!scale.Contains(j))
	  cost += not_in_scale_costs;
	cost += fabs(i-j) * costs_per_semitone;
	cost *= count[step][i];
        mat[i][j] = cost;
      }
    }


    for (i = 0; i < 12; i++)
    {
      cout << i << "\t: ";
      for (j = 0; j < 12; j++)
        cout << mat[i][j] << "\t";
      cout << endl;
    }

    // now we have something similar to a hamilton problem:
    // for each row i compute a column index v[i] so that all
    // v[i] are different and the sum of mat[i][v[i]]
    // is minimal.

    int v[12];
    for (i = 0; i < 12; i++)
      v[i] = i;
  }
}

#endif


void HBAnalyzer::CreateChords()
{
  long *best = new long [steps];
  for (int i = 0; i < steps; i++)
    best[i] = -1;

  HBContextIterator iter;
  while (iter())
  {
    const HBContext &ct = iter.Context();
    const HBChord chord = ct.Chord();
    const HBChord scale = ct.Scale();
    for (int i = 0; i < steps; i++)
    {
      long err = 0;
      for (int k = 0; k < 12; k++)
      {
        if (!chord.Contains(k))
          err += count[i][k];
        if (!scale.Contains(k))
          err += count[i][k];
      }
      if (best[i] == -1 || err < best[i])
      {
	*seq[i] = ct;
	seq[i]->SetSeqNr(i+1);
	best[i] = err;
      }
    }
  }
  delete [] best;
}

