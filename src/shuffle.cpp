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
#pragma hdrstop

#include "config.h"
#include "shuffle.h"
#include "song.h"
#include "eventwin.h"
#include "track.h"
#include "events.h"
#include "command.h"
#include "random.h"

// **************************************************************************
// tCmdShuffle
// *************************************************************************

class tSegmentedTrack : public tSimpleEventArray
{
    tTrack *track;
    long   track_fr_clock;
    long   track_to_clock;
    long   clocks_per_segm;
    long   length;       // number of steps
    int    skip_silence;
    int    n_segments;

  public:

    tSegmentedTrack() {
      track = 0;
      track_fr_clock =0;
      track_to_clock = 0;
      skip_silence = 0;
      clocks_per_segm = 16;
      length = 0;
      n_segments = 1;
    }

    void Init(tTrack *track, long fr_clock, long to_clock, long clocks_per_segm, int skip_silence);

    int  SegmentLength(int step);  // returns the length in segments of the i-th segment

    int  SegmentCount() const {   // returns the number of segments
      return n_segments;
    }

    void DeleteSource();
    int  CopySegment(int segm, long clock);


    long Segment2Clock(long segm) const {
      return segm * clocks_per_segm;
    }
    long Clock2Segment(long clock) const {
      return clock / clocks_per_segm;
    }
};


void tSegmentedTrack::Init(tTrack *track, long fr_clock, long to_clock, long clocks_per_segm, int skip_silence)
{
  this->track        = track;
  this->clocks_per_segm        = clocks_per_segm;
  this->skip_silence = skip_silence;
  this->track_fr_clock = fr_clock;
  this->track_to_clock = to_clock;

  tEventIterator iter(track);

  long delta_clock = fr_clock;
  long max_segm = (to_clock - fr_clock) / clocks_per_segm;
  long empty_segments = 0;
  for (long segm = 0; segm < max_segm; segm ++) {
    long event_count = 0;
    tEvent *e = iter.Range(fr_clock + Segment2Clock(segm), fr_clock + Segment2Clock(segm+1));

    while (e) {
      tEvent *c = e->Copy();
      c->Clock -= delta_clock;
      Put(c);
      e = iter.Next();
      event_count ++;
    }
    if (skip_silence && event_count == 0) {
      empty_segments ++;
      delta_clock += clocks_per_segm;
    }
  }
  n_segments = max_segm - empty_segments;
  if (n_segments <= 0)
    n_segments = 1;    // generate silence on empty tracks

  DeleteSource();
}


void tSegmentedTrack::DeleteSource()
{
  tEventIterator iter(track);
  tEvent *e = iter.Range(track_fr_clock, track_to_clock);
  while (e) {
    track->Kill(e);
    e = iter.Next();
  }
}


int tSegmentedTrack::SegmentLength(int segm)
{
  segm %= n_segments;  // cycle thru segments

  long fr_clock = Segment2Clock(segm);
  long to_clock = Segment2Clock(segm + 1);
  tEventIterator iter(this);
  tEvent *e = iter.Range(fr_clock, to_clock);
  while (e) {
    long clk = e->Clock + e->GetLength();

    // while (clk overlaps more than half of the next segment)
    //   add a full segment
    while (clk > to_clock + clocks_per_segm/2)
      to_clock += clocks_per_segm;

    e = iter.Next();
  }

// cout << "segm length: " << segm << ": " << Clock2Segment(to_clock - fr_clock) << endl;

  return Clock2Segment(to_clock - fr_clock);
}


int tSegmentedTrack::CopySegment(int segm, long clock)
{
  segm %= n_segments;  // cycle thru segments

  long fr_clock = Segment2Clock(segm);
  long to_clock = Segment2Clock(segm + 1);
  long count = 0;
  tEventIterator iter(this);
  tEvent *e = iter.Range(fr_clock, to_clock);
  while (e) {
    tEvent *c = e->Copy();
    c->Clock = c->Clock - fr_clock + clock;
    track->Put(c);
    e = iter.Next();
    count ++;
  }
  return count;
}



// **************************************************************************
// IntIterator
// *************************************************************************

class tIntIterator
{
  public:
    virtual int get() = 0;

    virtual tIntIterator& next() {
      if (paused_steps > 0)
        paused_steps -= 1;
      return *this;
    }

    virtual void reset() {
      paused_steps = 0;
    }

    virtual void pause(int steps) {
      paused_steps = steps;
    }

    virtual int paused() const {
      return paused_steps > 0;
    }

    tIntIterator() {
      paused_steps = 0;
    }
  protected:
    int paused_steps;
};

