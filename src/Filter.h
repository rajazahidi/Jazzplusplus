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
class JZTrack;


#define FltKeyOn        0
#define FltKeyPressure  1  // SN++ PolyAftertouch gehoert to KeyOn Events!
#define FltControl      2
#define FltProgram      3
#define FltPitch        4
#define FltTempo        5
#define FltChnPressure  6  // SN++ Channel Aftertouch
#define FltSysEx        7

#define nFltEvents      8

//*****************************************************************************
//*****************************************************************************
class JZFilterEvent
{
  public:
    int Stat;
    const char* Name;
    bool Selected;
    int MinValue, MaxValue;
    int FromValue, ToValue;
};

//*****************************************************************************
//*****************************************************************************
class JZFilter : public wxObject
{
    friend class tFilterDlg;
    wxDialog* DialogBox;
    void copy(const JZFilter& Other);

  public:

    JZFilterEvent* FltEvents;
    bool OtherSelected;

    JZSong* mpSong;

    int FromClock, ToClock;        // einschl .. ausschl

    int FromTrack, ToTrack;        // 1..n einschl .. einschl

    void Dialog(wxFrame *parent, int ShowEventStats = 1);

    JZFilter(JZSong* pSong);
    JZFilter(JZFilter* pOtherFilter);
    JZFilter(const JZFilter& Other);
    JZFilter& operator = (const JZFilter& Rhs);
    virtual ~JZFilter();

    int IsSelected(JZEvent* pEvent)
    {
      int Value = pEvent->GetValue();
      for (int i = 0; i < nFltEvents; ++i)
      {
        if (pEvent->Stat == FltEvents[i].Stat)
        {
           // SN++ Aftertouch gehoert eigendlich zu KeyOn Events.
          if (pEvent->Stat == StatKeyPressure)
          {
            int aval = pEvent->IsKeyPressure()->Key;
            return
              FltEvents[i].Selected &&
              FltEvents[i].FromValue <= aval &&
              aval <= FltEvents[i].ToValue;
          }
          if (pEvent->Stat == StatTimeSignat)
          {
            return FltEvents[i].Selected;
          }
          // SN++
          if (pEvent->Stat == StatChnPressure)
          {
            return FltEvents[i].Selected;
          }

          if (pEvent->Stat == StatSysEx)
          {
            return FltEvents[i].Selected;
          }

          return
            FltEvents[i].Selected &&
            FltEvents[i].FromValue <= Value &&
            Value <= FltEvents[i].ToValue;
        }
      }
      return OtherSelected;
    }
};

// void GlobalFilterDlg(wxButton& but, wxMouseEvent& event);
// void GlobalFilterDlgNoStats(wxButton& but, wxMouseEvent& event);

//*****************************************************************************
// Description:
//   This is the track iterator class declaration.
//*****************************************************************************
class JZTrackIterator
{
  public:

    JZTrackIterator(JZFilter* pFilter, bool Reverse = false);
    JZTrack* First();
    JZTrack* Next();
    int Count() const;

  private:

    JZFilter* mpFilter;
    JZSong* mpSong;
    int mTrackIndex;
    bool mReverse;
};

#endif // !defined(JZ_FILTER_H)
