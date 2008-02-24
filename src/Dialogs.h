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

#ifndef JZ_DIALOGS_H
#define JZ_DIALOGS_H

#include "PropertyListDialog.h"

class JZPianoFrame;
class tFilter;
class JZSong;
class JZEventFrame;
class tTrack;
class JZEvent;

class tShiftDlg : public tPropertyListDlg
{
 public:
  long  Steps;        // 0 was static
  long Unit;
  
  tFilter* Filter;
  JZSong* Song;
  JZEventFrame* EventWin;
  
  tShiftDlg(JZEventFrame *w, tFilter *f, long Unit);
  void AddProperties();
  bool OnClose();
  void OnHelp();
};

class tCleanupDlg : public tPropertyListDlg
{
  public:

    static int lowLimit;          // 1/32
    static bool shortenOverlaps;

    tFilter *Filter;
    JZSong   *Song;
    JZEventFrame *EventWin;

    tCleanupDlg(JZEventFrame *w, tFilter *f);
    void AddProperties();
    //tNamedChoice Steps;
    bool OnClose();
    void OnHelp();
};

class tSearchReplaceDlg : public tPropertyListDlg
{
  public:

    static int frCtrl;
    static int toCtrl;
/*     tNamedChoice frList; */
/*     tNamedChoice toList; */

    tFilter *Filter;
    JZSong   *Song;
    JZEventFrame *EventWin;

    tSearchReplaceDlg(JZEventFrame *w, tFilter *f);
    void AddProperties();
    bool OnClose();
    void OnHelp();
};

// Transpose
class tTransposeDlg : public tPropertyListDlg
{
  public:

    static int  Notes;        // 0
    static bool FitIntoScale;
    static int Scale;

    JZEventFrame *EventWin;
    tFilter *Filter;
    JZSong   *Song;

    //tNamedChoice ScaleDlg;
    tTransposeDlg(JZEventFrame *w, tFilter *f);
    void AddProperties();
    bool OnClose();
    void OnHelp();
};

// SetChannel
class tSetChannelDlg : public tPropertyListDlg
{
  public:

    static int  NewChannel;        // 0

    tFilter *Filter;
    JZSong   *Song;

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
    JZSong   *Song;

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
    JZSong     *Song;
    JZEventFrame *EventWin;

    tLengthDlg(JZEventFrame *win, tFilter *f);
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
    JZSong     *Song;
    JZEventFrame *EventWin;

    tSeqLengthDlg(JZEventFrame *win, tFilter *f);
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
    JZSong     *Song;
    JZEventFrame *EventWin;

    tMidiDelayDlg(JZEventFrame *win, tFilter *f);
    void AddProperties();
    bool OnClose();
    void OnHelp();
};

class tDeleteDlg : public tPropertyListDlg
{
  tFilter *Filter;
  JZEventFrame *EventWin;

 public:
  static bool LeaveSpace;        // 1

  tDeleteDlg(JZEventFrame *w, tFilter *f);
  void AddProperties();
  bool OnClose();
  void OnHelp();
};


class tSnapDlg : public tPropertyListDlg
{
  public:

    tSnapDlg(JZPianoFrame *w, int* snapptr);
    void AddProperties();
    //tNamedChoice Steps;
    bool OnClose();
    void OnHelp();

  private:

    JZPianoFrame* win;
    int* ptr;
};

class tQuantizeDlg : public tPropertyListDlg
{
  public:

    static bool NoteStart;  // 1
    static bool NoteLength; // 0
    static int  QntStep;    // 1/16
    static int  Groove;     // -x .. +x
    static int  Delay;      // -x .. +x

    tFilter *Filter;
    JZSong   *Song;
    JZEventFrame *EventWin;

    long Quantize(long);

    tQuantizeDlg(JZEventFrame *w, tFilter *f);
    void AddProperties();
    //tNamedChoice Steps;
    bool OnClose();
    void OnHelp();
};

void EventDialog(
  JZEvent*,
  JZPianoFrame*,
  tTrack*,
  long Clock,
  int Channel,
  int Pitch);

#endif // !defined(JZ_DIALOGS_H)
