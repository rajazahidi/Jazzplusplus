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

#ifndef JZ_HARMONYBROWSERANALYZER_H
#define JZ_HARMONYBROWSERANALYZER_H

class JZFilter;
class JZTrack;
class HBContext;
class tKeyOn;
class HBChord;

class HBAnalyzer
{
  public:

    HBAnalyzer(HBContext **seq, int n_seq);

    ~HBAnalyzer();

    int Analyze(JZFilter *f, int eighth_per_chord = 8);

    int Transpose(JZFilter *f, int eighth_per_chord = 8);

    void Init(JZFilter *f, int steps_per_bar);

    void Exit();

    int Steps() const;

    int Step2Clock(int step);

    HBContext* GetContext(int step) const;

  private:

    void IterateEvents(void (HBAnalyzer::*Action)(tKeyOn *on, JZTrack *t));
    void CountEvent(tKeyOn *on, JZTrack *t);
    void TransposeEvent(tKeyOn *on, JZTrack *t);
    void CreateChords();
    int NumCount(int i);
    int MaxCount(int i, const HBChord &done);
    void GenerateMapping();

  private:

    HBContext** seq;
    int max_seq;

    int start_clock, stop_clock;
    int eighths_per_chord;
    int mSteps;
    JZFilter* mpFilter;
    JZTrack* mpTrack;

    int** count;
    int** delta;
};

inline
int HBAnalyzer::Steps() const
{
  return mSteps;
}

inline
HBContext* HBAnalyzer::GetContext(int step) const
{
  return seq[step % max_seq];
}

#endif // !defined(JZ_HARMONYBROWSERANALYZER_H)