class tCycleIntIter : public tIntIterator
{
  public:
    tCycleIntIter(int lwb, int upb) // range lwb...upb-1
    {
      this->lwb = lwb;
      this->upb = upb;
      val = lwb;
    }
    virtual int get() {
      return val;
    }
    virtual tIntIterator& next() {
      tIntIterator::next();
      int len = upb - lwb;
      val -= lwb;
      val = (val + 1) % len;
      val += lwb;
      return *this;
    }
    virtual void reset() {
      tIntIterator::reset();
      val = lwb;
    }
  protected:
    int lwb, upb, val;
};

class tRandomIntIter : public tCycleIntIter
{
  public:
    tRandomIntIter(int lwb, int upb)
      : tCycleIntIter(lwb, upb),
        map(0, 1, lwb, upb)
    {}
    virtual tIntIterator& next() {
      tIntIterator::next();
      val = (int)map(rnd.asDouble());
      return *this;
    }
    virtual void reset() {
      tIntIterator::next();
      val = (int)map(rnd.asDouble());
    }
  protected:
    tMapper map;
};

class tSyncIntIter : public tIntIterator
{
  public:
    tSyncIntIter(tIntIterator &o) : iter(o) {}
    virtual int get() {
      return iter.get();
    }
  protected:
    tIntIterator &iter;
};

// **************************************************************************
// tRndShuffle
// *************************************************************************

class tRndShuffle {
  public:
    enum TrackMode { Sync, Rand, Excl };
    tRndShuffle(tFilter *f);
    void SetTrackMode(TrackMode x) { track_mode = x; }
    void SetSkipSilence(Bool x) { skip_silence = x; }
    void SetRandomOrder(Bool x) { random_order = x; }
    void SetClocksPerSegment(long x) { clocks_per_segm = x; }
    void RunOther();
    void RunExcl();
    void Execute();
  protected:

    tIntIterator *NewIter(int lwb, int upb, tIntIterator *sync = 0);
    void Prepare();
    void Unprepare();

    tFilter *filter;
    TrackMode track_mode;
    int skip_silence;
    int random_order;
    int clocks_per_segm;

    int n_tracks;
    tSegmentedTrack **tracks;
    tIntIterator **iters;
    tIntIterator *sync;
};

tRndShuffle::tRndShuffle(tFilter *f)
  : filter(f)
{
  track_mode = Excl;
  skip_silence = 1;
  random_order = 1;
  clocks_per_segm = 96;
}

void tRndShuffle::Prepare() {
  int i;

  tTrackIterator iter(filter);

  n_tracks = iter.Count();
  tracks = new tSegmentedTrack * [n_tracks];
  tTrack *t = iter.First();
  for (i = 0; i < n_tracks; i++) {
    tracks[i] = new tSegmentedTrack();
    tracks[i]->Init(t, filter->FromClock, filter->ToClock, clocks_per_segm, skip_silence);
    t = iter.Next();
  }

  sync = NewIter(0, tracks[0]->SegmentCount(), 0);
  iters = new tIntIterator * [n_tracks];
  for (i = 0; i < n_tracks; i++)
    iters[i] = NewIter(0, tracks[i]->SegmentCount(), sync);
}

void tRndShuffle::Unprepare() {
  int i;
  for (i = 0; i < n_tracks; i++) {
    delete tracks[i];
    delete iters[i];
  }
  delete [] tracks; tracks = 0;
  delete [] iters; iters = 0;
  delete sync; sync = 0;

  tTrackIterator iter(filter);
  tTrack *t = iter.First();
  while (t) {
    t->Cleanup();
    t = iter.Next();
  }
}

tIntIterator *tRndShuffle::NewIter(int lwb, int upb, tIntIterator *sync) {
#if 1
  if (track_mode == Sync && sync)
    return new tSyncIntIter(*sync);
  if (random_order)
    return new tRandomIntIter(lwb, upb);
  return new tCycleIntIter(lwb, upb);
#else
  cout << "new iter: lwb = " << lwb << ", upb " << upb << endl;
  return new tRandomIntIter(lwb, upb);
#endif
}


void tRndShuffle::RunOther() {
  int i;

  Prepare();
  long clock = filter->FromClock;
  while (clock < filter->ToClock) {

    for (i = 0; i < n_tracks; i++) {
      if (!iters[i]->paused()) {
	int segm = iters[i]->get();
	tracks[i]->CopySegment(segm, clock);
	iters[i]->pause(tracks[i]->SegmentLength(segm));
	iters[i]->next();
      }
      else
	iters[i]->tIntIterator::next();
    }

    clock += clocks_per_segm;
    sync->next();
  }
  Unprepare();
}


