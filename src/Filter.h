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

#ifndef JZ_FILTER_H
#define JZ_FILTER_H

#include "Events.h"

class JZSong;
class tTrack;


#define FltKeyOn        0
#define FltKeyPressure  1  // SN++ PolyAftertouch gehoert to KeyOn Events!
#define FltControl        2
#define FltProgram        3
#define FltPitch        4
#define FltTempo        5
#define FltChnPressure  6  // SN++ Channel Aftertouch
#define FltSysEx        7

#define nFltEvents 8


class tFltEvent
{
  public:
    int  Stat;
    char *Name;
    bool Selected;
    int MinValue, MaxValue;
    int FromValue, ToValue;
};



class tFilter : public wxObject
{
    friend class tFilterDlg;
    wxDialog *DialogBox;
    void copy(tFilter const &o);

  public:

    tFltEvent *FltEvents;
    bool      OtherSelected;

    JZSong *Song;
    long FromClock, ToClock;        // einschl .. ausschl
    int  FromTrack, ToTrack;        // 1..n einschl .. einschl

    void Dialog(wxFrame *parent, int ShowEventStats = 1);

    tFilter(JZSong *);
    tFilter(tFilter *o);
    tFilter(tFilter const &o);
    tFilter& operator=(tFilter const &o);
    virtual ~tFilter();

    int IsSelected(JZEvent *e)
    {
      int val = e->GetValue();
      for (int i = 0; i < nFltEvents; i++)
      {
        if (e->Stat == FltEvents[i].Stat)
        {
           // SN++ Aftertouch gehoert eigendlich zu KeyOn Events.
          if (e->Stat == StatKeyPressure) {
            int aval = e->IsKeyPressure()->Key;
            return FltEvents[i].Selected &&
                   FltEvents[i].FromValue <= aval && aval <= FltEvents[i].ToValue;
          }
           if( e->Stat == StatTimeSignat)
            return FltEvents[i].Selected;
          // SN++
          if( e->Stat == StatChnPressure)
            return FltEvents[i].Selected;

          if (e->Stat == StatSysEx)
            return FltEvents[i].Selected;

          return FltEvents[i].Selected && FltEvents[i].FromValue <= val && val <= FltEvents[i].ToValue;
        }
      }
      return OtherSelected;
    }

};


// extern tFilter *GlobalFilter;

// void GlobalFilterDlg(wxButton& but, wxMouseEvent& event);
// void GlobalFilterDlgNoStats(wxButton& but, wxMouseEvent& event);


// ----------------------------------------------------------------------
// get selected Tracks from Filter
// ----------------------------------------------------------------------


class tTrackIterator
{
  tFilter *Filter;
  JZSong   *Song;
  int     TrackNr;
  int     Reverse;
  public:
    tTrackIterator(tFilter *f, int Reverse = 0);
    tTrack *First();
    tTrack *Next();
    int    Count() const;
};

#endif // !defined(JZ_FILTER_H)
