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

#include "proplistdlg.h"

#ifndef dialogs_h
#define dialogs_h

#ifndef eventwin_h
#include "eventwin.h"
#endif

class tPianoWin;

// ***************************************************************************
// Menu-Funktionen
// ***************************************************************************

class tShiftDlg : public tPropertyListDlg
{
 public:
  long  Steps;	// 0 was static
  long Unit;
  
  tFilter   *Filter;
  tSong     *Song;
  tEventWin *EventWin;
  
  tShiftDlg(tEventWin *w, tFilter *f, long Unit);
  void AddProperties();
  bool OnClose();
  void OnHelp();
};




class tCleanupDlg : public tPropertyListDlg
{
  public:

    static long lowLimit;	// 1/32
    static Bool shortenOverlaps;

    tFilter *Filter;
    tSong   *Song;
    tEventWin *EventWin;

    tCleanupDlg(tEventWin *w, tFilter *f);
    void AddProperties();
    //tNamedChoice Steps;
    bool OnClose();
    void OnHelp();
};




class tSearchReplaceDlg : public tPropertyListDlg
{
  public:

    static long frCtrl;
    static long toCtrl;
/*     tNamedChoice frList; */
/*     tNamedChoice toList; */

    tFilter *Filter;
    tSong   *Song;
    tEventWin *EventWin;

    tSearchReplaceDlg(tEventWin *w, tFilter *f);
    void AddProperties();
    bool OnClose();
    void OnHelp();
};



// Transpose

class tTransposeDlg : public tPropertyListDlg
{
  public:

    static int  Notes;	// 0
    static bool FitIntoScale;
    static long Scale;

    tEventWin *EventWin;
    tFilter *Filter;
    tSong   *Song;

    //tNamedChoice ScaleDlg;
    tTransposeDlg(tEventWin *w, tFilter *f);
    void AddProperties();
    bool OnClose();
    void OnHelp();
};



// SetChannel

class tSetChannelDlg : public tPropertyListDlg
{
  public:

    static int  NewChannel;	// 0

    tFilter *Filter;
    tSong   *Song;

    tSetChannelDlg(tFilter *f);
    void AddProperties();
    bool OnClose();
    void OnHelp();
};



// Velocity

class tVelocityDlg : public tPropertyListDlg
{
  public:

    static int FromValue, ToValue, Mode;
    static char *mode_str;

    tFilter *Filter;
    tSong   *Song;

    tVelocityDlg(tFilter *f);
    void AddProperties();
    bool OnClose();
    void OnHelp();
};


// Length

class tLengthDlg : public tPropertyListDlg
{
  public:

    static int FromValue, ToValue, Mode;
    static char *mode_str;

    tFilter   *Filter;
    tSong     *Song;
    tEventWin *EventWin;

    tLengthDlg(tEventWin *win, tFilter *f);
    void AddProperties();
    bool OnClose();
    void OnHelp();
};

// seqLength

class tSeqLengthDlg : public tPropertyListDlg
{
  public:


    static double scale;

    tFilter   *Filter;
    tSong     *Song;
    tEventWin *EventWin;

    tSeqLengthDlg(tEventWin *win, tFilter *f);
    void AddProperties();
    bool OnClose();
    void OnHelp();
};



// midiDelay

class tMidiDelayDlg : public tPropertyListDlg
{
  public:

    static double scale;
    static long clockDelay;
    static int repeat;

    tFilter   *Filter;
    tSong     *Song;
    tEventWin *EventWin;

    tMidiDelayDlg(tEventWin *win, tFilter *f);
    void AddProperties();
    bool OnClose();
    void OnHelp();
};





class tDeleteDlg : public tPropertyListDlg
{
  tFilter *Filter;
  tEventWin *EventWin;

 public:
  static Bool LeaveSpace;	// 1

  tDeleteDlg(tEventWin *w, tFilter *f);
  void AddProperties();
  bool OnClose();
  void OnHelp();
};


class tSnapDlg : public tPropertyListDlg
{
    tPianoWin *win;
    long      *ptr;
  public:

    tSnapDlg(tPianoWin *w, long *snapptr);
    void AddProperties();
    //tNamedChoice Steps;
    bool OnClose();
    void OnHelp();
};

class tQuantizeDlg : public tPropertyListDlg
{
  public:

    static bool NoteStart;	// 1
    static bool NoteLength;	// 0
    static long QntStep;	// 1/16
    static int  Groove;		// -x .. +x
    static int  Delay;		// -x .. +x

    tFilter *Filter;
    tSong   *Song;
    tEventWin *EventWin;

    long Quantize(long);

    tQuantizeDlg(tEventWin *w, tFilter *f);
    void AddProperties();
    //tNamedChoice Steps;
    bool OnClose();
    void OnHelp();
};

// ***************************************************************************
// Einzel-Events
// **************************************************************************

class tEvent;
class tPianoWin;
class tEventArray;

void EventDialog(tEvent *, tPianoWin *, tTrack *, long Clock, int Channel, int Pitch);

#endif