void tRndShuffle::RunExcl() {
  int i, k;

  Prepare();
  long clock = filter->FromClock;
  while (clock < filter->ToClock) {

    // count available tracks
    int tracks_avail = 0;
    for (i = 0; i < n_tracks; i++)
      if (!iters[i]->paused())
	tracks_avail ++;

    if (tracks_avail > 0) {

      // choose one by random
      int track_nr = (int)(rnd.asDouble() * tracks_avail);
      for (i = 0; i < n_tracks; i++) {
	if (!iters[i]->paused())
	  if (track_nr-- == 0)
	    break;
      }

      // let the coosen track copy a segment
      int segm = iters[i]->get();
      tracks[i]->CopySegment(segm, clock);
      iters[i]->pause(tracks[i]->SegmentLength(segm));
      iters[i]->next();
    }

    // switch pause only to the next step
    for (k = 0; k < n_tracks; k++)
      if (i != k)
        iters[k]->tIntIterator::next();

    clock += clocks_per_segm;
    sync->next();
  }
  Unprepare();
}


void tRndShuffle::Execute()
{
  if (track_mode == Excl)
    RunExcl();
  else
    RunOther();
}


// **************************************************************************
// ShuffleDlg
// *************************************************************************

int  tShuffleDlg::track_mode      = 1;   // 0 = all in sync, 1 = all random, 2 = one exclusive
bool tShuffleDlg::skip_silence    = 1;   // 1
bool tShuffleDlg::random_order    = 1;	 // 0
long tShuffleDlg::clocks_per_segm = 24;  // 1/4 * 96

static tNamedValue ShuffleSteps[] =
{
  //tNamedValue( "1/96",  1 ),
  //tNamedValue( "1/48",  2 ),
  //tNamedValue( "1/32",  3 ),
  tNamedValue( "1/24",  4 ),
  tNamedValue( "1/16",  6 ),
  tNamedValue( "1/8",  12 ),
  tNamedValue( "1/4",  24 ),
  tNamedValue( "1/2",  48 ),
  tNamedValue( "3/4",  72 ),
  tNamedValue( "7/8",  84 ),
  tNamedValue( "1/1",  96 ),
  tNamedValue( "2/1", 192 ),
  tNamedValue( "3/1", 288 ),
  tNamedValue( "4/1", 384 ),
  tNamedValue( "8/1", 768 ),
  tNamedValue(  0,      0  )
};


tShuffleDlg::tShuffleDlg(tEventWin *w, tFilter *f)
  : tPropertyListDlg("Random Shuffle"),
     StepList("segment", ShuffleSteps, &clocks_per_segm)
{
  filter = f;
  win    = w;
}

const char *tShuffleDlg::modetxt[] = { "All tracks in sync", "All tracks independently", "One track at a time", 0 };

void tShuffleDlg::AddProperties()
{
#ifndef __PORTING
  Add(StepList.mkFormItem(200));
  Add(wxMakeFormNewLine());
  Add(wxMakeFormBool("skip silence", &skip_silence));
  Add(wxMakeFormNewLine());
  Add(wxMakeFormBool("random order", &random_order));
  Add(wxMakeFormNewLine());



#if 0
  Add(wxMakeFormShort("mode ", &track_mode, wxFORM_DEFAULT,
                       new wxList(wxMakeConstraintRange(0, 2), 0)));
#else

  modestr = copystring(modetxt[track_mode]);
  for (int i = 0; modetxt[i]; i++)
    modelist.Append((wxObject *)modetxt[i]);  // ???
  Add(wxMakeFormString("Track mode", (char **)&modestr, wxFORM_CHOICE,
      new wxList(wxMakeConstraintStrings(&modelist), 0), NULL, wxHORIZONTAL));

#endif

  AssociatePanel(panel);
#endif // __PORTING

}



bool tShuffleDlg::OnClose()
{
  wxBeginBusyCursor();

  for (int i = 0; modetxt[i]; i++) {
    if (!strcmp(modestr, modetxt[i])) {
      track_mode = i;
      break;
    }
  }

  tSong *song = filter->Song;
  song->NewUndoBuffer();

  StepList.GetValue();
  long cps = clocks_per_segm * song->TicksPerQuarter / 24L;
  tRndShuffle shuffle(filter);
  shuffle.SetTrackMode((tRndShuffle::TrackMode)track_mode);
  shuffle.SetSkipSilence(skip_silence);
  shuffle.SetRandomOrder(random_order);
  shuffle.SetClocksPerSegment(cps);
  shuffle.Execute();

  win->Redraw();
  if (win->NextWin)
    win->NextWin->Redraw();

  wxEndBusyCursor();
  //  wxForm::OnOk();
  return FALSE;
}

void tShuffleDlg::OnHelp()
{
  HelpInstance->ShowTopic("Random Shuffle");
}

